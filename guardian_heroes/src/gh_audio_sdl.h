#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace gh::audio {

// Guest output is six planar big-endian float channels, 256 samples at 48 kHz.
class SdlOutput {
 public:
  static constexpr uint32_t kSampleRate = 48'000;
  static constexpr uint32_t kChannels = 6;
  static constexpr uint32_t kChannelSamples = 256;
  static constexpr uint32_t kFrameSamples = kChannels * kChannelSamples;

  struct Endpoint {
    std::string name;
    int channels = 0;
    int frequency = 0;
    uint32_t format = 0;
    uint8_t submitted_channels = 0;
  };
  struct Statistics {
    uint64_t submitted_frames = 0;
    uint64_t consumed_frames = 0;
    uint64_t silence_frames = 0;
    uint64_t callbacks = 0;
    double max_submit_gap_ms = 0;
    double max_callback_gap_ms = 0;
  };

  SdlOutput(std::function<bool()> muted, std::function<void()> frame_consumed,
            std::function<void(std::string_view)> report_error);
  ~SdlOutput();
  SdlOutput(const SdlOutput&) = delete;
  SdlOutput& operator=(const SdlOutput&) = delete;

  bool Open(std::string_view device_name, bool start_paused = false);
  void Close();
  bool Submit(std::span<const float> planar_big_endian);
  size_t QueuedFrames() const;
  Statistics TakeStatistics();
  const Endpoint& GetEndpoint() const;
  bool Pause();
  bool Resume();
  bool SaveState(std::vector<uint8_t>& state) const;
  bool RestoreState(std::span<const uint8_t> state);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace gh::audio
