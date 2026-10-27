#include "native_runtime.h"

#include <bit>

GH_HOOK(sub_82496E58) {
  GH_PROLOGUE();
  const uint8_t disabled = GH_LOAD_U8(0x829CC39C);
  ctx.r10.s64 = -2103656472;
  ctx.r11.u64 = disabled;
  ctx.cr6.compare<uint32_t>(disabled, 0, ctx.xer);
  if (disabled) {
    ctx.r3.u64 = 0;
  } else {
    const uint8_t flags = GH_LOAD_U8(ctx.r3.u32 + 31);
    ctx.r11.u64 = flags;
    ctx.r3.u64 = (flags >> 1) & 1;
  }
}

GH_HOOK(sub_824A5938) {
  GH_PROLOGUE();
  const int32_t state = std::bit_cast<int16_t>(GH_LOAD_U16(ctx.r3.u32 + 4));
  ctx.r11.s64 = state;
  ctx.cr6.compare<int32_t>(state, 0, ctx.xer);
  if (state > 0) ctx.cr6.compare<int32_t>(state, 3, ctx.xer);
  ctx.r3.u64 = state > 0 && state <= 3;
}

GH_HOOK(sub_825D1170) {
  GH_PROLOGUE();
  const uint32_t blocked = GH_LOAD_U32(ctx.r13.u32 + 336);
  ctx.r11.u64 = blocked;
  ctx.cr6.compare<uint32_t>(blocked, 0, ctx.xer);
  if (blocked) {
    ctx.r3.u64 = 0;
  } else {
    const uint32_t thread_data = GH_LOAD_U32(ctx.r13.u32 + 256);
    ctx.r11.u64 = thread_data;
    ctx.r3.u64 = GH_LOAD_U32(thread_data + 352);
  }
}

GH_HOOK(sub_82495D38) {
  GH_PROLOGUE();
  const uint8_t mode = GH_LOAD_U8(0x829CC394);
  ctx.r10.s64 = -2103656472;
  ctx.r11.u64 = mode;
  ctx.cr6.compare<uint32_t>(mode, 1, ctx.xer);
  if (mode == 1) {
    const uint8_t value = GH_LOAD_U8(ctx.r3.u32);
    ctx.r11.u64 = value;
    ctx.r3.s64 = std::bit_cast<int8_t>(value);
  } else {
    ctx.r3.u64 = 0;
  }
}

GH_HOOK(sub_826FA128) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  ctx.cr6.compare<uint32_t>(object, 0, ctx.xer);
  if (!object) return;
  uint32_t active = GH_LOAD_U32(object + 15364);
  ctx.cr6.compare<int32_t>(std::bit_cast<int32_t>(active), 0, ctx.xer);
  if (!active) {
    active = GH_LOAD_U32(object + 15432);
    ctx.cr6.compare<int32_t>(std::bit_cast<int32_t>(active), 0, ctx.xer);
  }
  ctx.r11.u64 = active;
  ctx.r3.u64 = active != 0;
}

// Map the supported HRESULT families, leaving unrelated failures unchanged.
GH_HOOK(sub_82643B70) {
  GH_PROLOGUE();
  const uint32_t address = ctx.r3.u32;
  const uint32_t error = GH_LOAD_U32(address);
  const int32_t signed_error = std::bit_cast<int32_t>(error);
  ctx.r11.u64 = error;
  ctx.cr6.compare<int32_t>(signed_error, 0, ctx.xer);
  if (!error) return;
  ctx.r10.s64 = -2147024882;
  ctx.cr6.compare<int32_t>(signed_error, ctx.r10.s32, ctx.xer);
  if (error == 0x8007000E) return;
  ctx.r10.s64 = -2147467263;
  ctx.cr6.compare<int32_t>(signed_error, ctx.r10.s32, ctx.xer);
  if (error == 0x80004001) return;
  const uint32_t facility = (error >> 16) & 0x1FFF;
  ctx.xer.ca = signed_error < 0 && (error & 0xFFFF) != 0;
  ctx.r10.u64 = facility;
  ctx.cr6.compare<int32_t>(facility, 2198, ctx.xer);
  if (facility == 2198) return;
  ctx.r10.s64 = -2005401480;
  const uint64_t delta = uint64_t(error) - ctx.r10.u64;
  const uint32_t code = uint32_t(delta);
  ctx.r11.u64 = delta;
  ctx.cr0.compare<int32_t>(std::bit_cast<int32_t>(code), 0, ctx.xer);
  uint32_t mapped = 0x88960004;
  if (code != 0) {
    ctx.cr6.compare<uint32_t>(code, 30, ctx.xer);
    if (code != 30) {
      ctx.r10.u64 = 0xFFF89;
      ctx.cr6.compare<uint32_t>(code, 0xFFF89, ctx.xer);
      if (code != 0xFFF89) {
        ctx.r10.u64 = 0x1EFF89;
        ctx.cr6.compare<uint32_t>(code, 0x1EFF89, ctx.xer);
        if (code != 0x1EFF89) return;
        mapped = 0x88960003;
      }
    }
  }
  ctx.r11.s64 = std::bit_cast<int32_t>(mapped);
  GH_STORE_U32(address, mapped);
}

