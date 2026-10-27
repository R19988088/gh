#include "native_runtime.h"

#include <bit>
#include <initializer_list>

namespace {
template <unsigned Bits>
uint64_t ReadValue(uint8_t* base, uint32_t address) {
  if constexpr (Bits == 8) return GH_LOAD_U8(address);
  else if constexpr (Bits == 16) return GH_LOAD_U16(address);
  else if constexpr (Bits == 32) return GH_LOAD_U32(address);
  else return GH_LOAD_U64(address);
}

template <uint32_t Offset>
void RelativeTable(GuestContext& ctx, uint8_t* base) {
  const uint32_t holder = GH_LOAD_U32(ctx.r3.u32);
  ctx.r11.u64 = holder;
  ctx.cr6.compare<uint32_t>(holder, 0, ctx.xer);
  if (holder == 0) {
    ctx.r3.u64 = 0;
    return;
  }
  const uint32_t table = GH_LOAD_U32(holder);
  const uint32_t relative = GH_LOAD_U32(table + Offset);
  ctx.r11.u64 = table;
  ctx.r10.u64 = relative;
  ctx.r3.u64 = uint64_t(table) + relative;
}

template <unsigned Bits, uint32_t Offset, bool ThreadLocal = false>
void Field(GuestContext& ctx, uint8_t* base) {
  ctx.r3.u64 = ReadValue<Bits>(base, (ThreadLocal ? ctx.r13.u32 : ctx.r3.u32) + Offset);
}

template <int Expected, bool RequireFlag = false, bool SelectByFlag = false>
void StateEquals(GuestContext& ctx, uint8_t* base) {
  const uint32_t object = ctx.r3.u32;
  const uint8_t flag = GH_LOAD_U8(object + 1716);
  ctx.r11.u64 = flag;
  ctx.cr6.compare<uint32_t>(flag, 0, ctx.xer);
  if constexpr (RequireFlag) {
    if (flag == 0) {
      ctx.r3.u64 = 0;
      return;
    }
  }
  const uint32_t state = GH_LOAD_U32(object + 1180);
  const int expected = SelectByFlag ? (flag ? 11 : 10) : Expected;
  ctx.r11.u64 = uint64_t(state) - expected;
  ctx.r10.u64 = std::countl_zero(ctx.r11.u32);
  ctx.r3.u64 = state == uint32_t(expected);
}

template <unsigned Shift>
void EnabledFlag(GuestContext& ctx, uint8_t* base) {
  const uint32_t object = ctx.r3.u32;
  ctx.r11.u64 = GH_LOAD_U32(object + 100);
  ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
  if (ctx.r11.u32 == 0) {
    ctx.r11.u64 = GH_LOAD_U32(object + 396) & 0x80000000u;
    ctx.cr0.compare<int32_t>(ctx.r11.s32, 0, ctx.xer);
    if (ctx.r11.u32 == 0) {
      ctx.r3.u64 = 0;
      return;
    }
  }
  ctx.r11.u64 = GH_LOAD_U32(object + 396);
  ctx.r3.u64 = (ctx.r11.u32 >> Shift) & 1;
}

template <unsigned Shift, uint32_t Mask, unsigned Bits = 32>
void IndexedBits(GuestContext& ctx, uint8_t* base) {
  const uint32_t address = ctx.r3.u32 + (ctx.r4.u32 + 48) * 24;
  ctx.r11.u64 = ReadValue<Bits>(base, address);
  ctx.r3.u64 = (ctx.r11.u32 >> Shift) & Mask;
}

template <unsigned FlagBit, unsigned ValueShift, uint32_t Mask>
void IndexedKind(GuestContext& ctx, uint8_t* base) {
  const uint32_t address = ctx.r3.u32 + (ctx.r4.u32 + 48) * 24;
  const uint32_t flags = GH_LOAD_U32(address + 16);
  const uint32_t value = GH_LOAD_U32(address + 12);
  const bool special = (flags & (1u << FlagBit)) != 0;
  ctx.r9.u64 = value;
  ctx.r11.u64 = special ? UINT64_MAX : 0;
  ctx.r10.u64 = special ? 4 : ((value >> ValueShift) & Mask & ~4u);
  ctx.r3.u64 = ctx.r10.u32 & 7;
}

template <uint32_t Offset, uint32_t FieldOffset>
void NullableField(GuestContext& ctx, uint8_t* base) {
  const uint32_t pointer = GH_LOAD_U32(ctx.r3.u32 + Offset);
  ctx.r11.u64 = pointer;
  ctx.cr6.compare<uint32_t>(pointer, 0, ctx.xer);
  ctx.r3.u64 = pointer ? GH_LOAD_U32(pointer + FieldOffset) : 0;
}

template <int Lower, int Upper, int Delta>
void ChangeCase(GuestContext& ctx) {
  const int32_t character = ctx.r3.s32;
  ctx.cr6.compare<int32_t>(character, Lower, ctx.xer);
  if (character < Lower) return;
  ctx.cr6.compare<int32_t>(character, Upper, ctx.xer);
  if (character <= Upper) ctx.r3.u64 += Delta;
}
}  // namespace

GH_HOOK(sub_82498FE0) {
  GH_PROLOGUE();
  RelativeTable<4896>(ctx, base);
}

GH_HOOK(sub_82499378) {
  GH_PROLOGUE();
  RelativeTable<800>(ctx, base);
}

GH_HOOK(sub_82499918) {
  GH_PROLOGUE();
  RelativeTable<2080>(ctx, base);
}

GH_HOOK(sub_82499350) {
  GH_PROLOGUE();
  RelativeTable<768>(ctx, base);
}

GH_HOOK(sub_824998F0) {
  GH_PROLOGUE();
  RelativeTable<2048>(ctx, base);
}

GH_HOOK(sub_82499008) {
  GH_PROLOGUE();
  RelativeTable<4928>(ctx, base);
}

GH_HOOK(sub_824993F0) {
  GH_PROLOGUE();
  RelativeTable<928>(ctx, base);
}

GH_HOOK(sub_82499990) {
  GH_PROLOGUE();
  RelativeTable<2176>(ctx, base);
}

GH_HOOK(sub_82499648) {
  GH_PROLOGUE();
  RelativeTable<1504>(ctx, base);
}

GH_HOOK(sub_82499260) {
  GH_PROLOGUE();
  RelativeTable<576>(ctx, base);
}

GH_HOOK(sub_82499800) {
  GH_PROLOGUE();
  RelativeTable<1856>(ctx, base);
}

GH_HOOK(sub_82499170) {
  GH_PROLOGUE();
  RelativeTable<352>(ctx, base);
}

GH_HOOK(sub_824996E8) {
  GH_PROLOGUE();
  RelativeTable<1632>(ctx, base);
}

GH_HOOK(sub_82499D78) {
  GH_PROLOGUE();
  RelativeTable<3008>(ctx, base);
}

GH_HOOK(sub_82499120) {
  GH_PROLOGUE();
  RelativeTable<288>(ctx, base);
}

GH_HOOK(sub_82499698) {
  GH_PROLOGUE();
  RelativeTable<1568>(ctx, base);
}

GH_HOOK(sub_82499D28) {
  GH_PROLOGUE();
  RelativeTable<2944>(ctx, base);
}

