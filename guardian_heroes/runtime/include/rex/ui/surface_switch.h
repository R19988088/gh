#pragma once

#include <rex/ui/surface.h>

struct NWindow;

namespace rex::ui {

class Window;

class ViNWindowSurface final : public Surface {
 public:
  ViNWindowSurface(NWindow* window, const Window& owner) : window_(window), owner_(owner) {}
  TypeIndex GetType() const override { return kTypeIndex_ViNWindow; }
  NWindow* window() const { return window_; }

 protected:
  bool GetSizeImpl(uint32_t& width_out, uint32_t& height_out) const override;

 private:
  NWindow* window_;
  const Window& owner_;
};

}  // namespace rex::ui
