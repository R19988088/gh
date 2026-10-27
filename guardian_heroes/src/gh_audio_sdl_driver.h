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

#pragma once
#include <functional>

#include <memory>
#include "gh_audio_driver.h"
#include <rex/thread.h>

namespace gh::audio { using rex::X_STATUS; }

namespace gh::audio { class SdlOutput; }

namespace gh::audio::sdl {

class SDLAudioDriver : public AudioDriver {
 public:
  SDLAudioDriver(rex::memory::Memory* memory, std::function<void()> consumed);
  ~SDLAudioDriver() override;
  bool Initialize(bool start_paused = false);
  void SubmitFrame(uint32_t frame_ptr) override;
  void Shutdown();
  bool Pause() override;
  bool Resume() override;
  bool SaveState(std::vector<uint8_t>&) override;
  bool RestoreState(std::span<const uint8_t>) override;

 private:
  std::unique_ptr<gh::audio::SdlOutput> output_;
  uint32_t diagnostic_submit_count_ = 0;
};

}  // namespace gh::audio::sdl
