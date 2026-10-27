/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <algorithm>
#include <atomic>
#include <cstring>
#include <limits>
#include <optional>

#include <fmt/format.h>

#include <rex/chrono/clock.h>
#include <rex/cvar.h>
#include <rex/dbg.h>
#include <rex/perf/counter.h>
#include <rex/literals.h>
#include <rex/logging.h>
#include <rex/math.h>
#include <rex/ppc/context.h>
#include <rex/platform/exceptions.h>
#include "gh_system_runtime.h"
#include <rex/stream.h>
#include "gh_system_kernel_state.h"
#include <rex/system/function_dispatcher.h>
#include "gh_system_thread_state.h"
#include "gh_system_user_module.h"
#include <rex/kernel/xboxkrnl/threading.h>
#include "gh_system_xevent.h"
#include "gh_system_xmutant.h"
#include "gh_system_xthread.h"

#include "gh_kernel.h"
#include "gh_kernel_guest.h"
#include "gh_threading.h"
#include "gh_guest_continuation_title.h"
#include "native_memory.h"
#include <rex/thread.h>
#include <rex/vec128.h>

REXCVAR_DEFINE_BOOL(ignore_thread_priorities, true, "Kernel",
                    "Ignores game-specified thread priorities");

REXCVAR_DEFINE_BOOL(ignore_thread_affinities, true, "Kernel",
                    "Ignores game-specified thread affinities");

