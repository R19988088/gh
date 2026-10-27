#include "native_runtime.h"

namespace {
// A removed active node becomes the first free node. Reads stay interleaved
// with writes because the container and its nodes may share storage.
template <uint32_t head_offset>
void PopFront(GuestContext& ctx, uint8_t* base) {
  const uint32_t owner = ctx.r3.u32;
  const uint32_t node = GH_LOAD_U32(owner + head_offset);
  ctx.r10.u64 = node;
  ctx.r11.u64 = ctx.r3.u64;
  ctx.cr6.compare<uint32_t>(node, 0, ctx.xer);
  if (!node) {
    ctx.r3.u64 = 0;
    return;
  }
  const uint32_t next = GH_LOAD_U32(node + 4);
  ctx.r9.u64 = next;
  ctx.r3.u64 = GH_LOAD_U32(node);
  ctx.cr6.compare<uint32_t>(next, 0, ctx.xer);
  GH_STORE_U32(owner + head_offset, next);
  if (next) {
    ctx.r8.u64 = 0;
    GH_STORE_U32(next + 8, 0u);
  } else {
    GH_STORE_U32(owner + head_offset + 4, 0u);
  }
  ctx.r9.u64 = GH_LOAD_U32(owner);
  GH_STORE_U32(node + 4, ctx.r9.u32);
  ctx.r9.u64 = GH_LOAD_U32(owner + head_offset + 8);
  GH_STORE_U32(owner, node);
  ctx.r10.u64 = ctx.r9.u64 - 1;
  GH_STORE_U32(owner + head_offset + 8, ctx.r10.u32);
}

template <uint32_t head_offset, bool decrement_in_r9>
void EraseNode(GuestContext& ctx, uint8_t* base) {
  const uint32_t owner = ctx.r3.u32;
  const uint32_t node = ctx.r4.u32;
  ctx.r10.u64 = GH_LOAD_U32(owner + head_offset);
  ctx.r11.u64 = ctx.r3.u64;
  ctx.r3.u64 = GH_LOAD_U32(node);
  if (node == ctx.r10.u32) {
    ctx.r10.u64 = GH_LOAD_U32(node + 4);
    GH_STORE_U32(owner + head_offset, ctx.r10.u32);
  } else {
    ctx.r10.u64 = GH_LOAD_U32(node + 8);
    ctx.r9.u64 = GH_LOAD_U32(node + 4);
    GH_STORE_U32(ctx.r10.u32 + 4, ctx.r9.u32);
  }
  ctx.r10.u64 = GH_LOAD_U32(owner + head_offset + 4);
  ctx.cr6.compare<uint32_t>(node, ctx.r10.u32, ctx.xer);
  if (node == ctx.r10.u32) {
    ctx.r10.u64 = GH_LOAD_U32(node + 8);
    GH_STORE_U32(owner + head_offset + 4, ctx.r10.u32);
  } else {
    ctx.r10.u64 = GH_LOAD_U32(node + 4);
    ctx.r9.u64 = GH_LOAD_U32(node + 8);
    GH_STORE_U32(ctx.r10.u32 + 8, ctx.r9.u32);
  }
  ctx.r10.u64 = GH_LOAD_U32(owner);
  GH_STORE_U32(node + 4, ctx.r10.u32);
  ctx.r10.u64 = GH_LOAD_U32(owner + head_offset + 8);
  if constexpr (decrement_in_r9) ctx.r9.u64 = ctx.r10.u64 - 1;
  else ctx.r10.u64 -= 1;
  GH_STORE_U32(owner, node);
  GH_STORE_U32(owner + head_offset + 8,
                decrement_in_r9 ? ctx.r9.u32 : ctx.r10.u32);
}

template <bool first>
void ScanSlots(GuestContext& ctx, uint8_t* base) {
  const uint32_t owner = ctx.r3.u32;
  ctx.r11.u64 = 0;
  if constexpr (first) {
    GH_STORE_U8(ctx.r6.u32, uint8_t(0));
    ctx.r10.u64 = 0;
    GH_STORE_U32(ctx.r4.u32, owner);
    ctx.r9.u64 = 0;
    GH_STORE_U32(ctx.r5.u32, 0u);
    GH_STORE_U8(owner + 512, uint8_t(0));
  } else {
    GH_STORE_U32(ctx.r5.u32, 0u);
    GH_STORE_U8(ctx.r6.u32, uint8_t(0));
    ctx.r11.u64 = GH_LOAD_U8(owner + 512);
    ctx.r10.u64 = ctx.r11.u64;
    ctx.cr6.compare<uint32_t>(ctx.r11.u32, 127, ctx.xer);
    if (ctx.r11.u32 >= 127) {
      ctx.r3.s64 = -2142240746;
      return;
    }
  }
  uint32_t index = ctx.r11.u32;
  for (; index < 127; ++index) {
    const uint32_t value = GH_LOAD_U32(owner + index * 4);
    if (value) {
      ctx.r9.u64 = value;
      ctx.r11.u64 = index;
      ctx.r10.u64 = index + 1;
      ctx.cr6.compare<uint32_t>(value, 0, ctx.xer);
      GH_STORE_U32(ctx.r5.u32, value);
      GH_STORE_U8(owner + 512, uint8_t(index + 1));
      ctx.r3.u64 = 0;
      GH_STORE_U8(ctx.r6.u32, uint8_t(index));
      return;
    }
  }
  ctx.r11.u64 = 127;
  ctx.r10.u64 = 127;
  ctx.r9.u64 = 508;
  ctx.cr6.compare<uint32_t>(127, 127, ctx.xer);
  ctx.r3.s64 = -2142240746;
}

template <uint32_t Stride>
void FindRecord(GuestContext& ctx, uint8_t* base) {
  const uint32_t owner = ctx.r3.u32;
  const uint32_t count = GH_LOAD_U32(owner);
  ctx.r8.u64 = count;
  ctx.r11.u64 = ctx.r3.u64;
  ctx.r3.u64 = 0;
  ctx.r10.u64 = 0;
  ctx.cr6.compare<uint32_t>(count, 0, ctx.xer);
  if (!count) return;
  const uint32_t records = GH_LOAD_U32(owner + 4);
  ctx.r9.u64 = records;
  uint64_t index = 0;
  uint32_t value = 0;
  for (; index < count; ++index) {
    const uint64_t offset = index * Stride;
    value = GH_LOAD_U32(records + uint32_t(offset));
    if (value == ctx.r4.u32) break;
  }
  ctx.r7.u64 = value;
  ctx.r10.u64 = index;
  ctx.r11.u64 = index * Stride;
  if (index < count) {
    if constexpr (Stride == 8) ctx.r11.u64 = uint32_t(ctx.r11.u64);
    ctx.r3.u64 = ctx.r11.u64 + records;
    ctx.cr6.compare<uint32_t>(value, ctx.r4.u32, ctx.xer);
  } else ctx.cr6.compare<uint32_t>(uint32_t(index), count, ctx.xer);
}
}  // namespace

