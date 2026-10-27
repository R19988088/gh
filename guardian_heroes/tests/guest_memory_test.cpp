#include "../src/native_memory.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <sys/mman.h>

namespace {
constexpr size_t kHostSize = 0x100002000ull;
struct MmioState {
  uint32_t value = 0;
  uint32_t loads = 0;
  uint32_t stores = 0;
};

uint32_t Load(void* opaque, uint32_t address) {
  auto& state = *static_cast<MmioState*>(opaque);
  ++state.loads;
  return state.value ^ address;
}
void Store(void* opaque, uint32_t address, uint32_t value) {
  auto& state = *static_cast<MmioState*>(opaque);
  ++state.stores;
  state.value = value ^ address;
}
}  // namespace

int main() {
  auto* base = static_cast<uint8_t*>(mmap(nullptr, kHostSize, PROT_READ | PROT_WRITE,
                                          MAP_PRIVATE | MAP_ANON, -1, 0));
  assert(base != MAP_FAILED);

  MmioState state{0xA5000000u};
  GuestMemory::MmioHandler handler{&state, Load, Store};
  GuestMemory memory(base, kHostSize, &handler);

  memory.Write8(0x200, 0x12);
  memory.Write16(0x201, 0x3456);
  memory.Write32(0x203, 0x789ABCDEu);
  memory.Write64(0x207, 0x0123456789ABCDEFull);
  assert(memory.Read8(0x200) == 0x12);
  assert(memory.Read16(0x201) == 0x3456);
  assert(memory.Read32(0x203) == 0x789ABCDEu);
  assert(memory.Read64(0x207) == 0x0123456789ABCDEFull);

  // The physical alias has the same byte order and a 4 KiB host adjustment.
  memory.Write32(0xE0000000u, 0xCAFEBABEu);
  assert(memory.Read32(0xE0000000u) == 0xCAFEBABEu);
  assert(memory.Address(0xE0000000u) == base + uint64_t(0xE0000000u) + GuestMemory::HostOffset(0xE0000000u));
  assert(memory.Contains(0xFFFFFFFFu, 1));
  assert(!memory.Contains(0xFFFFFFFFu, 2));
  assert(!memory.Contains(0xFFFFFFFEu, 4));
  assert(memory.Contains(0x7EFFFFFFu, 1));
  assert(memory.Contains(0x7EFFFFFEu, 2));
  assert(memory.Address(0xFFFFFFFFu, 1) ==
         base + uint64_t(0xFFFFFFFFu) + GuestMemory::HostOffset(0xFFFFFFFFu));
  bool span_threw = false;
  try {
    (void)memory.Address(0xFFFFFFFFu, 2);
  } catch (const std::out_of_range&) {
    span_threw = true;
  }
  assert(span_threw);
  GuestMemory bounded_memory(base, 0x1000, &handler);
  assert(bounded_memory.Address(0xFFFu, 1) == base + 0xFFFu);
  span_threw = false;
  try {
    (void)bounded_memory.Address(0xFFFu, 2);
  } catch (const std::out_of_range&) {
    span_threw = true;
  }
  assert(span_threw);

  const uint32_t before = state.loads;
  assert(memory.Read32(0x7F000100u) == (state.value ^ 0x7F000100u));
  assert(state.loads == before + 1);
  assert(memory.Read8(0x7F000100u) == static_cast<uint8_t>(state.value ^ 0x7F000100u));
  assert(state.loads == before + 2);
  assert(memory.Read16(0x7F000100u) == static_cast<uint16_t>(state.value ^ 0x7F000100u));
  assert(state.loads == before + 3);
  const auto stores_before = state.stores;
  memory.WriteMmio8(0x7F000120u, 0x12345678u);
  assert(state.value == (0x12345678u ^ 0x7F000120u));
  memory.WriteMmio16(0x7F000124u, 0x89ABCDEFu);
  assert(state.value == (0x89ABCDEFu ^ 0x7F000124u));
  assert(state.stores == stores_before + 2);
  memory.WriteRam32(0x7F000140u, 0x55667788u);
  const auto loads_before_direct = state.loads;
  assert(memory.ReadRam32(0x7F000140u) == 0x55667788u);
  assert(state.loads == loads_before_direct);  // REX_LOAD bypasses the MMIO table.
  memory.Write64(0x7F000200u, 0x1122334455667788ull);
  assert(state.stores == stores_before + 4);  // 64-bit MMIO is two 32-bit transactions.

  // Generated code may construct a view without passing an explicit handler;
  // the runtime adapter supplies the process-wide MMIO table in that case.
  GuestMemory::SetGlobalMmioHandler(&handler);
  GuestMemory global_memory = GuestMemory::WithGlobalMmio(base);
  const uint32_t global_before = state.loads;
  (void)global_memory.Read32(0x7F000300u);
  assert(state.loads == global_before + 1);
  GuestMemory::SetGlobalMmioHandler(nullptr);

  bool threw = false;
  try {
    (void)memory.Read16(0xFFFFFFFFu);
  } catch (const std::out_of_range&) {
    threw = true;
  }
  assert(threw);
  munmap(base, kHostSize);
  std::puts("PASS: guest memory mapping, endian access, MMIO, aliases and bounds");
}
