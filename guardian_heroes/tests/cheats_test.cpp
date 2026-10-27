// Native menu/storage fixtures; the builder and action predicates are real guest code.
#include "../src/cheats.cpp"
REX_EXTERN(sub_824A51C0);
REX_EXTERN(sub_824A5880);
REX_EXTERN(sub_824A8488);
REX_EXTERN(sub_824A58C8);
#include <cassert>
#include <cstdio>
#include <set>
#include <sys/mman.h>

#undef REX_LOOKUP_FUNC
#define REX_LOOKUP_FUNC(x, y) \
  (*reinterpret_cast<GuestFunction**>(x + REX_IMAGE_BASE + REX_IMAGE_SIZE + \
                                      (u64(u32(y) - REX_CODE_BASE) * 2)))

namespace {
uint32_t heap = 0x100000;
std::set<uint32_t> allocations;
bool chinese = true;
int original_actions = 0;
int layouts = 0;
uint32_t Allocate(uint32_t bytes) { const auto p = heap; heap += (bytes + 15) & ~15; return p; }
uint32_t StringData(uint8_t* base, uint32_t s) {
  return REX_LOAD_U32(s + 20) < 8 ? s : REX_LOAD_U32(s);
}
std::u16string ReadString(uint8_t* base, uint32_t s) {
  std::u16string result;
  for (uint32_t i = 0; i < REX_LOAD_U32(s + 16); ++i) result += REX_LOAD_U16(StringData(base, s) + i * 2);
  return result;
}
void WriteString(uint8_t* base, uint32_t s, std::u16string_view text) {
  if (REX_LOAD_U32(s + 20) >= 8) assert(allocations.erase(REX_LOAD_U32(s)) == 1);
  uint32_t data = s;
  if (text.size() >= 8) {
    data = Allocate((text.size() + 1) * 2);
    allocations.insert(data);
    REX_STORE_U32(s, data);
  }
  REX_STORE_U32(s + 16, text.size());
  REX_STORE_U32(s + 20, text.size() < 8 ? 7 : text.size());
  for (size_t i = 0; i < text.size(); ++i) REX_STORE_U16(data + i * 2, text[i]);
  REX_STORE_U16(data + text.size() * 2, 0);
}
std::u16string SelectedLabel(uint8_t* base, uint32_t menu, uint32_t index) {
  const uint32_t mapping = REX_LOAD_U32(menu + 1680);
  return ReadString(base, REX_LOAD_U32(menu + 1664) + REX_LOAD_U32(mapping + index * 4) * 80);
}
void Call(GuestFunction* function, uint8_t* base, uint32_t object) {
  GuestContext context{};
  context.r1.u32 = 0x90000;
  context.r3.u32 = object;
  function(context, base);
}
void Confirm(uint8_t* base, uint32_t scene, uint32_t selected) {
  REX_STORE_U32(scene + 9312 + 1180, selected);
  const uint32_t menu = scene + 9312;
  REX_STORE_U32(menu + 60, 0x1000);
  Call(sub_824A5880, base, menu);
  assert(REX_LOAD_U8(menu + 1186) == 1);
  const bool toggle = cheat_page && selected < switches.size();
  const int previous_layouts = layouts;
  const uint32_t animation = REX_LOAD_U32(menu + 1208);
  Call(sub_824D67D8, base, scene);
  if (toggle) {
    assert(layouts == previous_layouts);
    assert(REX_LOAD_U32(menu + 1208) == animation);
  }
  if (cheat_page) {
    assert(REX_LOAD_U8(menu + 1186) == 0);
    assert(REX_LOAD_U8(menu + 1188) == 1);
    assert(REX_LOAD_U16(menu + 6) != 4);
  }
}
}  // namespace

extern "C" REX_FUNC(sub_82496E58) { ctx.r3.u32 = 0; }
extern "C" REX_FUNC(sub_82495BC0) {
  for (uint32_t i = 0; i < 6; ++i) REX_STORE_U8(ctx.r3.u32 + i, 2);
}
extern "C" REX_FUNC(__imp__sub_82468FA8) {
  assert(REX_LOAD_U8(ctx.r3.u32 + 226) == 2);
  REX_STORE_U8(ctx.r3.u32 + 226, 3);
}
extern "C" REX_FUNC(__imp__sub_824418C8) {}
extern "C" REX_FUNC(__imp__sub_824429A8) {}
extern "C" REX_FUNC(__imp__sub_82441248) {}