GH_HOOK(sub_8263A170) { GH_PROLOGUE(); PopFront<24>(ctx, base); }
GH_HOOK(sub_82634E90) { GH_PROLOGUE(); PopFront<204>(ctx, base); }
GH_HOOK(sub_82675738) { GH_PROLOGUE(); PopFront<36>(ctx, base); }
GH_HOOK(sub_8263A1D0) { GH_PROLOGUE(); EraseNode<24, false>(ctx, base); }
GH_HOOK(sub_82636250) { GH_PROLOGUE(); EraseNode<204, false>(ctx, base); }
GH_HOOK(sub_8268E900) { GH_PROLOGUE(); EraseNode<204, true>(ctx, base); }
GH_HOOK(sub_82803DE0) { GH_PROLOGUE(); ScanSlots<true>(ctx, base); }
GH_HOOK(sub_82803E48) { GH_PROLOGUE(); ScanSlots<false>(ctx, base); }
GH_HOOK(sub_826B62A0) { GH_PROLOGUE(); FindRecord<24>(ctx, base); }
GH_HOOK(sub_826B5938) { GH_PROLOGUE(); FindRecord<8>(ctx, base); }

// Handle bit 31 selects a pool; bits 0..23 select a slot and 24..30 its generation.
GH_HOOK(sub_826DBB98) {
  GH_PROLOGUE();
  const uint32_t owner = ctx.r3.u32;
  const uint32_t handle = ctx.r4.u32;
  ctx.r11.u64 = GH_LOAD_U32(owner + 20);
  ctx.r10.u64 = handle & 0x80000000u;
  ctx.cr6.compare<uint32_t>(ctx.r10.u32, ctx.r11.u32, ctx.xer);
  if (ctx.r10.u32 == ctx.r11.u32) {
    ctx.r10.u64 = GH_LOAD_U32(owner);
    ctx.r11.u64 = uint64_t((handle << 3) & 0x07FFFFF8u) + ctx.r10.u64;
    ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r10.u32, ctx.xer);
    if (ctx.r11.u32 >= ctx.r10.u32) {
      ctx.r10.u64 = GH_LOAD_U32(owner + 16);
      ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r10.u32, ctx.xer);
      if (ctx.r11.u32 < ctx.r10.u32) {
        ctx.r10.u64 = GH_LOAD_U8(ctx.r11.u32 + 4);
        ctx.r9.u64 = (handle >> 24) & 127;
        ctx.cr6.compare<uint32_t>(ctx.r10.u32, ctx.r9.u32, ctx.xer);
        if (ctx.r10.u32 == ctx.r9.u32) {
          ctx.r3.u64 = GH_LOAD_U32(ctx.r11.u32);
          return;
        }
      }
    }
  }
  ctx.r3.u64 = 0;
}

