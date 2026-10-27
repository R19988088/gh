#include "native_runtime.h"

namespace {
template <unsigned Bits>
uint64_t LoadField(uint8_t* base, uint32_t address) {
  if constexpr (Bits == 8) return GH_LOAD_U8(address);
  if constexpr (Bits == 16) return GH_LOAD_U16(address);
  if constexpr (Bits == 32) return GH_LOAD_U32(address);
  if constexpr (Bits == 64) return GH_LOAD_U64(address);
}

template <unsigned Bits, uint32_t Offset>
void FieldValue(GuestContext& ctx, uint8_t* base) {
  ctx.r3.u64 = LoadField<Bits>(base, ctx.r3.u32 + Offset);
}

template <unsigned Bits, int64_t Page, int32_t Offset>
void GlobalValue(GuestContext& ctx, uint8_t* base) {
  constexpr uint32_t address = uint32_t(Page + Offset);
  ctx.r11.s64 = Page;
  ctx.r3.u64 = LoadField<Bits>(base, address);
}

template <unsigned Bits, int64_t Page, int32_t BaseOffset, int32_t Offset>
void GlobalBaseValue(GuestContext& ctx, uint8_t* base) {
  constexpr int64_t object = Page + BaseOffset;
  ctx.r11.s64 = Page;
  ctx.r10.s64 = object;
  ctx.r3.u64 = LoadField<Bits>(base, uint32_t(object + Offset));
}

template <unsigned Register, uint32_t PointerOffset, unsigned Bits, uint32_t FieldOffset>
void IndirectValue(GuestContext& ctx, uint8_t* base) {
  const uint32_t object = Register == 13 ? ctx.r13.u32 : ctx.r3.u32;
  const uint32_t pointer = LoadField<32>(base, object + PointerOffset);
  ctx.r11.u64 = pointer;
  ctx.r3.u64 = LoadField<Bits>(base, pointer + FieldOffset);
}

template <uint32_t PointerOffset, int64_t Offset>
void SubobjectAddress(GuestContext& ctx, uint8_t* base) {
  const uint32_t pointer = LoadField<32>(base, ctx.r3.u32 + PointerOffset);
  ctx.r11.u64 = pointer;
  ctx.r3.u64 = uint64_t(pointer) + Offset;
}

template <uint32_t PointerOffset, unsigned Bits, uint32_t Offset>
void LargeOffsetValue(GuestContext& ctx, uint8_t* base) {
  const uint32_t pointer = LoadField<32>(base, ctx.r3.u32 + PointerOffset);
  ctx.r10.u64 = Offset;
  ctx.r11.u64 = pointer;
  ctx.r3.u64 = LoadField<Bits>(base, pointer + Offset);
}
}  // namespace

GH_HOOK(sub_824E21D8) {
  GH_PROLOGUE();
  LargeOffsetValue<4, 32, 94620>(ctx, base);
}

GH_HOOK(sub_824E21F0) {
  GH_PROLOGUE();
  LargeOffsetValue<4, 32, 94624>(ctx, base);
}

GH_HOOK(sub_824E2208) {
  GH_PROLOGUE();
  LargeOffsetValue<4, 32, 94628>(ctx, base);
}

GH_HOOK(sub_824E2220) {
  GH_PROLOGUE();
  LargeOffsetValue<4, 32, 94632>(ctx, base);
}

GH_HOOK(sub_824E2238) {
  GH_PROLOGUE();
  LargeOffsetValue<4, 32, 94636>(ctx, base);
}

GH_HOOK(sub_824118E8) {
  GH_PROLOGUE();
  GlobalBaseValue<8, -2103967744, -16600, 67>(ctx, base);
}

GH_HOOK(sub_82411908) {
  GH_PROLOGUE();
  GlobalBaseValue<8, -2103967744, -16600, 64>(ctx, base);
}

GH_HOOK(sub_82411928) {
  GH_PROLOGUE();
  GlobalBaseValue<8, -2103967744, -16600, 65>(ctx, base);
}

