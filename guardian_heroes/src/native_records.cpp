#include "native_runtime.h"
#include "native_memory.h"

#include <array>
#include <cstring>

GH_EXTERN(__imp__sub_8243F2C8);
GH_EXTERN(__imp__sub_82435928);
GH_EXTERN(__imp__sub_8243FCE0);
GH_EXTERN(__imp__sub_8246AC40);
GH_EXTERN(__imp__sub_82495BC0);
GH_EXTERN(__imp__sub_82420AB0);
GH_EXTERN(__imp__sub_826CA638);
GH_EXTERN(__imp__sub_82495BA0);
GH_EXTERN(__imp__sub_82689358);
GH_EXTERN(__imp__sub_82420D50);
GH_EXTERN(__imp__sub_827D3DC0);
GH_EXTERN(__imp__sub_82495C88);
GH_EXTERN(__imp__sub_8281D550);
GH_EXTERN(__imp__sub_8269B3E8);
GH_EXTERN(__imp__sub_824257D8);
GH_EXTERN(__imp__sub_824964A0);
GH_EXTERN(__imp__sub_826536C0);
GH_EXTERN(__imp__sub_8248C130);
GH_EXTERN(__imp__sub_82411A08);
GH_EXTERN(__imp__sub_8280FD08);

GH_HOOK(sub_82887F88) {
  uint64_t pointer = ctx.r3.u64;
  while (GH_LOAD_U16(uint32_t(pointer)) != 0) pointer += 2;
  const uint32_t bytes = uint32_t(pointer + 2 - ctx.r3.u64);
  ctx.r10.u64 = 0;
  ctx.cr0.compare<uint32_t>(0, 0, ctx.xer);
  ctx.xer.ca = (int32_t(bytes) < 0) && (bytes & 1);
  ctx.r11.s64 = int32_t(bytes) >> 1;
  ctx.r3.u64 = ctx.r11.u64 - 1;
}

GH_HOOK(sub_8243F2C8) {
  const uint32_t object = ctx.r3.u32;
  if (!NativeMemoryRange(object, 48)) return __imp__sub_8243F2C8(ctx, base);
  std::memset(GH_RAW(object + 24), 0, 24);
  ctx.r11.u64 = 0;
}

GH_HOOK(sub_82435928) {
  const uint32_t object = ctx.r3.u32;
  if (!NativeMemoryRange(object, 9)) return __imp__sub_82435928(ctx, base);
  const uint8_t record[] = {uint8_t(ctx.r4.u16 >> 8), ctx.r4.u8,
      uint8_t(ctx.r4.u16 >> 8), ctx.r4.u8, uint8_t(ctx.r4.u16 >> 8), ctx.r4.u8, 0, 1, 0};
  std::memcpy(GH_RAW(object), record, sizeof(record));
  ctx.r11.u64 = 0;
  ctx.r10.u64 = 1;
}

GH_HOOK(sub_8243FCE0) {
  const uint32_t object = ctx.r3.u32;
  if (!NativeMemoryRange(object, 24)) return __imp__sub_8243FCE0(ctx, base);
  std::memset(GH_RAW(object), 0, 7);
  std::memset(GH_RAW(object + 8), 0, 16);
  ctx.r11.u64 = 0;
}

GH_HOOK(sub_8246AC40) {
  const uint32_t object = ctx.r3.u32;
  if (!NativeMemoryRange(object, 233)) return __imp__sub_8246AC40(ctx, base);
  GH_STORE_U8(object + 232, 0);
  std::memset(GH_RAW(object + 24), 0, 24);
  ctx.r11.u64 = 0;
}

GH_HOOK(sub_82495BC0) {
  const uint32_t object = ctx.r3.u32;
  if (!NativeMemoryRange(object, 6)) return __imp__sub_82495BC0(ctx, base);
  constexpr uint8_t record[] = {8, 6, 4, 4, 6, 10};
  std::memcpy(GH_RAW(object), record, sizeof(record));
  ctx.r10.u64 = 6; ctx.r11.u64 = 4; ctx.r9.u64 = 8; ctx.r8.u64 = 10;
}

GH_HOOK(sub_82420AB0) {
  const uint32_t object = ctx.r3.u32;
  if (!NativeMemoryRange(object, 64)) return __imp__sub_82420AB0(ctx, base);
  for (uint32_t offset : {4u, 36u, 52u}) std::memset(GH_RAW(object + offset), 0, 12);
  ctx.r11.u64 = 0;
}