GH_HOOK(sub_82499440) {
  GH_PROLOGUE();
  RelativeTable<992>(ctx, base);
}

GH_HOOK(sub_824999E0) {
  GH_PROLOGUE();
  RelativeTable<2240>(ctx, base);
}

GH_HOOK(sub_82499580) {
  GH_PROLOGUE();
  RelativeTable<1344>(ctx, base);
}

GH_HOOK(sub_82499BC0) {
  GH_PROLOGUE();
  RelativeTable<2624>(ctx, base);
}

GH_HOOK(sub_82499B20) {
  GH_PROLOGUE();
  RelativeTable<2496>(ctx, base);
}

GH_HOOK(sub_824995F8) {
  GH_PROLOGUE();
  RelativeTable<1440>(ctx, base);
}

GH_HOOK(sub_82499C38) {
  GH_PROLOGUE();
  RelativeTable<2752>(ctx, base);
}

GH_HOOK(sub_82499508) {
  GH_PROLOGUE();
  RelativeTable<1152>(ctx, base);
}

GH_HOOK(sub_82499AD0) {
  GH_PROLOGUE();
  RelativeTable<2432>(ctx, base);
}

GH_HOOK(sub_824991E8) {
  GH_PROLOGUE();
  RelativeTable<448>(ctx, base);
}

GH_HOOK(sub_82499760) {
  GH_PROLOGUE();
  RelativeTable<1728>(ctx, base);
}

GH_HOOK(sub_82499DF0) {
  GH_PROLOGUE();
  RelativeTable<3104>(ctx, base);
}

GH_HOOK(sub_82499080) {
  GH_PROLOGUE();
  RelativeTable<160>(ctx, base);
}

GH_HOOK(sub_824994E0) {
  GH_PROLOGUE();
  RelativeTable<1120>(ctx, base);
}

GH_HOOK(sub_82499AA8) {
  GH_PROLOGUE();
  RelativeTable<2400>(ctx, base);
}

GH_HOOK(sub_82499B98) {
  GH_PROLOGUE();
  RelativeTable<2592>(ctx, base);
}

GH_HOOK(sub_82499148) {
  GH_PROLOGUE();
  RelativeTable<320>(ctx, base);
}

GH_HOOK(sub_824996C0) {
  GH_PROLOGUE();
  RelativeTable<1600>(ctx, base);
}

GH_HOOK(sub_82499D50) {
  GH_PROLOGUE();
  RelativeTable<2976>(ctx, base);
}

GH_HOOK(sub_824993C8) {
  GH_PROLOGUE();
  RelativeTable<864>(ctx, base);
}

GH_HOOK(sub_82499968) {
  GH_PROLOGUE();
  RelativeTable<2144>(ctx, base);
}

GH_HOOK(sub_82499620) {
  GH_PROLOGUE();
  RelativeTable<1472>(ctx, base);
}

GH_HOOK(sub_82499C60) {
  GH_PROLOGUE();
  RelativeTable<2784>(ctx, base);
}

GH_HOOK(sub_824990A8) {
  GH_PROLOGUE();
  RelativeTable<192>(ctx, base);
}

GH_HOOK(sub_82499530) {
  GH_PROLOGUE();
  RelativeTable<1184>(ctx, base);
}

GH_HOOK(sub_82499B48) {
  GH_PROLOGUE();
  RelativeTable<2528>(ctx, base);
}

GH_HOOK(sub_82499198) {
  GH_PROLOGUE();
  RelativeTable<384>(ctx, base);
}

GH_HOOK(sub_82499710) {
  GH_PROLOGUE();
  RelativeTable<1664>(ctx, base);
}

GH_HOOK(sub_82499DA0) {
  GH_PROLOGUE();
  RelativeTable<3040>(ctx, base);
}

GH_HOOK(sub_824997D8) {
  GH_PROLOGUE();
  RelativeTable<1824>(ctx, base);
}

GH_HOOK(sub_82499E68) {
  GH_PROLOGUE();
  RelativeTable<3200>(ctx, base);
}

GH_HOOK(sub_82498FB8) {
  GH_PROLOGUE();
  RelativeTable<4864>(ctx, base);
}

GH_HOOK(sub_82499300) {
  GH_PROLOGUE();
  RelativeTable<704>(ctx, base);
}

GH_HOOK(sub_824998A0) {
  GH_PROLOGUE();
  RelativeTable<1984>(ctx, base);
}

GH_HOOK(sub_82499670) {
  GH_PROLOGUE();
  RelativeTable<1536>(ctx, base);
}

GH_HOOK(sub_824990F8) {
  GH_PROLOGUE();
  RelativeTable<256>(ctx, base);
}

GH_HOOK(sub_824995D0) {
  GH_PROLOGUE();
  RelativeTable<1408>(ctx, base);
}

GH_HOOK(sub_82499C10) {
  GH_PROLOGUE();
  RelativeTable<2720>(ctx, base);
}

GH_HOOK(sub_82499030) {
  GH_PROLOGUE();
  RelativeTable<96>(ctx, base);
}

GH_HOOK(sub_82499468) {
  GH_PROLOGUE();
  RelativeTable<1024>(ctx, base);
}

GH_HOOK(sub_82499A08) {
  GH_PROLOGUE();
  RelativeTable<2272>(ctx, base);
}

GH_HOOK(sub_82499238) {
  GH_PROLOGUE();
  RelativeTable<544>(ctx, base);
}

GH_HOOK(sub_824997B0) {
  GH_PROLOGUE();
  RelativeTable<1792>(ctx, base);
}

GH_HOOK(sub_82499210) {
  GH_PROLOGUE();
  RelativeTable<480>(ctx, base);
}

GH_HOOK(sub_82499788) {
  GH_PROLOGUE();
  RelativeTable<1760>(ctx, base);
}

GH_HOOK(sub_82499E18) {
  GH_PROLOGUE();
  RelativeTable<3136>(ctx, base);
}

GH_HOOK(sub_82499418) {
  GH_PROLOGUE();
  RelativeTable<960>(ctx, base);
}

GH_HOOK(sub_824999B8) {
  GH_PROLOGUE();
  RelativeTable<2208>(ctx, base);
}

GH_HOOK(sub_82499C88) {
  GH_PROLOGUE();
  RelativeTable<2816>(ctx, base);
}

GH_HOOK(sub_82499AF8) {
  GH_PROLOGUE();
  RelativeTable<2464>(ctx, base);
}

GH_HOOK(sub_824993A0) {
  GH_PROLOGUE();
  RelativeTable<832>(ctx, base);
}

GH_HOOK(sub_82499940) {
  GH_PROLOGUE();
  RelativeTable<2112>(ctx, base);
}

GH_HOOK(sub_824994B8) {
  GH_PROLOGUE();
  RelativeTable<1088>(ctx, base);
}

GH_HOOK(sub_82499A80) {
  GH_PROLOGUE();
  RelativeTable<2368>(ctx, base);
}

GH_HOOK(sub_82498F90) {
  GH_PROLOGUE();
  RelativeTable<4832>(ctx, base);
}

GH_HOOK(sub_824992D8) {
  GH_PROLOGUE();
  RelativeTable<672>(ctx, base);
}

