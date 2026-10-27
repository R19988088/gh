#pragma once

#include <cstdint>

namespace gh::audio::xma {

inline constexpr uint32_t kMaxFrameLength = 0x7FFF;

// XMA packets begin with a four-byte, byte-oriented header. These helpers only
// decode those header fields; XmaContext owns decoder and traversal state.
uint8_t GetPacketFrameCount(const uint8_t* packet) noexcept;
uint32_t GetPacketFrameOffset(const uint8_t* packet) noexcept;
uint8_t GetPacketMetadata(const uint8_t* packet) noexcept;
bool IsPacketXma2Type(const uint8_t* packet) noexcept;
uint8_t GetPacketSkipCount(const uint8_t* packet) noexcept;

}  // namespace gh::audio::xma
