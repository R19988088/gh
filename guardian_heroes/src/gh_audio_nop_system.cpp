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

#include <rex/audio/flags.h>
#include "gh_audio_nop_system.h"

namespace gh::audio::nop {

std::unique_ptr<AudioSystem> NopAudioSystem::Create(
    rex::runtime::FunctionDispatcher* function_dispatcher) {
  return std::make_unique<NopAudioSystem>(function_dispatcher);
}

NopAudioSystem::NopAudioSystem(rex::runtime::FunctionDispatcher* function_dispatcher)
    : AudioSystem(function_dispatcher) {}

NopAudioSystem::~NopAudioSystem() { Shutdown(); }

X_STATUS NopAudioSystem::CreateDriver(size_t /*index*/, std::function<void()> /*consumed*/,
                                      AudioDriver** /*out_driver*/) {
  return X_STATUS_NOT_IMPLEMENTED;
}

void NopAudioSystem::DestroyDriver(AudioDriver* driver) {
  (void)driver;
  assert_always();
}

}  // namespace gh::audio::nop