GH_HOOK(sub_82411948) {
  GH_PROLOGUE();
  GlobalBaseValue<8, -2103967744, -16600, 66>(ctx, base);
}

GH_HOOK(sub_82415690) {
  GH_PROLOGUE();
  GlobalBaseValue<8, -2103967744, -5048, 66>(ctx, base);
}

GH_HOOK(sub_825CED98) {
  GH_PROLOGUE();
  IndirectValue<13, 256, 32, 332>(ctx, base);
}

GH_HOOK(sub_826E0960) {
  GH_PROLOGUE();
  IndirectValue<3, 0, 16, 7>(ctx, base);
}

GH_HOOK(sub_82709A28) {
  GH_PROLOGUE();
  IndirectValue<3, 12, 32, 2792>(ctx, base);
}

GH_HOOK(sub_82709CC0) {
  GH_PROLOGUE();
  IndirectValue<3, 12, 32, 31104>(ctx, base);
}

GH_HOOK(sub_824E21C8) {
  GH_PROLOGUE();
  SubobjectAddress<4, 94376>(ctx, base);
}

GH_HOOK(sub_824E2250) {
  GH_PROLOGUE();
  SubobjectAddress<4, 86404>(ctx, base);
}

GH_HOOK(sub_824E2260) {
  GH_PROLOGUE();
  SubobjectAddress<4, 90644>(ctx, base);
}

GH_HOOK(sub_824100E0) {
  GH_PROLOGUE();
  GlobalValue<8, -2103967744, -20076>(ctx, base);
}

GH_HOOK(sub_82410150) {
  GH_PROLOGUE();
  GlobalValue<8, -2103967744, -20087>(ctx, base);
}

GH_HOOK(sub_82410170) {
  GH_PROLOGUE();
  GlobalValue<8, -2103967744, -20088>(ctx, base);
}

GH_HOOK(sub_82411888) {
  GH_PROLOGUE();
  GlobalValue<32, -2103967744, -16948>(ctx, base);
}

GH_HOOK(sub_824118D8) {
  GH_PROLOGUE();
  GlobalValue<32, -2103967744, -16932>(ctx, base);
}

GH_HOOK(sub_82419D38) {
  GH_PROLOGUE();
  GlobalValue<8, -2103967744, -4460>(ctx, base);
}

GH_HOOK(sub_82419D48) {
  GH_PROLOGUE();
  GlobalValue<8, -2103967744, -4459>(ctx, base);
}

GH_HOOK(sub_82419D70) {
  GH_PROLOGUE();
  GlobalValue<8, -2103902208, 4354>(ctx, base);
}

GH_HOOK(sub_82419DA0) {
  GH_PROLOGUE();
  GlobalValue<8, -2103902208, 4355>(ctx, base);
}

GH_HOOK(sub_82423110) {
  GH_PROLOGUE();
  GlobalValue<8, -2103902208, 5432>(ctx, base);
}

GH_HOOK(sub_82429860) {
  GH_PROLOGUE();
  GlobalValue<32, -2103902208, 7056>(ctx, base);
}

GH_HOOK(sub_82429960) {
  GH_PROLOGUE();
  GlobalValue<32, -2103902208, 7036>(ctx, base);
}

GH_HOOK(sub_8242B430) {
  GH_PROLOGUE();
  GlobalValue<32, -2103902208, 9704>(ctx, base);
}

GH_HOOK(sub_8242E888) {
  GH_PROLOGUE();
  GlobalValue<32, -2103836672, 2140>(ctx, base);
}

GH_HOOK(sub_8242E898) {
  GH_PROLOGUE();
  GlobalValue<32, -2103836672, 2136>(ctx, base);
}

GH_HOOK(sub_8242E8C8) {
  GH_PROLOGUE();
  GlobalValue<32, -2103836672, 2120>(ctx, base);
}

GH_HOOK(sub_8242EA80) {
  GH_PROLOGUE();
  GlobalValue<8, -2103836672, 2012>(ctx, base);
}

