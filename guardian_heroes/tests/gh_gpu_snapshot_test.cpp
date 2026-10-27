#include "gh_system_runtime.h"
#include "gh_gpu_vulkan_command_processor.h"
#include "gh_gpu_vulkan_graphics_system.h"
#include <rex/stream.h>
#include <rex/ui/vulkan/util.h>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>

namespace rex::graphics::vulkan {
bool RunRenderTargetSnapshotTest(VulkanCommandProcessor&);
struct VulkanSnapshotTestAccess {
  static inline uint32_t kAddress = 0;
  static constexpr uint32_t kLength = 8192;
  static bool Copy(VulkanCommandProcessor& cp, std::vector<uint8_t>& bytes, bool upload) {
    return cp.TransferSnapshotBuffer(bytes, upload, 0, [&](VkCommandBuffer cmd, VkBuffer staging) {
      VkBufferCopy copy{upload ? 0 : kAddress, upload ? kAddress : 0, bytes.size()};
      cp.GetVulkanDevice()->functions().vkCmdCopyBuffer(cmd,
          upload ? staging : cp.shared_memory_->buffer(),
          upload ? cp.shared_memory_->buffer() : staging, 1, &copy);
    });
  }
  static void Run(VulkanGraphicsSystem& gpu) {
    auto& cp = *static_cast<VulkanCommandProcessor*>(gpu.command_processor());
    assert(cp.is_paused());
    uint32_t allocation = cp.memory_->SystemHeapAlloc(kLength, 4096, memory::kSystemHeapPhysical);
    assert(allocation);
    kAddress = cp.memory_->GetPhysicalAddress(allocation);
    assert(kAddress < SharedMemory::kBufferSize - kLength);
    std::memset(cp.memory_->TranslatePhysical(kAddress), 0, kLength);
    cp.InitializeRingBuffer(kAddress, 10);
    cp.read_ptr_update_freq_ = 32;
    cp.counter_ = 0x13579;
    cp.WriteRegister(XE_GPU_REG_DC_LUT_WRITE_EN_MASK, 7);
    cp.WriteRegister(XE_GPU_REG_DC_LUT_RW_MODE, 0);
    cp.WriteRegister(XE_GPU_REG_DC_LUT_RW_INDEX, 42);
    cp.WriteRegister(XE_GPU_REG_DC_LUT_SEQ_COLOR, 0x12340);
    cp.WriteRegister(XE_GPU_REG_DC_LUT_RW_MODE, 1);
    cp.WriteRegister(XE_GPU_REG_DC_LUT_RW_INDEX, 43);
    cp.WriteRegister(XE_GPU_REG_DC_LUT_PWL_DATA, 0x12345678);
    cp.WriteRegister(XE_GPU_REG_DC_LUT_PWL_DATA, 0x23456789);
    cp.extended_register_values_[0x6000] = 0x87654321;
    gpu.register_file()->values[0x2000] = 0x12345678;
    cp.guest_commands_.RestoreBinState(0x123456789ABCDEF0ull, 0xFEDCBA9876543210ull);
    std::array<uint32_t, 3> code{0, 0, 0};
    cp.active_vertex_shader_ = cp.LoadShader(xenos::ShaderType::kVertex, 0, code.data(), code.size());
    code[0] = 1;
    cp.active_pixel_shader_ = cp.LoadShader(xenos::ShaderType::kPixel, 0, code.data(), code.size());
    assert(cp.BeginSubmission(false));
    assert(cp.shared_memory_->RequestRange(kAddress, kLength));
    std::vector<uint8_t> expected(kLength);
    for (size_t i = 0; i < expected.size(); ++i) expected[i] = uint8_t(i * 29 + 71);
    assert(Copy(cp, expected, true));
    cp.shared_memory_->RangeWrittenByGpu(kAddress, kLength);
    // The main-memory copy deliberately differs from authoritative GPU data.
    assert(std::memcmp(cp.memory_->TranslatePhysical(kAddress), expected.data(), kLength));

    auto& history = cp.memexport_readback_buffers_[0x1234];
    history.current_index = 1;
    history.last_used_frame = cp.frame_current_;
    history.sizes[0] = 4096;
    history.written_size[0] = 64;
    history.submission_written[0] = cp.GetCompletedSubmission();
    auto* vd = cp.GetVulkanDevice();
    assert(ui::vulkan::util::CreateDedicatedAllocationBuffer(vd, 4096,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT, ui::vulkan::util::MemoryPurpose::kReadback,
        history.buffers[0], history.memories[0]));
    assert(vd->functions().vkMapMemory(vd->device(), history.memories[0], 0, VK_WHOLE_SIZE, 0,
        &history.mapped_data[0]) == VK_SUCCESS);
    std::memset(history.mapped_data[0], 0xB7, 64);
    VkMappedMemoryRange range{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
    range.memory = history.memories[0];
    range.size = VK_WHOLE_SIZE;
    assert(vd->functions().vkFlushMappedMemoryRanges(vd->device(), 1, &range) == VK_SUCCESS);

    std::array<uint8_t, 128 * 1024> base_before{}, base_after{};
    stream::ByteStream before(base_before.data(), base_before.size());
    assert(cp.CommandProcessor::Save(&before));
    assert(cp.BeginGuestOcclusionQuery(kAddress));
    cp.active_occlusion_query_.accumulated_samples = 197;
    std::array<uint8_t, 64> too_small{};
    stream::ByteStream limited(too_small.data(), too_small.size());
    bool refused = false;
    try { cp.Save(&limited); }
    catch (const std::out_of_range&) { refused = true; }
    assert(refused && cp.active_occlusion_query_.valid &&
        cp.active_occlusion_query_.accumulated_samples == 197);
    auto snapshot = std::make_unique_for_overwrite<uint8_t[]>(1024ull * 1024 * 1024);
    stream::ByteStream save(snapshot.get(), 1024ull * 1024 * 1024);
    assert(gpu.Save(&save));
    assert(cp.active_occlusion_query_.valid);
    assert(cp.active_occlusion_query_.accumulated_samples == 197);
    uint64_t samples;
    assert(cp.DrainSnapshotQuery(samples) && samples == 197);
    cp.counter_ = 0;
    cp.primary_buffer_ptr_ = cp.primary_buffer_size_ = cp.read_ptr_update_freq_ = 0;
    cp.WriteRegister(XE_GPU_REG_DC_LUT_RW_MODE, 0);
    cp.WriteRegister(XE_GPU_REG_DC_LUT_RW_INDEX, 42);
    cp.WriteRegister(XE_GPU_REG_DC_LUT_SEQ_COLOR, 0xABCC0);
    cp.WriteRegister(XE_GPU_REG_DC_LUT_RW_MODE, 1);
    cp.WriteRegister(XE_GPU_REG_DC_LUT_RW_INDEX, 43);
    cp.WriteRegister(XE_GPU_REG_DC_LUT_PWL_DATA, 0xABCD1234);
    cp.extended_register_values_.clear();
    gpu.register_file()->values[0x2000] = 0;
    cp.guest_commands_.RestoreBinState(0, 0);
    cp.active_vertex_shader_ = nullptr;
    cp.active_pixel_shader_ = nullptr;
    std::vector<uint8_t> changed(kLength, 0xDA);
    assert(Copy(cp, changed, true));
    std::memset(history.mapped_data[0], 0, 64);
    stream::ByteStream restore(snapshot.get(), save.offset());
    assert(gpu.Restore(&restore));
    assert(restore.offset() == save.offset());
    assert(cp.active_occlusion_query_.valid);
    assert(cp.DrainSnapshotQuery(samples) && samples == 197);
    stream::ByteStream after(base_after.data(), base_after.size());
    assert(cp.CommandProcessor::Save(&after));
    assert(before.offset() == after.offset());
    assert(!std::memcmp(base_before.data(), base_after.data(), before.offset()));
    assert(Copy(cp, changed, false));
    assert(changed == expected);
    const auto& restored = cp.memexport_readback_buffers_.at(0x1234);
    assert(restored.current_index == 1 && restored.written_size[0] == 64);
    for (uint32_t i = 0; i < 64; ++i)
      assert(static_cast<const uint8_t*>(restored.mapped_data[0])[i] == 0xB7);
    assert(RunRenderTargetSnapshotTest(cp));
    std::printf("Vulkan snapshot: CPU/PM4/shaders, authoritative GPU pages, delayed readback, active query roundtrip passed (%zu bytes)\n", save.offset());
  }
};
}

int main() {
  using rex::X_STATUS;
  rex::Runtime runtime({});
  rex::RuntimeConfig config;
  config.tool_mode = true;
  assert(runtime.Setup(std::move(config)) == X_STATUS_SUCCESS);
  rex::graphics::vulkan::VulkanGraphicsSystem gpu;
  assert(gpu.SetupGuestGpu(runtime.function_dispatcher(), runtime.kernel_state()) == X_STATUS_SUCCESS);
  gpu.Pause();
  rex::graphics::vulkan::VulkanSnapshotTestAccess::Run(gpu);
  gpu.Shutdown();
  std::puts("Vulkan paused snapshot shutdown passed");
}
