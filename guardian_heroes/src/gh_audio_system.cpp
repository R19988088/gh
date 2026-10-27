/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <algorithm>
#include <array>
#include <chrono>
#include <rex/assert.h>
#include "gh_audio_driver.h"
#include "gh_audio_system.h"
#include <rex/audio/flags.h>
#include "gh_audio_decoder.h"
#include "gh_audio_state_io.h"
#include <rex/dbg.h>
#include <rex/logging.h>
#include <rex/math.h>
#include <rex/memory/ring_buffer.h>
#include <rex/stream.h>
#include <rex/string/buffer.h>
#include <rex/system/thread_state.h>
#include <rex/thread.h>
#include <rex/cvar.h>

REXCVAR_DEFINE_INT32(
    audio_maxqframes, 8, "Audio",
    "Max buffered audio frames (range 4-64). Lower reduces latency but may cause stuttering.");

// As with normal Microsoft, there are like twelve different ways to access
// the audio APIs. Early games use XMA*() methods almost exclusively to touch
// decoders. Later games use XAudio*() and direct memory writes to the XMA
// structures (as opposed to the XMA* calls), meaning that we have to support
// both.
//
// For ease of implementation, most audio related processing is handled in
// AudioSystem, and the functions here call off to it.
// The XMA*() functions just manipulate the audio system in the guest context
// and let the normal AudioSystem handling take it, to prevent duplicate
// implementations. They can be found in xboxkrnl_audio_xma.cc

