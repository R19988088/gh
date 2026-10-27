#include "native_helpers_test.h"

REX_EXTERN(sub_82887F88);
REX_EXTERN(__imp__sub_82887F88);
REX_EXTERN(sub_8243F2C8);
REX_EXTERN(__imp__sub_8243F2C8);
REX_EXTERN(sub_82435928);
REX_EXTERN(__imp__sub_82435928);
REX_EXTERN(sub_8243FCE0);
REX_EXTERN(__imp__sub_8243FCE0);
REX_EXTERN(sub_8246AC40);
REX_EXTERN(__imp__sub_8246AC40);
REX_EXTERN(sub_82495BC0);
REX_EXTERN(__imp__sub_82495BC0);
REX_EXTERN(sub_82420AB0);
REX_EXTERN(__imp__sub_82420AB0);
REX_EXTERN(sub_826CA638);
REX_EXTERN(__imp__sub_826CA638);
REX_EXTERN(sub_82495BA0);
REX_EXTERN(__imp__sub_82495BA0);
REX_EXTERN(sub_82689358);
REX_EXTERN(__imp__sub_82689358);
REX_EXTERN(sub_824156A0);
REX_EXTERN(__imp__sub_824156A0);
REX_EXTERN(sub_82420D50);
REX_EXTERN(__imp__sub_82420D50);
REX_EXTERN(sub_827EACF8);
REX_EXTERN(__imp__sub_827EACF8);
REX_EXTERN(sub_8278EE28);
REX_EXTERN(__imp__sub_8278EE28);
REX_EXTERN(sub_827D3DC0);
REX_EXTERN(__imp__sub_827D3DC0);
REX_EXTERN(sub_82495C88);
REX_EXTERN(__imp__sub_82495C88);
REX_EXTERN(sub_8281D550);
REX_EXTERN(__imp__sub_8281D550);
REX_EXTERN(sub_8269B3E8);
REX_EXTERN(__imp__sub_8269B3E8);
REX_EXTERN(sub_824257D8);
REX_EXTERN(__imp__sub_824257D8);
REX_EXTERN(sub_824964A0);
REX_EXTERN(__imp__sub_824964A0);
REX_EXTERN(sub_826536C0);
REX_EXTERN(__imp__sub_826536C0);
REX_EXTERN(sub_8248C130);
REX_EXTERN(__imp__sub_8248C130);
REX_EXTERN(sub_82411A08);
REX_EXTERN(__imp__sub_82411A08);
REX_EXTERN(sub_82425808);
REX_EXTERN(__imp__sub_82425808);
REX_EXTERN(sub_8280FD08);
REX_EXTERN(__imp__sub_8280FD08);

struct RecordHook { const char* name; GuestFunction* original; GuestFunction* native; };

