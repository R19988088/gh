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

#include "gh_audio_driver.h"

namespace gh::audio {

AudioDriver::AudioDriver(rex::memory::Memory* memory) : memory_(memory) {}

AudioDriver::~AudioDriver() = default;

}  // namespace gh::audio