namespace rex::runtime {
  PPCFunc* ResolveIndirectFunction(uint32_t) { assert(false); return nullptr; }
}
extern "C" REX_FUNC(MenuEnabled) { ctx.r3.u32 = 1; }

namespace gh::localization { bool IsChinese() { return chinese; } }
extern "C" REX_FUNC(sub_82495B30) { ctx.r3.u32 = kGame; }
extern "C" REX_FUNC(sub_8242E838) { ctx.r3.u32 = 0; }
extern "C" REX_FUNC(sub_82424CE8) {}
extern "C" REX_FUNC(sub_824404B0) { ctx.r3 = ctx.r4; }
extern "C" REX_FUNC(sub_824AA710) {
  std::memset(base + ctx.r3.u32, 0, 80);
  REX_STORE_U32(ctx.r3.u32 + 20, 7);
  REX_STORE_U32(ctx.r3.u32 + 48, 7);
}
extern "C" REX_FUNC(sub_82417CF0) {
  std::u16string text;
  for (uint32_t i = 0; i < ctx.r5.u32; ++i) text += REX_LOAD_U16(ctx.r4.u32 + i * 2);
  WriteString(base, ctx.r3.u32, text);
}
extern "C" REX_FUNC(sub_82886C30) { assert(allocations.erase(ctx.r3.u32) == 1); }
extern "C" REX_FUNC(sub_82887F88) {
  uint32_t size = 0;
  while (REX_LOAD_U16(ctx.r3.u32 + size * 2)) ++size;
  ctx.r3.u32 = size;
}
extern "C" REX_FUNC(sub_824B3248) {
  const uint32_t vector = ctx.r3.u32, source = ctx.r4.u32;
  const uint32_t row = REX_LOAD_U32(vector + 4);
  const auto label = ReadString(base, source), description = ReadString(base, source + 28);
  std::memcpy(base + row, base + source, 80);
  REX_STORE_U32(row + 20, 7);
  REX_STORE_U32(row + 48, 7);
  WriteString(base, row, label);
  WriteString(base, row + 28, description);
  REX_STORE_U32(vector + 4, row + 80);
}
extern "C" REX_FUNC(sub_824AAE10) {
  ++layouts;
  const uint32_t menu = ctx.r3.u32, mapping = REX_LOAD_U32(menu + 1680);
  uint32_t count = 0, index = 0;
  for (uint32_t row = REX_LOAD_U32(menu + 1664); row < REX_LOAD_U32(menu + 1668); row += 80, ++index) {
    if (REX_LOAD_U32(row + 72) != 3) {
      REX_STORE_U32(mapping + count * 4, index);
      ++count;
    }
  }
  REX_STORE_U32(menu + 1684, mapping + count * 4);
  REX_STORE_U32(menu + 1700, count);
  const float scale = std::bit_cast<float>(REX_LOAD_U32(menu + 256));
  REX_STORE_U32(menu + 1216, std::bit_cast<uint32_t>((count + 1) * 56.0f * scale));
}
#define LABEL_STUB(name) extern "C" REX_FUNC(name) { \
  const uint32_t text = 0x80000; \
  REX_STORE_U16(text, u'A' + ctx.r3.u32); REX_STORE_U16(text + 2, 0); ctx.r3.u32 = text; }
