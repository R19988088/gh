// SDL output implementation adapted from Xenia/ReXGlue's BSD-licensed driver.
#include "gh_audio_sdl.h"

#include "gh_audio_conversion.h"
#include "gh_audio_state_io.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <mutex>
#include <queue>
#include <stack>
#include <utility>

#include <SDL3/SDL.h>
#if __has_include(<SDL3/SDL_audio_state.h>)
#include <SDL3/SDL_audio_state.h>
#define GH_SDL_AUDIO_STATE_AVAILABLE 1
#endif

namespace gh::audio {

class SdlOutput::Impl {
 public:
  Impl(std::function<bool()> muted, std::function<void()> consumed,
       std::function<void(std::string_view)> report_error)
      : muted(std::move(muted)), consumed(std::move(consumed)),
        report_error(std::move(report_error)) {}

  bool Fail(const char* operation) {
    if (report_error) report_error(std::string(operation) + ": " + SDL_GetError());
    return false;
  }

  static void Callback(void* userdata, SDL_AudioStream* stream, int additional_amount, int) {
    auto& self = *static_cast<Impl*>(userdata);
    {
      std::lock_guard lock(self.mutex);
      const auto now = std::chrono::steady_clock::now();
      if (self.last_callback != std::chrono::steady_clock::time_point{})
        self.statistics.max_callback_gap_ms = std::max(self.statistics.max_callback_gap_ms,
            std::chrono::duration<double, std::milli>(now - self.last_callback).count());
      self.last_callback = now;
      ++self.statistics.callbacks;
    }
    std::array<float, kFrameSamples> data;
    const int len = sizeof(float) * kChannelSamples * self.endpoint.submitted_channels;
    // Match the existing output policy: one mix snapshot for the whole callback.
    const auto fold = GetStereoFold();
    const auto mix = GetSurroundMix();
    const auto gain = GetOutputGain();
    while (additional_amount > 0) {
      std::unique_lock lock(self.mutex);
      if (self.queued.empty()) {
        ++self.statistics.silence_frames;
        std::memset(data.data(), 0, len);
        if (!SDL_PutAudioStreamData(stream, data.data(), len)) {
          self.Fail("SDL_PutAudioStreamData silence");
          break;
        }
      } else {
        auto* buffer = self.queued.front();
        self.queued.pop();
        if (self.muted && self.muted()) {
          std::memset(data.data(), 0, len);
        } else if (self.endpoint.submitted_channels == 2) {
          conversion::sequential_6_BE_to_interleaved_2_LE(
              data.data(), buffer, kChannelSamples, fold, gain);
        } else {
          conversion::sequential_6_BE_to_interleaved_6_LE(
              data.data(), buffer, kChannelSamples, mix, gain);
        }
        const bool written = SDL_PutAudioStreamData(stream, data.data(), len);
        self.unused.push(buffer);
        if (!written) {
          self.Fail("SDL_PutAudioStreamData");
          break;
        }
        ++self.statistics.consumed_frames;
        if (self.consumed) self.consumed();
      }
      additional_amount -= len;
    }
  }

