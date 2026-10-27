#include "native_runtime.h"

#include <bit>

// Sum two table costs for small absolute indices, or use the fallback cost.
GH_HOOK(sub_8278E858) {
  GH_PROLOGUE();
  const uint64_t sign4 = ctx.r4.s32 < 0 ? UINT64_MAX : 0;
  const uint64_t sign5 = ctx.r5.s32 < 0 ? UINT64_MAX : 0;
  const uint64_t xor4 = ctx.r4.u64 ^ sign4;
  const uint64_t xor5 = ctx.r5.u64 ^ sign5;
  const uint64_t index4 = xor4 - sign4;
  const uint64_t index5 = xor5 - sign5;
  ctx.xer.ca = ctx.r5.s32 < 0 && (ctx.r5.u32 & 0x7FFFFFFF) != 0;
  ctx.r9.u64 = xor4;
  ctx.r8.u64 = xor5;
  ctx.r11.u64 = index4;
  ctx.r10.u64 = index5;
  ctx.cr6.compare<int32_t>(std::bit_cast<int32_t>(uint32_t(index4)), 158, ctx.xer);
  bool fallback = ctx.cr6.gt;
  if (!fallback) {
    ctx.cr6.compare<int32_t>(std::bit_cast<int32_t>(uint32_t(index5)), 158, ctx.xer);
    fallback = ctx.cr6.gt;
  }
  if (fallback) {
    ctx.r11.u64 = GH_LOAD_U32(ctx.r6.u32 + 20);
    ctx.r3.u64 = uint32_t(ctx.r11.u32 << 1);
    return;
  }
  ctx.r9.s64 = -2104164352;
  ctx.r8.u64 = uint32_t(index4) << 2;
  ctx.r5.u64 = uint32_t(index5) << 2;
  ctx.r7.s64 = -2104194264;
  ctx.r3.u64 = GH_LOAD_U32(ctx.r5.u32 + ctx.r7.u32);
  ctx.r4.u64 = GH_LOAD_U32(ctx.r8.u32 + ctx.r7.u32);
  ctx.r10.u64 = uint32_t(ctx.r3.u32 << 2);
  ctx.r11.u64 = uint32_t(ctx.r4.u32 << 2);
  ctx.r10.u64 = GH_LOAD_U32(ctx.r10.u32 + ctx.r6.u32);
  ctx.r11.u64 = GH_LOAD_U32(ctx.r11.u32 + ctx.r6.u32);
  ctx.r3.u64 = ctx.r10.u64 + ctx.r11.u64;
}

// Binary search the fixed sorted table and round up to the next entry.
GH_HOOK(sub_8267A938) {
  GH_PROLOGUE();
  const uint32_t requested = ctx.r3.u32;
  ctx.cr6.compare<uint32_t>(requested, 1, ctx.xer);
  if (requested <= 1) {
    ctx.r3.u64 = 1;
    return;
  }
  ctx.cr6.compare<uint32_t>(requested, 9973, ctx.xer);
  if (requested >= 9973) {
    ctx.r3.u64 = 9973;
    return;
  }
  int32_t low = 0;
  int32_t high = 1229;
  ctx.r7.s64 = -2110148120;
  for (;;) {
    const int32_t middle = (low + high) >> 1;
    const uint32_t offset = uint32_t(middle) << 2;
    const uint32_t entry = GH_LOAD_U32(ctx.r7.u32 + offset);
    ctx.xer.ca = 0;
    ctx.r10.s64 = middle;
    ctx.r6.u64 = offset;
    ctx.r11.u64 = entry;
    ctx.cr6.compare<uint32_t>(entry, requested, ctx.xer);
    if (entry == requested) {
      ctx.r8.s64 = high;
      ctx.r9.s64 = low;
      ctx.r3.u64 = entry;
      return;
    }
    if (entry > requested) high = middle - 1;
    else low = middle + 1;
    ctx.cr6.compare<int32_t>(low, high, ctx.xer);
    if (low > high) {
      ctx.r8.s64 = high;
      ctx.r9.s64 = low;
      ctx.r11.u64 = uint32_t(low) << 2;
      ctx.r3.u64 = GH_LOAD_U32(ctx.r7.u32 + ctx.r11.u32);
      return;
    }
  }
}

// Read the low-byte slot and report whether it contains a nonzero value.
GH_HOOK(sub_82803DB8) {
  GH_PROLOGUE();
  const uint32_t slot = (ctx.r4.u32 & 0xFF) * 4;
  const uint32_t value = GH_LOAD_U32(ctx.r3.u32 + slot);
  ctx.r10.s64 = -2142240768;
  ctx.r8.u64 = ctx.r10.u64 | 22;
  ctx.r11.u64 = value;
  ctx.r9.u64 = value;
  ctx.r7.u64 = uint64_t(value) - 1;
  GH_STORE_U32(ctx.r5.u32, value);
  ctx.xer.ca = value != 0;
  ctx.r5.u64 = value ? 0 : UINT64_MAX;
  ctx.r3.u64 = value ? 0 : ctx.r8.u64;
}

// Select within the interval using the guest's wrapping 32-bit sign tests.
GH_HOOK(sub_828ABA20) {
  GH_PROLOGUE();
  const uint64_t lower = ctx.r3.u64;
  const uint64_t value = ctx.r4.u64;
  const uint64_t upper = ctx.r5.u64;
  const uint64_t from_lower = value - lower;
  const uint64_t from_upper = value - upper;
  const uint64_t difference = from_upper ^ from_lower;
  const bool below = (uint32_t(from_lower) & 0x80000000u) != 0;
  const bool between = (uint32_t(difference) & 0x80000000u) != 0;
  const uint64_t selected_lower = below ? lower : 0;
  const uint64_t selected_upper = !below && !between ? upper : 0;
  ctx.xer.ca = below && (uint32_t(from_lower) & 0x7FFFFFFF) != 0;
  ctx.r9.u64 = from_upper;
  ctx.r8.u64 = difference;
  ctx.r10.u64 = below ? UINT64_MAX : 0;
  ctx.r7.u64 = between ? value : 0;
  ctx.r6.u64 = below || between ? UINT64_MAX : 0;
  ctx.r4.u64 = selected_lower;
  ctx.r11.u64 = selected_lower | selected_upper;
  ctx.r3.u64 = ctx.r11.u64 | ctx.r7.u64;
}

// Test the selected global bit; PPC word shifts use only six index bits.
GH_HOOK(sub_82434A00) {
  GH_PROLOGUE();
  const uint32_t index = GH_LOAD_U32(0x82911E60);
  ctx.r11.u64 = index;
  ctx.cr6.compare<int32_t>(std::bit_cast<int32_t>(index), 0, ctx.xer);
  if (ctx.cr6.lt) {
    ctx.r3.u64 = 0;
    return;
  }
  const uint32_t bit = index & 32 ? 0 : 1u << (index & 31);
  ctx.r10.s64 = -2103836672;
  ctx.r9.u64 = 1;
  ctx.r8.u64 = bit;
  ctx.r11.u64 = GH_LOAD_U32(0x829A21F8);
  const uint32_t selected = bit & ctx.r11.u32;
  ctx.r7.u64 = selected;
  ctx.r6.u64 = uint64_t(selected) - 1;
  ctx.r3.u64 = selected != 0;
  ctx.xer.ca = selected != 0;
}