LABEL_STUB(sub_824CA510)
LABEL_STUB(sub_824CA568)
LABEL_STUB(sub_824CA5C0)
LABEL_STUB(sub_824CA618)
LABEL_STUB(sub_824CC050)
LABEL_STUB(sub_824CC0A8)
LABEL_STUB(sub_824CC100)
LABEL_STUB(sub_824CC158)
extern "C" REX_FUNC(__imp__sub_824D67D8) {
  if (REX_LOAD_U8(ctx.r3.u32 + 9312 + 1186)) ++original_actions;
}
extern "C" REX_FUNC(__imp__sub_824DEA90) {}
extern "C" REX_FUNC(__imp__sub_82478988) {
  REX_STORE_U16(ctx.r3.u32 + 202, REX_LOAD_U16(ctx.r3.u32 + 202) - 1);
  REX_STORE_U16(ctx.r3.u32 + 206, 0);
}
extern "C" REX_FUNC(__imp__sub_824E4308) {
  assert(REX_LOAD_U8(kGame + 942) > 0);
  REX_STORE_U8(kGame + 942, REX_LOAD_U8(kGame + 942) - 1);
}

int main() {
  auto* base = static_cast<uint8_t*>(mmap(nullptr, 0x83000000, PROT_READ | PROT_WRITE,
      MAP_PRIVATE | MAP_ANON, -1, 0));
  assert(base != MAP_FAILED);
  constexpr uint32_t scene = 0x10000, menu = scene + 9312;
  constexpr uint32_t vtable = 0x41000;
  REX_STORE_U32(menu, vtable);
  REX_STORE_U32(menu + 252, std::bit_cast<uint32_t>(1.0f));
  REX_STORE_U32(menu + 256, std::bit_cast<uint32_t>(1.0f));
  guest::RegisterFunction<GuestContext>(0x82410000, MenuEnabled);
  guest::RegisterFunction<GuestContext>(0x824A5880, sub_824A5880);
  guest::RegisterFunction<GuestContext>(0x824A58C8, sub_824A58C8);
  REX_LOOKUP_FUNC(base, 0x82410000) = MenuEnabled;
  REX_LOOKUP_FUNC(base, 0x824A5880) = sub_824A5880;
  REX_LOOKUP_FUNC(base, 0x824A58C8) = sub_824A58C8;
  for (uint32_t offset : {36u, 72u, 76u}) REX_STORE_U32(vtable + offset, 0x82410000);
  REX_STORE_U32(vtable + 32, 0x824A5880);
  REX_STORE_U32(vtable + 68, 0x824A58C8);
  REX_STORE_U32(menu + 1196, 0x2000);
  REX_STORE_U32(menu + 1664, 0x30000);
  REX_STORE_U32(menu + 1668, 0x30000);
  REX_STORE_U32(menu + 1680, 0x40000);
  REX_STORE_U8(scene, 1);
  REX_STORE_U16(scene + 2504, 5);
  REX_STORE_U16(menu + 4, 3);
  REX_STORE_U8(menu + 1188, 1);
  REX_STORE_U32(menu + 1192, 0x1000);
  const std::array<GuestFunction*, 6> original = {__imp__sub_824BFF00, __imp__sub_824BFF78,
      __imp__sub_824BFFE8, __imp__sub_824C0058, __imp__sub_824C00C8, __imp__sub_824C0158};
  const std::array<GuestFunction*, 6> hooked = {sub_824BFF00, sub_824BFF78,
      sub_824BFFE8, sub_824C0058, sub_824C00C8, sub_824C0158};
  for (uint32_t mode = 0; mode < 4; ++mode) {
    REX_STORE_U8(kGame + 940, mode);
    auto call = GuestContext{}; call.r1.u32 = 0x90000;
    gh::cheats::RebuildPauseMenu(call, base, menu);
    const uint32_t count = mode == 2 ? 8 : mode == 1 ? 7 : 6;
    assert((REX_LOAD_U32(menu + 1684) - REX_LOAD_U32(menu + 1680)) / 4 == count + 1);
    assert(SelectedLabel(base, menu, 2) == u"作弊");
    for (uint32_t index = 0; index < count; ++index) {
      const uint32_t displayed = index < 2 ? index : index + 1;
      assert(SelectedLabel(base, menu, displayed) == std::u16string(1, u'A' + index));
      for (size_t action = 0; action < original.size(); ++action) {
        GuestContext reference{}; reference.r1.u32 = 0x90000; reference.r3.u32 = menu;
        REX_STORE_U32(menu + 1180, index); original[action](reference, base);
        GuestContext actual{}; actual.r1.u32 = 0x90000; actual.r3.u32 = menu;
        REX_STORE_U32(menu + 1180, displayed); hooked[action](actual, base);
        assert(reference.r3.u8 == actual.r3.u8);
        assert(REX_LOAD_U32(menu + 1180) == displayed);
      }
    }
  }
  for (uint32_t i = 0; i < switches.size(); ++i) {
    assert(switches[i] == (i < 4));
  }
  Confirm(base, scene, 2);
  for (uint32_t index = 0; index < switches.size(); ++index) {
    assert(SelectedLabel(base, menu, index).ends_with(index < 4 ? u"开启" : u"关闭"));
  }
  assert(REX_LOAD_U32(menu + 1700) == switches.size() + 1);
  assert(SelectedLabel(base, menu, 4) == u"力量 max    关闭");
  assert(SelectedLabel(base, menu, 9) == u"幸运 max    关闭");
  assert(std::bit_cast<float>(REX_LOAD_U32(menu + 1216)) <= 621.0f);
  switches.fill(false);
  // Original native navigation still moves in both directions after entering.
  REX_STORE_U32(menu + 60, 2); Call(sub_824A8488, base, menu);
  assert(REX_LOAD_U32(menu + 1180) == 1);
  REX_STORE_U32(menu + 60, 1); Call(sub_824A8488, base, menu);
  assert(REX_LOAD_U32(menu + 1180) == 0);
  REX_STORE_U32(menu + 60, 0);
  constexpr uint32_t player = 0x50000, enemy = 0x51000, node = 0x52000;
  REX_STORE_U32(kGame + 1676, node); REX_STORE_U32(node, player);
  REX_STORE_U8(player + 1166, 1); REX_STORE_U8(enemy + 1166, 1);
  REX_STORE_U16(player + 204, 300); REX_STORE_U16(player + 208, 120);
  REX_STORE_U16(player + 202, 10); REX_STORE_U16(player + 206, 5);
  // A child may be selected with native horizontal input while the master is off.
  REX_STORE_U32(menu + 1180, 1);
  REX_STORE_U8(menu + 1188, 1); REX_STORE_U32(menu + 60, 0x40);
  Call(sub_824D67D8, base, scene);
  assert(REX_LOAD_U8(menu + 1188) == 1);
  REX_STORE_U32(menu + 60, 0);
  assert(!gh::cheats::RefillHealth(base, player));
  Confirm(base, scene, 0);
  assert(REX_LOAD_U16(player + 202) == 300);
  Call(sub_82478988, base, player);
  assert(REX_LOAD_U16(player + 202) == 300 && REX_LOAD_U16(player + 206) == 0);
  assert(!gh::cheats::RefillHealth(base, enemy));
  REX_STORE_U16(player + 202, 0);  // Restore before the original lethal-hit check.
  assert(gh::cheats::RefillHealth(base, player));
  GuestContext cost{}; cost.r1.u32 = 0x90000; cost.r3.u32 = player; cost.r4.u32 = 300;
  sub_824411C0(cost, base);
  assert(cost.r3.u32 == 0 && REX_LOAD_U16(player + 202) == 300);
  Confirm(base, scene, 1); Confirm(base, scene, 2);
  assert(REX_LOAD_U16(player + 206) == 120);
  Call(sub_82478988, base, player);
  assert(REX_LOAD_U16(player + 202) == 299 && REX_LOAD_U16(player + 206) == 120);
  REX_STORE_U8(kGame + 942, 3); Call(sub_824DEA90, base, scene);
  assert(REX_LOAD_U8(kGame + 942) == 3);
  Confirm(base, scene, 3); assert(REX_LOAD_U8(kGame + 942) == 99);
  REX_STORE_U8(kGame + 942, 0);
  Call(sub_824E4308, base, 0); assert(REX_LOAD_U8(kGame + 942) == 99);
  Confirm(base, scene, 0); REX_STORE_U8(kGame + 942, 7);
  Call(sub_824DEA90, base, scene); assert(REX_LOAD_U8(kGame + 942) == 7);
  assert(SelectedLabel(base, menu, 2).ends_with(u"开启"));
  assert(SelectedLabel(base, menu, 3).ends_with(u"开启"));
  // All six attribute switches start off and affect only their own attribute.
  constexpr uint32_t resource = 0x54000;
  REX_STORE_U32(player + 168, resource);
  REX_STORE_U8(player + 225, 10);
  for (uint32_t i = 0; i < 6; ++i) {
    REX_STORE_U8(player + 226 + i, i + 2);
    REX_STORE_U8(resource + 14 + i, i + 1);
  }
  for (uint32_t i = 0; i < 6; ++i) assert(SelectedLabel(base, menu, i + 4).ends_with(u"关闭"));
  for (uint32_t i = 0; i < 6; ++i) {
    Confirm(base, scene, i + 4);  // Master is off.
    assert(REX_LOAD_U8(player + 226 + i) == i + 2);
    switches[0] = true; Call(sub_824DEA90, base, scene);
    for (uint32_t j = 0; j < 6; ++j) assert(REX_LOAD_U8(player + 226 + j) == (j == i ? 32 : j + 2));
    if (i == 1) assert(REX_LOAD_U16(player + 204) == 740);
    if (i == 2) assert(REX_LOAD_U16(player + 288) == 138);
    if (i == 3) assert(REX_LOAD_U16(player + 208) == 740);
    Confirm(base, scene, i + 4);
    assert(REX_LOAD_U8(player + 226 + i) == i + 2);
    switches[0] = false;
  }
  // Mode-specific native max is base+32, including the custom-character bonus.
  REX_STORE_U8(kGame + 1177, 2); REX_STORE_U8(player + 84, 7);
  switches[0] = true;
  for (uint32_t i = 0; i < 6; ++i) Confirm(base, scene, i + 4);
  for (uint32_t i = 0; i < 6; ++i) assert(REX_LOAD_U8(player + 226 + i) == 35 + i);
  assert(REX_LOAD_U16(player + 208) == 100);
  switches[0] = false; Call(sub_824DEA90, base, scene);
  for (uint32_t i = 0; i < 6; ++i) assert(REX_LOAD_U8(player + 226 + i) == i + 2);
  switches[0] = true; Call(sub_824DEA90, base, scene);
  Call(sub_82468FA8, base, player); // Rebuild receives normal values; keeps growth.
  Call(sub_824DEA90, base, scene);
  switches[0] = false; Call(sub_824DEA90, base, scene);
  assert(REX_LOAD_U8(player + 226) == 3);
  Confirm(base, scene, 10); assert(SelectedLabel(base, menu, 2) == u"作弊");
  assert(REX_LOAD_U32(menu + 252) == std::bit_cast<uint32_t>(1.0f));
  assert(REX_LOAD_U32(menu + 256) == std::bit_cast<uint32_t>(1.0f));
  Confirm(base, scene, 3); assert(original_actions == 1);
  REX_STORE_U8(menu + 1186, 0); Confirm(base, scene, 2);
  // The cheat page handles a raw B event even before the native predicate sets +1184.
  REX_STORE_U8(menu + 1184, 0); REX_STORE_U32(menu + 60, REX_LOAD_U32(menu + 1196));
  Call(sub_824D67D8, base, scene);
  assert(!cheat_page && REX_LOAD_U32(menu + 1180) == 2);
  REX_STORE_U8(menu + 1186, 0); Confirm(base, scene, 2);
  REX_STORE_U32(menu + 60, REX_LOAD_U32(menu + 1196)); Call(sub_824A58C8, base, menu);
  assert(REX_LOAD_U8(menu + 1184) == 1); Call(sub_824D67D8, base, scene);
  assert(!cheat_page && REX_LOAD_U32(menu + 1180) == 2);
  chinese = false;
  auto call = GuestContext{}; call.r1.u32 = 0x90000;
  gh::cheats::RebuildPauseMenu(call, base, menu);
  assert(SelectedLabel(base, menu, 2) == u"Cheats");
  ClearRows(call, base, menu); assert(allocations.empty());
  std::puts("PASS: launch defaults, independent toggles, master gate, player vitals/credits, six reversible attributes, single cheat page, immediate effects, stable layout/animation on toggles, native input gate/navigation/confirmation/back, menu indices and labels in all 4 modes");
  munmap(base, 0x83000000);
}
