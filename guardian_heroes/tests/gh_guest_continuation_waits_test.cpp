#include "gh_guest_continuation_title.h"
#include "gh_guest_continuation_waits.h"
#include "gh_kernel.h"
#include "gh_system_kernel_state.h"
#include "gh_system_runtime.h"
#include "gh_system_thread_state.h"
#include "gh_system_xevent.h"
#include "gh_system_xsemaphore.h"
#include "gh_system_xthread.h"
#include "native_memory.h"
#include <rex/chrono/clock.h>
#include <rex/kernel/xboxkrnl/threading.h>
#include <rex/stream.h>
#include <array>
#include <atomic>
#include <cassert>
#include <cstdio>
using namespace rex::system;
using rex::X_STATUS;
using namespace std::chrono_literals;
namespace cont = gh::guest::continuation;
namespace {
constexpr uint32_t entry = 0x82000000, single = entry + 0x20,
    multiple = entry + 0x30, signal_wait = entry + 0x40, delay = entry + 0x50,
    apc = entry + 0x60;
uint32_t slots = 0, handle_list = 0, interval = 0;
std::array<rex::X_HANDLE, 5> events{};
rex::X_HANDLE semaphore = 0, signal_semaphore = 0;
std::atomic<unsigned> apcs = 0;
std::atomic<unsigned> entered = 0;
void Original(GuestContext&, uint8_t*) { assert(false && "continuation bypassed"); }
void Apc(GuestContext& context, uint8_t*) {
  assert(context.r3.u32 == 0x1234 && context.r4.u32 == 0x5678 && context.r5.u32 == 0x9ABC);
  ++apcs;
}
cont::Transition Run(GuestContext& context, uint8_t* base, cont::Frame& frame) {
  const GuestMemory memory(base);
  if (frame.pc == entry) {
    const auto index = context.r3.u32;
    context.r20.u32 = index;
    memory.Write32(slots + index * 8, 1);
    ++entered;
    if (index == 1) {
      context.r3.u32 = 2; context.r4.u32 = handle_list; context.r5.u32 = 0;
      context.r6.u32 = 0; context.r7.u32 = 0; context.r8.u32 = 0;
      return cont::Transition::Call(multiple, entry + 4);
    }
    if (index == 2) {
      context.r3.u32 = signal_semaphore; context.r4.u32 = events[index];
      context.r5.u32 = 0; context.r6.u32 = 0; context.r7.u32 = 0;
      return cont::Transition::Call(signal_wait, entry + 4);
    }
    if (index == 3) {
      context.r3.u32 = 0; context.r4.u32 = 0; context.r5.u32 = interval;
      return cont::Transition::Call(delay, entry + 4);
    }
    context.r3.u32 = events[index]; context.r4.u32 = 0;
    context.r5.u32 = index == 4; context.r6.u32 = 0;
    return cont::Transition::Call(single, entry + 4);
  }
  memory.Write32(slots + context.r20.u32 * 8 + 4, context.r3.u32);
  memory.Write32(slots + context.r20.u32 * 8, 2);
  return cont::Transition::Return();
}
void Finished(const object_ref<XThread>& thread) {
  uint64_t timeout = uint64_t(int64_t(-20'000'000));
  assert(thread->Wait(0, 0, 0, &timeout) == X_STATUS_SUCCESS);
  thread->thread()->Join();
}
}
int main() {
  rex::Runtime runtime({}); rex::RuntimeConfig config; config.tool_mode = true;
  assert(runtime.Setup(std::move(config)) == X_STATUS_SUCCESS);
  auto* kernel = runtime.kernel_state(); auto* memory = runtime.memory();
  assert(runtime.function_dispatcher()->InitializeFunctionTable(entry, 0x100, entry, 0x10000, true));
  assert(runtime.function_dispatcher()->SetFunction(entry, Original));
  assert(runtime.function_dispatcher()->SetFunction(apc, Apc));
  cont::Program program;
  program.AddCompiled(entry, {entry, entry + 4}, Run);
  program.AddQueuedHost(single, cont::BeginNtWaitSingle);
  program.AddQueuedHost(multiple, cont::BeginNtWaitMultiple);
  program.AddQueuedHost(signal_wait, cont::BeginSignalAndWait);
  program.AddQueuedHost(delay, cont::BeginKeDelay);
  cont::InstallTitleProgram(&program);
  slots = memory->SystemHeapAlloc(64); memory->Fill(slots, 64, 0);
  handle_list = slots + 40; interval = slots + 48;
  const GuestMemory guest(memory->virtual_membase());
  guest.Write64(interval, uint64_t(int64_t(-36'000'000'000ll)));
  std::array<object_ref<XEvent>, 5> old_events;
  for (size_t i = 0; i < events.size(); ++i) {
    old_events[i] = make_object<XEvent>(kernel); old_events[i]->Initialize(false, false);
    events[i] = old_events[i]->handle();
  }
  auto old_semaphore = make_object<XSemaphore>(kernel); assert(old_semaphore->Initialize(1, 3));
  auto old_signal = make_object<XSemaphore>(kernel); assert(old_signal->Initialize(0, 3));
  semaphore = old_semaphore->handle(); signal_semaphore = old_signal->handle();
  guest.Write32(handle_list, semaphore); guest.Write32(handle_list + 4, events[1]);
  std::array<object_ref<XThread>, 5> old_threads;
  std::array<rex::X_HANDLE, 5> threads;
  for (uint32_t i = 0; i < threads.size(); ++i) {
    old_threads[i] = make_object<XThread>(kernel, 16 * 1024, 0, entry, i, 0, true);
    threads[i] = old_threads[i]->handle();
    assert(old_threads[i]->Create() == X_STATUS_SUCCESS);
  }
  const auto startup_deadline = std::chrono::steady_clock::now() + 2s;
  while (entered != threads.size() && std::chrono::steady_clock::now() < startup_deadline)
    std::this_thread::yield();
  assert(entered == threads.size());
  gh::guest::continuation::TitleCoordinator::PauseResult paused;
  // Wait until every Machine is parked in a queued import, not its entry step.
  for (unsigned attempt = 0; attempt < 200; ++attempt) {
    paused = kernel->PauseGuestContinuations(2s);
    if (!paused.stop) std::fprintf(stderr, "queued import pause failed: %s\n", paused.reason.c_str());
    assert(paused.stop);
    bool ready = paused.stop->threads().size() == threads.size();
    for (const auto& image : paused.stop->threads()) {
      cont::Machine machine(program); machine.Restore(image.execution);
      ready &= machine.frames().size() == 2;
    }
    if (ready) break;
    paused.stop.reset(); std::this_thread::yield();
  }
  assert(paused.stop && paused.stop->threads().size() == threads.size());
  for (const auto& image : paused.stop->threads()) {
    cont::Machine machine(program); machine.Restore(image.execution); assert(machine.frames().size() == 2);
  }
  // A producer inspecting a parked alertable wait must not borrow its PCR or
  // stall the global wait dispatcher behind that thread's guest APC lock.
  auto* alert_thread = old_threads[4]->guest_object<X_KTHREAD>();
  auto* alert_pcr = memory->TranslateVirtual<X_KPCR*>(
      old_threads[4]->thread_state()->context()->r13.u32);
  auto lock_owner = std::atomic_ref<uint32_t>(alert_thread->apc_lock.prcb_of_owner.value);
  assert(alert_pcr->current_irql == 0 && lock_owner.load() == 0);
  lock_owner.store(0x12345678);
  std::atomic<bool> producer_finished = false;
  std::thread producer([&] {
    gh::kernel::NotifyWaiters();
    producer_finished = true;
  });
  const auto producer_deadline = std::chrono::steady_clock::now() + 200ms;
  while (!producer_finished && std::chrono::steady_clock::now() < producer_deadline)
    std::this_thread::yield();
  const bool inspection_returned = producer_finished;
  const auto inspected_irql = alert_pcr->current_irql;
  const auto inspected_owner = lock_owner.load();
  lock_owner.store(0);
  producer.join();
  assert(inspection_returned && inspected_irql == 0 && inspected_owner == 0x12345678);
  bool in_place_rejected = false;
  try { paused.stop->Restore(paused.stop->threads()); }
  catch (const std::invalid_argument&) { in_place_rejected = true; }
  assert(in_place_rejected);
  assert(kernel->object_table()->ReleaseHandle(events[0]) == X_STATUS_SUCCESS);
  const auto retained_handle = old_events[0]->handle();
  kernel->PauseTimersForSnapshot(); assert(rex::chrono::Clock::PauseGuestClock());
  rex::chrono::Clock::GuestState clock; assert(rex::chrono::Clock::CaptureGuestState(clock));
  std::vector<uint8_t> kernel_bytes(1 << 20), memory_bytes(64 << 20);
  rex::stream::ByteStream ks(kernel_bytes.data(), kernel_bytes.size()), ms(memory_bytes.data(), memory_bytes.size());
  assert(kernel->Save(&ks, *paused.stop)); assert(memory->Save(&ms));
  paused.stop.reset(); kernel->TerminateTitle();
  old_threads = {}; old_events = {}; old_semaphore.reset(); old_signal.reset();
  ms.set_offset(0); assert(memory->Restore(&ms)); ks.set_offset(0); assert(kernel->Restore(&ks, false));
  assert(rex::chrono::Clock::RestoreGuestState(clock));
  assert(!kernel->object_table()->LookupObject<XEvent>(events[0]));
  assert(kernel->PrepareRestoredContinuations());
  std::array<object_ref<XThread>, 5> resumed;
  for (size_t i = 0; i < threads.size(); ++i) resumed[i] = kernel->object_table()->LookupObject<XThread>(threads[i]);
  auto restored_semaphore = kernel->object_table()->LookupObject<XSemaphore>(semaphore);
  auto restored_signal = kernel->object_table()->LookupObject<XSemaphore>(signal_semaphore);
  assert(static_cast<XObject*>(restored_semaphore.get())->GetKernelWaitable());
  auto* sem = static_cast<gh::kernel::Semaphore*>(static_cast<XObject*>(restored_semaphore.get())->GetKernelWaitable());
  auto* sig = static_cast<gh::kernel::Semaphore*>(static_cast<XObject*>(restored_signal.get())->GetKernelWaitable());
  assert(sem->count() == 1 && sig->count() == 1);
  // The original guest handle was closed, but the anonymous wait reference was saved.
  auto retained = kernel->object_table()->LookupObject<XEvent>(retained_handle);
  assert(retained); retained->Pulse(0, false);
  auto gate = kernel->object_table()->LookupObject<XEvent>(events[1]); gate->Pulse(0, false);
  auto target = kernel->object_table()->LookupObject<XEvent>(events[2]); target->Pulse(0, false);
  assert(sem->count() == 0 && sig->count() == 1);
  // Direct KeInsertQueueApc must wake an indefinite native alertable wait too,
  // without depending on the NtQueueApcThread host-callback hint.
  gh::kernel::Event native_event(false);
  gh::kernel::Waitable* native_objects[] = {&native_event};
  std::atomic<bool> native_waiting = false, native_finished = false;
  gh::kernel::WaitResult native_result = gh::kernel::WaitResult::Failed;
  std::thread native_waiter([&] {
    unsigned checks = 0;
    native_result = gh::kernel::WaitMultiple(native_objects, false,
        std::chrono::nanoseconds::max(), [&] {
          if (++checks == 2) native_waiting = true;
          return resumed[4]->HasDeliverableAPCs();
        }).first;
    native_finished = true;
  });
  while (!native_waiting) std::this_thread::yield();
  // Reset takes wait_mutex without notifying it, after the native waiter has
  // released that mutex inside its indefinite condition-variable wait.
  native_event.Reset();
  const auto apc_address = memory->SystemHeapAlloc(XAPC::kSize);
  auto* queued_apc = memory->TranslateVirtual<XAPC*>(apc_address);
  rex::kernel::xboxkrnl::xeKeInitializeApc(queued_apc, resumed[4]->guest_object(),
      XAPC::kDummyKernelRoutine, XAPC::kDummyRundownRoutine, apc, 1, 0x1234);
  assert(rex::kernel::xboxkrnl::xeKeInsertQueueApc(queued_apc, 0x5678, 0x9ABC, 0,
      resumed[4]->thread_state()->context()));
  const auto native_deadline = std::chrono::steady_clock::now() + 200ms;
  while (!native_finished && std::chrono::steady_clock::now() < native_deadline)
    std::this_thread::yield();
  const bool native_woke = native_finished;
  native_event.Set();
  native_waiter.join();
  assert(native_woke && native_result == gh::kernel::WaitResult::Alerted);
  clock.tick_count += clock.frequency * 3601;
  assert(rex::chrono::Clock::RestoreGuestState(clock));
  rex::chrono::Clock::ResumeGuestClock(); kernel->ResumeTimersAfterSnapshot();
  assert(kernel->ResumeRestoredContinuations());
  for (const auto& thread : resumed) Finished(thread);
  for (unsigned i = 0; i < threads.size(); ++i) {
    assert(guest.Read32(slots + i * 8) == 2);
    assert(guest.Read32(slots + i * 8 + 4) == (i == 4 ? X_STATUS_USER_APC : X_STATUS_SUCCESS));
  }
  assert(apcs == 1 && sig->count() == 1);
  cont::InstallTitleProgram(nullptr);
  std::puts("Queued guest imports: read-only foreign APC inspection, direct-insert indefinite native wake, fresh native owners, closed original handle, Pulse/WaitAll, exactly-once signal, guest-time delay and alertable APC restore passed");
}
