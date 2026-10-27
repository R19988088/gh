#include "native_runtime.h"

#include <bit>

namespace {
uint32_t WordBit(uint32_t index) {
  return index & 32 ? 0 : 1u << (index & 31);
}

// The generated word masks duplicate the intermediate low word into r10's high half.
uint64_t PackedRenderState(GuestContext& ctx, uint32_t word) {
  const uint32_t special = word & 0x1010;
  ctx.cr0.compare<int32_t>(special, 0, ctx.xer);
  ctx.r9.u64 = (word & 0xFFFF) << 4;
  const uint32_t high = ((special | ctx.r9.u32) << 12) & 0xFFFF0000;
  const uint32_t first_mask = high & ~0x00100000u;
  ctx.r10.u64 = (uint64_t(first_mask) << 32) | (first_mask & ~0x10000000u);
  return ctx.r10.u64 | (word & 0xFFFF);
}

void MarkRenderState(GuestContext& ctx, uint8_t* base, uint32_t object) {
  ctx.r11.u64 = GH_LOAD_U64(object + 16) | 1024;
  GH_STORE_U64(object + 16, ctx.r11.u64);
  ctx.r11.u64 |= 4;
  GH_STORE_U64(object + 16, ctx.r11.u64);
  ctx.r11.u64 |= 2;
  GH_STORE_U64(object + 16, ctx.r11.u64);
  ctx.r11.u64 |= 1;
  GH_STORE_U64(object + 16, ctx.r11.u64);
}

void SetRenderField(GuestContext& ctx, uint8_t* base, uint32_t mask, unsigned shift) {
  const uint32_t object = ctx.r3.u32;
  const uint32_t old_word = GH_LOAD_U32(object + 12152);
  const uint32_t control = GH_LOAD_U32(object + 12156);
  const uint32_t word = (old_word & ~mask) | ((ctx.r4.u32 << shift) & mask);
  ctx.r10.u64 = word;
  ctx.r11.u64 = control;
  ctx.r9.u64 = control & 0x80000000u;
  ctx.cr0.compare<int32_t>(ctx.r9.s32, 0, ctx.xer);
  GH_STORE_U32(object + 12152, word);
  if (!(control & 0x80000000u)) return;
  ctx.r10.u64 = control & 0x40000000u;
  ctx.cr0.compare<int32_t>(ctx.r10.s32, 0, ctx.xer);
  ctx.r11.u64 = GH_LOAD_U32(object + 12152);
  if (!(control & 0x40000000u)) ctx.r11.u64 = PackedRenderState(ctx, ctx.r11.u32);
  GH_STORE_U32(object + 10552, ctx.r11.u32);
  GH_STORE_U32(object + 10584, ctx.r11.u32);
  GH_STORE_U32(object + 10588, ctx.r11.u32);
  GH_STORE_U32(object + 10592, ctx.r11.u32);
  MarkRenderState(ctx, base, object);
}

void SetRenderToggle(GuestContext& ctx, uint8_t* base, uint32_t offset, unsigned shift) {
  const uint32_t object = ctx.r3.u32;
  const uint32_t enabled = GH_LOAD_U32(object + 12704);
  const uint32_t old_word = GH_LOAD_U32(object + 10548);
  GH_STORE_U32(object + offset, ctx.r4.u32);
  ctx.xer.ca = enabled == 0;
  ctx.r10.u64 = (old_word & ~(1u << shift)) |
      (enabled ? (ctx.r4.u32 & 1) << shift : 0);
  GH_STORE_U32(object + 10548, ctx.r10.u32);
  ctx.r11.u64 = GH_LOAD_U64(object + 16) | 2048;
  GH_STORE_U64(object + 16, ctx.r11.u64);
  ctx.r11.u64 |= 131072;
  GH_STORE_U64(object + 16, ctx.r11.u64);
}

void TestGlobalWordBit(GuestContext& ctx, uint8_t* base, uint32_t address) {
  const uint32_t bit = WordBit(ctx.r3.u32);
  ctx.r10.u64 = 1;
  ctx.r9.u64 = bit;
  ctx.r11.u64 = GH_LOAD_U32(address);
  const uint32_t selected = bit & ctx.r11.u32;
  ctx.r8.u64 = selected;
  ctx.r7.u64 = uint64_t(selected) - 1;
  ctx.r3.u64 = selected != 0;
  ctx.xer.ca = selected != 0;
}
}  // namespace

