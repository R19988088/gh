#include "native_runtime.h"
#include "native_memory.h"

#include <bit>
#include <cstring>

namespace {
template <unsigned Bits>
inline void StoreField(uint8_t* base, uint32_t address, uint64_t value) {
  if constexpr (Bits == 8) GH_STORE_U8(address, uint8_t(value));
  else if constexpr (Bits == 16) GH_STORE_U16(address, uint16_t(value));
  else if constexpr (Bits == 32) GH_STORE_U32(address, uint32_t(value));
  else GH_STORE_U64(address, value);
}

template <unsigned Bits, int32_t... Offsets>
inline void WriteFields(uint8_t* base, uint32_t object, uint64_t value) {
  constexpr bool contiguous = [] {
    constexpr int32_t fields[] = {Offsets...};
    for (size_t i = 1; i < sizeof...(Offsets); ++i)
      if (fields[i] != fields[i - 1] + Bits / 8) return false;
    return true;
  }();
  // Physical aliases retain the original scalar access order.
  if constexpr (sizeof...(Offsets) > 1 && contiguous) {
    constexpr int32_t offsets[] = {Offsets...};
    constexpr uint32_t bytes = sizeof...(Offsets) * (Bits / 8);
    const uint32_t start = object + offsets[0];
    if (!value && NativeMemoryRange(start, bytes)) {
      std::memset(GH_RAW(start), 0, bytes);
      return;
    }
  }
  (StoreField<Bits>(base, object + Offsets, value), ...);
}
}  // namespace

GH_HOOK(sub_82410160) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, -20087>(base, uint32_t(0xFFFFFFFF82980000ull), input3);
  ctx.r11.u64 = 0xFFFFFFFF82980000ull;
}

GH_HOOK(sub_82422F90) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 2504>(base, uint32_t(0xFFFFFFFF82910000ull), input3);
  ctx.r11.u64 = 0xFFFFFFFF82910000ull;
}

GH_HOOK(sub_8242B748) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 7760>(base, uint32_t(0xFFFFFFFF82910000ull), input3);
  ctx.r11.u64 = 0xFFFFFFFF82910000ull;
}

GH_HOOK(sub_8242B820) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 6436>(base, uint32_t(0xFFFFFFFF829A0000ull), input3);
  ctx.r11.u64 = 0xFFFFFFFF829A0000ull;
}

GH_HOOK(sub_8242B830) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 6440>(base, uint32_t(0xFFFFFFFF829A0000ull), input3);
  ctx.r11.u64 = 0xFFFFFFFF829A0000ull;
}

GH_HOOK(sub_8242B840) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 6444>(base, uint32_t(0xFFFFFFFF829A0000ull), input3);
  ctx.r11.u64 = 0xFFFFFFFF829A0000ull;
}

GH_HOOK(sub_8242BC58) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 6212>(base, uint32_t(0xFFFFFFFF829A0000ull), input3);
  ctx.r11.u64 = 0xFFFFFFFF829A0000ull;
}

GH_HOOK(sub_82434B90) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 8724>(base, uint32_t(0xFFFFFFFF829A0000ull), input3);
  ctx.r11.u64 = 0xFFFFFFFF829A0000ull;
}

GH_HOOK(sub_82435B98) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 7836>(base, uint32_t(0xFFFFFFFF82910000ull), input3);
  ctx.r11.u64 = 0xFFFFFFFF82910000ull;
}

GH_HOOK(sub_8243EC38) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, -19540>(base, uint32_t(0xFFFFFFFF829B0000ull), input3);
  ctx.r11.u64 = 0xFFFFFFFF829B0000ull;
}

GH_HOOK(sub_82496728) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  WriteFields<8, 20>(base, uint32_t((input4 + input3)), input5);
  ctx.r11.u64 = (input4 + input3);
}

GH_HOOK(sub_824A4508) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, -13920>(base, uint32_t(0xFFFFFFFF829D0000ull), input3);
  ctx.r11.u64 = 0xFFFFFFFF829D0000ull;
}

GH_HOOK(sub_82667CA8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input6 = ctx.r6.u64;
  WriteFields<16, 0>(base, uint32_t(((uint64_t(std::rotl(uint32_t(input6), 1)) & 0xFFFFFFFEull) + input4)), input3);
  ctx.r11.u64 = (uint64_t(std::rotl(uint32_t(input6), 1)) & 0xFFFFFFFEull);
}

GH_HOOK(sub_82764FE8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  WriteFields<8, 13>(base, uint32_t((input3 + input4)), input5);
  ctx.r11.u64 = (input3 + input4);
}

GH_HOOK(sub_82886DF0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 23068>(base, uint32_t(0xFFFFFFFF82A00000ull), input3);
  ctx.r11.u64 = 0xFFFFFFFF82A00000ull;
}

GH_HOOK(sub_8288C0E8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 23160>(base, uint32_t(0xFFFFFFFF82A00000ull), input3);
  ctx.r11.u64 = 0xFFFFFFFF82A00000ull;
}

GH_HOOK(sub_8288EC58) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 23564>(base, uint32_t(0xFFFFFFFF82A00000ull), input3);
  ctx.r11.u64 = 0xFFFFFFFF82A00000ull;
}

GH_HOOK(sub_824118F8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 67>(base, uint32_t(0xFFFFFFFF8297BF28ull), input3);
  ctx.r10.u64 = 0xFFFFFFFF8297BF28ull;
  ctx.r11.u64 = 0xFFFFFFFF82980000ull;
}

GH_HOOK(sub_82411918) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 64>(base, uint32_t(0xFFFFFFFF8297BF28ull), input3);
  ctx.r10.u64 = 0xFFFFFFFF8297BF28ull;
  ctx.r11.u64 = 0xFFFFFFFF82980000ull;
}

GH_HOOK(sub_82411938) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 65>(base, uint32_t(0xFFFFFFFF8297BF28ull), input3);
  ctx.r10.u64 = 0xFFFFFFFF8297BF28ull;
  ctx.r11.u64 = 0xFFFFFFFF82980000ull;
}

GH_HOOK(sub_82411958) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 66>(base, uint32_t(0xFFFFFFFF8297BF28ull), input3);
  ctx.r10.u64 = 0xFFFFFFFF8297BF28ull;
  ctx.r11.u64 = 0xFFFFFFFF82980000ull;
}

GH_HOOK(sub_82419DB0) {
  GH_PROLOGUE();
  WriteFields<8, 4355>(base, uint32_t(0xFFFFFFFF82990000ull), 0ull);
  ctx.r10.u64 = 0xFFFFFFFF82990000ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82421B80) {
  GH_PROLOGUE();
  WriteFields<16, 5400>(base, uint32_t(0xFFFFFFFF82990000ull), 0ull);
  ctx.r10.u64 = 0xFFFFFFFF82990000ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82421C70) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0>(base, uint32_t(input3), 0xFFFFFFFF82002570ull);
  ctx.r10.u64 = 0xFFFFFFFF82002570ull;
  ctx.r11.u64 = 0xFFFFFFFF82000000ull;
}

GH_HOOK(sub_82422F68) {
  GH_PROLOGUE();
  WriteFields<8, 5840>(base, uint32_t(0xFFFFFFFF82990000ull), 1ull);
  ctx.r10.u64 = 0xFFFFFFFF82990000ull;
  ctx.r11.u64 = 1ull;
}

GH_HOOK(sub_82429A50) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 4>(base, uint32_t(0xFFFFFFFF82911D7Cull), input3);
  ctx.r10.u64 = 0xFFFFFFFF82911D7Cull;
  ctx.r11.u64 = 0xFFFFFFFF82910000ull;
}

GH_HOOK(sub_8242A3D8) {
  GH_PROLOGUE();
  WriteFields<8, 7256>(base, uint32_t(0xFFFFFFFF82990000ull), 0ull);
  ctx.r10.u64 = 0xFFFFFFFF82990000ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8242A3E8) {
  GH_PROLOGUE();
  WriteFields<8, 7256>(base, uint32_t(0xFFFFFFFF82990000ull), 1ull);
  ctx.r10.u64 = 0xFFFFFFFF82990000ull;
  ctx.r11.u64 = 1ull;
}

GH_HOOK(sub_8242C3F0) {
  GH_PROLOGUE();
  WriteFields<32, 6392>(base, uint32_t(0xFFFFFFFF829A0000ull), 2ull);
  ctx.r10.u64 = 0xFFFFFFFF829A0000ull;
  ctx.r11.u64 = 2ull;
}

GH_HOOK(sub_8242C400) {
  GH_PROLOGUE();
  WriteFields<32, 6392>(base, uint32_t(0xFFFFFFFF829A0000ull), 1ull);
  ctx.r10.u64 = 0xFFFFFFFF829A0000ull;
  ctx.r11.u64 = 1ull;
}

GH_HOOK(sub_8242F400) {
  GH_PROLOGUE();
  WriteFields<8, 6481>(base, uint32_t(0xFFFFFFFF829A0000ull), 0ull);
  ctx.r10.u64 = 0xFFFFFFFF829A0000ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8242F420) {
  GH_PROLOGUE();
  WriteFields<8, 6482>(base, uint32_t(0xFFFFFFFF829A0000ull), 0ull);
  ctx.r10.u64 = 0xFFFFFFFF829A0000ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82434968) {
  GH_PROLOGUE();
  WriteFields<8, 8717>(base, uint32_t(0xFFFFFFFF829A0000ull), 0ull);
  ctx.r10.u64 = 0xFFFFFFFF829A0000ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82434AC0) {
  GH_PROLOGUE();
  WriteFields<8, 8682>(base, uint32_t(0xFFFFFFFF829A0000ull), 0ull);
  ctx.r10.u64 = 0xFFFFFFFF829A0000ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82435B00) {
  GH_PROLOGUE();
  WriteFields<32, -21272>(base, uint32_t(0xFFFFFFFF829B0000ull), 0ull);
  ctx.r10.u64 = 0xFFFFFFFF829B0000ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82496538) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<8, 20>(base, uint32_t((input4 + input3)), 0xFFFFFFFFFFFFFFFFull);
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFull;
  ctx.r11.u64 = (input4 + input3);
}

GH_HOOK(sub_825D8E30) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 12156u);
  WriteFields<32, 12156>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 23)) & 1065353216ull) | (field0 & 0xFFFFFFFFC07FFFFFull)));
  ctx.r11.u64 = ((uint64_t(std::rotl(uint32_t(input4), 23)) & 1065353216ull) | (field0 & 0xFFFFFFFFC07FFFFFull));
}

GH_HOOK(sub_82691810) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0>(base, uint32_t(input3), 0xFFFFFFFF823A0AF4ull);
  ctx.r10.u64 = 0xFFFFFFFF823A0AF4ull;
  ctx.r11.u64 = 0xFFFFFFFF823A0000ull;
}

GH_HOOK(sub_8269AD68) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0>(base, uint32_t(input3), 0xFFFFFFFF8239F988ull);
  ctx.r10.u64 = 0xFFFFFFFF8239F988ull;
  ctx.r11.u64 = 0xFFFFFFFF823A0000ull;
}

GH_HOOK(sub_8269BCA8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0>(base, uint32_t(input3), 0xFFFFFFFF823A1800ull);
  ctx.r10.u64 = 0xFFFFFFFF823A1800ull;
  ctx.r11.u64 = 0xFFFFFFFF823A0000ull;
}

GH_HOOK(sub_826CAD80) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0>(base, uint32_t(input3), 0xFFFFFFFF823A26A4ull);
  ctx.r11.u64 = 0xFFFFFFFF823A26A4ull;
}

GH_HOOK(sub_82774B00) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 92u);
  WriteFields<32, 92>(base, uint32_t(input3), input4);
  ctx.r3.u64 = field0;
  ctx.r11.u64 = input3;
}

GH_HOOK(sub_8288E530) {
  GH_PROLOGUE();
  WriteFields<32, 29472>(base, uint32_t(0xFFFFFFFF82A00000ull), 0ull);
  ctx.r10.u64 = 0xFFFFFFFF82A00000ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82410548) {
  GH_PROLOGUE();
  const uint64_t field0 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82980000ull) + -20084u);
  WriteFields<32, -20084>(base, uint32_t(0xFFFFFFFF82980000ull), (field0 & 0xFFFFFFFCull));
  ctx.r10.u64 = 0xFFFFFFFF82980000ull;
  ctx.r11.u64 = (field0 & 0xFFFFFFFCull);
}

GH_HOOK(sub_82410590) {
  GH_PROLOGUE();
  const uint64_t field0 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82980000ull) + -20084u);
  WriteFields<32, -20084>(base, uint32_t(0xFFFFFFFF82980000ull), ((uint64_t(uint32_t((field0 & 0xFFFFFFFFull))) * 0x100000001ull) & 0xFFFFFFFFFFFFFFFBull));
  ctx.r10.u64 = 0xFFFFFFFF82980000ull;
  ctx.r11.u64 = ((uint64_t(uint32_t((field0 & 0xFFFFFFFFull))) * 0x100000001ull) & 0xFFFFFFFFFFFFFFFBull);
}

GH_HOOK(sub_824105A8) {
  GH_PROLOGUE();
  const uint64_t field0 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82980000ull) + -20084u);
  WriteFields<32, -20084>(base, uint32_t(0xFFFFFFFF82980000ull), (field0 | 4ull));
  ctx.r10.u64 = 0xFFFFFFFF82980000ull;
  ctx.r11.u64 = (field0 | 4ull);
}

GH_HOOK(sub_82419D58) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 0>(base, uint32_t(((uint64_t(std::rotl(uint32_t(input4), 2)) & 0xFFFFFFFCull) - 2103972524ull)), input3);
  ctx.r9.u64 = 0xFFFFFFFF8297ED54ull;
  ctx.r10.u64 = (uint64_t(std::rotl(uint32_t(input4), 2)) & 0xFFFFFFFCull);
  ctx.r11.u64 = 0xFFFFFFFF82980000ull;
}

GH_HOOK(sub_825CFF78) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82A00000ull) + 26460u);
  WriteFields<32, 26460>(base, uint32_t(0xFFFFFFFF82A00000ull), input3);
  ctx.r3.u64 = field0;
  ctx.r10.u64 = 0xFFFFFFFF82A00000ull;
  ctx.r11.u64 = input3;
}

GH_HOOK(sub_825EF4D8) {
  GH_PROLOGUE();
  WriteFields<8, -5913>(base, uint32_t(0xFFFFFFFF829D0000ull), 1ull);
  ctx.r3.u64 = 1ull;
  ctx.r10.u64 = 0xFFFFFFFF829D0000ull;
  ctx.r11.u64 = 1ull;
}

GH_HOOK(sub_825EF4F0) {
  GH_PROLOGUE();
  WriteFields<8, -5913>(base, uint32_t(0xFFFFFFFF829D0000ull), 0ull);
  ctx.r3.u64 = 1ull;
  ctx.r10.u64 = 0xFFFFFFFF829D0000ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_825EF5E0) {
  GH_PROLOGUE();
  WriteFields<8, -5915>(base, uint32_t(0xFFFFFFFF829D0000ull), 1ull);
  ctx.r3.u64 = 1ull;
  ctx.r10.u64 = 0xFFFFFFFF829D0000ull;
  ctx.r11.u64 = 1ull;
}

GH_HOOK(sub_825EF5F8) {
  GH_PROLOGUE();
  WriteFields<8, -5916>(base, uint32_t(0xFFFFFFFF829D0000ull), 1ull);
  ctx.r3.u64 = 1ull;
  ctx.r10.u64 = 0xFFFFFFFF829D0000ull;
  ctx.r11.u64 = 1ull;
}

GH_HOOK(sub_8262DF00) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t input6 = ctx.r6.u64;
  WriteFields<8, 0>(base, uint32_t((((uint64_t(std::rotl(uint32_t((input4 + 32ull)), 2)) & 0xFFFFFFFCull) + input5) + input3)), input6);
  ctx.r11.u64 = ((uint64_t(std::rotl(uint32_t((input4 + 32ull)), 2)) & 0xFFFFFFFCull) + input5);
}

GH_HOOK(sub_82651510) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 20u);
  WriteFields<32, 60>(base, uint32_t(input3), 1ull);
  ctx.r3.u64 = field0;
  ctx.r10.u64 = 1ull;
  ctx.r11.u64 = input3;
}

GH_HOOK(sub_8267FB40) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 4u);
  WriteFields<32, 4>(base, uint32_t(input3), (field0 + 1ull));
  ctx.r3.u64 = (field0 + 1ull);
  ctx.r10.u64 = input3;
  ctx.r11.u64 = field0;
}

GH_HOOK(sub_8276AEC8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  WriteFields<32, 500>(base, uint32_t(input3), input4);
  WriteFields<32, 504>(base, uint32_t(input3), input5);
  ctx.r3.u64 = 0ull;
  ctx.r11.u64 = input3;
}

GH_HOOK(sub_828037C0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  WriteFields<8, 1524>(base, uint32_t(input3), input4);
  WriteFields<32, 1528>(base, uint32_t(input3), input5);
  ctx.r3.u64 = 0ull;
  ctx.r11.u64 = input3;
}

GH_HOOK(sub_82803EB0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 512>(base, uint32_t(input3), 0ull);
  ctx.r3.u64 = 0ull;
  ctx.r10.u64 = 0ull;
  ctx.r11.u64 = input3;
}

GH_HOOK(sub_82821D70) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 7200>(base, uint32_t(input3), 0ull);
  ctx.r3.u64 = 0ull;
  ctx.r10.u64 = 0ull;
  ctx.r11.u64 = input3;
}

GH_HOOK(sub_8288EC40) {
  GH_PROLOGUE();
  WriteFields<32, 23560>(base, uint32_t(0xFFFFFFFF82A00000ull), 0xFFFFFFFF8288EBE8ull);
  ctx.r10.u64 = 0xFFFFFFFF82A00000ull;
  ctx.r11.u64 = 0xFFFFFFFF8288EBE8ull;
}

GH_HOOK(sub_82904398) {
  GH_PROLOGUE();
  const uint64_t field0 = GH_LOAD_U32(uint32_t(0xFFFFFFFF823A0000ull) + 10600u);
  WriteFields<32, -13672>(base, uint32_t(0xFFFFFFFF829E0000ull), field0);
  ctx.r10.u64 = 0xFFFFFFFF829E0000ull;
  ctx.r11.u64 = field0;
}

GH_HOOK(sub_82904E98) {
  GH_PROLOGUE();
  WriteFields<32, -27232>(base, uint32_t(0xFFFFFFFF82980000ull), 0xFFFFFFFF823A2BF0ull);
  ctx.r10.u64 = 0xFFFFFFFF82980000ull;
  ctx.r11.u64 = 0xFFFFFFFF823A2BF0ull;
}

