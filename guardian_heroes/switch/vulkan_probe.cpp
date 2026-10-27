#include <switch.h>
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vkGetInstanceProcAddr(VkInstance instance, const char* name);
extern "C" VKAPI_ATTR void VKAPI_CALL
vkDestroyInstance(VkInstance instance, const VkAllocationCallbacks* allocator);

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <limits>
#include <sys/stat.h>
#include <vector>

namespace {
FILE* log_file = nullptr;

void Log(const char* format, ...) {
  if (!log_file) return;
  va_list args;
  va_start(args, format);
  vfprintf(log_file, format, args);
  va_end(args);
  fflush(log_file);
}

bool Check(VkResult result, const char* operation) {
  if (result == VK_SUCCESS) return true;
  Log("FAIL %s: VkResult %d\n", operation, static_cast<int>(result));
  return false;
}

struct ClearProbe {
#define GH_VK_GLOBAL_FUNCTIONS(X) \
  X(vkCreateInstance)
#define GH_VK_INSTANCE_FUNCTIONS(X) \
  X(vkGetDeviceProcAddr) X(vkDestroySurfaceKHR) \
  X(vkEnumeratePhysicalDevices) X(vkGetPhysicalDeviceQueueFamilyProperties) \
  X(vkGetPhysicalDeviceSurfaceSupportKHR) X(vkGetPhysicalDeviceProperties) \
  X(vkGetPhysicalDeviceFeatures) X(vkGetPhysicalDeviceMemoryProperties) \
  X(vkEnumerateDeviceExtensionProperties) X(vkCreateDevice) \
  X(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) X(vkGetPhysicalDeviceSurfaceFormatsKHR)
#define GH_VK_DEVICE_FUNCTIONS(X) \
  X(vkDeviceWaitIdle) X(vkDestroySemaphore) X(vkDestroyCommandPool) \
  X(vkDestroySwapchainKHR) X(vkGetDeviceQueue) X(vkCreateCommandPool) \
  X(vkAllocateCommandBuffers) X(vkCreateSemaphore) X(vkCreateSwapchainKHR) \
  X(vkGetSwapchainImagesKHR) X(vkAcquireNextImageKHR) X(vkResetCommandBuffer) \
  X(vkBeginCommandBuffer) X(vkCmdPipelineBarrier) X(vkCmdClearColorImage) \
  X(vkEndCommandBuffer) X(vkQueueSubmit) X(vkQueuePresentKHR) X(vkQueueWaitIdle)
#define GH_VK_DECLARE(name) PFN_##name name = nullptr;
  GH_VK_GLOBAL_FUNCTIONS(GH_VK_DECLARE)
  GH_VK_INSTANCE_FUNCTIONS(GH_VK_DECLARE)
  GH_VK_DEVICE_FUNCTIONS(GH_VK_DECLARE)
  GH_VK_DECLARE(vkDestroyDevice)
#undef GH_VK_DECLARE
  VkInstance instance = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkPhysicalDevice physical = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkQueue queue = VK_NULL_HANDLE;
  uint32_t family = 0;
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  VkCommandPool pool = VK_NULL_HANDLE;
  VkCommandBuffer commands = VK_NULL_HANDLE;
  VkSemaphore acquired = VK_NULL_HANDLE;
  std::vector<VkImage> images;
  std::vector<VkSemaphore> rendered;

  ~ClearProbe() {
    if (device) {
      if (vkDeviceWaitIdle) vkDeviceWaitIdle(device);
      for (auto semaphore : rendered) {
        if (semaphore) vkDestroySemaphore(device, semaphore, nullptr);
      }
      if (acquired) vkDestroySemaphore(device, acquired, nullptr);
      if (pool) vkDestroyCommandPool(device, pool, nullptr);
      if (swapchain) vkDestroySwapchainKHR(device, swapchain, nullptr);
      vkDestroyDevice(device, nullptr);
    }
    if (surface) vkDestroySurfaceKHR(instance, surface, nullptr);
    if (instance) vkDestroyInstance(instance, nullptr);
  }

