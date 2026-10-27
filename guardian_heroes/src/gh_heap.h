#pragma once
// Page heaps derived from Xenia, Copyright 2020 Ben Vanik, BSD license.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string_view>
#include <utility>
#include <vector>
#include "gh_host_memory.h"

namespace gh::memory {
// Host services preserve the existing OS mapping and global lock domain.
class HeapHost {
 public:
  virtual ~HeapHost() = default;
  virtual uint32_t HostPageSize() const = 0;
  virtual uint32_t HostAllocationGranularity() const = 0;
  virtual void* AllocateHost(void*, size_t, AllocationType, PageAccess) = 0;
  virtual bool ReleaseHost(void*, size_t, DeallocationType) = 0;
  virtual bool ProtectHost(void*, size_t, PageAccess, PageAccess* old_access = nullptr) = 0;
  virtual std::recursive_mutex& GlobalMutex() = 0;
  virtual bool ProtectOnRelease() const = 0;
  virtual bool ScribbleHeap() const = 0;
  virtual void HeapLog(int level, std::string_view message) = 0;
  // Inclusive intersection of all callbacks' permitted unwatch ranges.
  virtual std::pair<uint32_t, uint32_t> InvalidatePhysicalRange(
      uint32_t start, uint32_t length, bool exact) = 0;
};

class HeapStream {
 public:
  virtual ~HeapStream() = default;
  virtual void Read(void* data, size_t size) = 0;
  virtual void Write(const void* data, size_t size) = 0;
  template <typename T> T Read() { T value; Read(&value, sizeof(value)); return value; }
  template <typename T> void Write(T value) { Write(&value, sizeof(value)); }
};

enum class HeapType : uint8_t {
  kGuestVirtual,
  kGuestXex,
  kGuestPhysical,
  kHostPhysical,
};

enum MemoryAllocationFlag : uint32_t {
  kMemoryAllocationReserve = 1 << 0,
  kMemoryAllocationCommit = 1 << 1,
};

enum MemoryProtectFlag : uint32_t {
  kMemoryProtectRead = 1 << 0,
  kMemoryProtectWrite = 1 << 1,
  kMemoryProtectNoCache = 1 << 2,
  kMemoryProtectWriteCombine = 1 << 3,

  kMemoryProtectNoAccess = 0,
};

// Equivalent to the Win32 MEMORY_BASIC_INFORMATION struct.
struct HeapAllocationInfo {
  // A pointer to the base address of the region of pages.
  uint32_t base_address;
  // A pointer to the base address of a range of pages allocated by the
  // VirtualAlloc function. The page pointed to by the BaseAddress member is
  // contained within this allocation range.
  uint32_t allocation_base;
  // The memory protection option when the region was initially allocated.
  uint32_t allocation_protect;
  // The size specified when the region was initially allocated, in bytes.
  uint32_t allocation_size;
  // The size of the region beginning at the base address in which all pages
  // have identical attributes, in bytes.
  uint32_t region_size;
  // The state of the pages in the region (commit/free/reserve).
  uint32_t state;
  // The access protection of the pages in the region.
  uint32_t protect;
};

// Describes a single page in the page table.
union PageEntry {
  uint64_t qword;
  struct {
    // Base address of the allocated region in 4k pages.
    uint32_t base_address : 20;
    // Total number of pages in the allocated region in 4k pages.
    uint32_t region_page_count : 20;
    // Protection bits specified during region allocation.
    // Composed of bits from MemoryProtectFlag.
    uint32_t allocation_protect : 4;
    // Current protection bits as of the last Protect.
    // Composed of bits from MemoryProtectFlag.
    uint32_t current_protect : 4;
    // Allocation state of the page as a MemoryAllocationFlag bit mask.
    uint32_t state : 2;
    uint32_t reserved : 14;
  };
};

// Heap abstraction for page-based allocation.
class BaseHeap {
 public:
  virtual ~BaseHeap();

  // Offset of the heap in relative to membase, without host_address_offset
  // adjustment.
  uint32_t heap_base() const { return heap_base_; }

  // Length of the heap range.
  uint32_t heap_size() const { return heap_size_; }

  // Size of each page within the heap range in bytes.
  uint32_t page_size() const { return page_size_; }

  // Type of specified heap
  HeapType heap_type() const { return heap_type_; }

  // Offset added to the virtual addresses to convert them to host addresses
  // (not including membase).
  uint32_t host_address_offset() const { return host_address_offset_; }