GH_HOOK(sub_82410560) {
  GH_PROLOGUE();
  const uint64_t field0 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82980000ull) + -20084u);
  WriteFields<32, -20084>(base, uint32_t(0xFFFFFFFF82980000ull), ((field0 & 0xFFFFFFFEull) | 2ull));
  ctx.r9.u64 = (field0 & 0xFFFFFFFEull);
  ctx.r10.u64 = 0xFFFFFFFF82980000ull;
  ctx.r11.u64 = ((field0 & 0xFFFFFFFEull) | 2ull);
}

GH_HOOK(sub_82410578) {
  GH_PROLOGUE();
  const uint64_t field0 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82980000ull) + -20084u);
  WriteFields<32, -20084>(base, uint32_t(0xFFFFFFFF82980000ull), (1ull | (field0 & 0xFFFFFFFFFFFFFFFCull)));
  ctx.r9.u64 = 1ull;
  ctx.r10.u64 = 0xFFFFFFFF82980000ull;
  ctx.r11.u64 = (1ull | (field0 & 0xFFFFFFFFFFFFFFFCull));
}

GH_HOOK(sub_8242AE70) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 4>(base, uint32_t(input3), 0ull);
  WriteFields<32, 0>(base, uint32_t(input3), 0xFFFFFFFF82009400ull);
  ctx.r9.u64 = 0xFFFFFFFF82009400ull;
  ctx.r10.u64 = 0ull;
  ctx.r11.u64 = 0xFFFFFFFF82010000ull;
}

GH_HOOK(sub_8242AE88) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 4>(base, uint32_t(input3), 0ull);
  WriteFields<32, 0>(base, uint32_t(input3), 0xFFFFFFFF8200E048ull);
  ctx.r9.u64 = 0xFFFFFFFF8200E048ull;
  ctx.r10.u64 = 0ull;
  ctx.r11.u64 = 0xFFFFFFFF82010000ull;
}

GH_HOOK(sub_825D8258) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 10752>(base, uint32_t(input3), (input4 & 65535ull));
  const uint64_t field0 = GH_LOAD_U64(uint32_t(input3) + 32u);
  WriteFields<64, 32>(base, uint32_t(input3), (field0 | 524288ull));
  ctx.r11.u64 = (field0 | 524288ull);
}

GH_HOOK(sub_8269CAA8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  WriteFields<32, 4>(base, uint32_t(input3), input4);
  WriteFields<16, 8>(base, uint32_t(input3), input5);
  WriteFields<32, 0>(base, uint32_t(input3), 0xFFFFFFFF823A1804ull);
  ctx.r10.u64 = 0xFFFFFFFF823A1804ull;
  ctx.r11.u64 = 0xFFFFFFFF823A0000ull;
}

GH_HOOK(sub_826F0DC8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 8u);
  WriteFields<32, 8>(base, uint32_t(input3), (field0 + 1ull));
  ctx.r3.u64 = (field0 + 1ull);
  ctx.r9.u64 = field0;
  ctx.r10.u64 = (input3 + 4ull);
  ctx.r11.u64 = input3;
}

GH_HOOK(sub_826F1570) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 12u);
  WriteFields<32, 12>(base, uint32_t(input3), (field0 + 1ull));
  ctx.r3.u64 = (field0 + 1ull);
  ctx.r9.u64 = field0;
  ctx.r10.u64 = (input3 + 8ull);
  ctx.r11.u64 = input3;
}

GH_HOOK(sub_82707800) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 324u);
  WriteFields<32, 324>(base, uint32_t(input3), (field0 + 1ull));
  ctx.r3.u64 = 0ull;
  ctx.r9.u64 = (field0 + 1ull);
  ctx.r10.u64 = input3;
  ctx.r11.u64 = field0;
}

GH_HOOK(sub_827078C8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 320u);
  WriteFields<32, 320>(base, uint32_t(input3), (field0 + 1ull));
  ctx.r3.u64 = 0ull;
  ctx.r9.u64 = (field0 + 1ull);
  ctx.r10.u64 = input3;
  ctx.r11.u64 = field0;
}

GH_HOOK(sub_82707BE0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<64, 48>(base, uint32_t(input3), 0ull);
  WriteFields<32, 44>(base, uint32_t(input3), 0ull);
  ctx.r3.u64 = 0ull;
  ctx.r10.u64 = input3;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82803898) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 1524>(base, uint32_t(input3), 0ull);
  WriteFields<32, 1528>(base, uint32_t(input3), 0ull);
  ctx.r3.u64 = 0ull;
  ctx.r10.u64 = input3;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82891930) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  WriteFields<32, 4>(base, uint32_t(0xFFFFFFFF8297A0E0ull), input3);
  WriteFields<32, 12>(base, uint32_t(0xFFFFFFFF8297A0E0ull), input4);
  WriteFields<32, 8>(base, uint32_t(0xFFFFFFFF8297A0E0ull), input5);
  ctx.r11.u64 = 0xFFFFFFFF8297A0E0ull;
}

GH_HOOK(sub_82899950) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 4>(base, uint32_t(input3), 0xFFFFFFFFFFFFFFFFull);
  WriteFields<32, 0>(base, uint32_t(input3), 0xFFFFFFFF823EC7B8ull);
  ctx.r9.u64 = 0xFFFFFFFF823EC7B8ull;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFull;
  ctx.r11.u64 = 0xFFFFFFFF823F0000ull;
}

GH_HOOK(sub_824189D0) {
  GH_PROLOGUE();
  WriteFields<32, -4924>(base, uint32_t(0xFFFFFFFF82980000ull), 1073741824ull);
  WriteFields<32, 4, 8>(base, uint32_t(0xFFFFFFFF8297ECC4ull), 1073741824ull);
  ctx.r9.u64 = 0xFFFFFFFF8297ECC4ull;
  ctx.r10.u64 = 0xFFFFFFFF82980000ull;
  ctx.r11.u64 = 1073741824ull;
}

GH_HOOK(sub_82421B90) {
  GH_PROLOGUE();
  const uint64_t field0 = GH_LOAD_U16(uint32_t(0xFFFFFFFF82990000ull) + 5400u);
  WriteFields<16, 5400>(base, uint32_t(0xFFFFFFFF82990000ull), (((uint64_t(uint32_t((field0 & 65535ull))) * 0x100000001ull) & 0xFFFFFFFFFFFFFFF9ull) | 2ull));
  ctx.r9.u64 = ((uint64_t(uint32_t((field0 & 65535ull))) * 0x100000001ull) & 0xFFFFFFFFFFFFFFF9ull);
  ctx.r10.u64 = 0xFFFFFFFF82990000ull;
  ctx.r11.u64 = (((uint64_t(uint32_t((field0 & 65535ull))) * 0x100000001ull) & 0xFFFFFFFFFFFFFFF9ull) | 2ull);
}

GH_HOOK(sub_82421BB0) {
  GH_PROLOGUE();
  const uint64_t field0 = GH_LOAD_U16(uint32_t(0xFFFFFFFF82990000ull) + 5400u);
  WriteFields<16, 5400>(base, uint32_t(0xFFFFFFFF82990000ull), (((uint64_t(uint32_t((field0 & 65535ull))) * 0x100000001ull) & 0xFFFFFFFFFFFFFFF9ull) | 4ull));
  ctx.r9.u64 = ((uint64_t(uint32_t((field0 & 65535ull))) * 0x100000001ull) & 0xFFFFFFFFFFFFFFF9ull);
  ctx.r10.u64 = 0xFFFFFFFF82990000ull;
  ctx.r11.u64 = (((uint64_t(uint32_t((field0 & 65535ull))) * 0x100000001ull) & 0xFFFFFFFFFFFFFFF9ull) | 4ull);
}

GH_HOOK(sub_82434948) {
  GH_PROLOGUE();
  WriteFields<8, 8716>(base, uint32_t(0xFFFFFFFF829A0000ull), 0ull);
  WriteFields<32, 8720>(base, uint32_t(0xFFFFFFFF829A0000ull), 0ull);
  ctx.r8.u64 = 0xFFFFFFFF829A0000ull;
  ctx.r9.u64 = 0xFFFFFFFF829A0000ull;
  ctx.r10.u64 = 0ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_824A4948) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 1726>(base, uint32_t(input3), 0ull);
  WriteFields<32, 1192>(base, uint32_t(input3), 4096ull);
  WriteFields<32, 1200>(base, uint32_t(input3), 4194304ull);
  ctx.r9.u64 = 4194304ull;
  ctx.r10.u64 = 4096ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_825D75B0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10568u);
  WriteFields<32, 10568>(base, uint32_t(input3), ((field0 & 0xFFFFFFF8ull) | (input4 & 0xFFFFFFFF00000007ull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 64ull));
  ctx.r4.u64 = ((field0 & 0xFFFFFFF8ull) | (input4 & 0xFFFFFFFF00000007ull));
  ctx.r11.u64 = (field1 | 64ull);
}

GH_HOOK(sub_825D75E0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10568u);
  WriteFields<32, 10568>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 3)) & 2040ull) | (field0 & 0xFFFFFFFFFFFFF807ull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 64ull));
  ctx.r11.u64 = (field1 | 64ull);
}

GH_HOOK(sub_825D7AD0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10556u);
  WriteFields<32, 10556>(base, uint32_t(input3), ((field0 & 0xFFFFFFF8ull) | (input4 & 0xFFFFFFFF00000007ull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 512ull));
  ctx.r4.u64 = ((field0 & 0xFFFFFFF8ull) | (input4 & 0xFFFFFFFF00000007ull));
  ctx.r11.u64 = (field1 | 512ull);
}

GH_HOOK(sub_825D7D18) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10548u);
  WriteFields<32, 10548>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 2)) & 4ull) | (field0 & 0xFFFFFFFFFFFFFFFBull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 2048ull));
  ctx.r11.u64 = (field1 | 2048ull);
}

GH_HOOK(sub_825D7DF8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10548u);
  WriteFields<32, 10548>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 8)) & 1792ull) | (field0 & 0xFFFFFFFFFFFFF8FFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 2048ull));
  ctx.r11.u64 = (field1 | 2048ull);
}

GH_HOOK(sub_825D7E98) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10548u);
  WriteFields<32, 10548>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 14)) & 114688ull) | (field0 & 0xFFFFFFFFFFFE3FFFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 2048ull));
  ctx.r11.u64 = (field1 | 2048ull);
}

GH_HOOK(sub_825D7EC8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10548u);
  WriteFields<32, 10548>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 20)) & 7340032ull) | (field0 & 0xFFFFFFFFFF8FFFFFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 2048ull));
  ctx.r11.u64 = (field1 | 2048ull);
}

GH_HOOK(sub_825D7F68) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10548u);
  WriteFields<32, 10548>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 26)) & 469762048ull) | (field0 & 0xFFFFFFFFE3FFFFFFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 2048ull));
  ctx.r11.u64 = (field1 | 2048ull);
}

GH_HOOK(sub_825D8228) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10568u);
  WriteFields<32, 10568>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 15)) & 32768ull) | (field0 & 0xFFFFFFFFFFFF7FFFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 64ull));
  ctx.r11.u64 = (field1 | 64ull);
}

GH_HOOK(sub_825D8590) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10540u);
  WriteFields<32, 10540>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 28)) & 0xF0000000ull) | (field0 & 0xFFFFFFFF0FFFFFFFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 8192ull));
  ctx.r11.u64 = (field1 | 8192ull);
}

GH_HOOK(sub_825D86C0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10544u);
  WriteFields<32, 10544>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 28)) & 0xF0000000ull) | (field0 & 0xFFFFFFFF0FFFFFFFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 4096ull));
  ctx.r11.u64 = (field1 | 4096ull);
}

GH_HOOK(sub_825D8B70) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10568u);
  WriteFields<32, 10568>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 21)) & 2097152ull) | (field0 & 0xFFFFFFFFFFDFFFFFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 64ull));
  ctx.r11.u64 = (field1 | 64ull);
}

GH_HOOK(sub_825D8BA0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 10456>(base, uint32_t(input3), input4);
  const uint64_t field0 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field0 | 0x4000000000ull));
  ctx.r10.u64 = field0;
  ctx.r11.u64 = (field0 | 0x4000000000ull);
  ctx.r12.u64 = 0x4000000000ull;
}

GH_HOOK(sub_825D8BC8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10556u);
  WriteFields<32, 10556>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 4)) & 16ull) | (field0 & 0xFFFFFFFFFFFFFFEFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 512ull));
  ctx.r11.u64 = (field1 | 512ull);
}

GH_HOOK(sub_825D8BF8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10556u);
  WriteFields<32, 10556>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 24)) & 0xFF000000ull) | (field0 & 0xFFFFFFFF00FFFFFFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 512ull));
  ctx.r11.u64 = (field1 | 512ull);
}

GH_HOOK(sub_825D8CD8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 12156u);
  WriteFields<32, 12156>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 20)) & 7340032ull) | (field0 & 0xFFFFFFFFFF8FFFFFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 131072ull));
  ctx.r11.u64 = (field1 | 131072ull);
}

GH_HOOK(sub_825D8D70) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10560u);
  WriteFields<32, 10560>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 3)) & 8ull) | (field0 & 0xFFFFFFFFFFFFFFF7ull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 256ull));
  ctx.r11.u64 = (field1 | 256ull);
}

GH_HOOK(sub_825D8DA0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10560u);
  WriteFields<32, 10560>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 2)) & 4ull) | (field0 & 0xFFFFFFFFFFFFFFFBull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 256ull));
  ctx.r11.u64 = (field1 | 256ull);
}

GH_HOOK(sub_825D8DD0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10560u);
  WriteFields<32, 10560>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 5)) & 32ull) | (field0 & 0xFFFFFFFFFFFFFFDFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 256ull));
  ctx.r11.u64 = (field1 | 256ull);
}

GH_HOOK(sub_826C9770) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 4>(base, uint32_t(input3), 0ull);
  WriteFields<32, 0>(base, uint32_t(input3), 0xFFFFFFFF823A269Cull);
  WriteFields<32, 8>(base, uint32_t(input3), 0ull);
  ctx.r10.u64 = 0xFFFFFFFF823A269Cull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_826CF0E8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 28u);
  WriteFields<32, 0>(base, uint32_t(input4), field0);
  const uint64_t field1 = GH_LOAD_U16(uint32_t(input3) + 32u);
  WriteFields<16, 4>(base, uint32_t(input4), field1);
  ctx.r3.u64 = 0ull;
  ctx.r10.u64 = field0;
  ctx.r11.u64 = field1;
}

GH_HOOK(sub_826DA900) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 3>(base, uint32_t(input3), input4);
  WriteFields<16, 0>(base, uint32_t(input3), 5ull);
  WriteFields<8, 2>(base, uint32_t(input3), 5ull);
  ctx.r3.u64 = 7ull;
  ctx.r10.u64 = input3;
  ctx.r11.u64 = 5ull;
}

GH_HOOK(sub_82707DF8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 52u);
  WriteFields<32, 52>(base, uint32_t(input3), ((input4 & 0xFFFFFFFFull) + field0));
  ctx.r3.u64 = 0ull;
  ctx.r8.u64 = ((input4 & 0xFFFFFFFFull) + field0);
  ctx.r9.u64 = input3;
  ctx.r10.u64 = (input4 & 0xFFFFFFFFull);
  ctx.r11.u64 = field0;
}

GH_HOOK(sub_8283C078) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 2156>(base, uint32_t(input3), 0xFFFFFFFF8283BCE8ull);
  WriteFields<32, 2160>(base, uint32_t(input3), 0xFFFFFFFF8283BEB8ull);
  ctx.r8.u64 = 0xFFFFFFFF8283BEB8ull;
  ctx.r9.u64 = 0xFFFFFFFF8283BCE8ull;
  ctx.r10.u64 = 0xFFFFFFFF82840000ull;
  ctx.r11.u64 = 0xFFFFFFFF82840000ull;
}

GH_HOOK(sub_82873180) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 7088>(base, uint32_t(input3), 0xFFFFFFFF828727C8ull);
  WriteFields<32, 7084>(base, uint32_t(input3), 0xFFFFFFFF828726B8ull);
  ctx.r8.u64 = 0xFFFFFFFF828726B8ull;
  ctx.r9.u64 = 0xFFFFFFFF828727C8ull;
  ctx.r10.u64 = 0xFFFFFFFF82870000ull;
  ctx.r11.u64 = 0xFFFFFFFF82870000ull;
}

GH_HOOK(sub_82890350) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82980000ull) + -24944u);
  WriteFields<32, -24944>(base, uint32_t(0xFFFFFFFF82980000ull), ((field0 & ~input4) | (input3 & input4)));
  ctx.r3.u64 = field0;
  ctx.r9.u64 = (field0 & ~input4);
  ctx.r10.u64 = 0xFFFFFFFF82980000ull;
  ctx.r11.u64 = ((field0 & ~input4) | (input3 & input4));
}

GH_HOOK(sub_82899430) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t input6 = ctx.r6.u64;
  const uint64_t input7 = ctx.r7.u64;
  WriteFields<32, 0>(base, uint32_t(input3), input4);
  WriteFields<32, 4>(base, uint32_t(input3), input5);
  WriteFields<32, 8>(base, uint32_t(input3), input6);
  WriteFields<32, 12>(base, uint32_t(input3), input7);
  ctx.r3.u64 = 1ull;
  ctx.r11.u64 = input3;
}

GH_HOOK(sub_82411D48) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t value0 = (((uint64_t(std::rotl(uint32_t(input3), 8)) & 65280ull) | 0xFFFFFFFFFFFF0000ull) | (input4 & 255ull));
  WriteFields<32, -17436>(base, uint32_t(0xFFFFFFFF82980000ull), ((uint64_t(std::rotl(uint32_t(value0), 8)) & 0xFFFFFF00ull) | (input5 & 0xFFFFFFFF000000FFull)));
  ctx.r5.u64 = ((uint64_t(std::rotl(uint32_t(value0), 8)) & 0xFFFFFF00ull) | (input5 & 0xFFFFFFFF000000FFull));
  ctx.r8.u64 = 0xFFFFFFFF82980000ull;
  ctx.r9.u64 = value0;
  ctx.r10.u64 = (input4 & 255ull);
  ctx.r11.u64 = ((uint64_t(std::rotl(uint32_t(input3), 8)) & 65280ull) | 0xFFFFFFFFFFFF0000ull);
}

GH_HOOK(sub_825D8480) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10540u);
  WriteFields<32, 10540>(base, uint32_t(input3), ((field0 & 0xFFFFFFF0ull) | input4));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 8192ull));
  ctx.r11.u64 = (field1 | 8192ull);
}

