#include "gh_graphics.h"

#include <cassert>

namespace {

class ContractBackend final : public gh::graphics::Backend {
 public:
  gh::graphics::Status CreateResource(const gh::graphics::ResourceDesc& desc,
                                      gh::graphics::ResourceHandle* out) override {
    if (!out || !gh::graphics::IsValid(desc)) return gh::graphics::Status::kInvalidArgument;
    *out = {++next_handle_};
    return gh::graphics::Status::kSuccess;
  }
  gh::graphics::Status DestroyResource(gh::graphics::ResourceHandle resource) override {
    return resource ? gh::graphics::Status::kSuccess : gh::graphics::Status::kInvalidArgument;
  }
  gh::graphics::Status CreatePipeline(const gh::graphics::PipelineDesc&,
                                      gh::graphics::PipelineHandle* out) override {
    if (!out) return gh::graphics::Status::kInvalidArgument;
    *out = {++next_handle_};
    return gh::graphics::Status::kSuccess;
  }
  gh::graphics::Status DestroyPipeline(gh::graphics::PipelineHandle pipeline) override {
    return pipeline ? gh::graphics::Status::kSuccess : gh::graphics::Status::kInvalidArgument;
  }
  gh::graphics::Status Submit(std::span<const gh::graphics::Command> commands,
                              gh::graphics::Fence* out) override {
    if (!out || commands.empty()) return gh::graphics::Status::kInvalidArgument;
    *out = {++next_fence_};
    return gh::graphics::Status::kSuccess;
  }
  gh::graphics::Status Wait(gh::graphics::Fence fence, uint64_t) override {
    return fence ? gh::graphics::Status::kSuccess : gh::graphics::Status::kInvalidArgument;
  }
  gh::graphics::Presenter& presenter() override { return presenter_; }

 private:
  class TestPresenter final : public gh::graphics::Presenter {
   public:
    gh::graphics::Status AcquireFrame(gh::graphics::Frame* frame) override {
      if (!frame) return gh::graphics::Status::kInvalidArgument;
      *frame = {{99}, ++frame_id_};
      return gh::graphics::Status::kSuccess;
    }
    gh::graphics::Status Present(const gh::graphics::Frame& frame,
                                 gh::graphics::Fence after) override {
      return frame.color_target && after ? gh::graphics::Status::kSuccess
                                         : gh::graphics::Status::kInvalidArgument;
    }

   private:
    uint64_t frame_id_ = 0;
  } presenter_;
  uint64_t next_handle_ = 0;
  uint64_t next_fence_ = 0;
};

}  // namespace

int main() {
  using namespace gh::graphics;

  const ResourceDesc buffer{ResourceKind::kBuffer, 256, 0, 0, 1,
                            PixelFormat::kUnknown, kResourceVertexBuffer};
  const ResourceDesc texture{ResourceKind::kTexture, 0, 64, 32, 1,
                            PixelFormat::kRgba8Unorm, kResourceRenderTarget};
  assert(IsValid(buffer));
  assert(IsValid(texture));
  assert(!IsValid(ResourceDesc{}));

  CommandList list;
  const ResourceHandle color{1}, vertices{2}, indices{3}, copy{4};
  const PipelineHandle pipeline{5};
  assert(!list.SetRenderTarget({}));
  assert(list.SetRenderTarget(color));
  assert(list.ClearColor(color, {0.1f, 0.2f, 0.3f, 1.0f}));
  assert(list.DrawIndexed(pipeline, vertices, indices, 6, 2, -1));
  assert(list.CopyResource(color, copy));
  assert(list.Resolve(color, copy));
  assert(!list.CopyResource(color, color));
  assert(list.commands().size() == 5);
  assert(list.commands()[2].type == CommandType::kDrawIndexed);
  assert(list.commands()[2].index_count == 6);
  assert(list.commands()[2].first_index == 2);
  assert(list.commands()[2].vertex_offset == -1);
  list.Reset();
  assert(list.empty());

  ContractBackend backend;
  ResourceHandle created;
  assert(backend.CreateResource(texture, &created) == Status::kSuccess);
  assert(created);
  PipelineHandle created_pipeline;
  assert(backend.CreatePipeline({}, &created_pipeline) == Status::kSuccess);
  CommandList submission;
  assert(submission.SetRenderTarget(created));
  Fence submitted;
  assert(backend.Submit(submission.commands(), &submitted) == Status::kSuccess);
  assert(backend.Wait(submitted, 1'000'000) == Status::kSuccess);
  Frame frame;
  assert(backend.presenter().AcquireFrame(&frame) == Status::kSuccess);
  assert(backend.presenter().Present(frame, submitted) == Status::kSuccess);
  assert(backend.DestroyPipeline(created_pipeline) == Status::kSuccess);
  assert(backend.DestroyResource(created) == Status::kSuccess);
}
