#include "gh_gpu_vulkan_command_processor.h"
#include "gh_gpu_vulkan_render_target_cache.h"
#include <rex/stream.h>
#include <algorithm>
#include <bit>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

namespace rex::graphics::vulkan {
struct RenderTargetSnapshotTestAccess {
  static bool Run(VulkanCommandProcessor& cp) {
    auto& rt = *cp.render_target_cache_;
    using Key = VulkanRenderTargetCache::RenderTargetKey;
    assert(cp.is_paused());
    assert(rt.GetPath() == RenderTargetCache::Path::kHostRenderTargets);
    std::vector<Key> keys;
    auto add = [&](bool depth, uint32_t format, xenos::MsaaSamples samples) {
      Key key;
      key.base_tiles = uint32_t(keys.size()) * 100;
      key.pitch_tiles_at_32bpp = 16;
      key.is_depth = depth;
      key.resource_format = format;
      key.msaa_samples = samples;
      keys.push_back(key);
      return key;
    };
    add(false, uint32_t(xenos::ColorRenderTargetFormat::k_8_8_8_8), xenos::MsaaSamples::k4X);
    add(false, uint32_t(xenos::ColorRenderTargetFormat::k_16_16_16_16), xenos::MsaaSamples::k4X);
    add(false, uint32_t(xenos::ColorRenderTargetFormat::k_32_32_FLOAT), xenos::MsaaSamples::k2X);
    auto unorm = add(true, uint32_t(xenos::DepthRenderTargetFormat::kD24S8), xenos::MsaaSamples::k1X);
    auto floating = add(true, uint32_t(xenos::DepthRenderTargetFormat::kD24FS8), xenos::MsaaSamples::k4X);
    std::sort(keys.begin(), keys.end(), [](Key a, Key b) { return a.key < b.key; });
    constexpr size_t capacity = 256 * 1024 * 1024;
    std::vector<uint8_t> expected(capacity), actual(capacity);
    stream::ByteStream input(expected.data(), expected.size());
    input.Write<uint32_t>(0x31535452);
    input.Write<uint32_t>(uint32_t(rt.GetPath()));
    input.Write(rt.draw_resolution_scale_x());
    input.Write(rt.draw_resolution_scale_y());
    std::vector<uint8_t> edram(size_t(xenos::kEdramSizeBytes) * rt.draw_resolution_scale_x() *
                               rt.draw_resolution_scale_y());
    for (size_t i = 0; i < edram.size(); ++i) edram[i] = uint8_t(i * 13 + 91);
    input.Write(std::span<const uint8_t>(edram));
    input.Write<uint32_t>(uint32_t(keys.size()));
    const auto max_buffer = std::min<VkDeviceSize>(64 * 1024 * 1024,
        cp.GetVulkanDevice()->properties().maxStorageBufferRange);
    for (Key key : keys) {
      const auto format = key.is_depth ? rt.GetDepthVulkanFormat(key.GetDepthFormat())
                                       : rt.GetColorVulkanFormat(key.GetColorFormat());
      const uint32_t width = key.GetWidth() * rt.draw_resolution_scale_x();
      const uint32_t height = rt.GetRenderTargetHeight(key.pitch_tiles_at_32bpp, key.msaa_samples) *
                              rt.draw_resolution_scale_y();
      const uint32_t samples = key.msaa_samples == xenos::MsaaSamples::k2X &&
                                      !rt.msaa_2x_attachments_supported()
                                  ? 4 : 1u << uint32_t(key.msaa_samples);
      const size_t row_bytes = size_t(width) * samples * 8;
      const uint32_t chunk_rows = uint32_t(std::min<VkDeviceSize>(height, max_buffer / row_bytes));
      input.Write(key.key);
      input.Write<uint32_t>(format);
      input.Write(width);
      input.Write(height);
      input.Write(samples);
      input.Write(chunk_rows);
      for (uint32_t y = 0; y < height; y += chunk_rows) {
        const uint32_t rows = std::min(chunk_rows, height - y);
        std::vector<uint32_t> pixels(size_t(width) * rows * samples * 2);
        for (size_t i = 0; i < pixels.size() / 2; ++i) {
          const uint32_t sample = uint32_t(i % samples);
          const uint32_t pattern = uint32_t(i) * 1664525u + y * 1013904223u + sample * 17;
          if (key.is_depth) {
            // Float32 precision here exceeds the guest's 24-bit representation.
            float depth = std::bit_cast<float>(0x3E123457u + (pattern & 0x3FFFFu));
            if (format == VK_FORMAT_D24_UNORM_S8_UINT) {
              // Endpoints are exact in both UNORM24 and sampled float32.
              depth = (pattern & 1) ? 1.0f : 0.0f;
            }
            pixels[i * 2] = std::bit_cast<uint32_t>(depth);
            pixels[i * 2 + 1] = (pattern ^ (pattern >> 8)) & 255;
          } else if (format == VK_FORMAT_R8G8B8A8_UNORM) {
            pixels[i * 2] = pattern;
            pixels[i * 2 + 1] = 0;
          } else if (format == VK_FORMAT_R32G32_SFLOAT) {
            // Raw integer views must retain distinct NaN payloads and signs.
            pixels[i * 2] = 0x7FC00000u | (pattern & 0x3FFFFFu);
            pixels[i * 2 + 1] = 0xFFC00000u | ((pattern >> 3) & 0x3FFFFFu);
          } else {
            // SNORM -32768 and -32767 both represent -1; both bits must survive.
            pixels[i * 2] = (pattern & 1) ? 0x80008001u : 0x80018000u;
            pixels[i * 2 + 1] = pattern;
          }
        }
        input.Write(pixels.data(), pixels.size() * sizeof(uint32_t));
      }
    }
    input.Write<uint32_t>(uint32_t(keys.size()));
    for (uint32_t i = 0; i < keys.size(); ++i) {
      input.Write<uint32_t>(i * 100);
      input.Write<uint32_t>(i + 1 == keys.size() ? xenos::kEdramTileCount : (i + 1) * 100);
      input.Write(keys[i].key);
      // Keep both precision owners independently of the primary color owner.
      input.Write(unorm.key);
      input.Write(floating.key);
    }
    stream::ByteStream restore(expected.data(), input.offset());
    assert(rt.RestoreState(&restore));
    assert(restore.offset() == input.offset());
    stream::ByteStream save(actual.data(), actual.size());
    assert(rt.SaveState(&save));
    assert(save.offset() == input.offset());
    if (std::memcmp(expected.data(), actual.data(), input.offset())) {
      size_t mismatch = 0;
      while (expected[mismatch] == actual[mismatch]) ++mismatch;
      std::fprintf(stderr, "HostRT snapshot mismatch at byte %zu: %02x != %02x\n",
                   mismatch, expected[mismatch], actual[mismatch]);
      return false;
    }
    // Replay also covers existing images and their tracked attachment layout.
    stream::ByteStream replay(actual.data(), save.offset());
    assert(rt.RestoreState(&replay));
    std::puts("HostRT snapshot: raw 32/64-bit colors, NaN/SNORM encodings, distinct MSAA samples, float32 depth, stencil, EDRAM and independent depth owners passed");
    return true;
  }
};
bool RunRenderTargetSnapshotTest(VulkanCommandProcessor& cp) {
  return RenderTargetSnapshotTestAccess::Run(cp);
}
}  // namespace rex::graphics::vulkan