// Nullable byte-string equality, with the guest's final comparison state.
GH_HOOK(sub_824279F0) {
  GH_PROLOGUE();
  uint64_t left = ctx.r3.u64;
  uint64_t right = ctx.r4.u64;
  ctx.r11.u64 = left;
  ctx.r10.u64 = right;
  ctx.cr6.compare<uint32_t>(uint32_t(left), 0, ctx.xer);
  if (!uint32_t(left)) {
    ctx.r3.u64 = 0;
    return;
  }
  ctx.cr6.compare<uint32_t>(uint32_t(right), 0, ctx.xer);
  if (!uint32_t(right)) {
    ctx.r3.u64 = 0;
    return;
  }
  uint64_t difference;
  do {
    const uint8_t a = GH_LOAD_U8(uint32_t(left));
    const uint8_t b = GH_LOAD_U8(uint32_t(right));
    ctx.r8.u64 = b;
    difference = uint64_t(a) - b;
    ctx.cr6.compare<int32_t>(a, 0, ctx.xer);
    if (!a) break;
    ++left;
    ++right;
    ctx.cr6.compare<int32_t>(std::bit_cast<int32_t>(uint32_t(difference)), 0, ctx.xer);
  } while (!difference);
  ctx.r9.u64 = difference;
  ctx.r10.u64 = right;
  ctx.r11.u64 = difference == 0 ? 32 : __builtin_clz(uint32_t(difference));
  ctx.r3.u64 = difference == 0;
}

GH_HOOK(sub_82434978) {
  GH_PROLOGUE();
  const int32_t index = ctx.r3.s32;
  ctx.cr6.compare<int32_t>(index, 0, ctx.xer);
  if (index < 0) {
    ctx.r3.u64 = 0;
    return;
  }
  const uint32_t mask = (uint32_t(index) & 32) ? 0 : 1u << (uint32_t(index) & 31);
  const uint32_t flags = GH_LOAD_U32(0x829A21F4);
  const uint32_t selected = mask & flags;
  ctx.r11.u64 = flags;
  ctx.r10.u64 = 1;
  ctx.r9.u64 = mask;
  ctx.r8.u64 = selected;
  ctx.r7.u64 = uint64_t(selected) - 1;
  ctx.r3.u64 = selected != 0;
  ctx.xer.ca = selected != 0;
}

GH_HOOK(sub_824348C8) {
  GH_PROLOGUE();
  uint8_t active = GH_LOAD_U8(0x829A21EB);
  ctx.cr6.compare<uint32_t>(active, 0, ctx.xer);
  if (!active) {
    active = GH_LOAD_U8(0x829A2208);
    ctx.cr6.compare<uint32_t>(active, 0, ctx.xer);
  }
  ctx.r10.u64 = active;
  ctx.r11.u64 = active != 0;
  ctx.r3.u64 = active != 0;
}

GH_HOOK(sub_82425CE0) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  const uint32_t previous = GH_LOAD_U32(object + 136);
  ctx.r11.u64 = previous;
  ctx.cr6.compare<int32_t>(std::bit_cast<int32_t>(previous), ctx.r4.s32, ctx.xer);
  if (previous == ctx.r4.u32) return;
  ctx.r11.u64 = 0;
  GH_STORE_U32(object + 136, ctx.r4.u32);
  ctx.r10.u64 = 1;
  GH_STORE_U32(object + 140, uint32_t(0));
  GH_STORE_U8(object + 176, uint8_t(0));
  GH_STORE_U8(object + 177, uint8_t(0));
  GH_STORE_U8(object + 179, uint8_t(1));
  GH_STORE_U8(object + 181, uint8_t(1));
  GH_STORE_U32(object + 184, uint32_t(0));
  GH_STORE_U32(object + 188, uint32_t(0));
}

