#include "gh_graphics.h"

namespace gh::graphics {
namespace {

bool Valid(ResourceHandle handle) noexcept { return static_cast<bool>(handle); }
bool Valid(PipelineHandle handle) noexcept { return static_cast<bool>(handle); }

}  // namespace

bool CommandList::SetRenderTarget(ResourceHandle target) {
  if (!Valid(target)) return false;
  Command command;
  command.type = CommandType::kSetRenderTarget;
  command.target = target;
  commands_.push_back(command);
  return true;
}

bool CommandList::ClearColor(ResourceHandle target, Color color) {
  if (!Valid(target)) return false;
  Command command;
  command.type = CommandType::kClearColor;
  command.target = target;
  command.clear_color = color;
  commands_.push_back(command);
  return true;
}

bool CommandList::DrawIndexed(PipelineHandle pipeline, ResourceHandle vertex_buffer,
                              ResourceHandle index_buffer, uint32_t index_count,
                              uint32_t first_index, int32_t vertex_offset) {
  if (!Valid(pipeline) || !Valid(vertex_buffer) || !Valid(index_buffer) || !index_count) {
    return false;
  }
  Command command;
  command.type = CommandType::kDrawIndexed;
  command.pipeline = pipeline;
  command.vertex_buffer = vertex_buffer;
  command.index_buffer = index_buffer;
  command.index_count = index_count;
  command.first_index = first_index;
  command.vertex_offset = vertex_offset;
  commands_.push_back(command);
  return true;
}

bool CommandList::CopyResource(ResourceHandle source, ResourceHandle destination) {
  if (!Valid(source) || !Valid(destination) || source == destination) return false;
  Command command;
  command.type = CommandType::kCopyResource;
  command.source = source;
  command.target = destination;
  commands_.push_back(command);
  return true;
}

bool CommandList::Resolve(ResourceHandle source, ResourceHandle destination) {
  if (!Valid(source) || !Valid(destination) || source == destination) return false;
  Command command;
  command.type = CommandType::kResolve;
  command.source = source;
  command.target = destination;
  commands_.push_back(command);
  return true;
}

bool IsValid(const ResourceDesc& desc) noexcept {
  if (!desc.usage || !desc.mip_levels) return false;
  if (desc.kind == ResourceKind::kBuffer) {
    return desc.byte_size != 0 && desc.width == 0 && desc.height == 0 &&
           desc.format == PixelFormat::kUnknown;
  }
  return desc.byte_size == 0 && desc.width != 0 && desc.height != 0 &&
         desc.format != PixelFormat::kUnknown;
}

}  // namespace gh::graphics
