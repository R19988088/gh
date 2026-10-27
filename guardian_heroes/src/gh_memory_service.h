#pragma once

#include "gh_guest_arena.h"
#include "gh_heap.h"

#include <filesystem>
#include <functional>
#include <memory>

namespace gh::memory {

// Owns guest address-space layout, heaps, physical watches and page recovery.
// The caller supplies its existing global lock and diagnostic/configuration hooks.
class MemoryService final : private HeapHost {
 public:
  using Log = std::function<void(int, std::string_view)>;
  using PhysicalMemoryInvalidationCallback = std::pair<uint32_t, uint32_t> (*)(
      void*, uint32_t, uint32_t, bool);
  MemoryService(std::recursive_mutex& global_mutex, std::function<bool()> protect_on_release,
                std::function<bool()> scribble_heap, Log log);
  ~MemoryService();
  MemoryService(const MemoryService&) = delete;
  MemoryService& operator=(const MemoryService&) = delete;

  // Install the host exception adapter between physical precommit and the
  // title's remaining reserved allocations, preserving initialization order.
  bool Initialize(bool protect_zero, const std::function<bool()>& install_mmio);
  void Reset();
  const std::filesystem::path& file_name() const { return file_name_; }
  uint8_t* virtual_membase() const { return virtual_membase_; }
  uint8_t* physical_membase() const { return physical_membase_; }
  uint32_t system_page_size() const { return system_page_size_; }
  const BaseHeap* LookupHeap(uint32_t address) const;
  BaseHeap* LookupHeap(uint32_t address) {
    return const_cast<BaseHeap*>(static_cast<const MemoryService*>(this)->LookupHeap(address));
  }
  BaseHeap* LookupHeapByType(bool physical, uint32_t page_size);
  VirtualHeap* GetPhysicalHeap();
  template <typename T = uint8_t*> T TranslateVirtual(uint32_t address) const {
    auto* pointer = virtual_membase_ + address;
    if (const auto* heap = LookupHeap(address)) pointer += heap->host_address_offset();
    return reinterpret_cast<T>(pointer);
  }
  uint32_t HostToGuestVirtual(const void*) const;
  uint32_t GetPhysicalAddress(uint32_t) const;
  void Zero(uint32_t, uint32_t);
  void Fill(uint32_t, uint32_t, uint8_t);
  void Copy(uint32_t, uint32_t, uint32_t);
  uint32_t SearchAligned(uint32_t, uint32_t, const uint32_t*, size_t);
  bool PrepareMappedRange(uint32_t address, uint32_t size);
  bool AccessViolationCallback(std::unique_lock<std::recursive_mutex>, void*, bool);
  bool TriggerPhysicalMemoryCallbacks(std::unique_lock<std::recursive_mutex>, uint32_t,
                                      uint32_t, bool, bool, bool unprotect = true);
  void* RegisterPhysicalMemoryInvalidationCallback(PhysicalMemoryInvalidationCallback, void*);
  void UnregisterPhysicalMemoryInvalidationCallback(void*);
  void EnablePhysicalMemoryAccessCallbacks(uint32_t, uint32_t, bool, bool);
  uint32_t SystemHeapAlloc(uint32_t size, uint32_t alignment, bool physical);
  void SystemHeapFree(uint32_t, uint32_t* = nullptr);
  void GetHeapsPageStatsSummary(const BaseHeap* const*, size_t, uint32_t&, uint32_t&,
                                uint32_t&, uint32_t&);
  void DumpMap();
  bool Save(HeapStream*);
  bool Restore(HeapStream*);

 private:
  uint32_t HostPageSize() const override { return system_page_size_; }
  uint32_t HostAllocationGranularity() const override { return system_allocation_granularity_; }
  void* AllocateHost(void*, size_t, AllocationType, PageAccess) override;
  bool ReleaseHost(void*, size_t, DeallocationType) override;
  bool ProtectHost(void*, size_t, PageAccess, PageAccess*) override;
  std::recursive_mutex& GlobalMutex() override { return global_mutex_; }
  bool ProtectOnRelease() const override { return protect_on_release_(); }
  bool ScribbleHeap() const override { return scribble_heap_(); }
  void HeapLog(int level, std::string_view message) override { if (log_) log_(level, message); }
  std::pair<uint32_t, uint32_t> InvalidatePhysicalRange(uint32_t, uint32_t, bool) override;

  std::recursive_mutex& global_mutex_;
  std::function<bool()> protect_on_release_;
  std::function<bool()> scribble_heap_;
  Log log_;
  std::filesystem::path file_name_;
  uint32_t system_page_size_ = 0;
  uint32_t system_allocation_granularity_ = 0;
  uint8_t* virtual_membase_ = nullptr;
  uint8_t* physical_membase_ = nullptr;
  std::unique_ptr<GuestArena> arena_;
  struct {
    VirtualHeap v00000000, v40000000, v80000000, v90000000, physical;
    PhysicalHeap vA0000000, vC0000000, vE0000000;
  } heaps_;
  std::vector<std::pair<PhysicalMemoryInvalidationCallback, void*>*>
      physical_memory_invalidation_callbacks_;
};

}  // namespace gh::memory
