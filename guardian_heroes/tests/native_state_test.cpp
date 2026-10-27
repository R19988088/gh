#include "native_helpers_test.h"

REX_EXTERN(__imp__sub_824D0BE8);
REX_EXTERN(__imp__sub_82435950);
REX_EXTERN(__imp__sub_8243A9D8);
REX_EXTERN(__imp__sub_82496508);
REX_EXTERN(__imp__sub_82425D18);
REX_EXTERN(sub_824D0BE8);
REX_EXTERN(sub_82435950);
REX_EXTERN(sub_8243A9D8);
REX_EXTERN(sub_82496508);
REX_EXTERN(sub_82425D18);

namespace {
uint32_t Next(uint32_t& state) {
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  return state;
}
}  // namespace

int main() {
  NativeHelpersTest test;
  constexpr uint32_t object = NativeHelpersTest::kObject;
  constexpr uint32_t data = NativeHelpersTest::kData;
  uint32_t random = 0x92B7A10D;
  uint32_t seed = 0;

  const uint32_t differences[] = {0, 1, 2, 3, 4, 7, 0xFFFFFFFF,
      0xFFFFFFFE, 0xFFFFFFFD, 0xFFFFFFFC, 0x7FFFFFFF, 0x80000000};
  const uint16_t states[] = {0, 7, 8, 9, 0xFFFF};
  for (uint32_t difference : differences) {
    for (uint16_t state : states) {
      auto context = test.Context(seed++);
      context.r3.u64 = 0xABCD123400000000ull | object;
      test.Fill(object, 4096, seed);
      const uint32_t begin = Next(random);
      test.Store32(object + 3500, begin);
      test.Store32(object + 3504, begin + difference);
      test.Store16(object + 976, state);
      test.Check("queue readiness", __imp__sub_824D0BE8, sub_824D0BE8,
                 context, {{object, 4096}});
    }
  }
  for (unsigned i = 0; i < 128; ++i) {
    auto context = test.Context(seed++);
    context.r3.u64 = 0x1234567800000000ull | object;
    test.Fill(object, 16, seed);
    const uint16_t current = Next(random);
    test.Store16(object, current);
    test.Store16(object + 2, i % 3 == 0 ? current : uint16_t(Next(random)));
    test.Store8(object + 7, i % 2 == 0 ? 0 : uint8_t(Next(random) | 1));
    test.Check("state commit", __imp__sub_82435950, sub_82435950,
               context, {{object, 16}});
  }

  const uint32_t counters[] = {0, 1, 2, 0x7FFFFFFE, 0x7FFFFFFF,
      0x80000000, 0x80000001, 0xFFFFFFFE, 0xFFFFFFFF};
  for (uint32_t limit : counters) {
    for (uint32_t current : counters) {
      auto context = test.Context(seed++);
      context.r3.u64 = 0xF123456700000000ull | object;
      test.Fill(object, 16, seed);
      test.Store32(object, current);
      test.Store32(object + 8, limit);
      test.Check("bounded increment", __imp__sub_8243A9D8, sub_8243A9D8,
                 context, {{object, 16}});
    }
  }
  for (unsigned i = 0; i < 128; ++i) {
    auto context = test.Context(seed++);
    context.r3.u64 = 0x9876543200000000ull | object;
    test.Fill(object, 16, seed);
    test.Store32(object, Next(random));
    test.Store32(object + 8, Next(random));
    test.Check("random bounded increment", __imp__sub_8243A9D8,
               sub_8243A9D8, context, {{object, 16}});
  }

  for (uint32_t value = 0; value < 256; ++value) {
    auto context = test.Context(seed++);
    const uint32_t slot = Next(random) % 128;
    context.r3.u64 = 0x1357246800000000ull | object;
    context.r4.u64 = 0xFEDCBA9800000000ull | slot;
    test.Fill(object, 256, seed);
    test.Store8(object + slot + 20, value);
    test.Check("slot availability", __imp__sub_82496508, sub_82496508,
               context, {{object, 256}});
  }
  for (uint32_t slot : {0x80000000u, 0xFFFFFFFEu, 0xFFFFFFFFu}) {
    auto context = test.Context(seed++);
    context.r3.u64 = 0x1234567800000000ull | object;
    context.r4.u64 = 0x1020304000000000ull | slot;
    test.Fill(object, 256, seed);
    test.Check("negative slot", __imp__sub_82496508, sub_82496508,
               context, {{object, 256}});
  }

  const uint16_t counts[] = {0, 1, 2, 0x7FFF, 0x8000, 0xFFFF};
  const uint32_t requests[] = {0, 1, 2, 0x7FFF, 0xFFFF, 0x7FFFFFFF,
      0x80000000, 0xFFFFFFFF};
  for (uint16_t count : counts) {
    for (uint32_t request : requests) {
      auto context = test.Context(seed++);
      context.r3.u64 = 0x1234567800000000ull | object;
      context.r4.u64 = 0x8765432100000000ull | request;
      test.Fill(object, 256, seed);
      test.Fill(data, 256, seed);
      const uint32_t index = Next(random) % 32;
      test.Store32(object + 136, index | 0xC0000000);
      test.Store32(object + 112, data);
      test.Store16(data + index * 4 + 2, count);
      test.Check("entry selection", __imp__sub_82425D18, sub_82425D18,
                 context, {{object, 256}, {data, 256}});
    }
  }
  for (unsigned i = 0; i < 128; ++i) {
    auto context = test.Context(seed++);
    context.r3.u64 = 0xE123456700000000ull | object;
    context.r4.u64 = uint64_t(Next(random)) << 32 | Next(random);
    test.Fill(object, 256, seed);
    test.Fill(data, 256, seed);
    const uint32_t index = Next(random) % 32;
    const uint32_t table = i % 3 == 0 ? object + 174 : data;
    const uint32_t offset = table == data ? index * 4 : 0;
    test.Store32(object + 136, offset / 4 | 0x40000000);
    test.Store32(object + 112, table);
    test.Store16(table + offset + 2, uint16_t(Next(random)));
    test.Check("random entry selection", __imp__sub_82425D18, sub_82425D18,
               context, {{object, 256}, {data, 256}});
  }
  test.Report("native state helpers");
}