GH_HOOK(sub_82803538) {
  GH_PROLOGUE();
  const uint64_t table = ctx.r3.u64;
  ctx.r10.u64 = 0;
  ctx.r11.u64 = table;
  ctx.cr6.compare<uint32_t>(uint32_t(table), 0, ctx.xer);
  ctx.r3.u64 = 0;
  if (!uint32_t(table)) return;
  ctx.cr6.compare<int32_t>(ctx.r4.s32, 127, ctx.xer);
  if (ctx.r4.s32 >= 127) {
    ctx.r3.s64 = -2147024809;
    return;
  }
  const uint32_t offset = ctx.r4.u32 * 12;
  const uint64_t entry = table + offset;
  ctx.r9.u64 = offset;
  ctx.r11.u64 = entry;
  GH_STORE_U32(uint32_t(entry), uint32_t(0));
  GH_STORE_U32(uint32_t(entry) + 4, uint32_t(0));
  GH_STORE_U32(uint32_t(entry) + 8, uint32_t(0));
}

GH_HOOK(sub_824A5880) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  const uint8_t enabled = GH_LOAD_U8(object + 1188);
  const uint32_t active = enabled ? GH_LOAD_U32(object + 60) : 0;
  const uint32_t mask = GH_LOAD_U32(object + 1192);
  const uint32_t selected = active & mask;
  ctx.r11.u64 = ctx.r3.u64;
  ctx.r10.u64 = active;
  ctx.r9.u64 = mask;
  ctx.r8.u64 = selected;
  ctx.cr6.compare<uint32_t>(selected, 0, ctx.xer);
  ctx.r3.u64 = selected == 0;
  if (selected) {
    ctx.r10.u64 = 1;
    ctx.r9.u64 = 4;
    GH_STORE_U8(object + 1186, uint8_t(1));
    GH_STORE_U16(object + 6, uint16_t(4));
  }
}

GH_HOOK(sub_824987A0) {
  GH_PROLOGUE();
  const uint32_t upper = uint32_t(GH_LOAD_U16(ctx.r3.u32 + 20)) << 16;
  const uint32_t lower = uint32_t(GH_LOAD_U16(ctx.r3.u32 + 18)) << 16;
  const uint32_t input = GH_LOAD_U32(ctx.r4.u32);
  const int64_t value = std::bit_cast<int32_t>(input);
  const int64_t maximum = std::bit_cast<int32_t>(upper);
  const int64_t minimum = std::bit_cast<int32_t>(lower);
  ctx.r8.u64 = upper;
  ctx.r7.u64 = lower;
  ctx.r11.s64 = value;
  ctx.r10.s64 = maximum;
  ctx.r9.s64 = minimum;
  ctx.cr6.compare<int64_t>(value, maximum, ctx.xer);
  uint32_t result = upper;
  if (value <= maximum) {
    ctx.cr6.compare<int64_t>(value, minimum, ctx.xer);
    result = value < minimum ? lower : input;
  }
  GH_STORE_U32(ctx.r4.u32, result);
}

GH_HOOK(sub_82421B40) {
  GH_PROLOGUE();
  const uint16_t flags = GH_LOAD_U16(0x82991518);
  const uint32_t nibble = flags & 15;
  const uint32_t selected = flags & 9;
  // The wrapping guest mask retains the duplicated high word too.
  const uint64_t scratch = uint64_t(nibble) << 32 | selected;
  ctx.r11.u64 = flags;
  ctx.r10.u64 = scratch;
  ctx.r9.u64 = scratch - 1;
  ctx.r3.u64 = selected != 0;
  ctx.xer.ca = selected != 0;
}