// Set or clear one bit of the object's flag word.
GH_HOOK(sub_8246BCE0) {
  GH_PROLOGUE();
  const uint32_t flags = GH_LOAD_U32(ctx.r3.u32 + 1860);
  const uint32_t bit = WordBit(ctx.r4.u32);
  ctx.cr6.compare<uint32_t>(ctx.r5.u8, 0, ctx.xer);
  ctx.r11.u64 = 1;
  ctx.r10.u64 = flags;
  ctx.r9.u64 = bit;
  ctx.r8.u64 = ctx.r5.u8 ? flags | bit : flags & ~bit;
  GH_STORE_U32(ctx.r3.u32 + 1860, ctx.r8.u32);
}

GH_HOOK(sub_8242E8A8) {
  GH_PROLOGUE();
  TestGlobalWordBit(ctx, base, 0x829A084C);
}

// Update the three packed rendering fields and their dependent copies.
GH_HOOK(sub_825D7768) {
  GH_PROLOGUE();
  SetRenderField(ctx, base, 0x1F, 0);
}
GH_HOOK(sub_825D77F8) {
  GH_PROLOGUE();
  SetRenderField(ctx, base, 0x1F00, 8);
}
GH_HOOK(sub_825D76D8) {
  GH_PROLOGUE();
  SetRenderField(ctx, base, 0xE0, 5);
}
GH_HOOK(sub_825D7CD8) {
  GH_PROLOGUE();
  SetRenderToggle(ctx, base, 12180, 1);
}
GH_HOOK(sub_825D7D80) {
  GH_PROLOGUE();
  SetRenderToggle(ctx, base, 12184, 0);
}

GH_HOOK(sub_825D7648) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  const uint32_t control = (GH_LOAD_U32(object + 12156) & 0x7FFFFFFF) |
      ((ctx.r4.u32 & 1) << 31);
  ctx.r11.u64 = control;
  GH_STORE_U32(object + 12156, control);
  ctx.r10.u64 = control & 0x40000000;
  ctx.cr0.compare<int32_t>(ctx.r10.s32, 0, ctx.xer);
  ctx.r11.u64 = GH_LOAD_U32(object + 12152);
  if (!(control & 0x40000000)) ctx.r11.u64 = PackedRenderState(ctx, ctx.r11.u32);
  ctx.cr6.compare<int32_t>(ctx.r4.s32, 0, ctx.xer);
  if (!ctx.r4.u32) ctx.r11.u64 = 0x10001;
  GH_STORE_U32(object + 10588, ctx.r11.u32);
  GH_STORE_U32(object + 10552, ctx.r11.u32);
  GH_STORE_U32(object + 10584, ctx.r11.u32);
  GH_STORE_U32(object + 10592, ctx.r11.u32);
  MarkRenderState(ctx, base, object);
}

GH_HOOK(sub_825D87E0) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  const bool disabled = ctx.r4.u32 == 0;
  ctx.cr6.compare<uint32_t>(ctx.r4.u32, 0, ctx.xer);
  GH_STORE_U32(object + 10572, disabled ? 1024 : 1087);
  const uint32_t flags = GH_LOAD_U32(object + 10564);
  ctx.r10.u64 = (flags & ~0x10000u) | (disabled ? 0x10000u : 0);
  GH_STORE_U32(object + 10564, ctx.r10.u32);
  ctx.r11.u64 = GH_LOAD_U64(object + 16) | 32;
  GH_STORE_U64(object + 16, ctx.r11.u64);
  ctx.r11.u64 |= 128;
  GH_STORE_U64(object + 16, ctx.r11.u64);
}

