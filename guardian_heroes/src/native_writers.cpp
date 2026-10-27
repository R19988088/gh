#include "native_runtime.h"
#include "native_memory.h"

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
  // Keep scalar accesses for aliases, MMIO, and wrapping address ranges.
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

GH_HOOK(sub_8262E8F8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<8, 4>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_827789A0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 1416u);
  WriteFields<32, 2428>(base, uint32_t(input3), 0ull);
  WriteFields<8, 2432>(base, uint32_t(input3), 0ull);
  WriteFields<32, 2436>(base, uint32_t(input3), 0ull);
  WriteFields<8, 2433>(base, uint32_t(input3), field0);
  ctx.r10.u64 = field0;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8281E1F8) {
  GH_PROLOGUE();
  const uint64_t input5 = ctx.r5.u64;
  WriteFields<32, 0>(base, uint32_t(input5), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_825FB190) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 4>(base, uint32_t(input3), 0ull);
  WriteFields<32, 0>(base, uint32_t(input3), 0xFFFFFFFFFFFFFFFFull);
  WriteFields<32, 8>(base, uint32_t(input3), 0ull);
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_827871A8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 30672u);
  WriteFields<32, 0>(base, uint32_t(input4), field0);
  const uint64_t field1 = GH_LOAD_U32(uint32_t(input3) + 30676u);
  WriteFields<32, 0>(base, uint32_t(input5), field1);
  ctx.r10.u64 = field1;
  ctx.r11.u64 = field0;
}

GH_HOOK(sub_824A4AB0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 1726, 1727, 1728, 1729>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_826CF0D8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 8u);
  WriteFields<32, 0>(base, uint32_t(input4), field0);
  ctx.r3.u64 = 0ull;
  ctx.r11.u64 = field0;
}

GH_HOOK(sub_82703778) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 72>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_8242F430) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0>(base, uint32_t(input3), 0ull);
  WriteFields<8, 0>(base, uint32_t(input3), 0xFFFFFFFFFFFFFFFFull);
  WriteFields<8, 1, 2, 3>(base, uint32_t(input3), 0ull);
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8243F2A8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<16, 202, 204>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_82441E08) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 233>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82411A50) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 0>(base, uint32_t(input3), 1ull);
  WriteFields<32, 4>(base, uint32_t(input3), 6ull);
  WriteFields<32, 8, 12>(base, uint32_t(input3), 1ull);
  ctx.r10.u64 = 6ull;
  ctx.r11.u64 = 1ull;
}

GH_HOOK(sub_8247BAD8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 208>(base, uint32_t(input3), 3ull);
  ctx.r11.u64 = 3ull;
}

GH_HOOK(sub_82481800) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<16, 358>(base, uint32_t(input3), 1ull);
  WriteFields<8, 363>(base, uint32_t(input3), 1ull);
  ctx.r11.u64 = 1ull;
}

GH_HOOK(sub_826C9A90) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0, 4, 8>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8276E040) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 44, 52, 48, 112, 116, 120>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82774B20) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<64, 64>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_8260B3E0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t input6 = ctx.r6.u64;
  WriteFields<32, 1352>(base, uint32_t(input3), input4);
  WriteFields<32, 1344>(base, uint32_t(input3), input5);
  WriteFields<32, 1348>(base, uint32_t(input3), input6);
}

GH_HOOK(sub_826F4140) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 356>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_827073C8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U64(uint32_t(input3) + 288u);
  WriteFields<64, 0>(base, uint32_t(input4), field0);
  ctx.r3.u64 = 0ull;
  ctx.r11.u64 = field0;
}

GH_HOOK(sub_8243AA18) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 4u);
  WriteFields<32, 4>(base, uint32_t(input3), (field0 - 1ull));
  ctx.r11.u64 = (field0 - 1ull);
}

GH_HOOK(sub_824A6C30) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 12, 284, 160>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_824D5C98) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 12>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_827EF388) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 128, 124, 64, 96>(base, uint32_t(input3), 0ull);
  WriteFields<32, 76>(base, uint32_t(input3), 1ull);
  ctx.r10.u64 = 1ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_824A5960) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 244, 432, 628, 776, 956>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_825F74D0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 4>(base, uint32_t(input3), 0ull);
  WriteFields<32, 0>(base, uint32_t(input3), 0xFFFFFFFFFFFFFFFFull);
  WriteFields<32, 8, 56, 60, 76, 80>(base, uint32_t(input3), 0ull);
  ctx.r10.u64 = 0xFFFFFFFFFFFFFFFFull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82415238) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 84>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_8247BAB8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 208>(base, uint32_t(input3), 1ull);
  ctx.r11.u64 = 1ull;
}

