/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2021 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

// This file contains some functions used to help parse XMA data.

#pragma once

#include <stdint.h>

#include "gh_xma_packet.h"

namespace rex::audio::xma {

static constexpr uint32_t kMaxFrameLength = gh::audio::xma::kMaxFrameLength;

// Get number of frames that /begin/ in this packet. Valid only for XMA2 packets.
inline uint8_t GetPacketFrameCount(const uint8_t* packet) {
  return gh::audio::xma::GetPacketFrameCount(packet);
}

// Get the first frame offset in bits
inline uint32_t GetPacketFrameOffset(const uint8_t* packet) {
  return gh::audio::xma::GetPacketFrameOffset(packet);
}

inline uint8_t GetPacketMetadata(const uint8_t* packet) {
  return gh::audio::xma::GetPacketMetadata(packet);
}

inline bool IsPacketXma2Type(const uint8_t* packet) {
  return gh::audio::xma::IsPacketXma2Type(packet);
}

inline uint8_t GetPacketSkipCount(const uint8_t* packet) {
  return gh::audio::xma::GetPacketSkipCount(packet);
}

}  // namespace rex::audio::xma