// Extended identifiers carry a fixed twelve-byte type signature.
GH_HOOK(sub_8263BF50) {
  GH_PROLOGUE();
  const uint64_t object = ctx.r3.u64;
  ctx.r11.u64 = object;
  ctx.r3.u64 = GH_LOAD_U16(uint32_t(object));
  ctx.cr6.compare<uint32_t>(ctx.r3.u32, 65534, ctx.xer);
  if (ctx.r3.u32 != 65534) return;
  ctx.r9.u64 = object + 24;
  uint64_t left = object + 28;
  uint64_t right = uint64_t(int64_t(-2110268300));
  ctx.r8.u64 = left + 12;
  uint64_t difference = 0;
  for (unsigned i = 0; i < 12; ++i) {
    const uint32_t a = GH_LOAD_U8(uint32_t(left));
    ctx.r6.u64 = GH_LOAD_U8(uint32_t(right));
    difference = uint64_t(a) - ctx.r6.u64;
    if (difference) break;
    ++left;
    ++right;
    ctx.cr6.compare<int32_t>(int32_t(uint32_t(left)), ctx.r8.s32, ctx.xer);
  }
  ctx.r7.u64 = difference;
  ctx.r10.u64 = right;
  ctx.r11.u64 = left;
  ctx.cr0.compare<int32_t>(ctx.r7.s32, 0, ctx.xer);
  if (!difference) {
    ctx.r11.u64 = GH_LOAD_U32(ctx.r9.u32);
    ctx.r3.u64 = ctx.r11.u32 & 0xFFFF;
  } else ctx.r3.u64 = 0;
}

// Return a completed item to the free queue, advancing the active head if needed.
GH_HOOK(sub_826E8878) {
  GH_PROLOGUE();
  const uint32_t owner = ctx.r3.u32;
  const uint32_t node = ctx.r4.u32;
  ctx.r11.u64 = GH_LOAD_U32(node);
  ctx.r8.u64 = ctx.r3.u64;
  ctx.r10.u64 = 0;
  ctx.r9.u64 = GH_LOAD_U32(ctx.r11.u32);
  if (ctx.r9.u32 == 259) GH_STORE_U32(ctx.r11.u32, 0u);
  GH_STORE_U32(node, 0u);
  ctx.r11.u64 = ctx.r8.u64 + 8;
  ctx.r9.u64 = GH_LOAD_U32(owner + 8);
  if (node == ctx.r9.u32) {
    ctx.r9.u64 = GH_LOAD_U32(ctx.r11.u32);
    if (ctx.r9.u32) {
      ctx.r7.u64 = GH_LOAD_U32(ctx.r11.u32 + 4);
      if (ctx.r9.u32 == ctx.r7.u32) GH_STORE_U32(ctx.r11.u32 + 4, 0u);
      ctx.r7.u64 = GH_LOAD_U32(ctx.r9.u32 + 4);
      GH_STORE_U32(ctx.r11.u32, ctx.r7.u32);
      GH_STORE_U32(ctx.r9.u32 + 4, 0u);
    }
  }
  GH_STORE_U32(node + 4, 0u);
  ctx.r10.u64 = GH_LOAD_U32(owner + 4);
  ctx.cr6.compare<uint32_t>(ctx.r10.u32, 0, ctx.xer);
  if (ctx.r10.u32) GH_STORE_U32(ctx.r10.u32 + 4, node);
  else GH_STORE_U32(owner, node);
  ctx.r3.u64 = GH_LOAD_U32(ctx.r11.u32);
  GH_STORE_U32(owner + 4, node);
}

