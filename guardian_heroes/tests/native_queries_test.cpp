#include "native_helpers_test.h"

#include <array>

REX_EXTERN(sub_82496E58);
REX_EXTERN(__imp__sub_82496E58);
REX_EXTERN(sub_82498EC8);
REX_EXTERN(__imp__sub_82498EC8);
REX_EXTERN(sub_824A5938);
REX_EXTERN(__imp__sub_824A5938);
REX_EXTERN(sub_82498EF0);
REX_EXTERN(__imp__sub_82498EF0);
REX_EXTERN(sub_82498F18);
REX_EXTERN(__imp__sub_82498F18);
REX_EXTERN(sub_825D1170);
REX_EXTERN(__imp__sub_825D1170);
REX_EXTERN(sub_82495D38);
REX_EXTERN(__imp__sub_82495D38);
REX_EXTERN(sub_826FA128);
REX_EXTERN(__imp__sub_826FA128);
REX_EXTERN(sub_82643B70);
REX_EXTERN(__imp__sub_82643B70);
REX_EXTERN(sub_824279F0);
REX_EXTERN(__imp__sub_824279F0);
REX_EXTERN(sub_82434978);
REX_EXTERN(__imp__sub_82434978);
REX_EXTERN(sub_824348C8);
REX_EXTERN(__imp__sub_824348C8);
REX_EXTERN(sub_82425CE0);
REX_EXTERN(__imp__sub_82425CE0);
REX_EXTERN(sub_82803538);
REX_EXTERN(__imp__sub_82803538);
REX_EXTERN(sub_824A5880);
REX_EXTERN(__imp__sub_824A5880);
REX_EXTERN(sub_824987A0);
REX_EXTERN(__imp__sub_824987A0);
REX_EXTERN(sub_82499E40);
REX_EXTERN(__imp__sub_82499E40);
REX_EXTERN(sub_82421B40);
REX_EXTERN(__imp__sub_82421B40);
REX_EXTERN(sub_827667E0);
REX_EXTERN(__imp__sub_827667E0);
REX_EXTERN(sub_82499CD8);
REX_EXTERN(__imp__sub_82499CD8);
REX_EXTERN(sub_82434BA0);
REX_EXTERN(__imp__sub_82434BA0);
REX_EXTERN(sub_8242ED00);
REX_EXTERN(__imp__sub_8242ED00);
REX_EXTERN(sub_82803460);
REX_EXTERN(__imp__sub_82803460);
REX_EXTERN(sub_82499D00);
REX_EXTERN(__imp__sub_82499D00);
REX_EXTERN(sub_82496990);
REX_EXTERN(__imp__sub_82496990);

namespace {
uint32_t Next(uint32_t& state) {
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return state;
}
struct ResourceQuery {
  const char* name;
  GuestFunction* original;
  GuestFunction* native;
  uint32_t offset;
};
}  // namespace