GH_HOOK(sub_825D8B38) {
  GH_PROLOGUE();
  const uint32_t flags = GH_LOAD_U32(ctx.r3.u32 + 10688);
  ctx.r4.u64 = (ctx.r4.u64 & 0xFFFFFFFF00000001ull) | (flags & 0xFFFFFFFEu);
  ctx.r12.u64 = uint64_t(1) << 35;
  GH_STORE_U32(ctx.r3.u32 + 10688, ctx.r4.u32);
  ctx.r11.u64 = GH_LOAD_U64(ctx.r3.u32 + 32) | ctx.r12.u64;
  GH_STORE_U64(ctx.r3.u32 + 32, ctx.r11.u64);
}

GH_HOOK(sub_825D7610) {
  GH_PROLOGUE();
  const uint32_t flags = GH_LOAD_U32(ctx.r3.u32 + 10556);
  const uint32_t word = (flags & ~8u) | ((ctx.r4.u32 & 1) << 3);
  ctx.r11.u64 = word;
  GH_STORE_U32(ctx.r3.u32 + 10556, word);
  ctx.r11.u64 = GH_LOAD_U64(ctx.r3.u32 + 16) | 512;
  GH_STORE_U64(ctx.r3.u32 + 16, ctx.r11.u64);
  ctx.r11.u64 |= 262144;
  GH_STORE_U64(ctx.r3.u32 + 16, ctx.r11.u64);
}

GH_HOOK(sub_824349A8) {
  GH_PROLOGUE();
  const uint32_t index = GH_LOAD_U32(0x82911E60);
  ctx.r11.u64 = index;
  ctx.cr6.compare<int32_t>(std::bit_cast<int32_t>(index), 0, ctx.xer);
  if (ctx.cr6.lt) {
    ctx.r3.u64 = 0;
    return;
  }
  ctx.r10.s64 = -2103836672;
  ctx.r9.u64 = 1;
  ctx.r8.u64 = WordBit(index);
  ctx.r11.u64 = GH_LOAD_U32(0x829A21F4);
  const uint32_t selected = ctx.r8.u32 & ctx.r11.u32;
  ctx.r7.u64 = selected;
  ctx.r6.u64 = uint64_t(selected) - 1;
  ctx.r3.u64 = selected != 0;
  ctx.xer.ca = selected != 0;
}

GH_HOOK(sub_824349E0) {
  GH_PROLOGUE();
  TestGlobalWordBit(ctx, base, 0x829A21F8);
}

// Unsigned subtraction carry and the sign bit together select signed values above four.
GH_HOOK(sub_82838288) {
  GH_PROLOGUE();
  const uint64_t value = ctx.r4.u64;
  const uint32_t nonnegative = !(ctx.r4.u32 & 0x80000000u);
  ctx.r11.u64 = 4;
  ctx.r10.u64 = 4 - value;
  ctx.r9.u64 = ~(value ^ 4);
  ctx.r8.u64 = nonnegative;
  ctx.r7.u64 = nonnegative + (ctx.r4.u32 <= 4);
  ctx.r3.u64 = ctx.r7.u32 & 1;
  ctx.xer.ca = 0;
}

GH_HOOK(sub_824380B0) {
  GH_PROLOGUE();
  const uint8_t enabled = GH_LOAD_U8(0x829AACF8);
  const uint8_t value = GH_LOAD_U8(0x82911E9C);
  ctx.r11.s64 = -2103771136;
  ctx.r9.u64 = enabled;
  ctx.r10.u64 = value;
  ctx.r8.u64 = uint64_t(0) - enabled;
  ctx.r6.u64 = enabled ? UINT64_MAX : 0;
  ctx.r3.u64 = enabled ? value : 0;
  ctx.xer.ca = enabled == 0;
}