GH_HOOK(sub_826CA638) {
  const uint32_t object = ctx.r3.u32;
  if (!NativeMemoryRange(object, 32)) return __imp__sub_826CA638(ctx, base);
  GH_STORE_U32(object, ctx.r4.u32);
  GH_STORE_U32(object + 4, 0);
  GH_STORE_U32(object + 8, ctx.r5.u32);
  GH_STORE_U32(object + 12, ctx.r6.u32);
  GH_STORE_U32(object + 16, ctx.r7.u32);
  GH_STORE_U32(object + 20, ctx.r7.u32);
  GH_STORE_U32(object + 24, ctx.r8.u32);
  GH_STORE_U32(object + 28, 0);
  ctx.r11.u64 = 0;
}

GH_HOOK(sub_82495BA0) {
  const uint32_t object = ctx.r3.u32;
  if (!NativeMemoryRange(object, 6)) return __imp__sub_82495BA0(ctx, base);
  std::memset(GH_RAW(object), 0, 6);
  ctx.r11.u64 = 0;
}

GH_HOOK(sub_82689358) {
  const uint32_t object = ctx.r3.u32;
  if (!NativeMemoryRange(object, 220)) return __imp__sub_82689358(ctx, base);
  GH_STORE_U32(object + 216, 16);
  std::memset(GH_RAW(object + 204), 0, 12);
  GH_STORE_U32(object + 4, object + 8);
  GH_STORE_U32(object + 8, 0);
  for (uint32_t i = 0; i < 16; ++i) GH_STORE_U32(object + 16 + 12 * i, i ? object + 12 * i : 0);
  GH_STORE_U32(object, object + 192);
  ctx.r10.u64 = uint64_t(object) + 188;
  ctx.r8.u64 = object + 180;
  ctx.r9.u64 = uint64_t(object) + 192;
  ctx.r11.u64 = 192;
  ctx.ctr.u64 = 0;
}

GH_HOOK(sub_824156A0) {
  const uint8_t value = ctx.r3.u8;
  ctx.cr6.compare<uint32_t>(value, 0, ctx.xer);
  ctx.r9.s64 = -2103967744ll - 5048;
  ctx.r11.u64 = 0;
  ctx.r10.u64 = value != 0;
  GH_STORE_U8(ctx.r9.u32 + 66, 0);
  GH_STORE_U8(ctx.r9.u32 + 67, ctx.r10.u8);
}

GH_HOOK(sub_82420D50) {
  const uint32_t object = ctx.r3.u32;
  if (!NativeMemoryRange(object, 73)) return __imp__sub_82420D50(ctx, base);
  ctx.r10.s64 = -2113929216ll; ctx.r11.u64 = 0;
  ctx.r9.u64 = ctx.r10.u64 + 4104; ctx.r8.u64 = 8;
  for (uint32_t offset : {0u, 32u, 48u}) {
    GH_STORE_U32(object + offset, ctx.r9.u32);
    std::memset(GH_RAW(object + offset + 4), 0, 12);
  }
  GH_STORE_U8(object + 16, 8); GH_STORE_U8(object + 17, 0);
  std::memset(GH_RAW(object + 20), 0, 12);
  GH_STORE_U32(object + 64, ctx.r4.u32);
  GH_STORE_U32(object + 68, ctx.r5.u32);
  GH_STORE_U8(object + 72, ctx.r6.u8);
}

GH_HOOK(sub_827EACF8) {
  uint64_t destination = ctx.r3.u64;
  ctx.r3.s64 = int64_t(ctx.r5.s32) * int64_t(ctx.r6.s32);
  ctx.cr0.compare<int32_t>(ctx.r3.s32, 0, ctx.xer);
  ctx.r11.u64 = destination; ctx.r10.u64 = 0;
  if (!ctx.cr0.gt) return;
  const uint64_t delta = ctx.r4.u64 - destination;
  uint64_t offset = 0;
  uint8_t value;
  do {
    value = GH_LOAD_U8(uint32_t(delta) + uint32_t(destination));
    offset += ctx.r5.u64;
    ctx.cr6.compare<int32_t>(int32_t(offset), ctx.r3.s32, ctx.xer);
    GH_STORE_U8(uint32_t(destination), value);
    destination += ctx.r5.u64;
  } while (ctx.cr6.lt);
  ctx.r9.u64 = delta; ctx.r8.u64 = value;
  ctx.r10.u64 = offset; ctx.r11.u64 = destination;
}

