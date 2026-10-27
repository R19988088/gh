#include "gh_switch_audio.h"

#include "gh_audio.h"
#include "gh_audio_conversion.h"
#include "gh_audio_driver.h"
#include "gh_audio_system.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <deque>
#include <mutex>
#include <thread>
#include <utility>

#ifdef __cplusplus
extern "C" {
#endif
#include <switch/services/audout.h>
#ifdef __cplusplus
}
#endif

namespace gh::audio::switch_ {
namespace {

constexpr u32 kRate = 48'000;
constexpr u32 kChannels = 2;
constexpr size_t kFrameBytes = AudoutOutput::kSamplesPerChannel * kChannels * sizeof(int16_t);
constexpr size_t kAudioBufferBytes = 0x1000;
constexpr size_t kOutputSlots = 6;
constexpr size_t kMaximumQueuedFrames = 16;
constexpr u64 kWaitTimeoutNs = 10'000'000;

struct OutputSlot {
  AudioOutBuffer buffer{};
  alignas(0x1000) std::array<uint8_t, kAudioBufferBytes> data{};
  bool busy = false;
  bool contains_game_frame = false;
};

using GuestFrame = std::array<float, AudoutOutput::kFrameSamples>;

void ConvertFrame(const GuestFrame& input, std::array<int16_t, 2 *
                                                           AudoutOutput::kSamplesPerChannel>& output) {
  std::array<float, 2 * AudoutOutput::kSamplesPerChannel> stereo{};
  conversion::sequential_6_BE_to_interleaved_2_LE(
      stereo.data(), input.data(), AudoutOutput::kSamplesPerChannel, GetStereoFold(),
      GetOutputGain());
  for (size_t i = 0; i < stereo.size(); ++i) {
    const float sample = std::isfinite(stereo[i]) ? std::clamp(stereo[i], -1.0f, 1.0f) : 0.0f;
    output[i] = static_cast<int16_t>(std::lrint(sample * 32767.0f));
  }
}

}  // namespace

class AudoutOutput::Impl {
 public:
  Impl(std::function<void()> consumed, std::function<void(std::string_view)> report_error)
      : frame_consumed(std::move(consumed)), report_error(std::move(report_error)) {}

  bool Fail(const char* operation, Result result) {
    if (report_error) {
      char message[128]{};
      std::snprintf(message, sizeof(message), "%s failed (libnx result 0x%08x)", operation,
                    static_cast<unsigned>(result));
      report_error(message);
    }
    return false;
  }

  void Convert(GuestFrame& frame, OutputSlot& slot) {
    std::array<int16_t, 2 * AudoutOutput::kSamplesPerChannel> pcm{};
    ConvertFrame(frame, pcm);
    std::memcpy(slot.data.data(), pcm.data(), kFrameBytes);
  }