namespace rex::system {

const uint32_t XAPC::kSize;
const uint32_t XAPC::kDummyKernelRoutine;
const uint32_t XAPC::kDummyRundownRoutine;

using namespace rex::literals;

uint32_t next_xthread_id_ = 0;

XThread::XThread(KernelState* kernel_state)
    : XObject(kernel_state, kObjectType),
      guest_thread_(true),
      kernel_thread_event_(std::make_unique<gh::kernel::Event>(true)) {}

XThread::XThread(KernelState* kernel_state, uint32_t stack_size, uint32_t xapi_thread_startup,
                 uint32_t start_address, uint32_t start_context, uint32_t creation_flags,
                 bool guest_thread, bool main_thread, uint32_t guest_process)
    : XObject(kernel_state, kObjectType),
      thread_id_(++next_xthread_id_),
      guest_thread_(guest_thread),
      main_thread_(main_thread),
      apc_lock_old_irql_(0),
      kernel_thread_event_(std::make_unique<gh::kernel::Event>(true)) {
  creation_params_.stack_size = stack_size;
  creation_params_.xapi_thread_startup = xapi_thread_startup;
  creation_params_.start_address = start_address;
  creation_params_.start_context = start_context;

  // top 8 bits = processor ID (or 0 for default)
  // bit 0 = 1 to create suspended
  creation_params_.creation_flags = creation_flags;
  creation_params_.guest_process = guest_process;

  // Adjust stack size - min of 16k.
  if (creation_params_.stack_size < 16 * 1024) {
    creation_params_.stack_size = 16 * 1024;
  }

  if (!guest_thread_) {
    host_object_ = true;
  }

  // The kernel does not take a reference. We must unregister in the dtor.
  kernel_state_->RegisterThread(this);
}

XThread::~XThread() {
  if (main_fiber_) {
    main_fiber_->Destroy();
    main_fiber_ = nullptr;
  }

  // Unregister first to prevent lookups while deleting.
  if (kernel_state_) {
    kernel_state_->UnregisterThread(this);
  }

  thread_.reset();

  if (kernel_state_) {
    kernel_state_->memory()->SystemHeapFree(scratch_address_);
    kernel_state_->memory()->SystemHeapFree(tls_static_address_);
    kernel_state_->memory()->SystemHeapFree(pcr_address_);
    FreeStack();
  }

  if (thread_) {
    // TODO(benvanik): platform kill
    REXSYS_ERROR("Thread disposed without exiting");
  }
}

gh::kernel::Waitable* XThread::GetKernelWaitable() { return kernel_thread_event_.get(); }

bool XThread::is_alive() const {
  return thread_ && !kernel_thread_event_->signaled();
}

bool XThread::is_created_suspended() const {
  return guest_thread_ && thread_ && !execution_started_.load() && is_alive() &&
      memory()->TranslateVirtual<const X_KTHREAD*>(guest_object())->suspend_count != 0;
}

void XThread::WakeForTitleTermination() {
  if (!is_alive()) return;
  // Resume native suspensions before taking a mutex the target may hold.
  uint32_t previous = 0;
  while (thread_->Resume(&previous) && previous > 1) {
  }
#if REX_PLATFORM_LINUX || REX_PLATFORM_MAC
  {
    std::lock_guard lock(suspend_mutex_);
    guest_object<X_KTHREAD>()->suspend_count = 0;
    suspend_cv_.notify_all();
  }
#else
  reinterpret_cast<std::atomic_uint8_t*>(
      &guest_object<X_KTHREAD>()->suspend_count)->store(0);
#endif
  if (is_alive()) thread_->QueueUserCallback([] {});
}

thread_local XThread* current_xthread_tls_ = nullptr;

namespace {

XThread* GetBoundCurrentXThread() {
  return current_xthread_tls_;
}

}  // namespace

bool XThread::IsInThread() {
  return GetBoundCurrentXThread() != nullptr;
}

bool XThread::IsInThread(XThread* other) {
  return GetBoundCurrentXThread() == other;
}

XThread* XThread::GetCurrentThread() {
  XThread* thread = GetBoundCurrentXThread();
  if (!thread) {
    assert_always("Attempting to use kernel stuff from a non-kernel thread");
  }
  return thread;
}

void XThread::CheckTitleTermination() {
  XThread* self = GetBoundCurrentXThread();
  if (self && self->is_guest_thread() && self->is_running() &&
      self->kernel_state()->is_terminating_title()) {
    // Unwind cleanly at this safe point. Does not return.
    self->Exit(0);
  }
}

bool XThread::CanPauseTitleContinuation() {
  auto* self = GetBoundCurrentXThread();
  if (!self || !self->is_guest_thread()) return false;
  auto* thread = self->guest_object<X_KTHREAD>();
  const auto* pcr = self->memory()->TranslateVirtual<X_KPCR*>(self->pcr_address_);
  // Import/native call stacks are already excluded by Machine::Run. These
  // guest states can span instructions and must finish before parking too.
  return !std::atomic_ref<int32_t>(thread->apc_disable_count).load() &&
      !thread->executing_kernel_apc &&
      !static_cast<uint32_t>(thread->fiber_ptr) && !pcr->current_irql &&
      !static_cast<uint32_t>(pcr->prcb_data.dpc_active) &&
      gh::thread::GlobalCriticalRegion::depth() == 0 &&
      !self->kernel_state_->critical_sections().IsOwnedBy(self->guest_object());
}

uint32_t XThread::GetCurrentThreadHandle() {
  XThread* thread = XThread::GetCurrentThread();
  return thread->handle();
}

uint32_t XThread::GetCurrentThreadId() {
  XThread* thread = XThread::GetCurrentThread();
  return thread->guest_object<X_KTHREAD>()->thread_id;
}

uint32_t XThread::GetLastError() {
  XThread* thread = XThread::GetCurrentThread();
  return thread->last_error();
}

void XThread::SetLastError(uint32_t error_code) {
  XThread* thread = XThread::GetCurrentThread();
  thread->set_last_error(error_code);
}

uint32_t XThread::last_error() {
  return guest_object<X_KTHREAD>()->last_error;
}

void XThread::set_last_error(uint32_t error_code) {
  guest_object<X_KTHREAD>()->last_error = error_code;
}

void XThread::set_name(const std::string_view name) {
  std::lock_guard<std::mutex> lock(thread_lock_);
  thread_name_ = fmt::format("{} ({:08X})", name, handle());

  if (thread_) {
    // May be getting set before the thread is created.
    // One the thread is ready it will handle it.
    thread_->set_name(thread_name_);
  }
}

static uint8_t next_cpu = 0;
static uint8_t GetFakeCpuNumber(uint8_t proc_mask) {
  // NOTE: proc_mask is logical processors, not physical processors or cores.
  if (!proc_mask) {
    next_cpu = (next_cpu + 1) % 6;
    return next_cpu;  // is this reasonable?
    // TODO(Triang3l): Does the following apply here?
    // https://docs.microsoft.com/en-us/windows/win32/dxtecharts/coding-for-multiple-cores
    // "On Xbox 360, you must explicitly assign software threads to a particular
    //  hardware thread by using XSetThreadProcessor. Otherwise, all child
    //  threads will stay on the same hardware thread as the parent."
  }
  assert_false(proc_mask & 0xC0);

  uint8_t cpu_number = 7 - rex::lzcnt(proc_mask);
  assert_true(1 << cpu_number == proc_mask);
  assert_true(cpu_number < 6);
  return cpu_number;
}

void XThread::InitializeGuestObject() {
  auto guest_thread = guest_object<X_KTHREAD>();
  uint32_t guest_ptr = guest_object();

  guest_thread->header.type = 6;  // ThreadObject
  guest_thread->suspend_count = (creation_params_.creation_flags & X_CREATE_SUSPENDED) ? 1 : 0;

  // Self-referencing pointers for wait timeout timer/block
  guest_thread->unk_10 = guest_ptr + 0x010;
  guest_thread->unk_14 = guest_ptr + 0x010;

  guest_thread->wait_timeout_block.wait_list_entry.flink_ptr = guest_ptr + 0x20;
  guest_thread->wait_timeout_block.wait_list_entry.blink_ptr = guest_ptr + 0x20;
  guest_thread->wait_timeout_block.thread = guest_ptr;
  guest_thread->wait_timeout_block.object = guest_ptr + 0x18;
  guest_thread->wait_timeout_block.wait_result_xstatus = 0x0100;
  guest_thread->wait_timeout_block.wait_type = 0x0201;

  guest_thread->stack_base = stack_base_;
  guest_thread->stack_limit = stack_limit_;
  guest_thread->stack_kernel = stack_base_ - 240;
  guest_thread->tls_address = tls_dynamic_address_;
  guest_thread->thread_state = 0;

  // Initialize APC lists (kernel + user mode)
  guest_thread->apc_lists[0].Initialize(memory());
  guest_thread->apc_lists[1].Initialize(memory());

  // Set process pointer - use guest_process if provided, else default to title process.
  uint32_t process_ptr = creation_params_.guest_process
                             ? creation_params_.guest_process
                             : kernel_state_->process_info_block_address();
  guest_thread->process = process_ptr;
  guest_thread->may_queue_apcs = 1;

  // Set PRCB pointers (derived from this thread's PCR).
  uint32_t kpcrb = pcr_address_ + offsetof(X_KPCR, prcb_data);
  guest_thread->a_prcb_ptr = kpcrb;
  guest_thread->another_prcb_ptr = kpcrb;

  // PPCContext for spinlock helpers (valid before thread runs; r13 set at construction).
  auto* ctx = thread_state_->context();

  // Set per-thread process type and link into process thread list.
  if (process_ptr) {
    auto target_process = memory()->TranslateVirtual<X_KPROCESS*>(process_ptr);
    guest_thread->process_type = target_process->process_type;
    guest_thread->process_type_dup = target_process->process_type;

    auto old_irql =
        kernel::xboxkrnl::xeKeKfAcquireSpinLock(ctx, &target_process->thread_list_spinlock);
    util::XeInsertTailList(&target_process->thread_list, &guest_thread->process_threads, memory());
    target_process->thread_count = target_process->thread_count + 1;
    kernel::xboxkrnl::xeKeKfReleaseSpinLock(ctx, &target_process->thread_list_spinlock, old_irql);
  } else {
    guest_thread->process_type = X_PROCTYPE_USER;
    guest_thread->process_type_dup = X_PROCTYPE_USER;
  }

  guest_thread->msr_mask = 0xFDFFD7FF;
  // current_cpu is expected to be initialized externally via SetActiveCpu.
  guest_thread->stack_alloc_base = stack_base_;
  guest_thread->create_time = chrono::Clock::QueryGuestSystemTime();

  // Initialize timer_list as self-referencing
  guest_thread->timer_list.flink_ptr = guest_ptr + offsetof(X_KTHREAD, timer_list);
  guest_thread->timer_list.blink_ptr = guest_ptr + offsetof(X_KTHREAD, timer_list);

  guest_thread->thread_id = thread_id_;
  guest_thread->start_address = creation_params_.start_address;

  // Initialize unk_154 list as self-referencing
  guest_thread->unk_154.flink_ptr = guest_ptr + offsetof(X_KTHREAD, unk_154);
  guest_thread->unk_154.blink_ptr = guest_ptr + offsetof(X_KTHREAD, unk_154);

  guest_thread->last_error = 0;
  guest_thread->creation_flags = creation_params_.creation_flags;
  guest_thread->unk_17C = 1;
}

bool XThread::AllocateStack(uint32_t size) {
  auto heap = memory()->LookupHeap(kStackAddressRangeBegin);

  auto alignment = heap->page_size();
  auto padding = heap->page_size() * 2;  // Guard page size * 2
  size = rex::round_up(size, alignment);
  auto actual_size = size + padding;

  uint32_t address = 0;
  if (!heap->AllocRange(kStackAddressRangeBegin, kStackAddressRangeEnd, actual_size, alignment,
                        memory::kMemoryAllocationReserve | memory::kMemoryAllocationCommit,
                        memory::kMemoryProtectRead | memory::kMemoryProtectWrite, false,
                        &address)) {
    return false;
  }

  stack_alloc_base_ = address;
  stack_alloc_size_ = actual_size;
  stack_limit_ = address + (padding / 2);
  stack_base_ = stack_limit_ + size;

  // Initialize the stack with junk
  memory()->Fill(stack_alloc_base_, actual_size, 0xBE);

  // Setup the guard pages
  heap->Protect(stack_alloc_base_, padding / 2, memory::kMemoryProtectNoAccess);
  heap->Protect(stack_base_, padding / 2, memory::kMemoryProtectNoAccess);

  return true;
}

void XThread::FreeStack() {
  if (stack_alloc_base_) {
    auto heap = memory()->LookupHeap(kStackAddressRangeBegin);
    heap->Release(stack_alloc_base_);

    stack_alloc_base_ = 0;
    stack_alloc_size_ = 0;
    stack_base_ = 0;
    stack_limit_ = 0;
  }
}

X_STATUS XThread::Create() {
  // Thread kernel object.
  if (!CreateNative<X_KTHREAD>()) {
    REXSYS_WARN("Unable to allocate thread object");
    return X_STATUS_NO_MEMORY;
  }

  // Allocate a stack.
  if (!AllocateStack(creation_params_.stack_size)) {
    return X_STATUS_NO_MEMORY;
  }

  // Allocate thread scratch.
  // This is used by interrupts/APCs/etc so we can round-trip pointers through.
  scratch_size_ = 4 * 16;
  scratch_address_ = memory()->SystemHeapAlloc(scratch_size_);

  // Allocate TLS block.
  // Games will specify a certain number of 4b slots that each thread will get.
  xex2_opt_tls_info* tls_header = nullptr;
  auto module = kernel_state_->GetExecutableModule();
  if (module) {
    module->GetOptHeader(XEX_HEADER_TLS_INFO, &tls_header);
  }

  const uint32_t kDefaultTlsSlotCount = 1024;
  uint32_t tls_slots = kDefaultTlsSlotCount;
  uint32_t tls_extended_size = 0;
  if (tls_header && tls_header->slot_count) {
    tls_slots = tls_header->slot_count;
    tls_extended_size = tls_header->data_size;
  }

  // Allocate both the slots and the extended data.
  // Some TLS is compiled with the binary (declspec(thread)) vars. The game
  // will directly access those through 0(r13).
  uint32_t tls_slot_size = tls_slots * 4;
  tls_total_size_ = tls_slot_size + tls_extended_size;
  tls_static_address_ = memory()->SystemHeapAlloc(tls_total_size_);
  tls_dynamic_address_ = tls_static_address_ + tls_extended_size;
  if (!tls_static_address_) {
    REXSYS_WARN("Unable to allocate thread local storage block");
    return X_STATUS_NO_MEMORY;
  }

  // Zero all of TLS.
  memory()->Fill(tls_static_address_, tls_total_size_, 0);
  if (tls_extended_size) {
    // If game has extended data, copy in the default values.
    assert_not_zero(tls_header->raw_data_address);
    memory()->Copy(tls_static_address_, tls_header->raw_data_address, tls_header->raw_data_size);
  }

  // Allocate thread state block from heap.
  // https://web.archive.org/web/20170704035330/https://www.microsoft.com/msj/archive/S2CE.aspx
  // This is set as r13 for user code and some special inlined Win32 calls
  // (like GetLastError/etc) will poke it directly.
  // We try to use it as our primary store of data just to keep things all
  // consistent.
  // 0x000: pointer to tls data
  // 0x100: pointer to TEB(?)
  // 0x10C: Current CPU(?)
  // 0x150: if >0 then error states don't get set (DPC active bool?)
  // TEB:
  // 0x14C: thread id
  // 0x160: last error
  // So, at offset 0x100 we have a 4b pointer to offset 200, then have the
  // structure.
  pcr_address_ = memory()->SystemHeapAlloc(0x2D8);
  if (!pcr_address_) {
    REXSYS_WARN("Unable to allocate thread state block");
    return X_STATUS_NO_MEMORY;
  }

  // Create thread state - needed for interrupt callbacks and kernel exports
  thread_state_ =
      std::make_unique<runtime::ThreadState>(thread_id_, stack_base_, pcr_address_, memory());

  REXSYS_NOISY_DEBUG("XThread{:08X} ({:X}) Stack: {:08X}-{:08X}", handle(), thread_id_,
                     stack_limit_, stack_base_);

  uint8_t cpu_index = GetFakeCpuNumber(static_cast<uint8_t>(creation_params_.creation_flags >> 24));

  // Initialize the KTHREAD object.
  InitializeGuestObject();

  X_KPCR* pcr = memory()->TranslateVirtual<X_KPCR*>(pcr_address_);

  pcr->tls_ptr = tls_static_address_;
  pcr->pcr_ptr = pcr_address_;
  pcr->prcb_data.current_thread = guest_object();
  pcr->prcb = pcr_address_ + offsetof(X_KPCR, prcb_data);
  pcr->host_stash = reinterpret_cast<uint64_t>(thread_state_->context());
  pcr->current_irql = 0;

  pcr->stack_base_ptr = stack_base_;
  pcr->stack_end_ptr = stack_limit_;

  pcr->prcb_data.dpc_active = 0;

  // Always retain when starting - the thread owns itself until exited.
  RetainHandle();

  if (!CreateNativeHostThread()) {
    REXSYS_ERROR("CreateThread failed");
    return X_STATUS_NO_MEMORY;
  }

  // Set the thread name based on host ID (for easier debugging).
  if (thread_name_.empty()) {
    set_name(fmt::format("XThread{:04X}", thread_->system_id()));
  }

  if (creation_params_.creation_flags & 0x60) {
    thread_->set_priority(creation_params_.creation_flags & 0x20 ? 1 : 0);
  }

  // Assign the newly created thread to the logical processor, and also set up
  // the current CPU in KPCR and KTHREAD.
  SetActiveCpu(cpu_index);

  // TODO(tomc): do we need thread notifications (related to processor thread management)?

  if ((creation_params_.creation_flags & X_CREATE_SUSPENDED) == 0) {
    // Start the thread now that we're all setup.
    thread_->Resume();
  }

  return X_STATUS_SUCCESS;
}

bool XThread::CreateNativeHostThread() {
  gh::kernel::HostThread::CreationParameters params;
  params.stack_size = 16_MiB;  // Allocate a big host stack.
  params.create_suspended = true;
  thread_ = gh::kernel::HostThread::Create(params, [this]() {
    rex::initialize_seh_thread();
    runtime::ThreadState::Bind(thread_state_.get());

    // Set thread ID override. This is used by logging.
    rex::thread::set_current_thread_id(handle());

    // Set name immediately, if we have one.
    thread_->set_name(thread_name_);

    PROFILE_THREAD_ENTER(thread_name_.c_str());
    PROFILE_THREAD_CREATED();

    // Execute user code.
    current_xthread_tls_ = this;
    gh::kernel::SetThreadLocalValue(handle());
    execution_started_ = true;
    running_ = true;
    CheckTitleTermination();
    Execute();
    Exit(0);
  });

  return bool(thread_);
}

X_STATUS XThread::Exit(int exit_code) {
  // This may only be called on the thread itself.
  assert_true(XThread::GetCurrentThread() == this);
  // Keep the object alive until HostThread::Exit() begins native stack unwind.
  // Otherwise ReleaseHandle() below may delete `this`
  // while this method is still running.
  auto self = retain_object(this);

  // Mark as terminated before running down APCs.
  auto kthread = guest_object<X_KTHREAD>();
  kthread->terminated = 1;

  // TODO(benvanik): dispatch events? waiters? etc?
  RundownAPCs();

  // Set exit code.
  X_KTHREAD* thread = guest_object<X_KTHREAD>();
  thread->header.signal_state = 1;
  thread->exit_status = exit_code;

  // Unlink thread from process thread list.
  uint32_t process_guest = thread->process;
  if (process_guest) {
    auto* ctx = thread_state_->context();
    auto kprocess = memory()->TranslateVirtual<X_KPROCESS*>(process_guest);
    auto old_irql = kernel::xboxkrnl::xeKeKfAcquireSpinLock(ctx, &kprocess->thread_list_spinlock);
    util::XeRemoveEntryList(&thread->process_threads, memory());
    kprocess->thread_count = kprocess->thread_count - 1;
    kernel::xboxkrnl::xeKeKfReleaseSpinLock(ctx, &kprocess->thread_list_spinlock, old_irql);
  }

  kernel_state_->AbandonMutantsOwnedBy(this);
  kernel_state_->OnThreadExit(this);

  // TODO(tomc): do we need thread notifications (related to processor thread management)?

  // Stack and APC scratch belong to execution, not the retained handle. No APC
  // delivery can return past this exit, and completed-thread snapshots need no
  // scratch allocation to retain or reconstruct.
  memory()->SystemHeapFree(scratch_address_);
  scratch_address_ = scratch_size_ = 0;
  FreeStack();

  // NOTE: unless PlatformExit fails, expect it to never return!
  current_xthread_tls_ = nullptr;
  gh::kernel::SetThreadLocalValue(0);
  PROFILE_THREAD_EXIT();

  running_ = false;
  kernel_thread_event_->Set();
  ReleaseHandle();

  // NOTE: this does not return!
  gh::kernel::HostThread::Exit(exit_code);
  return X_STATUS_SUCCESS;
}

X_STATUS XThread::Terminate(int exit_code) {
  // TODO(benvanik): inform the profiler that this thread is exiting.

  // Set exit code.
  X_KTHREAD* thread = guest_object<X_KTHREAD>();
  thread->header.signal_state = 1;
  thread->exit_status = exit_code;

  // TODO(tomc): do we need thread notifications (related to processor thread management)?

  running_ = false;
  if (XThread::IsInThread(this)) {
    // Same lifetime rule as Exit(): don't allow ReleaseHandle() to destroy
    // the thread object before HostThread::Exit() begins native stack unwind.
    auto self = retain_object(this);
    kernel_state_->AbandonMutantsOwnedBy(this);
    memory()->SystemHeapFree(scratch_address_);
    scratch_address_ = scratch_size_ = 0;
    FreeStack();
    ReleaseHandle();
    kernel_thread_event_->Set();
    gh::kernel::HostThread::Exit(exit_code);
  } else {
    thread_->Terminate(exit_code);
    memory()->SystemHeapFree(scratch_address_);
    scratch_address_ = scratch_size_ = 0;
    FreeStack();
    kernel_thread_event_->Set();
    ReleaseHandle();
  }

  return X_STATUS_SUCCESS;
}

void XThread::Execute() {
  // Exit unwinds through Machine/context-copy scopes after releasing the
  // execution handle. Keep ThreadState storage alive until those scopes finish.
  auto execution_lifetime = retain_object(this);
  std::optional<gh::guest::continuation::TitleCoordinator::Registration> continuation_thread;
  if (gh::guest::continuation::TitleProgram()) {
    continuation_thread.emplace(
        kernel_state_->title_continuations(), thread_id_, &XThread::CanPauseTitleContinuation);
  }
  REXSYS_NOISY_DEBUG("Execute thid {} (handle={:08X}, '{}', native={:08X})", thread_id_, handle(),
                     thread_name_, thread_->system_id());

  // Let the kernel know we are starting.
  kernel_state_->OnThreadExecute(this);

  if (!restored_execution_.empty()) {
    // Guest TLS/PCR/stack and pending APC lists came from the same memory image.
    // Do not replay thread startup, arguments or creation-time APC delivery.
    auto execution = std::move(restored_execution_);
    main_fiber_ = rex::thread::Fiber::ConvertCurrentThread();
    auto* context = thread_state_->context();
    if (restored_machine_) {
      auto machine = std::move(restored_machine_);
      gh::guest::continuation::ResumeTitle(*machine, *context,
          memory()->virtual_membase(), &XThread::CheckTitleTermination);
    } else {
      gh::guest::continuation::ResumeTitle(*gh::guest::continuation::TitleProgram(), *context,
          memory()->virtual_membase(), execution, &XThread::CheckTitleTermination);
    }
    Exit(creation_params_.xapi_thread_startup ? 0 : static_cast<int>(context->r3.u32));
  }

  // Dispatch any APCs that were queued before the thread was created first.
  DeliverAPCs();

  uint32_t address;
  std::vector<uint64_t> args;
  bool want_exit_code;
  int exit_code = 0;

  // If a XapiThreadStartup value is present, we use that as a trampoline.
  // Otherwise, we are a raw thread.
  if (creation_params_.xapi_thread_startup) {
    address = creation_params_.xapi_thread_startup;
    args.push_back(creation_params_.start_address);
    args.push_back(creation_params_.start_context);
    want_exit_code = false;
  } else {
    // Run user code.
    address = creation_params_.start_address;
    args.push_back(creation_params_.start_context);
    want_exit_code = true;
  }

  // NOTE(tomc): JIT execution replaced with direct function calls
  // In rexglue, guest code is compiled ahead of time and called directly.
  // The start_address points to a 32bit guest address, for which the function
  // dispatcher maintains a lookup table to retrieve the host function pointer.
  auto* runtime = Runtime::instance();
  if (!runtime || !runtime->function_dispatcher()) {
    REXSYS_ERROR("XThread::Execute - Runtime not initialized");
    return;
  }

  auto* dispatcher = runtime->function_dispatcher();
  auto* memory = runtime->memory();
  PPCFunc* func = dispatcher->GetFunction(address);
  if (!func) {
    REXSYS_ERROR("XThread::Execute - No function registered at {:08X}", address);
    return;
  }

  auto* ctx = thread_state_->context();
  uint8_t* base = memory->virtual_membase();

  // Pass arguments in r3, r4, ... per PPC calling convention
  if (args.size() > 0)
    ctx->r3.u64 = args[0];
  if (args.size() > 1)
    ctx->r4.u64 = args[1];
  if (args.size() > 2)
    ctx->r5.u64 = args[2];
  if (args.size() > 3)
    ctx->r6.u64 = args[3];
  if (args.size() > 4)
    ctx->r7.u64 = args[4];
  if (args.size() > 5)
    ctx->r8.u64 = args[5];
  if (args.size() > 6)
    ctx->r9.u64 = args[6];
  if (args.size() > 7)
    ctx->r10.u64 = args[7];

  ctx->fpscr.InitHost();

  // Convert this host thread to a fiber so SwitchTo works bidirectionally.
  // Required on Windows before any CreateFiber; provides the fallback handle
  // when another fiber switches back to the main execution context.
  main_fiber_ = rex::thread::Fiber::ConvertCurrentThread();

  // Execute the function
  REXSYS_NOISY_DEBUG("XThread::Execute - Calling function at {:08X}", address);
  if (!gh::guest::continuation::TryExecuteTitle(*ctx, base, address,
                                               &XThread::CheckTitleTermination)) {
    func(*ctx, base);
  }

  exit_code = static_cast<int>(ctx->r3.u32);

  // If we got here it means the execute completed without an exit being called.
  // Treat the return code as an implicit exit code (if desired).
  Exit(!want_exit_code ? 0 : exit_code);
}

void XThread::EnterCriticalRegion() {
  std::atomic_ref<int32_t>(guest_object<X_KTHREAD>()->apc_disable_count).fetch_sub(1);
}

void XThread::LeaveCriticalRegion() {
  auto kthread = guest_object<X_KTHREAD>();
  auto apc_disable_count = std::atomic_ref<int32_t>(kthread->apc_disable_count).fetch_add(1) + 1;
  if (apc_disable_count == 0) {
    // NOTE: intentionally not calling CheckApcs() here.
    // Delivering APCs here can cause them to fire in wrong contexts.
  }
}

void XThread::LockApc() {
  auto kthread = guest_object<X_KTHREAD>();
  apc_lock_old_irql_ =
      kernel::xboxkrnl::xeKeKfAcquireSpinLock(thread_state_->context(), &kthread->apc_lock);
}

void XThread::UnlockApc(bool queue_delivery) {
  auto kthread = guest_object<X_KTHREAD>();
  auto mem = memory();
  const GuestMemory guest_memory(mem->virtual_membase());
  bool needs_apc = gh::kernel::GuestApcQueue::Pending(guest_memory, guest_object(), 0) ||
                   gh::kernel::GuestApcQueue::Pending(guest_memory, guest_object(), 1);
  kernel::xboxkrnl::xeKeKfReleaseSpinLock(thread_state_->context(), &kthread->apc_lock,
                                          apc_lock_old_irql_);
  if (needs_apc && queue_delivery) {
    // Match Edge/Canary behavior: callback is only a wakeup hint.
    // User APC execution happens on alertable wait return paths.
    thread_->QueueUserCallback([]() {});
    gh::kernel::NotifyWaiters();
  }
}

void XThread::EnqueueApc(uint32_t normal_routine, uint32_t normal_context, uint32_t arg1,
                         uint32_t arg2) {
  uint32_t apc_ptr = memory()->SystemHeapAlloc(XAPC::kSize);
  if (!apc_ptr) {
    REXSYS_WARN("EnqueueApc: allocation failed (thid={}, normal={:08X})", thread_id_,
                normal_routine);
    return;
  }
  auto apc = memory()->TranslateVirtual<XAPC*>(apc_ptr);
  kernel::xboxkrnl::xeKeInitializeApc(apc, guest_object(), XAPC::kDummyKernelRoutine,
                                      XAPC::kDummyRundownRoutine, normal_routine,
                                      1 /* user apc mode */, normal_context);

  // Important: use the caller PPC context when queuing to another thread.
  // Using the target thread context here can corrupt APC lock/IRQL bookkeeping.
  PPCContext* queue_ctx =
      runtime::ThreadState::Get() ? runtime::current_ppc_context() : thread_state_->context();

  if (!kernel::xboxkrnl::xeKeInsertQueueApc(apc, arg1, arg2, 0, queue_ctx)) {
    memory()->SystemHeapFree(apc_ptr);
    REXSYS_ERROR(
        "EnqueueApc: queue rejected (thid={}, normal={:08X}, ctx={:08X}, arg1={:08X}, "
        "arg2={:08X})",
        thread_id_, normal_routine, normal_context, arg1, arg2);
    return;
  }
  // Match Edge/Canary behavior: callback is only a wakeup hint.
  // APCs are delivered via alertable wait handling.
  thread_->QueueUserCallback([]() {});
}

bool XThread::HasDeliverableAPCs() {
  // Producers scan queued waits under wait_mutex, even while this guest is parked.
  // Observing readiness must neither borrow its PCR nor serialize a guest lock.
  return gh::kernel::GuestApcQueue::Deliverable(
      GuestMemory(memory()->virtual_membase()), guest_object());
}

void XThread::DeliverAPCs() {
  auto mem = memory();
  auto* ctx = thread_state_->context();
  auto kthread = guest_object<X_KTHREAD>();
  auto* dispatcher = kernel_state_->function_dispatcher();

  auto old_irql = kernel::xboxkrnl::xeKeKfAcquireSpinLock(ctx, &kthread->apc_lock);
  const GuestMemory guest_memory(mem->virtual_membase());

  while (gh::kernel::GuestApcQueue::Deliverable(guest_memory, guest_object())) {
    PERF_counter_inc(kApcQueueDepth);
    uint32_t apc_ptr = gh::kernel::GuestApcQueue::Pop(guest_memory, guest_object(), 1);
    XAPC* apc = mem->TranslateVirtual<XAPC*>(apc_ptr);
    bool needs_freeing = apc->kernel_routine != XAPC::kDummyKernelRoutine;

    kernel::xboxkrnl::xeKeKfReleaseSpinLock(ctx, &kthread->apc_lock, old_irql);

    uint8_t* scratch_ptr = mem->TranslateVirtual(scratch_address_);
    memory::store_and_swap<uint32_t>(scratch_ptr + 0, apc->normal_routine);
    memory::store_and_swap<uint32_t>(scratch_ptr + 4, apc->normal_context);
    memory::store_and_swap<uint32_t>(scratch_ptr + 8, apc->arg1);
    memory::store_and_swap<uint32_t>(scratch_ptr + 12, apc->arg2);

    if (apc->kernel_routine != XAPC::kDummyKernelRoutine) {
      if (dispatcher->GetFunction(apc->kernel_routine)) {
        uint64_t kernel_args[] = {apc_ptr, scratch_address_ + 0, scratch_address_ + 4,
                                  scratch_address_ + 8, scratch_address_ + 12};
        dispatcher->ExecuteTrap(thread_state_.get(), apc->kernel_routine, kernel_args,
                                rex::countof(kernel_args));
      } else {
        REXSYS_ERROR("DeliverAPCs: kernel_routine {:08X} not found", uint32_t(apc->kernel_routine));
      }
    } else {
      mem->SystemHeapFree(apc_ptr);
      needs_freeing = false;
    }

    uint32_t normal_routine = memory::load_and_swap<uint32_t>(scratch_ptr + 0);
    uint32_t normal_context = memory::load_and_swap<uint32_t>(scratch_ptr + 4);
    uint32_t arg1 = memory::load_and_swap<uint32_t>(scratch_ptr + 8);
    uint32_t arg2 = memory::load_and_swap<uint32_t>(scratch_ptr + 12);

    if (normal_routine) {
      if (dispatcher->GetFunction(normal_routine)) {
        uint64_t normal_args[] = {normal_context, arg1, arg2};
        dispatcher->ExecuteTrap(thread_state_.get(), normal_routine, normal_args,
                                rex::countof(normal_args));
      } else {
        REXSYS_ERROR("DeliverAPCs: normal_routine {:08X} not found", normal_routine);
      }
    }

    REXSYS_DEBUG("Completed delivery of APC to {:08X} ({:08X}, {:08X}, {:08X})", normal_routine,
                 normal_context, arg1, arg2);

    if (needs_freeing) {
      mem->SystemHeapFree(apc_ptr);
    }

    old_irql = kernel::xboxkrnl::xeKeKfAcquireSpinLock(ctx, &kthread->apc_lock);
  }

  kernel::xboxkrnl::xeKeKfReleaseSpinLock(ctx, &kthread->apc_lock, old_irql);
}

void XThread::RundownAPCs() {
  assert_true(XThread::GetCurrentThread() == this);
  auto mem = memory();
  auto kthread = guest_object<X_KTHREAD>();
  auto* ctx = thread_state_->context();

  // Rundown both user (1) and kernel (0) APC lists.
  for (int mode = 1; mode >= 0; --mode) {
    auto old_irql = kernel::xboxkrnl::xeKeKfAcquireSpinLock(ctx, &kthread->apc_lock);
    const GuestMemory guest_memory(mem->virtual_membase());

    while (gh::kernel::GuestApcQueue::Pending(guest_memory, guest_object(), mode)) {
      uint32_t apc_ptr = gh::kernel::GuestApcQueue::Pop(guest_memory, guest_object(), mode);
      XAPC* apc = mem->TranslateVirtual<XAPC*>(apc_ptr);
      bool needs_freeing = apc->kernel_routine == XAPC::kDummyKernelRoutine;

      kernel::xboxkrnl::xeKeKfReleaseSpinLock(ctx, &kthread->apc_lock, old_irql);

      if (apc->rundown_routine == XAPC::kDummyRundownRoutine) {
        // No-op.
      } else if (apc->rundown_routine) {
        auto* dispatcher = kernel_state_->function_dispatcher();
        if (dispatcher->GetFunction(apc->rundown_routine)) {
          uint64_t rundown_args[] = {apc_ptr};
          dispatcher->Execute(thread_state_.get(), apc->rundown_routine, rundown_args,
                              rex::countof(rundown_args));
        } else {
          REXSYS_WARN("RundownAPCs: rundown_routine {:08X} not found",
                      uint32_t(apc->rundown_routine));
        }
      }

      if (needs_freeing) {
        mem->SystemHeapFree(apc_ptr);
      }

      old_irql = kernel::xboxkrnl::xeKeKfAcquireSpinLock(ctx, &kthread->apc_lock);
    }
    kernel::xboxkrnl::xeKeKfReleaseSpinLock(ctx, &kthread->apc_lock, old_irql);
  }
}

int32_t XThread::QueryPriority() {
  return thread_->priority();
}

void XThread::SetPriority(int32_t increment) {
  priority_ = increment;

  // Write priority to guest X_KTHREAD struct.
  auto kthread = guest_object<X_KTHREAD>();
  kthread->priority = static_cast<uint8_t>(std::clamp(increment, 0, 31));

  int32_t target_priority = 0;
  if (increment > 0x22) {
    target_priority = rex::thread::ThreadPriority::kHighest;
  } else if (increment > 0x11) {
    target_priority = rex::thread::ThreadPriority::kAboveNormal;
  } else if (increment < -0x22) {
    target_priority = rex::thread::ThreadPriority::kLowest;
  } else if (increment < -0x11) {
    target_priority = rex::thread::ThreadPriority::kBelowNormal;
  } else {
    target_priority = rex::thread::ThreadPriority::kNormal;
  }
  if (!REXCVAR_GET(ignore_thread_priorities)) {
    thread_->set_priority(target_priority);
  }
}

void XThread::SetAffinity(uint32_t affinity) {
  SetActiveCpu(GetFakeCpuNumber(affinity));
}

uint8_t XThread::active_cpu() const {
  // Prefer reading from guest KTHREAD (always available for guest threads,
  // kept in sync by SetActiveCpu). Avoids dependency on pcr_address_ which
  // may not be set yet if the thread is mid-creation.
  if (is_guest_thread()) {
    auto* kthread = memory()->TranslateVirtual<const X_KTHREAD*>(guest_object());
    return kthread->current_cpu;
  }
  if (!pcr_address_) {
    return 0;
  }
  const X_KPCR& pcr = *memory()->TranslateVirtual<const X_KPCR*>(pcr_address_);
  return pcr.prcb_data.current_cpu;
}

void XThread::SetActiveCpu(uint8_t cpu_index) {
  // May be called during thread creation - don't skip if current == new.

  assert_true(cpu_index < 6);

  // Write to guest KTHREAD (always available for guest threads).
  if (is_guest_thread()) {
    X_KTHREAD& thread_object = *memory()->TranslateVirtual<X_KTHREAD*>(guest_object());
    thread_object.current_cpu = cpu_index;
  }

  // Write to PCR if allocated (may not be during early creation).
  if (pcr_address_) {
    X_KPCR& pcr = *memory()->TranslateVirtual<X_KPCR*>(pcr_address_);
    pcr.prcb_data.current_cpu = cpu_index;
  }

  if (rex::thread::logical_processor_count() >= 6) {
    if (!REXCVAR_GET(ignore_thread_affinities)) {
      thread_->set_affinity_mask(uint64_t(1) << cpu_index);
    }
  } else {
    REXSYS_WARN("Too few processor cores - scheduling will be wonky");
  }
}

bool XThread::GetTLSValue(uint32_t slot, uint32_t* value_out) {
  const auto dynamic_size = tls_total_size_ - (tls_dynamic_address_ - tls_static_address_);
  return gh::kernel::GuestTls::Read(GuestMemory(memory()->virtual_membase()),
                                    {tls_dynamic_address_, dynamic_size}, slot, value_out);
}

bool XThread::SetTLSValue(uint32_t slot, uint32_t value) {
  const auto dynamic_size = tls_total_size_ - (tls_dynamic_address_ - tls_static_address_);
  return gh::kernel::GuestTls::Write(GuestMemory(memory()->virtual_membase()),
                                     {tls_dynamic_address_, dynamic_size}, slot, value);
}

uint32_t XThread::suspend_count() {
  return guest_object<X_KTHREAD>()->suspend_count;
}

X_STATUS XThread::Resume(uint32_t* out_suspend_count) {
  auto guest_thread = guest_object<X_KTHREAD>();
  uint32_t unused_host_suspend_count = 0;

#if REX_PLATFORM_WIN32
  uint8_t previous_suspend_count =
      reinterpret_cast<std::atomic_uint8_t*>(&guest_thread->suspend_count)->fetch_sub(1);
  if (out_suspend_count) {
    *out_suspend_count = previous_suspend_count;
  }
  if (!execution_started_ && previous_suspend_count > 1) return X_STATUS_SUCCESS;
  return thread_->Resume(&unused_host_suspend_count) ? X_STATUS_SUCCESS : X_STATUS_UNSUCCESSFUL;
#elif REX_PLATFORM_LINUX || REX_PLATFORM_MAC
  {
    std::lock_guard<std::mutex> lock(suspend_mutex_);
    uint8_t previous = guest_thread->suspend_count;
    if (previous > 0) {
      guest_thread->suspend_count--;
    }
    if (out_suspend_count) {
      *out_suspend_count = previous;
    }
    if (!execution_started_ && previous > 1) return X_STATUS_SUCCESS;
    suspend_cv_.notify_all();
  }

  // Balance each native suspension; SelfSuspend wakes from the guest count.
  thread_->Resume(&unused_host_suspend_count);
  return X_STATUS_SUCCESS;
#else
  uint8_t previous_suspend_count = guest_thread->suspend_count;
  if (guest_thread->suspend_count > 0) {
    --guest_thread->suspend_count;
  }
  if (out_suspend_count) {
    *out_suspend_count = previous_suspend_count;
  }
  return thread_->Resume(&unused_host_suspend_count) ? X_STATUS_SUCCESS : X_STATUS_UNSUCCESSFUL;
#endif
}

X_STATUS XThread::Suspend(uint32_t* out_suspend_count) {
  CheckTitleTermination();
  const bool before_first_execute = is_created_suspended();
  auto guest_thread = guest_object<X_KTHREAD>();
  uint8_t previous_suspend_count =
      reinterpret_cast<std::atomic_uint8_t*>(&guest_thread->suspend_count)->fetch_add(1);
  if (out_suspend_count) {
    *out_suspend_count = previous_suspend_count;
  }
  if (before_first_execute) return X_STATUS_SUCCESS;

  uint32_t unused_host_suspend_count = 0;
  // Wrapped to 0 - treat as not suspended.
  if (guest_thread->suspend_count == 0) {
    return X_STATUS_SUCCESS;
  }
  const bool suspended = thread_->Suspend(&unused_host_suspend_count);
  CheckTitleTermination();
  return suspended ? X_STATUS_SUCCESS : X_STATUS_UNSUCCESSFUL;
}

#if REX_PLATFORM_LINUX || REX_PLATFORM_MAC
uint32_t XThread::SelfSuspend() {
  CheckTitleTermination();
  auto guest_thread = guest_object<X_KTHREAD>();
  std::unique_lock<std::mutex> lock(suspend_mutex_);
  uint32_t previous = guest_thread->suspend_count;
  guest_thread->suspend_count++;
  suspend_cv_.wait(lock, [this, guest_thread]() {
    return guest_thread->suspend_count == 0 ||
           (is_guest_thread() && kernel_state_->is_terminating_title());
  });
  lock.unlock();
  CheckTitleTermination();
  return previous;
}
#endif

X_STATUS XThread::Delay(uint32_t processor_mode, uint32_t alertable, uint64_t interval) {
  CheckTitleTermination();
  const auto timeout_ticks = static_cast<int64_t>(interval);
  std::chrono::nanoseconds timeout;
  if (timeout_ticks < 0) {
    const auto ticks = static_cast<uint64_t>(-(timeout_ticks + 1)) + 1;
    const auto milliseconds = std::min<uint64_t>(
        ticks / 10'000, std::numeric_limits<uint32_t>::max());
    timeout = std::chrono::milliseconds(chrono::Clock::ScaleGuestDurationMillis(
        static_cast<uint32_t>(milliseconds)));
  } else {
    const auto scaled_timeout = chrono::Clock::ScaleGuestDurationFileTime(timeout_ticks);
    timeout = gh::kernel::GuestTimeoutToDuration(
        scaled_timeout, chrono::Clock::QueryGuestSystemTime());
  }
  if (!alertable && timeout == std::chrono::nanoseconds::zero()) {
    if (priority_ <= rex::thread::ThreadPriority::kBelowNormal) {
      rex::thread::Sleep(std::chrono::microseconds(100));
    } else {
      rex::thread::MaybeYield();
    }
    CheckTitleTermination();
    return X_STATUS_SUCCESS;
  }

  gh::kernel::Event delay_event(false);
  gh::kernel::Waitable* waitables[] = {&delay_event};
  const auto interrupted = [alertable] {
    auto* thread = GetBoundCurrentXThread();
    return thread &&
           ((thread->is_guest_thread() && thread->kernel_state()->is_terminating_title()) ||
            (alertable && thread->HasDeliverableAPCs()));
  };
  if (!alertable) {
    gh::kernel::WaitMultiple(waitables, false, timeout, interrupted);
    CheckTitleTermination();
    return X_STATUS_SUCCESS;
  }

  const auto started = std::chrono::steady_clock::now();
  const auto maximum_timeout = std::chrono::steady_clock::time_point::max() - started;
  const auto deadline = timeout >= maximum_timeout
                            ? std::chrono::steady_clock::time_point::max()
                            : started + timeout;
  // Callbacks run outside the project wait lock.
  const bool initial_callback = gh::kernel::HostThread::DispatchUserCallbacks();
  CheckTitleTermination();
  if (initial_callback) return X_STATUS_USER_APC;
  for (;;) {
    const auto remaining = std::max(std::chrono::nanoseconds::zero(),
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            deadline - std::chrono::steady_clock::now()));
    // Keep the existing 1 ms host callback polling bound between project waits.
    const auto slice = std::min(remaining, std::chrono::nanoseconds(std::chrono::milliseconds(1)));
    const auto result = gh::kernel::WaitMultiple(waitables, false, slice, interrupted);
    CheckTitleTermination();
    const bool callback = gh::kernel::HostThread::DispatchUserCallbacks();
    CheckTitleTermination();
    if (result.first == gh::kernel::WaitResult::Alerted ||
        callback) {
      return X_STATUS_USER_APC;
    }
    if (std::chrono::steady_clock::now() >= deadline) return X_STATUS_SUCCESS;
  }
}

struct ThreadSavedState {
  uint32_t thread_id;
  bool is_main_thread;  // Is this the main thread?
  bool is_running;

