#pragma once

#include <memory>
#include <string_view>

#include <rex/ui/window.h>

struct NWindow;

namespace rex::ui {

class WindowSwitch final : public Window {
 public:
  WindowSwitch(WindowedAppContext& app_context, std::string_view title,
               uint32_t desired_logical_width, uint32_t desired_logical_height);
  ~WindowSwitch() override;

 protected:
  bool OpenImpl() override;
  void RequestCloseImpl() override;
  std::unique_ptr<Surface> CreateSurfaceImpl(Surface::TypeFlags allowed_types) override;
  void RequestPaintImpl() override;

 private:
  void PerformClose();

  NWindow* nwindow_ = nullptr;
  std::shared_ptr<uint8_t> paint_lifetime_ = std::make_shared<uint8_t>(0);
};

}  // namespace rex::ui