GH_HOOK(sub_826E8858) {
  GH_PROLOGUE();
  uint32_t node = GH_LOAD_U32(ctx.r3.u32);
  uint64_t count = 0;
  while (node) {
    node = GH_LOAD_U32(node + 4);
    ++count;
  }
  ctx.r11.u64 = node;
  ctx.r3.u64 = count;
  ctx.cr6.compare<uint32_t>(node, 0, ctx.xer);
}

GH_HOOK(sub_826E2E68) {
  GH_PROLOGUE();
  const uint32_t owner = ctx.r3.u32;
  const uint32_t count = GH_LOAD_U32(owner + 8);
  ctx.r9.u64 = count;
  uint64_t index = 0;
  if (count) {
    uint64_t cursor = ctx.r3.u64 + 96;
    uint64_t key = 0;
    for (; index < count; ++index, cursor += 4) {
      const uint32_t item = GH_LOAD_U32(uint32_t(cursor));
      key = GH_LOAD_U64(item);
      if (ctx.r4.u64 == key) break;
    }
    ctx.r8.u64 = key;
    ctx.r10.u64 = cursor;
  }
  ctx.cr6.compare<uint32_t>(uint32_t(index), count, ctx.xer);
  if (index < count) {
    ctx.r11.u64 = uint32_t((index + 24) << 2);
    ctx.r3.u64 = GH_LOAD_U32(owner + ctx.r11.u32);
  } else {
    ctx.r11.u64 = index;
    ctx.r3.u64 = 0;
  }
}

GH_HOOK(sub_82667980) {
  GH_PROLOGUE();
  const uint64_t source = ctx.r3.u64;
  const int32_t count = ctx.r4.s32;
  ctx.r11.u64 = 0;
  ctx.r10.u64 = source;
  ctx.cr6.compare<int32_t>(ctx.r6.s32, 3, ctx.xer);
  GH_STORE_U32(ctx.r5.u32 + 48, 0u);
  if (ctx.r6.s32 != 3) return;
  ctx.r9.u64 = GH_LOAD_U32(ctx.r5.u32 + 40);
  ctx.cr6.compare<int32_t>(ctx.r9.s32, 0, ctx.xer);
  if (!ctx.r9.u32) return;
  ctx.cr6.compare<uint32_t>(uint32_t(source), 0, ctx.xer);
  if (!uint32_t(source)) return;
  ctx.cr6.compare<int32_t>(count, 0, ctx.xer);
  if (count <= 0) return;
  uint64_t index = 0;
  uint32_t value = 0;
  for (; index < uint32_t(count); ++index) {
    value = GH_LOAD_U32(uint32_t(source + index * 4));
    if (value) break;
  }
  ctx.r9.u64 = value;
  ctx.r10.u64 = source + index * 4;
  if (index < uint32_t(count)) {
    ctx.r11.u64 = 1;
    ctx.cr6.compare<int32_t>(ctx.r9.s32, 0, ctx.xer);
    GH_STORE_U32(ctx.r5.u32 + 48, 1u);
  } else {
    ctx.r11.u64 = index;
    ctx.cr6.compare<int32_t>(int32_t(index), count, ctx.xer);
  }
}