int main() {
  NativeHelpersTest test;
  constexpr uint32_t object = NativeHelpersTest::kObject;
  constexpr uint32_t data = NativeHelpersTest::kData;
  constexpr uint64_t high = 0xD531A79600000000ull;
  uint32_t seed = 0;
  const RecordHook constructors[] = {
    {"sub_8243F2C8", __imp__sub_8243F2C8, sub_8243F2C8},
    {"sub_82435928", __imp__sub_82435928, sub_82435928},
    {"sub_8243FCE0", __imp__sub_8243FCE0, sub_8243FCE0},
    {"sub_8246AC40", __imp__sub_8246AC40, sub_8246AC40},
    {"sub_82495BC0", __imp__sub_82495BC0, sub_82495BC0},
    {"sub_82420AB0", __imp__sub_82420AB0, sub_82420AB0},
    {"sub_826CA638", __imp__sub_826CA638, sub_826CA638},
    {"sub_82495BA0", __imp__sub_82495BA0, sub_82495BA0},
    {"sub_82689358", __imp__sub_82689358, sub_82689358},
    {"sub_82420D50", __imp__sub_82420D50, sub_82420D50},
    {"sub_82495C88", __imp__sub_82495C88, sub_82495C88},
    {"sub_8281D550", __imp__sub_8281D550, sub_8281D550},
    {"sub_8269B3E8", __imp__sub_8269B3E8, sub_8269B3E8},
    {"sub_824257D8", __imp__sub_824257D8, sub_824257D8},
    {"sub_824964A0", __imp__sub_824964A0, sub_824964A0},
    {"sub_826536C0", __imp__sub_826536C0, sub_826536C0},
    {"sub_8248C130", __imp__sub_8248C130, sub_8248C130},
    {"sub_82411A08", __imp__sub_82411A08, sub_82411A08},
    {"sub_82425808", __imp__sub_82425808, sub_82425808},
    {"sub_8280FD08", __imp__sub_8280FD08, sub_8280FD08},
  };
  // Misalignment, both ordinary virtual ranges, physical aliases and range crossings.
  for (const auto& hook : constructors) {
    for (uint32_t address : {object, object + 1, object + 3, 0x80010000u,
                             0x7EFFFFF0u, 0x9FFFFFF0u, 0xE0010000u}) {
      for (uint32_t sample = 0; sample < 16; ++sample) {
        auto ctx = test.Context(seed++);
        ctx.r3.u64 = (sample & 1 ? high : 0) | address;
        ctx.r4.u64 = 0x3A5917BC00000000ull | (sample * 0x1234567u);
        ctx.r5.u64 = ~ctx.r4.u64;
        ctx.r6.u64 = sample * 0xF1234567ull;
        ctx.r7.u64 = 0x718392A400000000ull | sample;
        ctx.r8.u64 = 0x5172839400000000ull | (sample % 3 ? sample * 0x1234567u : 0);
        test.Fill(address - 16, 608, seed);
        test.Store8(address + 177, sample % 3 ? uint8_t(sample * 17) : 0);
        test.Check(hook.name, hook.original, hook.native, ctx, {{address - 16, 608}});
      }
    }
  }

  for (uint32_t length : {0u, 1u, 2u, 15u, 16u, 31u, 127u, 255u}) {
    for (uint32_t address : {object, object + 1, 0x80010000u, 0xE0010000u}) {
      auto ctx = test.Context(seed++);
      ctx.r3.u64 = high | address;
      test.Fill(address - 8, 544, seed);
      for (uint32_t i = 0; i < length; ++i) test.Store16(address + 2 * i, uint16_t(0x100 + i));
      test.Store16(address + 2 * length, 0);
      test.Check("UTF16 length", __imp__sub_82887F88, sub_82887F88, ctx, {{address - 8, 544}});
    }
  }

  constexpr uint32_t globals = uint32_t(-2103967744ll - 5048);
  for (uint32_t value = 0; value < 256; ++value) {
    auto ctx = test.Context(seed++);
    ctx.r3.u64 = high | value;
    test.Fill(globals + 64, 8, seed);
    test.Check("global byte flags", __imp__sub_824156A0, sub_824156A0, ctx, {{globals + 64, 8}});
  }

  for (uint32_t value : {0u, 1u, 2u, 3u, 6u, 7u, 14u, 15u, 30u, 31u,
                         32u, 0x7FFFFFFFu, 0x80000000u, 0xFFFFFFE1u, 0xFFFFFFFFu}) {
    for (uint64_t upper : {uint64_t(0), high}) {
      auto ctx = test.Context(seed++);
      ctx.r3.u64 = upper | value; ctx.r4.u64 = high | data;
      test.Fill(data - 8, 32, seed);
      test.Check("magnitude bucket", __imp__sub_8278EE28, sub_8278EE28, ctx, {{data - 8, 32}});
    }
  }

  for (int32_t delta : {-20, -15, -8, -4, -1, 0, 1, 4, 8, 15, 20, 128}) {
    for (uint32_t source : {object + 32, 0x80010020u, 0xE0010020u}) {
      auto ctx = test.Context(seed++);
      ctx.r3.u64 = high | uint32_t(source + delta); ctx.r4.u64 = high | source;
      test.Fill(source - 32, 208, seed);
      test.Check("sequential 16-byte copy", __imp__sub_827D3DC0, sub_827D3DC0, ctx,
                 {{source - 32, 208}});
    }
  }

  for (uint32_t stride : {0u, 1u, 2u, 3u, 8u, 31u, 0xFFFFFFFFu}) {
    for (uint32_t count : {0u, 1u, 2u, 7u}) {
      for (int32_t delta : {-3, 0, 1, 7, 128}) {
        auto ctx = test.Context(seed++);
        ctx.r3.u64 = high | uint32_t(object + 32 + delta); ctx.r4.u64 = high | (object + 32);
        ctx.r5.u64 = 0x1928374600000000ull | stride;
        ctx.r6.u64 = 0x5128937400000000ull | count;
        test.Fill(object, 512, seed);
        test.Check("strided byte copy", __imp__sub_827EACF8, sub_827EACF8, ctx, {{object, 512}});
      }
    }
  }
  test.Report("native records (25 functions)");
}