GH_HOOK(sub_82499878) {
  GH_PROLOGUE();
  RelativeTable<1952>(ctx, base);
}

GH_HOOK(sub_82499328) {
  GH_PROLOGUE();
  RelativeTable<736>(ctx, base);
}

GH_HOOK(sub_824998C8) {
  GH_PROLOGUE();
  RelativeTable<2016>(ctx, base);
}

GH_HOOK(sub_82498F68) {
  GH_PROLOGUE();
  RelativeTable<4800>(ctx, base);
}

GH_HOOK(sub_824992B0) {
  GH_PROLOGUE();
  RelativeTable<640>(ctx, base);
}

GH_HOOK(sub_82499850) {
  GH_PROLOGUE();
  RelativeTable<1920>(ctx, base);
}

GH_HOOK(sub_82499CB0) {
  GH_PROLOGUE();
  RelativeTable<2848>(ctx, base);
}

GH_HOOK(sub_824990D0) {
  GH_PROLOGUE();
  RelativeTable<224>(ctx, base);
}

GH_HOOK(sub_824995A8) {
  GH_PROLOGUE();
  RelativeTable<1376>(ctx, base);
}

GH_HOOK(sub_82499BE8) {
  GH_PROLOGUE();
  RelativeTable<2688>(ctx, base);
}

GH_HOOK(sub_82499A58) {
  GH_PROLOGUE();
  RelativeTable<2336>(ctx, base);
}

GH_HOOK(sub_82499058) {
  GH_PROLOGUE();
  RelativeTable<128>(ctx, base);
}

GH_HOOK(sub_82499490) {
  GH_PROLOGUE();
  RelativeTable<1056>(ctx, base);
}

GH_HOOK(sub_82499A30) {
  GH_PROLOGUE();
  RelativeTable<2304>(ctx, base);
}

GH_HOOK(sub_824991C0) {
  GH_PROLOGUE();
  RelativeTable<416>(ctx, base);
}

GH_HOOK(sub_82499738) {
  GH_PROLOGUE();
  RelativeTable<1696>(ctx, base);
}

GH_HOOK(sub_82499DC8) {
  GH_PROLOGUE();
  RelativeTable<3072>(ctx, base);
}

GH_HOOK(sub_82498F40) {
  GH_PROLOGUE();
  RelativeTable<4768>(ctx, base);
}

GH_HOOK(sub_82499288) {
  GH_PROLOGUE();
  RelativeTable<608>(ctx, base);
}

GH_HOOK(sub_82499828) {
  GH_PROLOGUE();
  RelativeTable<1888>(ctx, base);
}

GH_HOOK(sub_82499558) {
  GH_PROLOGUE();
  RelativeTable<1216>(ctx, base);
}

GH_HOOK(sub_82499B70) {
  GH_PROLOGUE();
  RelativeTable<2560>(ctx, base);
}

GH_HOOK(sub_82771F80) {
  GH_PROLOGUE();
  Field<32, 224, false>(ctx, base);
}

GH_HOOK(sub_826F1838) {
  GH_PROLOGUE();
  Field<32, 528, false>(ctx, base);
}

GH_HOOK(sub_826E2AF8) {
  GH_PROLOGUE();
  Field<32, 236, false>(ctx, base);
}

GH_HOOK(sub_827701B0) {
  GH_PROLOGUE();
  Field<32, 448, false>(ctx, base);
}

GH_HOOK(sub_82774AF8) {
  GH_PROLOGUE();
  Field<32, 92, false>(ctx, base);
}

GH_HOOK(sub_82771F70) {
  GH_PROLOGUE();
  Field<32, 216, false>(ctx, base);
}

GH_HOOK(sub_826F1820) {
  GH_PROLOGUE();
  Field<32, 516, false>(ctx, base);
}

GH_HOOK(sub_82771F78) {
  GH_PROLOGUE();
  Field<32, 220, false>(ctx, base);
}

GH_HOOK(sub_826F1828) {
  GH_PROLOGUE();
  Field<32, 520, false>(ctx, base);
}

GH_HOOK(sub_8276AD00) {
  GH_PROLOGUE();
  Field<32, 48, false>(ctx, base);
}

GH_HOOK(sub_826F15E0) {
  GH_PROLOGUE();
  Field<32, 532, false>(ctx, base);
}

GH_HOOK(sub_826F1830) {
  GH_PROLOGUE();
  Field<32, 524, false>(ctx, base);
}

GH_HOOK(sub_826B7E40) {
  GH_PROLOGUE();
  Field<32, 80, false>(ctx, base);
}

GH_HOOK(sub_8276ACF8) {
  GH_PROLOGUE();
  Field<32, 44, false>(ctx, base);
}

GH_HOOK(sub_82899988) {
  GH_PROLOGUE();
  Field<32, 4, false>(ctx, base);
}

GH_HOOK(sub_8276AD08) {
  GH_PROLOGUE();
  Field<32, 52, false>(ctx, base);
}

GH_HOOK(sub_82705810) {
  GH_PROLOGUE();
  Field<32, 120, false>(ctx, base);
}

GH_HOOK(sub_827701A0) {
  GH_PROLOGUE();
  Field<32, 444, false>(ctx, base);
}

GH_HOOK(sub_8276DF20) {
  GH_PROLOGUE();
  Field<32, 284, false>(ctx, base);
}

GH_HOOK(sub_824C9720) {
  GH_PROLOGUE();
  StateEquals<1>(ctx, base);
}

GH_HOOK(sub_824C97E0) {
  GH_PROLOGUE();
  StateEquals<8>(ctx, base);
}

GH_HOOK(sub_824C9800) {
  GH_PROLOGUE();
  StateEquals<9>(ctx, base);
}

GH_HOOK(sub_824C97C0) {
  GH_PROLOGUE();
  StateEquals<7>(ctx, base);
}

GH_HOOK(sub_824C9780) {
  GH_PROLOGUE();
  StateEquals<4>(ctx, base);
}

GH_HOOK(sub_824C9740) {
  GH_PROLOGUE();
  StateEquals<2>(ctx, base);
}

GH_HOOK(sub_824C9760) {
  GH_PROLOGUE();
  StateEquals<3>(ctx, base);
}

GH_HOOK(sub_824C97A0) {
  GH_PROLOGUE();
  StateEquals<5>(ctx, base);
}

GH_HOOK(sub_8264E85C) {
  GH_PROLOGUE();
  const uint32_t pointer = GH_LOAD_U32(ctx.r3.u32);
  ctx.r10.u64 = GH_LOAD_U32(pointer);
  ctx.r11.u64 = std::countl_zero(uint32_t(0xC0000017u - ctx.r10.u32));
  ctx.r3.u64 = ctx.r10.u32 == 0xC0000017u;
}

GH_HOOK(sub_8264BF80) {
  GH_PROLOGUE();
  ctx.r11.s64 = -1073741824;
  ctx.r10.u64 = uint64_t(int64_t(-1073741824)) | 5;
  ctx.r9.u64 = GH_LOAD_U32(ctx.r3.u32);
  ctx.r8.u64 = GH_LOAD_U32(ctx.r9.u32);
  ctx.r7.u64 = ctx.r10.u64 - ctx.r8.u64;
  ctx.r6.u64 = std::countl_zero(ctx.r7.u32);
  ctx.r3.u64 = ctx.r8.u32 == 0xC0000005u;
}