  void Run() {
    for (;;) {
      OutputSlot* next = nullptr;
      GuestFrame frame{};
      {
        std::unique_lock lock(mutex);
        if (!running) break;
        if (pause_requested && queue.empty() && active_slots == 0) {
          paused = true;
          changed.notify_all();
          changed.wait(lock, [this] { return !running || !paused; });
          if (!running) break;
        }
        if (paused) {
          changed.wait(lock, [this] { return !running || !paused; });
          if (!running) break;
        }
        if (!queue.empty()) {
          for (auto& slot : slots) {
            if (!slot.busy) {
              next = &slot;
              break;
            }
          }
          if (next) {
            frame = std::move(queue.front());
            queue.pop_front();
            next->busy = true;
            next->contains_game_frame = true;
            ++active_slots;
            changed.notify_all();
          }
        }
      }

      if (next) {
        Convert(frame, *next);
        next->buffer = {};
        next->buffer.buffer = next->data.data();
        next->buffer.buffer_size = next->data.size();
        next->buffer.data_size = kFrameBytes;
        next->buffer.data_offset = 0;
        const Result result = audoutAppendAudioOutBuffer(&next->buffer);
        if (R_FAILED(result)) {
          {
            std::lock_guard lock(mutex);
            next->busy = false;
            next->contains_game_frame = false;
            --active_slots;
            running = false;
            changed.notify_all();
          }
          Fail("audoutAppendAudioOutBuffer", result);
          continue;
        }
        continue;
      }

      bool should_wait_for_device = false;
      {
        std::unique_lock lock(mutex);
        should_wait_for_device = active_slots != 0;
        if (!should_wait_for_device && queue.empty() && !pause_requested && running) {
          changed.wait_for(lock, std::chrono::milliseconds(10), [this] {
            return !running || paused || pause_requested || !queue.empty();
          });
        }
      }
      if (!should_wait_for_device) continue;

      AudioOutBuffer* released = nullptr;
      u32 released_count = 0;
      const Result wait_result = audoutWaitPlayFinish(&released, &released_count,
                                                      kWaitTimeoutNs);
      if (R_FAILED(wait_result)) continue;  // A timeout is the worker's stop/pause poll.
      while (released) {
        AudioOutBuffer* current = released;
        released = released->next;
        bool consumed = false;
        {
          std::lock_guard lock(mutex);
          for (auto& slot : slots) {
            if (&slot.buffer == current) {
              consumed = slot.contains_game_frame;
              slot.busy = false;
              slot.contains_game_frame = false;
              if (active_slots) --active_slots;
              break;
            }
          }
          changed.notify_all();
        }
        if (consumed && frame_consumed) frame_consumed();
      }
      (void)released_count;
    }
  }

  std::function<void()> frame_consumed;
  std::function<void(std::string_view)> report_error;
  std::array<OutputSlot, kOutputSlots> slots{};
  std::deque<GuestFrame> queue;
  mutable std::mutex mutex;
  std::condition_variable changed;
  std::thread worker;
  uint32_t sample_rate = 0;
  uint32_t channels = 0;
  bool initialized = false;
  bool running = false;
  bool pause_requested = false;
  bool paused = false;
  size_t active_slots = 0;
};

AudoutOutput::AudoutOutput(std::function<void()> frame_consumed,
                           std::function<void(std::string_view)> report_error)
    : impl_(std::make_unique<Impl>(std::move(frame_consumed), std::move(report_error))) {}

AudoutOutput::~AudoutOutput() { Close(); }

bool AudoutOutput::Open(bool start_paused) {
  auto& self = *impl_;
  Close();
  Result result = audoutInitialize();
  if (R_FAILED(result)) return self.Fail("audoutInitialize", result);
  self.initialized = true;

  char device_name[0x100]{};
  PcmFormat format = PcmFormat_Invalid;
  AudioOutState state = AudioOutState_Stopped;
  result = audoutOpenAudioOut("MainAudioOut", device_name, kRate, kChannels,
                              &self.sample_rate, &self.channels, &format, &state);
  if (R_FAILED(result)) {
    self.Fail("audoutOpenAudioOut", result);
    Close();
    return false;
  }
  if (self.sample_rate != kRate || self.channels != kChannels || format != PcmFormat_Int16) {
    if (self.report_error) self.report_error("audout did not provide 48 kHz stereo PCM16");
    Close();
    return false;
  }

  if (!start_paused) {
    result = audoutStartAudioOut();
    if (R_FAILED(result)) {
      self.Fail("audoutStartAudioOut", result);
      Close();
      return false;
    }
  }
  {
    std::lock_guard lock(self.mutex);
    self.running = true;
    self.paused = start_paused;
    self.pause_requested = start_paused;
    self.active_slots = 0;
    self.queue.clear();
    for (auto& slot : self.slots) slot = {};
  }
  self.worker = std::thread([&self] { self.Run(); });
  return true;
}

void AudoutOutput::Close() {
  auto& self = *impl_;
  {
    std::lock_guard lock(self.mutex);
    self.running = false;
    self.paused = false;
    self.pause_requested = false;
    self.changed.notify_all();
  }
  if (self.worker.joinable()) self.worker.join();
  if (self.initialized) {
    audoutStopAudioOut();
    bool flushed = false;
    audoutFlushAudioOutBuffers(&flushed);
    audoutExit();
    self.initialized = false;
  }
  std::lock_guard lock(self.mutex);
  self.queue.clear();
  self.active_slots = 0;
  for (auto& slot : self.slots) slot = {};
}

bool AudoutOutput::Submit(std::span<const float> planar_big_endian) {
  if (planar_big_endian.size() != kFrameSamples) return false;
  auto& self = *impl_;
  GuestFrame frame{};
  std::memcpy(frame.data(), planar_big_endian.data(), sizeof(frame));
  std::unique_lock lock(self.mutex);
  self.changed.wait(lock, [&self] {
    return !self.running || self.queue.size() < kMaximumQueuedFrames;
  });
  if (!self.running) return false;
  self.queue.push_back(std::move(frame));
  self.changed.notify_all();
  return true;
}

bool AudoutOutput::Pause() {
  auto& self = *impl_;
  std::unique_lock lock(self.mutex);
  if (!self.running) return false;
  self.pause_requested = true;
  self.changed.notify_all();
  self.changed.wait(lock, [&self] { return !self.running || self.paused; });
  const bool ready = self.running && self.paused;
  lock.unlock();
  if (!ready) return false;
  const Result result = audoutStopAudioOut();
  return R_SUCCEEDED(result) || self.Fail("audoutStopAudioOut", result);
}

bool AudoutOutput::Resume() {
  auto& self = *impl_;
  std::lock_guard lock(self.mutex);
  if (!self.running) return false;
  if (self.paused) {
    const Result result = audoutStartAudioOut();
    if (R_FAILED(result)) return self.Fail("audoutStartAudioOut", result);
  }
  self.paused = false;
  self.pause_requested = false;
  self.changed.notify_all();
  return true;
}

namespace {

class SwitchAudioDriver final : public AudioDriver {
 public:
  SwitchAudioDriver(rex::memory::Memory* memory, std::function<void()> consumed)
      : AudioDriver(memory), output_(std::move(consumed), [](std::string_view) {}) {}