GH_HOOK(sub_82496C40) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<16, 12>(base, uint32_t(input3), 0ull);
  WriteFields<8, 14>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82774B10) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<64, 56>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_824A5B90) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<16, 6>(base, uint32_t(input3), 3ull);
  ctx.r11.u64 = 3ull;
}

GH_HOOK(sub_826C9A70) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 0>(base, uint32_t(input3), input4);
  WriteFields<32, 4, 8>(base, uint32_t(input3), (input3 + 4ull));
  WriteFields<32, 12>(base, uint32_t(input3), 0ull);
  ctx.r10.u64 = 0ull;
  ctx.r11.u64 = (input3 + 4ull);
}

GH_HOOK(sub_826752F8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 264>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_82786AE0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 31532u);
  WriteFields<32, 31532>(base, uint32_t(input3), (field0 | 0x1ull));
  ctx.r10.u64 = (field0 | 0x1ull);
  ctx.r11.u64 = field0;
}

GH_HOOK(sub_8260F630) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 1376u);
  WriteFields<32, 1376>(base, uint32_t(input3), (field0 | 0x1000ull));
  ctx.r11.u64 = (field0 | 0x1000ull);
}

GH_HOOK(sub_82458500) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 600>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8246BDA0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 644>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_824A6D10) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 244, 432, 628, 776, 956, 1228, 1500, 1376>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_826F67B8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 36>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_82697098) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 28u);
  WriteFields<32, 0>(base, uint32_t(input4), field0);
  ctx.r3.u64 = 0ull;
  ctx.r11.u64 = field0;
}

GH_HOOK(sub_8243F2B8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<16, 210, 212>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_8247BAA8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 208>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_825FB320) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 4, 0>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82685F00) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 56, 40>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8241BAD0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0, 4, 8, 12, 16>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_825D8E20) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 13872>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_826DA0E0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  WriteFields<32, 23>(base, uint32_t(input3), input5);
  WriteFields<32, 19>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_82703600) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 56>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_82703610) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 64>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_82481810) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<16, 358>(base, uint32_t(input3), 4ull);
  WriteFields<8, 363>(base, uint32_t(input3), 1ull);
  ctx.r10.u64 = 1ull;
  ctx.r11.u64 = 4ull;
}

GH_HOOK(sub_826E08F8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<16, 0>(base, uint32_t(input3), input4);
  ctx.r3.u64 = 2ull;
}

GH_HOOK(sub_82779F40) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 31144>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_8276ACF0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 52>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_8275D990) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 248u);
  WriteFields<32, 284, 476, 4036>(base, uint32_t(input3), 0ull);
  WriteFields<32, 4044>(base, uint32_t(input3), field0);
  ctx.r10.u64 = field0;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8276AAA8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 12>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82707B08) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<64, 48>(base, uint32_t(input3), 0ull);
  WriteFields<32, 44>(base, uint32_t(input3), 1ull);
  WriteFields<32, 56, 60>(base, uint32_t(input3), 0ull);
  ctx.r10.u64 = 1ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_827868D0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 30636, 30616, 30620, 30624, 30628, 30632>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_828290B0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 7872>(base, uint32_t(input3), 1ull);
  ctx.r11.u64 = 1ull;
}

GH_HOOK(sub_82770198) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 444>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_827D30F8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 14552>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82418CF8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t input6 = ctx.r6.u64;
  WriteFields<16, 0>(base, uint32_t(input3), input4);
  WriteFields<16, 2>(base, uint32_t(input3), input5);
  WriteFields<16, 4>(base, uint32_t(input3), input6);
}

GH_HOOK(sub_826DA050) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 15>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_826A8678) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t input6 = ctx.r6.u64;
  const uint64_t input7 = ctx.r7.u64;
  WriteFields<32, 0>(base, uint32_t(input3), input4);
  WriteFields<32, 4>(base, uint32_t(input3), input5);
  WriteFields<32, 8>(base, uint32_t(input3), input6);
  WriteFields<32, 12>(base, uint32_t(input3), 0ull);
  WriteFields<32, 16>(base, uint32_t(input3), input7);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_826E08A8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<16, 7>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_8246B050) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 233, 382>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8246B7B8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 1600, 1668>(base, uint32_t(input3), 8ull);
  ctx.r11.u64 = 8ull;
}