GH_HOOK(sub_8278EE28) {
  const uint64_t sign = ctx.r3.s32 < 0 ? ~uint64_t(0) : 0;
  ctx.xer.ca = (ctx.r3.s32 < 0) && ((ctx.r3.u32 & 0x7FFFFFFF) != 0);
  ctx.r10.u64 = ctx.r3.u64 ^ sign;
  const int32_t magnitude = int32_t(ctx.r10.u64 - sign);
  ctx.cr0.compare<int32_t>(magnitude, 0, ctx.xer);
  uint32_t bucket = 0;
  if (magnitude != 0) {
    bucket = 1;
    for (int32_t threshold : {3, 7, 15, 31}) {
      ctx.cr6.compare<int32_t>(magnitude, threshold, ctx.xer);
      if (ctx.cr6.lt) break;
      ++bucket;
    }
  }
  ctx.r11.u64 = bucket;
  GH_STORE_U32(ctx.r4.u32, bucket);
}

GH_HOOK(sub_827D3DC0) {
  const uint32_t destination = ctx.r3.u32, source = ctx.r4.u32;
  if (!NativeCopyRange(destination, source, 16)) return __imp__sub_827D3DC0(ctx, base);
  ctx.r11.u64 = GH_LOAD_U32(source); ctx.r10.u64 = GH_LOAD_U32(source + 4);
  ctx.r9.u64 = GH_LOAD_U32(source + 8); ctx.r8.u64 = GH_LOAD_U32(source + 12);
  std::memcpy(GH_RAW(destination), GH_RAW(source), 16);
}

GH_HOOK(sub_82495C88) {
  const uint32_t object = ctx.r3.u32;
  if (!NativeMemoryRange(object, 524)) return __imp__sub_82495C88(ctx, base);
  static constexpr auto record = [] {
    std::array<uint8_t, 524> bytes{};
    bytes[5] = 4;
    for (size_t i = 0; i < 12; ++i) { bytes[7 + i] = 45; bytes[19 + i] = i; }
    bytes[32] = bytes[33] = 255;
    for (size_t i = 68; i < bytes.size(); i += 8) {
      bytes[i + 1] = 200; bytes[i + 2] = 2;
      for (size_t j = 3; j < 8; ++j) bytes[i + j] = 255;
    }
    return bytes;
  }();
  std::memcpy(GH_RAW(object), record.data(), record.size());
  ctx.r9.u64 = 0; ctx.r10.u64 = 65535;
  ctx.r11.u64 = (ctx.r3.u64 + 66) & 0xFFFFFFFF00000000ull;
  ctx.r11.u32 = object + 522;
  ctx.r8.u64 = 255; ctx.r6.u64 = 200; ctx.r7.u64 = 2; ctx.ctr.u64 = 0;
}

GH_HOOK(sub_8281D550) {
  const uint32_t object = ctx.r3.u32;
  if (!NativeMemoryRange(object, 20)) return __imp__sub_8281D550(ctx, base);
  const uint32_t first = GH_LOAD_U32(object);
  ctx.r10.u64 = first; ctx.r11.u64 = 0; ctx.r9.u64 = 32;
  GH_STORE_U32(object + 4, 0); GH_STORE_U32(object + 8, first);
  GH_STORE_U32(object + 12, 0); GH_STORE_U32(object + 16, 32);
}

GH_HOOK(sub_8269B3E8) {
  const uint32_t object = ctx.r3.u32;
  if (!NativeMemoryRange(object, 48)) return __imp__sub_8269B3E8(ctx, base);
  const uint32_t input7 = ctx.r7.u32;
  const bool enabled = ctx.r8.u32 != 0;
  ctx.xer.ca = enabled; ctx.r9.u64 = enabled;
  ctx.r10.s64 = -2110128128ll; ctx.r8.u64 = ctx.r10.u64 + 6088;
  ctx.r7.u64 = enabled ? 8 : 0; ctx.r11.u64 = 0;
  GH_STORE_U32(object, ctx.r8.u32);
  GH_STORE_U32(object + 4, object); GH_STORE_U32(object + 8, object);
  GH_STORE_U32(object + 12, ctx.r4.u32); GH_STORE_U32(object + 16, ctx.r6.u32);
  std::memset(GH_RAW(object + 20), 0, 16);
  GH_STORE_U32(object + 36, input7); GH_STORE_U8(object + 40, ctx.r7.u8);
  GH_STORE_U32(object + 44, ctx.r5.u32);
}

