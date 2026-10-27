// Audio scheduling derived from Xenia, Copyright 2022 Ben Vanik.
// Released under the BSD license; see rexglue-sdk-main/LICENSE.
#include "gh_audio_runtime.h"
#include <algorithm>
#include <bit>
#include <cassert>
#include <chrono>

namespace gh::audio {
XmaRuntime::~XmaRuntime() { Stop(); }
bool XmaRuntime::Initialize(GuestAudioMemory memory, uint32_t context_virtual,
                            uint32_t context_physical, std::function<void()> decoded) {
  Stop();
  context_base_ = context_virtual;
  registers_.fill(0);
  allocated_.fill(false);
  registers_[0x600] = context_physical;
  registers_[0x607] = 1;
  decoded_ = std::move(decoded);
  memory.cancelled = [this] { return !running_.load(); };
  for (size_t i = 0; i < contexts_.size(); ++i) {
    if (contexts_[i].Setup(uint32_t(i), memory, context_base_ + uint32_t(i) * 64)) return false;
  }
  running_ = true;
  paused_ = false;
  work_.Reset(); pause_ready_.Reset(); resume_.Reset();
  worker_ = std::make_unique<kernel::Thread>([this] { Run(); });
  return true;
}
void XmaRuntime::Run() {
  while (running_) {
    bool did_work = false;
    for (auto& context : contexts_) {
      if (!running_) break;
      if (context.Work()) {
        context.SignalWorkDone();
        if (decoded_) decoded_();
        did_work = true;
      }
    }
    if (paused_ && running_) {
      worker_parked_ = true;
      pause_ready_.Set();
      resume_.Wait();
      worker_parked_ = false;
    }
    if (!did_work && running_) work_.Wait();
  }
}
void XmaRuntime::Stop() {
  running_ = false;
  paused_ = false;
  work_.Set(); resume_.Set();
  pause_ready_.Set();
  if (worker_) { worker_->Join(); worker_.reset(); }
  for (auto& context : contexts_) context.Disable();
}
void XmaRuntime::Pause() {
  if (!running_ || paused_.exchange(true)) return;
  work_.Set(); pause_ready_.Wait();
}
void XmaRuntime::Resume() { if (paused_.exchange(false)) resume_.Set(); }
int XmaRuntime::ContextIndex(uint32_t ptr) const {
  if (ptr < context_base_ || ptr >= context_base_ + kContextCount * 64 || (ptr & 63)) return -1;
  return int((ptr-context_base_)/64);
}
uint32_t XmaRuntime::AllocateContext() {
  std::lock_guard state_lock(state_mutex_);
  std::lock_guard lock(allocation_mutex_);
  for (size_t i = 0; i < allocated_.size(); ++i) if (!allocated_[i]) {
    allocated_[i] = true;
    contexts_[i].set_is_allocated(true);
    return contexts_[i].guest_ptr();
  }
  return 0;
}
void XmaRuntime::ReleaseContext(uint32_t ptr) {
  std::lock_guard state_lock(state_mutex_);
  const int index = ContextIndex(ptr);
  assert(index >= 0);
  if (index < 0) return;
  std::lock_guard lock(allocation_mutex_);
  contexts_[index].Release(); allocated_[index] = false;
}
bool XmaRuntime::BlockOnContext(uint32_t ptr, bool poll) {
  const int index = ContextIndex(ptr);
  return index >= 0 && contexts_[index].Block(poll);
}
uint32_t XmaRuntime::ReadRegister(uint32_t address) {
  std::lock_guard state_lock(state_mutex_);
  const uint32_t index = (address & 0xFFFF)/4;
  if (index == 0x606) {
    registers_[0x606] = registers_[0x607];
    registers_[0x607] = (registers_[0x607] + 1) % kContextCount;
  }
  return std::byteswap(registers_[index]);
}
void XmaRuntime::WriteRegister(uint32_t address, uint32_t value) {
  std::lock_guard state_lock(state_mutex_);
  const uint32_t index = (address & 0xFFFF)/4;
  value = std::byteswap(value);
  registers_[index] = value;
  uint32_t group = 0;
  enum { None, Kick, Lock, Clear } command = None;
  if (index >= 0x650 && index <= 0x659) { group = index-0x650; command = Kick; }
  else if (index >= 0x690 && index <= 0x699) { group = index-0x690; command = Lock; }
  else if (index >= 0x6A0 && index <= 0x6A9) { group = index-0x6A0; command = Clear; }
  for (uint32_t bit = 0; bit < 32; ++bit) if (value & (uint32_t{1} << bit)) {
    if (command == None) break;
    auto& context = contexts_[group*32+bit];
    if (command == Kick) context.Enable();
    else if (command == Lock) { context.Disable(); context.Block(false); }
    else context.Clear();
  }
  if (command == Kick) {
    // Keep the existing inline kick path to avoid a worker-sweep delay on the
    // realtime render callback. The context serializes inline and worker decode.
    for (uint32_t bit = 0; bit < 32; ++bit) if (value & (uint32_t{1} << bit)) {
      auto& context = contexts_[group*32+bit];
      if (context.Work()) context.SignalWorkDone();
    }
    work_.Set();
  }
}

bool XmaRuntime::SaveState(State& state) {
  std::lock_guard state_lock(state_mutex_);
  if (!running_ || !paused_ || !worker_parked_) return false;
  std::lock_guard allocation_lock(allocation_mutex_);
  State saved;
  saved.context_base = context_base_;
  saved.registers = registers_;
  saved.allocated = allocated_;
  saved.contexts.reserve(contexts_.size());
  for (auto& context : contexts_) {
    saved.contexts.push_back(context.SaveState());
    if (saved.contexts.back().decoder.empty()) return false;
  }
  state = std::move(saved);
  return true;
}

bool XmaRuntime::RestoreState(const State& state) {
  std::lock_guard state_lock(state_mutex_);
  if (!running_ || !paused_ || !worker_parked_ || state.version != 1 ||
      state.context_base != context_base_ || state.registers[0x600] != registers_[0x600] ||
      state.contexts.size() != contexts_.size()) return false;
  std::lock_guard allocation_lock(allocation_mutex_);
  // Validate all payloads before replacing any context. Allocation failures
  // still leave the worker parked; the caller must not resume a failed restore.
  for (size_t i = 0; i < contexts_.size(); ++i) {
    const auto& context = state.contexts[i];
    if (context.version != 1 || context.id != i ||
        context.guest_ptr != context_base_ + i * 64 ||
        context.allocated != state.allocated[i] || context.remaining_subframes > 8 ||
        context.loop_output_limit > 8 || context.pending_output_limit > 8 ||
        context.pending_start_skip > 8) return false;
    XmaFrameDecoder validation;
    if (!validation.RestoreState(context.decoder)) return false;
  }
  for (size_t i = 0; i < contexts_.size(); ++i)
    if (!contexts_[i].RestoreState(state.contexts[i])) return false;
  registers_ = state.registers;
  allocated_ = state.allocated;
  work_.Set();
  return true;
}

ClientScheduler::ClientScheduler(uint32_t frames) : queued_frames_(std::clamp(frames, 4u, 64u)) {
  for (size_t i = 0; i < kClientCount; ++i) {
    credits_[i] = std::make_unique<kernel::Semaphore>(0, queued_frames_);
    waits_[i] = credits_[i].get();
  }
  waits_.back() = &wake_;
}
ClientScheduler::~ClientScheduler() { Stop(); CloseAll(); }
int ClientScheduler::FindFree() const {
  std::lock_guard lock(mutex_);
  for (size_t i = 0; i < kClientCount; ++i) if (!clients_[i].in_use) return int(i);
  return -1;
}
bool ClientScheduler::Attach(size_t index, Callback callback, Endpoint endpoint) {
  std::lock_guard lock(mutex_);
  if (index >= kClientCount || clients_[index].in_use) return false;
  clients_[index] = {callback, std::move(endpoint), true};
  const bool released = credits_[index]->Release(queued_frames_);
  assert(released);
  return released;
}
void ClientScheduler::Detach(size_t index, bool free_argument) {
  std::lock_guard lock(mutex_);
  if (index >= kClientCount || !clients_[index].in_use) return;
  auto& client = clients_[index];
  // Close joins device callbacks before the credit source is drained/reused.
  if (client.endpoint.close) client.endpoint.close();
  if (free_argument && client.endpoint.free_argument) client.endpoint.free_argument();
  client = {};
  while (credits_[index]->Wait(std::chrono::nanoseconds::zero()) == kernel::WaitResult::Signaled) {}
}
void ClientScheduler::CloseAll(bool free_arguments) {
  for (size_t i = 0; i < kClientCount; ++i) Detach(i, free_arguments);
}
bool ClientScheduler::Submit(size_t index, uint32_t samples) {
  std::lock_guard lock(mutex_);
  if (index >= kClientCount || !clients_[index].in_use || !clients_[index].endpoint.submit) return false;
  clients_[index].endpoint.submit(samples);
  return true;
}
bool ClientScheduler::FrameConsumed(size_t index) {
  return index < kClientCount && credits_[index]->Release();
}
bool ClientScheduler::InUse(size_t index) const {
  std::lock_guard lock(mutex_);
  return index < kClientCount && clients_[index].in_use;
}
ClientScheduler::Callback ClientScheduler::GetCallback(size_t index) const {
  std::lock_guard lock(mutex_);
  return index < kClientCount && clients_[index].in_use ? clients_[index].callback : Callback{};
}
void ClientScheduler::Start() {
  paused_ = false; worker_parked_ = false; endpoints_paused_ = false;
  wake_.Reset(); pause_ready_.Reset(); resume_.Reset(); running_ = true;
}
void ClientScheduler::Stop() {
  running_ = false; paused_ = false; wake_.Set(); resume_.Set(); pause_ready_.Set();
}
void ClientScheduler::Run(const std::function<void(size_t, Callback)>& dispatch) {
  while (running_) {
    if (paused_) {
      worker_parked_ = true;
      pause_ready_.Set(); resume_.Wait();
      worker_parked_ = false;
      continue;
    }
    auto result = kernel::WaitMultiple(waits_, false, std::chrono::milliseconds(500));
    if (!running_) break;
    if (paused_) {
      // Pause may race a wait that already consumed one callback credit.
      if (result.first == kernel::WaitResult::Signaled && result.second < kClientCount)
        credits_[result.second]->Release();
      continue;
    }
    if (result.first == kernel::WaitResult::Signaled && result.second == kClientCount) {
      continue;
    }
    bool pumped = false;
    if (result.first == kernel::WaitResult::Signaled) {
      auto callback = GetCallback(result.second);
      if (callback.function) dispatch(result.second, callback);
      pumped = true;
    }
    if (!pumped && running_) {
      // Retain the idle backoff, but allow shutdown/pause to wake it.
      wake_.Wait(std::chrono::milliseconds(500));
    }
  }
}
void ClientScheduler::Pause() {
  if (!running_ || paused_.exchange(true)) return;
  wake_.Set(); pause_ready_.Wait();
  std::lock_guard lock(mutex_);
  endpoints_paused_ = true;
  for (auto& client : clients_)
    if (client.in_use && client.endpoint.pause && !client.endpoint.pause()) endpoints_paused_ = false;
}
bool ClientScheduler::Resume() {
  if (!paused_) return true;
  {
    std::lock_guard lock(mutex_);
    endpoints_paused_ = false;
    for (auto& client : clients_) {
      if (client.in_use && client.endpoint.resume && !client.endpoint.resume()) {
        // Keep all endpoints parked if one device cannot resume.
        endpoints_paused_ = true;
        for (auto& endpoint : clients_)
          if (endpoint.in_use && endpoint.endpoint.pause && !endpoint.endpoint.pause())
            endpoints_paused_ = false;
        return false;
      }
    }
  }
  if (paused_.exchange(false)) resume_.Set();
  return true;
}
bool ClientScheduler::SaveState(State& state) {
  std::lock_guard lock(mutex_);
  if (!paused_ || !worker_parked_ || !endpoints_paused_) return false;
  State saved; saved.queued_frames = queued_frames_;
  for (size_t i = 0; i < kClientCount; ++i) {
    auto& target = saved.clients[i]; const auto& client = clients_[i];
    target.in_use = client.in_use; target.callback = client.callback;
    target.credits = credits_[i]->count();
    if (client.in_use && (!client.endpoint.save || !client.endpoint.save(target.endpoint))) return false;
  }
  state = std::move(saved); return true;
}
bool ClientScheduler::RestoreState(const State& state) {
  std::lock_guard lock(mutex_);
  if (!paused_ || !worker_parked_ || state.version != 1 || state.queued_frames != queued_frames_) return false;
  for (size_t i = 0; i < kClientCount; ++i) {
    const auto& source = state.clients[i]; const auto& client = clients_[i];
    if (source.in_use != client.in_use || source.credits > queued_frames_ ||
        source.callback.function != client.callback.function ||
        source.callback.argument != client.callback.argument ||
        source.callback.wrapped_argument != client.callback.wrapped_argument) return false;
  }
  for (size_t i = 0; i < kClientCount; ++i) {
    const auto& source = state.clients[i]; const auto& client = clients_[i];
    if (source.in_use && (!client.endpoint.restore || !client.endpoint.restore(source.endpoint))) return false;
    while (credits_[i]->Wait(std::chrono::nanoseconds::zero()) == kernel::WaitResult::Signaled) {}
    if (source.credits && !credits_[i]->Release(source.credits)) return false;
  }
  endpoints_paused_ = true;
  return true;
}
}  // namespace gh::audio