// Collapse repeated subtraction on the ordinary nonnegative remainder path.
GH_HOOK(sub_827667E0) {
  GH_PROLOGUE();
  const uint64_t input = ctx.r3.u64;
  const uint64_t divisor = ctx.r4.u64;
  const int32_t value = ctx.r3.s32;
  const int32_t step = ctx.r4.s32;
  ctx.cr6.compare<int32_t>(value, 0, ctx.xer);
  if (value >= 0) {
    ctx.cr6.compare<int32_t>(step, 0, ctx.xer);
    if (!step) {
      ctx.r3.u64 = 0;
      return;
    }
    if (step > 0) {
      const uint32_t quotient = uint32_t(value) / uint32_t(step);
      const int32_t terminal = int32_t(uint32_t(value) % uint32_t(step)) - step;
      ctx.r3.u64 = input - uint64_t(quotient) * divisor;
      ctx.cr0.compare<int32_t>(terminal, 0, ctx.xer);
      return;
    }
  }
  uint64_t result = input;
  do {
    result -= divisor;
  } while (std::bit_cast<int32_t>(uint32_t(result)) >= 0);
  ctx.cr0.compare<int32_t>(std::bit_cast<int32_t>(uint32_t(result)), 0, ctx.xer);
  ctx.r3.u64 = result + divisor;
}

GH_HOOK(sub_82434BA0) {
  GH_PROLOGUE();
  const uint32_t index = GH_LOAD_U32(0x82911E60);
  ctx.cr6.compare<int32_t>(std::bit_cast<int32_t>(index), 0, ctx.xer);
  if (std::bit_cast<int32_t>(index) < 0) {
    ctx.r10.s64 = -2103836672;
    ctx.r11.u64 = 0;
    GH_STORE_U32(0x829A2214, uint32_t(0));
  } else {
    const uint32_t mask = (index & 32) ? 0 : 1u << (index & 31);
    ctx.r10.u64 = 1;
    ctx.r9.s64 = -2103836672;
    ctx.r11.u64 = mask;
    GH_STORE_U32(0x829A2214, mask);
  }
}

GH_HOOK(sub_8242ED00) {
  GH_PROLOGUE();
  const uint8_t first = GH_LOAD_U8(0x829A1912);
  ctx.r11.s64 = -2103836672;
  ctx.r10.u64 = first;
  ctx.cr6.compare<uint32_t>(first, 0, ctx.xer);
  if (!first) {
    ctx.r3.u64 = 1;
    return;
  }
  const uint8_t second = GH_LOAD_U8(0x829A1913);
  ctx.r10.u64 = second;
  ctx.r9.u64 = second == 0 ? 32 : __builtin_clz(uint32_t(second));
  ctx.r3.u64 = second == 0;
}

GH_HOOK(sub_82803460) {
  GH_PROLOGUE();
  const uint64_t table = ctx.r3.u64;
  ctx.r10.u64 = table;
  ctx.r3.u64 = 0;
  ctx.cr6.compare<int32_t>(ctx.r4.s32, 7, ctx.xer);
  if (ctx.r4.s32 >= 7) {
    ctx.r3.s64 = -2147024809;
    return;
  }
  const uint64_t tripled = ctx.r4.u64 + uint32_t(ctx.r4.u32 << 1);
  const uint32_t offset = ctx.r4.u32 * 12;
  const uint64_t entry = table + offset;
  ctx.r9.u64 = 1;
  ctx.r8.u64 = tripled;
  ctx.r11.u64 = entry;
  GH_STORE_U32(uint32_t(entry), uint32_t(1));
  GH_STORE_U32(uint32_t(entry) + 4, ctx.r5.u32);
  GH_STORE_U32(uint32_t(entry) + 8, ctx.r6.u32);
}

GH_HOOK(sub_82496990) {
  GH_PROLOGUE();
  const uint32_t object = ctx.r3.u32;
  const uint8_t first = GH_LOAD_U8(object + 985);
  const int64_t signed_first = std::bit_cast<int8_t>(first);
  ctx.r11.u64 = first;
  ctx.r10.s64 = signed_first;
  ctx.cr6.compare<int32_t>(int32_t(signed_first), ctx.r4.s32, ctx.xer);
  if (int32_t(signed_first) == ctx.r4.s32) {
    ctx.r3.u64 = 1;
    return;
  }
  const uint8_t second = GH_LOAD_U8(object + 986);
  const int64_t signed_second = std::bit_cast<int8_t>(second);
  const uint64_t difference = ctx.r4.u64 - uint64_t(signed_second);
  const bool different = uint32_t(difference) != 0;
  ctx.r11.u64 = second;
  ctx.r10.u64 = 2;
  ctx.r9.s64 = signed_second;
  ctx.r8.u64 = difference;
  ctx.r7.u64 = difference - 1;
  ctx.r5.u64 = different ? 0 : UINT64_MAX;
  ctx.r3.u64 = different ? 0 : 2;
  ctx.xer.ca = different;
}

