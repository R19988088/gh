#include "gh_guest_arena.h"

#include <cassert>
#include <cstdio>

#if defined(_WIN32)
#include <windows.h>
#endif

namespace {
void Commit(uint8_t* address, size_t size) {
#if defined(_WIN32)
  void* committed = VirtualAlloc(address, size, MEM_COMMIT, PAGE_READWRITE);
  assert(committed == address);
#else
  (void)address;
  (void)size;
#endif
}

void CheckAliases(gh::memory::GuestArena& arena) {
  auto* base = arena.virtual_base();
  auto* physical = arena.physical_base();
  assert(base && physical == base + 0x100000000ull);
  const size_t granularity = arena.allocation_granularity();
  const size_t physical_4k_adjustment = granularity > 0x1000 ? 0x1000 : 0;
  Commit(base, granularity);
  Commit(base + 0x40000000, granularity);
  Commit(base + 0x80000000, granularity);
  Commit(physical, granularity * 2);
  base[0] = 0x17;
  base[0x40000000] = 0x29;
  base[0x80000000] = 0x31;
  assert(base[0x90000000] == 0x31);
  base[0x90000001] = 0x37;
  assert(base[0x80000001] == 0x37);
  physical[0] = 0x41;
  assert(base[0x7F000000] == 0x41);
  assert(base[0xA0000000] == 0x41);
  assert(base[0xC0000000] == 0x41);
  base[0xA0000001] = 0x43;
  assert(physical[1] == 0x43 && base[0xC0000001] == 0x43);
  physical[0x1000] = 0x47;
  assert(base[0xE0000000ull + physical_4k_adjustment] == 0x47);
  base[0xE0000001ull + physical_4k_adjustment] = 0x53;
  assert(physical[0x1001] == 0x53);
  assert(base[0] == 0x17 && base[0x40000000] == 0x29);
  assert(base[0x80000000] == 0x31);
}
}  // namespace

int main() {
  gh::memory::GuestArena arena;
  for (int cycle = 0; cycle < 2; ++cycle) {
    if (!arena.Initialize()) {
      std::fprintf(stderr, "%s\n", arena.error().c_str());
      return 1;
    }
    // A fresh backing must not retain bytes from the previous arena lifetime.
    Commit(arena.physical_base(), arena.allocation_granularity());
    assert(arena.physical_base()[0] == 0);
    CheckAliases(arena);
    arena.Close();
    assert(!arena.virtual_base() && !arena.physical_base());
    arena.Close();
  }
  // Two live instances keep backing identities and data separate.
  gh::memory::GuestArena other;
  assert(arena.Initialize());
  assert(other.Initialize());
  assert(arena.name() != other.name());
  CheckAliases(arena);
  Commit(other.physical_base(), other.allocation_granularity());
  assert(other.physical_base()[0] == 0);
  std::puts("Guest arena: aliases, physical 4K offset, lifetime and isolation passed");
}
