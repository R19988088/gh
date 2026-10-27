#include "gh_image.h"
#include "crypto/TinySHA1.hpp"
#include "crypto/rijndael-alg-fst.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

static uint32_t Read32(const uint8_t* p) {
  return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
}
// Original loader's CBC loop and block traversal, retained as a differential
// oracle for the real encrypted NORMAL payload (not project parser helpers).
static void LegacyCbc(const uint8_t* key, const uint8_t* input, size_t size, uint8_t* output) {
  uint32_t schedule[4 * (MAXNR + 1)];
  const int rounds = rijndaelKeySetupDec(schedule, key, 128);
  uint8_t previous[16]{};
  for (size_t offset = 0; offset < size; offset += 16) {
    rijndaelDecrypt(schedule, rounds, input + offset, output + offset);
    for (size_t i = 0; i < 16; ++i) {
      output[offset + i] ^= previous[i];
      previous[i] = input[offset + i];
    }
  }
}
static int LegacyNormal(std::span<const uint8_t> payload, const uint8_t* format,
                        const uint8_t* key, uint32_t size, std::vector<uint8_t>& image) {
  std::vector<uint8_t> decrypted(payload.size());
  LegacyCbc(key, payload.data(), payload.size(), decrypted.data());
  std::vector<uint8_t> compressed(payload.size());
  auto* p = decrypted.data();
  auto* d = compressed.data();
  auto* block_info = format + 12;
  while (const uint32_t block_size = Read32(block_info)) {
    uint8_t digest[20];
    sha1::SHA1 hash;
    hash.processBytes(p, block_size);
    hash.finalize(digest);
    if (std::memcmp(digest, block_info + 4, 20)) return 2;
    auto* next = p + block_size;
    block_info = p;
    p += 24;
    for (;;) {
      const size_t length = (p[0] << 8) | p[1];
      p += 2;
      if (!length) break;
      std::memcpy(d, p, length);
      p += length;
      d += length;
    }
    p = next;
  }
  image.assign(size, 0);
  return gh::image::DecompressLzx({compressed.data(), size_t(d - compressed.data())},
                                 image, Read32(format + 8));
}

int main(int argc, char** argv) {
  assert(argc == 2);
  const gh::image::Key zero{};
  // NIST AES-128 CBC vector, plus a non-aligned payload rejection.
  const gh::image::Key aes_key{0x2b,0x7e,0x15,0x16,0x28,0xae,0xd2,0xa6,
                              0xab,0xf7,0x15,0x88,0x09,0xcf,0x4f,0x3c};
  const std::array<uint8_t,16> cipher{0x3a,0xd7,0x7b,0xb4,0x0d,0x7a,0x36,0x60,
                                     0xa8,0x9e,0xca,0xf3,0x24,0x66,0xef,0x97};
  const std::array<uint8_t,16> plain{0x6b,0xc1,0xbe,0xe2,0x2e,0x40,0x9f,0x96,
                                    0xe9,0x3d,0x7e,0x11,0x73,0x93,0x17,0x2a};
  std::array<uint8_t,16> output{};
  assert(gh::image::DecryptCbc(aes_key, cipher, output) && output == plain);
  assert(!gh::image::DecryptCbc(zero, std::span(cipher).first(15), output));
  const std::array<uint8_t,8> none{0,0,0,8,0,0,0,0};
  std::vector<uint8_t> decoded;
  assert(!gh::image::DecodePayload(plain, none, zero, 0, decoded));
  assert(std::equal(decoded.begin(), decoded.end(), plain.begin()));
  const std::array<uint8_t,24> basic{0,0,0,24,0,1,0,1,
                                    0,0,0,16,0,0,0,7,0,0,0,16,0,0,0,9};
  std::array<uint8_t,32> encrypted{}, expected{};
  std::copy(cipher.begin(), cipher.end(), encrypted.begin());
  std::copy(cipher.begin(), cipher.end(), encrypted.begin()+16);
  LegacyCbc(aes_key.data(), encrypted.data(), encrypted.size(), expected.data());
  assert(!gh::image::DecodePayload(encrypted, basic, aes_key, 64, decoded));
  assert(std::equal(expected.begin(), expected.begin()+16, decoded.begin()));
  assert(std::equal(expected.begin()+16, expected.end(), decoded.begin()+23));
  assert(std::all_of(decoded.begin()+16, decoded.begin()+23, [](auto b){return !b;}));
  assert(gh::image::DecodePayload(encrypted, basic, aes_key, 47, decoded));
  assert(gh::image::DecodePayload(std::span(encrypted).first(16), basic, aes_key, 64, decoded));

  std::ifstream file(argv[1], std::ios::binary);
  std::vector<uint8_t> xex((std::istreambuf_iterator<char>(file)), {});
  assert(xex.size() > 24);
  const uint32_t security = Read32(xex.data()+16);
  const uint32_t size = Read32(xex.data()+security+4);
  const uint32_t count = Read32(xex.data()+20);
  uint32_t format_offset = 0;
  for (uint32_t i=0;i<count;++i) {
    if (Read32(xex.data()+24+i*8)==0x3FF) format_offset=Read32(xex.data()+28+i*8);
  }
  assert(format_offset);
  const auto format = std::span(xex).subspan(format_offset, Read32(xex.data()+format_offset));
  const auto payload = std::span(xex).subspan(Read32(xex.data()+8));
  bool loaded = false;
  for (bool devkit : {false, true}) {
    const auto key = gh::image::SessionKey(
        std::span<const uint8_t,16>(xex.data()+security+0x150,16), devkit);
    std::vector<uint8_t> legacy;
    const int before = LegacyNormal(payload, format.data(), key.data(), size, legacy);
    const int after = gh::image::DecodePayload(payload, format, key, size, decoded);
    assert(before == after);
    if (!after) {
      assert(decoded == legacy);
      assert(decoded[0]=='M' && decoded[1]=='Z');
      loaded = true;
      std::printf("default.xex %s key: %zu bytes match legacy payload\n",
                  devkit ? "devkit" : "retail", decoded.size());
    }
  }
  assert(loaded);
  assert(gh::image::DecodePayload(payload.first(16), format, zero, size, decoded));
  assert(gh::image::DecompressLzx({}, output, 0));
  std::puts("XEX payload decoding passed");
}