GH_HOOK(sub_8288B988) {
  GH_PROLOGUE();
  const uint32_t first = GH_LOAD_U32(0x82979830);
  const uint32_t second = GH_LOAD_U32(0x82A05A74);
  const uint32_t difference = (first | 1) - second;
  ctx.r10.u64 = second;
  ctx.r11.u64 = difference ? __builtin_clz(difference) : 32;
  ctx.r3.u64 = difference == 0;
}

GH_HOOK(sub_82410600) {
  GH_PROLOGUE();
  const uint32_t value = ctx.r3.u8;
  ctx.r10.s64 = -2103967744;
  ctx.r9.u64 = value;
  ctx.r8.u64 = value ? __builtin_clz(value) : 32;
  ctx.r11.u64 = GH_LOAD_U32(0x8297B190);
  ctx.r7.u64 = value == 0;
  GH_STORE_U8(ctx.r11.u32 + 162, uint8_t(value));
  ctx.r11.u64 = GH_LOAD_U32(0x8297B190);
  GH_STORE_U8(ctx.r11.u32 + 161, ctx.r7.u8);
}

GH_HOOK(sub_8243EEA0) {
  GH_PROLOGUE();
  const uint32_t index = 2 * uint32_t(ctx.r5.u8) + ctx.r4.u8;
  ctx.r11.u64 = 5 * index;
  ctx.r10.u64 = 10 * index;
  ctx.r3.s64 = int16_t(10 * index);
}

GH_HOOK(sub_82419C88) {
  GH_PROLOGUE();
  ctx.cr6.compare<int32_t>(ctx.r3.s32, 4, ctx.xer);
  if (!ctx.cr6.lt) return;
  ctx.r11.u64 = ctx.r3.u32 << 1;
  ctx.r10.s64 = -2103902208;
  ctx.r9.u64 = ctx.r3.u64 + ctx.r11.u64;
  ctx.r8.s64 = -2103898160;
  ctx.r7.u64 = ctx.r9.u32 << 2;
  GH_STORE_U8(ctx.r7.u32 + ctx.r8.u32, ctx.r4.u8);
}

GH_HOOK(sub_8241B800) {
  GH_PROLOGUE();
  ctx.r9.u64 = ctx.r3.u64 + uint32_t(ctx.r3.u32 << 2);
  ctx.r10.s64 = -2103897796;
  ctx.r11.u64 = ctx.r9.u32 << 2;
  ctx.r3.u64 = ctx.r11.u64 + ctx.r10.u64;
}

GH_HOOK(sub_82778978) {
  GH_PROLOGUE();
  const uint32_t index = ctx.r5.u32 << 2;
  const uint32_t factor = GH_LOAD_U32(0x823A6800 + index);
  const int64_t product = int64_t(std::bit_cast<int32_t>(factor)) * ctx.r4.s32;
  const int64_t rounded = product + 131072;
  const int32_t word = std::bit_cast<int32_t>(uint32_t(rounded));
  ctx.r11.s64 = -2110128128;
  ctx.r10.u64 = index;
  ctx.r9.s64 = -2110101504;
  ctx.r7.u64 = factor;
  ctx.r6.s64 = product;
  ctx.r5.s64 = rounded;
  ctx.r4.s64 = word >> 18;
  ctx.r3.s64 = ctx.r4.s16;
  ctx.xer.ca = word < 0 && (uint32_t(word) & 0x3FFFF) != 0;
}

GH_HOOK(sub_8282F660) {
  GH_PROLOGUE();
  const int32_t selector = ctx.r4.s32;
  ctx.cr6.compare<int32_t>(selector, selector == 0 ? 0 : selector == 1 ? 1 : 2, ctx.xer);
  ctx.r3.u64 = uint32_t(selector) < 3 ? GH_LOAD_U32(ctx.r3.u32 + 56 + uint32_t(selector) * 4) : 0;
}

