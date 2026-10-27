#pragma once
// MMIO handling derived from Xenia, Copyright 2014 Ben Vanik, BSD license.
#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string_view>
#include <vector>
#include "gh_host_memory.h"

namespace gh::memory {
class FaultHost {
 public:
  virtual ~FaultHost() = default;
  virtual std::recursive_mutex& GlobalMutex() = 0;
  virtual uint32_t HostPageSize() const = 0;
  virtual bool QueryProtection(void*, size_t&, PageAccess&) = 0;
  virtual void FaultLog(std::string_view message) = 0;
};

struct FaultContext {
  uint64_t address = 0;
  uint64_t instruction = 0;
  bool is_write = false;
  std::array<uint64_t, 32> x{};
  struct Vector { uint32_t u32[4]{}; };
  std::array<Vector, 32> v{};
  uint64_t sp = 0;
  uint32_t changed_x = 0;
  uint32_t changed_v = 0;
  uint64_t resume_pc = 0;
  uint64_t fault_address() const { return address; }
  uint64_t pc() const { return instruction; }
  uint64_t& ModifyIntRegister(uint32_t i) { changed_x |= uint32_t(1) << i; return x[i]; }
  uint64_t& ModifyXRegister(uint32_t i) { return ModifyIntRegister(i); }
  Vector& ModifyVRegister(uint32_t i) { changed_v |= uint32_t(1) << i; return v[i]; }
  void set_resume_pc(uint64_t value) { resume_pc = value; }
};

typedef uint32_t (*MMIOReadCallback)(void* ppc_context, void* callback_context, uint32_t addr);
typedef void (*MMIOWriteCallback)(void* ppc_context, void* callback_context, uint32_t addr,
                                  uint32_t value);

struct MMIORange {
  uint32_t address;
  uint32_t mask;
  uint32_t size;
  void* callback_context;
  MMIOReadCallback read;
  MMIOWriteCallback write;
};

class MMIO {
 public:
  using HostToGuestVirtual = uint32_t (*)(const void*, const void*);
  using AccessViolationCallback = bool (*)(std::unique_lock<std::recursive_mutex>, void*, void*, bool);
  MMIO(FaultHost& host, uint8_t* virtual_membase, uint8_t* physical_membase,
       uint8_t* membase_end, HostToGuestVirtual translate, const void* translate_context,
       AccessViolationCallback violation, void* violation_context);
  bool RegisterRange(uint32_t virtual_address, uint32_t mask, uint32_t size, void* context,
                     MMIOReadCallback read_callback, MMIOWriteCallback write_callback);
  MMIORange* LookupRange(uint32_t virtual_address);
  bool CheckLoad(uint32_t virtual_address, uint32_t* out_value);
  bool CheckStore(uint32_t virtual_address, uint32_t value);
  bool HandleFault(FaultContext& fault);
 private:
  FaultHost& host_;
  uint8_t* virtual_membase_;
  uint8_t* physical_membase_;
  uint8_t* memory_end_;
  std::vector<MMIORange> mapped_ranges_;
  HostToGuestVirtual host_to_guest_virtual_;
  const void* host_to_guest_virtual_context_;
  AccessViolationCallback access_violation_callback_;
  void* access_violation_callback_context_;
  struct DecodedLoadStore {
    // Matches the Xn/Wn register number for 0 reads and ignored writes in many
    // usage cases.
    static constexpr uint8_t kArm64RegZero = 31;

    // Matches the actual register number encoding for an SP base in AArch64
    // load and store instructions.
    static constexpr uint8_t kArm64MemBaseRegSp = kArm64RegZero;

    static constexpr uint8_t kArm64ValueRegX0 = 0;
    static constexpr uint8_t kArm64ValueRegZero = kArm64ValueRegX0 + kArm64RegZero;
    static constexpr uint8_t kArm64ValueRegV0 = 32;

    size_t length;
    // Inidicates this is a load (or conversely a store).
    bool is_load;
    // Indicates the memory must be swapped.
    bool byte_swap;
    // Source (for store) or target (for load) register.
    // For x86-64:
    // AX  CX  DX  BX  SP  BP  SI  DI   // REX.R=0
    // R8  R9  R10 R11 R12 R13 R14 R15  // REX.R=1
    // For AArch64:
    // - kArm64ValueRegX0 + [0...30]: Xn (Wn for 32 bits - upper 32 bits of Xn
    //   are zeroed on Wn write).
    // - kArm64ValueRegZero: Zero constant for register read, ignored register
    //   write (though memory must still be accessed - a MMIO load may have side
    //   effects even if the result is discarded).
    // - kArm64ValueRegV0 + [0...31]: Vn (Sn for 32 bits).
    uint8_t value_reg;
    // [base + (index * scale) + displacement]
    bool mem_has_base;
    // On AArch64, if mem_base_reg is kArm64MemBaseRegSp, the base register is
    // SP, not Xn.
    uint8_t mem_base_reg;
    // For AArch64 pre- and post-indexing. In case of a load, the base register
    // is written back after the loaded data is written to the register,
    // overwriting the value register if it's the same.
    bool mem_base_writeback;
    int32_t mem_base_writeback_offset;
    bool mem_has_index;
    uint8_t mem_index_reg;
    uint8_t mem_index_size;
    bool mem_index_sign_extend;
    uint8_t mem_scale;
    ptrdiff_t mem_displacement;
    bool is_constant;
    int32_t constant;
  };

  static bool TryDecodeLoadStore(const uint8_t* p, DecodedLoadStore& decoded_out);
};

}  // namespace gh::memory
