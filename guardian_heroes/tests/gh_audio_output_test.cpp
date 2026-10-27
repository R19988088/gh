#include "gh_audio_sdl.h"
#include "gh_audio_xma.h"

#include <array>
#include <algorithm>
#include <atomic>
#include <bit>
#include <cassert>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <mutex>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio_state.h>
extern "C" {
#include <libavcodec/avcodec.h>
}

using namespace std::chrono_literals;

namespace {

void TestSoftwareStreamState() {
  const SDL_AudioSpec source{SDL_AUDIO_F32, 2, 48000}, destination{SDL_AUDIO_F32, 2, 44100};
  std::vector<float> samples(30000);
  for (size_t i = 0; i < samples.size(); ++i)
    samples[i] = std::sin(float(i * 17 % 1051) * 0.017f) * 0.7f;
  const int map[]{1, 0};
  for (const int prefix_frames : {307, 4507, 9511}) {
    auto* reference = SDL_CreateAudioStream(&source, &destination);
    auto* restored = SDL_CreateAudioStream(&source, &destination);
    assert(reference && restored);
    assert(SDL_SetAudioStreamInputChannelMap(reference, map, 2));
    assert(SDL_SetAudioStreamFrequencyRatio(reference, 1.03125f));
    assert(SDL_SetAudioStreamGain(reference, 0.65f));
    assert(SDL_PutAudioStreamData(reference, samples.data(), 12000 * sizeof(float)));
    assert(SDL_FlushAudioStream(reference));
    assert(SDL_PutAudioStreamData(reference, samples.data() + 12000, 10000 * sizeof(float)));
    std::vector<float> prefix(prefix_frames * 2);
    assert(SDL_GetAudioStreamData(reference, prefix.data(), int(prefix.size() * sizeof(float))) ==
           int(prefix.size() * sizeof(float)));
    void* state = nullptr; size_t size = 0;
    assert(GH_SDL_SaveAudioStreamState(reference, &state, &size));
    assert(GH_SDL_RestoreAudioStreamState(restored, state, size));
    assert(!GH_SDL_RestoreAudioStreamState(restored, state, 1));
    SDL_free(state);
    // Continue both streams across existing chunk boundaries and a final flush.
    for (auto* stream : {reference, restored}) {
      assert(SDL_PutAudioStreamData(stream, samples.data() + 22000, 8000 * sizeof(float)));
      assert(SDL_FlushAudioStream(stream));
    }
    const int available = SDL_GetAudioStreamAvailable(reference);
    assert(available > 0 && SDL_GetAudioStreamAvailable(restored) == available);
    std::vector<uint8_t> expected(available), actual(available);
    assert(SDL_GetAudioStreamData(reference, expected.data(), available) == available);
    assert(SDL_GetAudioStreamData(restored, actual.data(), available) == available);
    assert(actual == expected);
    assert(std::any_of(actual.begin(), actual.end(), [](uint8_t b) { return b != 0; }));
    SDL_DestroyAudioStream(reference); SDL_DestroyAudioStream(restored);
  }
}

void TestPausedOutputState() {
  gh::audio::SdlOutput output({}, {}, {});
  std::array<float, gh::audio::SdlOutput::kFrameSamples> first{}, second{};
  first[17] = 0.25f; second[91] = -0.75f;
  assert(output.Submit(first) && output.Submit(second));
  std::vector<uint8_t> saved, restored;
  assert(!output.SaveState(saved));
  assert(output.Pause() && output.SaveState(saved));
  assert(output.Submit(first) && output.QueuedFrames() == 3);
  assert(output.RestoreState(saved) && output.QueuedFrames() == 2);
  assert(output.SaveState(restored) && restored == saved);
  assert(!output.RestoreState({saved.data(), 1}));
  assert(output.SaveState(restored) && restored == saved);
  assert(output.Resume() && !output.RestoreState(saved));
}

void TestDeviceQueue(bool system_output) {
  std::mutex mutex;
  std::condition_variable consumed;
  unsigned completed = 0;
  std::atomic<bool> muted = false;
  gh::audio::SdlOutput output([&] { return muted.load(); }, [&] {
    std::lock_guard lock(mutex);
    ++completed;
    consumed.notify_one();
  }, [](std::string_view error) {
    std::fprintf(stderr, "%.*s\n", static_cast<int>(error.size()), error.data());
    assert(false);
  });
  std::array<float, gh::audio::SdlOutput::kFrameSamples> frame{};
  assert(output.Submit(frame) && output.Submit(frame));
  assert(output.QueuedFrames() == 2);
  output.Close();
  assert(output.QueuedFrames() == 0 && completed == 0);
  if (!system_output) {
    bool has_dummy = false;
    for (int index = 0; index < SDL_GetNumAudioDrivers(); ++index) {
      has_dummy |= std::strcmp(SDL_GetAudioDriver(index), "dummy") == 0;
    }
    if (!has_dummy) {
      std::puts("SDL dummy backend unavailable; device callback check skipped");
      return;
    }
    assert(SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy"));
  }
  for (unsigned cycle = 1; cycle <= 2; ++cycle) {
    assert(output.Open(""));
    assert(output.GetEndpoint().submitted_channels == 2 ||
           output.GetEndpoint().submitted_channels == 6);
    assert(output.Submit(frame));
    std::unique_lock lock(mutex);
    assert(consumed.wait_for(lock, 2s, [&] { return completed == cycle; }));
    lock.unlock();
    output.Close();
    assert(output.QueuedFrames() == 0);
    muted = true;
  }
  output.Close();
}

uint32_t ReadBits(const std::array<uint8_t, 2048>& packet, size_t offset, size_t count) {
  uint32_t result = 0;
  for (size_t index = 0; index < count; ++index) {
    result = (result << 1) | ((packet[(offset + index) / 8] >> (7 - (offset + index) % 8)) & 1);
  }
  return result;
}

void TestXmaDecode(const char* bank_path) {
  std::ifstream bank(bank_path, std::ios::binary);
  assert(bank);
  std::array<uint8_t, 2048> packet{};
  // The checked-in BGM bank's first stream begins at its wave-data segment.
  bank.seekg(8192);
  bank.read(reinterpret_cast<char*>(packet.data()), packet.size());
  assert(bank.gcount() == packet.size());
  assert((packet[2] & 7) == 1);

  auto* codec = avcodec_find_decoder(AV_CODEC_ID_XMAFRAMES);
  assert(codec);
  auto* context = avcodec_alloc_context3(codec);
  assert(context);
  context->sample_rate = 44100;
  context->channels = 2;
  context->flags2 |= AV_CODEC_FLAG2_SKIP_MANUAL;
  assert(avcodec_open2(context, codec, nullptr) == 0);
  auto* reference_packet = av_packet_alloc();
  auto* reference_frame = av_frame_alloc();
  assert(reference_packet && reference_frame);

  gh::audio::XmaFrameDecoder decoder;
  assert(decoder.Initialize());
  assert(decoder.Prepare(44100, true) == 1);
  assert(decoder.Prepare(44100, true) == 0);
  std::array<uint8_t, 2048> actual{};
  std::array<uint8_t, 2048> first_decoded{};
  std::vector<uint8_t> first_packet;
  size_t offset = 32 + ReadBits(packet, 6, 15);
  for (unsigned frame_index = 0; frame_index < 4; ++frame_index) {
    const size_t bits = ReadBits(packet, offset, 15);
    const size_t leading = offset % 8;
    assert(bits && offset + bits < packet.size() * 8);
    const size_t bytes = (leading + bits + 7) / 8;
    const size_t trailing = bytes * 8 - leading - bits;
    std::vector<uint8_t> frame(1 + bytes);
    frame[0] = (leading << 5) | (trailing << 2);
    std::memcpy(frame.data() + 1, packet.data() + offset / 8, bytes);
    frame[1] &= 0xFFu >> leading;
    frame.back() &= 0xFFu << trailing;
    assert(decoder.Decode(frame, actual));

    av_packet_unref(reference_packet);
    assert(av_new_packet(reference_packet, frame.size()) == 0);
    std::memcpy(reference_packet->data, frame.data(), frame.size());
    assert(avcodec_send_packet(context, reference_packet) == 0);
    assert(avcodec_receive_frame(context, reference_frame) == 0);
    assert(reference_frame->nb_samples == 512 && reference_frame->format == AV_SAMPLE_FMT_FLTP);
    for (size_t sample = 0; sample < 512; ++sample) {
      for (size_t channel = 0; channel < 2; ++channel) {
        const float value = reinterpret_cast<float*>(reference_frame->data[channel])[sample];
#if defined(__x86_64__) || defined(_M_X64)
        // The established SSSE3 path rounds before saturated packing.
        const auto converted = std::clamp(std::lrint(value * 32767.0f), -32768L, 32767L);
        const uint16_t expected = static_cast<uint16_t>(static_cast<int16_t>(converted));
#else
        const float lower = std::isgreater(value, -1.0f) ? value : -1.0f;
        const float clamped = std::isless(lower, 1.0f) ? lower : 1.0f;
        const uint16_t expected = static_cast<uint16_t>(static_cast<int16_t>(clamped * 32767.0f));
#endif
        const size_t index = (sample * 2 + channel) * 2;
        assert(actual[index] == (expected >> 8) && actual[index + 1] == (expected & 255));
      }
    }
    if (frame_index == 0) { first_decoded = actual; first_packet = frame; }
    offset += bits;
  }
  decoder.Reset();
  assert(decoder.Prepare(44100, true) == 1);
  assert(decoder.Decode(first_packet, actual) && actual == first_decoded);
  assert(!decoder.Decode({}, actual));
  av_frame_free(&reference_frame);
  av_packet_free(&reference_packet);
  avcodec_free_context(&context);
}

}  // namespace

int main(int argc, char** argv) {
  assert(argc == 2 || (argc == 3 && std::strcmp(argv[2], "--system-output") == 0));
  TestSoftwareStreamState();
  TestPausedOutputState();
  std::puts("SDL software stream: exact resampling/history and paused frame queue restore passed");
  TestDeviceQueue(argc == 3);
  TestXmaDecode(argv[1]);
}
