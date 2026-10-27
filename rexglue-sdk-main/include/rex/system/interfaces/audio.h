/**
 * @file        system/interfaces/audio.h
 * @brief       Abstract audio system interface for dependency injection
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#pragma once

#include <rex/system/xtypes.h>

namespace rex::system {
class KernelState;
}
namespace rex::stream {
class ByteStream;
}
namespace rex::system {
}

namespace rex::system {

class IAudioSystem {
 public:
  virtual ~IAudioSystem() = default;
  virtual X_STATUS Setup(KernelState* kernel_state) = 0;
  virtual void Shutdown() = 0;
  virtual void Pause() {}
  virtual void Resume() {}
  virtual bool Save(rex::stream::ByteStream* stream) {
    (void)stream;
    return true;
  }
  virtual bool Restore(rex::stream::ByteStream* stream) {
    (void)stream;
    return true;
  }
};

}  // namespace rex::system