  std::function<bool()> muted;
  std::function<void()> consumed;
  std::function<void(std::string_view)> report_error;
  SDL_AudioStream* stream = nullptr;
  bool initialized = false;
  bool paused = false;
  Endpoint endpoint;
  mutable std::mutex mutex;
  std::queue<float*> queued;
  std::stack<float*> unused;
  Statistics statistics;
  std::chrono::steady_clock::time_point last_submit{}, last_callback{};
};

SdlOutput::SdlOutput(std::function<bool()> muted, std::function<void()> frame_consumed,
                     std::function<void(std::string_view)> report_error)
    : impl_(std::make_unique<Impl>(std::move(muted), std::move(frame_consumed),
                                  std::move(report_error))) {}

SdlOutput::~SdlOutput() { Close(); }

bool SdlOutput::Open(std::string_view device_name, bool start_paused) {
  auto& self = *impl_;
  Close();
  SDL_SetHint(SDL_HINT_AUDIO_CATEGORY, "playback");
  SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "rexglue");
  if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) return self.Fail("SDL_InitSubSystem audio");
  self.initialized = true;
  SDL_AudioSpec desired{};
  desired.freq = kSampleRate;
  desired.format = SDL_AUDIO_F32LE;
  SDL_AudioDeviceID playback = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
  if (!device_name.empty()) {
    int count = 0;
    auto* devices = SDL_GetAudioPlaybackDevices(&count);
    if (!devices) return self.Fail("SDL_GetAudioPlaybackDevices");
    for (int index = 0; index < count; ++index) {
      const auto* name = SDL_GetAudioDeviceName(devices[index]);
      if (name && device_name == name) { playback = devices[index]; break; }
    }
    SDL_free(devices);
    if (playback == SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK) {
      if (self.report_error) self.report_error("Requested audio playback device was not found");
      return false;
    }
  }
  // CoreAudio accepts the requested channel count when opening its AudioQueue.
  // Query first: after opening six channels, GetAudioDeviceFormat reports that
  // six-channel queue even when the physical speakers are stereo.
  SDL_AudioSpec preferred{};
  if (!SDL_GetAudioDeviceFormat(playback, &preferred, nullptr))
    return self.Fail("SDL_GetAudioDeviceFormat preferred");
  desired.channels = preferred.channels <= 2 ? 2 : kChannels;
  self.endpoint.submitted_channels = static_cast<uint8_t>(desired.channels);
  self.stream = SDL_OpenAudioDeviceStream(playback, &desired, Impl::Callback, &self);
  if (!self.stream) return self.Fail("SDL_OpenAudioDeviceStream");
  auto device = SDL_GetAudioStreamDevice(self.stream);
  if (!device) return self.Fail("SDL_GetAudioStreamDevice");
  SDL_AudioSpec obtained{};
  if (!SDL_GetAudioDeviceFormat(device, &obtained, nullptr)) obtained = desired;
  const auto* name = SDL_GetAudioDeviceName(device);
  self.endpoint.name = name ? name : "?";
  self.endpoint.channels = obtained.channels;
  self.endpoint.frequency = obtained.freq;
  self.endpoint.format = obtained.format;
  self.paused = start_paused;
  if (!start_paused && !SDL_ResumeAudioDevice(device)) return self.Fail("SDL_ResumeAudioDevice");
  return true;
}

void SdlOutput::Close() {
  auto& self = *impl_;
  if (self.stream) {
    SDL_DestroyAudioStream(self.stream);
    self.stream = nullptr;
  }
  if (self.initialized) {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    self.initialized = false;
  }
  std::lock_guard lock(self.mutex);
  self.paused = false;
  self.statistics = {};
  self.last_submit = self.last_callback = {};
  while (!self.unused.empty()) { delete[] self.unused.top(); self.unused.pop(); }
  while (!self.queued.empty()) { delete[] self.queued.front(); self.queued.pop(); }
}

bool SdlOutput::Submit(std::span<const float> samples) {
  if (samples.size() != kFrameSamples) return false;
  auto& self = *impl_;
  float* frame;
  {
    std::lock_guard lock(self.mutex);
    if (self.unused.empty()) frame = new float[kFrameSamples];
    else { frame = self.unused.top(); self.unused.pop(); }
  }
  std::memcpy(frame, samples.data(), samples.size_bytes());
  std::lock_guard lock(self.mutex);
  self.queued.push(frame);
  const auto now = std::chrono::steady_clock::now();
  if (self.last_submit != std::chrono::steady_clock::time_point{})
    self.statistics.max_submit_gap_ms = std::max(self.statistics.max_submit_gap_ms,
        std::chrono::duration<double, std::milli>(now - self.last_submit).count());
  self.last_submit = now;
  ++self.statistics.submitted_frames;
  return true;
}

SdlOutput::Statistics SdlOutput::TakeStatistics() {
  std::lock_guard lock(impl_->mutex);
  return std::exchange(impl_->statistics, Statistics{});
}

size_t SdlOutput::QueuedFrames() const {
  std::lock_guard lock(impl_->mutex);
  return impl_->queued.size();
}

const SdlOutput::Endpoint& SdlOutput::GetEndpoint() const { return impl_->endpoint; }

bool SdlOutput::Pause() {
  auto& self = *impl_;
  if (self.stream && !SDL_PauseAudioStreamDevice(self.stream)) return self.Fail("SDL pause");
  // Device pause prevents a new mix; stream lock waits out a current callback.
  if (self.stream) SDL_LockAudioStream(self.stream);
  {
    std::lock_guard lock(self.mutex);
    self.paused = true;
    self.last_submit = self.last_callback = {};
  }
  if (self.stream) SDL_UnlockAudioStream(self.stream);
  return true;
}
bool SdlOutput::Resume() {
  auto& self = *impl_;
  if (self.stream && !SDL_ResumeAudioStreamDevice(self.stream)) return self.Fail("SDL resume");
  std::lock_guard lock(self.mutex); self.paused = false;
  return true;
}