  uint32_t apc_head;
  uint32_t tls_static_address;
  uint32_t tls_dynamic_address;
  uint32_t tls_total_size;
  uint32_t pcr_address;
  uint32_t stack_base;        // High address
  uint32_t stack_limit;       // Low address
  uint32_t stack_alloc_base;  // Allocation address
  uint32_t stack_alloc_size;  // Allocation size

  // Context (invalid if not running)
  struct {
    uint64_t lr;
    uint64_t ctr;
    uint64_t r[32];
    double f[32];
    vec128_t v[128];
    uint32_t cr[8];
    uint32_t fpscr;
    uint8_t xer_ca;
    uint8_t xer_ov;
    uint8_t xer_so;
    uint8_t vscr_sat;
    uint32_t pc;
  } context;
};

bool XThread::Save(stream::ByteStream* stream) { return SaveState(stream, nullptr); }

bool XThread::SaveContinuation(stream::ByteStream* stream,
    const gh::guest::continuation::TitleCoordinator::ThreadImage& image) {
  if (!is_alive() || image.thread_id != thread_id_) return false;
  return SaveState(stream, &image);
}

bool XThread::SaveState(stream::ByteStream* stream,
    const gh::guest::continuation::TitleCoordinator::ThreadImage* image) {
  if (!guest_thread_) {
    // Host XThreads are expected to be recreated on their own.
    return false;
  }

  REXSYS_DEBUG("XThread {:08X} serializing...", handle());

  const bool created_suspended = is_created_suspended();
  if (is_alive() && !image && !created_suspended) {
    REXSYS_WARN("Cannot save unfinished XThread {:08X} without a coordinated continuation stop",
                handle());
    return false;
  }

  if (!SaveObject(stream)) {
    return false;
  }

  stream->Write(image ? kThreadContinuationSignature : created_suspended
      ? kThreadCreatedSuspendedSignature : kThreadSaveSignature);
  stream->Write(thread_name_);

  ThreadSavedState state{};
  state.thread_id = thread_id_;
  state.is_main_thread = main_thread_;
  state.is_running = running_;
  state.apc_head = 0;  // APCs now live in guest-memory typed lists
  state.tls_static_address = tls_static_address_;
  state.tls_dynamic_address = tls_dynamic_address_;
  state.tls_total_size = tls_total_size_;
  state.pcr_address = pcr_address_;
  state.stack_base = stack_base_;
  state.stack_limit = stack_limit_;
  state.stack_alloc_base = stack_alloc_base_;
  state.stack_alloc_size = stack_alloc_size_;

  stream->Write(&state, sizeof(ThreadSavedState));
  if (image || created_suspended) {
    stream->Write(creation_params_);
    stream->Write(scratch_address_);
    stream->Write(scratch_size_);
    stream->Write(priority_);
    if (image) {
      stream->Write<uint32_t>(static_cast<uint32_t>(image->execution.size()));
      stream->Write(std::span<const uint8_t>(image->execution));
    } else {
      static_assert(std::is_trivially_copyable_v<GuestContext>);
      stream->Write(*thread_state_->context());
      stream->Write<uint32_t>(guest_object<X_KTHREAD>()->suspend_count);
    }
  }
  return true;
}

object_ref<XThread> XThread::Restore(KernelState* kernel_state, stream::ByteStream* stream) {
  // Decode and validate every frame before publishing this thread's handles.
  stream::ByteStream probe(stream->data(), stream->data_length(), stream->offset());
  probe.Read<uint32_t>();  // allocated_guest_object
  const auto guest_object = probe.Read<uint32_t>();
  const auto handle_count = probe.Read<uint32_t>();
  probe.Advance(size_t(handle_count) * sizeof(X_HANDLE));
  const auto signature = probe.Read<uint32_t>();
  const bool continuation = signature == kThreadContinuationSignature;
  const bool created_suspended = signature == kThreadCreatedSuspendedSignature;
  if (!continuation && !created_suspended && signature != kThreadSaveSignature) {
    REXSYS_ERROR("Could not restore XThread - invalid magic!");
    return nullptr;
  }
  auto thread_name = probe.Read<std::string>();
  ThreadSavedState state{};
  probe.Read(&state, sizeof(state));
  const bool completed = guest_object &&
      kernel_state->memory()->TranslateVirtual<X_KTHREAD*>(guest_object)->header.signal_state;
  CreationParams creation{};
  uint32_t scratch_address = 0, scratch_size = 0;
  int32_t priority = 0;
  std::vector<uint8_t> execution;
  GuestContext startup_context{};
  if (continuation || created_suspended) {
    const auto* program = gh::guest::continuation::TitleProgram();
    if (completed || (continuation && (!program || !state.is_running)) ||
        (created_suspended && state.is_running)) return nullptr;
    creation = probe.Read<CreationParams>();
    scratch_address = probe.Read<uint32_t>();
    scratch_size = probe.Read<uint32_t>();
    priority = probe.Read<int32_t>();
    if (created_suspended) {
      startup_context = probe.Read<GuestContext>();
      const auto suspend_count = probe.Read<uint32_t>();
      const auto* guest_thread = kernel_state->memory()->TranslateVirtual<X_KTHREAD*>(guest_object);
      if (!suspend_count || suspend_count != guest_thread->suspend_count ||
          startup_context.r13.u64 != state.pcr_address || !scratch_address || !scratch_size)
        return nullptr;
    } else {
      const auto size = probe.Read<uint32_t>();
      if (size > probe.data_length() - probe.offset()) return nullptr;
      execution.resize(size);
      probe.Read(execution);
      try {
        gh::guest::continuation::Machine validated(*program);
        validated.Restore(execution);
        if (validated.context.r13.u64 != state.pcr_address) return nullptr;
      } catch (const std::exception& error) {
        REXSYS_ERROR("Invalid restored XThread continuation: {}", error.what());
        return nullptr;
      }
    }
  } else if (state.is_running || !completed) {
    REXSYS_ERROR("Cannot restore unfinished XThread without continuation payload");
    return nullptr;
  }

  auto thread = new XThread(nullptr);
  thread->kernel_state_ = kernel_state;
  thread->RestoreObject(stream);
  stream->set_offset(probe.offset());
  thread->thread_name_ = std::move(thread_name);
  thread->thread_id_ = state.thread_id;
  thread->main_thread_ = state.is_main_thread;
  thread->tls_static_address_ = state.tls_static_address;
  thread->tls_dynamic_address_ = state.tls_dynamic_address;
  thread->tls_total_size_ = state.tls_total_size;
  thread->pcr_address_ = state.pcr_address;
  thread->stack_base_ = state.stack_base;
  thread->stack_limit_ = state.stack_limit;
  thread->stack_alloc_base_ = state.stack_alloc_base;
  thread->stack_alloc_size_ = state.stack_alloc_size;
  thread->creation_params_ = creation;
  thread->scratch_address_ = scratch_address;
  thread->scratch_size_ = scratch_size;
  thread->priority_ = priority;
  thread->execution_started_ = continuation || completed;
  thread->restored_execution_ = std::move(execution);
  next_xthread_id_ = std::max(next_xthread_id_, state.thread_id);
  kernel_state->RegisterThread(thread);
  thread->thread_state_ = std::make_unique<runtime::ThreadState>(
      thread->thread_id_, thread->stack_base_, thread->pcr_address_, kernel_state->memory());
  if (continuation || created_suspended) {
    auto* pcr = kernel_state->memory()->TranslateVirtual<X_KPCR*>(state.pcr_address);
    pcr->host_stash = reinterpret_cast<uint64_t>(thread->thread_state_->context());
    if (created_suspended) {
      *thread->thread_state_->context() = startup_context;
      if (!thread->CreateNativeHostThread()) return nullptr;
      thread->thread_->set_name(thread->thread_name_);
      thread->thread_->set_priority(thread->priority_);
      thread->SetActiveCpu(thread->active_cpu());
    }
  } else {
    thread->kernel_thread_event_->Set();
  }
  return object_ref<XThread>(thread);
}

void XThread::AbandonGuestStorageForRestore() {
  XObject::AbandonGuestStorageForRestore();
  scratch_address_ = scratch_size_ = 0;
  tls_static_address_ = tls_dynamic_address_ = tls_total_size_ = pcr_address_ = 0;
  stack_alloc_base_ = stack_alloc_size_ = stack_base_ = stack_limit_ = 0;
}

bool XThread::PrepareContinuationRestore(std::shared_ptr<ContinuationStartGate> gate) {
  if (thread_ || restored_execution_.empty()) return false;
  gh::kernel::HostThread::CreationParameters params;
  params.stack_size = 16_MiB;
  params.create_suspended = true;
  // The saved handle reference count already includes the execution reference.
  thread_ = gh::kernel::HostThread::Create(params, [this, gate = std::move(gate)] {
    rex::initialize_seh_thread();
    runtime::ThreadState::Bind(thread_state_.get());
    rex::thread::set_current_thread_id(handle());
    current_xthread_tls_ = this;
    gh::kernel::SetThreadLocalValue(handle());
    running_ = true;
    for (const auto& [mutant, recursion] : pending_mutant_acquires_) {
      if (!mutant->RestoreOwnershipOnCurrentThread(recursion)) {
        std::lock_guard lock(gate->mutex);
        gate->cancelled = true;
      }
    }
    pending_mutant_acquires_.clear();
    try {
      restored_machine_ = std::make_unique<gh::guest::continuation::Machine>(
          *gh::guest::continuation::TitleProgram());
      restored_machine_->Restore(restored_execution_);
      restored_machine_->PreparePendingHost(memory()->virtual_membase(), true);
    } catch (const std::exception& error) {
      REXSYS_ERROR("Cannot restore queued guest operation: {}", error.what());
      std::lock_guard lock(gate->mutex); gate->cancelled = true;
    }
    {
      std::unique_lock lock(gate->mutex);
      ++gate->ready;
      gate->changed.notify_all();
      gate->changed.wait(lock, [&] { return gate->start || gate->cancelled; });
      if (gate->cancelled) {
        restored_machine_.reset();
        running_ = false;
        kernel_thread_event_->Set();
        current_xthread_tls_ = nullptr;
        gh::kernel::SetThreadLocalValue(0);
        return;
      }
    }
    PROFILE_THREAD_ENTER(thread_name_.c_str());
    PROFILE_THREAD_CREATED();
    CheckTitleTermination();
    Execute();
    Exit(0);
  });
  if (!thread_) return false;
  thread_->set_name(thread_name_);
  thread_->set_priority(priority_);
  return true;
}

XHostThread::XHostThread(KernelState* kernel_state, uint32_t stack_size, uint32_t creation_flags,
                         std::function<int()> host_fn)
    : XThread(kernel_state, stack_size, 0, 0, 0, creation_flags, false), host_fn_(host_fn) {
  // NOTE(tomc): there was a start suspended check here before but I don't think we need it.
}

void XHostThread::Execute() {
  REXSYS_DEBUG("XThread::Execute thid {} (handle={:08X}, '{}', native={:08X}, <host>)", thread_id_,
               handle(), thread_name_, thread_->system_id());

  // Let the kernel know we are starting.
  kernel_state_->OnThreadExecute(this);

  int ret = host_fn_();

  // Exit.
  Exit(ret);
}

}  // namespace rex::system