GH_HOOK(sub_826324AC) {
  GH_PROLOGUE();
  const uint32_t pointer = GH_LOAD_U32(ctx.r3.u32);
  ctx.r10.u64 = GH_LOAD_U32(pointer);
  ctx.r11.u64 = std::countl_zero(uint32_t(0xC0000017u - ctx.r10.u32));
  ctx.r3.u64 = ctx.r10.u32 == 0xC0000017u;
}

GH_HOOK(sub_8288E990) {
  GH_PROLOGUE();
  const uint32_t pointer = GH_LOAD_U32(ctx.r3.u32);
  ctx.r10.u64 = GH_LOAD_U32(pointer);
  ctx.r11.u64 = std::countl_zero(uint32_t(0xC0000017u - ctx.r10.u32));
  ctx.r3.u64 = ctx.r10.u32 == 0xC0000017u;
}

GH_HOOK(sub_8280F2A0) {
  GH_PROLOGUE();
  ctx.r11.s64 = -1073741824;
  ctx.r10.u64 = uint64_t(int64_t(-1073741824)) | 5;
  ctx.r9.u64 = GH_LOAD_U32(ctx.r3.u32);
  ctx.r8.u64 = GH_LOAD_U32(ctx.r9.u32);
  ctx.r7.u64 = ctx.r10.u64 - ctx.r8.u64;
  ctx.r6.u64 = std::countl_zero(ctx.r7.u32);
  ctx.r3.u64 = ctx.r8.u32 == 0xC0000005u;
}

GH_HOOK(sub_826B80C8) {
  GH_PROLOGUE();
  EnabledFlag<29>(ctx, base);
}

GH_HOOK(sub_826B8200) {
  GH_PROLOGUE();
  EnabledFlag<14>(ctx, base);
}

GH_HOOK(sub_826B8098) {
  GH_PROLOGUE();
  EnabledFlag<30>(ctx, base);
}

GH_HOOK(sub_826B80F8) {
  GH_PROLOGUE();
  EnabledFlag<15>(ctx, base);
}

GH_HOOK(sub_8242B528) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(uint32_t(-2103836672 + 2296));
  ctx.r10.u64 = ctx.r11.u64 - 1;
  ctx.xer.ca = ctx.r11.u32 != 0;
  ctx.r3.u64 = ctx.xer.ca;
}

GH_HOOK(sub_82435BA8) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(uint32_t(-2103771136 + -21248));
  ctx.r10.u64 = ctx.r11.u64 - 1;
  ctx.xer.ca = ctx.r11.u32 != 0;
  ctx.r3.u64 = ctx.xer.ca;
}

GH_HOOK(sub_82435AE8) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(uint32_t(-2103771136 + -21272));
  ctx.r10.u64 = ctx.r11.u64 - 1;
  ctx.xer.ca = ctx.r11.u32 != 0;
  ctx.r3.u64 = ctx.xer.ca;
}

GH_HOOK(sub_82435BD8) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(uint32_t(-2103771136 + -20852));
  ctx.r10.u64 = ctx.r11.u64 - 1;
  ctx.r9.u64 = std::countl_zero(ctx.r10.u32);
  ctx.r3.u64 = ctx.r11.u32 == 1;
}

GH_HOOK(sub_82422F50) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(uint32_t(-2104426496 + 2500));
  ctx.r10.u64 = ctx.r11.u64 - 3;
  ctx.r9.u64 = std::countl_zero(ctx.r10.u32);
  ctx.r3.u64 = ctx.r11.u32 == 3;
}

GH_HOOK(sub_8242C410) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(uint32_t(-2103836672 + 6392));
  ctx.r10.u64 = ctx.r11.u64 - 1;
  ctx.r9.u64 = std::countl_zero(ctx.r10.u32);
  ctx.r3.u64 = ctx.r11.u32 == 1;
}

GH_HOOK(sub_82774B28) {
  GH_PROLOGUE();
  Field<64, 64, false>(ctx, base);
}

GH_HOOK(sub_82774B18) {
  GH_PROLOGUE();
  Field<64, 56, false>(ctx, base);
}

GH_HOOK(sub_826B7FC0) {
  GH_PROLOGUE();
  Field<64, 16, false>(ctx, base);
}

GH_HOOK(sub_825D94C8) {
  GH_PROLOGUE();
  ctx.r11.u64 = ctx.r3.u64 + ctx.r4.u64;
  ctx.r3.u64 = GH_LOAD_U8(ctx.r11.u32 + 12228);
}

GH_HOOK(sub_825D9548) {
  GH_PROLOGUE();
  ctx.r11.u64 = ctx.r3.u64 + ctx.r4.u64;
  ctx.r3.u64 = GH_LOAD_U8(ctx.r11.u32 + 12254);
}

GH_HOOK(sub_825D9300) {
  GH_PROLOGUE();
  ctx.r11.u64 = ctx.r3.u64 + ctx.r4.u64;
  ctx.r3.u64 = GH_LOAD_U8(ctx.r11.u32 + 10864);
}

GH_HOOK(sub_82496CE8) {
  GH_PROLOGUE();
  NullableField<20, 0>(ctx, base);
}

GH_HOOK(sub_82496CD0) {
  GH_PROLOGUE();
  NullableField<16, 0>(ctx, base);
}

GH_HOOK(sub_82619150) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(ctx.r3.u32 + 4);
  ctx.r10.u64 = GH_LOAD_U32(ctx.r11.u32 + 44);
  ctx.r3.u64 = ctx.r10.u64 - ctx.r6.u64;
}

GH_HOOK(sub_827704A8) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  ctx.r11.u64 = GH_LOAD_U32(object + 68);
  ctx.r10.u64 = GH_LOAD_U32(object + 52);
  ctx.r3.u64 = ctx.r10.u64 - ctx.r11.u64;
}

GH_HOOK(sub_825D8F18) {
  GH_PROLOGUE();
  IndexedKind<11, 21, 2047>(ctx, base);
}

GH_HOOK(sub_825D90C0) {
  GH_PROLOGUE();
  IndexedKind<10, 19, 8191>(ctx, base);
}

GH_HOOK(sub_8288A048) {
  GH_PROLOGUE();
  ChangeCase<65, 90, 32>(ctx);
}

GH_HOOK(sub_8288A0D0) {
  GH_PROLOGUE();
  ChangeCase<97, 122, -32>(ctx);
}

GH_HOOK(sub_825D0040) {
  GH_PROLOGUE();
  Field<8, 268, true>(ctx, base);
}

GH_HOOK(sub_826B8618) {
  GH_PROLOGUE();
  Field<8, 394, false>(ctx, base);
}

GH_HOOK(sub_82709A38) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(ctx.r3.u32 + 12);
  ctx.r10.u64 = GH_LOAD_U32(ctx.r11.u32 + 2272);
  ctx.cr6.compare<int32_t>(ctx.r10.s32, 0, ctx.xer);
  ctx.r3.u64 = ctx.r10.u32 ? GH_LOAD_U32(ctx.r11.u32 + 2280) : 1;
}

