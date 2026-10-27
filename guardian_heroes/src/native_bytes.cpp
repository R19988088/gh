#include "native_runtime.h"

#include <algorithm>
#include <cstring>

#include "native_memory.h"

// Select one of four byte counters without the guest CTR branch chain.
GH_HOOK(sub_8246B550) {
  GH_PROLOGUE();
  const uint64_t index = ctx.r4.u64 - 1;
  ctx.r11.u64 = index;
  ctx.cr6.compare<uint32_t>(uint32_t(index), 3, ctx.xer);
  if (uint32_t(index) > 3) return;
  ctx.cr6.compare<int32_t>(int32_t(index), 0, ctx.xer);
  ctx.ctr.u64 = index - (uint32_t(index) == 1 ? 1 : 2);
  const uint32_t read_offset = uint32_t(index) == 3 ? 1708 : 1396 + 68 * uint32_t(index);
  const uint32_t write_offset = uint32_t(index) == 3 ? 1696 : 1368 + 68 * uint32_t(index);
  const uint8_t value = GH_LOAD_U8(ctx.r3.u32 + read_offset);
  if (uint32_t(index) != 3) ctx.r10.u64 = value;
  ctx.r11.s64 = int64_t(int8_t(value)) + 1;
  GH_STORE_U32(ctx.r3.u32 + write_offset, ctx.r11.u32);
  GH_STORE_U32(ctx.r3.u32 + write_offset + 4, ctx.r11.u32);
}

// Copy the selected five-word record from the game's constant table.
GH_HOOK(sub_8241BAF0) {
  GH_PROLOGUE();
  const int64_t index = int8_t(GH_LOAD_U8(ctx.r3.u32 + 23));
  ctx.r8.u64 = uint32_t(index) << 2;
  ctx.r6.u64 = uint64_t(index) + ctx.r8.u64;
  ctx.r7.u64 = 1;
  const uint64_t record = uint64_t(int64_t(-2103902208) + 4408) + (ctx.r6.u32 << 2);
  ctx.r11.u64 = record - 4;
  ctx.r10.u64 = ctx.r3.u64 + 20;
  const uint32_t destination = ctx.r3.u32 + 24;
  const uint32_t source = uint32_t(record);
  GH_STORE_U8(ctx.r3.u32 + 22, 1);
  if (NativeCopyRange(destination, source, 20)) {
    ctx.r9.u64 = GH_LOAD_U32(source + 16);
    std::memcpy(GH_RAW(destination), GH_RAW(source), 20);
  } else {
    for (uint32_t offset = 0; offset < 20; offset += 4) {
      ctx.r9.u64 = GH_LOAD_U32(source + offset);
      GH_STORE_U32(destination + offset, ctx.r9.u32);
    }
  }
  ctx.r11.u32 = source + 16;
  ctx.r10.u32 = destination + 16;
  ctx.ctr.u64 = 0;
}

// Signed-byte adjustment with the original upper-bound-first clamp ordering.
GH_HOOK(sub_824A7998) {
  GH_PROLOGUE();
  const int64_t value = int64_t(int8_t(GH_LOAD_U8(ctx.r3.u32))) + ((ctx.r6.u32 & 255) ? 1 : -1);
  ctx.r11.s64 = value;
  ctx.r10.s64 = ctx.r5.s32;
  ctx.r9.s64 = ctx.r4.s32;
  ctx.cr6.compare<int64_t>(value, ctx.r10.s64, ctx.xer);
  if (value > ctx.r10.s64) {
    GH_STORE_U8(ctx.r3.u32, ctx.r10.u8);
  } else {
    ctx.cr6.compare<int64_t>(value, ctx.r9.s64, ctx.xer);
    GH_STORE_U8(ctx.r3.u32, value < ctx.r9.s64 ? ctx.r9.u8 : ctx.r11.u8);
  }
}

// Find the first free slot using local scan state instead of guest loop state.
GH_HOOK(sub_82498068) {
  GH_PROLOGUE();
  const uint64_t first = ctx.r3.u64 + 31;
  uint32_t index = 0;
  uint8_t value = 0;
  while (index < 90) {
    value = GH_LOAD_U8(uint32_t(first + 524 * index));
    if (!(value & 1)) break;
    ++index;
  }
  ctx.r9.u64 = value;
  ctx.r8.u64 = value & 1;
  ctx.r11.u64 = index;
  ctx.r10.u64 = first + 524 * index;
  ctx.cr6.compare<uint32_t>(0, 0, ctx.xer);
  ctx.r3.u64 = index == 90 ? UINT64_MAX : index;
}

// Bounded string copy: retain forward overlap order and bulk-fill the padding.
GH_HOOK(sub_82888BD8) {
  GH_PROLOGUE();
  ctx.r11.u64 = ctx.r3.u64;
  ctx.cr6.compare<uint32_t>(ctx.r5.u32, 0, ctx.xer);
  if (!ctx.r5.u32) return;
  const uint64_t difference = ctx.r4.u64 - ctx.r3.u64;
  uint64_t destination = ctx.r3.u64;
  uint64_t remaining = ctx.r5.u64;
  uint8_t value;
  do {
    value = GH_LOAD_U8(uint32_t(difference) + uint32_t(destination));
    GH_STORE_U8(uint32_t(destination), value);
    ++destination;
    if (!value) break;
    --remaining;
    ctx.xer.ca = 1;
  } while (uint32_t(remaining));
  ctx.r9.u64 = value;
  ctx.r10.u64 = difference;
  ctx.r11.u64 = destination;
  ctx.r5.u64 = remaining;
  ctx.cr6.compare<uint32_t>(uint32_t(remaining), 0, ctx.xer);
  if (!uint32_t(remaining)) {
    ctx.cr0.compare<int32_t>(0, 0, ctx.xer);
    return;
  }
  ctx.xer.ca = 1;
  ctx.r10.u64 = remaining - 1;
  const uint32_t padding = ctx.r10.u32;
  ctx.cr0.compare<int32_t>(int32_t(padding), 0, ctx.xer);
  if (!padding) return;
  ctx.r11.u64 = destination - 1;
  ctx.cr0.compare<uint32_t>(padding, 0, ctx.xer);
  if (NativeMemoryRange(uint32_t(destination), padding)) {
    std::memset(GH_RAW(destination), 0, padding);
  } else {
    for (uint32_t i = 0; i < padding; ++i) GH_STORE_U8(uint32_t(destination) + i, 0);
  }
  ctx.r11.u32 = uint32_t(destination) + padding - 1;
  ctx.ctr.u64 = ctx.r10.u64 - padding;
}
