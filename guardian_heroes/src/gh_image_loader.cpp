// XEX binding and delta patch execution derived from Xenia/ReXGlue (BSD license).
#include "gh_image_loader.h"

#include "crypto/TinySHA1.hpp"

#include <algorithm>
#include <cstring>
#include <format>
#include <limits>

namespace gh::image {
namespace {
uint32_t Be32(const uint8_t* p) {
  return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
}
uint16_t Be16(const uint8_t* p) { return uint16_t(p[0]) << 8 | p[1]; }
bool Contains(std::span<const uint8_t> bytes, size_t offset, size_t count) {
  return offset <= bytes.size() && count <= bytes.size() - offset;
}
std::array<uint8_t, 20> Digest(std::span<const uint8_t> bytes) {
  sha1::SHA1 hash;
  std::array<uint8_t, 20> digest;
  hash.processBytes(bytes.data(), bytes.size());
  hash.finalize(digest.data());
  return digest;
}
}  // namespace

void BindImports(const ImportLibrary& library, const ImportBindingTarget& target) {
  if (!target.resolve) return;
  for (const auto& slot : library.variable_slots) {
    const auto resolved = target.resolve(library.name, uint16_t(slot.ordinal));
    if (!resolved.variable) continue;
    const bool implemented = resolved.implemented && resolved.address;
    const uint32_t value = implemented ? resolved.address
        : 0xD000BEEF | ((resolved.ordinal & 0xFFF) << 16);
    target.write(slot.value_address, value);
    if (target.bound) target.bound(library, slot, resolved, implemented);
  }
}

int ApplyDeltaRecords(std::span<const uint8_t> records, uint32_t window_size,
                       std::span<uint8_t> image) {
  size_t offset = 0;
  while (offset < records.size()) {
    // A descriptor may include zero padding after its final 12-byte record.
    if (records.size() - offset < 12) {
      return std::all_of(records.begin() + offset, records.end(), [](auto b) { return !b; }) ? 0 : 1;
    }
    const auto* record = records.data() + offset;
    const uint32_t source = Be32(record), target = Be32(record + 4);
    const uint16_t output_size = Be16(record + 8), input_size = Be16(record + 10);
    if (!(source | target | output_size | input_size)) break;
    if (!Contains(image, target, output_size)) return 1;
    if (!input_size) std::memset(image.data() + target, 0, output_size);
    else {
      if (!Contains(image, source, output_size)) return 1;
      if (input_size == 1) {
        std::memmove(image.data() + target, image.data() + source, output_size);
      } else {
        if (!Contains(records, offset + 12, input_size)) return 1;
        const int result = DecompressLzx(records.subspan(offset + 12, input_size),
            image.subspan(target, output_size), window_size, image.subspan(source, output_size));
        if (result) return result;
      }
    }
    offset += 12 + (input_size > 1 ? input_size : 0);
  }
  return 0;
}

int ApplyDeltaPatch(const XexMetadata& patch, std::span<const uint8_t> payload,
                    XexMetadata& base, bool devkit, uint32_t page_size,
                    Key& base_session_key, Key& patch_session_key,
                    const DeltaPatchMemory& memory, PatchVersions& versions) {
  const auto log = [&](std::string_view text, bool warning = false) {
    if (memory.log) memory.log(warning, text);
  };
  OptionalHeader descriptor, format_header;
  if (!FindOptionalHeader(patch.HeaderBytes(), 0x5FF, descriptor) || descriptor.size < 0x58 ||
      !FindOptionalHeader(patch.HeaderBytes(), 0x3FF, format_header) || format_header.size < 36)
    return 1;
  const auto data = std::span(patch.HeaderBytes()).subspan(descriptor.offset, descriptor.size);
  const auto format = std::span(patch.HeaderBytes()).subspan(format_header.offset, format_header.size);
  const auto* p = data.data();
  versions = {Be32(p + 8), Be32(p + 4)};
  const auto signature = Digest(base.Security().signature);
  if (!std::equal(signature.begin(), signature.end(), p + 12))
    log("XEX patch signature hash doesn't match base XEX signature hash, patch will likely fail!", true);

  const uint32_t source_offset = Be32(p + 0x34), source_size = Be32(p + 0x38);
  const uint32_t target_offset = Be32(p + 0x3C), declared_target_size = Be32(p + 0x30);
  const auto& original_header = base.HeaderBytes();
  const uint32_t original_header_size = Be32(original_header.data() + 8);
  if (source_offset > original_header_size) {
    log("XEX header patch source is outside base XEX header area"); return 2;
  }
  if (source_size > original_header_size - source_offset) {
    log("XEX header patch source is too large"); return 3;
  }
  if (target_offset > declared_target_size) {
    log("XEX header patch target is outside base XEX header area"); return 4;
  }
  if (source_size > declared_target_size - target_offset) return 5;
  const uint32_t old_image_size = base.ImageSize(page_size);
  const uint32_t header_size = declared_target_size ? declared_target_size : target_offset + source_size;
  auto& header = base.HeaderBytes();
  if (!Contains(header, source_offset, source_size)) return 3;
  if (header_size > header.size()) header.resize(header_size);
  if (source_offset) std::memmove(header.data() + target_offset, header.data() + source_offset, source_size);
  if (header_size < header.size()) std::memset(header.data() + header_size, 0, header.size() - header_size);
  const uint32_t window = Be32(format.data() + 8);
  const size_t header_patch_size = size_t(Be16(p + 0x56)) + 12;
  if (!Contains(data, 0x4C, header_patch_size)) return 1;
  int result = ApplyDeltaRecords(data.subspan(0x4C, header_patch_size), window, header);
  if (result) {
    log(std::format("XEX header patch application failed, error code {}", result)); return result;
  }
  if (header.size() > header_size) header.resize(header_size);
  if (!base.Refresh()) return 1;
  const uint32_t new_image_size = base.ImageSize(page_size);
  if (new_image_size > old_image_size && !memory.grow(old_image_size, new_image_size)) return 6;

  const Key original_key = base_session_key;
  base_session_key = SessionKey(base.Security().key, devkit);
  DecryptCbc(base_session_key, patch.Security().key, patch_session_key);
  Key test_key;
  DecryptCbc(base_session_key, data.subspan(0x20, 16), test_key);
  if (test_key != original_key) {
    log("XEX patch image key doesn't match original XEX!"); return 7;
  }

  std::vector<uint8_t> decrypted;
  const uint16_t encryption = Be16(format.data() + 4);
  if (encryption == 1) {
    decrypted.resize(payload.size());
    if (!DecryptCbc(patch_session_key, payload, decrypted)) return 8;
    payload = decrypted;
  } else if (encryption) return 8;
  auto image = memory.image(std::max(old_image_size, new_image_size));
  const uint32_t image_source = Be32(p + 0x40), image_size = Be32(p + 0x44);
  const uint32_t image_target = Be32(p + 0x48);
  if (image_source && (!Contains(image, image_source, image_size) ||
                       !Contains(image, image_target, image_size))) return 1;
  if (image_source) std::memmove(image.data() + image_target, image.data() + image_source, image_size);
  const uint64_t image_target_size = uint64_t(image_target) + image_size;
  if (image_target_size < old_image_size)
    std::memset(image.data() + image_target_size, 0, old_image_size - image_target_size);

  uint32_t block_size = Be32(format.data() + 12);
  std::array<uint8_t, 20> expected_hash;
  std::copy_n(format.data() + 16, 20, expected_hash.begin());
  size_t offset = 0;
  while (block_size) {
    if (block_size < 24 || !Contains(payload, offset, block_size)) { result = 1; break; }
    const auto block = payload.subspan(offset, block_size);
    if (Digest(block) != expected_hash) {
      result = 9;
      log("XEX patch block hash doesn't match hash inside block info!"); break;
    }
    result = ApplyDeltaRecords(block.subspan(24), window, image);
    if (result) break;
    block_size = Be32(block.data());
    std::copy_n(block.data() + 4, 20, expected_hash.begin());
    offset += block.size();
  }
  if (!result) {
    if (old_image_size > new_image_size) memory.shrink(old_image_size, new_image_size);
  } else log(std::format("XEX patch application failed, error code {}", result));
  return result;
}
}  // namespace gh::image
