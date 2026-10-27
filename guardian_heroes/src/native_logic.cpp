#include "native_runtime.h"

#include <bit>
#include <algorithm>
#include <cstring>

#include "native_memory.h"

#if defined(__clang__)
#pragma STDC FENV_ACCESS ON
#endif

GH_EXTERN(__imp__sub_82498748);
GH_EXTERN(__imp__sub_828885C0);
GH_EXTERN(__imp__sub_82887388);
GH_EXTERN(__imp__sub_82886F00);

// memset: collapse the store loops while retaining the guest's scratch state.
GH_HOOK(sub_828885C0) {
  GH_PROLOGUE();
  const uint32_t destination = ctx.r3.u32;
  const uint32_t size = ctx.r5.u32;
  if (!NativeMemoryRange(destination, size)) {
    __imp__sub_828885C0(ctx, base);
    return;
  }
  const uint32_t prefix = std::min(size, (0u - destination) & 3u);
  const uint32_t remaining = size - prefix;
  const uint32_t tail = remaining & 3;
  if (size) std::memset(GH_RAW(destination), ctx.r4.u8, size);
  ctx.r4.u64 = (ctx.r4.u64 & 0xFFFFFFFF00000000ull) |
               (uint32_t(ctx.r4.u8) * 0x01010101u);
  ctx.r5.u64 -= prefix;
  ctx.r6.u64 = ctx.r3.u64 + size - tail;
  ctx.r0.u64 = tail;
  ctx.cr0.compare<int32_t>(tail, 0, ctx.xer);
  // The unrolled three-byte tail returns before its last CTR decrement.
  ctx.ctr.u64 = tail == 3 ? 1 : 0;
}

// Small forward memcpy. Overlapping ranges retain the original copy order.
GH_HOOK(sub_82887388) {
  GH_PROLOGUE();
  const uint32_t destination = ctx.r3.u32;
  const uint32_t source = ctx.r4.u32;
  const uint32_t size = ctx.r5.u32;
  if (!NativeCopyRange(destination, source, size)) {
    __imp__sub_82887388(ctx, base);
    return;
  }
  const uint32_t prefix = std::min(size, (0u - destination) & 3u);
  const uint32_t remaining = size - prefix;
  const uint32_t tail = remaining & 3;
  const uint32_t words = remaining & ~3u;
  if (words) {
    const uint32_t last_word = source + prefix + words - 4;
    uint32_t word;
    std::memcpy(&word, GH_RAW(last_word), sizeof(word));
    ctx.r7.u64 = __builtin_bswap32(word);
    if ((source + prefix) & 3) {
      ctx.r8.u64 = GH_LOAD_U8(last_word + 2);
      ctx.r9.u64 = GH_LOAD_U8(last_word + 1);
      ctx.r10.u64 = GH_LOAD_U8(last_word);
    }
  }
  ctx.r0.u64 = tail ? GH_LOAD_U8(source + size - 1) : 0;
  if (size) std::memcpy(GH_RAW(destination), GH_RAW(source), size);
  ctx.r4.u64 += size;
  ctx.r5.u64 -= prefix;
  ctx.r6.u64 = ctx.r3.u64 + size;
  ctx.cr0.compare<int32_t>(tail, 0, ctx.xer);
  ctx.ctr.u64 = 0;
}

