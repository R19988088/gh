#pragma once
/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include "gh_system_xobject.h"
#include <rex/system/xtypes.h>
#include <rex/thread.h>

#include <mutex>
#include <chrono>

namespace gh::kernel {
class Event;
class Timer;
}

namespace rex::system {

struct XDPC;

struct X_KTIMER {
  X_DISPATCH_HEADER header;         // 0x0
  rex::be<uint64_t> due_time;       // 0x10
  X_LIST_ENTRY table_bucket_entry;  // 0x18
  TypedGuestPointer<XDPC> dpc;      // 0x20
  rex::be<uint32_t> period;         // 0x24
};
static_assert_size(X_KTIMER, 0x28);

class XThread;

class XTimer : public XObject {
  friend struct XTimerSnapshotTestAccess;
 public:
  static const XObject::Type kObjectType = XObject::Type::Timer;

  explicit XTimer(KernelState* kernel_state);
  ~XTimer() override;

  void Initialize(uint32_t timer_type);

  X_STATUS SetTimer(int64_t due_time, uint32_t period_ms, uint32_t routine, uint32_t routine_arg,
                    bool resume);
  X_STATUS Cancel();
  void PauseForSnapshot();
  void ResumeAfterSnapshot();
  bool Save(stream::ByteStream* stream) override;
  static object_ref<XTimer> Restore(KernelState*, stream::ByteStream*);

 protected:
  gh::kernel::Waitable* GetKernelWaitable() override;

 private:
  std::mutex control_mutex_;
  object_ref<XThread> callback_thread_;
  std::unique_ptr<gh::kernel::Timer> kernel_timer_;
  std::unique_ptr<gh::kernel::Event> kernel_timer_event_;
  bool manual_reset_ = false;
  uint32_t routine_ = 0;
  uint32_t routine_arg_ = 0;
  bool paused_for_snapshot_ = false;
  uint64_t paused_guest_tick_ = 0;
  void Arm(std::chrono::nanoseconds due, std::chrono::nanoseconds period);
  void PauseTimerLocked();
  void UpdatePausedDeadline();
};

}  // namespace rex::system
