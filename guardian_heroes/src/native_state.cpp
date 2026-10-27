#include "native_runtime.h"

#include <bit>

// An empty four-byte queue is ready only in state 8.
GH_HOOK(sub_824D0BE8) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  const uint32_t end = GH_LOAD_U32(object + 3504);
  const uint32_t begin = GH_LOAD_U32(object + 3500);
  const uint64_t difference = uint64_t(end) - begin;
  const int32_t bytes = std::bit_cast<int32_t>(uint32_t(difference));
  const int32_t count = bytes >> 2;
  ctx.r11.u64 = end;
  ctx.r10.u64 = begin;
  ctx.r9.u64 = difference;
  ctx.r8.s64 = count;
  ctx.xer.ca = bytes < 0 && (uint32_t(bytes) & 3) != 0;
  ctx.cr0.compare<int32_t>(count, 0, ctx.xer);
  if (count != 0) {
    ctx.r3.u64 = 0;
    return;
  }
  const uint16_t state = GH_LOAD_U16(object + 976);
  const int64_t adjusted = int64_t(state) - 8;
  ctx.r11.s64 = adjusted;
  ctx.r10.u64 = adjusted == 0 ? 32 : __builtin_clz(uint32_t(adjusted));
  ctx.r3.u64 = state == 8;
}

// Commit a pending state and record whether it changed or was forced.
GH_HOOK(sub_82435950) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  const uint8_t forced = GH_LOAD_U8(object + 7);
  ctx.r11.u64 = forced;
  ctx.cr6.compare<uint32_t>(forced, 0, ctx.xer);
  if (forced == 0) {
    const uint16_t pending = GH_LOAD_U16(object + 2);
    const uint16_t current = GH_LOAD_U16(object);
    ctx.r11.u64 = pending;
    ctx.r10.u64 = current;
    ctx.cr6.compare<uint32_t>(pending, current, ctx.xer);
    if (pending == current) {
      ctx.r11.u64 = 0;
      GH_STORE_U8(object + 6, uint8_t(0));
      return;
    }
  }
  // Retain the second read pair and store order for guest memory aliases.
  const uint16_t current = GH_LOAD_U16(object);
  const uint16_t pending = GH_LOAD_U16(object + 2);
  ctx.r11.u64 = current;
  ctx.r10.u64 = 1;
  ctx.r9.u64 = pending;
  ctx.r8.u64 = 0;
  GH_STORE_U8(object + 6, uint8_t(1));
  GH_STORE_U8(object + 7, uint8_t(0));
  GH_STORE_U16(object + 4, current);
  GH_STORE_U16(object, pending);
}

// A zero limit permits free increment; otherwise stop at the signed limit.
GH_HOOK(sub_8243A9D8) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  const uint32_t limit = GH_LOAD_U32(object + 8);
  ctx.r11.u64 = limit;
  ctx.cr6.compare<int32_t>(std::bit_cast<int32_t>(limit), 0, ctx.xer);
  if (limit == 0) {
    const uint64_t next = uint64_t(GH_LOAD_U32(object)) + 1;
    ctx.r11.u64 = next;
    GH_STORE_U32(object, uint32_t(next));
    return;
  }
  const uint32_t current = GH_LOAD_U32(object);
  ctx.r10.u64 = current;
  ctx.cr6.compare<int32_t>(std::bit_cast<int32_t>(current),
                           std::bit_cast<int32_t>(limit), ctx.xer);
  if (std::bit_cast<int32_t>(current) < std::bit_cast<int32_t>(limit)) {
    const uint64_t next = uint64_t(current) + 1;
    ctx.r10.u64 = next;
    GH_STORE_U32(object, uint32_t(next));
    // Incrementing a value below the signed limit cannot overshoot it.
    ctx.cr6.compare<int32_t>(std::bit_cast<int32_t>(uint32_t(next)),
                             std::bit_cast<int32_t>(limit), ctx.xer);
  }
}

// Negative slots are absent; byte -1 marks an absent nonnegative slot.
GH_HOOK(sub_82496508) {
  GH_PROLOGUE();
  ctx.cr6.compare<int32_t>(ctx.r4.s32, 0, ctx.xer);
  if (ctx.r4.s32 < 0) {
    ctx.r3.u64 = 0;
    return;
  }
  const uint8_t value = GH_LOAD_U8(uint32_t(ctx.r3.u64 + ctx.r4.u64) + 20);
  const int64_t signed_value = std::bit_cast<int8_t>(value);
  const bool present = value != 0xFF;
  ctx.r10.u64 = value;
  ctx.r11.s64 = signed_value + 1;
  ctx.r9.s64 = signed_value;
  ctx.r3.u64 = present;
  ctx.xer.ca = present;
}

// Select an entry within its upper bound and reset its playback state.
GH_HOOK(sub_82425D18) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  const uint32_t index = GH_LOAD_U32(object + 136);
  const uint32_t table = GH_LOAD_U32(object + 112);
  const uint64_t entry = uint64_t(index << 2) + table;
  const uint16_t count = GH_LOAD_U16(uint32_t(entry) + 2);
  ctx.r10.u64 = entry;
  ctx.r11.u64 = count;
  ctx.cr6.compare<uint32_t>(count, 0, ctx.xer);
  if (count == 0) return;
  ctx.cr6.compare<int32_t>(ctx.r4.s32, count, ctx.xer);
  if (ctx.r4.s32 >= count) ctx.r4.u64 = uint64_t(count) - 1;
  ctx.r11.u64 = 0;
  GH_STORE_U32(object + 140, ctx.r4.u32);
  ctx.r10.u64 = 1;
  GH_STORE_U8(object + 176, uint8_t(0));
  GH_STORE_U8(object + 177, uint8_t(0));
  GH_STORE_U8(object + 179, uint8_t(1));
  GH_STORE_U8(object + 181, uint8_t(1));
  GH_STORE_U32(object + 184, uint32_t(0));
  GH_STORE_U32(object + 188, uint32_t(0));
}
