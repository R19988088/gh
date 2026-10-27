#include "gh_gpu_factory.h"

#include <rex/logging.h>
#include <rex/system/interfaces/graphics.h>

#if REX_HAS_D3D12
#include "gh_gpu_d3d12_graphics_system.h"
#endif
#if REX_HAS_VULKAN
#include "gh_gpu_vulkan_graphics_system.h"
#endif

#if defined(__SWITCH__) && defined(GH_HAS_DEKO3D_BACKEND)
namespace rex::graphics::deko {
rex::system::IGraphicsSystem* CreateDeko3dGraphicsSystem();
}
#endif

namespace gh::graphics {

std::unique_ptr<rex::system::IGraphicsSystem> CreateGraphicsSystem(
    std::string_view plugin, std::string_view backend) {
  if (plugin.empty() || plugin == "nop") {
    return nullptr;
  }
  if (plugin != "xenos") {
    REXLOG_ERROR("Unsupported graphics system '{}'", plugin);
    return nullptr;
  }
#if defined(__SWITCH__) && defined(GH_HAS_DEKO3D_BACKEND)
  // Switch uses the native deko3d command processor. "any" must not fall
  // through to a desktop Vulkan implementation.
  if (backend == "any" || backend == "deko3d") {
    return std::unique_ptr<rex::system::IGraphicsSystem>(
        rex::graphics::deko::CreateDeko3dGraphicsSystem());
  }
#endif
#if REX_HAS_D3D12
  if (backend == "any" || backend == "d3d12") {
    return std::make_unique<rex::graphics::d3d12::D3D12GraphicsSystem>();
  }
#endif
#if REX_HAS_VULKAN
  if (backend == "any" || backend == "vulkan") {
    return std::make_unique<rex::graphics::vulkan::VulkanGraphicsSystem>();
  }
#endif
  REXLOG_ERROR("Requested graphics backend '{}' is not compiled into this application", backend);
  return nullptr;
}

}  // namespace gh::graphics
