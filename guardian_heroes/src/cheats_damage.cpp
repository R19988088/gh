#include "generated/default/guardian_heroes_pch.h"
#include "cheats.h"

void GhKeepHealthMax(GuestRegister& character) {
  auto* base = rex::system::kernel_memory()->virtual_membase();
  gh::cheats::RefillHealth(base, character.u32);
}

void GhKeepSharedHealthMax(GuestRegister& character, GuestRegister& health) {
  auto* base = rex::system::kernel_memory()->virtual_membase();
  if (gh::cheats::RefillHealth(base, character.u32)) {
    health.u64 = REX_LOAD_U16(character.u32 + 202);
  }
}
