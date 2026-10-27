#include "gh_guest_continuation_title.h"
#include "gh_kernel.h"
#include "native_memory.h"
#include "gh_system_kernel_state.h"
#include "gh_system_runtime.h"
#include "gh_system_user_module.h"
#include "gh_system_xevent.h"
#include "gh_system_xiocompletion.h"
#include "gh_system_xmutant.h"
#include "gh_system_xthread.h"
#include "gh_system_xtimer.h"
#include <rex/stream.h>
#include <rex/chrono/clock.h>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdio>

namespace rex::system {
struct KernelDispatchSnapshotTestAccess {
  static void Start(KernelState& kernel) { kernel.StartDispatchThread(); }
  static void Executable(KernelState& kernel, object_ref<UserModule> module) {
    kernel.executable_module_ = std::move(module);
  }
};
struct XTimerSnapshotTestAccess {
  static gh::kernel::Timer::State State(XTimer& timer) {
    timer.UpdatePausedDeadline();
    return timer.kernel_timer_->CapturePausedState();
  }
  static XThread* Owner(XTimer& timer) { return timer.callback_thread_.get(); }
  static uint32_t Routine(XTimer& timer) { return timer.routine_; }
  static uint32_t Argument(XTimer& timer) { return timer.routine_arg_; }
  static void ResetSignal(XTimer& timer) { timer.kernel_timer_event_->Reset(); }
};
}