GH_HOOK(sub_8242F3F0) {
  GH_PROLOGUE();
  GlobalValue<8, -2103836672, 6481>(ctx, base);
}

GH_HOOK(sub_8242F410) {
  GH_PROLOGUE();
  GlobalValue<8, -2103836672, 6482>(ctx, base);
}

GH_HOOK(sub_824348F8) {
  GH_PROLOGUE();
  GlobalValue<32, -2103836672, 8720>(ctx, base);
}

GH_HOOK(sub_82434A38) {
  GH_PROLOGUE();
  GlobalValue<32, -2104426496, 7776>(ctx, base);
}

GH_HOOK(sub_82434AA0) {
  GH_PROLOGUE();
  GlobalValue<64, -2103836672, 8704>(ctx, base);
}

GH_HOOK(sub_82434AB0) {
  GH_PROLOGUE();
  GlobalValue<8, -2103836672, 8682>(ctx, base);
}

GH_HOOK(sub_82435BF0) {
  GH_PROLOGUE();
  GlobalValue<8, -2103771136, -20913>(ctx, base);
}

GH_HOOK(sub_82435C00) {
  GH_PROLOGUE();
  GlobalValue<8, -2103771136, -20888>(ctx, base);
}

GH_HOOK(sub_82435C30) {
  GH_PROLOGUE();
  GlobalValue<8, -2103771136, -21256>(ctx, base);
}

GH_HOOK(sub_8243EB40) {
  GH_PROLOGUE();
  GlobalValue<32, -2103771136, -19544>(ctx, base);
}

GH_HOOK(sub_8243EC28) {
  GH_PROLOGUE();
  GlobalValue<32, -2103771136, -19540>(ctx, base);
}

GH_HOOK(sub_824A44F8) {
  GH_PROLOGUE();
  GlobalValue<32, -2103640064, -13920>(ctx, base);
}

GH_HOOK(sub_825D2C00) {
  GH_PROLOGUE();
  GlobalValue<32, -2113929216, 1624>(ctx, base);
}

GH_HOOK(sub_825D6230) {
  GH_PROLOGUE();
  GlobalValue<32, -2103443456, 26448>(ctx, base);
}

GH_HOOK(sub_828960B0) {
  GH_PROLOGUE();
  GlobalValue<32, -2103443456, 26324>(ctx, base);
}

GH_HOOK(sub_82418EC8) {
  GH_PROLOGUE();
  FieldValue<8, 124>(ctx, base);
}

GH_HOOK(sub_8243F878) {
  GH_PROLOGUE();
  FieldValue<32, 168>(ctx, base);
}

GH_HOOK(sub_8246BC68) {
  GH_PROLOGUE();
  FieldValue<32, 1168>(ctx, base);
}

GH_HOOK(sub_82496C80) {
  GH_PROLOGUE();
  FieldValue<8, 56>(ctx, base);
}

GH_HOOK(sub_825D7D10) {
  GH_PROLOGUE();
  FieldValue<32, 12180>(ctx, base);
}

GH_HOOK(sub_825D7DB8) {
  GH_PROLOGUE();
  FieldValue<32, 12184>(ctx, base);
}

GH_HOOK(sub_825D82B0) {
  GH_PROLOGUE();
  FieldValue<32, 12164>(ctx, base);
}

GH_HOOK(sub_8262BEE0) {
  GH_PROLOGUE();
  FieldValue<8, 2144>(ctx, base);
}

GH_HOOK(sub_82636C38) {
  GH_PROLOGUE();
  FieldValue<32, 204>(ctx, base);
}

GH_HOOK(sub_8264F520) {
  GH_PROLOGUE();
  FieldValue<32, 24>(ctx, base);
}

GH_HOOK(sub_826519C0) {
  GH_PROLOGUE();
  FieldValue<32, 32>(ctx, base);
}

GH_HOOK(sub_8267EFA0) {
  GH_PROLOGUE();
  FieldValue<32, 96>(ctx, base);
}

