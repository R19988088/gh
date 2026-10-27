// On-console probe for the GH arena, independent of the game/runtime renderer.
#include "gh_guest_arena.h"
#include "gh_host_memory.h"
#include "guest_memory_switch.h"

#include <switch.h>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <sys/stat.h>

namespace {
FILE* log_file = nullptr;

void Log(const char* format, ...) {
  va_list arguments;
  va_start(arguments, format);
  va_list copy;
  va_copy(copy, arguments);
  std::vprintf(format, arguments);
  if (log_file) {
    std::vfprintf(log_file, format, copy);
    std::fflush(log_file);
  }
  va_end(copy);
  va_end(arguments);
}

bool Require(bool condition, const char* message) {
  Log("%s: %s (core=0x%08X)\n", condition ? "PASS" : "FAIL", message, RexGmLastResult());
  consoleUpdate(nullptr);
  return condition;
}

bool IsInaccessible(void* address) {
  MemoryInfo information{};
  u32 page = 0;
  return R_SUCCEEDED(svcQueryMemory(&information, &page, reinterpret_cast<u64>(address))) &&
         information.perm == Perm_None;
}

bool Probe(gh::memory::GuestArena& arena) {
  using namespace gh::memory;
  if (!Require(arena.Initialize(), "reserve arena")) {
    Log("%s\n", arena.error().c_str());
    return false;
  }
  if (!Require(RexGmSize() == 0x120000000ull && RexGmCommittedBytes() == 0,
               "4.5 GiB virtual reservation has zero backing")) return false;
  auto* base = arena.virtual_base();
  auto* physical = arena.physical_base();
  if (!Require(host::AllocFixed(physical, 0x2000, AllocationType::kCommit,
                               PageAccess::kReadWrite) == physical,
               "commit physical backing and map requested view")) return false;
  if (!Require(RexGmCommittedBytes() >= 0x2000 && RexGmCommittedBytes() <= 0x200000,
               "small commit uses at most one 2 MiB backing chunk")) return false;
  if (!Require(physical[0] == 0, "new backing is zeroed")) return false;
  const size_t mirrors[] = {0x7F000000, 0xA0000000, 0xC0000000, 0xE0000000};
  for (size_t mirror : mirrors) {
    if (!Require(RexGmFaultIn(reinterpret_cast<u64>(base + mirror)),
                 "explicitly map an alias view")) return false;
  }
  // Volatile forces accesses through the distinct virtual addresses.
  volatile uint8_t* bytes = base;
  volatile uint8_t* physical_bytes = physical;
  physical_bytes[0] = 0x41;
  if (!Require(bytes[0x7F000000] == 0x41 && bytes[0xA0000000] == 0x41 &&
               bytes[0xC0000000] == 0x41, "physical writes reach all unshifted aliases")) return false;
  bytes[0xA0000001] = 0x43;
  if (!Require(physical_bytes[1] == 0x43 && bytes[0xC0000001] == 0x43,
               "alias writes reach physical backing and another alias")) return false;
  physical_bytes[0x1000] = 0x47;
  if (!Require(bytes[0xE0000000] == 0x47, "E alias has exact 4 KiB backing displacement")) return false;
  bytes[0xE0000001] = 0x53;
  if (!Require(physical_bytes[0x1001] == 0x53, "shifted alias writes share backing")) return false;

  PageAccess old_access = PageAccess::kNoAccess;
  if (!Require(host::Protect(base + 0xA0000000, 0x1000, PageAccess::kNoAccess, &old_access) &&
               old_access == PageAccess::kReadWrite && IsInaccessible(base + 0xA0000000),
               "NONE unmaps one view without granting read access")) return false;
  // Do not deliberately fault: the probe has no exception recovery handler.
  if (!Require(physical_bytes[0] == 0x41 && bytes[0xC0000000] == 0x41,
               "protecting one alias preserves other aliases")) return false;
  if (!Require(host::Protect(base + 0xA0000000, 0x1000, PageAccess::kReadWrite) &&
               bytes[0xA0000000] == 0x41, "restore RW remaps the same bytes")) return false;
  errno = 0;
  if (!Require(!host::Protect(base + 0xA0000000, 0x1000, PageAccess::kReadOnly) && errno == ENOTSUP,
               "unsupported RO fails explicitly")) return false;
  bytes[0xA0000000] = 0x61;
  if (!Require(physical_bytes[0] == 0x61, "rejected RO leaves prior RW mapping intact")) return false;
  if (!Require(host::DeallocFixed(base + 0xA0000000, 0x1000, DeallocationType::kDecommit) &&
               IsInaccessible(base + 0xA0000000) && physical_bytes[0] == 0x61,
               "decommit removes view access while preserving shared backing")) return false;
  if (!Require(host::DeallocFixed(base + 0xC0000000, 0x1000, DeallocationType::kRelease) &&
               IsInaccessible(base + 0xC0000000) && physical_bytes[0] == 0x61,
               "release removes view access while preserving physical alias")) return false;
  arena.Close();
  if (!Require(!arena.virtual_base() && !RexGmBase() && RexGmCommittedBytes() == 0 &&
               RexGmMappedBytes() == 0, "close releases aliases, shadow and backing")) {
    Log("%s\n", arena.error().c_str());
    return false;
  }
  return true;
}
}  // namespace

int main() {
  consoleInit(nullptr);
  mkdir("sdmc:/switch", 0777);
  mkdir("sdmc:/switch/guardian-heroes", 0777);
  mkdir("sdmc:/switch/guardian-heroes/logs", 0777);
  log_file = std::fopen("sdmc:/switch/guardian-heroes/logs/memory-probe.log", "w");
  Log("Guardian Heroes Horizon memory probe\n");
  Log("Applet type=%u; real process handle=0x%X\n",
      static_cast<unsigned>(appletGetAppletType()), envGetOwnProcessHandle());
  if (!log_file) Log("SD log unavailable; console output remains visible.\n");
  gh::memory::GuestArena arena;
  const bool passed = Probe(arena);
  if (!passed) {
    arena.Close();
    if (arena.virtual_base()) Log("Cleanup retained backing: %s\n", arena.error().c_str());
  }
  Log("%s. Probe only; game runtime/exception handling is not exercised.\n",
      passed ? "Memory probe passed" : "Memory probe failed");
  Log("Application remains open.\n");
  while (appletMainLoop()) consoleUpdate(nullptr);
  if (log_file) std::fclose(log_file);
  consoleExit(nullptr);
  return passed ? 0 : 1;
}