using namespace rex::system;
using rex::X_STATUS;
using namespace std::chrono_literals;
namespace cont = gh::guest::continuation;
namespace {
constexpr uint32_t entry = 0x82000000;
uint32_t slots = 0;
std::array<rex::X_HANDLE, 2> mutants{};
rex::X_HANDLE timer_handle = 0;
std::atomic<unsigned> entered{0};
gh::kernel::Event both_entered(true);
gh::kernel::Event lifetime_finished(true);
std::atomic<bool> lifetime_destroyed{false};
std::atomic<bool> enqueue_deferred{false};
std::atomic<unsigned> deferred_runs{0};
std::atomic<unsigned> host_callback_runs{0};
gh::kernel::Event deferred_queued(true), deferred_completed(true);
void Original(GuestContext&, uint8_t*) { assert(false && "continuation bypassed"); }
void HostCallback(GuestContext& context, uint8_t*) {
  assert(XThread::IsInThread() && !XThread::GetCurrentThread()->is_guest_thread());
  assert(!cont::CurrentTitleExecution() && context.r3.u64 == 0x2468);
  context.r3.u64 = 0x1357;
  ++host_callback_runs;
}
cont::Transition HostContinuation(GuestContext&, uint8_t*, cont::Frame&) {
  assert(false && "host callback entered resumable guest execution");
  return cont::Transition::Return();
}
void PrestartApc(GuestContext& context, uint8_t* base) {
  assert(context.r3.u32 == 0xA55A && context.r4.u32 == 0x1122 && context.r5.u32 == 0x3344);
  GuestMemory(base).Write32(slots + 64, 1);
}
cont::Transition CreatedStartup(GuestContext& context, uint8_t* base, cont::Frame&) {
  assert(context.r3.u32 == entry + 0x24 && context.r4.u32 == 0x2468);
  assert(context.r24.u64 == 0x1122334455667788ull);
  uint32_t tls;
  assert(XThread::GetCurrentThread()->GetTLSValue(3, &tls) && tls == 0xCAFEBABE);
  assert(GuestMemory(base).Read32(slots + 64) == 1);
  GuestMemory(base).Write32(slots + 64, 2);
  return cont::Transition::Return();
}

cont::Transition Count(GuestContext& context, uint8_t* base, cont::Frame& frame) {
  auto* self = XThread::GetCurrentThread();
  assert(self->thread_state()->context() == &context);
  const GuestMemory memory(base);
  const auto index = (static_cast<uint32_t>(context.r3.u64) - slots) / 32;
  assert(index < 2);
  if (index == 0 && enqueue_deferred.exchange(false)) {
    self->kernel_state()->CompleteOverlappedDeferred(
        [] { ++deferred_runs; }, slots + 80, rex::X_RESULT(0), {},
        [] { deferred_completed.Set(); });
    deferred_queued.Set();
  }
  auto mutant = self->kernel_state()->object_table()->LookupObject<XMutant>(mutants[index]);
  assert(mutant);
  uint64_t poll = 0;
  if (frame.pc == entry) {
    if (!index) {
      auto timer = self->kernel_state()->object_table()->LookupObject<XTimer>(timer_handle);
      assert(timer && timer->SetTimer(-36'000'000'000ll, 250, entry + 0x80,
                                      0x13579BDF, false) == X_STATUS_SUCCESS);
    }
    assert(mutant->Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);
    if (index == 0) {
      assert(mutant->Wait(0, 0, 0, &poll) == X_STATUS_SUCCESS);
    } else {
      mutant->AbandonOwnedBy(self);
    }
    context.r20.u64 = 100 + index;
    if ((entered.fetch_or(1u << index) | (1u << index)) == 3) both_entered.Set();
    return cont::Transition::Next(entry + 4);
  }
  if (memory.Read32(slots + index * 32)) {
    if (index == 1)
      assert(mutant->Wait(0, 0, 0, &poll) == X_STATUS_ABANDONED_WAIT_0);
    assert(mutant->ReleaseMutant(0, false, false) == X_STATUS_SUCCESS);
    if (index == 0)
      assert(mutant->ReleaseMutant(0, false, false) == X_STATUS_SUCCESS);
    assert(mutant->ReleaseMutant(0, false, false) == X_STATUS_MUTANT_NOT_OWNED);
    memory.Write64(slots + index * 32 + 8, context.r20.u64);
    return cont::Transition::Return();
  }
  ++context.r20.u64;
  return cont::Transition::Next(entry + 4);
}

class FixtureModule final : public UserModule {
 public:
  explicit FixtureModule(KernelState* kernel, std::string path = "continuation-fixture.xex")
      : UserModule(kernel), fixture_path_(std::move(path)) {}
  const std::string& path() const override { return fixture_path_; }
  const std::string& name() const override { return fixture_path_; }
 private:
  const std::string fixture_path_;
};

class FixtureThread final : public XThread {
 public:
  using XThread::XThread;
  uint32_t scratch_address() const { return scratch_address_; }
};

cont::Transition ExitInsideMachine(GuestContext&, uint8_t*, cont::Frame&) {
  XThread::GetCurrentThread()->Exit(0);
  return cont::Transition::Return();
}
class LifetimeThread final : public XThread {
 public:
  explicit LifetimeThread(KernelState* kernel)
      : XThread(kernel, 16 * 1024, 0, entry + 0x10, 0, 1, true) {}
  ~LifetimeThread() override {
    // Exit released the last handle while Machine::Run was active. The
    // execution lifetime must defer destruction until its copy-back has ended.
    assert(!cont::CurrentTitleExecution());
    lifetime_destroyed = true;
  }
  void Execute() override {
    struct Completed {
      ~Completed() { lifetime_finished.Set(); }
    } completed;
    XThread::Execute();
  }
};

void WaitFinished(const object_ref<XThread>& thread) {
  uint64_t timeout = static_cast<uint64_t>(int64_t(-20'000'000));
  assert(thread->Wait(0, 0, 0, &timeout) == X_STATUS_SUCCESS);
  thread->thread()->Join();
  assert(!thread->is_alive());
}
}