bool SdlOutput::SaveState(std::vector<uint8_t>& bytes) const {
  auto& self = *impl_;
  if (self.stream) SDL_LockAudioStream(self.stream);
  struct Unlock { SDL_AudioStream* stream; ~Unlock() { if (stream) SDL_UnlockAudioStream(stream); } } unlock{self.stream};
  std::lock_guard lock(self.mutex);
  if (!self.paused) return false;
  std::vector<uint8_t> stream_state;
  if (self.stream) {
#if defined(GH_SDL_AUDIO_STATE_AVAILABLE)
    void* state = nullptr; size_t size = 0;
    if (!GH_SDL_SaveAudioStreamState(self.stream, &state, &size)) return false;
    stream_state.assign(static_cast<uint8_t*>(state), static_cast<uint8_t*>(state) + size);
    SDL_free(state);
#else
    return false;
#endif
  }
  state_io::Writer writer;
  writer.Put(uint32_t{1}); writer.Put(bool(self.stream));
  writer.Put(self.endpoint.channels); writer.Put(self.endpoint.frequency);
  writer.Put(self.endpoint.format); writer.Put(self.endpoint.submitted_channels);
  writer.Blob({reinterpret_cast<const uint8_t*>(self.endpoint.name.data()), self.endpoint.name.size()});
  writer.Blob(stream_state);
  auto queued = self.queued;
  writer.Put(static_cast<uint32_t>(queued.size()));
  while (!queued.empty()) {
    writer.Blob({reinterpret_cast<const uint8_t*>(queued.front()), kFrameSamples * sizeof(float)});
    queued.pop();
  }
  auto unused = self.unused;
  writer.Put(static_cast<uint32_t>(unused.size()));
  while (!unused.empty()) {
    writer.Blob({reinterpret_cast<const uint8_t*>(unused.top()), kFrameSamples * sizeof(float)});
    unused.pop();
  }
  bytes = std::move(writer.bytes);
  return true;
}

bool SdlOutput::RestoreState(std::span<const uint8_t> bytes) {
  auto& self = *impl_;
  if (self.stream) SDL_LockAudioStream(self.stream);
  struct Unlock { SDL_AudioStream* stream; ~Unlock() { if (stream) SDL_UnlockAudioStream(stream); } } unlock{self.stream};
  std::lock_guard lock(self.mutex);
  if (!self.paused) return false;
  try {
    state_io::Reader reader{bytes};
    if (reader.Get<uint32_t>() != 1 || reader.Get<bool>() != bool(self.stream)) return false;
    const auto channels = reader.Get<int>(), frequency = reader.Get<int>();
    const auto format = reader.Get<uint32_t>();
    const auto submitted = reader.Get<uint8_t>();
    const auto name = reader.Blob();
    if (self.stream && (channels != self.endpoint.channels || frequency != self.endpoint.frequency ||
        format != self.endpoint.format || submitted != self.endpoint.submitted_channels ||
        std::string(name.begin(), name.end()) != self.endpoint.name)) return false;
    const auto stream_state = reader.Blob();
    std::vector<std::unique_ptr<float[]>> queued, unused;
    auto read_frames = [&](auto& frames) {
      const auto count = reader.Get<uint32_t>();
      if (count > reader.bytes.size() / (sizeof(float) * kFrameSamples)) return false;
      for (uint32_t i = 0; i < count; ++i) {
        const auto frame = reader.Blob();
        if (frame.size() != sizeof(float) * kFrameSamples) return false;
        auto storage = std::make_unique<float[]>(kFrameSamples);
        std::memcpy(storage.get(), frame.data(), frame.size()); frames.push_back(std::move(storage));
      }
      return true;
    };
    if (!read_frames(queued) || !read_frames(unused) || !reader.bytes.empty()) return false;
    if (self.stream) {
#if defined(GH_SDL_AUDIO_STATE_AVAILABLE)
      if (!GH_SDL_RestoreAudioStreamState(self.stream, stream_state.data(), stream_state.size())) return false;
#else
      return false;
#endif
    } else if (!stream_state.empty()) return false;
    while (!self.queued.empty()) { delete[] self.queued.front(); self.queued.pop(); }
    while (!self.unused.empty()) { delete[] self.unused.top(); self.unused.pop(); }
    for (auto& frame : queued) self.queued.push(frame.release());
    for (auto it = unused.rbegin(); it != unused.rend(); ++it) self.unused.push(it->release());
    return true;
  } catch (const std::exception&) { return false; }
}

}  // namespace gh::audio
