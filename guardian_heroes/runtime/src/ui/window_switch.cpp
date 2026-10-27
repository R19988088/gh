#include <rex/ui/window_switch.h>

#include <switch.h>

#include <rex/ui/surface_switch.h>

namespace rex::ui {

bool ViNWindowSurface::GetSizeImpl(uint32_t& width_out, uint32_t& height_out) const {
  width_out = owner_.GetActualPhysicalWidth();
  height_out = owner_.GetActualPhysicalHeight();
  return width_out != 0 && height_out != 0;
}

std::unique_ptr<Window> Window::Create(WindowedAppContext& app_context,
                                       std::string_view title,
                                       uint32_t desired_logical_width,
                                       uint32_t desired_logical_height) {
  return std::make_unique<WindowSwitch>(app_context, title, desired_logical_width,
                                        desired_logical_height);
}

WindowSwitch::WindowSwitch(WindowedAppContext& app_context, std::string_view title,
                           uint32_t desired_logical_width, uint32_t desired_logical_height)
    : Window(app_context, title, desired_logical_width, desired_logical_height) {}

WindowSwitch::~WindowSwitch() {
  paint_lifetime_.reset();
  EnterDestructor();
  nwindow_ = nullptr;
}

bool WindowSwitch::OpenImpl() {
  nwindow_ = nwindowGetDefault();
  if (!nwindow_ || !nwindowIsValid(nwindow_)) {
    nwindow_ = nullptr;
    return false;
  }

  // The default applet NWindow fills the display; handheld and docked modes
  // have distinct output sizes.
  uint32_t width = 1280;
  uint32_t height = 720;
  s32 display_width = 0;
  s32 display_height = 0;
  if (R_SUCCEEDED(appletGetDefaultDisplayResolution(&display_width, &display_height)) &&
      display_width > 0 && display_height > 0) {
    width = uint32_t(display_width);
    height = uint32_t(display_height);
  } else if (appletGetOperationMode() == AppletOperationMode_Console) {
    width = 1920;
    height = 1080;
  }

  WindowDestructionReceiver destruction_receiver(this);
  OnActualSizeUpdate(width, height, destruction_receiver);
  if (destruction_receiver.IsWindowDestroyed()) {
    return true;
  }
  if (appletGetFocusState() == AppletFocusState_InFocus) {
    OnFocusUpdate(true, destruction_receiver);
  }
  return true;
}

void WindowSwitch::RequestCloseImpl() {
  PerformClose();
}

void WindowSwitch::PerformClose() {
  WindowDestructionReceiver destruction_receiver(this);
  OnBeforeClose(destruction_receiver);
  if (!destruction_receiver.IsWindowDestroyed()) {
    nwindow_ = nullptr;
    OnAfterClose();
  }
}

std::unique_ptr<Surface> WindowSwitch::CreateSurfaceImpl(Surface::TypeFlags allowed_types) {
  if (!nwindow_ || !(allowed_types & Surface::kTypeFlag_ViNWindow)) {
    return nullptr;
  }
  return std::make_unique<ViNWindowSurface>(nwindow_, *this);
}

void WindowSwitch::RequestPaintImpl() {
  std::weak_ptr<uint8_t> lifetime = paint_lifetime_;
  app_context().CallInUIThreadDeferred([this, lifetime] {
    if (lifetime.expired()) {
      return;
    }
    if (phase() == Phase::kOpen) {
      OnPaint();
    }
  });
}

}  // namespace rex::ui