GH_HOOK(sub_82709A58) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(ctx.r3.u32 + 12);
  ctx.r10.u64 = GH_LOAD_U32(ctx.r11.u32 + 2272);
  ctx.cr6.compare<int32_t>(ctx.r10.s32, 0, ctx.xer);
  ctx.r3.u64 = ctx.r10.u32 ? GH_LOAD_U32(ctx.r11.u32 + 2284) : 1;
}

GH_HOOK(sub_825D9288) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U8(ctx.r3.u32 + ctx.r4.u32 + 10890);
  ctx.r3.u64 = (ctx.r11.u32 >> 2) & 1;
}

GH_HOOK(sub_825D8FE8) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U8(ctx.r3.u32 + ctx.r4.u32 + 10890);
  ctx.r3.u64 = (ctx.r11.u32 >> 1) & 1;
}

GH_HOOK(sub_8268AAF8) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(ctx.r3.u32 + 288);
  ctx.r10.u64 = GH_LOAD_U32(ctx.r11.u32 + 40);
  ctx.r9.u64 = ctx.r10.u64 - 1;
  ctx.xer.ca = ctx.r10.u32 != 0;
  ctx.r3.u64 = ctx.xer.ca;
}

GH_HOOK(sub_8268AD70) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(ctx.r3.u32 + 324);
  ctx.r10.u64 = GH_LOAD_U32(ctx.r11.u32 + 40);
  ctx.r9.u64 = ctx.r10.u64 - 1;
  ctx.xer.ca = ctx.r10.u32 != 0;
  ctx.r3.u64 = ctx.xer.ca;
}

GH_HOOK(sub_825D9650) {
  GH_PROLOGUE();
  IndexedBits<13, 7>(ctx, base);
}

GH_HOOK(sub_825D9600) {
  GH_PROLOGUE();
  IndexedBits<10, 7>(ctx, base);
}

GH_HOOK(sub_826A3548) {
  GH_PROLOGUE();
  ctx.r3.u64 = GH_LOAD_U32(ctx.r3.u32 + 4);
  ctx.cr6.compare<uint32_t>(ctx.r3.u32, 0, ctx.xer);
  if (ctx.r3.u32 == 0) {
    ctx.r11.s64 = -2110128128;
    ctx.r3.s64 = -2110128128 + 6760;
  }
}

GH_HOOK(sub_82410260) {
  GH_PROLOGUE();
  ctx.r3.u64 = GH_LOAD_U32(ctx.r3.u32 + 4);
  ctx.cr6.compare<uint32_t>(ctx.r3.u32, 0, ctx.xer);
  if (ctx.r3.u32 == 0) {
    ctx.r11.s64 = -2113929216;
    ctx.r3.s64 = -2113929216 + 2812;
  }
}

GH_HOOK(sub_8242AF28) {
  GH_PROLOGUE();
  NullableField<4, 4>(ctx, base);
}

GH_HOOK(sub_8242AF48) {
  GH_PROLOGUE();
  NullableField<4, 0>(ctx, base);
}

GH_HOOK(sub_825D95A0) {
  GH_PROLOGUE();
  const uint32_t value = GH_LOAD_U32(ctx.r3.u32 + ctx.r4.u32 * 24 + 1172) & 3;
  ctx.r10.u64 = UINT64_MAX;
  ctx.xer.ca = value == 0;
  ctx.r11.u64 = value ? UINT64_MAX : 0;
  ctx.r3.u64 = ctx.r11.u64;
}

GH_HOOK(sub_82786958) {
  GH_PROLOGUE();
  const uint32_t value = GH_LOAD_U32(ctx.r3.u32 + 30640);
  ctx.r10.u64 = value;
  ctx.r11.u64 = 20;
  ctx.r9.u64 = uint64_t(20) - value;
  ctx.r8.u64 = ~(uint64_t(value) ^ 20);
  ctx.r7.u64 = (ctx.r8.u32 >> 31) & 1;
  ctx.r6.u64 = ctx.r7.u64 + (value <= 20);
  ctx.xer.ca = 0;
  ctx.r3.u64 = ctx.r6.u32 & 1;
}

GH_HOOK(sub_826A5810) {
  GH_PROLOGUE();
  const uint16_t left = GH_LOAD_U16(ctx.r3.u32);
  const uint16_t right = GH_LOAD_U16(ctx.r4.u32);
  ctx.r11.u64 = left;
  ctx.r10.u64 = right;
  ctx.cr6.compare<uint32_t>(left, right, ctx.xer);
  if (left < right) ctx.r3.u64 = UINT64_MAX;
  else {
    ctx.xer.ca = left == right;
    ctx.r11.u64 = left == right ? 0 : UINT64_MAX;
    ctx.r3.u64 = left != right;
  }
}

GH_HOOK(sub_8268D148) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(ctx.r3.u32 + 32);
  ctx.r10.u64 = ctx.r11.u64 - 1;
  ctx.xer.ca = ctx.r11.u32 != 0;
  ctx.r3.u64 = ctx.xer.ca;
}

GH_HOOK(sub_826F1590) {
  GH_PROLOGUE();
  ctx.r11.u64 = 35016;
  ctx.r3.u64 = GH_LOAD_U32(ctx.r3.u32 + 35016);
}

GH_HOOK(sub_82499E90) {
  GH_PROLOGUE();
  const uint32_t holder = GH_LOAD_U32(ctx.r3.u32);
  ctx.r11.u64 = holder;
  ctx.cr6.compare<uint32_t>(holder, 0, ctx.xer);
  if (holder == 0) { ctx.r3.u64 = 0; return; }
  ctx.r11.u64 = GH_LOAD_U32(holder);
  ctx.r9.u64 = uint32_t((ctx.r4.u32 + 101) << 5);
  ctx.r10.u64 = GH_LOAD_U32(ctx.r11.u32 + ctx.r9.u32);
  ctx.r3.u64 = ctx.r11.u64 + ctx.r10.u64;
}

GH_HOOK(sub_825E1F68) {
  GH_PROLOGUE();
  const uint32_t value = ctx.r3.u32;
  uint32_t last = 63;
  for (uint32_t candidate : {30u, 31u, 32u, 36u, 37u, 38u, 57u, 63u}) {
    if (value == candidate) { last = candidate; break; }
  }
  ctx.cr6.compare<uint32_t>(value, last, ctx.xer);
  ctx.r3.u64 = value == last;
}

GH_HOOK(sub_82667C40) {
  GH_PROLOGUE();
  ctx.r11.u64 = uint32_t(ctx.r6.u32 << 1);
  ctx.r10.u64 = GH_LOAD_U16(ctx.r3.u32 + ctx.r11.u32);
  ctx.r3.s64 = ctx.r10.s16;
}

GH_HOOK(sub_8268A228) {
  GH_PROLOGUE();
  ctx.r11.u64 = UINT64_MAX;
  ctx.r10.u64 = ~ctx.r4.u64;
  ctx.r9.u64 = ctx.r10.u64 - 1;
  ctx.xer.ca = ctx.r4.u32 != UINT32_MAX;
  ctx.r3.u64 = ctx.xer.ca;
}