GH_HOOK(sub_826C9AC0) {
  GH_PROLOGUE();
  const uint32_t owner = ctx.r3.u32;
  const uint32_t count = GH_LOAD_U32(owner + 8);
  ctx.r9.u64 = count;
  ctx.r11.u64 = ctx.r3.u64;
  ctx.r3.u64 = 0;
  ctx.r10.u64 = 0;
  ctx.cr6.compare<uint32_t>(count, 0, ctx.xer);
  if (!count) return;
  const uint32_t items = GH_LOAD_U32(owner);
  ctx.r8.u64 = items;
  uint64_t index = 0;
  uint32_t value = 0;
  for (; index < count; ++index) {
    value = GH_LOAD_U32(items + uint32_t(index * 4));
    if (value == ctx.r4.u32) break;
  }
  ctx.r7.u64 = value;
  ctx.r10.u64 = index;
  ctx.r11.u64 = index * 4;
  if (index < count) {
    ctx.r3.u64 = 1;
    ctx.cr6.compare<uint32_t>(value, ctx.r4.u32, ctx.xer);
  } else ctx.cr6.compare<uint32_t>(uint32_t(index), count, ctx.xer);
}

GH_HOOK(sub_826B7F10) {
  GH_PROLOGUE();
  const uint32_t node = ctx.r3.u32;
  ctx.r11.u64 = GH_LOAD_U32(node + 84);
  if (ctx.r11.u32) {
    ctx.r10.u64 = GH_LOAD_U32(node + 88);
    GH_STORE_U32(ctx.r11.u32 + 88, ctx.r10.u32);
  }
  ctx.r11.u64 = GH_LOAD_U32(node + 88);
  ctx.cr6.compare<uint32_t>(ctx.r11.u32, 0, ctx.xer);
  if (ctx.r11.u32) {
    ctx.r10.u64 = GH_LOAD_U32(node + 84);
    GH_STORE_U32(ctx.r11.u32 + 84, ctx.r10.u32);
  }
  ctx.r11.u64 = 0;
  GH_STORE_U32(node + 84, 0u);
  GH_STORE_U32(node + 88, 0u);
}

GH_HOOK(sub_826318B0) {
  GH_PROLOGUE();
  const uint32_t flags = GH_LOAD_U32(ctx.r4.u32 + 228);
  ctx.r3.u64 = GH_LOAD_U32(ctx.r4.u32 + 56);
  ctx.r11.u64 = (flags >> 7) & 1;
  ctx.cr0.compare<int32_t>(ctx.r11.s32, 0, ctx.xer);
  if (ctx.r11.u32) {
    ctx.r10.u64 = ctx.r3.u32 << 2;
    ctx.r11.u64 = GH_LOAD_U32(uint32_t(-2103640064) - 3504u);
    ctx.r3.u64 = GH_LOAD_U32(ctx.r11.u32 + ctx.r10.u32);
  }
}

GH_HOOK(sub_8268BF68) {
  GH_PROLOGUE();
  const uint32_t bit = ctx.r4.u32;
  const uint32_t offset = (bit >> 3) & 0x1FFC;
  const uint32_t mask = 1u << (bit & 31);
  ctx.r11.u64 = GH_LOAD_U32(ctx.r3.u32 + 8);
  ctx.r10.u64 = offset;
  ctx.r9.u64 = bit & 31;
  ctx.r8.u64 = 1;
  ctx.r7.u64 = mask;
  ctx.r6.u64 = GH_LOAD_U32(ctx.r11.u32 + offset);
  ctx.r5.u64 = mask ^ ctx.r6.u32;
  GH_STORE_U32(ctx.r11.u32 + offset, ctx.r5.u32);
}

GH_HOOK(sub_826A8698) {
  GH_PROLOGUE();
  const uint32_t owner = ctx.r3.u32;
  ctx.r11.u64 = GH_LOAD_U32(owner + 8);
  ctx.cr6.compare<uint32_t>(ctx.r4.u32, ctx.r11.u32, ctx.xer);
  if (ctx.r4.u32 > ctx.r11.u32) ctx.r3.s64 = -2145902591;
  else {
    GH_STORE_U32(owner + 12, ctx.r4.u32);
    ctx.r3.u64 = 0;
  }
}

