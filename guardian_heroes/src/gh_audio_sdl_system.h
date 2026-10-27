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

#include "gh_audio_system.h"

namespace gh::audio { using rex::X_STATUS; }

namespace gh::audio::sdl {

class SDLAudioSystem : public AudioSystem {
 public:
  explicit SDLAudioSystem(rex::runtime::FunctionDispatcher* function_dispatcher);
  ~SDLAudioSystem() override;

  static bool IsAvailable() { return true; }

  static std::unique_ptr<AudioSystem> Create(rex::runtime::FunctionDispatcher* function_dispatcher);

  X_STATUS CreateDriver(size_t index, std::function<void()> consumed,
                        AudioDriver** out_driver) override;
  void DestroyDriver(AudioDriver* driver) override;

 protected:
  void Initialize() override;
};

}  // namespace gh::audio::sdl