GH_HOOK(sub_82495F78) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(ctx.r3.u32 + 4);
  ctx.r10.u64 = UINT64_MAX;
  ctx.r9.u64 = ~ctx.r11.u64;
  ctx.r8.u64 = ctx.r9.u64 - 1;
  ctx.xer.ca = ctx.r11.u32 != UINT32_MAX;
  ctx.r3.u64 = ctx.xer.ca;
}

GH_HOOK(sub_82899968) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(ctx.r3.u32 + 4);
  ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
  if (ctx.r11.u32 != 0) ctx.cr6.compare<int32_t>(ctx.r11.s32, -1, ctx.xer);
  ctx.r3.u64 = ctx.r11.u32 != 0 && ctx.r11.u32 != UINT32_MAX;
}

GH_HOOK(sub_824259D0) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  const uint8_t first = GH_LOAD_U8(object + 392);
  ctx.cr6.compare<uint32_t>(first, 0, ctx.xer);
  bool enabled = first != 0;
  if (!enabled) {
    const uint8_t second = GH_LOAD_U8(object + 453);
    ctx.cr6.compare<uint32_t>(second, 0, ctx.xer);
    enabled = second != 0;
  }
  ctx.r11.u64 = enabled;
  ctx.r3.u64 = enabled;
}

GH_HOOK(sub_82776AC0) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  ctx.r11.u64 = GH_LOAD_U32(object + 1272);
  ctx.cr6.compare<int32_t>(ctx.r11.s32, 0, ctx.xer);
  ctx.r3.u64 = 0;
  if (ctx.r11.u32 != 0) {
    ctx.r11.u64 = GH_LOAD_U32(object + 2124);
    ctx.cr6.compare<int32_t>(ctx.r11.s32, 0, ctx.xer);
    ctx.r3.u64 = ctx.r11.s32 > 0;
  }
}

GH_HOOK(sub_8242BC00) {
  GH_PROLOGUE();
  ctx.r10.s64 = -2146107392;
  ctx.r9.u64 = uint64_t(int64_t(-2146107392)) | 20994;
  ctx.r11.u64 = GH_LOAD_U32(uint32_t(-2103836672 + 6448));
  ctx.r8.u64 = ctx.r9.u64 - ctx.r11.u64;
  ctx.r7.u64 = std::countl_zero(ctx.r8.u32);
  ctx.r3.u64 = ctx.r11.u32 == ctx.r9.u32;
}

GH_HOOK(sub_82435BC0) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(uint32_t(-2103771136 - 20852));
  ctx.r10.u64 = std::countl_zero(ctx.r11.u32);
  ctx.r3.u64 = ctx.r11.u32 == 0;
}

GH_HOOK(sub_82457D70) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U8(ctx.r3.u32 + 85);
  ctx.r10.u64 = ctx.r11.u64 - 1;
  ctx.xer.ca = ctx.r11.u32 != 0;
  ctx.r3.u64 = ctx.xer.ca;
}

GH_HOOK(sub_826A6220) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U16(ctx.r3.u32);
  ctx.r10.u64 = GH_LOAD_U16(ctx.r4.u32);
  ctx.r3.u64 = ctx.r11.u64 - ctx.r10.u64;
}

GH_HOOK(sub_82435C10) {
  GH_PROLOGUE();
  ctx.r10.s64 = -2103771136 - 20836;
  ctx.r11.u64 = GH_LOAD_U16(uint32_t(-2103771136 - 20834));
  ctx.r9.u64 = ctx.r11.u64 - 18;
  ctx.r8.u64 = std::countl_zero(ctx.r9.u32);
  ctx.r3.u64 = ctx.r11.u32 == 18;
}

GH_HOOK(sub_824962A8) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  ctx.r11.u64 = GH_LOAD_U8(object + 4261) < 128;
  ctx.r10.u64 = GH_LOAD_U8(object + 4262);
  ctx.cr6.compare<uint32_t>(ctx.r10.u32, 128, ctx.xer);
  ctx.r3.u64 = ctx.r11.u64 + (ctx.r10.u32 < 128);
}

GH_HOOK(sub_8251F558) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(ctx.r3.u32 + 3248);
  ctx.r10.u64 = uint64_t(0) - ctx.r11.u64;
  ctx.r9.u64 = ctx.r10.u64 & ~ctx.r11.u64;
  ctx.r3.u64 = (ctx.r9.u32 >> 31) & 1;
}

GH_HOOK(sub_82707F88) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  ctx.r11.u64 = GH_LOAD_U32(object + 52);
  ctx.r10.u64 = GH_LOAD_U32(object + 44);
  ctx.r3.u64 = ctx.r11.u64 + ctx.r10.u64;
}

GH_HOOK(sub_8242C4B0) {
  GH_PROLOGUE();
  ctx.r11.s64 = -2103836672;
  ctx.r9.u64 = GH_LOAD_U8(uint32_t(-2103836672 + 6418));
  ctx.r10.u64 = GH_LOAD_U32(uint32_t(-2103836672 + 2312));
  ctx.r3.u64 = ctx.r10.u64 - ctx.r9.u64;
}

GH_HOOK(sub_82481828) {
  GH_PROLOGUE();
  const int16_t value = std::bit_cast<int16_t>(GH_LOAD_U16(ctx.r3.u32 + 356));
  ctx.r11.s64 = value;
  ctx.cr6.compare<int32_t>(value, 2, ctx.xer);
  if (value == 2) { ctx.r3.u64 = 1; return; }
  ctx.r11.s64 = int64_t(value) - 3;
  ctx.r10.u64 = std::countl_zero(ctx.r11.u32);
  ctx.r3.u64 = value == 3;
}

GH_HOOK(sub_8243EEC0) {
  GH_PROLOGUE();
  ctx.r10.u64 = ctx.r4.u8;
  ctx.r9.u64 = ctx.r6.u8;
  ctx.r11.u64 = uint64_t(ctx.r4.u8) + ctx.r5.u8 + ctx.r6.u8;
  ctx.r3.u64 = ctx.r11.u64;
}

GH_HOOK(sub_825CF030) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(uint32_t(-2113929216 + 1804));
  ctx.r3.u64 = GH_LOAD_U32(ctx.r11.u32 + 16);
}

GH_HOOK(sub_8242B1E8) {
  GH_PROLOGUE();
  ctx.r11.s64 = -2103902208 + 9664;
  ctx.r10.u64 = GH_LOAD_U32(ctx.r11.u32);
  ctx.cr6.compare<uint32_t>(ctx.r10.u32, 997, ctx.xer);
  if (ctx.r10.u32 == 997) { ctx.r3.u64 = 0; return; }
  const uint32_t value = GH_LOAD_U32(ctx.r11.u32 + 24);
  ctx.r11.u64 = std::countl_zero(value);
  ctx.r3.u64 = value == 0;
}

GH_HOOK(sub_82421B60) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U16(uint32_t(-2103902208 + 5400));
  const uint32_t low = ctx.r11.u32 & 15;
  // The generated wrapping mask retains the repeated word in the upper half.
  ctx.r10.u64 = (uint64_t(low) << 32) | (low & 9);
  ctx.r9.u64 = std::countl_zero(ctx.r10.u32);
  ctx.r3.u64 = (low & 9) == 0;
}

