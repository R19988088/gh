#include "native_helpers_test.h"

#include <array>
#include <random>

REX_EXTERN(sub_8278E858);
REX_EXTERN(__imp__sub_8278E858);
REX_EXTERN(sub_8267A938);
REX_EXTERN(__imp__sub_8267A938);
REX_EXTERN(sub_82803DB8);
REX_EXTERN(__imp__sub_82803DB8);
REX_EXTERN(sub_828ABA20);
REX_EXTERN(__imp__sub_828ABA20);
REX_EXTERN(sub_82434A00);
REX_EXTERN(__imp__sub_82434A00);

namespace {
constexpr uint32_t kCostIndices = 0x82948B28;
constexpr uint32_t kSortedTable = 0x8239B1E8;
constexpr uint32_t kSelectedBit = 0x82911E60;
constexpr uint32_t kEnabledBits = 0x829A21F8;

void TestCostLookup(NativeHelpersTest& test, std::mt19937_64& random) {
  constexpr std::array<uint32_t, 13> values = {
      0, 1, 157, 158, 159, 0xFFFFFFFF, 0xFFFFFF62, 0xFFFFFF61,
      0x80000000, 0x7FFFFFFF, 23, 0xFFFFFFE9, 160};
  for (uint32_t i = 0; i < 159; ++i) test.Store32(kCostIndices + 4 * i, i % 6);
  for (uint32_t i = 0; i < values.size() * values.size() + 256; ++i) {
    auto ctx = test.Context(i);
    const uint32_t a = i < values.size() * values.size()
        ? values[i / values.size()] : uint32_t(int32_t(random() % 401) - 200);
    const uint32_t b = i < values.size() * values.size()
        ? values[i % values.size()] : uint32_t(int32_t(random() % 401) - 200);
    ctx.r4.u64 = (random() & 0xFFFFFFFF00000000ull) | a;
    ctx.r5.u64 = (random() & 0xFFFFFFFF00000000ull) | b;
    ctx.r6.u64 = (random() & 0xFFFFFFFF00000000ull) | test.kData;
    test.Fill(test.kData, 64, i);
    for (uint32_t j = 0; j < 6; ++j) test.Store32(test.kData + j * 4, uint32_t(random()));
    test.Check("sub_8278E858", __imp__sub_8278E858, sub_8278E858, ctx,
               {{kCostIndices, 159 * 4}, {test.kData, 64}});
  }
}

void TestSortedLookup(NativeHelpersTest& test, std::mt19937_64& random) {
  std::array<uint32_t, 1230> entries{};
  entries[0] = 1;
  size_t count = 1;
  for (uint32_t candidate = 2; candidate <= 9973; ++candidate) {
    bool prime = true;
    for (uint32_t factor = 2; factor * factor <= candidate; ++factor) {
      if (candidate % factor == 0) { prime = false; break; }
    }
    if (prime) entries[count++] = candidate;
  }
  for (uint32_t i = 0; i < entries.size(); ++i) test.Store32(kSortedTable + 4 * i, entries[i]);
  constexpr std::array<uint32_t, 15> edges = {
      0, 1, 2, 3, 4, 5, 6, 9971, 9972, 9973, 9974,
      0x7FFFFFFF, 0x80000000, 0xFFFFFFFF, 5000};
  for (uint32_t i = 0; i < edges.size() + 512; ++i) {
    auto ctx = test.Context(i);
    const uint32_t value = i < edges.size() ? edges[i]
        : i & 1 ? entries[random() % entries.size()] : uint32_t(random() % 10000);
    ctx.r3.u64 = (random() & 0xFFFFFFFF00000000ull) | value;
    test.Check("sub_8267A938", __imp__sub_8267A938, sub_8267A938, ctx,
               {{kSortedTable, entries.size() * 4}});
  }
}

void TestSlotLookup(NativeHelpersTest& test, std::mt19937_64& random) {
  constexpr std::array<uint32_t, 6> values = {0, 1, 0xFFFFFFFF, 0x80000000, 22, 0x7FFFFFFF};
  for (uint32_t i = 0; i < 384; ++i) {
    auto ctx = test.Context(i);
    ctx.r4.u64 = random();
    const uint32_t slot = uint32_t(ctx.r4.u8) * 4;
    const uint32_t output = i % 3 == 0 ? test.kObject + slot
        : i % 3 == 1 ? test.kObject : test.kData;
    ctx.r3.u64 = (random() & 0xFFFFFFFF00000000ull) | test.kObject;
    ctx.r5.u64 = (random() & 0xFFFFFFFF00000000ull) | output;
    ctx.r6.u64 = random();
    test.Fill(test.kObject, 1024, i);
    test.Fill(test.kData, 16, i);
    test.Store32(test.kObject + slot, i < 192 ? values[i % values.size()] : uint32_t(random()));
    test.Check("sub_82803DB8", __imp__sub_82803DB8, sub_82803DB8, ctx,
               {{test.kObject, 1024}, {test.kData, 16}});
  }
}

void TestIntervalSelection(NativeHelpersTest& test, std::mt19937_64& random) {
  constexpr std::array<uint32_t, 9> edges = {
      0, 1, 0xFFFFFFFF, 0x80000000, 0x7FFFFFFF, 0x80000001,
      0x7FFFFFFE, 158, 0xFFFFFF62};
  for (uint32_t i = 0; i < edges.size() * edges.size() * edges.size() + 512; ++i) {
    auto ctx = test.Context(i);
    if (i < edges.size() * edges.size() * edges.size()) {
      ctx.r3.u64 = (random() & 0xFFFFFFFF00000000ull) | edges[i / 81];
      ctx.r4.u64 = (random() & 0xFFFFFFFF00000000ull) | edges[(i / 9) % 9];
      ctx.r5.u64 = (random() & 0xFFFFFFFF00000000ull) | edges[i % 9];
    } else {
      ctx.r3.u64 = random();
      ctx.r4.u64 = random();
      ctx.r5.u64 = random();
    }
    test.Check("sub_828ABA20", __imp__sub_828ABA20, sub_828ABA20, ctx, {});
  }
}

void TestGlobalBit(NativeHelpersTest& test, std::mt19937_64& random) {
  constexpr std::array<uint32_t, 16> indices = {
      0, 1, 30, 31, 32, 33, 63, 64, 65, 95, 96, 127, 255,
      0x7FFFFFFF, 0x80000000, 0xFFFFFFFF};
  constexpr std::array<uint32_t, 5> masks = {0, 1, 0x80000000, 0xFFFFFFFF, 0x55555555};
  for (uint32_t i = 0; i < indices.size() * masks.size() + 256; ++i) {
    auto ctx = test.Context(i);
    test.Store32(kSelectedBit, i < indices.size() * masks.size()
        ? indices[i / masks.size()] : uint32_t(random()));
    test.Store32(kEnabledBits, i < indices.size() * masks.size()
        ? masks[i % masks.size()] : uint32_t(random()));
    test.Check("sub_82434A00", __imp__sub_82434A00, sub_82434A00, ctx,
               {{kSelectedBit, 4}, {kEnabledBits, 4}});
  }
}
}  // namespace

int main() {
  NativeHelpersTest test;
  std::mt19937_64 random(0x8278E8588267A938ull);
  TestCostLookup(test, random);
  TestSortedLookup(test, random);
  TestSlotLookup(test, random);
  TestIntervalSelection(test, random);
  TestGlobalBit(test, random);
  test.Report("native bits");
}
