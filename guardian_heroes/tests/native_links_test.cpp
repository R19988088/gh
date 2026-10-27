#include "native_helpers_test.h"

#include <array>
#include <random>

#define LINK_FUNCTIONS(X) \
  X(sub_8263A170) X(sub_82803DE0) X(sub_82803E48) X(sub_826DBB98) \
  X(sub_8268E900) X(sub_8263A1D0) X(sub_8263BF50) X(sub_826E8878) \
  X(sub_82636250) X(sub_826E8858) X(sub_826E2E68) X(sub_82667980) \
  X(sub_826C9AC0) X(sub_82634E90) X(sub_826B7F10) X(sub_826318B0) \
  X(sub_8268BF68) X(sub_826A8698) X(sub_826A7868) X(sub_826D26B8) \
  X(sub_826D2098) X(sub_826DA720) X(sub_82675738) X(sub_826B62A0) X(sub_826B5938)
#define DECLARE_LINK(name) REX_EXTERN(name); REX_EXTERN(__imp__##name);
LINK_FUNCTIONS(DECLARE_LINK)
#undef DECLARE_LINK

namespace {
constexpr uint32_t O = NativeHelpersTest::kObject;
constexpr uint32_t D = NativeHelpersTest::kData;
constexpr uint64_t H = 0x13579BDF00000000ull;
struct Function { const char* name; GuestFunction* original; GuestFunction* native; };
#define FUNCTION(name) {#name, __imp__##name, name}
const Function pops[] = {FUNCTION(sub_8263A170), FUNCTION(sub_82634E90), FUNCTION(sub_82675738)};
const Function erases[] = {FUNCTION(sub_8263A1D0), FUNCTION(sub_82636250), FUNCTION(sub_8268E900)};
const Function scans[] = {FUNCTION(sub_82803DE0), FUNCTION(sub_82803E48)};
const Function searches[] = {FUNCTION(sub_826C9AC0), FUNCTION(sub_826B62A0), FUNCTION(sub_826B5938)};
const Function handle = FUNCTION(sub_826DBB98);
const Function identifier = FUNCTION(sub_8263BF50);
const Function queue = FUNCTION(sub_826E8878);
const Function length = FUNCTION(sub_826E8858);
const Function key_search = FUNCTION(sub_826E2E68);
const Function any = FUNCTION(sub_82667980);
const Function unlink = FUNCTION(sub_826B7F10);
const Function indirect = FUNCTION(sub_826318B0);
const Function toggle = FUNCTION(sub_8268BF68);
const Function bound = FUNCTION(sub_826A8698);
const Function indexed = FUNCTION(sub_826A7868);
const Function packed_index = FUNCTION(sub_826D26B8);
const Function wrap = FUNCTION(sub_826D2098);
const Function packet = FUNCTION(sub_826DA720);
#undef FUNCTION

void Check(NativeHelpersTest& test, const Function& f, const GuestContext& ctx,
           std::initializer_list<NativeTestRegion> extra = {}) {
  // Each fixture rewrites both arenas before the original mutates either one.
  if (extra.size()) test.Check(f.name, f.original, f.native, ctx, extra);
  else test.Check(f.name, f.original, f.native, ctx, {{O, 2048}, {D, 16384}});
}
GuestContext Prepare(NativeHelpersTest& test, uint32_t seed) {
  auto ctx = test.Context(seed);
  ctx.r3.u64 = H | O;
  ctx.r4.u64 = H | D;
  test.Fill(O, 2048, seed);
  test.Fill(D, 16384, seed + 1);
  return ctx;
}

void Lists(NativeHelpersTest& test, uint32_t count, uint32_t selected,
           bool free_node, uint32_t seed) {
  for (unsigned suite = 0; suite < 6; ++suite) {
    const bool erase = suite >= 3;
    if (erase && !count) continue;
    auto ctx = Prepare(test, seed + suite);
    const uint32_t head_offset = suite % 3 == 0 ? 24 : (suite < 3 && suite % 3 == 2 ? 36 : 204);
    test.Store32(O, free_node ? D + 1024 : 0);
    test.Store32(O + head_offset, count ? D : 0);
    test.Store32(O + head_offset + 4, count ? D + (count - 1) * 32 : 0);
    test.Store32(O + head_offset + 8, count);
    for (uint32_t i = 0; i < count; ++i) {
      test.Store32(D + i * 32, 0xACED0000u + i);
      test.Store32(D + i * 32 + 4, i + 1 < count ? D + (i + 1) * 32 : 0);
      test.Store32(D + i * 32 + 8, i ? D + (i - 1) * 32 : 0);
    }
    if (erase) {
      ctx.r4.u64 = H | (D + selected * 32);
      Check(test, erases[suite - 3], ctx);
    } else Check(test, pops[suite], ctx);
  }
}

void Slots(NativeHelpersTest& test, bool first, uint32_t cursor, int found,
           unsigned alias, uint32_t seed) {
  auto ctx = Prepare(test, seed);
  for (uint32_t i = 0; i < 127; ++i) test.Store32(O + i * 4, 0);
  if (found >= 0) test.Store32(O + uint32_t(found) * 4, 0xCAFE0000u + found);
  test.Store8(O + 512, uint8_t(cursor));
  ctx.r4.u64 = H | (alias == 1 ? O + 8 : D + 16);
  ctx.r5.u64 = H | (alias == 2 ? O + 512 : (alias == 3 ? O + 4 : D));
  ctx.r6.u64 = H | (alias == 4 ? uint32_t(ctx.r5.u32) : D + 8);
  Check(test, scans[first ? 0 : 1], ctx);
}

void Handle(NativeHelpersTest& test, uint32_t slot, uint32_t count,
            uint32_t generation, bool matching_generation, bool matching_pool,
            bool overflow, uint32_t seed) {
  auto ctx = Prepare(test, seed);
  const uint32_t pool = seed & 1 ? 0x80000000u : 0;
  const uint32_t entries = overflow ? 0xFFFFFFF8u : D;
  ctx.r4.u64 = H | pool | (generation << 24) | slot;
  test.Store32(O + 20, matching_pool ? pool : pool ^ 0x80000000u);
  test.Store32(O, entries);
  test.Store32(O + 16, overflow ? 0xFFFFFFFFu : D + count * 8);
  if (!overflow) {
    for (uint32_t i = 0; i < 32; ++i) {
      test.Store32(D + i * 8, 0xDEAD0000u + i);
      test.Store8(D + i * 8 + 4, uint8_t(matching_generation ? generation : generation ^ 1));
    }
  }
  Check(test, handle, ctx);
}

void Identifier(NativeHelpersTest& test, uint16_t tag, int mismatch, uint32_t seed) {
  auto ctx = Prepare(test, seed);
  constexpr uint32_t signature = uint32_t(-2110268300);
  test.Fill(signature, 12, seed);
  test.Store16(O, tag);
  test.Store32(O + 24, 0x89ABCDEFu);
  for (uint32_t i = 0; i < 12; ++i) {
    const uint8_t value = uint8_t(seed + i * 37 + (i >> 4));
    test.Store8(O + 28 + i, int(i) == mismatch ? uint8_t(value ^ 0x80) : value);
  }
  Check(test, identifier, ctx, {{O, 2048}, {D, 16384}, {signature, 12}});
}

void Queue(NativeHelpersTest& test, uint32_t active_count, bool active_head,
           bool free_tail, uint32_t status, uint32_t seed) {
  auto ctx = Prepare(test, seed);
  const uint32_t node = active_head ? D : D + 256;
  ctx.r4.u64 = H | node;
  test.Store32(O, free_tail ? D + 512 : 0);
  test.Store32(O + 4, free_tail ? D + 512 : 0);
  test.Store32(O + 8, active_count ? D : 0);
  test.Store32(O + 12, active_count ? D + (active_count - 1) * 32 : 0);
  for (uint32_t i = 0; i < active_count; ++i) {
    test.Store32(D + i * 32, D + 1024 + i * 16);
    test.Store32(D + i * 32 + 4, i + 1 < active_count ? D + (i + 1) * 32 : 0);
  }
  test.Store32(node, D + 1024);
  if (!active_head) test.Store32(node + 4, 0);
  test.Store32(D + 1024, status);
  test.Store32(D + 512 + 4, 0);
  Check(test, queue, ctx);
}

void Length(NativeHelpersTest& test, uint32_t count, uint32_t seed) {
  auto ctx = Prepare(test, seed);
  test.Store32(O, count ? D : 0);
  for (uint32_t i = 0; i < count; ++i)
    test.Store32(D + i * 16 + 4, i + 1 < count ? D + (i + 1) * 16 : 0);
  Check(test, length, ctx);
}

void Search(NativeHelpersTest& test, uint32_t count, int found, uint32_t seed) {
  for (unsigned suite = 0; suite < 4; ++suite) {
    auto ctx = Prepare(test, seed + suite);
    const uint32_t stride = suite == 2 ? 24 : (suite == 3 ? 8 : 4);
    constexpr uint32_t key = 0x5ABC0000;
    ctx.r4.u64 = H | key;
    if (suite == 0) {
      test.Store32(O + 8, count);
      for (uint32_t i = 0; i < count; ++i) {
        const uint32_t item = D + i * 16;
        test.Store32(O + 96 + i * 4, item);
        test.Store64(item, int(i) == found ? ctx.r4.u64 : ctx.r4.u64 + i + 1);
      }
      Check(test, key_search, ctx);
    } else {
      if (suite == 1) { test.Store32(O, D); test.Store32(O + 8, count); }
      else { test.Store32(O, count); test.Store32(O + 4, D); }
      for (uint32_t i = 0; i < count; ++i)
        test.Store32(D + i * stride, int(i) == found ? key : key + i + 1);
      Check(test, searches[suite - 1], ctx);
    }
  }
}

void Any(NativeHelpersTest& test, int32_t count, int found, bool source,
         uint32_t enabled, uint32_t mode, bool alias, uint32_t seed) {
  auto ctx = Prepare(test, seed);
  const uint32_t array = alias ? O + 48 : D;
  ctx.r3.u64 = H | (source ? array : 0);
  ctx.r4.u64 = H | uint32_t(count);
  ctx.r5.u64 = H | O;
  ctx.r6.u64 = H | mode;
  test.Store32(O + 40, enabled);
  for (int32_t i = 0; i < count; ++i)
    test.Store32(array + uint32_t(i) * 4, i == found ? 0x80000000 : 0);
  Check(test, any, ctx);
}

void Unlink(NativeHelpersTest& test, uint32_t previous, uint32_t next, uint32_t seed) {
  auto ctx = Prepare(test, seed);
  test.Store32(O + 84, previous);
  test.Store32(O + 88, next);
  if (previous) test.Store32(previous + 88, O);
  if (next) test.Store32(next + 84, O);
  Check(test, unlink, ctx);
}

void Indirect(NativeHelpersTest& test, uint32_t id, uint32_t flags, uint32_t seed) {
  auto ctx = Prepare(test, seed);
  constexpr uint32_t global = uint32_t(-2103640064) - 3504u;
  ctx.r4.u64 = H | O;
  test.Store32(O + 228, flags);
  test.Store32(O + 56, id);
  test.Store32(global, D);
  Check(test, indirect, ctx, {{O, 2048}, {D, 16384}, {global, 4}});
}

void Toggle(NativeHelpersTest& test, uint32_t bit, bool alias, uint32_t seed) {
  auto ctx = Prepare(test, seed);
  ctx.r4.u64 = H | bit;
  test.Store32(O + 8, alias ? O : D);
  Check(test, toggle, ctx);
}

void Bound(NativeHelpersTest& test, uint32_t limit, uint32_t value, uint32_t seed) {
  auto ctx = Prepare(test, seed);
  ctx.r4.u64 = H | value;
  test.Store32(O + 8, limit);
  Check(test, bound, ctx);
}

void Indexed(NativeHelpersTest& test, uint16_t limit, uint32_t value,
             uint32_t destination, uint32_t seed) {
  auto ctx = Prepare(test, seed);
  ctx.r4.u64 = H | value;
  ctx.r5.u64 = H | destination;
  test.Store16(O + 24, limit);
  test.Store32(O + 64, D);
  Check(test, indexed, ctx);
}

void PackedIndex(NativeHelpersTest& test, uint32_t value, uint32_t origin,
                 uint16_t cursor, uint32_t seed) {
  auto ctx = Prepare(test, seed);
  ctx.r4.u64 = H | value;
  test.Store32(O + 904, origin);
  test.Store16(O + 908, cursor);
  Check(test, packed_index, ctx);
}

void Wrap(NativeHelpersTest& test, uint32_t previous, uint32_t value, uint32_t seed) {
  auto ctx = Prepare(test, seed);
  ctx.r3.u64 = H | previous;
  ctx.r4.u64 = H | value;
  Check(test, wrap, ctx);
}

void Packet(NativeHelpersTest& test, uint32_t source, uint32_t size,
            uint16_t header, uint32_t seed) {
  auto ctx = Prepare(test, seed);
  ctx.r4.u64 = H | source;
  ctx.r5.u64 = H | size;
  test.Store16(source, header);
  Check(test, packet, ctx);
}
}  // namespace

