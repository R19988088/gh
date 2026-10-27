#include "native_runtime.h"

#include <bit>

// Clearing this range never enters the generated copy loop: its bounds are equal.
GH_HOOK(sub_824AB4E0) {
  GH_PROLOGUE();
  const uint32_t begin = GH_LOAD_U32(ctx.r3.u32);
  const uint32_t end = GH_LOAD_U32(ctx.r3.u32 + 4);
  ctx.r11.u64 = begin;
  ctx.r9.u64 = end;
  ctx.cr6.compare<uint32_t>(begin, end, ctx.xer);
  if (begin != end) {
    ctx.r10.u64 = end;
    ctx.cr6.compare<uint32_t>(end, end, ctx.xer);
    GH_STORE_U32(ctx.r3.u32 + 4, begin);
  }
}

// Resolve the selected index, then the record's inline or external payload.
GH_HOOK(sub_824A8AD8) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  const uint32_t begin = GH_LOAD_U32(object + 1680);
  const uint32_t end = GH_LOAD_U32(object + 1684);
  const uint32_t selected = GH_LOAD_U32(object + 1180);
  const uint64_t bytes = uint64_t(end) - begin;
  const int32_t signed_bytes = std::bit_cast<int32_t>(uint32_t(bytes));
  const int64_t count = signed_bytes >> 2;
  ctx.r11.u64 = begin;
  ctx.r9.u64 = end;
  ctx.r10.u64 = selected;
  ctx.r8.u64 = bytes;
  ctx.r7.s64 = count;
  ctx.xer.ca = signed_bytes < 0 && (uint32_t(bytes) & 3);
  ctx.cr6.compare<uint32_t>(uint32_t(count), selected, ctx.xer);
  if (uint32_t(count) <= selected) {
    ctx.r3.u64 = 0;
    return;
  }

  ctx.r10.u64 = selected << 2;
  const uint32_t index = GH_LOAD_U32(begin + ctx.r10.u32);
  ctx.r11.u64 = index;
  ctx.cr6.compare<int32_t>(std::bit_cast<int32_t>(index), 0, ctx.xer);
  if (index & 0x80000000u) {
    ctx.r3.u64 = 0;
    return;
  }

  ctx.r9.u64 = index << 2;
  ctx.r10.u64 = GH_LOAD_U32(object + 1664);
  ctx.r11.u64 = index * 80u;
  ctx.r3.u64 = ctx.r10.u64 + ctx.r11.u64;
  ctx.r10.u64 = GH_LOAD_U32(ctx.r3.u32 + 16);
  ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
  if (!ctx.r10.u32) {
    ctx.r3.u64 = 0;
    return;
  }
  ctx.r11.u64 = GH_LOAD_U32(ctx.r3.u32 + 20);
  ctx.cr6.compare<uint32_t>(ctx.r11.u32, 8, ctx.xer);
  if (ctx.r11.u32 >= 8) ctx.r3.u64 = GH_LOAD_U32(ctx.r3.u32);
}

// Copy the selected five-field layout, preserving interleaved loads and stores.
GH_HOOK(sub_828192B0) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  const uint32_t destination = ctx.r4.u32;
  const bool alternate = ctx.r5.u32 != 0;
  const uint32_t first = object + (alternate ? 7116u : 7092u);
  ctx.r11.u64 = ctx.r3.u64;
  ctx.r3.u64 = ctx.r5.u64;
  ctx.cr6.compare<int32_t>(ctx.r5.s32, 0, ctx.xer);
  ctx.r10.u64 = GH_LOAD_U32(first);
  GH_STORE_U32(destination, ctx.r10.u32);
  ctx.r9.u64 = GH_LOAD_U32(first + 4);
  GH_STORE_U32(destination + 4, ctx.r9.u32);
  ctx.r8.u64 = GH_LOAD_U32(object + (alternate ? 7128u : 7112u));
  GH_STORE_U32(destination + 8, ctx.r8.u32);
  ctx.r7.u64 = GH_LOAD_U32(object + (alternate ? 7124u : 7104u));
  GH_STORE_U32(destination + 16, ctx.r7.u32);
  ctx.r6.u64 = GH_LOAD_U32(object + (alternate ? 7132u : 21144u));
  GH_STORE_U32(destination + 12, ctx.r6.u32);
}

// Two flag bits map to 0, 1, 4, 3.
GH_HOOK(sub_8268D158) {
  GH_PROLOGUE();
  const uint32_t kind = GH_LOAD_U32(ctx.r3.u32 + 16) & 3;
  ctx.r11.u64 = kind;
  ctx.r3.u64 = kind == 2 ? 4 : kind;
  ctx.cr6.compare<uint32_t>(kind, kind < 2 ? 1 : 3, ctx.xer);
}

// Read one index from the signed word-distance range, or return -1.
GH_HOOK(sub_824A6CE0) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  const uint32_t end = GH_LOAD_U32(object + 1684);
  const uint32_t begin = GH_LOAD_U32(object + 1680);
  const uint64_t bytes = uint64_t(end) - begin;
  const int32_t signed_bytes = std::bit_cast<int32_t>(uint32_t(bytes));
  const int64_t count = signed_bytes >> 2;
  ctx.r11.u64 = end;
  ctx.r10.u64 = begin;
  ctx.r9.u64 = bytes;
  ctx.r8.s64 = count;
  ctx.xer.ca = signed_bytes < 0 && (uint32_t(bytes) & 3);
  ctx.cr6.compare<uint32_t>(uint32_t(count), ctx.r4.u32, ctx.xer);
  if (uint32_t(count) <= ctx.r4.u32) {
    ctx.r3.s64 = -1;
    return;
  }
  ctx.r11.u64 = GH_LOAD_U32(object + 1680);
  ctx.r10.u64 = ctx.r4.u32 << 2;
  ctx.r3.u64 = GH_LOAD_U32(ctx.r11.u32 + ctx.r10.u32);
}