namespace gh::audio {

namespace {
constexpr std::chrono::milliseconds kWorkerShutdownTimeout{500};

void WriteXma(state_io::Writer& out, const XmaRuntime::State& state) {
  out.Put(state.version); out.Put(state.context_base); out.Put(state.registers); out.Put(state.allocated);
  out.Put(static_cast<uint32_t>(state.contexts.size()));
  for (const auto& c : state.contexts) {
    out.Put(c.version); out.Put(c.id); out.Put(c.guest_ptr);
    out.Put(c.allocated); out.Put(c.enabled); out.Put(c.completion_signaled); out.Blob(c.decoder);
    out.Put(c.raw_frame); out.Put(c.carry_frame); out.Put(c.remaining_subframe_blocks);
    out.Put(c.remaining_subframes); out.Put(c.loop_output_limit); out.Put(c.loop_start_skip_pending);
    out.Put(c.carry_valid); out.Put(c.pending_output_limit); out.Put(c.pending_start_skip);
  }
}
XmaRuntime::State ReadXma(state_io::Reader& in) {
  XmaRuntime::State state;
  state.version = in.Get<uint32_t>(); state.context_base = in.Get<uint32_t>();
  state.registers = in.Get<decltype(state.registers)>(); state.allocated = in.Get<decltype(state.allocated)>();
  if (in.Get<uint32_t>() != XmaRuntime::kContextCount) throw std::runtime_error("Invalid XMA context count");
  state.contexts.resize(XmaRuntime::kContextCount);
  for (auto& c : state.contexts) {
    c.version = in.Get<uint32_t>(); c.id = in.Get<uint32_t>(); c.guest_ptr = in.Get<uint32_t>();
    c.allocated = in.Get<bool>(); c.enabled = in.Get<bool>(); c.completion_signaled = in.Get<bool>();
    c.decoder = in.Blob(); c.raw_frame = in.Get<decltype(c.raw_frame)>(); c.carry_frame = in.Get<decltype(c.carry_frame)>();
    c.remaining_subframe_blocks = in.Get<int32_t>(); c.remaining_subframes = in.Get<uint8_t>();
    c.loop_output_limit = in.Get<uint8_t>(); c.loop_start_skip_pending = in.Get<bool>(); c.carry_valid = in.Get<bool>();
    c.pending_output_limit = in.Get<uint8_t>(); c.pending_start_skip = in.Get<uint8_t>();
  }
  return state;
}
void WriteClients(state_io::Writer& out, const ClientScheduler::State& state) {
  out.Put(state.version); out.Put(state.queued_frames);
  for (const auto& c : state.clients) {
    out.Put(c.in_use); out.Put(c.callback); out.Put(c.credits); out.Blob(c.endpoint);
  }
}
ClientScheduler::State ReadClients(state_io::Reader& in) {
  ClientScheduler::State state;
  state.version = in.Get<uint32_t>(); state.queued_frames = in.Get<uint32_t>();
  for (auto& c : state.clients) {
    c.in_use = in.Get<bool>(); c.callback = in.Get<ClientScheduler::Callback>();
    c.credits = in.Get<uint32_t>(); c.endpoint = in.Blob();
  }
  return state;
}
}  // namespace

AudioSystem::AudioSystem(rex::runtime::FunctionDispatcher* function_dispatcher)
    : memory_(function_dispatcher->memory()), function_dispatcher_(function_dispatcher),
      clients_(static_cast<uint32_t>(REXCVAR_GET(audio_maxqframes))) {
  queued_frames_ = clients_.queued_frames();
  xma_decoder_ = std::make_unique<XmaDecoder>(function_dispatcher_);
}

AudioSystem::~AudioSystem() { if (xma_decoder_) xma_decoder_->Shutdown(); }

X_STATUS AudioSystem::Setup(rex::system::KernelState* kernel_state) {
  const auto status = xma_decoder_->Setup(kernel_state);
  if (XFAILED(status)) return status;
  clients_.Start();
  worker_thread_ = rex::system::object_ref<rex::system::XHostThread>(
      new rex::system::XHostThread(kernel_state, 128 * 1024, 0, [this] {
        WorkerThreadMain();
        return 0;
      }));
  worker_thread_->set_name("Audio Worker");
  return worker_thread_->Create();
}

void AudioSystem::Initialize() {}
void AudioSystem::WorkerThreadMain() {
  Initialize();
  struct CallbackStatistics {
    uint64_t count = 0;
    double total_ms = 0, max_ms = 0;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  };
  std::array<CallbackStatistics, ClientScheduler::kClientCount> statistics;
  clients_.Run([this, &statistics](size_t index, gh::audio::ClientScheduler::Callback callback) {
    SCOPE_profile_cpu_i("apu", "rex::audio::AudioSystem->client_callback");
    uint64_t args[] = {callback.wrapped_argument};
    const auto begin = std::chrono::steady_clock::now();
    function_dispatcher_->Execute(worker_thread_->thread_state(), callback.function, args, 1);
    const auto end = std::chrono::steady_clock::now();
    auto& stats = statistics[index];
    const double elapsed_ms = std::chrono::duration<double, std::milli>(end - begin).count();
    ++stats.count;
    stats.total_ms += elapsed_ms;
    stats.max_ms = std::max(stats.max_ms, elapsed_ms);
    if (end - stats.start >= std::chrono::seconds(2)) {
      REXAPU_INFO("Audio scheduler client={} callback={:08X}: count={} avg={:.3f}ms max={:.3f}ms",
          index, callback.function, stats.count, stats.total_ms / stats.count, stats.max_ms);
      stats = {};
      stats.start = end;
    }
  });
}

void AudioSystem::Shutdown() {
  clients_.Stop();
  if (worker_thread_) {
    uint64_t timeout = static_cast<uint64_t>(-kWorkerShutdownTimeout.count() * 10'000);
    const bool exited = worker_thread_->Wait(0, 0, 0, &timeout) == X_STATUS_SUCCESS;
    if (!exited) {
      REXAPU_WARN("Audio worker did not exit within {} ms; terminating",
                  kWorkerShutdownTimeout.count());
      worker_thread_->Terminate(0);
    }
    // The guest completion event precedes native unwind and kernel unregister.
    worker_thread_->thread()->Join();
    worker_thread_.reset();
  }
  // The guest callback can kick or block on XMA contexts. Keep those contexts
  // alive until the callback thread has exited, including cancellation unwind.
  if (xma_decoder_) xma_decoder_->Shutdown();
  clients_.CloseAll();
}

X_STATUS AudioSystem::RegisterClient(uint32_t callback, uint32_t callback_arg, size_t* out_index) {
  auto global_lock = global_critical_region_.Acquire();
  const int index = clients_.FindFree();
  if (index < 0) return X_STATUS_UNSUCCESSFUL;
  AudioDriver* driver = nullptr;
  const auto status = CreateDriver(size_t(index), [this, index] {
    const bool released = clients_.FrameConsumed(size_t(index));
    assert_true(released);
  }, &driver);
  if (XFAILED(status)) return status;
  assert_not_null(driver);
  REXAPU_INFO("Audio client={} endpoint={:X} callback={:08X}", index,
      reinterpret_cast<uintptr_t>(driver), callback);
  const uint32_t pointer = memory_->SystemHeapAlloc(4);
  if (!pointer) {
    DestroyDriver(driver);
    return X_STATUS_UNSUCCESSFUL;
  }
  rex::memory::store_and_swap<uint32_t>(memory_->TranslateVirtual(pointer), callback_arg);
  const bool attached = AttachClient(size_t(index), {callback, callback_arg, pointer}, driver);
  if (!attached) {
    DestroyDriver(driver);
    memory_->SystemHeapFree(pointer);
    return X_STATUS_UNSUCCESSFUL;
  }
  if (out_index) *out_index = size_t(index);
  return X_STATUS_SUCCESS;
}

bool AudioSystem::AttachClient(size_t index, ClientScheduler::Callback callback, AudioDriver* driver) {
  return clients_.Attach(index, callback, {
      [driver](uint32_t samples) { driver->SubmitFrame(samples); },
      [this, driver] { DestroyDriver(driver); },
      [this, pointer = callback.wrapped_argument] { memory_->SystemHeapFree(pointer); },
      [driver] { return driver->Pause(); }, [driver] { return driver->Resume(); },
      [driver](auto& state) { return driver->SaveState(state); },
      [driver](auto state) { return driver->RestoreState(state); }});
}

void AudioSystem::SubmitFrame(size_t index, uint32_t samples_ptr) {
  SCOPE_profile_cpu_f("apu");
  const bool submitted = clients_.Submit(index, samples_ptr);
  assert_true(submitted);
}
void AudioSystem::UnregisterClient(size_t index) {
  auto global_lock = global_critical_region_.Acquire();
  clients_.Detach(index);
}

bool AudioSystem::Save(rex::stream::ByteStream* stream) {
  if (!is_paused() || restore_failed_) return false;
  XmaRuntime::State xma;
  ClientScheduler::State clients;
  if (!xma_decoder_->SaveState(xma) || !clients_.SaveState(clients)) return false;
  state_io::Writer writer;
  WriteXma(writer, xma); WriteClients(writer, clients);
  stream->Write(kAudioCompleteSaveSignature);
  stream->Write(static_cast<uint32_t>(writer.bytes.size()));
  stream->Write(std::span<const uint8_t>(writer.bytes));
  return true;
}

bool AudioSystem::Restore(rex::stream::ByteStream* stream) {
  if (!is_paused()) return false;
  try {
    const auto signature = stream->Read<uint32_t>();
    ClientScheduler::State clients;
    XmaRuntime::State xma;
    const bool complete = signature == kAudioCompleteSaveSignature;
    if (complete) {
      const auto size = stream->Read<uint32_t>();
      if (size > stream->data_length() - stream->offset()) return false;
      state_io::Reader reader{{stream->data() + stream->offset(), size}};
      xma = ReadXma(reader); clients = ReadClients(reader);
      if (!reader.bytes.empty() || clients.version != 1 || clients.queued_frames != queued_frames_) return false;
      stream->Advance(size);
    } else if (signature == kAudioSaveSignature) {
      clients.queued_frames = queued_frames_;
      const auto count = stream->Read<uint32_t>();
      if (count > ClientScheduler::kClientCount) return false;
      for (uint32_t i = 0; i < count; ++i) {
        const auto id = stream->Read<uint32_t>();
        if (id >= ClientScheduler::kClientCount || clients.clients[id].in_use) return false;
        auto& client = clients.clients[id]; client.in_use = true; client.credits = queued_frames_;
        client.callback.function = stream->Read<uint32_t>(); client.callback.argument = stream->Read<uint32_t>();
        client.callback.wrapped_argument = stream->Read<uint32_t>();
      }
    } else return false;
    // Current wrappers may refer to allocations from the heap being replaced.
    // Close all endpoints, including IDs absent in the incoming snapshot,
    // without freeing any address from the restored allocation map.
    restore_failed_ = true;
    clients_.CloseAll(false);
    for (size_t id = 0; id < ClientScheduler::kClientCount; ++id) {
      const auto& client = clients.clients[id];
      if (!client.in_use) continue;
      AudioDriver* driver = nullptr;
      const auto status = CreateDriver(id, [this, id] {
        const bool released = clients_.FrameConsumed(id);
        assert_true(released);
      }, &driver);
      if (XFAILED(status)) return false;
      if (!AttachClient(id, client.callback, driver)) { DestroyDriver(driver); return false; }
      if (!driver->Pause()) return false;
    }
    if (complete && (!clients_.RestoreState(clients) || !xma_decoder_->RestoreState(xma))) return false;
    restore_failed_ = false;
    return true;
  } catch (const std::exception& error) {
    REXAPU_ERROR("Audio restore failed: {}", error.what());
    return false;
  }
}
bool AudioSystem::PrepareRestore() {
  if (!is_paused()) return false;
  restore_failed_ = true;
  clients_.CloseAll(false);
  return true;
}
void AudioSystem::Pause() {
  if (clients_.paused()) return;
  clients_.Pause();
  xma_decoder_->Pause();
}
bool AudioSystem::Resume() {
  if (restore_failed_) return false;
  if (!clients_.paused()) return true;
  xma_decoder_->Resume();
  if (!clients_.Resume()) { xma_decoder_->Pause(); return false; }
  return true;
}
}  // namespace gh::audio