GH_HOOK(sub_8268D140) {
  GH_PROLOGUE();
  FieldValue<32, 28>(ctx, base);
}

GH_HOOK(sub_8268D1C8) {
  GH_PROLOGUE();
  FieldValue<16, 4>(ctx, base);
}

GH_HOOK(sub_826B70F8) {
  GH_PROLOGUE();
  FieldValue<32, 20>(ctx, base);
}

GH_HOOK(sub_826F15A0) {
  GH_PROLOGUE();
  FieldValue<32, 68>(ctx, base);
}

GH_HOOK(sub_826F67B0) {
  GH_PROLOGUE();
  FieldValue<32, 56>(ctx, base);
}

GH_HOOK(sub_82703628) {
  GH_PROLOGUE();
  FieldValue<32, 60>(ctx, base);
}

GH_HOOK(sub_82703630) {
  GH_PROLOGUE();
  FieldValue<32, 64>(ctx, base);
}

GH_HOOK(sub_82703790) {
  GH_PROLOGUE();
  FieldValue<32, 72>(ctx, base);
}

GH_HOOK(sub_8246BC58) {
  GH_PROLOGUE();
  ctx.r3.u64 += 1168;
}

GH_HOOK(sub_8246BC60) {
  GH_PROLOGUE();
  ctx.r3.u64 += 1172;
}

GH_HOOK(sub_8268D190) {
  GH_PROLOGUE();
  ctx.r3.u64 += 16;
}

GH_HOOK(sub_8263D2D0) {
  GH_PROLOGUE();
  IndirectValue<3, 48, 32, 184>(ctx, base);
}

GH_HOOK(sub_8268A8E8) {
  GH_PROLOGUE();
  IndirectValue<3, 292, 32, 0>(ctx, base);
}

GH_HOOK(sub_8268AAB8) {
  GH_PROLOGUE();
  IndirectValue<3, 288, 32, 4>(ctx, base);
}

GH_HOOK(sub_8268AAC8) {
  GH_PROLOGUE();
  IndirectValue<3, 288, 32, 8>(ctx, base);
}

GH_HOOK(sub_8268AAD8) {
  GH_PROLOGUE();
  IndirectValue<3, 288, 32, 0>(ctx, base);
}

GH_HOOK(sub_8268AAE8) {
  GH_PROLOGUE();
  IndirectValue<3, 292, 32, 4>(ctx, base);
}

GH_HOOK(sub_8268AD30) {
  GH_PROLOGUE();
  IndirectValue<3, 324, 32, 4>(ctx, base);
}

GH_HOOK(sub_8268AD40) {
  GH_PROLOGUE();
  IndirectValue<3, 324, 32, 8>(ctx, base);
}

GH_HOOK(sub_8268AD50) {
  GH_PROLOGUE();
  IndirectValue<3, 324, 32, 0>(ctx, base);
}

GH_HOOK(sub_8268AD60) {
  GH_PROLOGUE();
  IndirectValue<3, 328, 32, 4>(ctx, base);
}

GH_HOOK(sub_8268ADF8) {
  GH_PROLOGUE();
  IndirectValue<3, 328, 32, 80>(ctx, base);
}

GH_HOOK(sub_8268AE08) {
  GH_PROLOGUE();
  IndirectValue<3, 328, 32, 0>(ctx, base);
}

GH_HOOK(sub_826945F0) {
  GH_PROLOGUE();
  IndirectValue<3, 32, 16, 10>(ctx, base);
}

GH_HOOK(sub_82694658) {
  GH_PROLOGUE();
  IndirectValue<3, 32, 8, 9>(ctx, base);
}

GH_HOOK(sub_826949A8) {
  GH_PROLOGUE();
  IndirectValue<3, 32, 16, 12>(ctx, base);
}

GH_HOOK(sub_82687C00) {
  GH_PROLOGUE();
  SubobjectAddress<52, 9>(ctx, base);
}

