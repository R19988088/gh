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

#include <rex/chrono/clock.h>
#include <algorithm>
#include <rex/logging.h>
#include <rex/stream.h>
#include "gh_system_kernel_state.h"
#include "gh_system_xthread.h"
#include "gh_system_xtimer.h"

#include "gh_kernel.h"

namespace rex::system {

XTimer::XTimer(KernelState* kernel_state) : XObject(kernel_state, kObjectType) {}

XTimer::~XTimer() {
  Cancel();
}

void XTimer::Initialize(uint32_t timer_type) {
  assert_false(kernel_timer_);
  assert_true(timer_type == 0 || timer_type == 1);
  kernel_timer_ = std::make_unique<gh::kernel::Timer>();
  manual_reset_ = timer_type == 0;
  kernel_timer_event_ = std::make_unique<gh::kernel::Event>(manual_reset_);
  if (kernel_state_ && kernel_state_->timers_paused_for_snapshot()) PauseTimerLocked();
}

gh::kernel::Waitable* XTimer::GetKernelWaitable() { return kernel_timer_event_.get(); }

X_STATUS XTimer::SetTimer(int64_t due_time, uint32_t period_ms, uint32_t routine,
                          uint32_t routine_arg, bool resume) {
  if (resume) return X_STATUS_TIMER_RESUME_IGNORED;
  const auto due = gh::kernel::GuestTimeoutToDuration(
      chrono::Clock::ScaleGuestDurationFileTime(due_time),
      chrono::Clock::QueryGuestSystemTime());
  const auto period = std::chrono::milliseconds(chrono::Clock::ScaleGuestDurationMillis(period_ms));

  // Release thread references outside control_mutex_: their destructors acquire
  // the global kernel lock, which can be held by another timer caller.
  auto target = routine ? retain_object(XThread::GetCurrentThread()) : object_ref<XThread>{};
  object_ref<XThread> previous;
  std::lock_guard lock(control_mutex_);
  kernel_timer_->Cancel();
  previous = std::move(callback_thread_);
  callback_thread_ = std::move(target);
  kernel_timer_event_->Reset();
  routine_ = routine;
  routine_arg_ = routine_arg;
  if (kernel_state_ && kernel_state_->timers_paused_for_snapshot()) PauseTimerLocked();
  try {
    Arm(due, period);
    if (paused_for_snapshot_) paused_guest_tick_ = chrono::Clock::QueryGuestTickCount();
  } catch (const std::exception&) {
    target = std::move(callback_thread_);
    routine_ = routine_arg_ = 0;
    return X_STATUS_UNSUCCESSFUL;
  }
  return X_STATUS_SUCCESS;
}

void XTimer::Arm(std::chrono::nanoseconds due, std::chrono::nanoseconds period) {
  auto* callback_thread = callback_thread_.get();
  const auto routine = routine_, routine_arg = routine_arg_;
  kernel_timer_->Set(due, period, [this, callback_thread, routine, routine_arg] {
      kernel_timer_event_->Set();
      if (!callback_thread || !callback_thread->is_alive()) return;
      const uint64_t time = chrono::Clock::QueryGuestSystemTime();
      callback_thread->EnqueueApc(routine, routine_arg, static_cast<uint32_t>(time),
                                  static_cast<uint32_t>(time >> 32));
  });
}

X_STATUS XTimer::Cancel() {
  object_ref<XThread> target;
  {
    std::lock_guard lock(control_mutex_);
    if (kernel_timer_) kernel_timer_->Cancel();
    target = std::move(callback_thread_);
    routine_ = routine_arg_ = 0;
  }
  return X_STATUS_SUCCESS;
}

void XTimer::PauseForSnapshot() {
  std::lock_guard lock(control_mutex_);
  if (kernel_timer_) PauseTimerLocked();
}

void XTimer::PauseTimerLocked() {
  if (paused_for_snapshot_) return;
  kernel_timer_->Pause();
  paused_guest_tick_ = chrono::Clock::QueryGuestTickCount();
  paused_for_snapshot_ = true;
}

void XTimer::UpdatePausedDeadline() {
  if (!paused_for_snapshot_) return;
  const auto now = chrono::Clock::QueryGuestTickCount();
  const auto ticks = now > paused_guest_tick_ ? now - paused_guest_tick_ : 0;
  const auto ratio = chrono::Clock::guest_tick_ratio();
  const auto elapsed = static_cast<unsigned __int128>(ticks) * ratio.second * 1'000'000'000ull /
      (static_cast<unsigned __int128>(ratio.first) * chrono::Clock::QueryHostTickFrequency());
  const auto remaining = kernel_timer_->CapturePausedState().remaining.count();
  kernel_timer_->ElapsePaused(std::chrono::nanoseconds(
      static_cast<int64_t>(std::min<unsigned __int128>(elapsed, uint64_t(remaining)))));
  paused_guest_tick_ = now;
}

void XTimer::ResumeAfterSnapshot() {
  std::lock_guard lock(control_mutex_);
  if (kernel_timer_) {
    UpdatePausedDeadline();
    kernel_timer_->Resume();
    paused_for_snapshot_ = false;
  }
}

bool XTimer::Save(stream::ByteStream* stream) {
  std::lock_guard lock(control_mutex_);
  if (!kernel_timer_ || !kernel_state_ || !kernel_state_->timers_paused_for_snapshot()) return false;
  UpdatePausedDeadline();
  const auto state = kernel_timer_->CapturePausedState();
  if (!SaveObject(stream)) return false;
  stream->Write<uint32_t>(0x31524D54);  // TMR1
  stream->Write(manual_reset_);
  stream->Write(kernel_timer_event_->signaled());
  stream->Write(state.armed);
  stream->Write<int64_t>(state.remaining.count());
  stream->Write<int64_t>(state.period.count());
  stream->Write(routine_);
  stream->Write(routine_arg_);
  stream->Write<uint32_t>(callback_thread_ && callback_thread_->is_alive()
                             ? callback_thread_->handle() : 0);
  return true;
}

object_ref<XTimer> XTimer::Restore(KernelState* kernel_state, stream::ByteStream* stream) {
  auto timer = object_ref<XTimer>(new XTimer(nullptr));
  timer->kernel_state_ = kernel_state;
  if (!timer->RestoreObject(stream) || stream->Read<uint32_t>() != 0x31524D54) return nullptr;
  const bool manual = stream->Read<bool>();
  const bool signaled = stream->Read<bool>();
  const bool armed = stream->Read<bool>();
  const auto remaining = stream->Read<int64_t>();
  const auto period = stream->Read<int64_t>();
  timer->routine_ = stream->Read<uint32_t>();
  timer->routine_arg_ = stream->Read<uint32_t>();
  const auto callback_handle = stream->Read<uint32_t>();
  if (remaining < 0 || period < 0) return nullptr;
  timer->Initialize(manual ? 0 : 1);
  timer->PauseForSnapshot();
  if (signaled) timer->kernel_timer_event_->Set();
  if (callback_handle) {
    timer->callback_thread_ = kernel_state->object_table()->LookupObject<XThread>(callback_handle);
    if (!timer->callback_thread_) return nullptr;
  }
  timer->Arm(std::chrono::nanoseconds(remaining), std::chrono::nanoseconds(period));
  if (!armed) timer->kernel_timer_->Cancel();
  return timer;
}

}  // namespace rex::system
