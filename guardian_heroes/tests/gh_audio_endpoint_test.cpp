#include "gh_audio_sdl.h"
#include <SDL3/SDL.h>
#include <array>
#include <atomic>
#include <bit>
#include <cassert>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>
#ifdef __APPLE__
#include <CoreAudio/CoreAudio.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

using namespace std::chrono_literals;

#ifdef __APPLE__
template <typename T>
bool ReadProperty(AudioObjectID object, AudioObjectPropertySelector selector,
                  AudioObjectPropertyScope scope, AudioObjectPropertyElement element, T& value) {
  const AudioObjectPropertyAddress address{selector, scope, element};
  UInt32 size = sizeof(value);
  return AudioObjectHasProperty(object, &address) &&
      AudioObjectGetPropertyData(object, &address, 0, nullptr, &size, &value) == noErr;
}

void PrintHardwareState(std::string_view selected_name) {
  AudioDeviceID selected = kAudioObjectUnknown;
  const AudioObjectPropertyAddress list{kAudioHardwarePropertyDevices,
      kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
  UInt32 bytes = 0;
  assert(AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &list, 0, nullptr, &bytes) == noErr);
  std::vector<AudioDeviceID> devices(bytes / sizeof(AudioDeviceID));
  assert(AudioObjectGetPropertyData(kAudioObjectSystemObject, &list, 0, nullptr, &bytes, devices.data()) == noErr);
  for (const auto device : devices) {
    CFStringRef name = nullptr;
    if (!ReadProperty(device, kAudioObjectPropertyName, kAudioObjectPropertyScopeGlobal,
                      kAudioObjectPropertyElementMain, name)) continue;
    std::array<char, 1024> utf8{};
    const bool matched = CFStringGetCString(name, utf8.data(), utf8.size(), kCFStringEncodingUTF8) &&
        selected_name == utf8.data();
    CFRelease(name);
    if (matched) { selected = device; break; }
  }
  assert(selected != kAudioObjectUnknown);
  for (const UInt32 element : {0u, 1u, 2u}) {
    UInt32 mute = 0;
    Float32 volume = 0;
    if (ReadProperty(selected, kAudioDevicePropertyMute, kAudioDevicePropertyScopeOutput, element, mute))
      std::printf("CoreAudio device %u element %u mute=%u (read only)\n", selected, element, mute);
    if (ReadProperty(selected, kAudioDevicePropertyVolumeScalar, kAudioDevicePropertyScopeOutput, element, volume))
      std::printf("CoreAudio device %u element %u volume=%.4f (read only)\n", selected, element, volume);
  }
}
#endif