GH_HOOK(sub_824257D8) {
  const uint32_t object = ctx.r3.u32;
  if (!NativeMemoryRange(object, 192)) return __imp__sub_824257D8(ctx, base);
  GH_STORE_U32(object + 136, ctx.r4.u32); GH_STORE_U32(object + 140, 0);
  GH_STORE_U16(object + 176, 0);
  GH_STORE_U8(object + 179, 1); GH_STORE_U8(object + 181, 1);
  std::memset(GH_RAW(object + 184), 0, 8);
  ctx.r11.u64 = 0; ctx.r10.u64 = 1;
}

GH_HOOK(sub_824964A0) {
  const uint32_t object = ctx.r3.u32;
  if (!NativeMemoryRange(object, 44)) return __imp__sub_824964A0(ctx, base);
  std::memset(GH_RAW(object + 21), 255, 11);
  std::memset(GH_RAW(object + 33), 255, 11);
  ctx.r10.s64 = -1; ctx.r11.u64 = 12;
  ctx.r9.u64 = ctx.r3.u64 + 20; ctx.r8.u64 = ctx.r3.u64 + 32;
  ctx.ctr.u64 = 0;
}

GH_HOOK(sub_826536C0) {
  const uint32_t object = ctx.r3.u32;
  if (!NativeMemoryRange(object, 32)) return __imp__sub_826536C0(ctx, base);
  const uint32_t values[] = {ctx.r4.u32, 0, 0, 7, 1, 0, 1};
  for (uint32_t i = 0; i < 7; ++i) GH_STORE_U32(object + 4 + 4 * i, values[i]);
  ctx.r11.u64 = 0; ctx.r10.u64 = 1; ctx.r9.u64 = 7;
}

GH_HOOK(sub_8248C130) {
  const uint32_t object = ctx.r3.u32;
  if (!NativeMemoryRange(object, 25)) return __imp__sub_8248C130(ctx, base);
  std::memset(GH_RAW(object), 0, 15);
  GH_STORE_U8(object + 15, 1); GH_STORE_U8(object + 16, 1); GH_STORE_U8(object + 17, 0);
  std::memset(GH_RAW(object + 20), 0, 5);
  ctx.r11.u64 = 0; ctx.r10.u64 = 1;
}

GH_HOOK(sub_82411A08) {
  const uint32_t object = ctx.r3.u32;
  if (!NativeMemoryRange(object, 16)) return __imp__sub_82411A08(ctx, base);
  GH_STORE_U8(object, 1);
  GH_STORE_U32(object + 4, 6); GH_STORE_U32(object + 8, 7); GH_STORE_U32(object + 12, 0);
  ctx.r11.u64 = 1; ctx.r10.u64 = 6; ctx.r9.u64 = 7; ctx.r8.u64 = 0;
}

GH_HOOK(sub_82425808) {
  const uint32_t object = ctx.r3.u32;
  const uint8_t pending = GH_LOAD_U8(object + 177);
  ctx.r11.u64 = pending;
  ctx.cr6.compare<uint32_t>(pending, 0, ctx.xer);
  if (pending == 0) return;
  GH_STORE_U8(object + 177, 0); GH_STORE_U8(object + 181, 1); GH_STORE_U32(object + 184, 0);
  ctx.r11.u64 = 0; ctx.r10.u64 = 1;
}

GH_HOOK(sub_8280FD08) {
  const uint32_t object = ctx.r3.u32;
  if (!NativeMemoryRange(object, 88)) return __imp__sub_8280FD08(ctx, base);
  std::memset(GH_RAW(object + 4), 0, 24);
  ctx.r11.u64 = GH_LOAD_U32(object);
  ctx.r10.u64 = 3; ctx.r9.u64 = ctx.r11.u32 & 0x1FFFFFFF;
  GH_STORE_U32(object + 84, 3); GH_STORE_U32(object, ctx.r9.u32);
  ctx.ctr.u64 = 0;
}