GH_HOOK(sub_825D85B0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10544u);
  WriteFields<32, 10544>(base, uint32_t(input3), ((field0 & 0xFFFFFFF0ull) | input4));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 4096ull));
  ctx.r11.u64 = (field1 | 4096ull);
}

GH_HOOK(sub_82707BC0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U64(uint32_t(input3) + 48u);
  WriteFields<32, 44>(base, uint32_t(input3), 0ull);
  WriteFields<64, 48>(base, uint32_t(input3), (input4 + field0));
  ctx.r3.u64 = 0ull;
  ctx.r8.u64 = (input4 + field0);
  ctx.r9.u64 = 0ull;
  ctx.r10.u64 = input3;
  ctx.r11.u64 = field0;
}

GH_HOOK(sub_825D7C00) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10680u);
  WriteFields<32, 10680>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 10)) & 1024ull) | (field0 & 0xFFFFFFFFFFFFFBFFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 32u);
  WriteFields<64, 32>(base, uint32_t(input3), (field1 | 0x2000000000ull));
  ctx.r11.u64 = (field1 | 0x2000000000ull);
  ctx.r12.u64 = 0x2000000000ull;
}

GH_HOOK(sub_825D7D48) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10548u);
  WriteFields<32, 10548>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 4)) & 112ull) | (field0 & 0xFFFFFFFFFFFFFF8Full)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 2048ull));
  WriteFields<64, 16>(base, uint32_t(input3), ((field1 | 2048ull) | 131072ull));
  ctx.r11.u64 = ((field1 | 2048ull) | 131072ull);
}

GH_HOOK(sub_825D7DC0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10548u);
  WriteFields<32, 10548>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 7)) & 128ull) | (field0 & 0xFFFFFFFFFFFFFF7Full)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 2048ull));
  WriteFields<64, 16>(base, uint32_t(input3), ((field1 | 2048ull) | 131072ull));
  ctx.r11.u64 = ((field1 | 2048ull) | 131072ull);
}

GH_HOOK(sub_825D7E28) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10548u);
  WriteFields<32, 10548>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 11)) & 14336ull) | (field0 & 0xFFFFFFFFFFFFC7FFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 2048ull));
  WriteFields<64, 16>(base, uint32_t(input3), ((field1 | 2048ull) | 131072ull));
  ctx.r11.u64 = ((field1 | 2048ull) | 131072ull);
}

GH_HOOK(sub_825D7E60) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10548u);
  WriteFields<32, 10548>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 17)) & 917504ull) | (field0 & 0xFFFFFFFFFFF1FFFFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 2048ull));
  WriteFields<64, 16>(base, uint32_t(input3), ((field1 | 2048ull) | 131072ull));
  ctx.r11.u64 = ((field1 | 2048ull) | 131072ull);
}

GH_HOOK(sub_825D7EF8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10548u);
  WriteFields<32, 10548>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 23)) & 58720256ull) | (field0 & 0xFFFFFFFFFC7FFFFFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 2048ull));
  WriteFields<64, 16>(base, uint32_t(input3), ((field1 | 2048ull) | 131072ull));
  ctx.r11.u64 = ((field1 | 2048ull) | 131072ull);
}

GH_HOOK(sub_825D7F30) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10548u);
  WriteFields<32, 10548>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(input4), 29)) & 0xE0000000ull) | (field0 & 0xFFFFFFFF1FFFFFFFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 2048ull));
  WriteFields<64, 16>(base, uint32_t(input3), ((field1 | 2048ull) | 131072ull));
  ctx.r11.u64 = ((field1 | 2048ull) | 131072ull);
}

GH_HOOK(sub_825D84A0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10540u);
  const uint64_t value0 = (((uint64_t(uint32_t((field0 & 0xFFFFFFFFull))) * 0x100000001ull) & 0xFFFFFFFFFFFFFF0Full) | (uint64_t(std::rotl(uint32_t(input4), 4)) & 0xFFFFFFF0ull));
  WriteFields<32, 10540>(base, uint32_t(input3), value0);
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 8192ull));
  ctx.r10.u64 = (uint64_t(std::rotl(uint32_t(input4), 4)) & 0xFFFFFFF0ull);
  ctx.r11.u64 = (field1 | 8192ull);
}

GH_HOOK(sub_825D84C8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10540u);
  const uint64_t value0 = (((uint64_t(uint32_t((field0 & 0xFFFFFFFFull))) * 0x100000001ull) & 0xFFFFFFFFFFFFF0FFull) | (uint64_t(std::rotl(uint32_t(input4), 8)) & 0xFFFFFF00ull));
  WriteFields<32, 10540>(base, uint32_t(input3), value0);
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 8192ull));
  ctx.r10.u64 = (uint64_t(std::rotl(uint32_t(input4), 8)) & 0xFFFFFF00ull);
  ctx.r11.u64 = (field1 | 8192ull);
}

GH_HOOK(sub_825D84F0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10540u);
  const uint64_t value0 = (((uint64_t(uint32_t((field0 & 0xFFFFFFFFull))) * 0x100000001ull) & 0xFFFFFFFFFFFF0FFFull) | (uint64_t(std::rotl(uint32_t(input4), 12)) & 0xFFFFF000ull));
  WriteFields<32, 10540>(base, uint32_t(input3), value0);
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 8192ull));
  ctx.r10.u64 = (uint64_t(std::rotl(uint32_t(input4), 12)) & 0xFFFFF000ull);
  ctx.r11.u64 = (field1 | 8192ull);
}

GH_HOOK(sub_825D8518) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10540u);
  const uint64_t value0 = (((uint64_t(uint32_t((field0 & 0xFFFFFFFFull))) * 0x100000001ull) & 0xFFFFFFFFFFF0FFFFull) | (uint64_t(std::rotl(uint32_t(input4), 16)) & 0xFFFF0000ull));
  WriteFields<32, 10540>(base, uint32_t(input3), value0);
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 8192ull));
  ctx.r10.u64 = (uint64_t(std::rotl(uint32_t(input4), 16)) & 0xFFFF0000ull);
  ctx.r11.u64 = (field1 | 8192ull);
}

GH_HOOK(sub_825D8540) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10540u);
  const uint64_t value0 = (((uint64_t(uint32_t((field0 & 0xFFFFFFFFull))) * 0x100000001ull) & 0xFFFFFFFFFF0FFFFFull) | (uint64_t(std::rotl(uint32_t(input4), 20)) & 0xFFF00000ull));
  WriteFields<32, 10540>(base, uint32_t(input3), value0);
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 8192ull));
  ctx.r10.u64 = (uint64_t(std::rotl(uint32_t(input4), 20)) & 0xFFF00000ull);
  ctx.r11.u64 = (field1 | 8192ull);
}

GH_HOOK(sub_825D8568) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10540u);
  const uint64_t value0 = (((uint64_t(uint32_t((field0 & 0xFFFFFFFFull))) * 0x100000001ull) & 0xFFFFFFFFF0FFFFFFull) | (uint64_t(std::rotl(uint32_t(input4), 24)) & 0xFF000000ull));
  WriteFields<32, 10540>(base, uint32_t(input3), value0);
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 8192ull));
  ctx.r10.u64 = (uint64_t(std::rotl(uint32_t(input4), 24)) & 0xFF000000ull);
  ctx.r11.u64 = (field1 | 8192ull);
}

GH_HOOK(sub_825D85D0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10544u);
  const uint64_t value0 = (((uint64_t(uint32_t((field0 & 0xFFFFFFFFull))) * 0x100000001ull) & 0xFFFFFFFFFFFFFF0Full) | (uint64_t(std::rotl(uint32_t(input4), 4)) & 0xFFFFFFF0ull));
  WriteFields<32, 10544>(base, uint32_t(input3), value0);
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 4096ull));
  ctx.r10.u64 = (uint64_t(std::rotl(uint32_t(input4), 4)) & 0xFFFFFFF0ull);
  ctx.r11.u64 = (field1 | 4096ull);
}

GH_HOOK(sub_825D85F8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10544u);
  const uint64_t value0 = (((uint64_t(uint32_t((field0 & 0xFFFFFFFFull))) * 0x100000001ull) & 0xFFFFFFFFFFFFF0FFull) | (uint64_t(std::rotl(uint32_t(input4), 8)) & 0xFFFFFF00ull));
  WriteFields<32, 10544>(base, uint32_t(input3), value0);
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 4096ull));
  ctx.r10.u64 = (uint64_t(std::rotl(uint32_t(input4), 8)) & 0xFFFFFF00ull);
  ctx.r11.u64 = (field1 | 4096ull);
}

GH_HOOK(sub_825D8620) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10544u);
  const uint64_t value0 = (((uint64_t(uint32_t((field0 & 0xFFFFFFFFull))) * 0x100000001ull) & 0xFFFFFFFFFFFF0FFFull) | (uint64_t(std::rotl(uint32_t(input4), 12)) & 0xFFFFF000ull));
  WriteFields<32, 10544>(base, uint32_t(input3), value0);
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 4096ull));
  ctx.r10.u64 = (uint64_t(std::rotl(uint32_t(input4), 12)) & 0xFFFFF000ull);
  ctx.r11.u64 = (field1 | 4096ull);
}

GH_HOOK(sub_825D8648) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10544u);
  const uint64_t value0 = (((uint64_t(uint32_t((field0 & 0xFFFFFFFFull))) * 0x100000001ull) & 0xFFFFFFFFFFF0FFFFull) | (uint64_t(std::rotl(uint32_t(input4), 16)) & 0xFFFF0000ull));
  WriteFields<32, 10544>(base, uint32_t(input3), value0);
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 4096ull));
  ctx.r10.u64 = (uint64_t(std::rotl(uint32_t(input4), 16)) & 0xFFFF0000ull);
  ctx.r11.u64 = (field1 | 4096ull);
}

GH_HOOK(sub_825D8670) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10544u);
  const uint64_t value0 = (((uint64_t(uint32_t((field0 & 0xFFFFFFFFull))) * 0x100000001ull) & 0xFFFFFFFFFF0FFFFFull) | (uint64_t(std::rotl(uint32_t(input4), 20)) & 0xFFF00000ull));
  WriteFields<32, 10544>(base, uint32_t(input3), value0);
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 4096ull));
  ctx.r10.u64 = (uint64_t(std::rotl(uint32_t(input4), 20)) & 0xFFF00000ull);
  ctx.r11.u64 = (field1 | 4096ull);
}

GH_HOOK(sub_825D8698) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10544u);
  const uint64_t value0 = (((uint64_t(uint32_t((field0 & 0xFFFFFFFFull))) * 0x100000001ull) & 0xFFFFFFFFF0FFFFFFull) | (uint64_t(std::rotl(uint32_t(input4), 24)) & 0xFF000000ull));
  WriteFields<32, 10544>(base, uint32_t(input3), value0);
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field1 | 4096ull));
  ctx.r10.u64 = (uint64_t(std::rotl(uint32_t(input4), 24)) & 0xFF000000ull);
  ctx.r11.u64 = (field1 | 4096ull);
}

GH_HOOK(sub_825D8B00) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 10616u);
  WriteFields<32, 10616>(base, uint32_t(input3), ((field0 & 0xFFFFFFFCull) | (input4 & 0xFFFFFFFF00000003ull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 24u);
  WriteFields<64, 24>(base, uint32_t(input3), (field1 | 0x2000000000000ull));
  ctx.r4.u64 = ((field0 & 0xFFFFFFFCull) | (input4 & 0xFFFFFFFF00000003ull));
  ctx.r11.u64 = (field1 | 0x2000000000000ull);
  ctx.r12.u64 = 0x2000000000000ull;
}

GH_HOOK(sub_8269AD40) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U8(uint32_t(input3) + 40u);
  WriteFields<32, 20, 24, 32, 28>(base, uint32_t(input3), 0ull);
  WriteFields<8, 40>(base, uint32_t(input3), (field0 & 8ull));
  ctx.r9.u64 = (field0 & 8ull);
  ctx.r10.u64 = field0;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_826CAF20) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 4>(base, uint32_t(input3), 1ull);
  WriteFields<32, 0>(base, uint32_t(input3), 0xFFFFFFFF823A26A4ull);
  WriteFields<32, 8, 12>(base, uint32_t(input3), (input3 + 8ull));
  ctx.r9.u64 = 0xFFFFFFFF823A26A4ull;
  ctx.r10.u64 = 1ull;
  ctx.r11.u64 = (input3 + 8ull);
}

GH_HOOK(sub_826DA850) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  WriteFields<32, 3>(base, uint32_t(input3), input4);
  WriteFields<32, 7>(base, uint32_t(input3), input5);
  WriteFields<16, 0>(base, uint32_t(input3), 9ull);
  WriteFields<8, 2>(base, uint32_t(input3), 3ull);
  ctx.r3.u64 = 11ull;
  ctx.r9.u64 = 3ull;
  ctx.r10.u64 = 9ull;
  ctx.r11.u64 = input3;
}

GH_HOOK(sub_82421B18) {
  GH_PROLOGUE();
  WriteFields<16, 5400>(base, uint32_t(0xFFFFFFFF82990000ull), 0ull);
  WriteFields<32, 5396, 5392>(base, uint32_t(0xFFFFFFFF82990000ull), 0ull);
  ctx.r6.u64 = 0xFFFFFFFF82990000ull;
  ctx.r7.u64 = 0xFFFFFFFF82990000ull;
  ctx.r8.u64 = 0xFFFFFFFF82990000ull;
  ctx.r9.u64 = 0ull;
  ctx.r10.u64 = 0ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_826506A0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t input6 = ctx.r6.u64;
  const uint64_t input7 = ctx.r7.u64;
  WriteFields<32, 4>(base, uint32_t(input3), input7);
  WriteFields<32, 8>(base, uint32_t(input3), input6);
  WriteFields<32, 12>(base, uint32_t(input3), input4);
  WriteFields<32, 16>(base, uint32_t(input3), input5);
  WriteFields<32, 0>(base, uint32_t(input3), 0xFFFFFFFF823823F8ull);
  WriteFields<32, 20>(base, uint32_t(input3), 0ull);
  ctx.r10.u64 = 0ull;
  ctx.r11.u64 = 0xFFFFFFFF823823F8ull;
}

GH_HOOK(sub_826DA8A0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  WriteFields<32, 5>(base, uint32_t(input3), input4);
  WriteFields<16, 0>(base, uint32_t(input3), (input5 + 7ull));
  WriteFields<8, 2>(base, uint32_t(input3), 4ull);
  WriteFields<16, 3>(base, uint32_t(input3), 256ull);
  ctx.r3.u64 = 9ull;
  ctx.r8.u64 = 256ull;
  ctx.r9.u64 = 4ull;
  ctx.r10.u64 = input3;
  ctx.r11.u64 = (input5 + 7ull);
}

GH_HOOK(sub_827502D0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 15624>(base, uint32_t(input3), 1ull);
  WriteFields<32, 15628>(base, uint32_t(input3), 0ull);
  WriteFields<32, 15600, 460, 3492>(base, uint32_t(input3), 1ull);
  ctx.r3.u64 = 0ull;
  ctx.r9.u64 = 0ull;
  ctx.r10.u64 = input3;
  ctx.r11.u64 = 1ull;
}

GH_HOOK(sub_82863090) {
  GH_PROLOGUE();
  const uint64_t input1 = ctx.r1.u64;
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input6 = ctx.r6.u64;
  WriteFields<32, -16>(base, uint32_t(input1), (uint64_t(std::rotl(uint32_t(input3), 4)) & 0xFFFFFFF0ull));
  const uint64_t field0 = GH_LOAD_U8(uint32_t(input1) + -14u);
  const uint64_t field1 = GH_LOAD_U16(uint32_t(input1) + -16u);
  WriteFields<16, 0>(base, uint32_t(((input6 + (uint64_t(std::rotl(uint32_t(input6), 1)) & 0xFFFFFFFEull)) + input4)), field1);
  WriteFields<8, 2>(base, uint32_t(((input6 + (uint64_t(std::rotl(uint32_t(input6), 1)) & 0xFFFFFFFEull)) + input4)), field0);
  ctx.r9.u64 = field0;
  ctx.r10.u64 = field1;
  ctx.r11.u64 = ((input6 + (uint64_t(std::rotl(uint32_t(input6), 1)) & 0xFFFFFFFEull)) + input4);
}

GH_HOOK(sub_8242B7F0) {
  GH_PROLOGUE();
  WriteFields<64, 6344>(base, uint32_t(0xFFFFFFFF829A0000ull), 0ull);
  WriteFields<64, 8, 16, 24>(base, uint32_t(0xFFFFFFFF829A18C8ull), 0ull);
  ctx.r6.u64 = 0xFFFFFFFF829A18C8ull;
  ctx.r7.u64 = 0xFFFFFFFF829A0000ull;
  ctx.r8.u64 = 0ull;
  ctx.r9.u64 = 0ull;
  ctx.r10.u64 = 0ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82434860) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 8u);
  const uint64_t field1 = GH_LOAD_U32(uint32_t(input3) + 4u);
  WriteFields<32, 8>(base, uint32_t(input3), (field0 + 1ull));
  const uint64_t value0 = uint64_t(int64_t(std::bit_cast<int32_t>(uint32_t(field1))) * int64_t(std::bit_cast<int32_t>(uint32_t(1103515245ull))));
  WriteFields<32, 4>(base, uint32_t(input3), (value0 + 12345ull));
  ctx.r3.u64 = (value0 + 12345ull);
  ctx.r5.u64 = (field0 + 1ull);
  ctx.r6.u64 = 1103515245ull;
  ctx.r7.u64 = field1;
  ctx.r8.u64 = 1103495168ull;
  ctx.r9.u64 = input3;
  ctx.r10.u64 = field0;
  ctx.r11.u64 = value0;
}

GH_HOOK(sub_82623768) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t input6 = ctx.r6.u64;
  WriteFields<32, 0>(base, uint32_t(input3), 9ull);
  WriteFields<32, 0>(base, uint32_t(input4), 5ull);
  WriteFields<32, 0>(base, uint32_t(input5), 0xFFFFFFFF8236EAB0ull);
  WriteFields<32, 0>(base, uint32_t(input6), 0xFFFFFFFF8236FAB0ull);
  ctx.r3.u64 = 0ull;
  ctx.r9.u64 = 5ull;
  ctx.r10.u64 = 0xFFFFFFFF8236EAB0ull;
  ctx.r11.u64 = 0xFFFFFFFF8236FAB0ull;
}

