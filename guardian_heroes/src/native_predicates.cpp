#include "native_runtime.h"

#include <bit>

namespace {
template <unsigned Width>
uint32_t ReadField(uint8_t* base, uint32_t address) {
  if constexpr (Width == 8) return GH_LOAD_U8(address);
  else if constexpr (Width == 16) return GH_LOAD_U16(address);
  else return GH_LOAD_U32(address);
}

template <unsigned Width, uint32_t Offset, unsigned Start, uint32_t Mask,
          uint32_t Stride = 0>
void ExtractField(GuestContext& ctx, uint8_t* base) {
  const uint32_t address = ctx.r3.u32 + Offset + ctx.r4.u32 * Stride;
  const uint32_t value = ReadField<Width>(base, address);
  ctx.r11.u64 = value;
  ctx.r3.u64 = (value >> Start) & Mask;
}

template <unsigned Width, uint32_t PointerOffset, uint32_t Offset,
          unsigned Start, uint32_t Mask>
void ExtractPointerField(GuestContext& ctx, uint8_t* base) {
  const uint32_t pointer = GH_LOAD_U32(ctx.r3.u32 + PointerOffset);
  const uint32_t value = ReadField<Width>(base, pointer + Offset);
  ctx.r11.u64 = pointer;
  ctx.r10.u64 = value;
  ctx.r3.u64 = (value >> Start) & Mask;
}

template <unsigned Width, uint32_t Offset, uint32_t Expected, bool CountInR11>
void FieldEquals(GuestContext& ctx, uint8_t* base) {
  const uint32_t value = ReadField<Width>(base, ctx.r3.u32 + Offset);
  const int64_t difference = int64_t(value) - Expected;
  const uint32_t leading = std::countl_zero(uint32_t(difference));
  if constexpr (CountInR11) ctx.r11.u64 = leading;
  else {
    ctx.r11.s64 = difference;
    ctx.r10.u64 = leading;
  }
  ctx.r3.u64 = value == Expected;
}

template <uint32_t Offset>
void SignedByte(GuestContext& ctx, uint8_t* base) {
  const uint8_t value = GH_LOAD_U8(ctx.r3.u32 + Offset);
  ctx.r11.u64 = value;
  ctx.r3.s64 = std::bit_cast<int8_t>(value);
}

template <uint32_t Offset, unsigned LeftShift>
void SignedIndexedField(GuestContext& ctx, uint8_t* base) {
  const uint32_t address = ctx.r3.u32 + ctx.r4.u32 * 24 + Offset;
  const uint32_t word = GH_LOAD_U32(address) << LeftShift;
  const int32_t value = std::bit_cast<int32_t>(word);
  ctx.r11.u64 = word;
  ctx.xer.ca = value < 0 && (word & 0x07FFFFFF) != 0;
  ctx.r3.s64 = value >> 27;
}
}  // namespace

GH_HOOK(sub_825AFF30) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 29, false>(ctx, base);
}

GH_HOOK(sub_825D8740) {
  GH_PROLOGUE();
  ExtractField<8, 10540, 0, 0xfu, 0>(ctx, base);
}

GH_HOOK(sub_825C0B58) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 18, false>(ctx, base);
}

GH_HOOK(sub_8268D198) {
  GH_PROLOGUE();
  ExtractField<32, 16, 2, 0x7u, 0>(ctx, base);
}

GH_HOOK(sub_825D75D0) {
  GH_PROLOGUE();
  ExtractField<32, 10568, 0, 0x7u, 0>(ctx, base);
}

GH_HOOK(sub_825D7638) {
  GH_PROLOGUE();
  ExtractField<32, 10556, 3, 0x1u, 0>(ctx, base);
}

GH_HOOK(sub_82481850) {
  GH_PROLOGUE();
  FieldEquals<16, 356, 3, false>(ctx, base);
}

GH_HOOK(sub_82448500) {
  GH_PROLOGUE();
  FieldEquals<8, 85, 0, false>(ctx, base);
}

GH_HOOK(sub_82586D08) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 13, false>(ctx, base);
}

GH_HOOK(sub_8269A510) {
  GH_PROLOGUE();
  ExtractPointerField<8, 16, 7, 2, 0x1u>(ctx, base);
}

GH_HOOK(sub_825D86E0) {
  GH_PROLOGUE();
  ExtractField<32, 10540, 0, 0xfu, 0>(ctx, base);
}

GH_HOOK(sub_825D8750) {
  GH_PROLOGUE();
  ExtractField<32, 10540, 28, 0xfu, 0>(ctx, base);
}

