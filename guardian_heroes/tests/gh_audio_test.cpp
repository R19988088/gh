#include "gh_audio.h"
#include "gh_audio_conversion.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <bit>
#include <cmath>
#include <vector>

int main() {
  using namespace gh::audio;

  const std::array<uint8_t, 8> pcm = {0x00, 0x80, 0x00, 0x00,
                                      0x00, 0x40, 0x00, 0xC0};
  std::vector<float> decoded;
  assert(DecodePcm16LePacket(pcm, {48'000, 2}, decoded) == DecodeResult::kOk);
  assert(decoded.size() == 4);
  assert(decoded[0] == -1.0f);
  assert(decoded[1] == 0.0f);
  assert(decoded[2] > 0.49f && decoded[2] < 0.51f);
  assert(decoded[3] < -0.49f && decoded[3] > -0.51f);
  PcmFrames packet_frames;
  assert(DecodePacket(PacketCodec::kPcm16Le, pcm, {48'000, 2}, packet_frames) ==
         DecodeResult::kOk);
  assert(packet_frames.FrameCount() == 2);
  assert(DecodePacket(PacketCodec::kXma2, pcm, {48'000, 2}, packet_frames) ==
         DecodeResult::kUnsupportedCodec);

  AudioFormat format{48'000, 2};
  MixerQueue queue(format, 2);
  assert(queue.Enqueue(decoded));
  assert(!queue.Enqueue(decoded));
  std::array<float, 6> output{};
  queue.MixInto(output);
  assert(output[0] == -1.0f && output[1] == 0.0f);
  assert(output[2] > 0.49f && output[2] < 0.51f);
  assert(output[4] == 0.0f && output[5] == 0.0f);
  assert(queue.GetStats().overflow_frames == 2);
  assert(queue.GetStats().underrun_frames == 1);

  assert(queue.Enqueue(std::span<const float>(decoded.data(), 2)));
  queue.SetPaused(true);
  output.fill(0.25f);
  queue.MixInto(std::span<float>(output.data(), 2));
  assert(output[0] == 0.25f && output[1] == 0.25f);
  assert(queue.QueuedFrames() == 1);
  queue.SetPaused(false);
  output.fill(0.0f);
  queue.MixInto(std::span<float>(output.data(), 2));
  assert(output[0] == -1.0f && output[1] == 0.0f);

  assert(queue.Configure({44'100, 1}));
  assert(queue.Format().channels == 1);
  assert(!queue.Configure({44'100, 0}));

  // Each impulse must reach precisely its established stereo/surround lanes.
  for (size_t source = 0; source < 6; ++source) {
    std::array<float, 6 * 4> planar{};
    planar[source * 4] = std::bit_cast<float>(std::byteswap(std::bit_cast<uint32_t>(0.5f)));
    const StereoFold fold{0.6f, 0.4f, 0.2f, 0.5f};
    const SurroundMix mix{0.6f, 0.4f, 0.2f};
    std::array<float, 8> stereo{};
    std::array<float, 24> surround{};
    conversion::sequential_6_BE_to_interleaved_2_LE(stereo.data(), planar.data(), 4, fold, 0.8f);
    conversion::sequential_6_BE_to_interleaved_6_LE(surround.data(), planar.data(), 4, mix, 0.8f);
    const float stereo_left[] = {0.2f, 0, 0.12f, 0.04f, 0.08f, 0};
    const float stereo_right[] = {0, 0.2f, 0.12f, 0.04f, 0, 0.08f};
    const float surround_gain[] = {0.4f, 0.4f, 0.24f, 0.08f, 0.16f, 0.16f};
    assert(std::abs(stereo[0] - stereo_left[source]) < 1.0e-6f);
    assert(std::abs(stereo[1] - stereo_right[source]) < 1.0e-6f);
    for (size_t index = 0; index < surround.size(); ++index) {
      assert(std::abs(surround[index] - (index == source ? surround_gain[source] : 0.0f)) < 1.0e-6f);
    }
    for (size_t index = 2; index < stereo.size(); ++index) assert(stereo[index] == 0.0f);
  }
}