  bool Initialize(bool start_paused) { return output_.Open(start_paused); }
  void SubmitFrame(uint32_t frame_ptr) override {
    const auto* frame = memory_->TranslateVirtual<float*>(frame_ptr);
    if (!output_.Submit({frame, AudoutOutput::kFrameSamples})) {
      REXAPU_ERROR("Switch audio output rejected a guest frame");
    }
  }
  bool Pause() override { return output_.Pause(); }
  bool Resume() override { return output_.Resume(); }
  void Shutdown() { output_.Close(); }

 private:
  AudoutOutput output_;
};

}  // namespace

class SwitchAudioSystem final : public AudioSystem {
 public:
  explicit SwitchAudioSystem(rex::runtime::FunctionDispatcher* dispatcher)
      : AudioSystem(dispatcher) {}
  ~SwitchAudioSystem() override { Shutdown(); }

  static std::unique_ptr<AudioSystem> Create(rex::runtime::FunctionDispatcher* dispatcher) {
    return std::make_unique<SwitchAudioSystem>(dispatcher);
  }

  X_STATUS CreateDriver(size_t, std::function<void()> consumed, AudioDriver** out_driver) override {
    if (!out_driver) return X_STATUS_INVALID_PARAMETER;
    auto* driver = new SwitchAudioDriver(memory_, std::move(consumed));
    if (!driver->Initialize(is_paused())) {
      driver->Shutdown();
      delete driver;
      return X_STATUS_UNSUCCESSFUL;
    }
    *out_driver = driver;
    return X_STATUS_SUCCESS;
  }

  void DestroyDriver(AudioDriver* driver) override {
    auto* switch_driver = dynamic_cast<SwitchAudioDriver*>(driver);
    if (!switch_driver) return;
    switch_driver->Shutdown();
    delete switch_driver;
  }
};

std::unique_ptr<rex::system::IAudioSystem> CreateSwitchAudioSystem(
    rex::runtime::FunctionDispatcher* dispatcher) {
  return SwitchAudioSystem::Create(dispatcher);
}

}  // namespace gh::audio::switch_