GH_HOOK(sub_8268A8F8) {
  GH_PROLOGUE();
  SubobjectAddress<292, 88>(ctx, base);
}

GH_HOOK(sub_8268AAA8) {
  GH_PROLOGUE();
  SubobjectAddress<292, 8>(ctx, base);
}

GH_HOOK(sub_8268AD20) {
  GH_PROLOGUE();
  SubobjectAddress<328, 8>(ctx, base);
}

GH_HOOK(sub_8268ADE8) {
  GH_PROLOGUE();
  SubobjectAddress<328, 88>(ctx, base);
}

GH_HOOK(sub_82694648) {
  GH_PROLOGUE();
  SubobjectAddress<32, 10>(ctx, base);
}

GH_HOOK(sub_82694A00) {
  GH_PROLOGUE();
  SubobjectAddress<32, 12>(ctx, base);
}

GH_HOOK(sub_8256D9C8) {
  GH_PROLOGUE();
  GlobalValue<32, -2110652416, 32468>(ctx, base);
}

GH_HOOK(sub_82570C18) {
  GH_PROLOGUE();
  GlobalValue<32, -2110586880, -31908>(ctx, base);
}

GH_HOOK(sub_8257A0A0) {
  GH_PROLOGUE();
  GlobalValue<32, -2110586880, -25236>(ctx, base);
}

GH_HOOK(sub_8257A3C8) {
  GH_PROLOGUE();
  GlobalValue<32, -2110586880, -20192>(ctx, base);
}

GH_HOOK(sub_8257B830) {
  GH_PROLOGUE();
  GlobalValue<32, -2110586880, -19684>(ctx, base);
}

GH_HOOK(sub_82580430) {
  GH_PROLOGUE();
  GlobalValue<32, -2110586880, -14368>(ctx, base);
}

GH_HOOK(sub_82580C08) {
  GH_PROLOGUE();
  GlobalValue<32, -2110586880, -13020>(ctx, base);
}

GH_HOOK(sub_82583B10) {
  GH_PROLOGUE();
  GlobalValue<32, -2110586880, -7940>(ctx, base);
}

GH_HOOK(sub_82585078) {
  GH_PROLOGUE();
  GlobalValue<32, -2110586880, -3608>(ctx, base);
}

GH_HOOK(sub_8258C788) {
  GH_PROLOGUE();
  GlobalValue<32, -2110586880, 28>(ctx, base);
}

GH_HOOK(sub_8258E7C0) {
  GH_PROLOGUE();
  GlobalValue<32, -2110586880, 3192>(ctx, base);
}

GH_HOOK(sub_82590AB0) {
  GH_PROLOGUE();
  GlobalValue<32, -2110586880, 3928>(ctx, base);
}

GH_HOOK(sub_82592838) {
  GH_PROLOGUE();
  GlobalValue<32, -2110586880, 5340>(ctx, base);
}

GH_HOOK(sub_825A2AA8) {
  GH_PROLOGUE();
  GlobalValue<32, -2110586880, 17584>(ctx, base);
}

GH_HOOK(sub_825A9B40) {
  GH_PROLOGUE();
  GlobalValue<32, -2110586880, 26732>(ctx, base);
}

GH_HOOK(sub_825AA6B8) {
  GH_PROLOGUE();
  GlobalValue<32, -2110586880, 29168>(ctx, base);
}

GH_HOOK(sub_825ABC40) {
  GH_PROLOGUE();
  GlobalValue<32, -2110586880, 29756>(ctx, base);
}

GH_HOOK(sub_825ACA58) {
  GH_PROLOGUE();
  GlobalValue<32, -2110586880, 31516>(ctx, base);
}

GH_HOOK(sub_825AE310) {
  GH_PROLOGUE();
  GlobalValue<32, -2110586880, 32076>(ctx, base);
}

GH_HOOK(sub_825AFA80) {
  GH_PROLOGUE();
  GlobalValue<32, -2110521344, -31236>(ctx, base);
}