  bool Initialize() {
#define GH_VK_LOAD_GLOBAL(name) \
    name = reinterpret_cast<PFN_##name>(::vkGetInstanceProcAddr(VK_NULL_HANDLE, #name)); \
    if (!name) { Log("FAIL missing global function %s\n", #name); return false; }
    GH_VK_GLOBAL_FUNCTIONS(GH_VK_LOAD_GLOBAL)
#undef GH_VK_LOAD_GLOBAL
    const char* instance_extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME,
                                         VK_NN_VI_SURFACE_EXTENSION_NAME};
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "Guardian Heroes NVK probe";
    app.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    create.pApplicationInfo = &app;
    create.enabledExtensionCount = 2;
    create.ppEnabledExtensionNames = instance_extensions;
    if (!Check(vkCreateInstance(&create, nullptr, &instance), "vkCreateInstance")) return false;
#define GH_VK_LOAD_INSTANCE(name) \
    name = reinterpret_cast<PFN_##name>(::vkGetInstanceProcAddr(instance, #name)); \
    if (!name) { Log("FAIL missing instance function %s\n", #name); return false; }
    GH_VK_INSTANCE_FUNCTIONS(GH_VK_LOAD_INSTANCE)
    // Destruction remains available even when loading another device function fails.
    GH_VK_LOAD_INSTANCE(vkDestroyDevice)
#undef GH_VK_LOAD_INSTANCE
    Log("stage: Vulkan instance ready\n");

    auto create_surface = reinterpret_cast<PFN_vkCreateViSurfaceNN>(
        vkGetInstanceProcAddr(instance, "vkCreateViSurfaceNN"));
    if (!create_surface) {
      Log("FAIL VK_NN_vi_surface entry point unavailable\n");
      return false;
    }
    NWindow* window = nwindowGetDefault();
    const Result dimensions = nwindowSetDimensions(window, 1280, 720);
    if (R_FAILED(dimensions)) {
      Log("FAIL nwindowSetDimensions: 0x%08x\n", dimensions);
      return false;
    }
    VkViSurfaceCreateInfoNN vi{VK_STRUCTURE_TYPE_VI_SURFACE_CREATE_INFO_NN};
    vi.window = window;
    if (!Check(create_surface(instance, &vi, nullptr, &surface), "vkCreateViSurfaceNN")) return false;

    uint32_t count = 0;
    if (!Check(vkEnumeratePhysicalDevices(instance, &count, nullptr), "physical device count") || !count)
      return false;
    std::vector<VkPhysicalDevice> devices(count);
    if (!Check(vkEnumeratePhysicalDevices(instance, &count, devices.data()), "physical devices"))
      return false;
    for (auto candidate : devices) {
      uint32_t families = 0;
      vkGetPhysicalDeviceQueueFamilyProperties(candidate, &families, nullptr);
      std::vector<VkQueueFamilyProperties> properties(families);
      vkGetPhysicalDeviceQueueFamilyProperties(candidate, &families, properties.data());
      for (uint32_t i = 0; i < families; ++i) {
        VkBool32 present = VK_FALSE;
        if (!Check(vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface, &present),
                   "surface support")) return false;
        if (properties[i].queueCount && (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
          physical = candidate;
          family = i;
          break;
        }
      }
      if (physical) break;
    }
    if (!physical) {
      Log("FAIL no graphics/present queue\n");
      return false;
    }
    VkPhysicalDeviceProperties properties{};
    VkPhysicalDeviceFeatures features{};
    VkPhysicalDeviceMemoryProperties memory{};
    vkGetPhysicalDeviceProperties(physical, &properties);
    vkGetPhysicalDeviceFeatures(physical, &features);
    vkGetPhysicalDeviceMemoryProperties(physical, &memory);
    Log("device: %s API %u.%u.%u driver 0x%x\n", properties.deviceName,
        VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion),
        VK_VERSION_PATCH(properties.apiVersion), properties.driverVersion);
    Log("limits: maxImageDimension2D=%u maxStorageBufferRange=%u maxPushConstantsSize=%u\n",
        properties.limits.maxImageDimension2D, properties.limits.maxStorageBufferRange,
        properties.limits.maxPushConstantsSize);
    Log("features: geometry=%u tessellation=%u shaderInt64=%u fragmentStoresAndAtomics=%u\n",
        features.geometryShader, features.tessellationShader, features.shaderInt64,
        features.fragmentStoresAndAtomics);
    for (uint32_t i = 0; i < memory.memoryHeapCount; ++i)
      Log("heap %u: %llu bytes flags=0x%x\n", i,
          static_cast<unsigned long long>(memory.memoryHeaps[i].size), memory.memoryHeaps[i].flags);
    count = 0;
    if (!Check(vkEnumerateDeviceExtensionProperties(physical, nullptr, &count, nullptr),
               "device extension count")) return false;
    std::vector<VkExtensionProperties> extensions(count);
    if (!Check(vkEnumerateDeviceExtensionProperties(physical, nullptr, &count, extensions.data()),
               "device extensions")) return false;
    for (const auto& extension : extensions) Log("extension: %s\n", extension.extensionName);
    const char* device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    const float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_create{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue_create.queueFamilyIndex = family;
    queue_create.queueCount = 1;
    queue_create.pQueuePriorities = &priority;
    VkDeviceCreateInfo device_create{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_create.queueCreateInfoCount = 1;
    device_create.pQueueCreateInfos = &queue_create;
    device_create.enabledExtensionCount = 1;
    device_create.ppEnabledExtensionNames = device_extensions;
    if (!Check(vkCreateDevice(physical, &device_create, nullptr, &device), "vkCreateDevice")) return false;
#define GH_VK_LOAD_DEVICE(name) \
    name = reinterpret_cast<PFN_##name>(vkGetDeviceProcAddr(device, #name)); \
    if (!name) { Log("FAIL missing device function %s\n", #name); return false; }
    GH_VK_DEVICE_FUNCTIONS(GH_VK_LOAD_DEVICE)
#undef GH_VK_LOAD_DEVICE
    vkGetDeviceQueue(device, family, 0, &queue);
    VkCommandPoolCreateInfo pool_create{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_create.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_create.queueFamilyIndex = family;
    if (!Check(vkCreateCommandPool(device, &pool_create, nullptr, &pool), "command pool")) return false;
    VkCommandBufferAllocateInfo allocate{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocate.commandPool = pool;
    allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate.commandBufferCount = 1;
    if (!Check(vkAllocateCommandBuffers(device, &allocate, &commands), "command buffer")) return false;
    VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    return Check(vkCreateSemaphore(device, &semaphore, nullptr, &acquired), "acquire semaphore") &&
           CreateSwapchain();
  }

  bool CreateSwapchain() {
    if (!Check(vkDeviceWaitIdle(device), "idle before swapchain")) return false;
    for (auto semaphore : rendered) if (semaphore) vkDestroySemaphore(device, semaphore, nullptr);
    rendered.clear();
    VkSurfaceCapabilitiesKHR caps{};
    if (!Check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical, surface, &caps), "surface capabilities"))
      return false;
    if (!(caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
      Log("FAIL surface lacks TRANSFER_DST for clear probe\n");
      return false;
    }
    uint32_t count = 0;
    if (!Check(vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &count, nullptr),
               "surface format count") || !count) return false;
    std::vector<VkSurfaceFormatKHR> formats(count);
    if (!Check(vkGetPhysicalDeviceSurfaceFormatsKHR(physical, surface, &count, formats.data()),
               "surface formats")) return false;
    VkSurfaceFormatKHR format = formats[0];
    for (const auto& candidate : formats) {
      if (candidate.format == VK_FORMAT_R8G8B8A8_UNORM) {
        format = candidate;
        break;
      }
    }
    if (format.format == VK_FORMAT_UNDEFINED) format.format = VK_FORMAT_R8G8B8A8_UNORM;
    VkExtent2D extent = caps.currentExtent;
    if (extent.width == std::numeric_limits<uint32_t>::max()) {
      extent.width = std::clamp(1280u, caps.minImageExtent.width, caps.maxImageExtent.width);
      extent.height = std::clamp(720u, caps.minImageExtent.height, caps.maxImageExtent.height);
    }
    if (!extent.width || !extent.height) {
      Log("FAIL surface extent is zero\n");
      return false;
    }
    VkSwapchainCreateInfoKHR create{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    create.surface = surface;
    create.minImageCount = caps.minImageCount + 1;
    if (caps.maxImageCount) create.minImageCount = std::min(create.minImageCount, caps.maxImageCount);
    create.imageFormat = format.format;
    create.imageColorSpace = format.colorSpace;
    create.imageExtent = extent;
    create.imageArrayLayers = 1;
    create.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    create.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create.preTransform = caps.currentTransform;
    for (auto alpha : {VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
                       VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR, VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR}) {
      if (caps.supportedCompositeAlpha & alpha) {
        create.compositeAlpha = alpha;
        break;
      }
    }
    create.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    create.clipped = VK_TRUE;
    create.oldSwapchain = swapchain;
    VkSwapchainKHR replacement = VK_NULL_HANDLE;
    if (!Check(vkCreateSwapchainKHR(device, &create, nullptr, &replacement), "vkCreateSwapchainKHR"))
      return false;
    if (swapchain) vkDestroySwapchainKHR(device, swapchain, nullptr);
    swapchain = replacement;
    count = 0;
    if (!Check(vkGetSwapchainImagesKHR(device, swapchain, &count, nullptr), "swapchain image count"))
      return false;
    images.resize(count);
    if (!Check(vkGetSwapchainImagesKHR(device, swapchain, &count, images.data()), "swapchain images"))
      return false;
    rendered.resize(count, VK_NULL_HANDLE);
    VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    for (auto& signal : rendered)
      if (!Check(vkCreateSemaphore(device, &semaphore, nullptr, &signal), "render semaphore")) return false;
    Log("stage: swapchain %ux%u format=%u images=%u\n", extent.width, extent.height, format.format, count);
    return true;
  }

  bool Frame(uint64_t frame) {
    uint32_t image = 0;
    const VkResult acquire = vkAcquireNextImageKHR(device, swapchain, 1000000000ull,
                                                   acquired, VK_NULL_HANDLE, &image);
    if (acquire == VK_TIMEOUT || acquire == VK_NOT_READY) {
      Log("acquire image waiting: VkResult %d\n", static_cast<int>(acquire));
      return true;
    }
    if (acquire == VK_ERROR_OUT_OF_DATE_KHR) return CreateSwapchain();
    if (acquire != VK_SUBOPTIMAL_KHR && !Check(acquire, "acquire image")) return false;
    if (!Check(vkResetCommandBuffer(commands, 0), "reset commands")) return false;
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (!Check(vkBeginCommandBuffer(commands, &begin), "begin commands")) return false;
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = images[image];
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    // A one-second color step makes a stalled presentation visible without shaders.
    VkClearColorValue color{{0.05f, 0.10f, 0.15f, 1.0f}};
    color.float32[(frame / 60) % 3] = 0.6f;
    vkCmdClearColorImage(commands, images[image], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         &color, 1, &barrier.subresourceRange);
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = 0;
    vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    if (!Check(vkEndCommandBuffer(commands), "end commands")) return false;
    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &acquired;
    submit.pWaitDstStageMask = &wait_stage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commands;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &rendered[image];
    if (!Check(vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE), "queue submit")) return false;
    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &rendered[image];
    present.swapchainCount = 1;
    present.pSwapchains = &swapchain;
    present.pImageIndices = &image;
    const VkResult result = vkQueuePresentKHR(queue, &present);
    // Deliberately serialized: this diagnoses presentation, not game performance.
    if (!Check(vkQueueWaitIdle(queue), "present queue idle")) return false;
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) return CreateSwapchain();
    if (!Check(result, "queue present")) return false;
    if (frame % 300 == 0) Log("presented frame: %llu\n", static_cast<unsigned long long>(frame));
    return true;
  }
};
#undef GH_VK_GLOBAL_FUNCTIONS
#undef GH_VK_INSTANCE_FUNCTIONS
#undef GH_VK_DEVICE_FUNCTIONS
}  // namespace

int main(int, char**) {
  mkdir("sdmc:/switch", 0777);
  mkdir("sdmc:/switch/guardian-heroes", 0777);
  mkdir("sdmc:/switch/guardian-heroes/logs", 0777);
  log_file = fopen("sdmc:/switch/guardian-heroes/logs/vulkan-probe.log", "a");
  const auto type = appletGetAppletType();
  Log("\nGuardian Heroes NVK presentation probe, applet type=%u\n", static_cast<unsigned>(type));
  bool ok = false;
  if (type == AppletType_Application) {
    ClearProbe probe;
    ok = probe.Initialize();
    for (uint64_t frame = 0; ok && appletMainLoop(); ++frame) ok = probe.Frame(frame);
  } else {
    Log("FAIL launch through Application-mode hbmenu; album/applet mode is not accepted\n");
  }
  if (!ok) {
    consoleInit(nullptr);
    printf("Guardian Heroes NVK probe stopped.\nSee sdmc:/switch/guardian-heroes/logs/vulkan-probe.log\n");
    if (!log_file) printf("SD log could not be opened.\n");
    while (appletMainLoop()) consoleUpdate(nullptr);
    consoleExit(nullptr);
  }
  if (log_file) fclose(log_file);
  return ok ? 0 : 1;
}