GH_HOOK(sub_8243EC58) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<16, 0, 2>(base, uint32_t(input3), 0ull);
  WriteFields<8, 4, 5, 6, 7, 8, 9, 10, 11, 12>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_827163F0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U8(uint32_t(input3) + 0u);
  const uint64_t field1 = GH_LOAD_U8(uint32_t(input3) + 5u);
  const uint64_t field2 = GH_LOAD_U8(uint32_t(input3) + 4u);
  const uint64_t field3 = GH_LOAD_U8(uint32_t(input3) + 3u);
  const uint64_t field4 = GH_LOAD_U8(uint32_t(input3) + 1u);
  const uint64_t field5 = GH_LOAD_U8(uint32_t(input3) + 2u);
  WriteFields<8, 5>(base, uint32_t(input3), field0);
  WriteFields<8, 0>(base, uint32_t(input3), field1);
  WriteFields<8, 1>(base, uint32_t(input3), field2);
  WriteFields<8, 4>(base, uint32_t(input3), field4);
  WriteFields<8, 2>(base, uint32_t(input3), field3);
  WriteFields<8, 3>(base, uint32_t(input3), field5);
  ctx.r6.u64 = field5;
  ctx.r7.u64 = field4;
  ctx.r8.u64 = field3;
  ctx.r9.u64 = field2;
  ctx.r10.u64 = field1;
  ctx.r11.u64 = field0;
}

GH_HOOK(sub_8282CC90) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 27972>(base, uint32_t(input3), 0xFFFFFFFF8282C278ull);
  WriteFields<32, 27976>(base, uint32_t(input3), 0xFFFFFFFF8282C340ull);
  WriteFields<32, 27980>(base, uint32_t(input3), 0xFFFFFFFF8282C408ull);
  WriteFields<32, 27984>(base, uint32_t(input3), 0xFFFFFFFF8282C4F0ull);
  ctx.r4.u64 = 0xFFFFFFFF8282C4F0ull;
  ctx.r5.u64 = 0xFFFFFFFF8282C408ull;
  ctx.r6.u64 = 0xFFFFFFFF8282C340ull;
  ctx.r7.u64 = 0xFFFFFFFF8282C278ull;
  ctx.r8.u64 = 0xFFFFFFFF82830000ull;
  ctx.r9.u64 = 0xFFFFFFFF82830000ull;
  ctx.r10.u64 = 0xFFFFFFFF82830000ull;
  ctx.r11.u64 = 0xFFFFFFFF82830000ull;
}

GH_HOOK(sub_82419218) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 8>(base, uint32_t(input3), 0ull);
  WriteFields<32, 0>(base, uint32_t(input3), 0xFFFFFFFF82001EC0ull);
  WriteFields<32, 116>(base, uint32_t(input3), 15ull);
  WriteFields<32, 112>(base, uint32_t(input3), 0ull);
  WriteFields<8, 96>(base, uint32_t(input3), 0ull);
  WriteFields<32, 4>(base, uint32_t(input3), 12ull);
  WriteFields<32, 12>(base, uint32_t(input3), 0ull);
  WriteFields<8, 124>(base, uint32_t(input3), 0ull);
  ctx.r7.u64 = 12ull;
  ctx.r8.u64 = 15ull;
  ctx.r9.u64 = 0xFFFFFFFF82001EC0ull;
  ctx.r10.u64 = 0xFFFFFFFF82000000ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8248B408) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0, 8>(base, uint32_t(input3), 0ull);
  WriteFields<8, 12, 14, 13>(base, uint32_t(input3), 0ull);
  WriteFields<8, 16, 15>(base, uint32_t(input3), 1ull);
  WriteFields<8, 17>(base, uint32_t(input3), 0ull);
  WriteFields<32, 20>(base, uint32_t(input3), 0ull);
  WriteFields<8, 24>(base, uint32_t(input3), 0ull);
  WriteFields<32, 4>(base, uint32_t(input3), 0ull);
  ctx.r10.u64 = 1ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8248B440) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0, 8>(base, uint32_t(input3), 0ull);
  WriteFields<8, 12, 14, 13>(base, uint32_t(input3), 0ull);
  WriteFields<8, 16, 15>(base, uint32_t(input3), 1ull);
  WriteFields<8, 17>(base, uint32_t(input3), 0ull);
  WriteFields<32, 20>(base, uint32_t(input3), 0ull);
  WriteFields<8, 24>(base, uint32_t(input3), 0ull);
  WriteFields<32, 4>(base, uint32_t(input3), 1ull);
  ctx.r10.u64 = 1ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_825D95C8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t((((input4 + 48ull) * 24ull) + input3)) + 0u);
  WriteFields<32, 0>(base, uint32_t((((input4 + 48ull) * 24ull) + input3)), ((uint64_t(std::rotl(uint32_t(input5), 10)) & 7168ull) | (field0 & 0xFFFFFFFFFFFFE3FFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 24u);
  const uint64_t value0 = ((((input4 + 32ull) & 0xFFFFFFFFull) & 64ull) ? 0ull : (0x8000000000000000ull >> (((input4 + 32ull) & 0xFFFFFFFFull) & 63ull)));
  WriteFields<64, 24>(base, uint32_t(input3), (value0 | field1));
  ctx.r8.u64 = 1ull;
  ctx.r9.u64 = ((uint64_t(std::rotl(uint32_t(input5), 10)) & 7168ull) | (field0 & 0xFFFFFFFFFFFFE3FFull));
  ctx.r10.u64 = field1;
  ctx.r11.u64 = (value0 | field1);
}

GH_HOOK(sub_825D9618) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t((((input4 + 48ull) * 24ull) + input3)) + 0u);
  WriteFields<32, 0>(base, uint32_t((((input4 + 48ull) * 24ull) + input3)), ((uint64_t(std::rotl(uint32_t(input5), 13)) & 57344ull) | (field0 & 0xFFFFFFFFFFFF1FFFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 24u);
  const uint64_t value0 = ((((input4 + 32ull) & 0xFFFFFFFFull) & 64ull) ? 0ull : (0x8000000000000000ull >> (((input4 + 32ull) & 0xFFFFFFFFull) & 63ull)));
  WriteFields<64, 24>(base, uint32_t(input3), (value0 | field1));
  ctx.r8.u64 = 1ull;
  ctx.r9.u64 = ((uint64_t(std::rotl(uint32_t(input5), 13)) & 57344ull) | (field0 & 0xFFFFFFFFFFFF1FFFull));
  ctx.r10.u64 = field1;
  ctx.r11.u64 = (value0 | field1);
}

GH_HOOK(sub_825D9668) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t((((input4 + 48ull) * 24ull) + input3)) + 0u);
  WriteFields<32, 0>(base, uint32_t((((input4 + 48ull) * 24ull) + input3)), ((uint64_t(std::rotl(uint32_t(input5), 16)) & 458752ull) | (field0 & 0xFFFFFFFFFFF8FFFFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 24u);
  const uint64_t value0 = ((((input4 + 32ull) & 0xFFFFFFFFull) & 64ull) ? 0ull : (0x8000000000000000ull >> (((input4 + 32ull) & 0xFFFFFFFFull) & 63ull)));
  WriteFields<64, 24>(base, uint32_t(input3), (value0 | field1));
  ctx.r8.u64 = 1ull;
  ctx.r9.u64 = ((uint64_t(std::rotl(uint32_t(input5), 16)) & 458752ull) | (field0 & 0xFFFFFFFFFFF8FFFFull));
  ctx.r10.u64 = field1;
  ctx.r11.u64 = (value0 | field1);
}

GH_HOOK(sub_825DD468) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<64, 0, 8, 16, 24, 32>(base, uint32_t(input3), 0ull);
  WriteFields<32, 0>(base, uint32_t(input3), 7ull);
  WriteFields<32, 4>(base, uint32_t(input3), 1ull);
  WriteFields<32, 20>(base, uint32_t(input3), 0xFFFFFFFFFFFF0000ull);
  WriteFields<32, 24>(base, uint32_t(input3), input4);
  ctx.r8.u64 = 0xFFFFFFFFFFFF0000ull;
  ctx.r9.u64 = 1ull;
  ctx.r10.u64 = 7ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_829013A0) {
  GH_PROLOGUE();
  const uint64_t field0 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82080000ull) + 12960u);
  const uint64_t field1 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82080000ull) + 9688u);
  WriteFields<32, -12844>(base, uint32_t(0xFFFFFFFF829D0000ull), field0);
  WriteFields<32, 4, 8, 12, 16, 20, 24>(base, uint32_t(0xFFFFFFFF829CCDD4ull), field1);
  ctx.r7.u64 = 0xFFFFFFFF829CCDD4ull;
  ctx.r8.u64 = 0xFFFFFFFF829D0000ull;
  ctx.r9.u64 = 0xFFFFFFFF82080000ull;
  ctx.r10.u64 = field0;
  ctx.r11.u64 = field1;
}

GH_HOOK(sub_8248B478) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 4, 8>(base, uint32_t(input3), 0ull);
  WriteFields<8, 12, 14, 13>(base, uint32_t(input3), 0ull);
  WriteFields<8, 16, 15>(base, uint32_t(input3), 1ull);
  WriteFields<8, 17>(base, uint32_t(input3), 0ull);
  WriteFields<32, 20>(base, uint32_t(input3), 0ull);
  WriteFields<8, 24>(base, uint32_t(input3), 0ull);
  WriteFields<32, 0>(base, uint32_t(input3), 2ull);
  ctx.r9.u64 = 2ull;
  ctx.r10.u64 = 1ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8248B4B8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 8>(base, uint32_t(input3), 0ull);
  WriteFields<8, 12, 14, 13>(base, uint32_t(input3), 0ull);
  WriteFields<8, 16, 15>(base, uint32_t(input3), 1ull);
  WriteFields<8, 17>(base, uint32_t(input3), 0ull);
  WriteFields<32, 20, 4>(base, uint32_t(input3), 0ull);
  WriteFields<32, 0>(base, uint32_t(input3), 3ull);
  WriteFields<8, 24>(base, uint32_t(input3), 1ull);
  ctx.r9.u64 = 3ull;
  ctx.r10.u64 = 1ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8248B4F8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 8>(base, uint32_t(input3), 0ull);
  WriteFields<8, 12, 14>(base, uint32_t(input3), 0ull);
  WriteFields<8, 16, 15>(base, uint32_t(input3), 1ull);
  WriteFields<8, 17>(base, uint32_t(input3), 0ull);
  WriteFields<32, 20>(base, uint32_t(input3), 0ull);
  WriteFields<8, 24>(base, uint32_t(input3), 0ull);
  WriteFields<8, 13>(base, uint32_t(input3), 1ull);
  WriteFields<32, 4>(base, uint32_t(input3), 0ull);
  WriteFields<32, 0>(base, uint32_t(input3), 4ull);
  ctx.r9.u64 = 4ull;
  ctx.r10.u64 = 1ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8248B538) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 4, 8>(base, uint32_t(input3), 0ull);
  WriteFields<8, 12, 14, 13>(base, uint32_t(input3), 0ull);
  WriteFields<8, 16, 15>(base, uint32_t(input3), 1ull);
  WriteFields<8, 17>(base, uint32_t(input3), 0ull);
  WriteFields<32, 20>(base, uint32_t(input3), 0ull);
  WriteFields<8, 24>(base, uint32_t(input3), 0ull);
  WriteFields<32, 0>(base, uint32_t(input3), 5ull);
  ctx.r9.u64 = 5ull;
  ctx.r10.u64 = 1ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_825D9198) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t((((input4 + 48ull) * 24ull) + input3)) + 12u);
  WriteFields<32, 12>(base, uint32_t((((input4 + 48ull) * 24ull) + input3)), ((uint64_t(std::rotl(uint32_t(input5), 23)) & 25165824ull) | (field0 & 0xFFFFFFFFFE7FFFFFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 24u);
  const uint64_t value0 = ((((input4 + 32ull) & 0xFFFFFFFFull) & 64ull) ? 0ull : (0x8000000000000000ull >> (((input4 + 32ull) & 0xFFFFFFFFull) & 63ull)));
  WriteFields<64, 24>(base, uint32_t(input3), (value0 | field1));
  ctx.r8.u64 = ((uint64_t(std::rotl(uint32_t(input5), 23)) & 25165824ull) | (field0 & 0xFFFFFFFFFE7FFFFFull));
  ctx.r9.u64 = 0x8000000000000000ull;
  ctx.r10.u64 = value0;
  ctx.r11.u64 = (value0 | field1);
}

GH_HOOK(sub_825D96B8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t((((input4 + 48ull) * 24ull) + input3)) + 20u);
  WriteFields<32, 20>(base, uint32_t((((input4 + 48ull) * 24ull) + input3)), ((uint64_t(std::rotl(uint32_t(input5), 3)) & 24ull) | (field0 & 0xFFFFFFFFFFFFFFE7ull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 24u);
  const uint64_t value0 = ((((input4 + 32ull) & 0xFFFFFFFFull) & 64ull) ? 0ull : (0x8000000000000000ull >> (((input4 + 32ull) & 0xFFFFFFFFull) & 63ull)));
  WriteFields<64, 24>(base, uint32_t(input3), (value0 | field1));
  ctx.r8.u64 = ((uint64_t(std::rotl(uint32_t(input5), 3)) & 24ull) | (field0 & 0xFFFFFFFFFFFFFFE7ull));
  ctx.r9.u64 = 0x8000000000000000ull;
  ctx.r10.u64 = value0;
  ctx.r11.u64 = (value0 | field1);
}

GH_HOOK(sub_825D9710) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t((((input4 + 48ull) * 24ull) + input3)) + 16u);
  WriteFields<32, 16>(base, uint32_t((((input4 + 48ull) * 24ull) + input3)), ((uint64_t(std::rotl(uint32_t(input5), 22)) & 130023424ull) | (field0 & 0xFFFFFFFFF83FFFFFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 24u);
  const uint64_t value0 = ((((input4 + 32ull) & 0xFFFFFFFFull) & 64ull) ? 0ull : (0x8000000000000000ull >> (((input4 + 32ull) & 0xFFFFFFFFull) & 63ull)));
  WriteFields<64, 24>(base, uint32_t(input3), (value0 | field1));
  ctx.r8.u64 = ((uint64_t(std::rotl(uint32_t(input5), 22)) & 130023424ull) | (field0 & 0xFFFFFFFFF83FFFFFull));
  ctx.r9.u64 = 0x8000000000000000ull;
  ctx.r10.u64 = value0;
  ctx.r11.u64 = (value0 | field1);
}

GH_HOOK(sub_825D9768) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t((((input4 + 48ull) * 24ull) + input3)) + 16u);
  WriteFields<32, 16>(base, uint32_t((((input4 + 48ull) * 24ull) + input3)), ((uint64_t(std::rotl(uint32_t(input5), 27)) & 0xF8000000ull) | (field0 & 0xFFFFFFFF07FFFFFFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 24u);
  const uint64_t value0 = ((((input4 + 32ull) & 0xFFFFFFFFull) & 64ull) ? 0ull : (0x8000000000000000ull >> (((input4 + 32ull) & 0xFFFFFFFFull) & 63ull)));
  WriteFields<64, 24>(base, uint32_t(input3), (value0 | field1));
  ctx.r8.u64 = ((uint64_t(std::rotl(uint32_t(input5), 27)) & 0xF8000000ull) | (field0 & 0xFFFFFFFF07FFFFFFull));
  ctx.r9.u64 = 0x8000000000000000ull;
  ctx.r10.u64 = value0;
  ctx.r11.u64 = (value0 | field1);
}

GH_HOOK(sub_825D97C0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t((((input4 + 48ull) * 24ull) + input3)) + 20u);
  WriteFields<32, 20>(base, uint32_t((((input4 + 48ull) * 24ull) + input3)), ((uint64_t(std::rotl(uint32_t(input5), 2)) & 4ull) | (field0 & 0xFFFFFFFFFFFFFFFBull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 24u);
  const uint64_t value0 = ((((input4 + 32ull) & 0xFFFFFFFFull) & 64ull) ? 0ull : (0x8000000000000000ull >> (((input4 + 32ull) & 0xFFFFFFFFull) & 63ull)));
  WriteFields<64, 24>(base, uint32_t(input3), (value0 | field1));
  ctx.r8.u64 = ((uint64_t(std::rotl(uint32_t(input5), 2)) & 4ull) | (field0 & 0xFFFFFFFFFFFFFFFBull));
  ctx.r9.u64 = 0x8000000000000000ull;
  ctx.r10.u64 = value0;
  ctx.r11.u64 = (value0 | field1);
}

GH_HOOK(sub_82632DF8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 16u);
  WriteFields<32, 24>(base, uint32_t(input4), field0);
  const uint64_t field1 = GH_LOAD_U32(uint32_t(input3) + 396u);
  WriteFields<32, 28>(base, uint32_t(input4), field1);
  const uint64_t field2 = GH_LOAD_U32(uint32_t(input3) + 20u);
  WriteFields<32, 32>(base, uint32_t(input4), field2);
  const uint64_t field3 = GH_LOAD_U32(uint32_t(input3) + 24u);
  WriteFields<32, 36>(base, uint32_t(input4), field3);
  const uint64_t field4 = GH_LOAD_U32(uint32_t(input3) + 28u);
  WriteFields<32, 40>(base, uint32_t(input4), field4);
  const uint64_t field5 = GH_LOAD_U32(uint32_t(input3) + 32u);
  WriteFields<32, 44>(base, uint32_t(input4), field5);
  const uint64_t field6 = GH_LOAD_U32(uint32_t(input3) + 36u);
  WriteFields<32, 48>(base, uint32_t(input4), field6);
  ctx.r11.u64 = field6;
}

GH_HOOK(sub_8281D570) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t input6 = ctx.r6.u64;
  WriteFields<32, 20>(base, uint32_t(input3), input5);
  WriteFields<32, 0, 8>(base, uint32_t(input3), input4);
  WriteFields<32, 12>(base, uint32_t(input3), 0ull);
  WriteFields<32, 16>(base, uint32_t(input3), 32ull);
  WriteFields<32, 4>(base, uint32_t(input3), 0ull);
  WriteFields<32, 44>(base, uint32_t(input3), input6);
  WriteFields<32, 48, 56>(base, uint32_t(input3), 0ull);
  WriteFields<32, 52>(base, uint32_t(input3), 2147483647ull);
  ctx.r8.u64 = 2147483647ull;
  ctx.r9.u64 = 32ull;
  ctx.r10.u64 = 2147418112ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82496088) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 2>(base, uint32_t(input3), 2ull);
  WriteFields<8, 3>(base, uint32_t(input3), 3ull);
  WriteFields<8, 4>(base, uint32_t(input3), 1ull);
  WriteFields<8, 5>(base, uint32_t(input3), 5ull);
  WriteFields<8, 6>(base, uint32_t(input3), 7ull);
  WriteFields<8, 7>(base, uint32_t(input3), 4ull);
  WriteFields<8, 8>(base, uint32_t(input3), 6ull);
  WriteFields<8, 9>(base, uint32_t(input3), 0ull);
  ctx.r4.u64 = 0ull;
  ctx.r5.u64 = 6ull;
  ctx.r6.u64 = 4ull;
  ctx.r7.u64 = 7ull;
  ctx.r8.u64 = 5ull;
  ctx.r9.u64 = 1ull;
  ctx.r10.u64 = 3ull;
  ctx.r11.u64 = 2ull;
}

