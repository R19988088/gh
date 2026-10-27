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
  if (R_FAILED(hidInitialize())) {
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
    ExecutePendingFunctionsFromUIThread();
  }
  ExecutePendingFunctionsFromUIThread();
  return EXIT_SUCCESS;
}

void SwitchWindowedAppContext::NotifyUILoopOfPendingFunctions() {}

void SwitchWindowedAppContext::PlatformQuitFromUIThread() {
  appletUnlockExit();
}

}  // namespace rex::ui