  template <typename T = uint8_t*>
  inline T TranslateRelative(size_t relative_address) const {
    return reinterpret_cast<T>(membase_ + heap_base_ + host_address_offset_ + relative_address);
  }

  // Disposes and decommits all memory and clears the page table.
  virtual void Dispose();

  // Dumps information about all allocations within the heap to the log.
  void DumpMap();

  uint32_t GetTotalPageCount();
  uint32_t GetUnreservedPageCount();

  uint32_t total_page_count() const { return uint32_t(page_table_.size()); }
  uint32_t unreserved_page_count() const { return unreserved_page_count_; }
  uint32_t reserved_page_count() const { return total_page_count() - unreserved_page_count(); }

  // Allocates pages with the given properties and allocation strategy.
  // This can reserve and commit the pages as well as set protection modes.
  // This will fail if not enough contiguous pages can be found.
  virtual bool Alloc(uint32_t size, uint32_t alignment, uint32_t allocation_type, uint32_t protect,
                     bool top_down, uint32_t* out_address);

  // Allocates pages at the given address.
  // This can reserve and commit the pages as well as set protection modes.
  // This will fail if the pages are already allocated.
  virtual bool AllocFixed(uint32_t base_address, uint32_t size, uint32_t alignment,
                          uint32_t allocation_type, uint32_t protect);

  // Allocates pages at an address within the given address range.
  // This can reserve and commit the pages as well as set protection modes.
  // This will fail if not enough contiguous pages can be found.
  virtual bool AllocRange(uint32_t low_address, uint32_t high_address, uint32_t size,
                          uint32_t alignment, uint32_t allocation_type, uint32_t protect,
                          bool top_down, uint32_t* out_address);

  // Allocates from the system portion of the heap (top of virtual heaps).
  // Physical heaps delegate to regular Alloc since there is no split.
  virtual bool AllocSystemHeap(uint32_t size, uint32_t alignment, uint32_t allocation_type,
                               uint32_t protect, bool top_down, uint32_t* out_address);

  // Decommits pages in the given range.
  // Partial overlapping pages will also be decommitted.
  virtual bool Decommit(uint32_t address, uint32_t size);

  // Decommits and releases pages in the given range.
  // Partial overlapping pages will also be released.
  virtual bool Release(uint32_t address, uint32_t* out_region_size = nullptr);

  // Modifies the protection mode of pages within the given range.
  virtual bool Protect(uint32_t address, uint32_t size, uint32_t protect,
                       uint32_t* old_protect = nullptr);

  // Queries information about the given region of pages.
  bool QueryRegionInfo(uint32_t base_address, HeapAllocationInfo* out_info);

  // Queries the size of the region containing the given address.
  bool QuerySize(uint32_t address, uint32_t* out_size);

  // Queries the base and size of a region containing the given address.
  bool QueryBaseAndSize(uint32_t* in_out_address, uint32_t* out_size);

  // Queries the current protection mode of the region containing the given
  // address.
  bool QueryProtect(uint32_t address, uint32_t* out_protect);

  // Queries the currently strictest readability and writability for the entire
  // range.
  PageAccess QueryRangeAccess(uint32_t low_address, uint32_t high_address);

  bool Save(HeapStream* stream);
  bool Restore(HeapStream* stream);

  void Reset();

 protected:
  BaseHeap();

  // Callers must hold the global critical region - see
  // AcquireHostPageReconcileLock.
  bool SyncHostPageAccess(uint32_t start_page_number, uint32_t end_page_number);

  // Acquires the global critical region for operations that will run the host
  // page reconcile pass, which reads watch state guarded by it.
  //
  // It must be taken before heap_mutex_, not partway through: mutex.h requires
  // the global region to be acquired first and held longest, and
  // PhysicalHeap::Decommit/Release/Protect already call down into BaseHeap
  // with it held. Acquiring it inside SyncHostPageAccess instead would give
  // those paths heap_mutex_ -> global while PhysicalHeap has global ->
  // heap_mutex_, which deadlocks.
  //
  // Returns an empty (unlocked) guard when guest pages are at least host page
  // sized, since no reconcile happens then and the lock would be pure cost.
  virtual std::unique_lock<std::recursive_mutex> AcquireHostPageReconcileLock() const;