GH_HOOK(sub_825D9818) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t((((input4 + 48ull) * 24ull) + input3)) + 4u);
  const uint64_t value0 = uint64_t(std::rotl(uint32_t((uint64_t(std::rotl(uint32_t(uint64_t(std::countl_zero(uint32_t(input5)))), 27)) & 1ull)), 11));
  WriteFields<32, 4>(base, uint32_t((((input4 + 48ull) * 24ull) + input3)), ((value0 & 2048ull) | (field0 & 0xFFFFFFFFFFFFF7FFull)));
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input3) + 24u);
  const uint64_t value1 = ((((input4 + 32ull) & 0xFFFFFFFFull) & 64ull) ? 0ull : (0x8000000000000000ull >> (((input4 + 32ull) & 0xFFFFFFFFull) & 63ull)));
  WriteFields<64, 24>(base, uint32_t(input3), (value1 | field1));
  ctx.r7.u64 = ((value0 & 2048ull) | (field0 & 0xFFFFFFFFFFFFF7FFull));
  ctx.r8.u64 = 0x8000000000000000ull;
  ctx.r9.u64 = ((input4 + 32ull) & 0xFFFFFFFFull);
  ctx.r10.u64 = field1;
  ctx.r11.u64 = (value1 | field1);
}

GH_HOOK(sub_82684FC0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 4, 8>(base, uint32_t(input3), input3);
  WriteFields<32, 12>(base, uint32_t(input3), input4);
  WriteFields<32, 16>(base, uint32_t(input3), 0ull);
  WriteFields<32, 0>(base, uint32_t(input3), 0xFFFFFFFF8239FFA8ull);
  WriteFields<16, 108>(base, uint32_t(input3), 0ull);
  WriteFields<32, 112, 116, 120, 124, 128, 132, 136, 140>(base, uint32_t(input3), 0ull);
  ctx.r9.u64 = 0xFFFFFFFF8239FFA8ull;
  ctx.r10.u64 = 0xFFFFFFFF823A0000ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_828242B0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 8224>(base, uint32_t(input3), 0xFFFFFFFF82823D38ull);
  WriteFields<32, 8220>(base, uint32_t(input3), 0xFFFFFFFF828239E8ull);
  WriteFields<32, 8212>(base, uint32_t(input3), 0x828239E8ull);
  WriteFields<32, 8216>(base, uint32_t(input3), 0x82823D38ull);
  WriteFields<32, 8208>(base, uint32_t(input3), 0x828239E8ull);
  WriteFields<32, 8232>(base, uint32_t(input3), 0xFFFFFFFF82824068ull);
  WriteFields<32, 8228>(base, uint32_t(input3), 0xFFFFFFFF82822D10ull);
  ctx.r4.u64 = 0xFFFFFFFF82824068ull;
  ctx.r5.u64 = 0xFFFFFFFF828239E8ull;
  ctx.r6.u64 = 0x82823D38ull;
  ctx.r7.u64 = 0xFFFFFFFF82820000ull;
  ctx.r8.u64 = 0xFFFFFFFF82820000ull;
  ctx.r9.u64 = 0xFFFFFFFF82823D38ull;
  ctx.r10.u64 = 0x828239E8ull;
  ctx.r11.u64 = 0xFFFFFFFF82822D10ull;
}

GH_HOOK(sub_828ED510) {
  GH_PROLOGUE();
  WriteFields<32, 5924>(base, uint32_t(0xFFFFFFFF82990000ull), 0xFFFFFFFF82001008ull);
  WriteFields<32, 4, 8, 12, 20, 24, 28>(base, uint32_t(0xFFFFFFFF82991724ull), 0ull);
  WriteFields<32, 16>(base, uint32_t(0xFFFFFFFF82991724ull), 0xFFFFFFFF82001008ull);
  ctx.r5.u64 = 0xFFFFFFFF82001008ull;
  ctx.r6.u64 = 0xFFFFFFFF82991724ull;
  ctx.r7.u64 = 0xFFFFFFFF82000000ull;
  ctx.r8.u64 = 0xFFFFFFFF82001008ull;
  ctx.r9.u64 = 0ull;
  ctx.r10.u64 = 0ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82708080) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<64, 56>(base, uint32_t(input3), 0ull);
  WriteFields<32, 44>(base, uint32_t(input3), 0xFFFFFFFFFFFFFFFFull);
  WriteFields<32, 48>(base, uint32_t(input3), 1ull);
  WriteFields<64, 64, 72, 80, 88, 96>(base, uint32_t(input3), 0ull);
  WriteFields<32, 104, 108, 112, 116, 120, 124, 128, 132>(base, uint32_t(input3), 0ull);
  ctx.r9.u64 = 1ull;
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82847EB8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 128, 124, 64, 96>(base, uint32_t(input3), 0ull);
  WriteFields<32, 76>(base, uint32_t(input3), 1ull);
  WriteFields<32, 32, 36, 40, 44, 0, 8, 16, 24, 4, 12, 20, 28>(base, uint32_t(input3), 0ull);
  ctx.r10.u64 = 1ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82847F78) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 128, 136, 64>(base, uint32_t(input3), 0ull);
  WriteFields<32, 84>(base, uint32_t(input3), 1ull);
  WriteFields<32, 32, 36, 40, 44, 0, 8, 16, 24, 4, 12, 20, 28, 112>(base, uint32_t(input3), 0ull);
  ctx.r10.u64 = 1ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_824960D0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 10>(base, uint32_t(input3), 2ull);
  WriteFields<8, 11>(base, uint32_t(input3), 3ull);
  WriteFields<8, 12>(base, uint32_t(input3), 1ull);
  WriteFields<8, 13>(base, uint32_t(input3), 0ull);
  WriteFields<8, 14>(base, uint32_t(input3), 9ull);
  WriteFields<8, 15>(base, uint32_t(input3), 5ull);
  WriteFields<8, 16>(base, uint32_t(input3), 7ull);
  WriteFields<8, 17>(base, uint32_t(input3), 4ull);
  WriteFields<8, 18>(base, uint32_t(input3), 6ull);
  WriteFields<8, 19>(base, uint32_t(input3), 8ull);
  ctx.r4.u64 = 4ull;
  ctx.r5.u64 = 7ull;
  ctx.r6.u64 = 5ull;
  ctx.r7.u64 = 9ull;
  ctx.r8.u64 = 0ull;
  ctx.r9.u64 = 1ull;
  ctx.r10.u64 = 8ull;
  ctx.r11.u64 = 6ull;
}

GH_HOOK(sub_82712B18) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 21660, 3468, 3480, 3476, 21544, 21868>(base, uint32_t(input3), 0ull);
  WriteFields<32, 21676>(base, uint32_t(input3), 1ull);
  WriteFields<32, 3488>(base, uint32_t(input3), 0ull);
  WriteFields<32, 21576>(base, uint32_t(input3), 0xFFFFFFFFFFFFFFFFull);
  WriteFields<32, 408, 21664, 21672, 21668, 4020, 21784, 340, 332>(base, uint32_t(input3), 0ull);
  ctx.r9.u64 = 0xFFFFFFFFFFFFFFFFull;
  ctx.r10.u64 = 1ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8281D8C0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 2068>(base, uint32_t(input3), 0xFFFFFFFF8276A3A8ull);
  WriteFields<32, 2056>(base, uint32_t(input3), 0xFFFFFFFF828731A0ull);
  WriteFields<32, 2060>(base, uint32_t(input3), 0xFFFFFFFF828733E8ull);
  WriteFields<32, 2064>(base, uint32_t(input3), 0xFFFFFFFF82873C08ull);
  WriteFields<32, 2084>(base, uint32_t(input3), 0x8276A3A8ull);
  WriteFields<32, 2072>(base, uint32_t(input3), 0x828731A0ull);
  WriteFields<32, 2076>(base, uint32_t(input3), 0x828733E8ull);
  WriteFields<32, 2080>(base, uint32_t(input3), 0x82873C08ull);
  ctx.r4.u64 = 0x8276A3A8ull;
  ctx.r5.u64 = 0xFFFFFFFF82870000ull;
  ctx.r6.u64 = 0xFFFFFFFF828733E8ull;
  ctx.r7.u64 = 0xFFFFFFFF828731A0ull;
  ctx.r8.u64 = 0x82873C08ull;
  ctx.r9.u64 = 0x828733E8ull;
  ctx.r10.u64 = 0x828731A0ull;
  ctx.r11.u64 = 0xFFFFFFFF82873C08ull;
}

GH_HOOK(sub_8289A098) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 8>(base, uint32_t(input3), 0ull);
  WriteFields<32, 4>(base, uint32_t(input3), 0xFFFFFFFF823EC7E8ull);
  WriteFields<32, 12, 16, 20, 24, 28>(base, uint32_t(input3), 0ull);
  WriteFields<32, 32>(base, uint32_t(input3), 0xFFFFFFFF823EC7E8ull);
  WriteFields<32, 36, 40, 44, 48, 52, 56, 0>(base, uint32_t(input3), 0ull);
  WriteFields<32, 60>(base, uint32_t(input3), 0xFFFFFFFFFFFFFFFFull);
  WriteFields<32, 64>(base, uint32_t(input3), 0ull);
  ctx.r8.u64 = 0xFFFFFFFFFFFFFFFFull;
  ctx.r9.u64 = 0xFFFFFFFF823EC7E8ull;
  ctx.r10.u64 = 0xFFFFFFFF823F0000ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82901088) {
  GH_PROLOGUE();
  const uint64_t field0 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82060000ull) + -27940u);
  const uint64_t field1 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82050000ull) + 17724u);
  const uint64_t field2 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82050000ull) + 23236u);
  const uint64_t field3 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82060000ull) + -30936u);
  const uint64_t field4 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82050000ull) + 28776u);
  const uint64_t field5 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82060000ull) + -22356u);
  WriteFields<32, -13096>(base, uint32_t(0xFFFFFFFF829D0000ull), field0);
  WriteFields<32, 4>(base, uint32_t(0xFFFFFFFF829CCCD8ull), field1);
  WriteFields<32, 8>(base, uint32_t(0xFFFFFFFF829CCCD8ull), field2);
  WriteFields<32, 12>(base, uint32_t(0xFFFFFFFF829CCCD8ull), field3);
  WriteFields<32, 16>(base, uint32_t(0xFFFFFFFF829CCCD8ull), field4);
  WriteFields<32, 20>(base, uint32_t(0xFFFFFFFF829CCCD8ull), field5);
  WriteFields<32, 24>(base, uint32_t(0xFFFFFFFF829CCCD8ull), field1);
  ctx.r3.u64 = 0xFFFFFFFF829CCCD8ull;
  ctx.r4.u64 = 0xFFFFFFFF829D0000ull;
  ctx.r5.u64 = 0xFFFFFFFF82060000ull;
  ctx.r6.u64 = field5;
  ctx.r7.u64 = field4;
  ctx.r8.u64 = field3;
  ctx.r9.u64 = field2;
  ctx.r10.u64 = field0;
  ctx.r11.u64 = field1;
}

GH_HOOK(sub_829010E0) {
  GH_PROLOGUE();
  const uint64_t field0 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82050000ull) + -10868u);
  const uint64_t field1 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82040000ull) + -25288u);
  const uint64_t field2 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82040000ull) + -2152u);
  const uint64_t field3 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82050000ull) + -22968u);
  const uint64_t field4 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82040000ull) + 19184u);
  const uint64_t field5 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82050000ull) + 10716u);
  WriteFields<32, -13068>(base, uint32_t(0xFFFFFFFF829D0000ull), field0);
  WriteFields<32, 4>(base, uint32_t(0xFFFFFFFF829CCCF4ull), field1);
  WriteFields<32, 8>(base, uint32_t(0xFFFFFFFF829CCCF4ull), field2);
  WriteFields<32, 12>(base, uint32_t(0xFFFFFFFF829CCCF4ull), field3);
  WriteFields<32, 16>(base, uint32_t(0xFFFFFFFF829CCCF4ull), field4);
  WriteFields<32, 20>(base, uint32_t(0xFFFFFFFF829CCCF4ull), field5);
  WriteFields<32, 24>(base, uint32_t(0xFFFFFFFF829CCCF4ull), field1);
  ctx.r3.u64 = 0xFFFFFFFF829CCCF4ull;
  ctx.r4.u64 = 0xFFFFFFFF829D0000ull;
  ctx.r5.u64 = 0xFFFFFFFF82050000ull;
  ctx.r6.u64 = field5;
  ctx.r7.u64 = field4;
  ctx.r8.u64 = field3;
  ctx.r9.u64 = field2;
  ctx.r10.u64 = field0;
  ctx.r11.u64 = field1;
}

GH_HOOK(sub_82901138) {
  GH_PROLOGUE();
  const uint64_t field0 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82030000ull) + 15816u);
  const uint64_t field1 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82030000ull) + 2656u);
  const uint64_t field2 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82030000ull) + 6268u);
  const uint64_t field3 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82030000ull) + 12984u);
  const uint64_t field4 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82030000ull) + 9620u);
  const uint64_t field5 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82030000ull) + 19128u);
  WriteFields<32, -13040>(base, uint32_t(0xFFFFFFFF829D0000ull), field0);
  WriteFields<32, 4>(base, uint32_t(0xFFFFFFFF829CCD10ull), field1);
  WriteFields<32, 8>(base, uint32_t(0xFFFFFFFF829CCD10ull), field2);
  WriteFields<32, 12>(base, uint32_t(0xFFFFFFFF829CCD10ull), field3);
  WriteFields<32, 16>(base, uint32_t(0xFFFFFFFF829CCD10ull), field4);
  WriteFields<32, 20>(base, uint32_t(0xFFFFFFFF829CCD10ull), field5);
  WriteFields<32, 24>(base, uint32_t(0xFFFFFFFF829CCD10ull), field1);
  ctx.r3.u64 = 0xFFFFFFFF829CCD10ull;
  ctx.r4.u64 = 0xFFFFFFFF829D0000ull;
  ctx.r5.u64 = 0xFFFFFFFF82030000ull;
  ctx.r6.u64 = field5;
  ctx.r7.u64 = field4;
  ctx.r8.u64 = field3;
  ctx.r9.u64 = field2;
  ctx.r10.u64 = field0;
  ctx.r11.u64 = field1;
}

GH_HOOK(sub_82901190) {
  GH_PROLOGUE();
  const uint64_t field0 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82080000ull) + -20020u);
  const uint64_t field1 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82070000ull) + -4900u);
  const uint64_t field2 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82070000ull) + 9528u);
  const uint64_t field3 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82080000ull) + -26364u);
  const uint64_t field4 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82070000ull) + 23856u);
  const uint64_t field5 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82080000ull) + -5680u);
  WriteFields<32, -13012>(base, uint32_t(0xFFFFFFFF829D0000ull), field0);
  WriteFields<32, 4>(base, uint32_t(0xFFFFFFFF829CCD2Cull), field1);
  WriteFields<32, 8>(base, uint32_t(0xFFFFFFFF829CCD2Cull), field2);
  WriteFields<32, 12>(base, uint32_t(0xFFFFFFFF829CCD2Cull), field3);
  WriteFields<32, 16>(base, uint32_t(0xFFFFFFFF829CCD2Cull), field4);
  WriteFields<32, 20>(base, uint32_t(0xFFFFFFFF829CCD2Cull), field5);
  WriteFields<32, 24>(base, uint32_t(0xFFFFFFFF829CCD2Cull), field1);
  ctx.r3.u64 = 0xFFFFFFFF829CCD2Cull;
  ctx.r4.u64 = 0xFFFFFFFF829D0000ull;
  ctx.r5.u64 = 0xFFFFFFFF82080000ull;
  ctx.r6.u64 = field5;
  ctx.r7.u64 = field4;
  ctx.r8.u64 = field3;
  ctx.r9.u64 = field2;
  ctx.r10.u64 = field0;
  ctx.r11.u64 = field1;
}

GH_HOOK(sub_829011E8) {
  GH_PROLOGUE();
  const uint64_t field0 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82070000ull) + -31776u);
  const uint64_t field1 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82060000ull) + -9524u);
  const uint64_t field2 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82060000ull) + 2928u);
  const uint64_t field3 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82060000ull) + 28340u);
  const uint64_t field4 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82060000ull) + 15696u);
  const uint64_t field5 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82070000ull) + -19552u);
  WriteFields<32, -12984>(base, uint32_t(0xFFFFFFFF829D0000ull), field0);
  WriteFields<32, 4>(base, uint32_t(0xFFFFFFFF829CCD48ull), field1);
  WriteFields<32, 8>(base, uint32_t(0xFFFFFFFF829CCD48ull), field2);
  WriteFields<32, 12>(base, uint32_t(0xFFFFFFFF829CCD48ull), field3);
  WriteFields<32, 16>(base, uint32_t(0xFFFFFFFF829CCD48ull), field4);
  WriteFields<32, 20>(base, uint32_t(0xFFFFFFFF829CCD48ull), field5);
  WriteFields<32, 24>(base, uint32_t(0xFFFFFFFF829CCD48ull), field1);
  ctx.r3.u64 = 0xFFFFFFFF829CCD48ull;
  ctx.r4.u64 = 0xFFFFFFFF829D0000ull;
  ctx.r5.u64 = 0xFFFFFFFF82070000ull;
  ctx.r6.u64 = field5;
  ctx.r7.u64 = field4;
  ctx.r8.u64 = field3;
  ctx.r9.u64 = field2;
  ctx.r10.u64 = field0;
  ctx.r11.u64 = field1;
}

