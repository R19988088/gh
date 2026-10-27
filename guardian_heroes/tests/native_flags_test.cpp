#include "native_helpers_test.h"

#include <array>
#include <random>

#define NATIVE_FLAG_FUNCTIONS(X) \
  X(sub_8246BCE0) X(sub_8242E8A8) X(sub_825D7768) X(sub_825D7CD8) \
  X(sub_825D77F8) X(sub_825D7648) X(sub_825D76D8) X(sub_825D87E0) \
  X(sub_825D7D80) X(sub_825D8B38) X(sub_825D7610) X(sub_824349A8) \
  X(sub_824349E0) X(sub_82838288) X(sub_824380B0) X(sub_8288B988) \
  X(sub_82410600) X(sub_8243EEA0) X(sub_82419C88) X(sub_8241B800) \
  X(sub_82778978) X(sub_8282F660) X(sub_82669DA0) X(sub_828034A0) \
  X(sub_825DAA38)
#define DECLARE_NATIVE_FLAG(name) REX_EXTERN(name); REX_EXTERN(__imp__##name);
NATIVE_FLAG_FUNCTIONS(DECLARE_NATIVE_FLAG)
#undef DECLARE_NATIVE_FLAG

namespace {
struct Entry { const char* name; GuestFunction* original; GuestFunction* native; };
#define FLAG_ENTRY(name) Entry{#name, __imp__##name, name}
constexpr std::array<uint32_t, 17> kEdges = {
    0, 1, 2, 3, 4, 5, 30, 31, 32, 63, 64, 127, 255,
    0x7FFFFFFF, 0x80000000, 0xFFFFFFFE, 0xFFFFFFFF};

uint64_t HighWord(std::mt19937_64& random, uint32_t low) {
  return (random() & 0xFFFFFFFF00000000ull) | low;
}

void TestObjectBit(NativeHelpersTest& test, std::mt19937_64& random) {
  for (uint32_t i = 0; i < 256; ++i) {
    auto ctx = test.Context(i);
    ctx.r3.u64 = HighWord(random, test.kObject);
    ctx.r4.u64 = HighWord(random, kEdges[i % kEdges.size()]);
    ctx.r5.u64 = HighWord(random, i % 3 == 0 ? 0x100 : i % 3 == 1 ? 1 : 0xFF);
    test.Fill(test.kObject, 2048, i);
    test.Store32(test.kObject + 1860, i & 1 ? uint32_t(random()) : 0);
    test.Check("sub_8246BCE0", __imp__sub_8246BCE0, sub_8246BCE0, ctx,
               {{test.kObject, 2048}});
  }
}

void TestGlobalBits(NativeHelpersTest& test, std::mt19937_64& random) {
  const std::array<Entry, 3> entries = {
      FLAG_ENTRY(sub_8242E8A8), FLAG_ENTRY(sub_824349A8), FLAG_ENTRY(sub_824349E0)};
  constexpr std::array<uint32_t, 3> masks = {0, 0xFFFFFFFF, 0x80000001};
  for (const auto& entry : entries) {
    for (uint32_t i = 0; i < kEdges.size() * masks.size() + 128; ++i) {
      auto ctx = test.Context(i);
      const uint32_t index = i < kEdges.size() * masks.size()
          ? kEdges[i / masks.size()] : uint32_t(random());
      const uint32_t mask = i < kEdges.size() * masks.size()
          ? masks[i % masks.size()] : uint32_t(random());
      ctx.r3.u64 = HighWord(random, index);
      test.Store32(0x82911E60, index);
      test.Store32(0x829A084C, mask);
      test.Store32(0x829A21F4, mask);
      test.Store32(0x829A21F8, mask);
      test.Check(entry.name, entry.original, entry.native, ctx,
                 {{0x82911E60, 4}, {0x829A084C, 4}, {0x829A21F4, 8}});
    }
  }
}

void TestRenderMetadata(NativeHelpersTest& test, std::mt19937_64& random) {
  const std::array<Entry, 9> entries = {
      FLAG_ENTRY(sub_825D7768), FLAG_ENTRY(sub_825D7CD8), FLAG_ENTRY(sub_825D77F8),
      FLAG_ENTRY(sub_825D7648), FLAG_ENTRY(sub_825D76D8), FLAG_ENTRY(sub_825D87E0),
      FLAG_ENTRY(sub_825D7D80), FLAG_ENTRY(sub_825D8B38), FLAG_ENTRY(sub_825D7610)};
  constexpr std::array<uint32_t, 8> words = {
      0, 0x10, 0x1000, 0x1010, 0xFFFF, 0xFFFF0000, 0xFFFFFFFF, 0x12345678};
  for (const auto& entry : entries) {
    for (uint32_t i = 0; i < 192; ++i) {
      auto ctx = test.Context(i);
      ctx.r3.u64 = HighWord(random, test.kObject);
      ctx.r4.u64 = HighWord(random, i < 128 ? kEdges[(i / 4) % kEdges.size()] : uint32_t(random()));
      test.Fill(test.kObject, 16384, i);
      test.Store32(test.kObject + 12152, i < 128 ? words[(i / 16) % words.size()] : uint32_t(random()));
      test.Store32(test.kObject + 12156, (uint32_t(random()) & 0x3FFFFFFF) | ((i & 3) << 30));
      test.Store32(test.kObject + 12704, i % 3 == 0 ? 0 : i % 3 == 1 ? 1 : uint32_t(random()));
      test.Store64(test.kObject + 16, random());
      test.Store64(test.kObject + 32, random());
      test.Check(entry.name, entry.original, entry.native, ctx, {{test.kObject, 16384}});
    }
  }
}

void TestScalarQueries(NativeHelpersTest& test, std::mt19937_64& random) {
  for (uint32_t i = 0; i < 256; ++i) {
    auto ctx = test.Context(i);
    ctx.r4.u64 = HighWord(random, i < kEdges.size() ? kEdges[i] : uint32_t(random()));
    test.Check("sub_82838288", __imp__sub_82838288, sub_82838288, ctx, {});

    const uint8_t enabled = i < 4 ? uint8_t(i) : uint8_t(random());
    test.Store8(0x829AACF8, enabled);
    test.Store8(0x82911E9C, uint8_t(random()));
    test.Check("sub_824380B0", __imp__sub_824380B0, sub_824380B0, ctx,
               {{0x829AACF8, 1}, {0x82911E9C, 1}});

    const uint32_t first = i < kEdges.size() ? kEdges[i] : uint32_t(random());
    test.Store32(0x82979830, first);
    test.Store32(0x82A05A74, i & 1 ? first | 1 : uint32_t(random()));
    test.Check("sub_8288B988", __imp__sub_8288B988, sub_8288B988, ctx,
               {{0x82979830, 4}, {0x82A05A74, 4}});
  }
}

void TestByteState(NativeHelpersTest& test, std::mt19937_64& random) {
  constexpr uint32_t pointer = 0x8297B190;
  for (uint32_t i = 0; i < 160; ++i) {
    auto ctx = test.Context(i);
    const bool alias = i % 4 == 0;
    const uint8_t value = alias ? (i & 4 ? 0x82 : 0x83) : uint8_t(i);
    const uint32_t object = alias ? pointer - 162 : test.kObject;
    const uint32_t reloaded = alias ? (object & 0xFFFFFF) | (uint32_t(value) << 24) : object;
    ctx.r3.u64 = HighWord(random, 0x12340000u | value);
    test.Fill(object, 200, i);
    test.Fill(reloaded, 200, i);
    test.Store32(pointer, object);
    test.Check("sub_82410600", __imp__sub_82410600, sub_82410600, ctx,
               {{object, 200}, {reloaded, 200}, {pointer, 4}});
  }
}

void TestIndexedArithmetic(NativeHelpersTest& test, std::mt19937_64& random) {
  constexpr uint32_t table = 0x82990FD0;
  for (uint32_t i = 0; i < 256; ++i) {
    auto ctx = test.Context(i);
    ctx.r3.u64 = random();
    ctx.r4.u64 = random();
    ctx.r5.u64 = random();
    test.Check("sub_8243EEA0", __imp__sub_8243EEA0, sub_8243EEA0, ctx, {});
    test.Check("sub_8241B800", __imp__sub_8241B800, sub_8241B800, ctx, {});

    const int32_t index = i < 132 ? int32_t(i) - 128 : int32_t(i);
    ctx.r3.u64 = HighWord(random, uint32_t(index));
    test.Fill(table - 1536, 1600, i);
    test.Check("sub_82419C88", __imp__sub_82419C88, sub_82419C88, ctx,
               {{table - 1536, 1600}});
  }
}

void TestFixedPointTable(NativeHelpersTest& test, std::mt19937_64& random) {
  constexpr uint32_t table = 0x823A6800;
  for (uint32_t i = 0; i < 256; ++i) {
    auto ctx = test.Context(i);
    const uint32_t index = uint32_t(random() % 32);
    ctx.r4.u64 = HighWord(random, i < kEdges.size() ? kEdges[i] : uint32_t(random()));
    ctx.r5.u64 = HighWord(random, index);
    test.Fill(table, 128, i);
    test.Store32(table + index * 4, i < kEdges.size() ? kEdges[kEdges.size() - 1 - i] : uint32_t(random()));
    test.Check("sub_82778978", __imp__sub_82778978, sub_82778978, ctx, {{table, 128}});
  }
}

void TestThreeWayRead(NativeHelpersTest& test, std::mt19937_64& random) {
  for (uint32_t i = 0; i < 128; ++i) {
    auto ctx = test.Context(i);
    ctx.r3.u64 = HighWord(random, test.kObject);
    ctx.r4.u64 = HighWord(random, i < kEdges.size() ? kEdges[i] : uint32_t(random()));
    test.Fill(test.kObject, 80, i);
    test.Check("sub_8282F660", __imp__sub_8282F660, sub_8282F660, ctx,
               {{test.kObject, 80}});
  }
}

void TestMaskRank(NativeHelpersTest& test, std::mt19937_64& random) {
  for (uint32_t i = 0; i < 256; ++i) {
    auto ctx = test.Context(i);
    const uint32_t target = i < 32 ? 1u << i : i < 64 ? 0 : uint32_t(random());
    const uint32_t source = i < 32 ? 0xFFFFFFFF : uint32_t(random());
    ctx.r3.u64 = HighWord(random, source);
    ctx.r4.u64 = HighWord(random, target);
    ctx.r5.u64 = HighWord(random, test.kData);
    test.Fill(test.kData, 8, i);
    test.Check("sub_82669DA0", __imp__sub_82669DA0, sub_82669DA0, ctx,
               {{test.kData, 8}});
  }
}

void TestSlotAllocation(NativeHelpersTest& test, std::mt19937_64& random) {
  for (uint32_t i = 0; i < 160; ++i) {
    auto ctx = test.Context(i);
    const uint32_t slot = i % 5 == 0 ? 127 : 7 + uint32_t(random() % 120);
    const uint32_t output = i % 4 == 0 && slot < 127
        ? test.kObject + slot * 12 : test.kData;
    ctx.r3.u64 = HighWord(random, test.kObject);
    ctx.r4.u64 = HighWord(random, 9);
    ctx.r5.u64 = HighWord(random, 11);
    ctx.r6.u64 = HighWord(random, output);
    test.Fill(test.kObject, 1600, i);
    test.Fill(test.kData, 16, i);
    for (uint32_t j = 7; j < 127; ++j) test.Store32(test.kObject + j * 12, j == slot ? 0 : uint32_t(random()) | 1);
    test.Check("sub_828034A0", __imp__sub_828034A0, sub_828034A0, ctx,
               {{test.kObject, 1600}, {test.kData, 16}});
  }
}

void TestFormatWidths(NativeHelpersTest& test, std::mt19937_64& random) {
  for (uint32_t i = 0; i < 160; ++i) {
    auto ctx = test.Context(i);
    ctx.r3.u64 = HighWord(random, i < 80 ? i : uint32_t(random()));
    ctx.r4.u64 = HighWord(random, test.kData);
    ctx.r5.u64 = HighWord(random, test.kData + (i & 1 ? 0 : 4));
    test.Fill(test.kData, 16, i);
    test.Check("sub_825DAA38", __imp__sub_825DAA38, sub_825DAA38, ctx,
               {{test.kData, 16}});
  }
}
}  // namespace

int main() {
  NativeHelpersTest test;
  std::mt19937_64 random(0x825D7768828034A0ull);
  TestObjectBit(test, random);
  TestGlobalBits(test, random);
  TestRenderMetadata(test, random);
  TestScalarQueries(test, random);
  TestByteState(test, random);
  TestIndexedArithmetic(test, random);
  TestFixedPointTable(test, random);
  TestThreeWayRead(test, random);
  TestMaskRank(test, random);
  TestSlotAllocation(test, random);
  TestFormatWidths(test, random);
  test.Report("native flags");
}
