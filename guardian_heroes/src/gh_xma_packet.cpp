#include "gh_xma_packet.h"

namespace gh::audio::xma {

uint8_t GetPacketFrameCount(const uint8_t* packet) noexcept { return packet[0] >> 2; }

uint32_t GetPacketFrameOffset(const uint8_t* packet) noexcept {
  const uint32_t value = static_cast<uint16_t>(
      ((packet[0] & 0x3) << 13) | (packet[1] << 5) | (packet[2] >> 3));
  return value + 32;
}

uint8_t GetPacketMetadata(const uint8_t* packet) noexcept { return packet[2] & 0x7; }

bool IsPacketXma2Type(const uint8_t* packet) noexcept {
  return GetPacketMetadata(packet) == 1;
}

uint8_t GetPacketSkipCount(const uint8_t* packet) noexcept { return packet[3]; }

}  // namespace gh::audio::xma