GH_HOOK(sub_82901240) {
  GH_PROLOGUE();
  const uint64_t field0 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82020000ull) + 15212u);
  const uint64_t field1 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82020000ull) + 4848u);
  const uint64_t field2 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82020000ull) + 7740u);
  const uint64_t field3 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82020000ull) + 13768u);
  const uint64_t field4 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82020000ull) + 10688u);
  const uint64_t field5 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82020000ull) + 18076u);
  WriteFields<32, -12956>(base, uint32_t(0xFFFFFFFF829D0000ull), field0);
  WriteFields<32, 4>(base, uint32_t(0xFFFFFFFF829CCD64ull), field1);
  WriteFields<32, 8>(base, uint32_t(0xFFFFFFFF829CCD64ull), field2);
  WriteFields<32, 12>(base, uint32_t(0xFFFFFFFF829CCD64ull), field3);
  WriteFields<32, 16>(base, uint32_t(0xFFFFFFFF829CCD64ull), field4);
  WriteFields<32, 20>(base, uint32_t(0xFFFFFFFF829CCD64ull), field5);
  WriteFields<32, 24>(base, uint32_t(0xFFFFFFFF829CCD64ull), field1);
  ctx.r3.u64 = 0xFFFFFFFF829CCD64ull;
  ctx.r4.u64 = 0xFFFFFFFF829D0000ull;
  ctx.r5.u64 = 0xFFFFFFFF82020000ull;
  ctx.r6.u64 = field5;
  ctx.r7.u64 = field4;
  ctx.r8.u64 = field3;
  ctx.r9.u64 = field2;
  ctx.r10.u64 = field0;
  ctx.r11.u64 = field1;
}

GH_HOOK(sub_82901298) {
  GH_PROLOGUE();
  const uint64_t field0 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82030000ull) + -10472u);
  const uint64_t field1 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82020000ull) + 27892u);
  const uint64_t field2 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82030000ull) + -30504u);
  const uint64_t field3 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82030000ull) + -14664u);
  const uint64_t field4 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82030000ull) + -23372u);
  const uint64_t field5 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82030000ull) + -2848u);
  WriteFields<32, -12928>(base, uint32_t(0xFFFFFFFF829D0000ull), field0);
  WriteFields<32, 4>(base, uint32_t(0xFFFFFFFF829CCD80ull), field1);
  WriteFields<32, 8>(base, uint32_t(0xFFFFFFFF829CCD80ull), field2);
  WriteFields<32, 12>(base, uint32_t(0xFFFFFFFF829CCD80ull), field3);
  WriteFields<32, 16>(base, uint32_t(0xFFFFFFFF829CCD80ull), field4);
  WriteFields<32, 20>(base, uint32_t(0xFFFFFFFF829CCD80ull), field5);
  WriteFields<32, 24>(base, uint32_t(0xFFFFFFFF829CCD80ull), field1);
  ctx.r3.u64 = 0xFFFFFFFF829CCD80ull;
  ctx.r4.u64 = 0xFFFFFFFF829D0000ull;
  ctx.r5.u64 = 0xFFFFFFFF82030000ull;
  ctx.r6.u64 = field5;
  ctx.r7.u64 = field4;
  ctx.r8.u64 = field3;
  ctx.r9.u64 = field2;
  ctx.r10.u64 = field0;
  ctx.r11.u64 = field1;
}

GH_HOOK(sub_829012F0) {
  GH_PROLOGUE();
  const uint64_t field0 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82030000ull) + -10468u);
  const uint64_t field1 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82020000ull) + 27896u);
  const uint64_t field2 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82030000ull) + -30500u);
  const uint64_t field3 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82030000ull) + -14660u);
  const uint64_t field4 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82030000ull) + -23368u);
  const uint64_t field5 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82030000ull) + -2844u);
  WriteFields<32, -12900>(base, uint32_t(0xFFFFFFFF829D0000ull), field0);
  WriteFields<32, 4>(base, uint32_t(0xFFFFFFFF829CCD9Cull), field1);
  WriteFields<32, 8>(base, uint32_t(0xFFFFFFFF829CCD9Cull), field2);
  WriteFields<32, 12>(base, uint32_t(0xFFFFFFFF829CCD9Cull), field3);
  WriteFields<32, 16>(base, uint32_t(0xFFFFFFFF829CCD9Cull), field4);
  WriteFields<32, 20>(base, uint32_t(0xFFFFFFFF829CCD9Cull), field5);
  WriteFields<32, 24>(base, uint32_t(0xFFFFFFFF829CCD9Cull), field1);
  ctx.r3.u64 = 0xFFFFFFFF829CCD9Cull;
  ctx.r4.u64 = 0xFFFFFFFF829D0000ull;
  ctx.r5.u64 = 0xFFFFFFFF82030000ull;
  ctx.r6.u64 = field5;
  ctx.r7.u64 = field4;
  ctx.r8.u64 = field3;
  ctx.r9.u64 = field2;
  ctx.r10.u64 = field0;
  ctx.r11.u64 = field1;
}

GH_HOOK(sub_82901348) {
  GH_PROLOGUE();
  const uint64_t field0 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82080000ull) + 20680u);
  const uint64_t field1 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82080000ull) + 14268u);
  const uint64_t field2 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82080000ull) + 16068u);
  const uint64_t field3 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82080000ull) + 19988u);
  const uint64_t field4 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82080000ull) + 17680u);
  const uint64_t field5 = GH_LOAD_U32(uint32_t(0xFFFFFFFF82080000ull) + 22188u);
  WriteFields<32, -12872>(base, uint32_t(0xFFFFFFFF829D0000ull), field0);
  WriteFields<32, 4>(base, uint32_t(0xFFFFFFFF829CCDB8ull), field1);
  WriteFields<32, 8>(base, uint32_t(0xFFFFFFFF829CCDB8ull), field2);
  WriteFields<32, 12>(base, uint32_t(0xFFFFFFFF829CCDB8ull), field3);
  WriteFields<32, 16>(base, uint32_t(0xFFFFFFFF829CCDB8ull), field4);
  WriteFields<32, 20>(base, uint32_t(0xFFFFFFFF829CCDB8ull), field5);
  WriteFields<32, 24>(base, uint32_t(0xFFFFFFFF829CCDB8ull), field1);
  ctx.r3.u64 = 0xFFFFFFFF829CCDB8ull;
  ctx.r4.u64 = 0xFFFFFFFF829D0000ull;
  ctx.r5.u64 = 0xFFFFFFFF82080000ull;
  ctx.r6.u64 = field5;
  ctx.r7.u64 = field4;
  ctx.r8.u64 = field3;
  ctx.r9.u64 = field2;
  ctx.r10.u64 = field0;
  ctx.r11.u64 = field1;
}

GH_HOOK(sub_82495F90) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0, 8>(base, uint32_t(input3), 0ull);
  WriteFields<32, 4, 12>(base, uint32_t(input3), 0xFFFFFFFFFFFFFFFFull);
  WriteFields<32, 16>(base, uint32_t(input3), 0ull);
  WriteFields<32, 20>(base, uint32_t(input3), 0xFFFFFFFFFFFFFFFFull);
  WriteFields<32, 24>(base, uint32_t(input3), 0ull);
  WriteFields<32, 28>(base, uint32_t(input3), 0xFFFFFFFFFFFFFFFFull);
  WriteFields<32, 32>(base, uint32_t(input3), 0ull);
  WriteFields<32, 36>(base, uint32_t(input3), 0xFFFFFFFFFFFFFFFFull);
  WriteFields<32, 40>(base, uint32_t(input3), 0ull);
  WriteFields<32, 44>(base, uint32_t(input3), 0xFFFFFFFFFFFFFFFFull);
  WriteFields<32, 48>(base, uint32_t(input3), 0ull);
  WriteFields<32, 52>(base, uint32_t(input3), 0xFFFFFFFFFFFFFFFFull);
  WriteFields<32, 56>(base, uint32_t(input3), 0ull);
  WriteFields<32, 60>(base, uint32_t(input3), 0xFFFFFFFFFFFFFFFFull);
  WriteFields<32, 64>(base, uint32_t(input3), 0ull);
  WriteFields<32, 68>(base, uint32_t(input3), 0xFFFFFFFFFFFFFFFFull);
  WriteFields<32, 72>(base, uint32_t(input3), 0ull);
  WriteFields<32, 76>(base, uint32_t(input3), 0xFFFFFFFFFFFFFFFFull);
  ctx.r10.u64 = 0ull;
  ctx.r11.u64 = 0xFFFFFFFFFFFFFFFFull;
}

GH_HOOK(sub_82496B50) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 0>(base, uint32_t(input3), 0ull);
  WriteFields<32, 4, 8, 772>(base, uint32_t(input3), 13959168ull);
  WriteFields<32, 776>(base, uint32_t(input3), 0xFFFFFFFFFFA00000ull);
  WriteFields<32, 780, 784, 788, 792, 796>(base, uint32_t(input3), 0ull);
  WriteFields<16, 800>(base, uint32_t(input3), 0ull);
  WriteFields<16, 802>(base, uint32_t(input3), 150ull);
  WriteFields<32, 804>(base, uint32_t(input3), 0ull);
  WriteFields<8, 808>(base, uint32_t(input3), 0ull);
  WriteFields<32, 812>(base, uint32_t(input3), 0ull);
  WriteFields<8, 816>(base, uint32_t(input3), 0ull);
  WriteFields<8, 817>(base, uint32_t(input3), 1ull);
  ctx.r7.u64 = 1ull;
  ctx.r8.u64 = 150ull;
  ctx.r9.u64 = 0xFFFFFFFFFFA00000ull;
  ctx.r10.u64 = 13959168ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82496DF8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 26>(base, uint32_t(input3), 8ull);
  WriteFields<8, 14>(base, uint32_t(input3), 50ull);
  WriteFields<8, 21>(base, uint32_t(input3), 10ull);
  WriteFields<8, 25, 24, 23, 22>(base, uint32_t(input3), 8ull);
  WriteFields<8, 15, 16, 17, 18, 19, 20, 27, 28, 29, 30, 31, 32>(base, uint32_t(input3), 0ull);
  ctx.r8.u64 = 10ull;
  ctx.r9.u64 = 50ull;
  ctx.r10.u64 = 8ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8268F440) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  WriteFields<32, 4, 8>(base, uint32_t(input3), input3);
  WriteFields<32, 12>(base, uint32_t(input3), input4);
  WriteFields<32, 16>(base, uint32_t(input3), input5);
  WriteFields<32, 20>(base, uint32_t(input3), 0ull);
  WriteFields<32, 0>(base, uint32_t(input3), 0xFFFFFFFF823A06C0ull);
  WriteFields<8, 24>(base, uint32_t(input3), 0ull);
  WriteFields<32, 28>(base, uint32_t(input3), 0ull);
  WriteFields<8, 32>(base, uint32_t(input3), 0ull);
  WriteFields<32, 36>(base, uint32_t(input3), 0ull);
  WriteFields<32, 40>(base, uint32_t(input3), 1ull);
  WriteFields<8, 44, 45>(base, uint32_t(input3), 0ull);
  WriteFields<32, 52>(base, uint32_t(input3), (input3 + 48ull));
  WriteFields<32, 48>(base, uint32_t(input3), 0xFFFFFFFF8239F988ull);
  WriteFields<32, 56>(base, uint32_t(input3), (input3 + 48ull));
  WriteFields<32, 60>(base, uint32_t(input3), 0ull);
  ctx.r6.u64 = 0xFFFFFFFF8239F988ull;
  ctx.r7.u64 = 1ull;
  ctx.r8.u64 = 0xFFFFFFFF823A06C0ull;
  ctx.r9.u64 = 0xFFFFFFFF823A0000ull;
  ctx.r10.u64 = (input3 + 48ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82713030) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 408>(base, uint32_t(input3), input4);
  const uint64_t field0 = GH_LOAD_U32(uint32_t(((uint64_t(std::rotl(uint32_t(input4), 2)) & 0xFFFFFFFCull) - 2110111436ull)) + 0u);
  WriteFields<32, 412>(base, uint32_t(input3), field0);
  const uint64_t field1 = GH_LOAD_U32(uint32_t(((uint64_t(std::rotl(uint32_t(input4), 2)) & 0xFFFFFFFCull) - 2110111420ull)) + 0u);
  WriteFields<32, 416>(base, uint32_t(input3), field1);
  const uint64_t value0 = ((((field0 & 0xFFFFFFFFull) - 1ull) & 32ull) ? 0ull : uint64_t(uint32_t(1ull) << (((field0 & 0xFFFFFFFFull) - 1ull) & 31ull)));
  WriteFields<32, 420>(base, uint32_t(input3), value0);
  const uint64_t value1 = ((((field1 & 0xFFFFFFFFull) - 1ull) & 32ull) ? 0ull : uint64_t(uint32_t(1ull) << (((field1 & 0xFFFFFFFFull) - 1ull) & 31ull)));
  WriteFields<32, 424>(base, uint32_t(input3), value1);
  WriteFields<32, 428>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(value0), 1)) & 0xFFFFFFFEull) - 1ull));
  WriteFields<32, 432>(base, uint32_t(input3), ((uint64_t(std::rotl(uint32_t(value1), 1)) & 0xFFFFFFFEull) - 1ull));
  ctx.r4.u64 = ((field1 & 0xFFFFFFFFull) - 1ull);
  ctx.r5.u64 = ((field0 & 0xFFFFFFFFull) - 1ull);
  ctx.r6.u64 = field1;
  ctx.r7.u64 = field0;
  ctx.r8.u64 = ((uint64_t(std::rotl(uint32_t(value1), 1)) & 0xFFFFFFFEull) - 1ull);
  ctx.r9.u64 = ((uint64_t(std::rotl(uint32_t(value0), 1)) & 0xFFFFFFFEull) - 1ull);
  ctx.r10.u64 = (uint64_t(std::rotl(uint32_t(value1), 1)) & 0xFFFFFFFEull);
  ctx.r11.u64 = (uint64_t(std::rotl(uint32_t(value0), 1)) & 0xFFFFFFFEull);
}

GH_HOOK(sub_82841488) {
  GH_PROLOGUE();
  WriteFields<32, -640>(base, uint32_t(0xFFFFFFFF82A00000ull), 0xFFFFFFFF82841170ull);
  WriteFields<32, -644>(base, uint32_t(0xFFFFFFFF82A00000ull), 0xFFFFFFFF82840CC8ull);
  WriteFields<32, -648>(base, uint32_t(0xFFFFFFFF82A00000ull), 0xFFFFFFFF8283FEF0ull);
  WriteFields<32, -652>(base, uint32_t(0xFFFFFFFF82A00000ull), 0xFFFFFFFF828408A8ull);
  WriteFields<32, -656>(base, uint32_t(0xFFFFFFFF82A00000ull), 0xFFFFFFFF828403C0ull);
  WriteFields<32, -660>(base, uint32_t(0xFFFFFFFF82A00000ull), 0xFFFFFFFF8283FDD8ull);
  ctx.r3.u64 = 0xFFFFFFFF82A00000ull;
  ctx.r4.u64 = 0xFFFFFFFF82A00000ull;
  ctx.r5.u64 = 0xFFFFFFFF82A00000ull;
  ctx.r6.u64 = 0xFFFFFFFF82840000ull;
  ctx.r7.u64 = 0xFFFFFFFF82840000ull;
  ctx.r8.u64 = 0xFFFFFFFF82840000ull;
  ctx.r9.u64 = 0xFFFFFFFF8283FDD8ull;
  ctx.r10.u64 = 0xFFFFFFFF828403C0ull;
  ctx.r11.u64 = 0xFFFFFFFF828408A8ull;
}

GH_HOOK(sub_82651948) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  WriteFields<32, 8>(base, uint32_t(input3), 0ull);
  WriteFields<32, 4>(base, uint32_t(input3), 0xFFFFFFFF82382484ull);
  WriteFields<32, 12>(base, uint32_t(input3), input4);
  WriteFields<32, 16>(base, uint32_t(input3), input5);
  WriteFields<32, 0>(base, uint32_t(input3), 0xFFFFFFFF82382508ull);
  WriteFields<32, 4>(base, uint32_t(input3), 0xFFFFFFFF823824DCull);
  WriteFields<32, 20, 24, 28, 32, 36, 40, 44, 48>(base, uint32_t(input3), 0ull);
  WriteFields<64, 56>(base, uint32_t(input3), 0ull);
  WriteFields<32, 64, 76, 68, 72>(base, uint32_t(input3), 0ull);
  ctx.r8.u64 = 0xFFFFFFFF823824DCull;
  ctx.r9.u64 = 0xFFFFFFFF82382508ull;
  ctx.r10.u64 = 0xFFFFFFFF82382484ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8260E368) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0>(base, uint32_t(input3), 0xFFFFFFFF8260E120ull);
  WriteFields<32, 112>(base, uint32_t(input3), 0xFFFFFFFF8236ABE8ull);
  WriteFields<32, 104>(base, uint32_t(input3), 0ull);
  WriteFields<32, 4>(base, uint32_t(input3), 0xFFFFFFFF8260E1A0ull);
  WriteFields<32, 8>(base, uint32_t(input3), 0xFFFFFFFF8260E170ull);
  WriteFields<32, 12>(base, uint32_t(input3), 0xFFFFFFFF8260E230ull);
  WriteFields<32, 16>(base, uint32_t(input3), 0xFFFFFFFF8260E350ull);
  WriteFields<32, 108, 20>(base, uint32_t(input3), 0ull);
  WriteFields<32, 116>(base, uint32_t(input3), 123ull);
  WriteFields<32, 120, 124, 128>(base, uint32_t(input3), 0ull);
  ctx.r4.u64 = 0xFFFFFFFF8260E120ull;
  ctx.r5.u64 = 123ull;
  ctx.r6.u64 = 0xFFFFFFFF8260E350ull;
  ctx.r7.u64 = 0xFFFFFFFF8260E230ull;
  ctx.r8.u64 = 0xFFFFFFFF8260E170ull;
  ctx.r9.u64 = 0xFFFFFFFF8260E1A0ull;
  ctx.r10.u64 = 0xFFFFFFFF8236ABE8ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8243EC90) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0, 4, 8, 12, 16, 20, 24, 28, 32, 36>(base, uint32_t(input3), 0ull);
  WriteFields<16, 40, 42>(base, uint32_t(input3), 0ull);
  WriteFields<32, 44, 48, 52, 56, 60>(base, uint32_t(input3), 256ull);
  WriteFields<16, 64, 66, 68, 70, 72, 74>(base, uint32_t(input3), 0ull);
  WriteFields<8, 76, 77, 78, 79>(base, uint32_t(input3), 0ull);
  ctx.r10.u64 = 256ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_825F95A8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U16(uint32_t(input4) + 3u);
  const uint64_t value0 = ((uint64_t(std::rotl(uint32_t(field0), 8)) & 65280ull) | (uint64_t(std::rotl(uint32_t(field0), 24)) & 16777215ull));
  WriteFields<16, 3>(base, uint32_t(input3), value0);
  const uint64_t field1 = GH_LOAD_U16(uint32_t(input4) + 5u);
  const uint64_t value1 = ((uint64_t(std::rotl(uint32_t(field1), 8)) & 65280ull) | (uint64_t(std::rotl(uint32_t(field1), 24)) & 16777215ull));
  WriteFields<16, 5>(base, uint32_t(input3), value1);
  const uint64_t field2 = GH_LOAD_U16(uint32_t(input4) + 8u);
  const uint64_t value2 = ((uint64_t(std::rotl(uint32_t(field2), 8)) & 65280ull) | (uint64_t(std::rotl(uint32_t(field2), 24)) & 16777215ull));
  WriteFields<16, 8>(base, uint32_t(input3), value2);
  const uint64_t field3 = GH_LOAD_U16(uint32_t(input4) + 10u);
  const uint64_t value3 = ((uint64_t(std::rotl(uint32_t(field3), 8)) & 65280ull) | (uint64_t(std::rotl(uint32_t(field3), 24)) & 16777215ull));
  WriteFields<16, 10>(base, uint32_t(input3), value3);
  const uint64_t field4 = GH_LOAD_U16(uint32_t(input4) + 12u);
  const uint64_t value4 = ((uint64_t(std::rotl(uint32_t(field4), 8)) & 65280ull) | (uint64_t(std::rotl(uint32_t(field4), 24)) & 16777215ull));
  WriteFields<16, 12>(base, uint32_t(input3), value4);
  const uint64_t field5 = GH_LOAD_U16(uint32_t(input4) + 14u);
  const uint64_t value5 = ((uint64_t(std::rotl(uint32_t(field5), 8)) & 65280ull) | (uint64_t(std::rotl(uint32_t(field5), 24)) & 16777215ull));
  WriteFields<16, 14>(base, uint32_t(input3), value5);
  ctx.r10.u64 = (uint64_t(std::rotl(uint32_t(field5), 8)) & 65280ull);
  ctx.r11.u64 = value5;
}

