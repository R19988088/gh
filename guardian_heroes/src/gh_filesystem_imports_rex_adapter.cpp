#include "generated/default/guardian_heroes_pch.h"

#include "gh_filesystem_imports.h"

// The SDK implements NtFlushBuffersFile as a no-op with an optional guest IO
// status block; this entry preserves that behavior without runtime file APIs.
extern "C" void __imp__NtFlushBuffersFile(GuestContext& context, uint8_t* base) {
  context.r3.u32 = gh::filesystem::FlushGuestFile(base, context.r4.u32);
}
