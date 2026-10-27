#include "generated/default/guardian_heroes_pch.h"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <random>
#include <sys/mman.h>

REX_EXTERN(sub_82498748);
REX_EXTERN(__imp__sub_82498748);

namespace {
constexpr size_t kGuestSize = 0x83000000;
constexpr uint32_t kScaleAddress = 0x8236CC10;
constexpr uint32_t kObjectAddress = 0x1000;
constexpr uint32_t kStackAddress = 0x2000;

uint8_t* AllocateGuestMemory() {
  void* memory = mmap(nullptr, kGuestSize, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANON, -1, 0);
  if (memory == MAP_FAILED) {
    std::perror("mmap");
    std::exit(1);
  }
  return static_cast<uint8_t*>(memory);
}

void Store32(uint8_t* base, uint32_t address, uint32_t value) {
  REX_STORE_U32(address, value);
}

void CheckCase(uint8_t* original_memory, uint8_t* native_memory,
               uint32_t numerator, uint32_t denominator, uint32_t offset,
               uint32_t multiplier, uint32_t scale, uint32_t rounding,
               size_t case_number) {
  GuestContext original{};
  std::memset(&original, 0x5A, sizeof(original));
  original.r1.u64 = kStackAddress;
  original.r3.u64 = kObjectAddress;
  original.r4.u64 = numerator | 0x1234567800000000ull;
  original.r5.u64 = offset | 0x9876543200000000ull;
  original.xer.so = case_number & 1;
  original.xer.ov = 1;
  original.xer.ca = 1;
  original.fpscr.InitHost();
  original.fpscr.storeFromGuest(rounding);
  original.fpscr.enableFlushMode();
  GuestContext native{};
  std::memcpy(&native, &original, sizeof(original));
  for (auto* memory : {original_memory, native_memory}) {
    std::memset(memory + kStackAddress - 32, 0xA5, 32);
    Store32(memory, kObjectAddress + 24, denominator);
    Store32(memory, kObjectAddress + 28, multiplier);
    Store32(memory, kScaleAddress, scale);
  }
  __imp__sub_82498748(original, original_memory);
  native.fpscr.setcsr(native.fpscr.csr);
  sub_82498748(native, native_memory);
  if (std::memcmp(&original, &native, sizeof(original)) ||
      std::memcmp(original_memory + kObjectAddress,
                  native_memory + kObjectAddress, 32) ||
      std::memcmp(original_memory + kStackAddress - 32,
                  native_memory + kStackAddress - 32, 32)) {
    std::fprintf(stderr,
        "case %zu mismatch: n=%08x d=%08x offset=%08x multiplier=%08x scale=%08x round=%u\n",
        case_number, numerator, denominator, offset, multiplier, scale, rounding);
    std::fprintf(stderr, "f1: %016llx / %016llx, f5: %016llx / %016llx, f7: %016llx / %016llx\n",
        original.f1.u64, native.f1.u64, original.f5.u64, native.f5.u64,
        original.f7.u64, native.f7.u64);
    for (size_t i = 0; i < sizeof(original); ++i) {
      if (reinterpret_cast<uint8_t*>(&original)[i] !=
          reinterpret_cast<uint8_t*>(&native)[i]) {
        std::fprintf(stderr, "first context byte mismatch: %zu\n", i);
        break;
      }
    }
    std::exit(1);
  }
}

double TimeCalls(GuestFunction* function, uint8_t* memory, uint32_t iterations) {
  GuestContext context{};
  context.r1.u64 = kStackAddress;
  context.r3.u64 = kObjectAddress;
  context.r4.u64 = 123456;
  context.r5.u64 = 65536;
  context.fpscr.InitHost();
  context.fpscr.storeFromGuest(0);
  Store32(memory, kObjectAddress + 24, 12345);
  Store32(memory, kObjectAddress + 28, std::bit_cast<uint32_t>(1.25f));
  Store32(memory, kScaleAddress, std::bit_cast<uint32_t>(1.0f / 65536.0f));
  const auto start = std::chrono::steady_clock::now();
  for (uint32_t i = 0; i < iterations; ++i) {
    function(context, memory);
  }
  const auto elapsed = std::chrono::steady_clock::now() - start;
  return std::chrono::duration<double, std::nano>(elapsed).count() / iterations;
}
}  // namespace

int main() {
  auto* original = AllocateGuestMemory();
  auto* native = AllocateGuestMemory();
  constexpr std::array<uint32_t, 10> integers = {
      0, 1, 0xFFFFFFFF, 0x7FFFFFFF, 0x80000000,
      65536, 0xFFFF0000, 0x00FFFFFF, 0x01000001, 0xFEDCBA98};
  constexpr std::array<uint32_t, 14> floats = {
      0, 0x80000000, 0x3F800000, 0xBF800000, 0x37800000,
      0x00000001, 0x007FFFFF, 0x00800000, 0x7F7FFFFF,
      0x7F800000, 0xFF800000, 0x7FC12345, 0x7FA12345, 0x3EAAAAAB};
  size_t cases = 0;
  for (uint32_t rounding = 0; rounding < 4; ++rounding) {
    for (auto numerator : integers) {
      for (auto denominator : integers) {
        for (auto multiplier : floats) {
          for (auto scale : floats) {
            CheckCase(original, native, numerator, denominator, 0, multiplier,
                      scale, rounding, cases++);
          }
        }
      }
    }
  }
  std::mt19937 random(0x82498748);
  for (size_t i = 0; i < 100000; ++i) {
    const uint32_t numerator = random(), denominator = random(), offset = random();
    const uint32_t multiplier = random(), scale = random();
    CheckCase(original, native, numerator, denominator, offset, multiplier,
              scale, i & 3, cases++);
  }
  std::printf("PASS: %zu cases, full PPC context and guest stack match\n", cases);
  constexpr uint32_t iterations = 2000000;
  std::array<double, 5> reference_times{}, native_times{};
  for (size_t i = 0; i < reference_times.size(); ++i) {
    reference_times[i] = TimeCalls(__imp__sub_82498748, original, iterations);
    native_times[i] = TimeCalls(sub_82498748, native, iterations);
  }
  std::sort(reference_times.begin(), reference_times.end());
  std::sort(native_times.begin(), native_times.end());
  std::printf("Median helper time: original %.2f ns, native %.2f ns, %.2fx\n",
      reference_times[2], native_times[2], reference_times[2] / native_times[2]);
  munmap(original, kGuestSize);
  munmap(native, kGuestSize);
}
