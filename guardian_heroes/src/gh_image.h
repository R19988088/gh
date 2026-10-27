#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace gh::image {
using Key = std::array<uint8_t, 16>;

Key SessionKey(std::span<const uint8_t, 16> encrypted_key, bool devkit);
bool DecryptCbc(std::span<const uint8_t, 16> key, std::span<const uint8_t> input,
                std::span<uint8_t> output);
int DecompressLzx(std::span<const uint8_t> input, std::span<uint8_t> output,
                  uint32_t window_size, std::span<const uint8_t> reference = {});

// file_format is the complete big-endian XEX file-format optional header.
// BASIC allocation_size includes the guest heap page padding; NORMAL uses the
// security header image size. NONE uses the payload length.
int DecodePayload(std::span<const uint8_t> payload, std::span<const uint8_t> file_format,
                   std::span<const uint8_t, 16> session_key, uint32_t allocation_size,
                   std::vector<uint8_t>& image);
}  // namespace gh::image