GH_HOOK(sub_825AFB78) {
  GH_PROLOGUE();
  GlobalValue<32, -2110521344, -29836>(ctx, base);
}

GH_HOOK(sub_825B2470) {
  GH_PROLOGUE();
  GlobalValue<32, -2110521344, -23604>(ctx, base);
}

GH_HOOK(sub_825B4AE0) {
  GH_PROLOGUE();
  GlobalValue<32, -2110521344, -15196>(ctx, base);
}

GH_HOOK(sub_825B4D38) {
  GH_PROLOGUE();
  GlobalValue<32, -2110521344, -14420>(ctx, base);
}

GH_HOOK(sub_825B66F8) {
  GH_PROLOGUE();
  GlobalValue<32, -2110521344, -13316>(ctx, base);
}

GH_HOOK(sub_825B67F0) {
  GH_PROLOGUE();
  GlobalValue<32, -2110521344, -12540>(ctx, base);
}

GH_HOOK(sub_825B9500) {
  GH_PROLOGUE();
  GlobalValue<32, -2110521344, -10844>(ctx, base);
}

GH_HOOK(sub_825BA910) {
  GH_PROLOGUE();
  GlobalValue<32, -2110521344, -8260>(ctx, base);
}

GH_HOOK(sub_825BD378) {
  GH_PROLOGUE();
  GlobalValue<32, -2110521344, -6500>(ctx, base);
}

GH_HOOK(sub_825BE198) {
  GH_PROLOGUE();
  GlobalValue<32, -2110521344, -3892>(ctx, base);
}

GH_HOOK(sub_825C0708) {
  GH_PROLOGUE();
  GlobalValue<32, -2110521344, -2660>(ctx, base);
}

GH_HOOK(sub_825C3080) {
  GH_PROLOGUE();
  GlobalValue<32, -2110521344, -1644>(ctx, base);
}

GH_HOOK(sub_825C4710) {
  GH_PROLOGUE();
  GlobalValue<32, -2110521344, -372>(ctx, base);
}

GH_HOOK(sub_825C60E0) {
  GH_PROLOGUE();
  GlobalValue<32, -2110521344, 908>(ctx, base);
}

GH_HOOK(sub_824259F8) {
  GH_PROLOGUE();
  FieldValue<8, 452>(ctx, base);
}

GH_HOOK(sub_82427400) {
  GH_PROLOGUE();
  FieldValue<8, 360>(ctx, base);
}

GH_HOOK(sub_82449528) {
  GH_PROLOGUE();
  FieldValue<8, 564>(ctx, base);
}

GH_HOOK(sub_82449530) {
  GH_PROLOGUE();
  FieldValue<8, 565>(ctx, base);
}

GH_HOOK(sub_82449538) {
  GH_PROLOGUE();
  FieldValue<8, 569>(ctx, base);
}

GH_HOOK(sub_824584D8) {
  GH_PROLOGUE();
  FieldValue<32, 556>(ctx, base);
}

GH_HOOK(sub_824584E0) {
  GH_PROLOGUE();
  FieldValue<8, 596>(ctx, base);
}

GH_HOOK(sub_824584E8) {
  GH_PROLOGUE();
  FieldValue<8, 597>(ctx, base);
}

GH_HOOK(sub_824584F0) {
  GH_PROLOGUE();
  FieldValue<32, 560>(ctx, base);
}

GH_HOOK(sub_824584F8) {
  GH_PROLOGUE();
  FieldValue<8, 601>(ctx, base);
}

GH_HOOK(sub_82458510) {
  GH_PROLOGUE();
  FieldValue<32, 564>(ctx, base);
}

GH_HOOK(sub_8246BD60) {
  GH_PROLOGUE();
  FieldValue<32, 600>(ctx, base);
}

GH_HOOK(sub_8246BD68) {
  GH_PROLOGUE();
  FieldValue<8, 643>(ctx, base);
}

GH_HOOK(sub_8246BD70) {
  GH_PROLOGUE();
  FieldValue<8, 640>(ctx, base);
}

