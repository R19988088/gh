/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.
 * Released under the BSD license in the root LICENSE file.
 */

#pragma once

#include <cstdint>

namespace rex::graphics {

// The fields shared by every PM4 packet header. Keeping this decode independent
// from CommandProcessor makes the wire format directly testable while leaving
// packet execution and validation in the processor.
enum class PM4PacketType : uint32_t {
  kType0 = 0,
  kType1 = 1,
  kType2 = 2,
  kType3 = 3,
};

struct PM4PacketHeader {
  PM4PacketType type;
  uint32_t count;
  uint32_t opcode;
  bool predicate;
};

constexpr PM4PacketHeader DecodePM4PacketHeader(uint32_t packet) {
  return {
      static_cast<PM4PacketType>(packet >> 30),
      ((packet >> 16) & 0x3FFFu) + 1u,
      (packet >> 8) & 0x7Fu,
      (packet & 1u) != 0,
  };
}

}  // namespace rex::graphics