GH_HOOK(sub_825D7F88) {
  GH_PROLOGUE();
  ExtractField<32, 10548, 26, 0x7u, 0>(ctx, base);
}

GH_HOOK(sub_82583CD8) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 2, false>(ctx, base);
}

GH_HOOK(sub_825D8700) {
  GH_PROLOGUE();
  ExtractField<32, 10540, 8, 0xfu, 0>(ctx, base);
}

GH_HOOK(sub_825D87A0) {
  GH_PROLOGUE();
  ExtractField<16, 10544, 0, 0xfu, 0>(ctx, base);
}

GH_HOOK(sub_825D7758) {
  GH_PROLOGUE();
  ExtractField<32, 12152, 5, 0x7u, 0>(ctx, base);
}

GH_HOOK(sub_825D7878) {
  GH_PROLOGUE();
  ExtractField<32, 12152, 8, 0x1fu, 0>(ctx, base);
}

GH_HOOK(sub_825D7A60) {
  GH_PROLOGUE();
  ExtractField<32, 12156, 30, 0x1u, 0>(ctx, base);
}

GH_HOOK(sub_825D8E40) {
  GH_PROLOGUE();
  ExtractField<32, 12156, 23, 0x7fu, 0>(ctx, base);
}

GH_HOOK(sub_824962E8) {
  GH_PROLOGUE();
  SignedByte<127>(ctx, base);
}

GH_HOOK(sub_825D97A8) {
  GH_PROLOGUE();
  SignedIndexedField<1168, 0>(ctx, base);
}

GH_HOOK(sub_825C6230) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 3, false>(ctx, base);
}

GH_HOOK(sub_826A9080) {
  GH_PROLOGUE();
  ExtractField<32, 740, 30, 0x1u, 0>(ctx, base);
}

GH_HOOK(sub_826B8608) {
  GH_PROLOGUE();
  ExtractField<32, 396, 4, 0x1u, 0>(ctx, base);
}

GH_HOOK(sub_825D76C8) {
  GH_PROLOGUE();
  ExtractField<32, 12156, 31, 0x1u, 0>(ctx, base);
}

GH_HOOK(sub_82586D20) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 6, false>(ctx, base);
}

GH_HOOK(sub_82481868) {
  GH_PROLOGUE();
  FieldEquals<16, 356, 5, false>(ctx, base);
}

GH_HOOK(sub_825D8DC0) {
  GH_PROLOGUE();
  ExtractField<32, 10560, 2, 0x1u, 0>(ctx, base);
}

GH_HOOK(sub_825D9750) {
  GH_PROLOGUE();
  SignedIndexedField<1168, 5>(ctx, base);
}

GH_HOOK(sub_8269B548) {
  GH_PROLOGUE();
  ExtractPointerField<32, 16, 0, 5, 0xffffffu>(ctx, base);
}

GH_HOOK(sub_825D8710) {
  GH_PROLOGUE();
  ExtractField<32, 10540, 12, 0xfu, 0>(ctx, base);
}

GH_HOOK(sub_825D87C0) {
  GH_PROLOGUE();
  ExtractField<8, 10544, 0, 0xfu, 0>(ctx, base);
}

GH_HOOK(sub_825D9800) {
  GH_PROLOGUE();
  ExtractField<32, 1172, 2, 0x1u, 24>(ctx, base);
}

GH_HOOK(sub_825D7958) {
  GH_PROLOGUE();
  ExtractField<16, 12152, 0, 0x1fu, 0>(ctx, base);
}

GH_HOOK(sub_825D8B28) {
  GH_PROLOGUE();
  ExtractField<32, 10616, 0, 0x3u, 0>(ctx, base);
}

GH_HOOK(sub_824D7120) {
  GH_PROLOGUE();
  FieldEquals<16, 2504, 1, false>(ctx, base);
}

GH_HOOK(sub_8269ECE8) {
  GH_PROLOGUE();
  ExtractPointerField<32, 60, 0, 0, 0xffffu>(ctx, base);
}

GH_HOOK(sub_824A4B80) {
  GH_PROLOGUE();
  FieldEquals<16, 4, 3, false>(ctx, base);
}

GH_HOOK(sub_82590CE0) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 1, false>(ctx, base);
}

GH_HOOK(sub_825752E0) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 9, false>(ctx, base);
}

GH_HOOK(sub_82585EE0) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 8, false>(ctx, base);
}