GH_HOOK(sub_826B7F00) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 88>(base, uint32_t(input3), input4);
  WriteFields<32, 84>(base, uint32_t(input4), input3);
}

GH_HOOK(sub_826DDB68) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 200u);
  WriteFields<32, 200>(base, uint32_t(input3), (field0 + 1ull));
  ctx.r11.u64 = (field0 + 1ull);
}

GH_HOOK(sub_826F40C8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 368>(base, uint32_t(input3), 1ull);
  ctx.r11.u64 = 1ull;
}

GH_HOOK(sub_8247BB58) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 356>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_825E1638) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 13716u);
  WriteFields<32, 48>(base, uint32_t(input3), field0);
  ctx.r11.u64 = field0;
}

GH_HOOK(sub_82627A48) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 0, 4, 8, 12, 16>(base, uint32_t(input3), 0ull);
  WriteFields<32, 16>(base, uint32_t(input3), input4);
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 0u);
  WriteFields<32, 0>(base, uint32_t(input3), (field0 | 0x80000000ull));
  WriteFields<32, 4>(base, uint32_t(input4), input3);
  ctx.r11.u64 = (field0 | 0x80000000ull);
}

GH_HOOK(sub_824A6C20) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 8, 280, 156>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_824BD338) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 1664>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_825D7F98) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<8, 10499>(base, uint32_t(input3), input4);
  const uint64_t field0 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field0 | 0x10000000ull));
  ctx.r11.u64 = (field0 | 0x10000000ull);
}

GH_HOOK(sub_826DAC98) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 16>(base, uint32_t(input3), input3);
  ctx.r3.u64 = 0ull;
}

GH_HOOK(sub_826F23B8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 16>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8247BAC8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 208>(base, uint32_t(input3), 2ull);
  ctx.r11.u64 = 2ull;
}

GH_HOOK(sub_8287A0F0) {
  GH_PROLOGUE();
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 0>(base, uint32_t(input4), 0ull);
  ctx.r3.u64 = 0ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_824124E0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0, 4>(base, uint32_t(input3), 0ull);
  WriteFields<8, 8>(base, uint32_t(input3), 1ull);
  ctx.r10.u64 = 1ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82415240) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 88>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_8243EC48) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 0>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8276ACC8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 44, 48, 52>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_826CF5B0) {
  GH_PROLOGUE();
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 4>(base, uint32_t(input4), 0ull);
  WriteFields<32, 0>(base, uint32_t(input4), 15ull);
  WriteFields<32, 8>(base, uint32_t(input4), 0ull);
  ctx.r10.u64 = 15ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8276AF08) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 512>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_825D8378) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 12188>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_825D8E00) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<8, 10562>(base, uint32_t(input3), input4);
  const uint64_t field0 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field0 | 0x100ull));
  ctx.r11.u64 = (field0 | 0x100ull);
}

GH_HOOK(sub_82776AE8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  WriteFields<32, 30624>(base, uint32_t(input3), input4);
  WriteFields<32, 30628>(base, uint32_t(input3), input5);
}

GH_HOOK(sub_824BD358) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 1665>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8263D298) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 4>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_826746D8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 152u);
  WriteFields<32, 152>(base, uint32_t(input3), (field0 + 1ull));
  ctx.r11.u64 = (field0 + 1ull);
}

GH_HOOK(sub_82703608) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 60>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_82703780) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 76>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_825E3B50) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 13820u);
  WriteFields<32, 0>(base, uint32_t(input5), field0);
  const uint64_t field1 = GH_LOAD_U32(uint32_t(input3) + 13824u);
  WriteFields<32, 4>(base, uint32_t(input5), field1);
  const uint64_t field2 = GH_LOAD_U32(uint32_t(input3) + 21836u);
  WriteFields<32, 8>(base, uint32_t(input5), field2);
  const uint64_t field3 = GH_LOAD_U32(uint32_t(input3) + 13828u);
  WriteFields<32, 12>(base, uint32_t(input5), field3);
  ctx.r11.u64 = field3;
}