int main() {
  rex::Runtime runtime({});
  rex::RuntimeConfig config;
  config.tool_mode = true;
  assert(runtime.Setup(std::move(config)) == X_STATUS_SUCCESS);
  auto* kernel = runtime.kernel_state();
  auto* memory = runtime.memory();
  KernelDispatchSnapshotTestAccess::Start(*kernel);
  assert(runtime.function_dispatcher()->InitializeFunctionTable(entry, 0x100, entry, 0x10000, true));
  assert(runtime.function_dispatcher()->SetFunction(entry, Original));
  assert(runtime.function_dispatcher()->SetFunction(entry + 0x10, Original));
  assert(runtime.function_dispatcher()->SetFunction(entry + 0x20, Original));
  assert(runtime.function_dispatcher()->SetFunction(entry + 0x30, PrestartApc));
  assert(runtime.function_dispatcher()->SetFunction(entry + 0x40, HostCallback));
  cont::Program program;
  program.AddCompiled(entry, {entry, entry + 4}, Count);
  program.AddCompiled(entry + 0x10, {entry + 0x10}, ExitInsideMachine);
  program.AddCompiled(entry + 0x20, {entry + 0x20}, CreatedStartup);
  program.AddAtomicHost(entry + 0x30, PrestartApc);
  program.AddCompiled(entry + 0x40, {entry + 0x40}, HostContinuation);
  cont::InstallTitleProgram(&program);
  auto host_callback = make_object<XHostThread>(kernel, 16 * 1024, 0, [&] {
    auto* thread_state = XThread::GetCurrentThread()->thread_state();
    const auto stack = thread_state->context()->r1.u64;
    const auto lr = thread_state->context()->lr;
    uint64_t args[] = {0x2468};
    assert(runtime.function_dispatcher()->Execute(thread_state, entry + 0x40, args, 1) == 0x1357);
    assert(thread_state->context()->r1.u64 == stack && thread_state->context()->lr == lr);
    return 0;
  });
  assert(host_callback->Create() == X_STATUS_SUCCESS);
  WaitFinished(host_callback);
  assert(host_callback_runs == 1);
  host_callback->ReleaseHandle();
  host_callback.reset();
  slots = memory->SystemHeapAlloc(128);
  memory->Fill(slots, 128, 0);
  auto module = make_object<FixtureModule>(kernel);
  assert(kernel->RegisterUserModule(module));
  auto* loader = memory->TranslateVirtual<X_LDR_DATA_TABLE_ENTRY*>(module->hmodule_ptr());
  loader->flags = 0x12345678;
  std::array<object_ref<XMutant>, 2> old_mutants;
  std::array<object_ref<XThread>, 2> old_threads;
  std::array<rex::X_HANDLE, 2> handles{};
  std::array<uint32_t, 2> scratch{};
  auto old_timer = make_object<XTimer>(kernel);
  old_timer->Initialize(0);
  timer_handle = old_timer->handle();
  auto old_created = make_object<FixtureThread>(kernel, 16 * 1024, entry + 0x20,
      entry + 0x24, 0x2468, X_CREATE_SUSPENDED, true);
  assert(old_created->Create() == X_STATUS_SUCCESS);
  assert(old_created->Suspend() == X_STATUS_SUCCESS && old_created->suspend_count() == 2);
  const auto created_handle = old_created->handle();
  assert(old_created->SetTLSValue(3, 0xCAFEBABE));
  old_created->thread_state()->context()->r24.u64 = 0x1122334455667788ull;
  old_created->EnqueueApc(entry + 0x30, 0xA55A, 0x1122, 0x3344);
  const auto allocation_size = [&](uint32_t address) {
    uint32_t size = 0;
    assert(memory->LookupHeap(address)->QuerySize(address, &size));
    return size;
  };
  for (unsigned index = 0; index < 2; ++index) {
    old_mutants[index] = make_object<XMutant>(kernel);
    old_mutants[index]->Initialize(false);
    mutants[index] = old_mutants[index]->handle();
    auto thread = make_object<FixtureThread>(kernel, 16 * 1024, 0, entry,
                                             slots + index * 32, 0, true);
    old_threads[index] = thread;
    old_threads[index]->set_name(index ? "continuation-second" : "continuation-first");
    handles[index] = old_threads[index]->handle();
    assert(old_threads[index]->Create() == X_STATUS_SUCCESS);
    scratch[index] = thread->scratch_address();
    assert(scratch[index] && allocation_size(scratch[index]));
  }
  assert(both_entered.Wait(2s) == gh::kernel::WaitResult::Signaled);
  assert(kernel->PauseDispatchForSnapshot(2s));
  enqueue_deferred = true;
  assert(deferred_queued.Wait(2s) == gh::kernel::WaitResult::Signaled);
  assert(deferred_runs == 0 && !kernel->PauseDispatchForSnapshot(0ms));
  kernel->ResumeDispatchAfterSnapshot();
  assert(deferred_completed.Wait(2s) == gh::kernel::WaitResult::Signaled);
  assert(kernel->PauseDispatchForSnapshot(2s) && deferred_runs == 1);
  kernel->ResumeDispatchAfterSnapshot();
  // LoadXexImage holds its primary image outside RegisterUserModule's list.
  auto executable = make_object<FixtureModule>(kernel, "continuation-primary.xex");
  KernelDispatchSnapshotTestAccess::Executable(*kernel, executable);
  assert(!kernel->GetModule(executable->path(), true));
  const auto executable_handle = executable->handle();
  const auto executable_hmodule = executable->hmodule_ptr();
  auto paused = kernel->PauseGuestContinuations(2s);
  assert(paused.stop && paused.stop->threads().size() == 2);
  rex::X_HANDLE duplicate = 0;
  assert(kernel->object_table()->DuplicateHandle(handles[0], &duplicate) == X_STATUS_SUCCESS);
  std::array<uint64_t, 2> saved{};
  for (const auto& image : paused.stop->threads()) {
    cont::Machine machine(program);
    machine.Restore(image.execution);
    saved[(machine.context.r3.u32 - slots) / 32] = machine.context.r20.u64;
  }
  std::vector<uint8_t> kernel_bytes(1 << 20), memory_bytes(64 << 20);
  rex::stream::ByteStream kernel_stream(kernel_bytes.data(), kernel_bytes.size());
  rex::stream::ByteStream memory_stream(memory_bytes.data(), memory_bytes.size());
  auto unsupported_io = make_object<XIOCompletion>(kernel);
  assert(!kernel->Save(&kernel_stream, *paused.stop) && kernel_stream.offset() == 0);
  unsupported_io->ReleaseHandle();
  unsupported_io.reset();
  kernel->PauseTimersForSnapshot();
  assert(rex::chrono::Clock::PauseGuestClock());
  const auto before_deadline = XTimerSnapshotTestAccess::State(*old_timer);
  auto due_timer = make_object<XTimer>(kernel);
  due_timer->Initialize(1);
  const auto due_timer_handle = due_timer->handle();
  assert(due_timer->SetTimer(-1'000'000, 20, 0, 0, false) == X_STATUS_SUCCESS);
  uint64_t timer_poll = 0;
  assert(due_timer->Wait(0, 0, 0, &timer_poll) == X_STATUS_TIMEOUT);
  rex::chrono::Clock::GuestState advanced_clock;
  assert(rex::chrono::Clock::CaptureGuestState(advanced_clock));
  advanced_clock.tick_count += advanced_clock.frequency;
  assert(rex::chrono::Clock::RestoreGuestState(advanced_clock));
  const auto timer_state = XTimerSnapshotTestAccess::State(*old_timer);
  assert(timer_state.armed && timer_state.remaining > 3590s && timer_state.period == 250ms);
  assert(before_deadline.remaining - timer_state.remaining == 1s);
  assert(kernel->Save(&kernel_stream, *paused.stop));
  assert(memory->Save(&memory_stream));
  const auto kernel_size = kernel_stream.offset(), memory_size = memory_stream.offset();
  auto later_event = make_object<XEvent>(kernel);
  later_event->Initialize(true, false);
  later_event->RestoreName("post-snapshot-event");
  const auto later_handle = later_event->handle();
  paused.stop.reset();
  kernel->TerminateTitle();
  for (unsigned index = 0; index < 2; ++index) {
    assert(!old_threads[index]->is_alive());
    assert(allocation_size(scratch[index]) == 0);
  }
  loader->flags = 0;
  memory_stream.set_offset(0);
  assert(memory->Restore(&memory_stream));
  assert(memory_stream.offset() == memory_size);
  for (const auto address : scratch) assert(allocation_size(address));
  const GuestMemory guest(memory->virtual_membase());
  for (unsigned index = 0; index < 2; ++index) guest.Write32(slots + index * 32, 1);
  kernel_stream.set_offset(0);
  assert(kernel->Restore(&kernel_stream, false));
  assert(kernel_stream.offset() == kernel_size);
  assert(!kernel->object_table()->LookupObject<XEvent>(later_handle));
  rex::X_HANDLE missing = 0;
  assert(kernel->object_table()->GetObjectByName("post-snapshot-event", &missing) ==
         X_STATUS_OBJECT_NAME_NOT_FOUND);
  assert(kernel->object_table()->ReleaseHandle(later_handle) == X_STATUS_INVALID_HANDLE);
  later_event.reset();
  assert(kernel->GetModule(module->path(), true).get() == module.get());
  assert(kernel->GetExecutableModule().get() == executable.get());
  assert(kernel->object_table()->LookupObject<XModule>(executable_handle).get() ==
         executable.get());
  assert(executable->hmodule_ptr() == executable_hmodule);
  KernelDispatchSnapshotTestAccess::Executable(*kernel, {});
  assert(loader->flags == 0x12345678);
  std::array<object_ref<XThread>, 2> resumed;
  for (unsigned index = 0; index < 2; ++index) {
    resumed[index] = kernel->object_table()->LookupObject<XThread>(handles[index]);
    assert(resumed[index] && resumed[index].get() != old_threads[index].get());
    assert(resumed[index]->thread() && !resumed[index]->is_running());
  }
  assert(kernel->object_table()->LookupObject<XThread>(duplicate).get() == resumed[0].get());
  auto restored_timer = kernel->object_table()->LookupObject<XTimer>(timer_handle);
  auto restored_created = kernel->object_table()->LookupObject<XThread>(created_handle);
  assert(restored_created && restored_created.get() != old_created.get());
  assert(restored_created->thread() != old_created->thread());
  assert(restored_created->is_created_suspended() && restored_created->suspend_count() == 2);
  old_created.reset();
  auto restored_due = kernel->object_table()->LookupObject<XTimer>(due_timer_handle);
  assert(restored_timer && restored_timer.get() != old_timer.get() && restored_due);
  const auto replayed_timer = XTimerSnapshotTestAccess::State(*restored_timer);
  assert(replayed_timer.armed && replayed_timer.remaining == timer_state.remaining &&
         replayed_timer.period == timer_state.period);
  assert(XTimerSnapshotTestAccess::Owner(*restored_timer) == resumed[0].get());
  assert(XTimerSnapshotTestAccess::Routine(*restored_timer) == entry + 0x80 &&
         XTimerSnapshotTestAccess::Argument(*restored_timer) == 0x13579BDF);
  assert(restored_due->Wait(0, 0, 0, &timer_poll) == X_STATUS_TIMEOUT);
  old_timer.reset();
  due_timer.reset();
  // Destroy the old wrappers after guest memory and new wrappers exist.
  // Their destructors must not release the restored TLS/PCR/guest stacks.
  old_threads = {};
  old_mutants = {};
  rex::chrono::Clock::ResumeGuestClock();
  kernel->ResumeTimersAfterSnapshot();
  uint64_t timer_timeout = static_cast<uint64_t>(int64_t(-20'000'000));
  assert(restored_due->Wait(0, 0, 0, &timer_timeout) == X_STATUS_SUCCESS);
  assert(restored_due->Wait(0, 0, 0, &timer_timeout) == X_STATUS_SUCCESS);
  restored_due->Cancel();
  assert(kernel->ResumeRestoredContinuations());
  for (unsigned index = 0; index < 2; ++index) {
    WaitFinished(resumed[index]);
    assert(guest.Read64(slots + index * 32 + 8) == saved[index]);
    assert(allocation_size(scratch[index]) == 0);
  }
  assert(kernel->object_table()->ReleaseHandle(handles[0]) == X_STATUS_SUCCESS);
  assert(kernel->object_table()->ReleaseHandle(duplicate) == X_STATUS_SUCCESS);
  kernel->PauseTimersForSnapshot();
  assert(rex::chrono::Clock::PauseGuestClock());
  assert(rex::chrono::Clock::CaptureGuestState(advanced_clock));
  advanced_clock.tick_count += advanced_clock.frequency * 3601;
  assert(rex::chrono::Clock::RestoreGuestState(advanced_clock));
  rex::chrono::Clock::ResumeGuestClock();
  kernel->ResumeTimersAfterSnapshot();
  // The owner has exited and has no handles or live PCR/TLS, but the timer
  // remains waitable and must not queue an APC onto that retired execution.
  assert(restored_timer->Wait(0, 0, 0, &timer_timeout) == X_STATUS_SUCCESS);
  kernel->PauseTimersForSnapshot();
  assert(rex::chrono::Clock::PauseGuestClock());
  std::array<uint8_t, 256> orphan_timer_bytes{};
  XTimerSnapshotTestAccess::ResetSignal(*restored_timer);
  rex::stream::ByteStream orphan_stream(orphan_timer_bytes.data(), orphan_timer_bytes.size());
  assert(restored_timer->Save(&orphan_stream));
  orphan_stream.set_offset(0);
  auto orphan_timer = XTimer::Restore(kernel, &orphan_stream);
  assert(orphan_timer && !XTimerSnapshotTestAccess::Owner(*orphan_timer));
  assert(XTimerSnapshotTestAccess::Routine(*orphan_timer) == entry + 0x80 &&
         XTimerSnapshotTestAccess::Argument(*orphan_timer) == 0x13579BDF);
  restored_timer->Cancel();
  restored_timer.reset();
  resumed[0].reset();
  rex::chrono::Clock::ResumeGuestClock();
  kernel->ResumeTimersAfterSnapshot();
  assert(orphan_timer->Wait(0, 0, 0, &timer_timeout) == X_STATUS_SUCCESS);
  orphan_timer->Cancel();
  uint32_t previous_suspend = 0;
  assert(restored_created->Resume(&previous_suspend) == X_STATUS_SUCCESS && previous_suspend == 2);
  assert(restored_created->is_created_suspended() && restored_created->suspend_count() == 1);
  assert(guest.Read32(slots + 64) == 0);
  assert(restored_created->Resume(&previous_suspend) == X_STATUS_SUCCESS && previous_suspend == 1);
  WaitFinished(restored_created);
  assert(guest.Read32(slots + 64) == 2);
  auto lifetime = make_object<LifetimeThread>(kernel);
  const auto lifetime_handle = lifetime->handle(), lifetime_id = lifetime->thread_id();
  assert(lifetime->Create() == X_STATUS_SUCCESS);
  auto* native = lifetime->thread();
  assert(kernel->object_table()->ReleaseHandle(lifetime_handle) == X_STATUS_SUCCESS);
  lifetime.reset();
  assert(native->Resume());
  assert(lifetime_finished.Wait(2s) == gh::kernel::WaitResult::Signaled);
  assert(lifetime_destroyed && !kernel->GetThreadByID(lifetime_id));
  assert(!kernel->object_table()->LookupObject<XThread>(lifetime_handle));
  // Teardown must wake a dispatch host parked at its snapshot boundary.
  assert(kernel->PauseDispatchForSnapshot(2s));
  cont::InstallTitleProgram(nullptr);
  std::printf("live kernel restore: two new host threads resumed exact saved counters; "
              "memory, handles, primary/auxiliary module reuse, retired storage, mutant recursion/abandonment "
              "scratch cleanup, deferred dispatch pause, timer deadline/period/APC owner, "
              "created-suspended startup/APC "
              "and last-handle execution unwind passed\n");
}