GH_HOOK(sub_826F21C8) {
  GH_PROLOGUE();
  WriteFields<32, 17576, 17580>(base, uint32_t(0xFFFFFFFF829E0000ull), 0ull);
  WriteFields<32, 25568>(base, uint32_t(0xFFFFFFFF82940000ull), 1ull);
  WriteFields<32, 17584, 17588, 17592>(base, uint32_t(0xFFFFFFFF829E0000ull), 0ull);
  WriteFields<8, 17596>(base, uint32_t(0xFFFFFFFF829E0000ull), 0ull);
  WriteFields<32, 17608, 17600, 17604>(base, uint32_t(0xFFFFFFFF829E0000ull), 0ull);
  WriteFields<32, 25572>(base, uint32_t(0xFFFFFFFF82940000ull), 83887040ull);
  WriteFields<32, 17612>(base, uint32_t(0xFFFFFFFF829E0000ull), 0ull);
  ctx.r5.u64 = 0xFFFFFFFF829E0000ull;
  ctx.r6.u64 = 0xFFFFFFFF82940000ull;
  ctx.r7.u64 = 0xFFFFFFFF829E0000ull;
  ctx.r8.u64 = 0xFFFFFFFF829E0000ull;
  ctx.r9.u64 = 0ull;
  ctx.r10.u64 = 0ull;
  ctx.r11.u64 = 83887040ull;
}

GH_HOOK(sub_825F7608) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U16(uint32_t(input4) + 0u);
  const uint64_t value0 = ((uint64_t(std::rotl(uint32_t(field0), 8)) & 65280ull) | (uint64_t(std::rotl(uint32_t(field0), 24)) & 16777215ull));
  WriteFields<16, 0>(base, uint32_t(input3), value0);
  const uint64_t field1 = GH_LOAD_U32(uint32_t(input4) + 2u);
  const uint64_t value1 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field1), 16)) & 65535ull) | (field1 & 0xFFFFFFFFFFFF0000ull))), 24));
  const uint64_t value2 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field1), 16)) & 0xFFFF0000ull) | (field1 & 0xFFFFFFFF0000FFFFull))), 8));
  WriteFields<32, 2>(base, uint32_t(input3), ((value1 & 65535ull) | (value2 & 0xFFFF0000ull)));
  const uint64_t field2 = GH_LOAD_U16(uint32_t(input4) + 6u);
  const uint64_t value3 = ((uint64_t(std::rotl(uint32_t(field2), 8)) & 65280ull) | (uint64_t(std::rotl(uint32_t(field2), 24)) & 16777215ull));
  WriteFields<16, 6>(base, uint32_t(input3), value3);
  const uint64_t field3 = GH_LOAD_U16(uint32_t(input4) + 8u);
  const uint64_t value4 = ((uint64_t(std::rotl(uint32_t(field3), 8)) & 65280ull) | (uint64_t(std::rotl(uint32_t(field3), 24)) & 16777215ull));
  WriteFields<16, 8>(base, uint32_t(input3), value4);
  const uint64_t field4 = GH_LOAD_U32(uint32_t(input4) + 10u);
  const uint64_t value5 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field4), 16)) & 65535ull) | (field4 & 0xFFFFFFFFFFFF0000ull))), 24));
  const uint64_t value6 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field4), 16)) & 0xFFFF0000ull) | (field4 & 0xFFFFFFFF0000FFFFull))), 8));
  WriteFields<32, 10>(base, uint32_t(input3), ((value5 & 65535ull) | (value6 & 0xFFFF0000ull)));
  ctx.r9.u64 = ((uint64_t(std::rotl(uint32_t(field4), 16)) & 0xFFFF0000ull) | (field4 & 0xFFFFFFFF0000FFFFull));
  ctx.r10.u64 = (value6 & 0xFFFF0000ull);
  ctx.r11.u64 = ((value5 & 65535ull) | (value6 & 0xFFFF0000ull));
}

GH_HOOK(sub_825D90F8) {
  GH_PROLOGUE();
  const uint64_t input1 = ctx.r1.u64;
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t input30 = ctx.r30.u64;
  const uint64_t input31 = ctx.r31.u64;
  WriteFields<64, -16>(base, uint32_t(input1), input30);
  WriteFields<64, -8>(base, uint32_t(input1), input31);
  const uint64_t field0 = GH_LOAD_U8(uint32_t((input3 + input4)) + 10890u);
  const uint64_t field1 = GH_LOAD_U32(uint32_t((((input4 + 48ull) * 24ull) + input3)) + 12u);
  const uint64_t field2 = GH_LOAD_U32(uint32_t((((input4 + 48ull) * 24ull) + input3)) + 16u);
  const uint64_t value0 = (((uint64_t(std::rotl(uint32_t(field1), 31)) & 524287ull) | (field1 & 0xFFFFFFFFFFF80000ull)) & 0xFFFFFFFF800FFFFFull);
  const uint64_t value1 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field1), 31)) & 2146435072ull) | value0)), 13));
  const uint64_t value2 = ((value1 & 4095ull) & ((uint64_t(std::rotl(uint32_t(((field0 & 0xFFFFFFFEull) | input5)), 30)) & 1073741823ull) - 1ull));
  const uint64_t value3 = (((field0 & 0xFFFFFFFEull) | input5) & ~((uint64_t(std::rotl(uint32_t(((field0 & 0xFFFFFFFEull) | input5)), 30)) & 1073741823ull) - 1ull));
  WriteFields<32, 16>(base, uint32_t((((input4 + 48ull) * 24ull) + input3)), ((field2 & 0xFFFFFFFCull) | ((value2 + value3) & 0xFFFFFFFF00000003ull)));
  WriteFields<8, 10890>(base, uint32_t((input3 + input4)), ((field0 & 0xFFFFFFFEull) | input5));
  const uint64_t field3 = GH_LOAD_U64(uint32_t(input3) + 24u);
  const uint64_t value4 = ((((input4 + 32ull) & 0xFFFFFFFFull) & 64ull) ? 0ull : (0x8000000000000000ull >> (((input4 + 32ull) & 0xFFFFFFFFull) & 63ull)));
  WriteFields<64, 24>(base, uint32_t(input3), (value4 | field3));
  const uint64_t field4 = GH_LOAD_U64(uint32_t(input1) + -16u);
  const uint64_t field5 = GH_LOAD_U64(uint32_t(input1) + -8u);
  ctx.r4.u64 = field2;
  ctx.r5.u64 = value4;
  ctx.r6.u64 = ((field0 & 0xFFFFFFFEull) | input5);
  ctx.r7.u64 = value2;
  ctx.r8.u64 = ((field2 & 0xFFFFFFFCull) | ((value2 + value3) & 0xFFFFFFFF00000003ull));
  ctx.r9.u64 = ((input3 + input4) + 10890ull);
  ctx.r10.u64 = (((input4 + 48ull) * 24ull) + input3);
  ctx.r11.u64 = (value4 | field3);
  ctx.r30.u64 = field4;
  ctx.r31.u64 = field5;
}

GH_HOOK(sub_82850218) {
  GH_PROLOGUE();
  const uint64_t input1 = ctx.r1.u64;
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input30 = ctx.r30.u64;
  const uint64_t input31 = ctx.r31.u64;
  WriteFields<64, -16>(base, uint32_t(input1), input30);
  WriteFields<64, -8>(base, uint32_t(input1), input31);
  WriteFields<32, 15912>(base, uint32_t(input3), 0xFFFFFFFF8284EFA8ull);
  WriteFields<32, 3172>(base, uint32_t(input3), 0xFFFFFFFF8284EF58ull);
  WriteFields<32, 20720>(base, uint32_t(input3), 0xFFFFFFFF8284F7A0ull);
  WriteFields<32, 20712>(base, uint32_t(input3), 0xFFFFFFFF82850018ull);
  WriteFields<32, 20716>(base, uint32_t(input3), 0xFFFFFFFF82850118ull);
  WriteFields<32, 15896>(base, uint32_t(input3), 0xFFFFFFFF8284EF28ull);
  WriteFields<32, 15900>(base, uint32_t(input3), 0xFFFFFFFF828A5D48ull);
  WriteFields<32, 15904>(base, uint32_t(input3), 0xFFFFFFFF828A5BC0ull);
  WriteFields<32, 15908>(base, uint32_t(input3), 0xFFFFFFFF8284EE30ull);
  WriteFields<32, 3168>(base, uint32_t(input3), 0xFFFFFFFF828786D0ull);
  const uint64_t field0 = GH_LOAD_U64(uint32_t(input1) + -16u);
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input1) + -8u);
  ctx.r4.u64 = 0xFFFFFFFF828A5BC0ull;
  ctx.r5.u64 = 0xFFFFFFFF828A5D48ull;
  ctx.r6.u64 = 0xFFFFFFFF8284EF28ull;
  ctx.r7.u64 = 0xFFFFFFFF82850118ull;
  ctx.r8.u64 = 0xFFFFFFFF82850018ull;
  ctx.r9.u64 = 0xFFFFFFFF8284F7A0ull;
  ctx.r10.u64 = 0xFFFFFFFF828786D0ull;
  ctx.r11.u64 = 0xFFFFFFFF8284EE30ull;
  ctx.r30.u64 = field0;
  ctx.r31.u64 = field1;
}

GH_HOOK(sub_825D8F50) {
  GH_PROLOGUE();
  const uint64_t input1 = ctx.r1.u64;
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t input30 = ctx.r30.u64;
  const uint64_t input31 = ctx.r31.u64;
  WriteFields<64, -16>(base, uint32_t(input1), input30);
  WriteFields<64, -8>(base, uint32_t(input1), input31);
  const uint64_t field0 = GH_LOAD_U8(uint32_t((input3 + input4)) + 10890u);
  const uint64_t field1 = GH_LOAD_U32(uint32_t((((input4 + 48ull) * 24ull) + input3)) + 12u);
  const uint64_t field2 = GH_LOAD_U32(uint32_t((((input4 + 48ull) * 24ull) + input3)) + 16u);
  const uint64_t value0 = (((uint64_t(std::rotl(uint32_t(field1), 31)) & 524287ull) | (field1 & 0xFFFFFFFFFFF80000ull)) & 0xFFFFFFFF800FFFFFull);
  const uint64_t value1 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field1), 31)) & 2146435072ull) | value0)), 13));
  const uint64_t value2 = (((uint64_t(uint32_t((field0 & 0xFFFFFFFFull))) * 0x100000001ull) & 0xFFFFFFFFFFFFFFFDull) | (uint64_t(std::rotl(uint32_t(input5), 1)) & 0xFFFFFFFEull));
  const uint64_t value3 = (((value1 & 4095ull) & ((uint64_t(std::rotl(uint32_t(value2), 30)) & 1073741823ull) - 1ull)) + (value2 & ~((uint64_t(std::rotl(uint32_t(value2), 30)) & 1073741823ull) - 1ull)));
  WriteFields<32, 16>(base, uint32_t((((input4 + 48ull) * 24ull) + input3)), ((field2 & 0xFFFFFFFCull) | (value3 & 0xFFFFFFFF00000003ull)));
  WriteFields<8, 10890>(base, uint32_t((input3 + input4)), value2);
  const uint64_t field3 = GH_LOAD_U64(uint32_t(input3) + 24u);
  const uint64_t value4 = ((((input4 + 32ull) & 0xFFFFFFFFull) & 64ull) ? 0ull : (0x8000000000000000ull >> (((input4 + 32ull) & 0xFFFFFFFFull) & 63ull)));
  WriteFields<64, 24>(base, uint32_t(input3), (value4 | field3));
  const uint64_t field4 = GH_LOAD_U64(uint32_t(input1) + -16u);
  const uint64_t field5 = GH_LOAD_U64(uint32_t(input1) + -8u);
  ctx.r4.u64 = field2;
  ctx.r5.u64 = 0x8000000000000000ull;
  ctx.r6.u64 = value2;
  ctx.r7.u64 = value4;
  ctx.r8.u64 = ((field2 & 0xFFFFFFFCull) | (value3 & 0xFFFFFFFF00000003ull));
  ctx.r9.u64 = ((input3 + input4) + 10890ull);
  ctx.r10.u64 = (((input4 + 48ull) * 24ull) + input3);
  ctx.r11.u64 = (value4 | field3);
  ctx.r30.u64 = field4;
  ctx.r31.u64 = field5;
}

GH_HOOK(sub_825D91F0) {
  GH_PROLOGUE();
  const uint64_t input1 = ctx.r1.u64;
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t input30 = ctx.r30.u64;
  const uint64_t input31 = ctx.r31.u64;
  WriteFields<64, -16>(base, uint32_t(input1), input30);
  WriteFields<64, -8>(base, uint32_t(input1), input31);
  const uint64_t field0 = GH_LOAD_U8(uint32_t((input3 + input4)) + 10890u);
  const uint64_t field1 = GH_LOAD_U32(uint32_t((((input4 + 48ull) * 24ull) + input3)) + 12u);
  const uint64_t field2 = GH_LOAD_U32(uint32_t((((input4 + 48ull) * 24ull) + input3)) + 16u);
  const uint64_t value0 = (((uint64_t(std::rotl(uint32_t(field1), 31)) & 524287ull) | (field1 & 0xFFFFFFFFFFF80000ull)) & 0xFFFFFFFF800FFFFFull);
  const uint64_t value1 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field1), 31)) & 2146435072ull) | value0)), 13));
  const uint64_t value2 = (((uint64_t(uint32_t((field0 & 0xFFFFFFFFull))) * 0x100000001ull) & 0xFFFFFFFFFFFFFFFBull) | (uint64_t(std::rotl(uint32_t(input5), 2)) & 0xFFFFFFFCull));
  const uint64_t value3 = (((value1 & 4095ull) & ((uint64_t(std::rotl(uint32_t(value2), 30)) & 1073741823ull) - 1ull)) + (value2 & ~((uint64_t(std::rotl(uint32_t(value2), 30)) & 1073741823ull) - 1ull)));
  WriteFields<32, 16>(base, uint32_t((((input4 + 48ull) * 24ull) + input3)), ((field2 & 0xFFFFFFFCull) | (value3 & 0xFFFFFFFF00000003ull)));
  WriteFields<8, 10890>(base, uint32_t((input3 + input4)), value2);
  const uint64_t field3 = GH_LOAD_U64(uint32_t(input3) + 24u);
  const uint64_t value4 = ((((input4 + 32ull) & 0xFFFFFFFFull) & 64ull) ? 0ull : (0x8000000000000000ull >> (((input4 + 32ull) & 0xFFFFFFFFull) & 63ull)));
  WriteFields<64, 24>(base, uint32_t(input3), (value4 | field3));
  const uint64_t field4 = GH_LOAD_U64(uint32_t(input1) + -16u);
  const uint64_t field5 = GH_LOAD_U64(uint32_t(input1) + -8u);
  ctx.r4.u64 = field2;
  ctx.r5.u64 = 0x8000000000000000ull;
  ctx.r6.u64 = value2;
  ctx.r7.u64 = value4;
  ctx.r8.u64 = ((field2 & 0xFFFFFFFCull) | (value3 & 0xFFFFFFFF00000003ull));
  ctx.r9.u64 = ((input3 + input4) + 10890ull);
  ctx.r10.u64 = (((input4 + 48ull) * 24ull) + input3);
  ctx.r11.u64 = (value4 | field3);
  ctx.r30.u64 = field4;
  ctx.r31.u64 = field5;
}

GH_HOOK(sub_82886AD8) {
  GH_PROLOGUE();
  const uint64_t input1 = ctx.r1.u64;
  const uint64_t input30 = ctx.r30.u64;
  const uint64_t input31 = ctx.r31.u64;
  WriteFields<64, -16>(base, uint32_t(input1), input30);
  WriteFields<64, -8>(base, uint32_t(input1), input31);
  WriteFields<32, -25928>(base, uint32_t(0xFFFFFFFF82980000ull), 0xFFFFFFFF8288D618ull);
  WriteFields<32, 4>(base, uint32_t(0xFFFFFFFF82979AB8ull), 0xFFFFFFFF8288CAB8ull);
  WriteFields<32, 8>(base, uint32_t(0xFFFFFFFF82979AB8ull), 0xFFFFFFFF8288CAA8ull);
  WriteFields<32, 12>(base, uint32_t(0xFFFFFFFF82979AB8ull), 0xFFFFFFFF8288CAB0ull);
  WriteFields<32, 16>(base, uint32_t(0xFFFFFFFF82979AB8ull), 0xFFFFFFFF8288CA28ull);
  WriteFields<32, 20>(base, uint32_t(0xFFFFFFFF82979AB8ull), 0xFFFFFFFF8288D618ull);
  WriteFields<32, 24>(base, uint32_t(0xFFFFFFFF82979AB8ull), 0xFFFFFFFF8288D5C8ull);
  WriteFields<32, 28>(base, uint32_t(0xFFFFFFFF82979AB8ull), 0xFFFFFFFF8288CA48ull);
  WriteFields<32, 32>(base, uint32_t(0xFFFFFFFF82979AB8ull), 0xFFFFFFFF8288C978ull);
  WriteFields<32, 36>(base, uint32_t(0xFFFFFFFF82979AB8ull), 0xFFFFFFFF8288C8D8ull);
  const uint64_t field0 = GH_LOAD_U64(uint32_t(input1) + -16u);
  const uint64_t field1 = GH_LOAD_U64(uint32_t(input1) + -8u);
  ctx.r3.u64 = 0xFFFFFFFF82890000ull;
  ctx.r4.u64 = 0xFFFFFFFF82890000ull;
  ctx.r5.u64 = 0xFFFFFFFF82890000ull;
  ctx.r6.u64 = 0xFFFFFFFF82890000ull;
  ctx.r7.u64 = 0xFFFFFFFF82890000ull;
  ctx.r8.u64 = 0xFFFFFFFF82890000ull;
  ctx.r9.u64 = 0xFFFFFFFF8288C978ull;
  ctx.r10.u64 = 0xFFFFFFFF8288CA48ull;
  ctx.r11.u64 = 0xFFFFFFFF8288C8D8ull;
  ctx.r30.u64 = field0;
  ctx.r31.u64 = field1;
}

