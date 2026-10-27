/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2013 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#pragma once
#include <functional>

#include "gh_audio_runtime.h"
#include <atomic>
#include <queue>

#include <rex/kernel.h>
#include <rex/memory.h>
#include <rex/system/interfaces/audio.h>
#include <rex/system/function_dispatcher.h>
#include <rex/system/xthread.h>
#include <rex/thread.h>
#include <rex/thread/mutex.h>

namespace rex::stream {
class ByteStream;
}  // namespace rex::stream

namespace gh::audio { using rex::X_STATUS; }

namespace gh::audio {

constexpr rex::memory::fourcc_t kAudioSaveSignature = rex::memory::make_fourcc("XAUD");
constexpr rex::memory::fourcc_t kAudioCompleteSaveSignature = rex::memory::make_fourcc("XA2D");

class AudioDriver;
class XmaDecoder;

class AudioSystem : public rex::system::IAudioSystem {
 public:
  virtual ~AudioSystem();

  rex::memory::Memory* memory() const { return memory_; }
  rex::runtime::FunctionDispatcher* function_dispatcher() const { return function_dispatcher_; }
  XmaDecoder* xma_decoder() const { return xma_decoder_.get(); }

  X_STATUS Setup(rex::system::KernelState* kernel_state) override;
  void Shutdown() override;

  X_STATUS RegisterClient(uint32_t callback, uint32_t callback_arg, size_t* out_index);
  void UnregisterClient(size_t index);
  void SubmitFrame(size_t index, uint32_t samples_ptr);

  bool Save(rex::stream::ByteStream* stream) override;
  bool Restore(rex::stream::ByteStream* stream) override;
  // Call while paused, before replacing the guest heap. Endpoint wrappers give
  // up their old guest allocation ownership; the incoming heap owns those bytes.
  bool PrepareRestore() override;

  bool is_paused() const { return clients_.paused(); }
  void Pause() override;
  bool Resume() override;

 protected:
  explicit AudioSystem(rex::runtime::FunctionDispatcher* function_dispatcher);

  virtual void Initialize();

  void WorkerThreadMain();

  virtual X_STATUS CreateDriver(size_t index, std::function<void()> consumed,
                                AudioDriver** out_driver) = 0;
  virtual void DestroyDriver(AudioDriver* driver) = 0;

  static constexpr size_t kMaximumQueuedFrames = 64;

  rex::memory::Memory* memory_ = nullptr;
  rex::runtime::FunctionDispatcher* function_dispatcher_ = nullptr;
  std::unique_ptr<XmaDecoder> xma_decoder_;
  uint32_t queued_frames_;

  rex::system::object_ref<rex::system::XHostThread> worker_thread_;
  rex::thread::global_critical_region global_critical_region_;
  gh::audio::ClientScheduler clients_;
  bool restore_failed_ = false;
  bool AttachClient(size_t index, ClientScheduler::Callback callback, AudioDriver* driver);

};

}  // namespace gh::audio
