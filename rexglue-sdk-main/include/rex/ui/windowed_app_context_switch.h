#pragma once

#include <rex/ui/windowed_app_context.h>

namespace rex::ui {

// Horizon's applet loop is the platform UI loop for a Switch title.  This
// context supplies the same pending-function and shutdown contract as the
// desktop contexts without requiring SDL or an OS window.
class SwitchWindowedAppContext final : public WindowedAppContext {
 public:
  SwitchWindowedAppContext() = default;
  ~SwitchWindowedAppContext() override;

  bool Initialize();
  int RunMainMessageLoop();

 protected:
  void NotifyUILoopOfPendingFunctions() override;
  void PlatformQuitFromUIThread() override;

 private:
  bool initialized_ = false;
  bool notification_pending_ = false;
};

}  // namespace rex::ui
