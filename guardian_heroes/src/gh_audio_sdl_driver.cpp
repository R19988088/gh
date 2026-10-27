/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <rex/assert.h>
#include "gh_audio_sdl_driver.h"
#include <rex/audio/flags.h>
#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/perf/counter.h>
#include "gh_audio_sdl.h"

REXCVAR_DEFINE_BOOL(audio_mute, false, "Audio", "Mute audio output");
REXCVAR_DEFINE_STRING(audio_device, "", "Audio",
                     "Playback device name (empty uses the system default)");

namespace gh::audio::sdl {

SDLAudioDriver::SDLAudioDriver(rex::memory::Memory* memory, std::function<void()> consumed)
    : AudioDriver(memory), output_(std::make_unique<gh::audio::SdlOutput>(
          [] { return REXCVAR_GET(audio_mute); },
          std::move(consumed),
          [](std::string_view error) { REXAPU_ERROR("{}", error); })) {}

SDLAudioDriver::~SDLAudioDriver() = default;

bool SDLAudioDriver::Initialize(bool start_paused) {
  if (!output_->Open(REXCVAR_GET(audio_device), start_paused)) return false;
  const auto& endpoint = output_->GetEndpoint();
  REXAPU_INFO("audio endpoint '{}': {} ch, {} Hz, format 0x{:04X}; submitting {} ch",
              endpoint.name, endpoint.channels, endpoint.frequency, endpoint.format,
              static_cast<int>(endpoint.submitted_channels));
  return true;
}

void SDLAudioDriver::SubmitFrame(uint32_t frame_ptr) {
  const auto input_frame = memory_->TranslateVirtual<float*>(frame_ptr);
  constexpr auto frame_channels_ = gh::audio::SdlOutput::kChannels;
  constexpr auto channel_samples_ = gh::audio::SdlOutput::kChannelSamples;
  if (diagnostic_submit_count_ < 3 || diagnostic_submit_count_ % 256 == 0) {
    std::array<float, frame_channels_> channel_peak{};
    std::array<uint32_t, frame_channels_> nonzero_samples{};
    for (uint32_t channel = 0; channel < frame_channels_; ++channel) {
      for (uint32_t sample = 0; sample < channel_samples_; ++sample) {
        const float value = rex::byte_swap(input_frame[channel * channel_samples_ + sample]);
        if (std::isfinite(value)) {
          channel_peak[channel] = std::max(channel_peak[channel], std::abs(value));
          nonzero_samples[channel] += std::abs(value) > 1.0e-6f;
        }
      }
    }
    REXAPU_INFO(
        "Audio frame probe #{}: peak FL={:.6f} FR={:.6f} C={:.6f} LFE={:.6f} BL={:.6f} BR={:.6f}; "
        "nonzero={}/{}/{}/{}/{}/{}",
        diagnostic_submit_count_, channel_peak[0], channel_peak[1], channel_peak[2],
        channel_peak[3], channel_peak[4], channel_peak[5], nonzero_samples[0],
        nonzero_samples[1], nonzero_samples[2], nonzero_samples[3], nonzero_samples[4],
        nonzero_samples[5]);
  }
  diagnostic_submit_count_++;

  const bool submitted = output_->Submit({input_frame, gh::audio::SdlOutput::kFrameSamples});
  assert_true(submitted);
  if (diagnostic_submit_count_ % 256 == 0) {
    const auto statistics = output_->TakeStatistics();
    REXAPU_INFO("Audio endpoint {:X}: submitted={} consumed={} silence={} chunks/256 samples; "
               "callbacks={} queued={} max-submit-gap={:.3f}ms max-callback-gap={:.3f}ms",
               reinterpret_cast<uintptr_t>(this), statistics.submitted_frames,
               statistics.consumed_frames, statistics.silence_frames, statistics.callbacks,
               output_->QueuedFrames(), statistics.max_submit_gap_ms,
               statistics.max_callback_gap_ms);
  }
  PROFILE_BUFFER_QUEUE_DEPTH(static_cast<int64_t>(output_->QueuedFrames()));
}

void SDLAudioDriver::Shutdown() { output_->Close(); }
bool SDLAudioDriver::Pause() { return output_->Pause(); }
bool SDLAudioDriver::Resume() { return output_->Resume(); }
bool SDLAudioDriver::SaveState(std::vector<uint8_t>& state) { return output_->SaveState(state); }
bool SDLAudioDriver::RestoreState(std::span<const uint8_t> state) { return output_->RestoreState(state); }

}  // namespace gh::audio::sdl
