// Audio scheduling derived from Xenia, Copyright 2022 Ben Vanik.
// Released under the BSD license; see rexglue-sdk-main/LICENSE.
#pragma once
#include "gh_audio_context.h"
#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>

namespace gh::audio {

// Owns all hardware contexts, MMIO register state and decoder work scheduling.
// The runtime adapter allocates guest context storage and registers MMIO traps.
class XmaRuntime {
 public:
  static constexpr uint32_t kContextCount = 320;
  ~XmaRuntime();
  bool Initialize(GuestAudioMemory memory, uint32_t context_virtual, uint32_t context_physical,
                  std::function<void()> decoded = {});
  void Stop();
  void Pause();
  void Resume();
  bool paused() const { return paused_; }
  uint32_t context_array_ptr() const { return registers_[0x600]; }
  uint32_t AllocateContext();
  void ReleaseContext(uint32_t guest_ptr);
  bool BlockOnContext(uint32_t guest_ptr, bool poll);
  uint32_t ReadRegister(uint32_t address);
  void WriteRegister(uint32_t address, uint32_t value);
  struct State {
    uint32_t version = 1, context_base = 0;
    std::array<uint32_t, 0x4000> registers{};
    std::array<bool, kContextCount> allocated{};
    std::vector<XmaContext::State> contexts;
  };
  // Caller also stops guest/MMIO writers. The worker must remain parked until
  // guest memory and every other device have been restored.
  bool SaveState(State& state);
  bool RestoreState(const State& state);
 private:
  int ContextIndex(uint32_t guest_ptr) const;
  void Run();
  std::array<uint32_t, 0x4000> registers_{};
  std::array<XmaContext, kContextCount> contexts_;
  std::array<bool, kContextCount> allocated_{};
  std::mutex allocation_mutex_;
  std::recursive_mutex state_mutex_;
  uint32_t context_base_ = 0;
  std::atomic<bool> running_{false}, paused_{false};
  std::atomic<bool> worker_parked_{false};
  kernel::Event work_{false}, pause_ready_{false}, resume_{false};
  std::unique_ptr<kernel::Thread> worker_;
  std::function<void()> decoded_;
};

// Owns endpoint lifetime, guest callback metadata and playback credits. A
// device returns one credit only after consuming an entire submitted frame.
class ClientScheduler {
 public:
  static constexpr size_t kClientCount = 8;
  struct Callback { uint32_t function = 0, argument = 0, wrapped_argument = 0; };
  struct Endpoint {
    std::function<void(uint32_t)> submit;
    std::function<void()> close;
    std::function<void()> free_argument;
    std::function<bool()> pause{}, resume{};
    std::function<bool(std::vector<uint8_t>&)> save{};
    std::function<bool(std::span<const uint8_t>)> restore{};
  };
  explicit ClientScheduler(uint32_t queued_frames);
  ~ClientScheduler();
  int FindFree() const;
  bool Attach(size_t index, Callback callback, Endpoint endpoint);
  void Detach(size_t index, bool free_argument = true);
  void CloseAll(bool free_arguments = true);
  bool Submit(size_t index, uint32_t samples);
  bool FrameConsumed(size_t index);
  bool InUse(size_t index) const;
  Callback GetCallback(size_t index) const;
  void Start();
  void Stop();
  void Run(const std::function<void(size_t, Callback)>& dispatch);
  void Pause();
  bool Resume();
  bool running() const { return running_; }
  bool paused() const { return paused_; }
  uint32_t queued_frames() const { return queued_frames_; }
  struct State {
    uint32_t version = 1, queued_frames = 0;
    struct ClientState {
      Callback callback;
      bool in_use = false;
      uint32_t credits = 0;
      std::vector<uint8_t> endpoint;
    };
    std::array<ClientState, kClientCount> clients;
  };
  bool SaveState(State& state);
  bool RestoreState(const State& state);
 private:
  struct Client { Callback callback; Endpoint endpoint; bool in_use = false; };
  mutable std::recursive_mutex mutex_;
  std::array<Client, kClientCount> clients_;
  std::array<std::unique_ptr<kernel::Semaphore>, kClientCount> credits_;
  std::array<kernel::Waitable*, kClientCount + 1> waits_;
  kernel::Event wake_{false}, pause_ready_{false}, resume_{false};
  std::atomic<bool> running_{false}, paused_{false};
  std::atomic<bool> worker_parked_{false};
  bool endpoints_paused_ = false;
  uint32_t queued_frames_;
};
}  // namespace gh::audio