GH_HOOK(sub_825D7D70) {
  GH_PROLOGUE();
  ExtractField<32, 10548, 4, 0x7u, 0>(ctx, base);
}

GH_HOOK(sub_825D7E88) {
  GH_PROLOGUE();
  ExtractField<32, 10548, 17, 0x7u, 0>(ctx, base);
}

GH_HOOK(sub_82590CF8) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 10, false>(ctx, base);
}

GH_HOOK(sub_825D7E50) {
  GH_PROLOGUE();
  ExtractField<32, 10548, 11, 0x7u, 0>(ctx, base);
}

GH_HOOK(sub_825D7F20) {
  GH_PROLOGUE();
  ExtractField<32, 10548, 23, 0x7u, 0>(ctx, base);
}

GH_HOOK(sub_825D8720) {
  GH_PROLOGUE();
  ExtractField<16, 10540, 0, 0xfu, 0>(ctx, base);
}

GH_HOOK(sub_825D8780) {
  GH_PROLOGUE();
  ExtractField<32, 10544, 8, 0xfu, 0>(ctx, base);
}

GH_HOOK(sub_825B69C0) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 21, false>(ctx, base);
}

GH_HOOK(sub_825D7EB8) {
  GH_PROLOGUE();
  ExtractField<32, 10548, 14, 0x7u, 0>(ctx, base);
}

GH_HOOK(sub_825D8B90) {
  GH_PROLOGUE();
  ExtractField<32, 10568, 21, 0x1u, 0>(ctx, base);
}

GH_HOOK(sub_825D8DF0) {
  GH_PROLOGUE();
  ExtractField<32, 10560, 5, 0x1u, 0>(ctx, base);
}

GH_HOOK(sub_825752C8) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 15, false>(ctx, base);
}

GH_HOOK(sub_825810D8) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 11, false>(ctx, base);
}

GH_HOOK(sub_825D8730) {
  GH_PROLOGUE();
  ExtractField<32, 10540, 20, 0xfu, 0>(ctx, base);
}

GH_HOOK(sub_825CBD68) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 31, false>(ctx, base);
}

GH_HOOK(sub_8257BA90) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 19, false>(ctx, base);
}

GH_HOOK(sub_825D7D38) {
  GH_PROLOGUE();
  ExtractField<32, 10548, 2, 0x1u, 0>(ctx, base);
}

GH_HOOK(sub_825D7CC8) {
  GH_PROLOGUE();
  ExtractField<32, 24264, 0, 0x1u, 0>(ctx, base);
}

GH_HOOK(sub_8269A500) {
  GH_PROLOGUE();
  ExtractPointerField<8, 16, 7, 1, 0x1u>(ctx, base);
}

GH_HOOK(sub_825D7600) {
  GH_PROLOGUE();
  ExtractField<32, 10568, 3, 0xffu, 0>(ctx, base);
}

GH_HOOK(sub_825C0B40) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 4, false>(ctx, base);
}

GH_HOOK(sub_825D87B0) {
  GH_PROLOGUE();
  ExtractField<32, 10544, 20, 0xfu, 0>(ctx, base);
}

GH_HOOK(sub_825D8BE8) {
  GH_PROLOGUE();
  ExtractField<32, 10556, 4, 0x1u, 0>(ctx, base);
}

GH_HOOK(sub_8269A580) {
  GH_PROLOGUE();
  ExtractPointerField<8, 16, 7, 0, 0x1u>(ctx, base);
}

GH_HOOK(sub_82674758) {
  GH_PROLOGUE();
  FieldEquals<32, 116, 0, true>(ctx, base);
}

GH_HOOK(sub_8269A520) {
  GH_PROLOGUE();
  ExtractPointerField<8, 16, 7, 3, 0x1u>(ctx, base);
}

GH_HOOK(sub_82418EB8) {
  GH_PROLOGUE();
  FieldEquals<32, 12, 0, false>(ctx, base);
}

GH_HOOK(sub_825BAAF8) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 28, false>(ctx, base);
}

GH_HOOK(sub_826B8BF0) {
  GH_PROLOGUE();
  FieldEquals<32, 32, 997, true>(ctx, base);
}

GH_HOOK(sub_825D7F58) {
  GH_PROLOGUE();
  ExtractField<32, 10548, 29, 0x7u, 0>(ctx, base);
}

GH_HOOK(sub_825D86F0) {
  GH_PROLOGUE();
  ExtractField<32, 10540, 4, 0xfu, 0>(ctx, base);
}