GH_HOOK(sub_8246BD78) {
  GH_PROLOGUE();
  FieldValue<8, 641>(ctx, base);
}

GH_HOOK(sub_8246BD80) {
  GH_PROLOGUE();
  FieldValue<32, 604>(ctx, base);
}

GH_HOOK(sub_8246BD88) {
  GH_PROLOGUE();
  FieldValue<8, 645>(ctx, base);
}

GH_HOOK(sub_8246BD90) {
  GH_PROLOGUE();
  FieldValue<32, 648>(ctx, base);
}

GH_HOOK(sub_8246BD98) {
  GH_PROLOGUE();
  FieldValue<32, 652>(ctx, base);
}

GH_HOOK(sub_82481FD0) {
  GH_PROLOGUE();
  FieldValue<32, 312>(ctx, base);
}

GH_HOOK(sub_82482210) {
  GH_PROLOGUE();
  FieldValue<32, 464>(ctx, base);
}

GH_HOOK(sub_82482378) {
  GH_PROLOGUE();
  FieldValue<32, 584>(ctx, base);
}

GH_HOOK(sub_82482578) {
  GH_PROLOGUE();
  FieldValue<32, 356>(ctx, base);
}

GH_HOOK(sub_82483430) {
  GH_PROLOGUE();
  FieldValue<32, 212>(ctx, base);
}

GH_HOOK(sub_82483698) {
  GH_PROLOGUE();
  FieldValue<32, 208>(ctx, base);
}

GH_HOOK(sub_824838C0) {
  GH_PROLOGUE();
  FieldValue<32, 228>(ctx, base);
}

GH_HOOK(sub_82485180) {
  GH_PROLOGUE();
  FieldValue<32, 304>(ctx, base);
}

GH_HOOK(sub_82485260) {
  GH_PROLOGUE();
  FieldValue<32, 352>(ctx, base);
}

GH_HOOK(sub_82485340) {
  GH_PROLOGUE();
  FieldValue<32, 1312>(ctx, base);
}

GH_HOOK(sub_82485B98) {
  GH_PROLOGUE();
  FieldValue<32, 160>(ctx, base);
}

GH_HOOK(sub_82486018) {
  GH_PROLOGUE();
  FieldValue<32, 976>(ctx, base);
}

GH_HOOK(sub_824A4928) {
  GH_PROLOGUE();
  FieldValue<8, 440>(ctx, base);
}

GH_HOOK(sub_825D7FB0) {
  GH_PROLOGUE();
  FieldValue<8, 10499>(ctx, base);
}

GH_HOOK(sub_825D7FD0) {
  GH_PROLOGUE();
  FieldValue<8, 10498>(ctx, base);
}

GH_HOOK(sub_825D7FF0) {
  GH_PROLOGUE();
  FieldValue<8, 10497>(ctx, base);
}

GH_HOOK(sub_825D8010) {
  GH_PROLOGUE();
  FieldValue<8, 10495>(ctx, base);
}

GH_HOOK(sub_825D8030) {
  GH_PROLOGUE();
  FieldValue<8, 10494>(ctx, base);
}

GH_HOOK(sub_825D8050) {
  GH_PROLOGUE();
  FieldValue<8, 10493>(ctx, base);
}

GH_HOOK(sub_825D80B0) {
  GH_PROLOGUE();
  FieldValue<32, 12160>(ctx, base);
}

GH_HOOK(sub_825D8270) {
  GH_PROLOGUE();
  FieldValue<32, 10752>(ctx, base);
}

GH_HOOK(sub_825D82F0) {
  GH_PROLOGUE();
  FieldValue<32, 12168>(ctx, base);
}

GH_HOOK(sub_825D8330) {
  GH_PROLOGUE();
  FieldValue<32, 12172>(ctx, base);
}

GH_HOOK(sub_825D8370) {
  GH_PROLOGUE();
  FieldValue<32, 12176>(ctx, base);
}