GH_HOOK(sub_82513DA8) {
  GH_PROLOGUE();
  ctx.r11.u64 = uint64_t(GH_LOAD_U8(ctx.r3.u32 + 84)) - 54;
  ctx.r10.u64 = ctx.r11.u64 - 1;
  ctx.xer.ca = ctx.r11.u32 != 0;
  ctx.r3.u64 = ctx.xer.ca;
}

GH_HOOK(sub_825D9860) {
  GH_PROLOGUE();
  const uint32_t word = GH_LOAD_U32(ctx.r3.u32 + ctx.r4.u32 * 24 + 1156);
  ctx.r11.u64 = ~uint64_t(word);
  ctx.r3.u64 = ((word >> 11) & 1) == 0;
}

GH_HOOK(sub_82673940) {
  GH_PROLOGUE();
  const uint32_t value = ctx.r3.u16;
  ctx.r11.u64 = value;
  const uint32_t compared = value == 357 || value == 358 ? value : 359;
  ctx.cr6.compare<uint32_t>(value, compared, ctx.xer);
  ctx.r3.u64 = value >= 357 && value <= 359;
}

GH_HOOK(sub_82707F98) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  const uint32_t end = GH_LOAD_U32(object + 52);
  ctx.r10.u64 = UINT64_MAX;
  ctx.r9.u64 = GH_LOAD_U32(object + 48);
  ctx.r11.u64 = uint64_t(end) - ctx.r9.u64;
  ctx.r3.u64 = end >= ctx.r9.u32;
  ctx.xer.ca = 0;
}

GH_HOOK(sub_827093B8) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U64(ctx.r3.u32 + 64);
  ctx.r3.u64 = ctx.r11.u32;
}

GH_HOOK(sub_824C9820) {
  GH_PROLOGUE();
  StateEquals<10, true>(ctx, base);
}

GH_HOOK(sub_82635840) {
  GH_PROLOGUE();
  const uint32_t left = GH_LOAD_U32(ctx.r3.u32);
  ctx.r10.u64 = GH_LOAD_U32(ctx.r4.u32);
  ctx.r11.u64 = uint64_t(left) - ctx.r10.u64;
  ctx.xer.ca = left == ctx.r10.u32;
  ctx.r3.u64 = left == ctx.r10.u32 ? 0 : UINT64_MAX;
}

GH_HOOK(sub_826B7130) {
  GH_PROLOGUE();
  const uint32_t value = GH_LOAD_U32(ctx.r3.u32 + 20);
  ctx.r11.u64 = uint64_t(int64_t(-2147024896)) + 996 - value;
  ctx.r10.u64 = ctx.r11.u64 - 1;
  ctx.xer.ca = value != uint32_t(-2147024896 + 996);
  ctx.r3.u64 = ctx.xer.ca;
}

GH_HOOK(sub_8242B440) {
  GH_PROLOGUE();
  ctx.r10.u64 = UINT64_MAX;
  ctx.r11.u64 = GH_LOAD_U32(uint32_t(-2103902208 + 9704));
  ctx.r9.u64 = ~ctx.r11.u64;
  ctx.r8.u64 = ctx.r9.u64 - 1;
  ctx.xer.ca = ctx.r11.u32 != UINT32_MAX;
  ctx.r3.u64 = ctx.xer.ca;
}

GH_HOOK(sub_82436548) {
  GH_PROLOGUE();
  ctx.r11.s64 = -2103771136;
  ctx.r10.u64 = GH_LOAD_U8(uint32_t(-2103771136 - 21256));
  ctx.r9.u64 = std::countl_zero(ctx.r10.u32);
  ctx.r3.u64 = ctx.r10.u32 == 0;
}

GH_HOOK(sub_8246BD48) {
  GH_PROLOGUE();
  ctx.r3.u64 = GH_LOAD_U32(ctx.r3.u32 + 600);
  ctx.cr6.compare<int32_t>(ctx.r3.s32, 256, ctx.xer);
  if (ctx.r3.s32 >= 256) ctx.r3.u64 -= 256;
}

GH_HOOK(sub_827787A8) {
  GH_PROLOGUE();
  const uint64_t object = ctx.r3.u64;
  ctx.r10.u64 = GH_LOAD_U32(uint32_t(object) + 31544);
  ctx.r11.u64 = object;
  ctx.r3.u64 = 0;
  ctx.cr6.compare<int32_t>(ctx.r10.s32, 0, ctx.xer);
  if (ctx.r10.u32 == 0) return;
  ctx.r10.u64 = GH_LOAD_U32(uint32_t(object) + 2804);
  ctx.cr6.compare<int32_t>(ctx.r10.s32, 1, ctx.xer);
  if (ctx.r10.u32 != 1) return;
  ctx.r11.u64 = GH_LOAD_U32(uint32_t(object) + 2808);
  ctx.cr6.compare<int32_t>(ctx.r11.s32, 0, ctx.xer);
  ctx.r3.u64 = ctx.r11.u32 == 0;
}

GH_HOOK(sub_8269ECF8) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(ctx.r3.u32 + 16);
  ctx.r10.u64 = GH_LOAD_U32(ctx.r11.u32);
  ctx.r9.u64 = ctx.r10.u32 & 31;
  ctx.cr6.compare<int32_t>(ctx.r9.s32, 6, ctx.xer);
  ctx.r3.u64 = ctx.r9.u32 == 6 ? ctx.r11.u64 + 13 : 0;
}

GH_HOOK(sub_824101F4) {
  GH_PROLOGUE();
  ctx.r11.u64 = 1080885248;
  ctx.r10.u64 = 1080885248u + 5000;
  ctx.r9.u64 = GH_LOAD_U32(ctx.r3.u32);
  ctx.r8.u64 = GH_LOAD_U32(ctx.r9.u32);
  ctx.cr6.compare<uint32_t>(ctx.r8.u32, ctx.r10.u32, ctx.xer);
  ctx.r3.u64 = ctx.r8.u32 == ctx.r10.u32 ? UINT64_MAX : 1;
}

GH_HOOK(sub_824A51C0) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  ctx.r11.u64 = GH_LOAD_U8(object + 1188);
  ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
  ctx.r3.u64 = ctx.r11.u32 ? GH_LOAD_U32(object + 60) : 0;
}

GH_HOOK(sub_825D61FC) {
  GH_PROLOGUE();
  const uint32_t pointer = GH_LOAD_U32(ctx.r3.u32);
  const uint32_t value = GH_LOAD_U32(pointer);
  ctx.r11.u64 = uint64_t(int64_t(-1073741824)) + 23 - value;
  ctx.r10.u64 = ctx.r11.u64 - 1;
  ctx.xer.ca = value != 0xC0000017u;
  ctx.r3.u64 = ctx.xer.ca;
}

GH_HOOK(sub_82485C78) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  const uint32_t end = GH_LOAD_U32(object + 164);
  ctx.r10.u64 = GH_LOAD_U32(object + 160);
  ctx.r9.u64 = uint64_t(end) - ctx.r10.u64;
  const int32_t difference = ctx.r9.s32;
  ctx.xer.ca = difference < 0 && (uint32_t(difference) & 15) != 0;
  ctx.r11.s64 = difference >> 4;
  ctx.r3.u64 = uint64_t(int64_t(difference >> 4)) - 2;
}