GH_HOOK(sub_826A7868) {
  GH_PROLOGUE();
  const uint32_t owner = ctx.r3.u32;
  ctx.r10.u64 = GH_LOAD_U16(owner + 24);
  ctx.r11.u64 = ctx.r4.u32 & 0xFFFF;
  ctx.cr6.compare<uint32_t>(ctx.r11.u32, ctx.r10.u32, ctx.xer);
  if (ctx.r11.u32 > ctx.r10.u32) ctx.r3.s64 = -2147467259;
  else {
    ctx.r10.u64 = GH_LOAD_U32(owner + 64);
    const uint32_t offset = ctx.r11.u32 * 4;
    ctx.r3.u64 = 0;
    ctx.r11.u64 = GH_LOAD_U32(ctx.r10.u32 + offset);
    GH_STORE_U32(ctx.r5.u32, ctx.r11.u32);
  }
}

GH_HOOK(sub_826D26B8) {
  GH_PROLOGUE();
  const uint32_t value = ctx.r4.u32 & 0xFFFF;
  ctx.cr6.compare<uint32_t>(value, 8, ctx.xer);
  uint32_t mapped = value;
  if (value >= 8) {
    ctx.cr6.compare<uint32_t>(value, 16, ctx.xer);
    mapped = (value < 16 ? (value - 8) * 8 + 15 : (value - 16) * 32 + 103) & 0xFFFF;
  }
  ctx.r10.u64 = mapped;
  ctx.r9.u64 = GH_LOAD_U16(ctx.r3.u32 + 908);
  const uint32_t origin = GH_LOAD_U32(ctx.r3.u32 + 904);
  ctx.r11.u64 = ctx.r9.u64 - origin + mapped;
  ctx.r3.u64 = ctx.r11.u32 & 0xFFFF;
}

GH_HOOK(sub_826D2098) {
  GH_PROLOGUE();
  const uint32_t position = ctx.r4.u32 & 1023;
  const uint32_t previous = ctx.r3.u32 & 0xFFFF;
  ctx.r11.u64 = position;
  ctx.r10.u64 = ctx.r4.u32 & 0xFFFF;
  ctx.cr6.compare<uint32_t>(position, 256, ctx.xer);
  bool adjusted = false;
  uint32_t result = ctx.r4.u32 & 0xFFFF;
  if (position < 256) {
    ctx.r9.u64 = previous;
    ctx.cr6.compare<uint32_t>(previous, 512, ctx.xer);
    if (previous >= 512) {
      result = (result - 1024) & 0xFFFF;
      adjusted = true;
    }
  }
  if (!adjusted) {
    ctx.cr6.compare<uint32_t>(position, 768, ctx.xer);
    if (position >= 768) {
      ctx.cr6.compare<uint32_t>(previous, 512, ctx.xer);
      if (previous < 512) {
        result = (result + 1024) & 0xFFFF;
        adjusted = true;
      }
    }
  }
  if (adjusted) ctx.r4.u64 = result;
  ctx.r11.u64 = ctx.r4.u32 & 0xFC00;
  ctx.r10.u64 = previous;
  ctx.r3.u64 = ctx.r11.u64 | previous;
}

GH_HOOK(sub_826DA720) {
  GH_PROLOGUE();
  const uint32_t owner = ctx.r3.u32;
  ctx.r11.u64 = ctx.r3.u64;
  GH_STORE_U32(owner, ctx.r4.u32);
  ctx.cr6.compare<uint32_t>(ctx.r5.u32, 24, ctx.xer);
  ctx.r3.u64 = 0;
  if (ctx.r5.u32 < 24) return;
  ctx.r10.u64 = uint64_t(GH_LOAD_U16(ctx.r4.u32)) + 2;
  ctx.cr6.compare<uint32_t>(ctx.r10.u32, ctx.r5.u32, ctx.xer);
  if (ctx.r10.u32 != ctx.r5.u32) return;
  ctx.r10.u64 = ctx.r4.u64 + 24;
  ctx.r9.u64 = ctx.r5.u64 - 24;
  GH_STORE_U32(owner + 4, ctx.r10.u32);
  ctx.r3.u64 = 1;
  GH_STORE_U32(owner + 8, ctx.r9.u32);
}