GH_HOOK(sub_82498EC8) {
  GH_PROLOGUE();
  const uint32_t handle = GH_LOAD_U32(ctx.r3.u32);
  ctx.r11.u64 = handle;
  ctx.cr6.compare<uint32_t>(handle, 0, ctx.xer);
  if (!handle) {
    ctx.r3.u64 = 0;
    return;
  }
  const uint32_t resource = GH_LOAD_U32(handle);
  const uint32_t offset = GH_LOAD_U32(resource + 32);
  ctx.r11.u64 = resource;
  ctx.r10.u64 = offset;
  ctx.r3.u64 = uint64_t(resource) + offset;
}

GH_HOOK(sub_82498EF0) {
  GH_PROLOGUE();
  const uint32_t handle = GH_LOAD_U32(ctx.r3.u32);
  ctx.r11.u64 = handle;
  ctx.cr6.compare<uint32_t>(handle, 0, ctx.xer);
  if (!handle) {
    ctx.r3.u64 = 0;
    return;
  }
  const uint32_t resource = GH_LOAD_U32(handle);
  const uint32_t offset = GH_LOAD_U32(resource + 64);
  ctx.r11.u64 = resource;
  ctx.r10.u64 = offset;
  ctx.r3.u64 = uint64_t(resource) + offset;
}

GH_HOOK(sub_82498F18) {
  GH_PROLOGUE();
  const uint32_t handle = GH_LOAD_U32(ctx.r3.u32);
  ctx.r11.u64 = handle;
  ctx.cr6.compare<uint32_t>(handle, 0, ctx.xer);
  if (!handle) {
    ctx.r3.u64 = 0;
    return;
  }
  const uint32_t resource = GH_LOAD_U32(handle);
  const uint32_t offset = GH_LOAD_U32(resource + 4736);
  ctx.r11.u64 = resource;
  ctx.r10.u64 = offset;
  ctx.r3.u64 = uint64_t(resource) + offset;
}

GH_HOOK(sub_82499E40) {
  GH_PROLOGUE();
  const uint32_t handle = GH_LOAD_U32(ctx.r3.u32);
  ctx.r11.u64 = handle;
  ctx.cr6.compare<uint32_t>(handle, 0, ctx.xer);
  if (!handle) {
    ctx.r3.u64 = 0;
    return;
  }
  const uint32_t resource = GH_LOAD_U32(handle);
  const uint32_t offset = GH_LOAD_U32(resource + 3168);
  ctx.r11.u64 = resource;
  ctx.r10.u64 = offset;
  ctx.r3.u64 = uint64_t(resource) + offset;
}

GH_HOOK(sub_82499CD8) {
  GH_PROLOGUE();
  const uint32_t handle = GH_LOAD_U32(ctx.r3.u32);
  ctx.r11.u64 = handle;
  ctx.cr6.compare<uint32_t>(handle, 0, ctx.xer);
  if (!handle) {
    ctx.r3.u64 = 0;
    return;
  }
  const uint32_t resource = GH_LOAD_U32(handle);
  const uint32_t offset = GH_LOAD_U32(resource + 2880);
  ctx.r11.u64 = resource;
  ctx.r10.u64 = offset;
  ctx.r3.u64 = uint64_t(resource) + offset;
}

GH_HOOK(sub_82499D00) {
  GH_PROLOGUE();
  const uint32_t handle = GH_LOAD_U32(ctx.r3.u32);
  ctx.r11.u64 = handle;
  ctx.cr6.compare<uint32_t>(handle, 0, ctx.xer);
  if (!handle) {
    ctx.r3.u64 = 0;
    return;
  }
  const uint32_t resource = GH_LOAD_U32(handle);
  const uint32_t offset = GH_LOAD_U32(resource + 2912);
  ctx.r11.u64 = resource;
  ctx.r10.u64 = offset;
  ctx.r3.u64 = uint64_t(resource) + offset;
}