GH_HOOK(sub_82449FE8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 920, 392>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_824A5BC0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<16, 6>(base, uint32_t(input3), 5ull);
  ctx.r11.u64 = 5ull;
}

GH_HOOK(sub_824A4610) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<8, 1188>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_824A5BD0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 36, 184>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_824A6080) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 76, 264, 412>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_8260F3B8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 1376u);
  WriteFields<32, 1376>(base, uint32_t(input3), (field0 | 0x1ull));
  ctx.r11.u64 = (field0 | 0x1ull);
}

GH_HOOK(sub_826746E8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 116>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82674768) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 116>(base, uint32_t(input3), 1ull);
  ctx.r11.u64 = 1ull;
}

GH_HOOK(sub_825D7FD8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<8, 10497>(base, uint32_t(input3), input4);
  const uint64_t field0 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field0 | 0x10000000ull));
  ctx.r11.u64 = (field0 | 0x10000000ull);
}

GH_HOOK(sub_82449540) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 568>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_826B5720) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0, 4>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_826B7E48) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 80>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_827749F0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 44, 48, 52>(base, uint32_t(input3), 0ull);
  WriteFields<64, 56, 64>(base, uint32_t(input3), 0ull);
  WriteFields<32, 92>(base, uint32_t(input3), 1ull);
  WriteFields<64, 72, 80>(base, uint32_t(input3), 0ull);
  WriteFields<32, 88>(base, uint32_t(input3), 0ull);
  ctx.r10.u64 = 1ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82735528) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0, 4, 16, 20, 32, 36, 48, 52>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8276DF18) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 284>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_82642238) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 52>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8276AC90) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 48, 52, 44>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8276ACE0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 44>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_8276ACE8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 48>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_8241E6A8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<8, 80>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_826B8BD0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t input6 = ctx.r6.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 60u);
  WriteFields<32, 20>(base, uint32_t(input3), input4);
  WriteFields<32, 24>(base, uint32_t(input3), input5);
  WriteFields<32, 28>(base, uint32_t(input3), input6);
  WriteFields<32, 60>(base, uint32_t(input3), (field0 | 0x20000000ull));
  ctx.r11.u64 = (field0 | 0x20000000ull);
}

GH_HOOK(sub_828333E8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0, 4, 12, 16>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_824119A0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0, 4>(base, uint32_t(input3), 1ull);
  WriteFields<32, 8>(base, uint32_t(input3), 2ull);
  ctx.r10.u64 = 2ull;
  ctx.r11.u64 = 1ull;
}

GH_HOOK(sub_824119B8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_82422C00) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0, 4, 8, 12, 16, 20, 24>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_824A4938) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 1726>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_824BD348) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 1664>(base, uint32_t(input3), 1ull);
  ctx.r11.u64 = 1ull;
}

GH_HOOK(sub_824C01D8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 1672>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8267EF90) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 96>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8281D530) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t input6 = ctx.r6.u64;
  WriteFields<32, 20>(base, uint32_t(input3), input5);
  WriteFields<32, 0, 8>(base, uint32_t(input3), input4);
  WriteFields<32, 44>(base, uint32_t(input3), input6);
  WriteFields<32, 48, 56>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_824119D8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0>(base, uint32_t(input3), 1ull);
  ctx.r11.u64 = 1ull;
}

GH_HOOK(sub_82703618) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 68>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_8261D1A0) {
  GH_PROLOGUE();
  const uint64_t input6 = ctx.r6.u64;
  WriteFields<32, 0>(base, uint32_t(input6), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_824119C8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0, 4>(base, uint32_t(input3), 1ull);
  ctx.r11.u64 = 1ull;
}

GH_HOOK(sub_8261D198) {
  GH_PROLOGUE();
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t input6 = ctx.r6.u64;
  WriteFields<32, 0>(base, uint32_t(input6), input5);
}

GH_HOOK(sub_82804948) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U8(uint32_t(input3) + 16u);
  WriteFields<8, 0>(base, uint32_t(input4), field0);
  ctx.r3.u64 = 0ull;
  ctx.r11.u64 = field0;
}

GH_HOOK(sub_824A4930) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<8, 440>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_825D7FF8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<8, 10495>(base, uint32_t(input3), input4);
  const uint64_t field0 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field0 | 0x20000000ull));
  ctx.r11.u64 = (field0 | 0x20000000ull);
}

