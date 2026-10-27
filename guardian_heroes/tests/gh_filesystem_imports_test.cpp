#include "gh_filesystem_imports.h"

#include <array>
#include <cassert>
#include <cstdint>

int main() {
  std::array<uint8_t, 4096> memory{};
  memory[0x100] = 0xAA;
  memory[0x101] = 0xBB;
  memory[0x104] = 0xCC;

  assert(gh::filesystem::FlushGuestFile(memory.data(), 0x100) == 0);

  assert(memory[0x100] == 0 && memory[0x101] == 0 && memory[0x102] == 0 && memory[0x103] == 0);
  assert(memory[0x104] == 0 && memory[0x105] == 0 && memory[0x106] == 0 && memory[0x107] == 0);

  assert(gh::filesystem::FlushGuestFile(memory.data(), 0) == 0);
}
