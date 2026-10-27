#include "native_helpers_test.h"

#include <array>
#include <random>

REX_EXTERN(sub_824AB4E0);
REX_EXTERN(__imp__sub_824AB4E0);
REX_EXTERN(sub_824A8AD8);
REX_EXTERN(__imp__sub_824A8AD8);
REX_EXTERN(sub_828192B0);
REX_EXTERN(__imp__sub_828192B0);
REX_EXTERN(sub_8268D158);
REX_EXTERN(__imp__sub_8268D158);
REX_EXTERN(sub_824A6CE0);
REX_EXTERN(__imp__sub_824A6CE0);

namespace {
constexpr uint32_t kObject = NativeHelpersTest::kObject;
constexpr uint32_t kData = NativeHelpersTest::kData;
constexpr uint64_t kHigh = 0x8765432100000000ull;

void ClearRange(NativeHelpersTest& test, uint32_t begin, uint32_t end, uint32_t seed) {
  auto ctx = test.Context(seed);
  ctx.r3.u64 = kHigh | kObject;
  test.Fill(kObject, 32, seed);
  test.Store32(kObject, begin);
  test.Store32(kObject + 4, end);
  test.Check("clear range", __imp__sub_824AB4E0, sub_824AB4E0, ctx, {{kObject, 32}});
}

void Index(NativeHelpersTest& test, int32_t distance, uint32_t selected, uint32_t seed) {
  auto ctx = test.Context(seed);
  ctx.r3.u64 = kHigh | kObject;
  ctx.r4.u64 = kHigh | selected;
  test.Fill(kObject, 1800, seed);
  test.Fill(kData, 256, seed + 1);
  test.Store32(kObject + 1680, kData);
  test.Store32(kObject + 1684, kData + uint32_t(distance));
  test.Check("range index", __imp__sub_824A6CE0, sub_824A6CE0, ctx,
             {{kObject, 1800}, {kData, 256}});
}

void Payload(NativeHelpersTest& test, int32_t distance, uint32_t selected,
             uint32_t index, uint32_t active, uint32_t length, uint32_t seed) {
  auto ctx = test.Context(seed);
  ctx.r3.u64 = kHigh | kObject;
  test.Fill(kObject, 1800, seed);
  test.Fill(kData, 8192, seed + 1);
  test.Store32(kObject + 1680, kData);
  test.Store32(kObject + 1684, kData + uint32_t(distance));
  test.Store32(kObject + 1180, selected);
  test.Store32(kObject + 1664, kData + 4096);
  // Selected values are chosen so that every taken table load stays in this arena.
  if (selected != 0xFFFFFFFFu) test.Store32(kData + (selected << 2), index);
  if (!(index & 0x80000000u)) {
    const uint32_t record = kData + 4096 + index * 80u;
    test.Store32(record, 0xCAFEBABE);
    test.Store32(record + 16, active);
    test.Store32(record + 20, length);
  }
  test.Check("selected payload", __imp__sub_824A8AD8, sub_824A8AD8, ctx,
             {{kObject, 1800}, {kData, 8192}});
}

void Layout(NativeHelpersTest& test, uint64_t selector, uint32_t destination, uint32_t seed) {
  auto ctx = test.Context(seed);
  ctx.r3.u64 = kHigh | kObject;
  ctx.r4.u64 = kHigh | destination;
  ctx.r5.u64 = selector;
  test.Fill(kObject, 22000, seed);
  test.Fill(kData, 64, seed + 1);
  test.Check("five-field layout", __imp__sub_828192B0, sub_828192B0, ctx,
             {{kObject, 22000}, {kData, 64}});
}
}  // namespace

int main() {
  NativeHelpersTest test;
  uint32_t seed = 0;
  constexpr std::array<uint32_t, 5> bounds = {0, kData, kData + 8, 0x80000000, 0xFFFFFFFF};
  for (auto begin : bounds) {
    for (auto end : bounds) ClearRange(test, begin, end, seed++);
  }

  constexpr std::array<int32_t, 10> distances = {-9, -4, -1, 0, 1, 3, 4, 8, 12, 36};
  constexpr std::array<uint32_t, 8> selections = {
      0, 1, 2, 3, 8, 0x40000000, 0x80000000, 0xFFFFFFFF};
  for (auto distance : distances) {
    for (auto selected : selections) {
      Index(test, distance, selected, seed++);
      for (auto index : {0u, 1u, 2u, 0x40000000u, 0x80000000u, 0xFFFFFFFFu}) {
        for (auto active : {0u, 1u}) {
          for (auto length : {0u, 7u, 8u, 0xFFFFFFFFu}) {
            Payload(test, distance, selected, index, active, length, seed++);
          }
        }
      }
    }
  }

  for (uint64_t selector : std::array<uint64_t, 5>{
           0, kHigh, 1, kHigh | 0x80000000u, kHigh | 0xFFFFFFFFu}) {
    for (auto destination : {kData, kObject + 7092, kObject + 7096,
                             kObject + 7116, kObject + 7120, kObject + 21136}) {
      Layout(test, selector, destination, seed++);
    }
  }

  std::mt19937 random(0xA66A15);
  for (unsigned i = 0; i < 256; ++i) {
    const uint32_t flags = random();
    auto ctx = test.Context(seed++);
    ctx.r3.u64 = kHigh | kObject;
    test.Fill(kObject, 32, seed);
    test.Store32(kObject + 16, flags);
    test.Check("two-bit kind", __imp__sub_8268D158, sub_8268D158, ctx, {{kObject, 32}});
    const uint32_t count = random() % 32;
    const uint32_t selected = random() % 40;
    Index(test, count * 4, selected, seed++);
    Payload(test, count * 4, selected, random() % 32, random() & 1,
            random() % 16, seed++);
    ClearRange(test, random(), random(), seed++);
    Layout(test, (uint64_t(random()) << 32) | (random() & 1),
           (random() & 1) ? kData : kObject + 7088 + (random() % 16) * 4, seed++);
  }
  // All four kind values are covered independently of the random samples.
  for (uint32_t kind = 0; kind < 4; ++kind) {
    auto ctx = test.Context(seed++);
    ctx.r3.u64 = kHigh | kObject;
    test.Fill(kObject, 32, seed);
    test.Store32(kObject + 16, 0xFFFFFFFCu | kind);
    test.Check("two-bit kind", __imp__sub_8268D158, sub_8268D158, ctx, {{kObject, 32}});
  }
  test.Report("native array helpers");
}
