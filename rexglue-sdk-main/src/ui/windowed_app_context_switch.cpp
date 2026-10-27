#include <rex/ui/windowed_app_context_switch.h>

#include <switch.h>

#include <cstdlib>

namespace rex::ui {

SwitchWindowedAppContext::~SwitchWindowedAppContext() {
  if (initialized_) {
    hidExit();
    appletUnlockExit();
  }
}

bool SwitchWindowedAppContext::Initialize() {
  if (initialized_) {
    return true;
  }
  Result rc = hidInitialize();
  if (R_FAILED(rc)) {
    return false;
  }
  appletLockExit();
  initialized_ = true;
  return true;
}

int SwitchWindowedAppContext::RunMainMessageLoop() {
  if (!initialized_) {
    return EXIT_FAILURE;
  }
  while (appletMainLoop()) {
    notification_pending_ = false;
    ExecutePendingFunctionsFromUIThread();
    // libnx has no separate UI wake primitive.  The applet loop provides the
    // blocking point, while queued callbacks are drained on every iteration.
    (void)notification_pending_;
  }
  ExecutePendingFunctionsFromUIThread();
  return EXIT_SUCCESS;
}

void SwitchWindowedAppContext::NotifyUILoopOfPendingFunctions() {
  notification_pending_ = true;
}

void SwitchWindowedAppContext::PlatformQuitFromUIThread() {
  // appletMainLoop observes the exit request and returns false on the next
  // iteration.  Unlocking here also permits normal HOME/exit handling.
  appletUnlockExit();
}

}  // namespace rex::ui