int main() {
  NativeHelpersTest test;
  constexpr uint32_t object = NativeHelpersTest::kObject;
  constexpr uint32_t data = NativeHelpersTest::kData;
  constexpr uint64_t high = 0xD531A79600000000ull;
  uint32_t seed = 0;
  uint32_t random = 0x7B16D4E3;
  const ResourceQuery resources[] = {
      {"resource+32", __imp__sub_82498EC8, sub_82498EC8, 32},
      {"resource+64", __imp__sub_82498EF0, sub_82498EF0, 64},
      {"resource+4736", __imp__sub_82498F18, sub_82498F18, 4736},
      {"resource+3168", __imp__sub_82499E40, sub_82499E40, 3168},
      {"resource+2880", __imp__sub_82499CD8, sub_82499CD8, 2880},
      {"resource+2912", __imp__sub_82499D00, sub_82499D00, 2912},
  };
  for (const auto& query : resources) {
    for (unsigned i = 0; i < 80; ++i) {
      auto ctx = test.Context(seed++);
      ctx.r3.u64 = high | object;
      test.Fill(object, 64, seed);
      test.Fill(data, 8192, seed);
      const uint32_t handle = i % 4 == 0 ? 0 : object + 16;
      const uint32_t resource = i % 3 == 0 ? object : data;
      test.Store32(object, handle);
      test.Store32(object + 16, resource);
      test.Store32(resource + query.offset, i % 2 ? Next(random) : 0xFFFFFFFF);
      test.Check(query.name, query.original, query.native, ctx,
                 {{object, 8192}, {data, 8192}});
    }
  }

  for (uint32_t value = 0; value < 256; ++value) {
    for (uint32_t blocked : {0u, 1u, 255u}) {
      auto ctx = test.Context(seed++);
      ctx.r3.u64 = high | object;
      test.Fill(object, 64, seed);
      test.Store8(object + 31, value);
      test.Store8(0x829CC39C, blocked);
      test.Check("gated flag bit", __imp__sub_82496E58, sub_82496E58, ctx,
                 {{object, 64}, {0x829CC39C, 1}});
      ctx = test.Context(seed++);
      ctx.r3.u64 = high | object;
      test.Store8(object, value);
      test.Store8(0x829CC394, blocked);
      test.Check("mode-gated signed byte", __imp__sub_82495D38, sub_82495D38, ctx,
                 {{object, 64}, {0x829CC394, 1}});
    }
  }

  for (uint16_t state : {uint16_t(0), uint16_t(1), uint16_t(2), uint16_t(3),
                         uint16_t(4), uint16_t(0x7FFF), uint16_t(0x8000), uint16_t(0xFFFF)}) {
    auto ctx = test.Context(seed++);
    ctx.r3.u64 = high | object;
    test.Fill(object, 16, seed);
    test.Store16(object + 4, state);
    test.Check("state in 1..3", __imp__sub_824A5938, sub_824A5938, ctx, {{object, 16}});
  }

  for (uint32_t blocked : {0u, 1u, 0x80000000u, 0xFFFFFFFFu}) {
    for (uint32_t result : {0u, 1u, 0xFFFFFFFFu}) {
      auto ctx = test.Context(seed++);
      ctx.r13.u64 = high | object;
      test.Fill(object, 512, seed);
      test.Fill(data, 512, seed);
      test.Store32(object + 336, blocked);
      test.Store32(object + 256, data);
      test.Store32(data + 352, result);
      test.Check("thread result", __imp__sub_825D1170, sub_825D1170, ctx,
                 {{object, 512}, {data, 512}});
    }
  }

  for (uint32_t pointer : {0u, object}) {
    for (uint32_t first : {0u, 1u, 0x80000000u, 0xFFFFFFFFu}) {
      for (uint32_t second : {0u, 1u, 0x80000000u, 0xFFFFFFFFu}) {
        auto ctx = test.Context(seed++);
        ctx.r3.u64 = high | pointer;
        test.Fill(object, 16000, seed);
        test.Store32(object + 15364, first);
        test.Store32(object + 15432, second);
        test.Check("nullable active state", __imp__sub_826FA128, sub_826FA128, ctx,
                   {{object, 16000}});
      }
    }
  }

  const uint32_t error_codes[] = {0, 1, 0xFFFFFFFF, 0x8007000E, 0x80004001,
      0x88960000, 0x8896FFFF, 0x88780078, 0x88780079, 0x88780096,
      0x88880001, 0x88970001, 0x08960005, 0x88780077, 0x7FFFFFFF, 0x80000000};
  for (unsigned i = 0; i < 144; ++i) {
    const uint32_t error = i < std::size(error_codes) ? error_codes[i] : Next(random);
    auto ctx = test.Context(seed++);
    ctx.r3.u64 = high | object;
    test.Fill(object, 16, seed);
    test.Store32(object, error);
    test.Check("HRESULT mapping", __imp__sub_82643B70, sub_82643B70, ctx, {{object, 16}});
  }

  for (unsigned i = 0; i < 160; ++i) {
    auto ctx = test.Context(seed++);
    const uint32_t left = i % 17 == 0 ? 0 : object + 32;
    const uint32_t right = i % 19 == 0 ? 0 : (i % 5 == 0 ? object + 32 : data + 32);
    ctx.r3.u64 = high | left;
    ctx.r4.u64 = 0x5A3CC3A500000000ull | right;
    test.Fill(object, 256, seed);
    test.Fill(data, 256, seed);
    const uint32_t length = Next(random) % 96;
    for (uint32_t j = 0; j < length; ++j) {
      const uint8_t value = Next(random) % 255 + 1;
      test.Store8(object + 32 + j, value);
      test.Store8(data + 32 + j, value);
    }
    test.Store8(object + 32 + length, 0);
    test.Store8(data + 32 + length, 0);
    if (i % 3 == 1) test.Store8(data + 32 + length / 2, 0);
    if (i % 3 == 2) test.Store8(data + 32 + length / 2, 255);
    test.Check("nullable string equality", __imp__sub_824279F0, sub_824279F0, ctx,
               {{object, 256}, {data, 256}});
  }

  const uint32_t indices[] = {0, 1, 7, 15, 30, 31, 32, 33, 63, 64, 65,
      95, 96, 127, 128, 255, 256, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF};
  for (uint32_t index : indices) {
    for (uint32_t mask : {0u, 1u, 0x80000000u, 0xFFFFFFFFu, 0x55AA55AAu}) {
      auto ctx = test.Context(seed++);
      ctx.r3.u64 = high | index;
      test.Store32(0x829A21F4, mask);
      test.Check("wrapped flag selection", __imp__sub_82434978, sub_82434978, ctx,
                 {{0x829A21F4, 4}});
    }
    auto ctx = test.Context(seed++);
    test.Store32(0x82911E60, index);
    test.Fill(0x829A2214, 8, seed);
    test.Check("selected flag mask", __imp__sub_82434BA0, sub_82434BA0, ctx,
               {{0x82911E60, 4}, {0x829A2214, 8}});
  }

  for (uint32_t first : {0u, 1u, 128u, 255u}) {
    for (uint32_t second : {0u, 1u, 128u, 255u}) {
      auto ctx = test.Context(seed++);
      test.Store8(0x829A21EB, first);
      test.Store8(0x829A2208, second);
      test.Check("either active flag", __imp__sub_824348C8, sub_824348C8, ctx,
                 {{0x829A21EB, 1}, {0x829A2208, 1}});
      ctx = test.Context(seed++);
      test.Store8(0x829A1912, first);
      test.Store8(0x829A1913, second);
      test.Check("inhibition flags", __imp__sub_8242ED00, sub_8242ED00, ctx,
                 {{0x829A1912, 2}});
    }
  }

  for (unsigned i = 0; i < 128; ++i) {
    auto ctx = test.Context(seed++);
    const uint32_t previous = Next(random);
    const uint32_t selected = i % 2 ? previous : Next(random);
    ctx.r3.u64 = high | object;
    ctx.r4.u64 = 0x3141592600000000ull | selected;
    test.Fill(object, 256, seed);
    test.Store32(object + 136, previous);
    test.Check("select and reset playback", __imp__sub_82425CE0, sub_82425CE0, ctx,
               {{object, 256}});
  }

  for (uint32_t index : {0u, 1u, 6u, 7u, 8u, 125u, 126u, 127u, 128u,
                         0x7FFFFFFFu, 0xFFFFFFF0u, 0xFFFFFFFFu}) {
    for (bool null_table : {false, true}) {
      auto ctx = test.Context(seed++);
      ctx.r3.u64 = high | (null_table ? 0 : object + 4096);
      ctx.r4.u64 = 0x39A15CE700000000ull | index;
      test.Fill(object, 8192, seed);
      test.Check("clear indexed triplet", __imp__sub_82803538, sub_82803538, ctx,
                 {{object, 8192}});
    }
    auto ctx = test.Context(seed++);
    ctx.r3.u64 = high | (object + 4096);
    ctx.r4.u64 = 0x39A15CE700000000ull | index;
    ctx.r5.u64 = uint64_t(Next(random)) << 32 | Next(random);
    ctx.r6.u64 = uint64_t(Next(random)) << 32 | Next(random);
    test.Fill(object, 8192, seed);
    test.Check("set indexed triplet", __imp__sub_82803460, sub_82803460, ctx,
               {{object, 8192}});
  }

  for (uint32_t enabled : {0u, 1u, 255u}) {
    for (uint32_t active : {0u, 1u, 0x80000000u, 0xFFFFFFFFu}) {
      for (uint32_t mask : {0u, 1u, 0x80000000u, 0xFFFFFFFFu}) {
        auto ctx = test.Context(seed++);
        ctx.r3.u64 = high | object;
        test.Fill(object, 1280, seed);
        test.Store8(object + 1188, enabled);
        test.Store32(object + 60, active);
        test.Store32(object + 1192, mask);
        test.Check("masked state transition", __imp__sub_824A5880, sub_824A5880, ctx,
                   {{object, 1280}});
      }
    }
  }

  const std::array<std::array<uint16_t, 2>, 6> bounds = {{
      {0xFFFF, 1}, {0, 0}, {0x8000, 0x7FFF}, {4, 2}, {0x7FFF, 0x8000}, {0xFFFF, 0xFFFF}}};
  for (const auto& bound : bounds) {
    for (uint32_t value : {0u, 1u, 0xFFFFu, 0x10000u, 0x7FFFFFFFu,
                           0x80000000u, 0xFFFF0000u, 0xFFFFFFFFu}) {
      for (bool alias : {false, true}) {
        auto ctx = test.Context(seed++);
        ctx.r3.u64 = high | object;
        ctx.r4.u64 = 0x3F5196B800000000ull | (alias ? object + 18 : data);
        test.Fill(object, 64, seed);
        test.Fill(data, 64, seed);
        test.Store16(object + 18, bound[0]);
        test.Store16(object + 20, bound[1]);
        test.Store32(alias ? object + 18 : data, value);
        test.Check("fixed-point clamp", __imp__sub_824987A0, sub_824987A0, ctx,
                   {{object, 64}, {data, 64}});
      }
    }
  }

  for (uint32_t flags = 0; flags < 256; ++flags) {
    auto ctx = test.Context(seed++);
    test.Store16(0x82991518, uint16_t(flags | (Next(random) & 0xFF00)));
    test.Check("selected low-nibble flags", __imp__sub_82421B40, sub_82421B40, ctx,
               {{0x82991518, 2}});
  }

  for (uint32_t value : {0u, 1u, 2u, 31u, 127u, 1000u, 4095u}) {
    for (uint32_t divisor : {0u, 1u, 2u, 3u, 31u, 0x7FFFFFFFu}) {
      auto ctx = test.Context(seed++);
      ctx.r3.u64 = high | value;
      ctx.r4.u64 = 0x3A5917BC00000000ull | divisor;
      test.Check("remainder", __imp__sub_827667E0, sub_827667E0, ctx, {});
    }
  }
  for (const auto& pair : std::array<std::array<uint32_t, 2>, 7>{{
           {0xFFFFFFFF, 1}, {0xFFFFFFFE, 3}, {0xFFFFFF00, 17},
           {0, 0x80000000}, {1, 0x80000001},
           {0x7FFFFFFF, 0x7FFFFFFF}, {0x7FFFFFFE, 0x7FFFFFFF}}}) {
    auto ctx = test.Context(seed++);
    ctx.r3.u64 = high | pair[0];
    ctx.r4.u64 = 0x7193B3C800000000ull | pair[1];
    test.Check("signed remainder exits", __imp__sub_827667E0, sub_827667E0, ctx, {});
  }

  for (uint32_t first = 0; first < 256; ++first) {
    for (unsigned scenario = 0; scenario < 3; ++scenario) {
      auto ctx = test.Context(seed++);
      const uint8_t second = uint8_t(first + 1);
      const int32_t first_signed = int8_t(uint8_t(first));
      const int32_t second_signed = int8_t(second);
      const uint32_t requested = scenario == 0 ? uint32_t(first_signed)
          : scenario == 1 ? uint32_t(second_signed) : 1000;
      ctx.r3.u64 = high | object;
      ctx.r4.u64 = uint64_t(Next(random)) << 32 | requested;
      test.Fill(object, 1024, seed);
      test.Store8(object + 985, first);
      test.Store8(object + 986, second);
      test.Check("two signed-byte matches", __imp__sub_82496990, sub_82496990, ctx,
                 {{object, 1024}});
    }
  }
  test.Report("native queries (25 functions)");
}
