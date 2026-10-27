#include "generated/default/guardian_heroes_pch.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <sys/mman.h>

REX_EXTERN(sub_828885C0);
REX_EXTERN(__imp__sub_828885C0);
REX_EXTERN(sub_82887388);
REX_EXTERN(__imp__sub_82887388);
REX_EXTERN(sub_82886F00);
REX_EXTERN(__imp__sub_82886F00);

namespace {
constexpr size_t kGuestSize = 0x100002000ull;
constexpr uint32_t kArena = 0x10000;
constexpr uint32_t kArenaSize = 32768;
size_t cases = 0;

struct Function {
  const char* name;
  GuestFunction* original;
  GuestFunction* native;
  bool fill;
};
const std::array<Function, 3> functions = {{
    {"memset", __imp__sub_828885C0, sub_828885C0, true},
    {"small memcpy", __imp__sub_82887388, sub_82887388, false},
    {"aligned memcpy", __imp__sub_82886F00, sub_82886F00, false},
}};

uint8_t* AllocateGuestMemory() {
  void* memory = mmap(nullptr, kGuestSize, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANON, -1, 0);
  if (memory == MAP_FAILED) {
    std::perror("mmap");
    std::exit(1);
  }
  return static_cast<uint8_t*>(memory);
}

uint8_t* Arena(uint8_t* base, uint32_t address) {
  return REX_RAW_ADDR(address);
}

void Check(const Function& function, uint8_t* reference_memory, uint8_t* native_memory,
           uint32_t destination, uint32_t source_or_fill, uint32_t size,
           uint32_t arena = kArena, uint32_t stack = kArena + 24576) {
  // Compare the entire arena, including bytes before and after the destination.
  auto* reference_arena = Arena(reference_memory, arena);
  auto* native_arena = Arena(native_memory, arena);
  for (uint32_t i = 0; i < kArenaSize; ++i) {
    reference_arena[i] = uint8_t(i * 37 + (i >> 5) + cases);
  }
  std::memcpy(native_arena, reference_arena, kArenaSize);
  GuestContext reference;
  std::memset(&reference, 0x5A + (cases % 3), sizeof(reference));
  reference.r1.u64 = stack;
  reference.r3.u64 = 0x1234567800000000ull | destination;
  reference.r4.u64 = 0x8765432100000000ull | source_or_fill;
  reference.r5.u64 = 0xABCDEF0100000000ull | size;
  reference.xer.so = cases & 1;
  reference.xer.ca = (cases >> 1) & 1;
  GuestContext native;
  std::memcpy(&native, &reference, sizeof(native));
  function.original(reference, reference_memory);
  function.native(native, native_memory);
  if (std::memcmp(&reference, &native, sizeof(reference)) ||
      std::memcmp(reference_arena, native_arena, kArenaSize)) {
    std::fprintf(stderr, "%s case %zu: dst=%08x src/value=%08x size=%u stack=%08x\n",
                 function.name, cases, destination, source_or_fill, size, stack);
    for (size_t i = 0; i < sizeof(reference); ++i) {
      if (reinterpret_cast<uint8_t*>(&reference)[i] != reinterpret_cast<uint8_t*>(&native)[i]) {
        std::fprintf(stderr, "context byte %zu: %02x / %02x\n", i,
                     reinterpret_cast<uint8_t*>(&reference)[i], reinterpret_cast<uint8_t*>(&native)[i]);
        break;
      }
    }
    for (uint32_t i = 0; i < kArenaSize; ++i) {
      if (reference_arena[i] != native_arena[i]) {
        std::fprintf(stderr, "memory +%x: %02x / %02x\n", i, reference_arena[i], native_arena[i]);
        break;
      }
    }
    std::exit(1);
  }
  ++cases;
}

double Time(const Function& function, GuestFunction* call, uint8_t* memory, uint32_t size) {
  GuestContext context{};
  context.r1.u64 = kArena + 24576;
  constexpr uint32_t iterations = 100000;
  const auto start = std::chrono::steady_clock::now();
  for (uint32_t i = 0; i < iterations; ++i) {
    context.r3.u64 = kArena + 8192;
    context.r4.u64 = function.fill ? 0xA5 : kArena + 256;
    context.r5.u64 = size;
    call(context, memory);
  }
  return std::chrono::duration<double, std::nano>(
             std::chrono::steady_clock::now() - start).count() / iterations;
}
}  // namespace

int main() {
  auto* reference = AllocateGuestMemory();
  auto* native = AllocateGuestMemory();
  for (const auto& function : functions) {
    const size_t first = cases;
    for (uint32_t destination_alignment = 0; destination_alignment < 16; ++destination_alignment) {
      for (uint32_t source_alignment = 0; source_alignment < 16; ++source_alignment) {
        for (uint32_t size = 0; size <= 260; ++size) {
          Check(function, reference, native, kArena + 8192 + destination_alignment,
                function.fill ? 0x11223300u + source_alignment * 17 : kArena + 256 + source_alignment,
                size);
        }
      }
    }
    // Cross block-count and tail boundaries on the large memcpy fast path.
    for (uint32_t offset : {0u, 128u, 256u}) {
      for (uint32_t size = 128; size < 2200; ++size) {
        Check(function, reference, native, kArena + 8192 + offset,
              function.fill ? 0xFFFFFFFF : kArena + 256, size);
      }
    }
    for (uint32_t size : {4095u, 4096u, 4097u, 8192u}) {
      Check(function, reference, native, kArena + 12288,
            function.fill ? 0x80000080 : kArena + 256, size);
    }
    if (!function.fill) {
      for (int delta : {-127, -8, -1, 0, 1, 8, 127}) {
        for (uint32_t size : {1u, 4u, 7u, 16u, 128u, 259u, 512u}) {
          Check(function, reference, native, kArena + 8192 + delta, kArena + 8192, size);
        }
      }
    }
    // The guest memcpy saves its destination in a stack slot which may alias a buffer.
    for (uint32_t stack : {kArena + 8192 + 8, kArena + 256 + 8}) {
      for (uint32_t size : {0u, 128u, 132u, 135u, 1025u}) {
        Check(function, reference, native, kArena + 8192,
              function.fill ? 0xFF : kArena + 256, size, kArena, stack);
      }
    }
    for (uint32_t arena : {0x7F000000u, 0x80000000u, 0xA0000000u, 0xE0001000u}) {
      for (uint32_t size : {0u, 1u, 7u, 128u, 132u, 1025u}) {
        Check(function, reference, native, arena + 8192,
              function.fill ? 0xFF : arena + 256, size, arena, arena + 24576);
      }
    }
    std::printf("PASS: %s, %zu cases, full PPC context and memory match\n", function.name, cases - first);
    for (uint32_t size : {16u, 64u, 256u, 4096u}) {
      std::array<double, 3> original_times{}, native_times{};
      for (size_t i = 0; i < original_times.size(); ++i) {
        original_times[i] = Time(function, function.original, reference, size);
        native_times[i] = Time(function, function.native, native, size);
      }
      std::sort(original_times.begin(), original_times.end());
      std::sort(native_times.begin(), native_times.end());
      std::printf("  %u bytes: original %.2f ns, native %.2f ns (%.2fx)\n", size,
                  original_times[1], native_times[1], original_times[1] / native_times[1]);
    }
  }
  munmap(reference, kGuestSize);
  munmap(native, kGuestSize);
}
