#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace gh::graphics {

enum class Status : uint8_t {
  kSuccess,
  kInvalidArgument,
  kInvalidState,
  kUnsupported,
  kDeviceLost,
  kTimeout,
};

struct ResourceHandle {
  uint64_t value = 0;
  constexpr explicit operator bool() const noexcept { return value != 0; }
  friend constexpr bool operator==(ResourceHandle, ResourceHandle) = default;
};

struct PipelineHandle {
  uint64_t value = 0;
  constexpr explicit operator bool() const noexcept { return value != 0; }
  friend constexpr bool operator==(PipelineHandle, PipelineHandle) = default;
};

struct Fence {
  uint64_t value = 0;
  constexpr explicit operator bool() const noexcept { return value != 0; }
};

enum class ResourceKind : uint8_t { kBuffer, kTexture };
enum class PixelFormat : uint8_t {
  kUnknown,
  kR8Unorm,
  kRgba8Unorm,
  kBgra8Unorm,
  kRgb10A2Unorm,
  kRgba16Float,
  kDepth24Stencil8,
};

enum ResourceUsage : uint32_t {
  kResourceVertexBuffer = 1u << 0,
  kResourceIndexBuffer = 1u << 1,
  kResourceSampled = 1u << 2,
  kResourceRenderTarget = 1u << 3,
  kResourceDepthStencil = 1u << 4,
  kResourceCopySource = 1u << 5,
  kResourceCopyDestination = 1u << 6,
};

struct ResourceDesc {
  ResourceKind kind = ResourceKind::kBuffer;
  uint64_t byte_size = 0;
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t mip_levels = 1;
  PixelFormat format = PixelFormat::kUnknown;
  uint32_t usage = 0;
};

struct PipelineDesc {
  std::span<const std::byte> vertex_shader;
  std::span<const std::byte> pixel_shader;
  PixelFormat color_format = PixelFormat::kUnknown;
  PixelFormat depth_format = PixelFormat::kUnknown;
};

struct Color {
  float r = 0;
  float g = 0;
  float b = 0;
  float a = 1;
};

enum class CommandType : uint8_t {
  kSetRenderTarget,
  kClearColor,
  kDrawIndexed,
  kCopyResource,
  kResolve,
};

struct Command {
  CommandType type = CommandType::kSetRenderTarget;
  ResourceHandle target;
  ResourceHandle source;
  ResourceHandle vertex_buffer;
  ResourceHandle index_buffer;
  PipelineHandle pipeline;
  Color clear_color;
  uint32_t index_count = 0;
  uint32_t first_index = 0;
  int32_t vertex_offset = 0;
};

// Owns CPU-side commands until submission. Resource handles must remain alive
// until the fence returned by the backend has completed.
class CommandList {
 public:
  bool SetRenderTarget(ResourceHandle target);
  bool ClearColor(ResourceHandle target, Color color);
  bool DrawIndexed(PipelineHandle pipeline, ResourceHandle vertex_buffer,
                   ResourceHandle index_buffer, uint32_t index_count,
                   uint32_t first_index = 0, int32_t vertex_offset = 0);
  bool CopyResource(ResourceHandle source, ResourceHandle destination);
  bool Resolve(ResourceHandle source, ResourceHandle destination);

  std::span<const Command> commands() const noexcept { return commands_; }
  bool empty() const noexcept { return commands_.empty(); }
  void Reset() noexcept { commands_.clear(); }

 private:
  std::vector<Command> commands_;
};

struct Frame {
  // Presenter-owned target. It is valid for this acquired frame and is not
  // released through Backend::DestroyResource.
  ResourceHandle color_target;
  uint64_t id = 0;
};

class Presenter {
 public:
  virtual ~Presenter() = default;
  virtual Status AcquireFrame(Frame* frame) = 0;
  virtual Status Present(const Frame& frame, Fence after) = 0;
};

// Backend contract only. This project-owned interface currently has no
// implementation; the active game path still uses the SDK Xenos plugin.
class Backend {
 public:
  virtual ~Backend() = default;
  virtual Status CreateResource(const ResourceDesc& desc, ResourceHandle* out) = 0;
  // Implementations may defer physical release until prior submissions finish.
  virtual Status DestroyResource(ResourceHandle resource) = 0;
  virtual Status CreatePipeline(const PipelineDesc& desc, PipelineHandle* out) = 0;
  virtual Status DestroyPipeline(PipelineHandle pipeline) = 0;
  virtual Status Submit(std::span<const Command> commands, Fence* out) = 0;
  // Returns success when complete, kTimeout when the duration expires,
  // invalid argument for an empty fence, or kDeviceLost.
  virtual Status Wait(Fence fence, uint64_t timeout_ns) = 0;
  virtual Presenter& presenter() = 0;
};

bool IsValid(const ResourceDesc& desc) noexcept;

}  // namespace gh::graphics