GH_HOOK(sub_8242B218) {
  GH_PROLOGUE();
  ctx.r11.s64 = -2103902208 + 9664;
  ctx.r10.u64 = GH_LOAD_U32(ctx.r11.u32);
  ctx.cr6.compare<uint32_t>(ctx.r10.u32, 997, ctx.xer);
  if (ctx.r10.u32 == 997) { ctx.r3.u64 = 0; return; }
  ctx.r11.u64 = GH_LOAD_U32(ctx.r11.u32 + 24);
  ctx.r10.u64 = ctx.r11.u64 - 1;
  ctx.xer.ca = ctx.r11.u32 != 0;
  ctx.r3.u64 = ctx.xer.ca;
}

GH_HOOK(sub_8260F1C0) {
  GH_PROLOGUE();
  const uint32_t first = ctx.r3.u32;
  ctx.cr6.compare<uint32_t>(first, 0, ctx.xer);
  if (first == 0) { ctx.r3.u64 = 0; return; }
  ctx.cr6.compare<uint32_t>(ctx.r4.u32, 0, ctx.xer);
  if (ctx.r4.u32 == 0) { ctx.r3.u64 = 0; return; }
  ctx.r11.u64 = GH_LOAD_U32(ctx.r4.u32 + 8);
  ctx.r3.u64 = ctx.r11.u64 & ctx.r5.u64;
}

GH_HOOK(sub_8260F208) {
  GH_PROLOGUE();
  const uint32_t first = ctx.r3.u32;
  ctx.cr6.compare<uint32_t>(first, 0, ctx.xer);
  if (first == 0) { ctx.r3.u64 = 0; return; }
  ctx.cr6.compare<uint32_t>(ctx.r4.u32, 0, ctx.xer);
  ctx.r3.u64 = ctx.r4.u32 ? GH_LOAD_U8(ctx.r4.u32 + 29) : 0;
}

GH_HOOK(sub_824C9848) {
  GH_PROLOGUE();
  StateEquals<0, false, true>(ctx, base);
}

GH_HOOK(sub_826E08B0) {
  GH_PROLOGUE();
  const int32_t value = ctx.r3.s32;
  ctx.cr6.compare<int32_t>(value, 0, ctx.xer);
  ctx.r11.u64 = value == 0 ? 7 : 9;
  ctx.r3.u64 = ctx.r11.u64;
}

GH_HOOK(sub_8242FD38) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(uint32_t(-2103836672 + 6392));
  ctx.cr6.compare<int32_t>(ctx.r11.s32, 1, ctx.xer);
  if (ctx.r11.u32 == 1) ctx.r3.u64 = 1;
  else {
    ctx.r11.u64 = GH_LOAD_U8(uint32_t(-2103836672 + 1824));
    ctx.r3.u64 = ctx.r11.u32 & 1;
  }
}

GH_HOOK(sub_8269AD10) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(ctx.r3.u32 + 16);
  ctx.r10.u64 = GH_LOAD_U32(ctx.r11.u32) & 31;
  const uint32_t kind = ctx.r10.u32;
  ctx.cr6.compare<int32_t>(kind, kind == 4 ? 4 : 5, ctx.xer);
  ctx.r3.u64 = kind == 4 ? ctx.r11.u64 + 16 : kind == 5 ? ctx.r11.u64 + 20 : 0;
}

GH_HOOK(sub_825DC388) {
  GH_PROLOGUE();
  ctx.r11.u64 = (GH_LOAD_U32(ctx.r3.u32 + 44) >> 6) & 15;
  ctx.r3.u64 = ctx.r11.u64 + 1;
}

GH_HOOK(sub_8268FAE8) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(ctx.r3.u32 + 36);
  ctx.r3.u64 = 0;
  ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
  if (ctx.r11.u32 == 0) return;
  ctx.r11.u64 = GH_LOAD_U32(ctx.r11.u32 + 16);
  ctx.r10.u64 = GH_LOAD_U32(ctx.r11.u32);
  ctx.r9.u64 = ctx.r10.u32 & 31;
  ctx.cr6.compare<int32_t>(ctx.r9.s32, 3, ctx.xer);
  ctx.r3.u64 = ctx.r9.u32 == 3;
}

GH_HOOK(sub_824C9708) {
  GH_PROLOGUE();
  StateEquals<0>(ctx, base);
}

GH_HOOK(sub_825D9188) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U8(ctx.r3.u32 + ctx.r4.u32 + 10890);
  ctx.r3.u64 = ctx.r11.u32 & 1;
}

GH_HOOK(sub_8260F1E8) {
  GH_PROLOGUE();
  const uint32_t first = ctx.r3.u32;
  ctx.cr6.compare<uint32_t>(first, 0, ctx.xer);
  if (first == 0) { ctx.r3.u64 = 0; return; }
  ctx.cr6.compare<uint32_t>(ctx.r4.u32, 0, ctx.xer);
  ctx.r3.u64 = ctx.r4.u32 ? GH_LOAD_U32(ctx.r4.u32 + 12) : 0;
}

GH_HOOK(sub_82425D68) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  ctx.r11.u64 = GH_LOAD_U32(object + 140);
  ctx.cr6.compare<int32_t>(ctx.r11.s32, ctx.r4.s32, ctx.xer);
  ctx.r3.u64 = ctx.r11.u32 == ctx.r4.u32 ? GH_LOAD_U8(object + 181) : 0;
}

GH_HOOK(sub_826F3508) {
  GH_PROLOGUE();
  ctx.r9.u64 = ctx.r3.u64 + 34320;
  ctx.r11.u64 = GH_LOAD_U32(ctx.r9.u32);
  ctx.r10.u64 = 0;
  ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r9.u32, ctx.xer);
  if (ctx.r11.u32 != ctx.r9.u32) {
    ctx.cr0.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
    if (ctx.r11.u32 != 0) ctx.r10.u64 = ctx.r11.u64 - 1132;
  }
  ctx.r3.u64 = ctx.r10.u64;
}

GH_HOOK(sub_82411968) {
  GH_PROLOGUE();
  ctx.r11.u64 = GH_LOAD_U32(ctx.r3.u32 + 8);
  ctx.r10.u64 = GH_LOAD_U32(ctx.r4.u32 + 8);
  ctx.r9.u64 = ctx.r10.u64 - ctx.r11.u64;
  ctx.r8.u64 = ~(ctx.r11.u64 ^ ctx.r10.u64);
  ctx.r7.u64 = (ctx.r8.u32 >> 31) & 1;
  ctx.r6.u64 = ctx.r7.u64 + (ctx.r10.u32 >= ctx.r11.u32);
  ctx.xer.ca = 0;
  ctx.r3.u64 = ctx.r6.u32 & 1;
}

GH_HOOK(sub_825D96A0) {
  GH_PROLOGUE();
  IndexedBits<0, 7, 16>(ctx, base);
}

GH_HOOK(sub_82694FB8) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  ctx.r11.u64 = GH_LOAD_U32(object + 376);
  ctx.r3.u64 = GH_LOAD_U32(object + 372);
  ctx.cr6.compare<int32_t>(ctx.r11.s32, 0, ctx.xer);
  if (ctx.r11.u32 != 0) ctx.r3.u64 |= 64;
}

