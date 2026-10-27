#include "native_helpers_test.h"

#include <array>

REX_EXTERN(sub_8246B550);
REX_EXTERN(__imp__sub_8246B550);
REX_EXTERN(sub_8241BAF0);
REX_EXTERN(__imp__sub_8241BAF0);
REX_EXTERN(sub_824A7998);
REX_EXTERN(__imp__sub_824A7998);
REX_EXTERN(sub_82498068);
REX_EXTERN(__imp__sub_82498068);
REX_EXTERN(sub_82888BD8);
REX_EXTERN(__imp__sub_82888BD8);

int main() {
  NativeHelpersTest test;
  constexpr uint32_t object = NativeHelpersTest::kObject;
  constexpr uint32_t data = NativeHelpersTest::kData;
  constexpr uint32_t table = 0x82991138;

  for (uint32_t index : {0u, 1u, 2u, 3u, 4u, 5u, 100u, 0xFFFFFFFFu}) {
    for (uint32_t value = 0; value < 256; ++value) {
      test.Fill(object, 2048, value);
      for (uint32_t offset : {1396u, 1464u, 1532u, 1708u}) test.Store8(object + offset, value);
      auto ctx = test.Context(value);
      ctx.r3.u64 = 0x1234567800000000ull | object;
      ctx.r4.u64 = (uint64_t(value & 1) << 32) | index;
      test.Check("byte counters", __imp__sub_8246B550, sub_8246B550, ctx, {{object, 2048}});
    }
  }

  for (uint32_t index = 0; index < 256; ++index) {
    test.Fill(table - 4096, 8192, index);
    test.Fill(object, 128, index);
    test.Store8(object + 23, index);
    auto ctx = test.Context(index);
    ctx.r3.u64 = 0xFEDCBA9800000000ull | object;
    test.Check("five-word table record", __imp__sub_8241BAF0, sub_8241BAF0, ctx,
               {{object, 128}, {table - 4096, 8192}});
  }
  for (int offset : {-12, -4, 0, 4, 12}) {
    test.Fill(table - 4096, 8192, offset);
    const uint32_t alias_object = table + 20 + offset - 24;
    test.Store8(alias_object + 23, 1);
    auto ctx = test.Context(offset);
    ctx.r3.u64 = alias_object;
    test.Check("table record overlap", __imp__sub_8241BAF0, sub_8241BAF0, ctx,
               {{table - 4096, 8192}});
  }

  for (uint32_t value = 0; value < 256; ++value) {
    for (uint32_t direction : {0u, 1u, 0x100u, 0xFFFFFFFFu}) {
      for (auto bounds : std::array<std::array<int32_t, 2>, 5>{{
               {-128, 127}, {0, 4}, {1, 1}, {10, -10}, {INT32_MIN, INT32_MAX}}}) {
        test.Fill(object, 16, value);
        test.Store8(object, value);
        auto ctx = test.Context(value);
        ctx.r3.u64 = object;
        ctx.r4.u64 = 0x1234567800000000ull | uint32_t(bounds[0]);
        ctx.r5.u64 = 0x8765432100000000ull | uint32_t(bounds[1]);
        ctx.r6.u64 = direction;
        test.Check("byte clamp", __imp__sub_824A7998, sub_824A7998, ctx, {{object, 16}});
      }
    }
  }

  for (uint32_t first_free = 0; first_free <= 90; ++first_free) {
    for (uint32_t flag : {0u, 2u, 128u, 254u}) {
      test.Fill(object, 47200, first_free);
      for (uint32_t i = 0; i < 90; ++i) test.Store8(object + 31 + i * 524, i < first_free ? 255 : flag);
      auto ctx = test.Context(first_free);
      ctx.r3.u64 = 0x1234567800000000ull | object;
      test.Check("free-slot scan", __imp__sub_82498068, sub_82498068, ctx, {{object, 47200}});
    }
  }

  for (uint32_t size : {0u, 1u, 2u, 3u, 4u, 7u, 8u, 15u, 16u, 31u, 32u,
                        63u, 64u, 127u, 128u, 129u, 255u, 259u, 260u, 1024u}) {
    for (uint32_t null_at : {0u, 1u, size ? size - 1 : 0, size, size + 1}) {
      for (int delta : {-2048, -127, -1, 0, 1, 127, 2048}) {
        test.Fill(object, 0x18000, size);
        const uint32_t source = data + 4096;
        for (uint32_t i = 0; i <= size + 1; ++i) test.Store8(source + i, i % 255 + 1);
        test.Store8(source + null_at, 0);
        auto ctx = test.Context(size + null_at);
        ctx.r3.u64 = 0x1234567800000000ull | (source + delta);
        ctx.r4.u64 = 0x8765432100000000ull | source;
        ctx.r5.u64 = 0xABCDEF0100000000ull | size;
        test.Check("bounded string copy", __imp__sub_82888BD8, sub_82888BD8, ctx,
                   {{object, 0x18000}});
      }
    }
  }
  test.Report("byte processing (5 functions)");
}