GH_HOOK(sub_826E08E8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 0>(base, uint32_t(input3), input4);
  ctx.r3.u64 = 4ull;
}

GH_HOOK(sub_82411A30) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<8, 0>(base, uint32_t(input3), 1ull);
  WriteFields<32, 4>(base, uint32_t(input3), 6ull);
  WriteFields<32, 8>(base, uint32_t(input3), 1ull);
  WriteFields<32, 12>(base, uint32_t(input3), 0ull);
  ctx.r9.u64 = 0ull;
  ctx.r10.u64 = 6ull;
  ctx.r11.u64 = 1ull;
}

GH_HOOK(sub_824BBDB0) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 1744>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_825D8038) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<8, 10493>(base, uint32_t(input3), input4);
  const uint64_t field0 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field0 | 0x20000000ull));
  ctx.r11.u64 = (field0 | 0x20000000ull);
}

GH_HOOK(sub_82638770) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 464>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_826F4148) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 352>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_827701A8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 448>(base, uint32_t(input3), input4);
}

GH_HOOK(sub_827EF458) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 60, 68, 0, 72, 76>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_824119F8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0>(base, uint32_t(input3), 2ull);
  ctx.r11.u64 = 2ull;
}

GH_HOOK(sub_825D8018) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<8, 10494>(base, uint32_t(input3), input4);
  const uint64_t field0 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field0 | 0x20000000ull));
  ctx.r11.u64 = (field0 | 0x20000000ull);
}

GH_HOOK(sub_82697818) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U16(uint32_t(input3) + 8u);
  WriteFields<16, 0>(base, uint32_t(input4), field0);
  ctx.r3.u64 = 0ull;
  ctx.r11.u64 = field0;
}

GH_HOOK(sub_82411988) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0, 4>(base, uint32_t(input3), 0ull);
  WriteFields<32, 8>(base, uint32_t(input3), 2ull);
  ctx.r10.u64 = 2ull;
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8269A468) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 32>(base, uint32_t(input3), input4);
  ctx.r3.u64 = 0ull;
}

GH_HOOK(sub_82703A40) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t field0 = GH_LOAD_U32(uint32_t(input3) + 56u);
  WriteFields<32, 0>(base, uint32_t(input4), field0);
  ctx.r3.u64 = 0ull;
  ctx.r11.u64 = field0;
}

GH_HOOK(sub_825D7FB8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<8, 10498>(base, uint32_t(input3), input4);
  const uint64_t field0 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field0 | 0x10000000ull));
  ctx.r11.u64 = (field0 | 0x10000000ull);
}

GH_HOOK(sub_825DD830) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  WriteFields<32, 12120>(base, uint32_t(input3), input4);
  const uint64_t field0 = GH_LOAD_U64(uint32_t(input3) + 16u);
  WriteFields<64, 16>(base, uint32_t(input3), (field0 | 0x80000ull));
  ctx.r11.u64 = (field0 | 0x80000ull);
}

GH_HOOK(sub_824119E8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 0, 4>(base, uint32_t(input3), 2ull);
  ctx.r11.u64 = 2ull;
}

GH_HOOK(sub_8267EF80) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 96>(base, uint32_t(input3), 1ull);
  ctx.r11.u64 = 1ull;
}

GH_HOOK(sub_82770670) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  WriteFields<32, 56, 60, 64, 68>(base, uint32_t(input3), 0ull);
  ctx.r11.u64 = 0ull;
}

GH_HOOK(sub_8242A2D8) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t input4 = ctx.r4.u64;
  const uint64_t input5 = ctx.r5.u64;
  const uint64_t input6 = ctx.r6.u64;
  const uint64_t input7 = ctx.r7.u64;
  WriteFields<8, 1>(base, uint32_t(input3), input4);
  WriteFields<8, 2>(base, uint32_t(input3), input5);
  WriteFields<8, 3>(base, uint32_t(input3), input6);
  WriteFields<8, 0>(base, uint32_t(input3), input7);
}

GH_HOOK(sub_825EA330) {
  GH_PROLOGUE();
  const uint64_t input3 = ctx.r3.u64;
  const uint64_t field0 = GH_LOAD_U8(uint32_t(input3) + 11071u);
  WriteFields<8, 11071>(base, uint32_t(input3), (field0 | 0x4ull));
  ctx.r11.u64 = (field0 | 0x4ull);
}
