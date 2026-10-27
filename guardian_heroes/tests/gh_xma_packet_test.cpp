#include "gh_xma_packet.h"

#include <array>
#include <cassert>
#include <cstdint>

namespace {

// Original ReXGlue helper formulas, retained as a differential oracle.
uint8_t ReXGetPacketFrameCount(const uint8_t* packet) { return packet[0] >> 2; }

uint32_t ReXGetPacketFrameOffset(const uint8_t* packet) {
  const uint32_t value = static_cast<uint16_t>(
      ((packet[0] & 0x3) << 13) | (packet[1] << 5) | (packet[2] >> 3));
  return value + 32;
}

uint8_t ReXGetPacketMetadata(const uint8_t* packet) { return packet[2] & 0x7; }

bool ReXIsPacketXma2Type(const uint8_t* packet) {
  return ReXGetPacketMetadata(packet) == 1;
}

uint8_t ReXGetPacketSkipCount(const uint8_t* packet) { return packet[3]; }

}  // namespace

int main() {
  assert(gh::audio::xma::kMaxFrameLength == 0x7FFF);

  // Every possible first-three-byte combination covers all frame-count,
  // frame-offset, and metadata bit patterns in the XMA header.
  for (uint32_t header = 0; header < 0x1000000; ++header) {
    const std::array<uint8_t, 4> packet = {
        static_cast<uint8_t>(header >> 16), static_cast<uint8_t>(header >> 8),
        static_cast<uint8_t>(header), 0};
    assert(gh::audio::xma::GetPacketFrameCount(packet.data()) ==
           ReXGetPacketFrameCount(packet.data()));
    assert(gh::audio::xma::GetPacketFrameOffset(packet.data()) ==
           ReXGetPacketFrameOffset(packet.data()));
    assert(gh::audio::xma::GetPacketMetadata(packet.data()) ==
           ReXGetPacketMetadata(packet.data()));
    assert(gh::audio::xma::IsPacketXma2Type(packet.data()) ==
           ReXIsPacketXma2Type(packet.data()));
  }

  for (uint32_t skip_count = 0; skip_count <= 0xFF; ++skip_count) {
    const std::array<uint8_t, 4> packet = {0, 0, 0, static_cast<uint8_t>(skip_count)};
    assert(gh::audio::xma::GetPacketSkipCount(packet.data()) ==
           ReXGetPacketSkipCount(packet.data()));
  }
}