int main(int argc, char** argv) {
  assert(argc <= 2);
  const std::string selected_name = argc == 2 ? argv[1] : "";
  assert(SDL_InitSubSystem(SDL_INIT_AUDIO));
  SDL_AudioDeviceID selected = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
  if (!selected_name.empty()) {
    int count = 0;
    auto* devices = SDL_GetAudioPlaybackDevices(&count);
    assert(devices);
    for (int index = 0; index < count; ++index) {
      const char* name = SDL_GetAudioDeviceName(devices[index]);
      if (name && selected_name == name) { selected = devices[index]; break; }
    }
    SDL_free(devices);
    assert(selected != SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK);
  }
  SDL_AudioSpec preferred{};
  assert(SDL_GetAudioDeviceFormat(selected, &preferred, nullptr));
  const std::string physical_name = SDL_GetAudioDeviceName(selected);
  std::printf("SDL driver=%s; physical '%s' before open: %d ch, %d Hz\n",
      SDL_GetCurrentAudioDriver(), physical_name.c_str(), preferred.channels, preferred.freq);
#ifdef __APPLE__
  PrintHardwareState(physical_name);
#endif
  // Match the app: each SdlOutput owns its audio subsystem reference.
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
  std::mutex mutex;
  std::condition_variable ready;
  unsigned consumed = 0;
  gh::audio::SdlOutput output([] { return false; }, [&] {
    std::lock_guard lock(mutex);
    ++consumed;
    ready.notify_one();
  }, [](std::string_view error) {
    std::fprintf(stderr, "%.*s\n", static_cast<int>(error.size()), error.data());
    assert(false);
  });
  assert(output.Open(selected_name, true));
  const auto& endpoint = output.GetEndpoint();
  const unsigned expected_channels = preferred.channels <= 2 ? 2 : 6;
  assert(endpoint.name == physical_name && endpoint.submitted_channels == expected_channels);
  if (preferred.channels == 2) assert(endpoint.channels == 2);
  std::printf("opened '%s': %d ch; submitting %u ch\n", endpoint.name.c_str(), endpoint.channels,
      static_cast<unsigned>(endpoint.submitted_channels));
  constexpr unsigned frames = 120;
  for (unsigned frame_index = 0; frame_index < frames; ++frame_index) {
    std::array<float, gh::audio::SdlOutput::kFrameSamples> frame{};
    for (unsigned sample = 0; sample < gh::audio::SdlOutput::kChannelSamples; ++sample) {
      const auto position = frame_index * gh::audio::SdlOutput::kChannelSamples + sample;
      const float tone = 0.08f * std::sin(2.0f * 3.14159265358979323846f * 440.0f * position / 48000.0f);
      const float encoded = std::bit_cast<float>(std::byteswap(std::bit_cast<uint32_t>(tone)));
      frame[sample] = encoded;
      frame[gh::audio::SdlOutput::kChannelSamples + sample] = encoded;
    }
    assert(output.Submit(frame));
  }
  assert(consumed == 0 && output.QueuedFrames() == frames);
  assert(output.Resume());
  {
    std::unique_lock lock(mutex);
    assert(ready.wait_for(lock, 3s, [&] { return consumed == frames; }));
  }
  assert(output.QueuedFrames() == 0);
  assert(output.Pause());
  const auto statistics = output.TakeStatistics();
  assert(statistics.submitted_frames == frames && statistics.consumed_frames == frames);
  assert(statistics.callbacks > 0 && statistics.max_callback_gap_ms > 0);
  const auto reset_statistics = output.TakeStatistics();
  assert(reset_statistics.submitted_frames == 0 && reset_statistics.consumed_frames == 0 &&
         reset_statistics.silence_frames == 0 && reset_statistics.callbacks == 0);
  std::printf("endpoint statistics: submitted=%llu consumed=%llu silence=%llu callbacks=%llu "
              "max-submit-gap=%.3fms max-callback-gap=%.3fms\n",
              static_cast<unsigned long long>(statistics.submitted_frames),
              static_cast<unsigned long long>(statistics.consumed_frames),
              static_cast<unsigned long long>(statistics.silence_frames),
              static_cast<unsigned long long>(statistics.callbacks),
              statistics.max_submit_gap_ms, statistics.max_callback_gap_ms);
  std::printf("CoreAudio endpoint callback consumed %u/%u nonzero FL/FR frames; "
              "physical audibility requires listener confirmation\n", consumed, frames);
  std::array<std::unique_ptr<gh::audio::SdlOutput>, 2> companions;
  std::array<std::vector<uint8_t>, 2> companion_states;
  for (auto& companion : companions) {
    companion = std::make_unique<gh::audio::SdlOutput>(nullptr, nullptr, nullptr);
    assert(companion->Open(selected_name));
  }
  assert(output.Resume());
  std::array<float, gh::audio::SdlOutput::kFrameSamples> continuation_frame{};
  continuation_frame.fill(std::bit_cast<float>(std::byteswap(std::bit_cast<uint32_t>(0.02f))));
  for (unsigned index = 0; index < frames; ++index) assert(output.Submit(continuation_frame));
  {
    std::unique_lock lock(mutex);
    assert(ready.wait_for(lock, 3s, [&] { return consumed >= frames + 8; }));
  }
  assert(output.Pause());
  const unsigned saved_consumed = consumed;
  const auto saved_queued = output.QueuedFrames();
  assert(saved_queued > 0 && saved_consumed + saved_queued == frames * 2);
  std::vector<uint8_t> saved;
  assert(output.SaveState(saved));
  for (size_t index = 0; index < companions.size(); ++index) {
    assert(companions[index]->Pause() && companions[index]->SaveState(companion_states[index]));
    companions[index]->Close();
  }
  output.Close();
  for (size_t index = 0; index < companions.size(); ++index) {
    assert(companions[index]->Open(selected_name, true));
    assert(companions[index]->RestoreState(companion_states[index]));
  }
  assert(output.Open(selected_name, true));
  assert(output.RestoreState(saved) && output.QueuedFrames() == saved_queued);
  for (auto& companion : companions) assert(companion->Resume());
  assert(output.Resume());
  {
    std::unique_lock lock(mutex);
    assert(ready.wait_for(lock, 3s, [&] { return consumed == frames * 2; }));
  }
  assert(output.Pause());
  const auto restored_statistics = output.TakeStatistics();
  assert(restored_statistics.consumed_frames == saved_queued && restored_statistics.callbacks > 0);
  std::printf("CoreAudio rebuilt endpoint resumed %zu/%zu saved queued frames, callbacks=%llu\n",
              static_cast<size_t>(restored_statistics.consumed_frames), saved_queued,
              static_cast<unsigned long long>(restored_statistics.callbacks));
  output.Close();
  for (auto& companion : companions) companion->Close();
}
