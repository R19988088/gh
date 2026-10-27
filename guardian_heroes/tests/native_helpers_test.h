#pragma once

#include "generated/default/guardian_heroes_pch.h"

#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <sys/mman.h>

struct NativeTestRegion { uint32_t address; size_t size; };

class NativeHelpersTest {
 public:
  static constexpr uint32_t kObject = 0x10000;
  static constexpr uint32_t kData = 0x20000;
  static constexpr uint32_t kStack = 0x40000;

  NativeHelpersTest() : original_(Allocate()), native_(Allocate()) {}
  ~NativeHelpersTest() {
    munmap(original_, kGuestSize);
    munmap(native_, kGuestSize);
  }

  GuestContext Context(uint32_t seed) {
    GuestContext context;
    std::memset(&context, 0x5A + seed % 3, sizeof(context));
    context.r1.u64 = kStack;
    context.xer.so = seed & 1;
    context.xer.ov = (seed >> 1) & 1;
    context.xer.ca = (seed >> 2) & 1;
    Fill(kStack - 256, 512, seed);
    return context;
  }

  void Fill(uint32_t address, size_t size, uint32_t seed) {
    auto* destination = Raw(original_, address);
    for (size_t i = 0; i < size; ++i) destination[i] = uint8_t(seed + i * 37 + (i >> 4));
    std::memcpy(Raw(native_, address), destination, size);
  }

  void Store8(uint32_t address, uint8_t value) {
    for (auto* base : {original_, native_}) REX_STORE_U8(address, value);
  }
  void Store16(uint32_t address, uint16_t value) {
    for (auto* base : {original_, native_}) REX_STORE_U16(address, value);
  }
  void Store32(uint32_t address, uint32_t value) {
    for (auto* base : {original_, native_}) REX_STORE_U32(address, value);
  }
  void Store64(uint32_t address, uint64_t value) {
    for (auto* base : {original_, native_}) REX_STORE_U64(address, value);
  }

  void Check(const char* name, GuestFunction* original, GuestFunction* native,
             const GuestContext& input, std::initializer_list<NativeTestRegion> regions) {
    GuestContext reference, result;
    std::memcpy(&reference, &input, sizeof(input));
    std::memcpy(&result, &input, sizeof(input));
    original(reference, original_);
    native(result, native_);
    if (std::memcmp(&reference, &result, sizeof(result))) {
      for (size_t i = 0; i < sizeof(result); ++i) {
        if (reinterpret_cast<uint8_t*>(&reference)[i] != reinterpret_cast<uint8_t*>(&result)[i]) {
          std::fprintf(stderr, "%s case %zu: context byte %zu, %02x / %02x\n", name, cases_, i,
                       reinterpret_cast<uint8_t*>(&reference)[i], reinterpret_cast<uint8_t*>(&result)[i]);
          std::exit(1);
        }
      }
    }
    for (const auto& region : regions) Compare(name, region);
    Compare(name, {kStack - 256, 512});
    ++cases_;
  }

  void Report(const char* suite) {
    std::printf("PASS: %s, %zu cases, full PPC context and observed memory match\n", suite, cases_);
  }

 private:
  static constexpr size_t kGuestSize = 0x100002000ull;
  uint8_t* original_;
  uint8_t* native_;
  size_t cases_ = 0;

  static uint8_t* Allocate() {
    void* result = mmap(nullptr, kGuestSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (result == MAP_FAILED) { std::perror("mmap"); std::exit(1); }
    return static_cast<uint8_t*>(result);
  }
  static uint8_t* Raw(uint8_t* base, uint32_t address) { return REX_RAW_ADDR(address); }
  void Compare(const char* name, NativeTestRegion region) {
    const auto* reference = Raw(original_, region.address);
    const auto* result = Raw(native_, region.address);
    for (size_t i = 0; i < region.size; ++i) {
      if (reference[i] != result[i]) {
        std::fprintf(stderr, "%s case %zu: memory %08x, %02x / %02x\n", name, cases_,
                     uint32_t(region.address + i), reference[i], result[i]);
        std::exit(1);
      }
    }
  }
};
