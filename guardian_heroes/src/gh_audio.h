#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <span>
#include <vector>

namespace gh::audio {

/**
 * Weights applied when the guest's 5.1 render is folded to a stereo device.
 * Guest channel order is the XAudio default: fl fr fc lf bl br. The front
 * channels carry an implicit weight of 1.0, so `scale` alone sets the output
 * level.
 *
 * Defaults follow ITU-R BS.775 and Dolby Lo/Ro: center and surround at -3 dB,
 * LFE dropped. No published stereo downmix folds LFE, which is authored around
 * 10 dB hot by convention, so a title that wants it audible sets a weight that
 * undoes that offset rather than folding it at unity.
 */
struct StereoFold {
  float center = 0.70710678f;
  float surround = 0.70710678f;
  float lfe = 0.0f;
  float scale = 0.58578644f;  // 1/(1+0.707)
};

/**
 * Weights applied when the guest's 5.1 render reaches a device with more than
 * two channels, which the output stage passes through rather than folding.
 * Same channel order and the same implicit front weight of 1.0 as StereoFold,
 * so the two structs describe one mix in two destinations.
 *
 * Defaults are unity, a passthrough, because a title whose render really is
 * 5.1 wants its own mix reproduced. A title that packs something other than an
 * LFE into the LFE slot sets `lfe` to drop or attenuate it: that slot is
 * reproduced around 10 dB hot, so full-band content placed there arrives far
 * louder than it was authored, and a downstream downmix that folds it without
 * low-passing carries the whole band into the other channels.
 */
struct SurroundMix {
  float center = 1.0f;
  float surround = 1.0f;
  float lfe = 1.0f;
};


void SetStereoFold(const StereoFold& fold);
StereoFold GetStereoFold();
void SetSurroundMix(const SurroundMix& mix);
SurroundMix GetSurroundMix();
void SetOutputGain(float linear);
float GetOutputGain();

struct AudioFormat {
  uint32_t sample_rate = 0;
  uint8_t channels = 0;
};

enum class PacketCodec { kPcm16Le, kXma2 };
enum class DecodeResult { kOk, kUnsupportedCodec, kInvalidFormat, kInvalidPacket };

struct PcmFrames {
  AudioFormat format;
  std::vector<float> samples;

  size_t FrameCount() const noexcept {
    return format.channels == 0 ? 0 : samples.size() / format.channels;
  }
};

// PCM16LE packet payloads are raw interleaved signed samples, with no XMA
// packet header. Assembled XMA bitstreams use XmaFrameDecoder; this convenience
// function does not assemble the guest's packet/loop context.
DecodeResult DecodePacket(PacketCodec codec, std::span<const uint8_t> packet,
                         AudioFormat format, PcmFrames& output);
DecodeResult DecodePcm16LePacket(std::span<const uint8_t> packet,
                                 AudioFormat format,
                                 std::vector<float>& output);

class MixerQueue {
 public:
  struct Stats {
    uint64_t underrun_frames = 0;
    uint64_t overflow_frames = 0;
  };

  MixerQueue(AudioFormat format, size_t capacity_frames);

  bool Configure(AudioFormat format);
  AudioFormat Format() const;
  bool Enqueue(std::span<const float> interleaved_frames);
  void MixInto(std::span<float> interleaved_output);
  void SetPaused(bool paused);
  bool IsPaused() const;
  size_t QueuedFrames() const;
  Stats GetStats() const;

 private:
  static bool IsValidFormat(AudioFormat format) noexcept;

  mutable std::mutex mutex_;
  AudioFormat format_;
  std::vector<float> samples_;
  size_t capacity_frames_;
  size_t read_frame_ = 0;
  size_t queued_frames_ = 0;
  bool paused_ = false;
  Stats stats_;
};

}  // namespace gh::audio