GH_HOOK(sub_825D8790) {
  GH_PROLOGUE();
  ExtractField<32, 10544, 12, 0xfu, 0>(ctx, base);
}

GH_HOOK(sub_825D78E8) {
  GH_PROLOGUE();
  ExtractField<32, 12152, 21, 0x7u, 0>(ctx, base);
}

GH_HOOK(sub_825D7E18) {
  GH_PROLOGUE();
  ExtractField<32, 10548, 8, 0x7u, 0>(ctx, base);
}

GH_HOOK(sub_825D7EE8) {
  GH_PROLOGUE();
  ExtractField<32, 10548, 20, 0x7u, 0>(ctx, base);
}

GH_HOOK(sub_825C0B70) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 7, false>(ctx, base);
}

GH_HOOK(sub_825D80A0) {
  GH_PROLOGUE();
  ExtractField<32, 10564, 0, 0x3fu, 0>(ctx, base);
}

GH_HOOK(sub_8268D130) {
  GH_PROLOGUE();
  ExtractField<32, 12, 4, 0xfffffffu, 0>(ctx, base);
}

GH_HOOK(sub_825D8820) {
  GH_PROLOGUE();
  ExtractField<32, 10572, 0, 0x1u, 0>(ctx, base);
}

GH_HOOK(sub_825D8D90) {
  GH_PROLOGUE();
  ExtractField<32, 10560, 3, 0x1u, 0>(ctx, base);
}

GH_HOOK(sub_825D91D8) {
  GH_PROLOGUE();
  ExtractField<32, 1164, 23, 0x3u, 24>(ctx, base);
}

GH_HOOK(sub_826B8088) {
  GH_PROLOGUE();
  ExtractField<32, 396, 31, 0x1u, 0>(ctx, base);
}

GH_HOOK(sub_825722C8) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 17, false>(ctx, base);
}

GH_HOOK(sub_82583CF0) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 5, false>(ctx, base);
}

GH_HOOK(sub_825D8760) {
  GH_PROLOGUE();
  ExtractField<32, 10544, 0, 0xfu, 0>(ctx, base);
}

GH_HOOK(sub_824962D8) {
  GH_PROLOGUE();
  SignedByte<126>(ctx, base);
}

GH_HOOK(sub_825B97E8) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 16, false>(ctx, base);
}

GH_HOOK(sub_8256DBC8) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 14, false>(ctx, base);
}

GH_HOOK(sub_825D8CF8) {
  GH_PROLOGUE();
  ExtractField<32, 12156, 20, 0x7u, 0>(ctx, base);
}

GH_HOOK(sub_825D77E8) {
  GH_PROLOGUE();
  ExtractField<32, 12152, 0, 0x1fu, 0>(ctx, base);
}

GH_HOOK(sub_825D79C8) {
  GH_PROLOGUE();
  ExtractField<8, 12152, 0, 0x1fu, 0>(ctx, base);
}

GH_HOOK(sub_825D96F8) {
  GH_PROLOGUE();
  ExtractField<32, 1172, 3, 0x3u, 24>(ctx, base);
}

GH_HOOK(sub_825D8770) {
  GH_PROLOGUE();
  ExtractField<32, 10544, 4, 0xfu, 0>(ctx, base);
}

GH_HOOK(sub_825D8B60) {
  GH_PROLOGUE();
  ExtractField<32, 10688, 0, 0x1u, 0>(ctx, base);
}

GH_HOOK(sub_825D8D60) {
  GH_PROLOGUE();
  ExtractField<32, 12156, 17, 0x7u, 0>(ctx, base);
}

GH_HOOK(sub_8258A048) {
  GH_PROLOGUE();
  FieldEquals<16, 8, 12, false>(ctx, base);
}

GH_HOOK(sub_825D7C28) {
  GH_PROLOGUE();
  ExtractField<32, 10680, 10, 0x1u, 0>(ctx, base);
}

GH_HOOK(sub_825D8248) {
  GH_PROLOGUE();
  ExtractField<32, 10568, 15, 0x1u, 0>(ctx, base);
}

GH_HOOK(sub_825D87D0) {
  GH_PROLOGUE();
  ExtractField<32, 10544, 28, 0xfu, 0>(ctx, base);
}

GH_HOOK(sub_825D7AF0) {
  GH_PROLOGUE();
  ExtractField<32, 10556, 0, 0x7u, 0>(ctx, base);
}

GH_HOOK(sub_825D7DE8) {
  GH_PROLOGUE();
  ExtractField<32, 10548, 7, 0x1u, 0>(ctx, base);
}
