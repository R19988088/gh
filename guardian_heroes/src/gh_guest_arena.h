#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace gh::memory {

// Owns the host backing store and its contiguous guest/physical alias views.
// Heap allocation, page permissions and MMIO remain the caller's responsibility.
class GuestArena {
 public:
  GuestArena() = default;
  ~GuestArena();
  GuestArena(const GuestArena&) = delete;
  GuestArena& operator=(const GuestArena&) = delete;

  bool Initialize();
  void Close();
  uint8_t* virtual_base() const { return base_; }
  uint8_t* physical_base() const { return base_ ? base_ + 0x100000000ull : nullptr; }
  size_t allocation_granularity() const { return granularity_; }
  const std::string& name() const { return name_; }
  const std::string& error() const { return error_; }

 private:
  bool CreateBacking(size_t size);
  bool MapViews(uint8_t* requested_base);
  void UnmapViews();
  void SetSystemError(const char* operation);

  intptr_t backing_ = -1;
  uint8_t* base_ = nullptr;
  std::array<uint8_t*, 9> views_{};
  size_t granularity_ = 0;
  std::string name_;
  std::string error_;
};

}  // namespace gh::memory
