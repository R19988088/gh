#include "gh_image_loader.h"
#include "crypto/TinySHA1.hpp"
#include "crypto/rijndael-alg-fst.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {
using namespace gh::image;
void Put32(std::vector<uint8_t>& bytes, size_t offset, uint32_t value) {
  for (unsigned i = 0; i < 4; ++i) bytes[offset + i] = uint8_t(value >> (24 - i * 8));
}
void Put16(std::vector<uint8_t>& bytes, size_t offset, uint16_t value) {
  bytes[offset] = uint8_t(value >> 8); bytes[offset + 1] = uint8_t(value);
}
std::array<uint8_t, 20> Hash(std::span<const uint8_t> input) {
  sha1::SHA1 hash;
  std::array<uint8_t, 20> digest;
  hash.processBytes(input.data(), input.size()); hash.finalize(digest.data()); return digest;
}
std::vector<uint8_t> Encrypt(const Key& key, std::span<const uint8_t> input) {
  assert(input.size() % 16 == 0);
  uint32_t schedule[4 * (MAXNR + 1)];
  const int rounds = rijndaelKeySetupEnc(schedule, key.data(), 128);
  std::vector<uint8_t> result(input.size());
  uint8_t previous[16]{};
  for (size_t offset = 0; offset < input.size(); offset += 16) {
    uint8_t mixed[16];
    for (unsigned i = 0; i < 16; ++i) mixed[i] = input[offset + i] ^ previous[i];
    rijndaelEncrypt(schedule, rounds, mixed, result.data() + offset);
    std::memcpy(previous, result.data() + offset, 16);
  }
  return result;
}
// A valid LZX uncompressed block, including the Intel transform flag and R0..2.
std::vector<uint8_t> Lzx(std::span<const uint8_t> input) {
  std::vector<bool> bits;
  auto append = [&](uint32_t value, unsigned count) {
    for (unsigned i = count; i; --i) bits.push_back((value >> (i - 1)) & 1);
  };
  append(0, 1); append(3, 3); append(uint32_t(input.size()) >> 8, 16);
  append(uint32_t(input.size()) & 255, 8);
  while (bits.size() % 16) bits.push_back(false);
  std::vector<uint8_t> encoded;
  for (size_t i = 0; i < bits.size(); i += 16) {
    uint16_t word = 0;
    for (unsigned j = 0; j < 16; ++j) word = uint16_t((word << 1) | bits[i + j]);
    encoded.push_back(uint8_t(word)); encoded.push_back(uint8_t(word >> 8));
  }
  for (unsigned i = 0; i < 3; ++i) encoded.insert(encoded.end(), {1, 0, 0, 0});
  encoded.insert(encoded.end(), input.begin(), input.end());
  if (encoded.size() & 1) encoded.push_back(0);
  return encoded;
}
void Record(std::vector<uint8_t>& records, uint32_t source, uint32_t target,
            uint16_t output, uint16_t input, std::span<const uint8_t> compressed = {}) {
  const size_t offset = records.size(); records.resize(offset + 12);
  Put32(records, offset, source); Put32(records, offset + 4, target);
  Put16(records, offset + 8, output); Put16(records, offset + 10, input);
  records.insert(records.end(), compressed.begin(), compressed.end());
}
std::vector<uint8_t> Header(uint32_t pages) {
  std::vector<uint8_t> bytes(0x300);
  Put32(bytes, 0, 0x58455832); Put32(bytes, 8, bytes.size()); Put32(bytes, 16, 0x40);
  Put32(bytes, 0x40 + 4, pages * 4096); Put32(bytes, 0x40 + 0x110, 0x82000000);
  Put32(bytes, 0x40 + 0x180, 1); Put32(bytes, 0x40 + 0x184, (pages << 4) | 1);
  return bytes;
}
void TestBinding() {
  ImportLibrary library;
  library.name = "xboxkrnl.exe";
  library.variable_slots = {{1, 0x100, 0}, {2, 0x104, 0}, {3, 0x108, 0},
                            {4, 0x10C, 0}, {5, 0x110, 0}, {1, 0x114, 0}};
  std::vector<std::pair<uint32_t, uint32_t>> written;
  std::vector<uint32_t> queried, logged;
  ImportBindingTarget target;
  target.resolve = [&](std::string_view name, uint16_t ordinal) {
    assert(name == library.name); queried.push_back(ordinal);
    if (ordinal == 1) return ResolvedImport{true, true, 1, 0x90000000, "value"};
    if (ordinal == 2) return ResolvedImport{true, false, 0xABC, 0, "stub"};
    if (ordinal == 3) return ResolvedImport{true, true, 3, 0, "zero"};
    if (ordinal == 4) return ResolvedImport{false, true, 4, 0, "function"};
    return ResolvedImport{};
  };
  target.write = [&](uint32_t address, uint32_t value) { written.emplace_back(address, value); };
  target.bound = [&](const ImportLibrary&, const ImportFunction& slot, const ResolvedImport&, bool) {
    logged.push_back(slot.ordinal);
  };
  BindImports(library, target);
  const std::vector<std::pair<uint32_t, uint32_t>> expected{
      {0x100, 0x90000000}, {0x104, 0xD000BEEF | (0xABC << 16)},
      {0x108, 0xD000BEEF | (3 << 16)}, {0x114, 0x90000000}};
  assert(written == expected && queried == std::vector<uint32_t>({1, 2, 3, 4, 5, 1}));
  assert(logged == std::vector<uint32_t>({1, 2, 3, 1}));
}
void TestRecords() {
  std::vector<uint8_t> image(256), expected;
  for (unsigned i = 0; i < image.size(); ++i) image[i] = uint8_t(i);
  expected = image;
  std::vector<uint8_t> records;
  Record(records, 0, 4, 16, 1);  // Overlapping old-to-new copy.
  Record(records, 0, 32, 16, 0);
  const std::array<uint8_t, 16> plain{9, 8, 7, 6, 5, 4, 3, 2, 1};
  const auto compressed = Lzx(plain);
  Record(records, 0, 64, plain.size(), compressed.size(), compressed);
  Record(records, 0, 0, 0, 0);
  std::memmove(expected.data() + 4, expected.data(), 16);
  std::memset(expected.data() + 32, 0, 16);
  std::copy(plain.begin(), plain.end(), expected.begin() + 64);
  assert(!ApplyDeltaRecords(records, 32768, image) && image == expected);
  records.clear(); Record(records, 0, 250, 16, 1);
  assert(ApplyDeltaRecords(records, 32768, image));
  records.clear(); Record(records, 0, 0, 16, compressed.size(), compressed);
  records.pop_back(); assert(ApplyDeltaRecords(records, 32768, image));
}
void TestPatch(uint32_t pages, bool encrypted, bool bad_hash = false, bool bad_key = false) {
  XexMetadata base, patch;
  auto base_bytes = Header(2), patched_header = Header(pages);
  patched_header[0x40 + 0x150] = 0xA5;
  assert(base.Load(base_bytes));
  const Key original_key = SessionKey(base.Security().key, false);
  const Key updated_key = SessionKey(
      std::span<const uint8_t, 16>(patched_header.data() + 0x40 + 0x150, 16), false);
  assert(updated_key != original_key);
  Key base_key = original_key, patch_key{};
  const auto header_patch = Lzx(patched_header);
  std::vector<uint8_t> patch_bytes(0x1000);
  Put32(patch_bytes, 0, 0x58455832); Put32(patch_bytes, 8, patch_bytes.size());
  Put32(patch_bytes, 16, 0x80); Put32(patch_bytes, 20, 2);
  Put32(patch_bytes, 24, 0x3FF); Put32(patch_bytes, 28, 0x240);
  Put32(patch_bytes, 32, 0x5FF); Put32(patch_bytes, 36, 0x280);
  constexpr size_t format = 0x240, descriptor = 0x280;
  Put32(patch_bytes, format, 36); Put16(patch_bytes, format + 4, encrypted);
  Put16(patch_bytes, format + 6, 2); Put32(patch_bytes, format + 8, 32768);
  Put32(patch_bytes, descriptor, 0x58 + header_patch.size());
  Put32(patch_bytes, descriptor + 4, 0x20000000); Put32(patch_bytes, descriptor + 8, 0x10000000);
  const auto signature = Hash(base.Security().signature);
  std::copy(signature.begin(), signature.end(), patch_bytes.begin() + descriptor + 12);
  auto key_source = Encrypt(updated_key, original_key);
  if (bad_key) key_source[0] ^= 1;
  std::copy(key_source.begin(), key_source.end(), patch_bytes.begin() + descriptor + 0x20);
  Put32(patch_bytes, descriptor + 0x30, patched_header.size());
  Put32(patch_bytes, descriptor + 0x40, 64); Put32(patch_bytes, descriptor + 0x44, 64);
  std::vector<uint8_t> header_records;
  Record(header_records, 0, 0, patched_header.size(), header_patch.size(), header_patch);
  std::copy(header_records.begin(), header_records.end(), patch_bytes.begin() + descriptor + 0x4C);

  std::vector<uint8_t> first(24), second(24);
  Record(first, 0, 128, 16, 1);
  Record(first, 0, 192, 16, 0);
  const std::array<uint8_t, 16> plain{0xF1, 0xE2, 0xD3, 0xC4};
  const auto compressed = Lzx(plain);
  Record(first, 0, 256, plain.size(), compressed.size(), compressed);
  Record(second, 128, 512, 16, 1);
  Record(second, 0, 0, 0, 0);
  Put32(first, 0, second.size());
  const auto second_hash = Hash(second);
  std::copy(second_hash.begin(), second_hash.end(), first.begin() + 4);
  const auto first_hash = Hash(first);
  Put32(patch_bytes, format + 12, first.size());
  std::copy(first_hash.begin(), first_hash.end(), patch_bytes.begin() + format + 16);
  if (bad_hash) first.back() ^= 1;
  std::vector<uint8_t> payload = first;
  payload.insert(payload.end(), second.begin(), second.end());
  assert(patch.Load(patch_bytes));
  Key expected_patch_key;
  assert(DecryptCbc(updated_key, patch.Security().key, expected_patch_key));
  if (encrypted) {
    payload.resize((payload.size() + 15) & ~size_t(15));
    payload = Encrypt(expected_patch_key, payload);
  }
  std::vector<uint8_t> image(8192), expected;
  for (size_t i = 0; i < image.size(); ++i) image[i] = uint8_t(i);
  expected = image; expected.resize(std::max(8192u, pages * 4096));
  if (!bad_key) {
    std::memmove(expected.data(), expected.data() + 64, 64);
    std::fill(expected.begin() + 64, expected.begin() + 8192, 0);
    if (!bad_hash) {
      std::copy_n(expected.begin(), 16, expected.begin() + 128);
      std::copy(plain.begin(), plain.end(), expected.begin() + 256);
      std::copy_n(expected.begin() + 128, 16, expected.begin() + 512);
    }
  }
  unsigned grew = 0, shrank = 0;
  DeltaPatchMemory memory;
  memory.grow = [&](uint32_t old_size, uint32_t new_size) {
    assert(old_size == 8192 && new_size == pages * 4096); ++grew; image.resize(new_size); return true;
  };
  memory.shrink = [&](uint32_t old_size, uint32_t new_size) {
    assert(old_size == 8192 && new_size == pages * 4096); ++shrank;
  };
  memory.image = [&](uint32_t size) { assert(size == image.size()); return std::span(image); };
  PatchVersions versions;
  const int result = ApplyDeltaPatch(patch, payload, base, false, 4096,
                                     base_key, patch_key, memory, versions);
  assert(result == (bad_key ? 7 : bad_hash ? 9 : 0));
  assert(base.HeaderBytes() == patched_header && base.ImageSize(4096) == pages * 4096);
  assert(base_key == updated_key && patch_key == expected_patch_key);
  assert(versions.source == 0x10000000 && versions.target == 0x20000000);
  assert(grew == unsigned(pages > 2));
  assert(shrank == unsigned(!bad_hash && !bad_key && pages < 2));
  assert(image == expected);
}
}  // namespace
int main() {
  TestBinding(); TestRecords();
  TestPatch(3, false); TestPatch(1, true); TestPatch(2, true, true); TestPatch(2, false, false, true);
  std::puts("XEX import binding, overlapping/zero/LZX delta records, encrypted block chains and image resize passed");
}