int main() {
  NativeHelpersTest test;
  uint32_t seed = 0;
  for (uint32_t count : {0u, 1u, 2u, 4u}) {
    for (uint32_t selected = 0; selected < (count ? count : 1); ++selected)
      for (bool free_node : {false, true}) Lists(test, count, selected, free_node, seed++);
    Length(test, count, seed++);
    for (int found = -1; found < int(count); ++found) Search(test, count, found, seed++);
  }
  for (bool first : {false, true})
    for (uint32_t cursor : {0u, 1u, 63u, 126u, 127u, 255u})
      for (int found : {-1, 0, 1, 63, 126})
        for (unsigned alias = 0; alias < 5; ++alias) Slots(test, first, cursor, found, alias, seed++);
  for (uint32_t count : {0u, 1u, 16u})
    for (uint32_t slot : {0u, 1u, 15u, 16u})
      for (uint32_t generation : {0u, 1u, 127u})
        for (bool match_generation : {false, true})
          for (bool match_pool : {false, true})
            Handle(test, slot, count, generation, match_generation, match_pool, false, seed++);
  Handle(test, 1, 1, 0, true, true, true, seed++);
  for (uint16_t tag : {uint16_t(0), uint16_t(65533), uint16_t(65534), uint16_t(65535)})
    for (int mismatch = -1; mismatch < 12; ++mismatch) Identifier(test, tag, mismatch, seed++);
  for (uint32_t active : {0u, 1u, 3u})
    for (bool is_head : {false, true}) {
      if (is_head && !active) continue;
      for (bool free_tail : {false, true})
        for (uint32_t status : {0u, 259u, 0xFFFFFFFFu}) Queue(test, active, is_head, free_tail, status, seed++);
    }
  for (int32_t count : {-1, 0, 1, 4})
    for (int found : {-1, 0, 3})
      for (bool source : {false, true})
        for (uint32_t enabled : {0u, 1u, 0x80000000u})
          for (uint32_t mode : {2u, 3u, 4u})
            for (bool alias : {false, true}) Any(test, count, found, source, enabled, mode, alias, seed++);
  for (const auto& neighbors : std::array<std::array<uint32_t, 2>, 6>{{
           {0, 0}, {0, D}, {D, 0}, {D, D + 256}, {O, O}, {D, D}}})
    Unlink(test, neighbors[0], neighbors[1], seed++);
  for (uint32_t id : {0u, 1u, 63u, 0x40000000u})
    for (uint32_t flags : {0u, 0x7Fu, 0x80u, 0xFFFFFFFFu}) Indirect(test, id, flags, seed++);
  for (uint32_t bit : {0u, 1u, 31u, 32u, 65535u, 0xFFFFFFFFu}) Toggle(test, bit, false, seed++);
  Toggle(test, 64, true, seed++);
  for (uint32_t limit : {0u, 1u, 0xFFFFFFFFu})
    for (uint32_t value : {0u, 1u, 2u, 0xFFFFFFFFu}) Bound(test, limit, value, seed++);
  for (uint32_t limit : {0u, 1u, 63u})
    for (uint32_t value : {0u, 1u, 63u, 64u, 0xFFFFu, 0x10000u})
      for (uint32_t destination : {O + 64, D + 4, D + 4096}) Indexed(test, limit, value, destination, seed++);
  for (uint32_t value : {0u, 7u, 8u, 15u, 16u, 17u, 65535u, 0xFFFFFFFFu})
    for (uint32_t origin : {0u, 65535u, 0xFFFFFFFFu}) PackedIndex(test, value, origin, uint16_t(seed), seed++);
  for (uint32_t previous : {0u, 511u, 512u, 65535u})
    for (uint32_t position : {0u, 255u, 256u, 511u, 512u, 767u, 768u, 1023u})
      for (uint32_t epoch : {0u, 0xFC00u}) Wrap(test, previous, epoch | position, seed++);
  for (uint32_t source : {D, O, O + 4})
    for (uint32_t size : {0u, 23u, 24u, 25u, 64u, 65537u})
      for (uint16_t header : {uint16_t(0), uint16_t(22), uint16_t(23), uint16_t(65535)})
        Packet(test, source, size, header, seed++);

  std::mt19937 random(0x8263A170);
  for (unsigned i = 0; i < 64; ++i) {
    const uint32_t count = random() % 17;
    Lists(test, count, count ? random() % count : 0, random() & 1, seed++);
    Length(test, count, seed++);
    Search(test, count, int(random() % (count + 1)) - 1, seed++);
    Slots(test, random() & 1, random() % 128, int(random() % 128) - 1, random() % 5, seed++);
    Handle(test, random() % 32, random() % 33, random() % 128, random() & 1, random() & 1, false, seed++);
    Identifier(test, 65534, int(random() % 13) - 1, seed++);
    Queue(test, 1 + random() % 4, random() & 1, random() & 1, random(), seed++);
    Any(test, count, int(random() % (count + 1)) - 1, true, random() & 1, 3, false, seed++);
    Unlink(test, D, D + 256, seed++);
    Indirect(test, random() % 64, random(), seed++);
    Toggle(test, random(), false, seed++);
    Bound(test, random(), random(), seed++);
    Indexed(test, 63, random() % 65, D + (random() % 64) * 4, seed++);
    PackedIndex(test, random(), random(), uint16_t(random()), seed++);
    Wrap(test, random(), random(), seed++);
    const uint32_t size = 24 + random() % 64;
    Packet(test, D, size, uint16_t(random() & 1 ? size - 2 : size), seed++);
  }
  test.Report("native links");
}