GH_HOOK(sub_82775648) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input4) + 0u);
  WriteFields<32, 30976>(base, uint32_t(input3), field0);
  const uint64_t field1 = GH_LOAD_U32(uint32_t(input4) + 4u);
  WriteFields<32, 30980>(base, uint32_t(input3), field1);
  const uint64_t field2 = GH_LOAD_U32(uint32_t(input4) + 8u);
  WriteFields<32, 30984>(base, uint32_t(input3), field2);
  const uint64_t field3 = GH_LOAD_U32(uint32_t(input4) + 12u);
  WriteFields<32, 30988>(base, uint32_t(input3), field3);
  const uint64_t field4 = GH_LOAD_U32(uint32_t(input4) + 16u);
  WriteFields<32, 30992>(base, uint32_t(input3), field4);
  const uint64_t field5 = GH_LOAD_U32(uint32_t(input4) + 20u);
  WriteFields<32, 30996>(base, uint32_t(input3), field5);
  const uint64_t field6 = GH_LOAD_U32(uint32_t(input4) + 28u);
  WriteFields<32, 31004>(base, uint32_t(input3), field6);
  const uint64_t field7 = GH_LOAD_U32(uint32_t(input4) + 24u);
  WriteFields<32, 31000>(base, uint32_t(input3), field7);
  const uint64_t field8 = GH_LOAD_U32(uint32_t(input4) + 32u);
  WriteFields<32, 31008>(base, uint32_t(input3), field8);
  const uint64_t field9 = GH_LOAD_U32(uint32_t(input4) + 36u);
  WriteFields<32, 31012>(base, uint32_t(input3), field9);
  const uint64_t field10 = GH_LOAD_U32(uint32_t(input4) + 40u);
  WriteFields<32, 31016>(base, uint32_t(input3), field10);
  const uint64_t field11 = GH_LOAD_U32(uint32_t(input4) + 44u);
  WriteFields<32, 31020>(base, uint32_t(input3), field11);
  const uint64_t field12 = GH_LOAD_U32(uint32_t(input4) + 48u);
  WriteFields<32, 31028>(base, uint32_t(input3), field12);
  const uint64_t field13 = GH_LOAD_U32(uint32_t(input4) + 52u);
  WriteFields<32, 31032>(base, uint32_t(input3), field13);
  const uint64_t field14 = GH_LOAD_U32(uint32_t(input4) + 64u);
  WriteFields<32, 31036>(base, uint32_t(input3), field14);
  const uint64_t field15 = GH_LOAD_U32(uint32_t(input4) + 56u);
  WriteFields<32, 31040>(base, uint32_t(input3), field15);
  const uint64_t field16 = GH_LOAD_U32(uint32_t(input4) + 68u);
  WriteFields<32, 31044>(base, uint32_t(input3), field16);
  const uint64_t field17 = GH_LOAD_U32(uint32_t(input4) + 72u);
  WriteFields<32, 31048>(base, uint32_t(input3), field17);
  const uint64_t field18 = GH_LOAD_U32(uint32_t(input4) + 76u);
  WriteFields<32, 31052>(base, uint32_t(input3), field18);
  const uint64_t field19 = GH_LOAD_U32(uint32_t(input4) + 84u);
  WriteFields<32, 31060>(base, uint32_t(input3), field19);
  const uint64_t field20 = GH_LOAD_U32(uint32_t(input4) + 80u);
  WriteFields<32, 31056>(base, uint32_t(input3), field20);
  const uint64_t field21 = GH_LOAD_U32(uint32_t(input4) + 92u);
  WriteFields<32, 31068>(base, uint32_t(input3), field21);
  const uint64_t field22 = GH_LOAD_U32(uint32_t(input4) + 96u);
  WriteFields<32, 31072>(base, uint32_t(input3), field22);
  const uint64_t field23 = GH_LOAD_U32(uint32_t(input4) + 88u);
  WriteFields<32, 31064>(base, uint32_t(input3), field23);
  const uint64_t field24 = GH_LOAD_U32(uint32_t(input4) + 60u);
  WriteFields<32, 31080>(base, uint32_t(input3), field24);
  const uint64_t field25 = GH_LOAD_U32(uint32_t(input4) + 100u);
  WriteFields<32, 31076>(base, uint32_t(input3), field25);
  ctx.r5.u64 = field20;
  ctx.r6.u64 = field19;
  ctx.r7.u64 = field25;
  ctx.r8.u64 = field24;
  ctx.r9.u64 = field23;
  ctx.r10.u64 = field22;
  ctx.r11.u64 = field21;
}

GH_HOOK(sub_82774DE0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 7868, 7764, 7768, 7772, 19200, 19112, 19196, 19204, 7612, 7608, 7604, 7624, 7664, 7780, 7784, 7788, 7792, 7796, 7800, 7776, 7832, 6808, 6812, 6816, 6820, 6824, 6836, 6840, 2268, 772, 3460, 3464, 3492, 3020, 3592, 3596, 4428, 4432, 4460, 3988, 4560, 4564, 5396, 5400, 5428, 4956, 5528, 5532, 6364, 6368, 6396, 5924, 6496, 6500, 3400, 3404, 3408, 3396, 3016>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_825FA528) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input4) + 0u);
  const uint64_t value0 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field0), 16)) & 65535ull) | (field0 & 0xFFFFFFFFFFFF0000ull))), 24));
  const uint64_t value1 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field0), 16)) & 0xFFFF0000ull) | (field0 & 0xFFFFFFFF0000FFFFull))), 8));
  WriteFields<32, 0>(base, uint32_t(input3), ((value0 & 65535ull) | (value1 & 0xFFFF0000ull)));
  const uint64_t field1 = GH_LOAD_U32(uint32_t(input4) + 4u);
  const uint64_t value2 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field1), 16)) & 65535ull) | (field1 & 0xFFFFFFFFFFFF0000ull))), 24));
  const uint64_t value3 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field1), 16)) & 0xFFFF0000ull) | (field1 & 0xFFFFFFFF0000FFFFull))), 8));
  WriteFields<32, 4>(base, uint32_t(input3), ((value2 & 65535ull) | (value3 & 0xFFFF0000ull)));
  const uint64_t field2 = GH_LOAD_U32(uint32_t(input4) + 8u);
  const uint64_t value4 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field2), 16)) & 65535ull) | (field2 & 0xFFFFFFFFFFFF0000ull))), 24));
  const uint64_t value5 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field2), 16)) & 0xFFFF0000ull) | (field2 & 0xFFFFFFFF0000FFFFull))), 8));
  WriteFields<32, 8>(base, uint32_t(input3), ((value4 & 65535ull) | (value5 & 0xFFFF0000ull)));
  const uint64_t field3 = GH_LOAD_U32(uint32_t(input4) + 12u);
  const uint64_t value6 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field3), 16)) & 65535ull) | (field3 & 0xFFFFFFFFFFFF0000ull))), 24));
  const uint64_t value7 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field3), 16)) & 0xFFFF0000ull) | (field3 & 0xFFFFFFFF0000FFFFull))), 8));
  WriteFields<32, 12>(base, uint32_t(input3), ((value6 & 65535ull) | (value7 & 0xFFFF0000ull)));
  const uint64_t field4 = GH_LOAD_U32(uint32_t(input4) + 16u);
  const uint64_t value8 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field4), 16)) & 65535ull) | (field4 & 0xFFFFFFFFFFFF0000ull))), 24));
  const uint64_t value9 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field4), 16)) & 0xFFFF0000ull) | (field4 & 0xFFFFFFFF0000FFFFull))), 8));
  WriteFields<32, 16>(base, uint32_t(input3), ((value8 & 65535ull) | (value9 & 0xFFFF0000ull)));
  const uint64_t field5 = GH_LOAD_U32(uint32_t(input4) + 20u);
  const uint64_t value10 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field5), 16)) & 65535ull) | (field5 & 0xFFFFFFFFFFFF0000ull))), 24));
  const uint64_t value11 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field5), 16)) & 0xFFFF0000ull) | (field5 & 0xFFFFFFFF0000FFFFull))), 8));
  WriteFields<32, 20>(base, uint32_t(input3), ((value10 & 65535ull) | (value11 & 0xFFFF0000ull)));
  const uint64_t field6 = GH_LOAD_U32(uint32_t(input4) + 24u);
  const uint64_t value12 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field6), 16)) & 65535ull) | (field6 & 0xFFFFFFFFFFFF0000ull))), 24));
  const uint64_t value13 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field6), 16)) & 0xFFFF0000ull) | (field6 & 0xFFFFFFFF0000FFFFull))), 8));
  WriteFields<32, 24>(base, uint32_t(input3), ((value12 & 65535ull) | (value13 & 0xFFFF0000ull)));
  const uint64_t field7 = GH_LOAD_U32(uint32_t(input4) + 28u);
  const uint64_t value14 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field7), 16)) & 65535ull) | (field7 & 0xFFFFFFFFFFFF0000ull))), 24));
  const uint64_t value15 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field7), 16)) & 0xFFFF0000ull) | (field7 & 0xFFFFFFFF0000FFFFull))), 8));
  WriteFields<32, 28>(base, uint32_t(input3), ((value14 & 65535ull) | (value15 & 0xFFFF0000ull)));
  ctx.r9.u64 = ((uint64_t(std::rotl(uint32_t(field7), 16)) & 0xFFFF0000ull) | (field7 & 0xFFFFFFFF0000FFFFull));
  ctx.r10.u64 = (value15 & 0xFFFF0000ull);
  ctx.r11.u64 = ((value14 & 65535ull) | (value15 & 0xFFFF0000ull));
}

GH_HOOK(sub_827FB8B0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0, 4, 8>(base, uint32_t(input3), 0ull);
  WriteFields<32, 12, 16>(base, uint32_t(input3), 2ull);
  WriteFields<32, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 72, 76, 80, 84, 88, 92, 14464, 14468>(base, uint32_t(input3), 0ull);
  WriteFields<32, 14472, 14476>(base, uint32_t(input3), 1ull);
  WriteFields<32, 14480, 14484, 14488, 14492, 14496, 14500, 14504, 14508, 14512, 14516, 14520, 14524, 14528, 14532, 14536, 14540, 14544, 14548, 14552, 14556>(base, uint32_t(input3), 0ull);
  WriteFields<32, 14560>(base, uint32_t(input3), 1ull);
  WriteFields<32, 14580, 14584, 14588, 14592, 14596, 14600, 14604, 14608, 14612, 14616, 14620, 14624, 14628, 14632, 14636, 14640, 14644, 14648, 14652, 14656, 14660, 14664>(base, uint32_t(input3), 0ull);
  ctx.r9.u64 = 2ull;
  ctx.r10.u64 = 1ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_827D36B8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 12, 16>(base, uint32_t(input3), 2ull);
  WriteFields<32, 0, 4, 8, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 72, 76, 80, 84, 88, 14460, 14464>(base, uint32_t(input3), 0ull);
  WriteFields<32, 14468, 14472>(base, uint32_t(input3), 1ull);
  WriteFields<32, 14476, 14480, 14484, 14488, 14492, 14496, 14500, 14504, 14508, 14512, 14516, 14520, 14524, 14528, 14532, 14536, 14540, 14544, 14548, 14552>(base, uint32_t(input3), 0ull);
  WriteFields<32, 14556>(base, uint32_t(input3), 1ull);
  WriteFields<32, 14612, 14616, 14620, 14624, 14628, 14632, 14636, 14640, 14644, 14648, 14652, 14656, 14660, 14664, 14668, 14672, 14676, 14680, 14704, 14708, 14712, 14716>(base, uint32_t(input3), 0ull);
  WriteFields<32, 14684>(base, uint32_t(input3), 1ull);
  WriteFields<32, 14692, 14696, 14700>(base, uint32_t(input3), 0xFFFFFFFFFFFFFFFFull);
  WriteFields<32, 14688>(base, uint32_t(input3), 1ull);
  ctx.r9.u64 = 0xFFFFFFFFFFFFFFFFull;
  ctx.r10.u64 = 1ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_825F7690) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input4) + 0u);
  const uint64_t value0 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field0), 16)) & 65535ull) | (field0 & 0xFFFFFFFFFFFF0000ull))), 24));
  const uint64_t value1 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field0), 16)) & 0xFFFF0000ull) | (field0 & 0xFFFFFFFF0000FFFFull))), 8));
  WriteFields<32, 0>(base, uint32_t(input3), ((value0 & 65535ull) | (value1 & 0xFFFF0000ull)));
  const uint64_t field1 = GH_LOAD_U32(uint32_t(input4) + 4u);
  const uint64_t value2 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field1), 16)) & 65535ull) | (field1 & 0xFFFFFFFFFFFF0000ull))), 24));
  const uint64_t value3 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field1), 16)) & 0xFFFF0000ull) | (field1 & 0xFFFFFFFF0000FFFFull))), 8));
  WriteFields<32, 4>(base, uint32_t(input3), ((value2 & 65535ull) | (value3 & 0xFFFF0000ull)));
  const uint64_t field2 = GH_LOAD_U32(uint32_t(input4) + 8u);
  const uint64_t value4 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field2), 16)) & 65535ull) | (field2 & 0xFFFFFFFFFFFF0000ull))), 24));
  const uint64_t value5 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field2), 16)) & 0xFFFF0000ull) | (field2 & 0xFFFFFFFF0000FFFFull))), 8));
  WriteFields<32, 8>(base, uint32_t(input3), ((value4 & 65535ull) | (value5 & 0xFFFF0000ull)));
  const uint64_t field3 = GH_LOAD_U16(uint32_t(input4) + 12u);
  const uint64_t value6 = ((uint64_t(std::rotl(uint32_t(field3), 8)) & 65280ull) | (uint64_t(std::rotl(uint32_t(field3), 24)) & 16777215ull));
  WriteFields<16, 12>(base, uint32_t(input3), value6);
  const uint64_t field4 = GH_LOAD_U16(uint32_t(input4) + 14u);
  const uint64_t value7 = ((uint64_t(std::rotl(uint32_t(field4), 8)) & 65280ull) | (uint64_t(std::rotl(uint32_t(field4), 24)) & 16777215ull));
  WriteFields<16, 14>(base, uint32_t(input3), value7);
  const uint64_t field5 = GH_LOAD_U32(uint32_t(input4) + 16u);
  const uint64_t value8 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field5), 16)) & 65535ull) | (field5 & 0xFFFFFFFFFFFF0000ull))), 24));
  const uint64_t value9 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field5), 16)) & 0xFFFF0000ull) | (field5 & 0xFFFFFFFF0000FFFFull))), 8));
  WriteFields<32, 16>(base, uint32_t(input3), ((value8 & 65535ull) | (value9 & 0xFFFF0000ull)));
  const uint64_t field6 = GH_LOAD_U32(uint32_t(input4) + 20u);
  const uint64_t value10 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field6), 16)) & 65535ull) | (field6 & 0xFFFFFFFFFFFF0000ull))), 24));
  const uint64_t value11 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field6), 16)) & 0xFFFF0000ull) | (field6 & 0xFFFFFFFF0000FFFFull))), 8));
  WriteFields<32, 20>(base, uint32_t(input3), ((value10 & 65535ull) | (value11 & 0xFFFF0000ull)));
  const uint64_t field7 = GH_LOAD_U32(uint32_t(input4) + 24u);
  const uint64_t value12 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field7), 16)) & 65535ull) | (field7 & 0xFFFFFFFFFFFF0000ull))), 24));
  const uint64_t value13 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field7), 16)) & 0xFFFF0000ull) | (field7 & 0xFFFFFFFF0000FFFFull))), 8));
  WriteFields<32, 24>(base, uint32_t(input3), ((value12 & 65535ull) | (value13 & 0xFFFF0000ull)));
  const uint64_t field8 = GH_LOAD_U32(uint32_t(input4) + 28u);
  const uint64_t value14 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field8), 16)) & 65535ull) | (field8 & 0xFFFFFFFFFFFF0000ull))), 24));
  const uint64_t value15 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field8), 16)) & 0xFFFF0000ull) | (field8 & 0xFFFFFFFF0000FFFFull))), 8));
  WriteFields<32, 28>(base, uint32_t(input3), ((value14 & 65535ull) | (value15 & 0xFFFF0000ull)));
  const uint64_t field9 = GH_LOAD_U32(uint32_t(input4) + 32u);
  const uint64_t value16 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field9), 16)) & 65535ull) | (field9 & 0xFFFFFFFFFFFF0000ull))), 24));
  const uint64_t value17 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field9), 16)) & 0xFFFF0000ull) | (field9 & 0xFFFFFFFF0000FFFFull))), 8));
  WriteFields<32, 32>(base, uint32_t(input3), ((value16 & 65535ull) | (value17 & 0xFFFF0000ull)));
  const uint64_t field10 = GH_LOAD_U32(uint32_t(input4) + 36u);
  const uint64_t value18 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field10), 16)) & 65535ull) | (field10 & 0xFFFFFFFFFFFF0000ull))), 24));
  const uint64_t value19 = uint64_t(std::rotl(uint32_t(((uint64_t(std::rotl(uint32_t(field10), 16)) & 0xFFFF0000ull) | (field10 & 0xFFFFFFFF0000FFFFull))), 8));
  WriteFields<32, 36>(base, uint32_t(input3), ((value18 & 65535ull) | (value19 & 0xFFFF0000ull)));
  ctx.r9.u64 = ((uint64_t(std::rotl(uint32_t(field10), 16)) & 0xFFFF0000ull) | (field10 & 0xFFFFFFFF0000FFFFull));
  ctx.r10.u64 = (value19 & 0xFFFF0000ull);
  ctx.r11.u64 = ((value18 & 65535ull) | (value19 & 0xFFFF0000ull));
}