// Count source bits through the first selected target bit, retaining every output update.
GH_HOOK(sub_82669DA0) {
  GH_PROLOGUE();
  const uint32_t source = ctx.r3.u32;
  const uint32_t target = ctx.r4.u32;
  ctx.r11.s64 = -1;
  ctx.r10.u64 = ctx.r3.u64 & ctx.r4.u64;
  GH_STORE_U16(ctx.r5.u32, uint16_t(0xFFFF));
  ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
  if (!(source & target)) return;
  const uint32_t last = std::countr_zero(target);
  ctx.r9.u64 = last;
  for (uint32_t bit = 0; bit <= last; ++bit) {
    const uint32_t mask = 1u << bit;
    if (source & mask) {
      ctx.r10.u64 = GH_LOAD_U16(ctx.r5.u32);
      ctx.r8.u64 = ctx.r10.u64 + 1;
      GH_STORE_U16(ctx.r5.u32, ctx.r8.u16);
    }
  }
  ctx.r11.u64 = 1u << last;
  ctx.r10.u64 = ctx.r11.u64;
  ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
}

// Allocate the first unused record in the fixed 7..126 slot range.
GH_HOOK(sub_828034A0) {
  GH_PROLOGUE();
  const uint64_t records = ctx.r3.u64;
  const uint32_t output = ctx.r6.u32;
  uint32_t slot = 7;
  ctx.r10.u64 = records;
  ctx.r3.u64 = 0;
  for (; slot < 127; ++slot) {
    ctx.r7.u64 = GH_LOAD_U32(uint32_t(records) + slot * 12);
    if (!ctx.r7.u32) break;
  }
  ctx.r11.u64 = slot;
  ctx.cr6.compare<int32_t>(slot, 127, ctx.xer);
  if (slot == 127) {
    ctx.r9.u64 = 127;
    ctx.r8.u64 = 126 * 12;
    ctx.r3.s64 = -2142240587;
    return;
  }
  GH_STORE_U32(output, slot);
  ctx.r8.u64 = 1;
  ctx.r7.u64 = slot * 3;
  ctx.r11.u64 = slot * 12;
  GH_STORE_U32(uint32_t(records) + ctx.r11.u32, uint32_t(1));
  const uint32_t second_slot = GH_LOAD_U32(output);
  ctx.r9.u64 = uint64_t(second_slot) + uint32_t(second_slot << 1);
  ctx.r11.u64 = ctx.r9.u32 << 2;
  ctx.r8.u64 = records + ctx.r11.u64;
  GH_STORE_U32(ctx.r8.u32 + 4, ctx.r4.u32);
  const uint32_t third_slot = GH_LOAD_U32(output);
  ctx.r9.u64 = third_slot << 1;
  ctx.r7.u64 = uint64_t(third_slot) + ctx.r9.u64;
  ctx.r11.u64 = ctx.r7.u32 << 2;
  ctx.r6.u64 = records + ctx.r11.u64;
  GH_STORE_U32(ctx.r6.u32 + 8, ctx.r5.u32);
}

// Decode the resource format to its two layout widths.
GH_HOOK(sub_825DAA38) {
  GH_PROLOGUE();
  const uint32_t format = ctx.r3.u32;
  uint32_t compared;
  uint32_t first = 1;
  uint32_t second = 1;
  if (format <= 44) {
    if (format == 44) { compared = 44; first = 4; }
    else if (format < 11) compared = 11;
    else if (format <= 12) { compared = 12; first = 2; }
    else if (format <= 17) compared = 17;
    else if (format <= 20) { compared = 20; first = second = 4; }
    else if (format == 39) { compared = 39; first = 4; }
    else { compared = 40; if (format == 40) first = 2; }
  } else {
    if (format == 49) { compared = 49; first = second = 4; }
    else if (format <= 50) compared = 50;
    else if (format <= 53) { compared = 53; first = second = 4; }
    else if (format <= 57) compared = 57;
    else { compared = 61; if (format <= 61) first = second = 4; }
  }
  ctx.cr6.compare<uint32_t>(format, compared, ctx.xer);
  ctx.r11.u64 = first;
  if (first != second) ctx.r10.u64 = second;
  GH_STORE_U32(ctx.r4.u32, first);
  GH_STORE_U32(ctx.r5.u32, second);
}
