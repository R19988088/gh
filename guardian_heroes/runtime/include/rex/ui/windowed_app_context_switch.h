#pragma once

#include <rex/ui/windowed_app_context.h>

namespace rex::ui {

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