  // Whether a host page is currently write-watched for guest invalidation.
  // SyncHostPageAccess must not restore write access to such a page: the watch
  // is recorded in a flag that EnableAccessCallbacks checks before re-applying
  // protection, so silently unprotecting here would disable the watch forever.
  // Only PhysicalHeap tracks this; every other heap has no watch state.
  virtual bool IsHostPageWriteWatched(uint32_t) const { return false; }

  void Initialize(HeapHost* host, uint8_t* membase, HeapType heap_type, uint32_t heap_base,
                  uint32_t heap_size, uint32_t page_size, uint32_t host_address_offset = 0);

  HeapHost* host_;
  uint8_t* membase_;
  HeapType heap_type_;
  uint32_t heap_base_;
  uint32_t heap_size_;
  uint32_t page_size_;
  uint32_t page_size_shift_ = 0;
  uint32_t host_address_offset_;
  uint32_t unreserved_page_count_ = 0;
  std::recursive_mutex heap_mutex_;
  std::vector<PageEntry> page_table_;
};

// Normal heap allowing allocations from guest virtual address ranges.
class VirtualHeap : public BaseHeap {
 public:
  VirtualHeap();
  ~VirtualHeap() override;

  // Initializes the heap properties and allocates the page table.
  void Initialize(HeapHost* host, uint8_t* membase, HeapType heap_type, uint32_t heap_base,
                  uint32_t heap_size, uint32_t page_size);
};

// A heap for ranges of memory that are mapped to physical ranges.
// Physical ranges are used by the audio and graphics subsystems representing
// hardware wired directly to memory in the console.
//
// The physical heap and the behavior of sharing pages with virtual pages is
// implemented by having a 'parent' heap that is used to perform allocation in
// the guest virtual address space 1:1 with the physical address space.
class PhysicalHeap : public BaseHeap {
 public:
  PhysicalHeap();
  ~PhysicalHeap() override;

  // Initializes the heap properties and allocates the page table.
  void Initialize(HeapHost* host, uint8_t* membase, HeapType heap_type, uint32_t heap_base,
                  uint32_t heap_size, uint32_t page_size, VirtualHeap* parent_heap);

  bool Alloc(uint32_t size, uint32_t alignment, uint32_t allocation_type, uint32_t protect,
             bool top_down, uint32_t* out_address) override;
  bool AllocFixed(uint32_t base_address, uint32_t size, uint32_t alignment,
                  uint32_t allocation_type, uint32_t protect) override;
  bool AllocRange(uint32_t low_address, uint32_t high_address, uint32_t size, uint32_t alignment,
                  uint32_t allocation_type, uint32_t protect, bool top_down,
                  uint32_t* out_address) override;
  bool AllocSystemHeap(uint32_t size, uint32_t alignment, uint32_t allocation_type,
                       uint32_t protect, bool top_down, uint32_t* out_address) override;
  bool Decommit(uint32_t address, uint32_t size) override;
  bool Release(uint32_t base_address, uint32_t* out_region_size = nullptr) override;
  bool Protect(uint32_t address, uint32_t size, uint32_t protect,
               uint32_t* old_protect = nullptr) override;

  void EnableAccessCallbacks(uint32_t physical_address, uint32_t length,
                             bool enable_invalidation_notifications, bool enable_data_providers);
  // Returns true if any page in the range was watched.
  bool TriggerCallbacks(std::unique_lock<std::recursive_mutex> global_lock_locked_once,
                        uint32_t virtual_address, uint32_t length, bool is_write,
                        bool unwatch_exact_range, bool unprotect = true);

  uint32_t GetPhysicalAddress(uint32_t address) const;

 protected:
  bool IsHostPageWriteWatched(uint32_t host_page_number) const override;

  // Allocation on these heaps delegates to parent_heap_, whose own reconcile
  // pass takes the global critical region. Acquire whenever this heap or its
  // parent will need it, so the ordering holds even for a heap whose own guest
  // pages are large enough to skip reconciling but whose parent's are not.
  std::unique_lock<std::recursive_mutex> AcquireHostPageReconcileLock() const override;

  VirtualHeap* parent_heap_;

  uint32_t system_page_size_;
  uint32_t system_page_count_;

  struct SystemPageFlagsBlock {
    // Whether writing to each page should result trigger invalidation
    // callbacks.
    uint64_t notify_on_invalidation;
  };
  // Protected by global_critical_region. Flags for each 64 system pages,
  // interleaved as blocks, so bit scan can be used to quickly extract ranges.
  std::vector<SystemPageFlagsBlock> system_page_flags_;
};

}  // namespace gh::memory