// Large memcpy's aligned path; other alignment branches stay in the original.
GH_HOOK(sub_82886F00) {
  GH_PROLOGUE();
  const uint32_t destination = ctx.r3.u32;
  const uint32_t source = ctx.r4.u32;
  const uint32_t size = ctx.r5.u32;
  if ((destination & 127) || (source & 7) || source < 8 || size < 128 ||
      !NativeCopyRange(destination, source, size)) {
    __imp__sub_82886F00(ctx, base);
    return;
  }
  GH_STORE_U64(ctx.r1.u32 - 8, ctx.r3.u64);
  const uint32_t blocks = size & ~127u;
  const uint32_t tail_words = size & 120u;
  const uint32_t tail = size & 7u;
  ctx.xer.ca = 1;
  ctx.cr7.compare<uint32_t>(size, 128, ctx.xer);
  ctx.r9.u64 = 8 + 128 * ((((size + 127) >> 7) - 1) % 8 + 1);
  ctx.r11.u64 = ctx.r4.u64 - 8 + ctx.r5.u64 - ctx.r9.u64;
  ctx.r12.u64 = ctx.r3.u64 - 8 + ctx.r5.u64;
  ctx.r10.u64 = 8;
  ctx.r8.u64 = GH_LOAD_U64(source + blocks - 16);
  ctx.cr0.compare<uint32_t>(source + blocks - 8, ctx.r11.u32, ctx.xer);
  if ((size & 127) && !ctx.cr0.lt) ctx.r8.s64 = -1;
  ctx.r4.u64 -= 8;
  ctx.r4.u32 = source + blocks + tail_words - 8;
  ctx.r7.u64 = tail_words ? GH_LOAD_U64(source + blocks + tail_words - 8) : 0;
  ctx.r6.u64 = tail;
  ctx.cr6.compare<uint32_t>(tail, 0, ctx.xer);
  ctx.cr1.compare<uint32_t>(tail, 4, ctx.xer);
  if (tail == 4) {
    ctx.r6.u64 = 0;
    ctx.r5.u64 = GH_LOAD_U32(source + size - 4);
    ctx.cr0.compare<uint32_t>(0, 0, ctx.xer);
  } else if (tail) {
    ctx.r4.u64 += 7;
    ctx.r4.u32 = source + size - 1;
    ctx.r7.u64 = GH_LOAD_U8(source + size - 1);
  }
  std::memcpy(GH_RAW(destination), GH_RAW(source), size);
  ctx.ctr.u64 = 0;
  ctx.r3.u64 = GH_LOAD_U64(ctx.r1.u32 - 8);
}

// Called from coordinate calculations throughout character and effect drawing.
// Keep every single-precision rounding step and the guest-visible scratch state.
GH_HOOK(sub_82498748) {
  GH_PROLOGUE();
  const uint32_t multiplier_bits = GH_LOAD_U32(ctx.r3.u32 + 28);
  const uint32_t scale_bits = GH_LOAD_U32(0x8236CC10);
  // NaN payload selection depends on the original compiler's operand ordering.
  if ((multiplier_bits & 0x7F800000) == 0x7F800000 ||
      (scale_bits & 0x7F800000) == 0x7F800000) {
    __imp__sub_82498748(ctx, base);
    return;
  }
  const uint32_t sum = GH_LOAD_U32(ctx.r3.u32 + 24) + ctx.r5.u32;
  ctx.cr0.compare<int32_t>(std::bit_cast<int32_t>(sum), 0, ctx.xer);
  const int64_t divisor = sum ? std::bit_cast<int32_t>(sum) : 1;

  ctx.fpscr.disableFlushMode();
  const double multiplier = std::bit_cast<float>(multiplier_bits);
  const int64_t value = ctx.r4.s32;
  // The original leaves the sign-extended input in its stack scratch slot.
  GH_STORE_U64(ctx.r1.u32 - 16, static_cast<uint64_t>(value));
  const double scale = std::bit_cast<float>(scale_bits);
  const double rounded_divisor = float(double(divisor));
  const double rounded_value = float(double(value));
  const double denominator = float(rounded_divisor * scale);
  const double scaled_value = float(rounded_value * scale);
  const double numerator = float(scaled_value * multiplier);
  const double result = float(numerator / denominator);

  ctx.r9.s64 = -2110324736;
  ctx.r10.s64 = value;
  ctx.r11.s64 = divisor;
  ctx.f0.f64 = scale;
  ctx.f1.f64 = result;
  ctx.f4.f64 = denominator;
  ctx.f5.f64 = numerator;
  ctx.f6.f64 = rounded_divisor;
  ctx.f7.f64 = scaled_value;
  ctx.f8.f64 = double(divisor);
  ctx.f9.f64 = rounded_value;
  ctx.f10.f64 = double(value);
  ctx.f11.u64 = static_cast<uint64_t>(value);
  ctx.f12.u64 = static_cast<uint64_t>(divisor);
  ctx.f13.f64 = multiplier;
}