GH_HOOK(sub_825D8380) {
  GH_PROLOGUE();
  FieldValue<32, 12188>(ctx, base);
}

GH_HOOK(sub_825D88C0) {
  GH_PROLOGUE();
  FieldValue<32, 12280>(ctx, base);
}

GH_HOOK(sub_825D8958) {
  GH_PROLOGUE();
  FieldValue<32, 12284>(ctx, base);
}

GH_HOOK(sub_825D89F0) {
  GH_PROLOGUE();
  FieldValue<32, 12288>(ctx, base);
}

GH_HOOK(sub_825D8A88) {
  GH_PROLOGUE();
  FieldValue<32, 12292>(ctx, base);
}

GH_HOOK(sub_825D8BC0) {
  GH_PROLOGUE();
  FieldValue<32, 10456>(ctx, base);
}

GH_HOOK(sub_825D8C18) {
  GH_PROLOGUE();
  FieldValue<8, 10556>(ctx, base);
}

GH_HOOK(sub_825D8C48) {
  GH_PROLOGUE();
  FieldValue<32, 10700>(ctx, base);
}

GH_HOOK(sub_825D8C78) {
  GH_PROLOGUE();
  FieldValue<32, 10692>(ctx, base);
}

GH_HOOK(sub_825D8CA0) {
  GH_PROLOGUE();
  FieldValue<32, 10704>(ctx, base);
}

GH_HOOK(sub_825D8CD0) {
  GH_PROLOGUE();
  FieldValue<32, 10696>(ctx, base);
}

GH_HOOK(sub_825D8E18) {
  GH_PROLOGUE();
  FieldValue<8, 10562>(ctx, base);
}

GH_HOOK(sub_825D8E28) {
  GH_PROLOGUE();
  FieldValue<32, 13872>(ctx, base);
}

GH_HOOK(sub_8263D570) {
  GH_PROLOGUE();
  FieldValue<32, 140>(ctx, base);
}

GH_HOOK(sub_8263D578) {
  GH_PROLOGUE();
  FieldValue<32, 136>(ctx, base);
}

GH_HOOK(sub_8264FBA0) {
  GH_PROLOGUE();
  FieldValue<32, 144>(ctx, base);
}

GH_HOOK(sub_826519C8) {
  GH_PROLOGUE();
  FieldValue<32, 36>(ctx, base);
}

GH_HOOK(sub_826519D0) {
  GH_PROLOGUE();
  FieldValue<32, 16>(ctx, base);
}

GH_HOOK(sub_826836A8) {
  GH_PROLOGUE();
  FieldValue<32, 480>(ctx, base);
}

GH_HOOK(sub_82687218) {
  GH_PROLOGUE();
  FieldValue<32, 348>(ctx, base);
}

GH_HOOK(sub_826897C0) {
  GH_PROLOGUE();
  FieldValue<16, 456>(ctx, base);
}

GH_HOOK(sub_82689FA8) {
  GH_PROLOGUE();
  FieldValue<32, 264>(ctx, base);
}

GH_HOOK(sub_8268AD10) {
  GH_PROLOGUE();
  FieldValue<32, 308>(ctx, base);
}

GH_HOOK(sub_8268AD18) {
  GH_PROLOGUE();
  FieldValue<16, 320>(ctx, base);
}

GH_HOOK(sub_8269A4F8) {
  GH_PROLOGUE();
  FieldValue<8, 52>(ctx, base);
}

GH_HOOK(sub_8269EC70) {
  GH_PROLOGUE();
  FieldValue<8, 68>(ctx, base);
}

GH_HOOK(sub_8269EC90) {
  GH_PROLOGUE();
  FieldValue<32, 88>(ctx, base);
}

GH_HOOK(sub_826A9090) {
  GH_PROLOGUE();
  FieldValue<32, 396>(ctx, base);
}

GH_HOOK(sub_826A9098) {
  GH_PROLOGUE();
  FieldValue<32, 180>(ctx, base);
}
