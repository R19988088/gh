#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string_view>

#include <rex/system/interfaces/audio.h>

namespace rex::runtime {
class FunctionDispatcher;
}

namespace gh::audio::switch_ {

// libnx audout output for the game's 48 kHz six-channel BE float callback.
// The endpoint folds it to stereo PCM16, the only format exposed by audout.
class AudoutOutput {
 public:
  static constexpr size_t kChannels = 6;
  static constexpr size_t kSamplesPerChannel = 256;
  static constexpr size_t kFrameSamples = kChannels * kSamplesPerChannel;

  AudoutOutput(std::function<void()> frame_consumed,
               std::function<void(std::string_view)> report_error);
  ~AudoutOutput();
  AudoutOutput(const AudoutOutput&) = delete;
  AudoutOutput& operator=(const AudoutOutput&) = delete;

  bool Open(bool start_paused = false);
  void Close();
  bool Submit(std::span<const float> planar_big_endian);
  bool Pause();
  bool Resume();

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

std::unique_ptr<rex::system::IAudioSystem> CreateSwitchAudioSystem(
    rex::runtime::FunctionDispatcher* dispatcher);

}  // namespace gh::audio::switch_
