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
#include <span>
#include <vector>

#include <rex/kernel.h>
#include <rex/memory.h>

namespace gh::audio { using rex::X_STATUS; }

namespace gh::audio {

class AudioDriver {
 public:
  explicit AudioDriver(rex::memory::Memory* memory);
  virtual ~AudioDriver();

  virtual void SubmitFrame(uint32_t samples_ptr) = 0;
  virtual bool Pause() { return false; }
  virtual bool Resume() { return false; }
  virtual bool SaveState(std::vector<uint8_t>&) { return false; }
  virtual bool RestoreState(std::span<const uint8_t>) { return false; }

 protected:
  inline uint8_t* TranslatePhysical(uint32_t guest_address) const {
    return memory_->TranslatePhysical(guest_address);
  }

  rex::memory::Memory* memory_ = nullptr;
};

}  // namespace gh::audio
