// XEX payload decoding derived from Xenia, Copyright 2021 Ben Vanik.
// Released under the BSD license; see rexglue-sdk-main/LICENSE.
#include "gh_image.h"

#include <algorithm>
#include <bit>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <limits>

#include "crypto/TinySHA1.hpp"
#include "crypto/rijndael-alg-fst.c"
#include <mspack.h>
#include <lzx.h>

namespace gh::image {
namespace {
uint32_t Read32(const uint8_t* p) {
  return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
}
uint16_t Read16(const uint8_t* p) { return uint16_t(p[0]) << 8 | p[1]; }

class CbcDecoder {
 public:
  explicit CbcDecoder(std::span<const uint8_t, 16> key)
      : rounds_(rijndaelKeySetupDec(schedule_, key.data(), 128)) {}
  bool Decode(std::span<const uint8_t> input, std::span<uint8_t> output) {
    if (input.size() % 16 || output.size() < input.size()) return false;
    for (size_t offset = 0; offset < input.size(); offset += 16) {
      uint8_t ciphertext[16];
      std::memcpy(ciphertext, input.data() + offset, 16);
      rijndaelDecrypt(schedule_, rounds_, ciphertext, output.data() + offset);
      for (size_t i = 0; i < 16; ++i) output[offset + i] ^= previous_[i];
      std::memcpy(previous_, ciphertext, 16);
    }
    return true;
  }
 private:
  uint32_t schedule_[4 * (MAXNR + 1)];
  uint8_t previous_[16]{};
  int rounds_;
};

struct MemoryFile {
  uint8_t* data;
  size_t size;
  size_t offset = 0;
};
int Read(mspack_file* file, void* buffer, int count) {
  auto& f = *reinterpret_cast<MemoryFile*>(file);
  if (count < 0) return -1;
  size_t length = std::min(size_t(count), f.size - f.offset);
  if (length) std::memcpy(buffer, f.data + f.offset, length);
  f.offset += length;
  return static_cast<int>(length);
}
int Write(mspack_file* file, void* buffer, int count) {
  auto& f = *reinterpret_cast<MemoryFile*>(file);
  if (count < 0 || size_t(count) > f.size - f.offset) return -1;
  if (count) std::memcpy(f.data + f.offset, buffer, size_t(count));
  f.offset += size_t(count);
  return count;
}
void* Allocate(mspack_system*, size_t count) { return std::calloc(1, count); }
void Free(void* data) { std::free(data); }
void Copy(void* source, void* destination, size_t count) {
  std::memcpy(destination, source, count);
}
}  // namespace

Key SessionKey(std::span<const uint8_t, 16> encrypted_key, bool devkit) {
  constexpr Key retail{0x20, 0xB1, 0x85, 0xA5, 0x9D, 0x28, 0xFD, 0xC3,
                       0x40, 0x58, 0x3F, 0xBB, 0x08, 0x96, 0xBF, 0x91};
  constexpr Key development{};
  Key result;
  DecryptCbc(devkit ? development : retail, encrypted_key, result);
  return result;
}

bool DecryptCbc(std::span<const uint8_t, 16> key, std::span<const uint8_t> input,
                std::span<uint8_t> output) {
  return CbcDecoder(key).Decode(input, output);
}

int DecompressLzx(std::span<const uint8_t> input, std::span<uint8_t> output,
                  uint32_t window_size, std::span<const uint8_t> reference) {
  if (!std::has_single_bit(window_size) || input.size() >= INT_MAX ||
      output.size() >= INT_MAX || reference.size() > window_size) return 1;
  mspack_system system{};
  system.read = Read;
  system.write = Write;
  system.alloc = Allocate;
  system.free = Free;
  system.copy = Copy;
  MemoryFile source{const_cast<uint8_t*>(input.data()), input.size()};
  MemoryFile destination{output.data(), output.size()};
  auto* stream = lzxd_init(&system, reinterpret_cast<mspack_file*>(&source),
                           reinterpret_cast<mspack_file*>(&destination),
                           std::countr_zero(window_size), 0, 0x8000,
                           static_cast<off_t>(output.size()), 0);
  if (!stream) return 1;
  if (reference.data()) {
    const size_t padding = window_size - reference.size();
    std::memset(stream->window, 0, padding);
    std::memcpy(stream->window + padding, reference.data(), reference.size());
    stream->ref_data_size = window_size;
  }
  const int result = lzxd_decompress(stream, static_cast<off_t>(output.size()));
  lzxd_free(stream);
  return result ? result : (destination.offset == output.size() ? 0 : 1);
}

int DecodePayload(std::span<const uint8_t> payload, std::span<const uint8_t> file_format,
                   std::span<const uint8_t, 16> session_key, uint32_t allocation_size,
                   std::vector<uint8_t>& image) {
  if (file_format.size() < 8 || Read32(file_format.data()) != file_format.size() ||
      payload.size() > std::numeric_limits<uint32_t>::max()) return 1;
  const uint16_t encryption = Read16(file_format.data() + 4);
  const uint16_t compression = Read16(file_format.data() + 6);
  if (encryption > 1) return 1;
  CbcDecoder cipher(session_key);
  if (compression == 0) {
    image.assign(payload.size(), 0);
    if (!encryption) std::copy(payload.begin(), payload.end(), image.begin());
    else if (!cipher.Decode(payload, image)) return 1;
    return 0;
  }
  if (compression == 1) {
    if ((file_format.size() - 8) % 8) return 1;
    size_t source_size = 0, destination_size = 0;
    for (size_t i = 8; i < file_format.size(); i += 8) {
      const uint32_t data_size = Read32(file_format.data() + i);
      const uint32_t zero_size = Read32(file_format.data() + i + 4);
      if (source_size > payload.size() || data_size > payload.size() - source_size ||
          destination_size > allocation_size ||
          uint64_t(data_size) + zero_size > allocation_size - destination_size ||
          (encryption && data_size % 16)) return 1;
      source_size += data_size;
      destination_size += uint64_t(data_size) + zero_size;
    }
    image.assign(allocation_size, 0);
    size_t source = 0, destination = 0;
    for (size_t i = 8; i < file_format.size(); i += 8) {
      const uint32_t data_size = Read32(file_format.data() + i);
      const uint32_t zero_size = Read32(file_format.data() + i + 4);
      auto input = payload.subspan(source, data_size);
      auto output = std::span(image).subspan(destination, data_size);
      if (encryption) cipher.Decode(input, output);
      else std::copy(input.begin(), input.end(), output.begin());
      source += data_size;
      destination += size_t(data_size) + zero_size;
    }
    return 0;
  }
  if (compression != 2 || file_format.size() < 36) return 2;
  const uint32_t window = Read32(file_format.data() + 8);
  std::vector<uint8_t> plaintext;
  if (encryption) {
    plaintext.resize(payload.size());
    if (!cipher.Decode(payload, plaintext)) return 1;
    payload = plaintext;
  }
  uint32_t block_size = Read32(file_format.data() + 12);
  std::array<uint8_t, 20> expected_hash;
  std::copy_n(file_format.data() + 16, 20, expected_hash.begin());
  std::vector<uint8_t> compressed;
  compressed.reserve(payload.size());
  size_t offset = 0;
  while (block_size) {
    if (block_size < 24 || offset > payload.size() || block_size > payload.size() - offset)
      return 1;
    const auto block = payload.subspan(offset, block_size);
    sha1::SHA1 hash;
    uint8_t digest[20];
    hash.processBytes(block.data(), block.size());
    hash.finalize(digest);
    if (!std::equal(expected_hash.begin(), expected_hash.end(), digest)) return 2;
    size_t chunk = 24;
    for (;;) {
      if (block.size() - chunk < 2) return 1;
      const uint16_t length = Read16(block.data() + chunk);
      chunk += 2;
      if (!length) break;
      if (length > block.size() - chunk) return 1;
      compressed.insert(compressed.end(), block.begin() + chunk, block.begin() + chunk + length);
      chunk += length;
    }
    block_size = Read32(block.data());
    std::copy_n(block.data() + 4, 20, expected_hash.begin());
    offset += block.size();
  }
  image.assign(allocation_size, 0);
  return DecompressLzx(compressed, image, window);
}
}  // namespace gh::image
