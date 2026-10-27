#pragma once
/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "gh_memory_service.h"

#include <rex/memory/utils.h>
#include <rex/ppc/func.h>  // PPCFunc type
#include <rex/system/mmio_handler.h>
#include <rex/thread/mutex.h>

namespace gh::memory {
class GuestArena;
}  // namespace gh::memory

namespace rex::stream {
class ByteStream;
}  // namespace rex::stream

namespace rex::memory::detail {

/// Compensates for host allocation granularity coarser than 4KB on the 0xE0
/// physical heap. When the granularity exceeds 0x1000, the backing file maps the
/// 0xE0 heap at a 0x1000-byte offset that the mapping API rounds away (Windows
/// MapViewOfFileEx rounds down to 64KB; macOS arm64 uses 16KB pages), so guest
/// accesses must add it back. This must agree with the alias offset alignment in
/// gh::memory::GuestArena and heap host_address_offset for granularity > 0x1000:
///   - Windows:        64KB granularity  -> offset
///   - macOS arm64:    16KB granularity  -> offset
///   - macOS x86_64:    4KB granularity  -> no offset
///   - Linux (any):     4KB granularity  -> no offset (mmap handles it natively)
constexpr u32 PhysicalHostOffset([[maybe_unused]] u32 guest_addr) noexcept {
#if REX_PLATFORM_WIN32 || (REX_PLATFORM_MAC && defined(__aarch64__))
  return (guest_addr >= 0xE0000000u) ? 0x1000u : 0u;
#else
  return 0u;
#endif
}

}  // namespace rex::memory::detail

namespace rex::memory {

/// Lightweight guest-to-host pointer translation using the memory base.
/// For hooks and kernel code operating with the base pointer directly.
/// Same raw arithmetic as recompiled code; no Memory* or heap lookup needed.
template <typename T = u8*>
inline T GuestPtr(u8* base, u32 guest_address) noexcept {
  return reinterpret_cast<T>(base + guest_address + detail::PhysicalHostOffset(guest_address));
}

class Memory;

enum SystemHeapFlag : uint32_t {
  kSystemHeapVirtual = 1 << 0,
  kSystemHeapPhysical = 1 << 1,

  kSystemHeapDefault = kSystemHeapVirtual,
};

using gh::memory::HeapType;
using gh::memory::MemoryAllocationFlag;
using gh::memory::MemoryProtectFlag;
using gh::memory::HeapAllocationInfo;
using gh::memory::PageEntry;
using gh::memory::BaseHeap;
using gh::memory::VirtualHeap;
using gh::memory::PhysicalHeap;
using gh::memory::kMemoryAllocationReserve;
using gh::memory::kMemoryAllocationCommit;
using gh::memory::kMemoryProtectRead;
using gh::memory::kMemoryProtectWrite;
using gh::memory::kMemoryProtectNoCache;
using gh::memory::kMemoryProtectWriteCombine;
using gh::memory::kMemoryProtectNoAccess;

// Models the entire guest memory system on the console.
// This exposes interfaces to both virtual and physical memory and a TLB and
// page table for allocation, mapping, and protection.
//
// The memory is backed by a memory mapped file and is placed at a stable
// fixed address in the host address space (like 0x100000000). This allows
// efficient guest<->host address translations as well as easy sharing of the
// memory across various subsystems.
//
// The guest memory address space is split into several ranges that have varying
// properties such as page sizes, caching strategies, protections, and
// overlap with other ranges. Each range is represented by a BaseHeap of either
// VirtualHeap or PhysicalHeap depending on type. Heaps model the page tables
// and can handle reservation and committing of requested pages.
class Memory {
 public:
  Memory();
  ~Memory();

  // Initializes the memory system.
  // This may fail if the host address space could not be reserved or the
  // mapping to the file system fails.
  bool Initialize();

  // Resets all memory to zero and resets all allocations.
  void Reset();

  // Full file name and path of the memory-mapped file backing all memory.
  const std::filesystem::path& file_name() const { return service_->file_name(); }

  // Base address of virtual memory in the host address space.
  // This is often something like 0x100000000.
  inline uint8_t* virtual_membase() const { return service_->virtual_membase(); }

  // Translates a guest virtual address to a host address that can be accessed
  // as a normal pointer.
  // Note that the contents at the specified host address are big-endian.
  template <typename T = uint8_t*>
  inline T TranslateVirtual(uint32_t guest_address) const {
    return service_->TranslateVirtual<T>(guest_address);
  }

  // Base address of physical memory in the host address space.
  // This is often something like 0x200000000.
  inline uint8_t* physical_membase() const { return service_->physical_membase(); }

  // Translates a guest physical address to a host address that can be accessed
  // as a normal pointer.
  // Note that the contents at the specified host address are big-endian.
  template <typename T = uint8_t*>
  inline T TranslatePhysical(uint32_t guest_address) const {
    return reinterpret_cast<T>(physical_membase() + (guest_address & 0x1FFFFFFF));
  }

  // Translates a host address to a guest virtual address.
  // Note that the contents at the returned host address are big-endian.
  uint32_t HostToGuestVirtual(const void* host_address) const;

  // Returns the guest physical address for the guest virtual address, or
  // UINT32_MAX if it can't be obtained.
  uint32_t GetPhysicalAddress(uint32_t address) const;

  // Zeros out a range of memory at the given guest address.
  void Zero(uint32_t address, uint32_t size);

  // Fills a range of guest memory with the given byte value.
  void Fill(uint32_t address, uint32_t size, uint8_t value);

  // Copies a non-overlapping range of guest memory (like a memcpy).
  void Copy(uint32_t dest, uint32_t src, uint32_t size);

  // Searches the given range of guest memory for a run of dword values in
  // big-endian order.
  uint32_t SearchAligned(uint32_t start, uint32_t end, const uint32_t* values, size_t value_count);

  // Defines a memory-mapped IO (MMIO) virtual address range that when accessed
  // will trigger the specified read and write callbacks for dword read/writes.
  bool AddVirtualMappedRange(uint32_t virtual_address, uint32_t mask, uint32_t size, void* context,
                             runtime::MMIOReadCallback read_callback,
                             runtime::MMIOWriteCallback write_callback);

  // Gets the defined MMIO range for the given virtual address, if any.
  runtime::MMIORange* LookupVirtualMappedRange(uint32_t virtual_address);

  // Physical memory access callbacks, two types of them.
  //
  // This is simple per-system-page protection without reference counting or
  // stored ranges. Whenever a watched page is accessed, all callbacks for it
  // are triggered. Also the only way to remove callbacks is to trigger them
  // somehow. Since there are no references from pages to individual callbacks,
  // there's no way to disable only a specific callback for a page. Also
  // callbacks may be triggered spuriously, and handlers should properly ignore
  // pages they don't care about.
  //
  // Once callbacks are triggered for a page, the page is not watched anymore
  // until requested again later. It is, however, unwatched only in one guest
  // view of physical memory (because different views may have different
  // protection for the same memory) - but it's rare when the same memory is
  // used with different guest page sizes, and it's okay to fire a callback more
  // than once.
  //
  // Only accessing the guest virtual memory views of physical memory triggers
  // callbacks - data providers, for instance, must write to the host physical
  // heap directly, otherwise their threads may infinitely await themselves.
  //
  // - Invalidation notifications:
  //
  // Protecting from writing. One-shot callbacks for invalidation of various
  // kinds of physical memory caches (such as the GPU copy of the memory).
  //
  // May be triggered for a single page (in case of a write access violation or
  // when need to synchronize data given by data providers) or for multiple
  // pages (like when memory is released, or explicitly to trigger callbacks
  // when host-side code can't rely on regular access violations, like when
  // accessing a file).
  //
  // Since granularity of callbacks is one single page, an invalidation
  // notification handler must invalidate the all the data stored in the touched
  // pages.
  //
  // Because large ranges (like whole framebuffers) may be written to and
  // exceptions are expensive, it's better to unprotect multiple pages as a
  // result of a write access violation, so the shortest common range returned
  // by all the invalidation callbacks (clamped to a sane range and also not to
  // touch pages with provider callbacks) is unprotected.
  //
  // - Data providers:
  //
  // TODO(Triang3l): Implement data providers - more complicated because they
  // will need to be able to release the global lock.

  // Returns start and length of the smallest physical memory region surrounding
  // the watched region that can be safely unwatched, if it doesn't matter,
  // return (0, UINT32_MAX).
  typedef std::pair<uint32_t, uint32_t> (*PhysicalMemoryInvalidationCallback)(
      void* context_ptr, uint32_t physical_address_start, uint32_t length, bool exact_range);
  // Returns a handle for unregistering or for skipping one notification handler
  // while triggering data providers.
  void* RegisterPhysicalMemoryInvalidationCallback(PhysicalMemoryInvalidationCallback callback,
                                                   void* callback_context);
  // Unregisters a physical memory invalidation callback previously added with
  // RegisterPhysicalMemoryInvalidationCallback.
  void UnregisterPhysicalMemoryInvalidationCallback(void* callback_handle);

  // Enables physical memory access callbacks for the specified memory range,
  // snapped to system page boundaries.
  void EnablePhysicalMemoryAccessCallbacks(uint32_t physical_address, uint32_t length,
                                           bool enable_invalidation_notifications,
                                           bool enable_data_providers);

  // Forces triggering of watch callbacks for a virtual address range if pages
  // are watched there and unwatching them. Returns whether any page was
  // watched. Must be called with global critical region locking depth of 1.
  // TODO(Triang3l): Implement data providers - this is why locking depth of 1
  // will be required in the future.
  bool TriggerPhysicalMemoryCallbacks(
      std::unique_lock<std::recursive_mutex> global_lock_locked_once, uint32_t virtual_address,
      uint32_t length, bool is_write, bool unwatch_exact_range, bool unprotect = true);

  // Allocates virtual memory from the 'system' heap.
  // System memory is kept separate from game memory but is still accessible
  // using normal guest virtual addresses. Kernel structures and other internal
  // 'system' allocations should come from this heap when possible.
  uint32_t SystemHeapAlloc(uint32_t size, uint32_t alignment = 0x20,
                           uint32_t system_heap_flags = kSystemHeapDefault);

  // Frees memory allocated with SystemHeapAlloc.
  void SystemHeapFree(uint32_t address, uint32_t* out_region_size = nullptr);

  // Gets the heap for the address space containing the given address.
  const BaseHeap* LookupHeap(uint32_t address) const;

  inline BaseHeap* LookupHeap(uint32_t address) {
    return const_cast<BaseHeap*>(const_cast<const memory::Memory*>(this)->LookupHeap(address));
  }

  // Gets the heap with the given properties.
  BaseHeap* LookupHeapByType(bool physical, uint32_t page_size);

  // Aggregates page statistics across the given heaps.
  void GetHeapsPageStatsSummary(const BaseHeap* const* provided_heaps, size_t heaps_count,
                                uint32_t& unreserved_pages, uint32_t& reserved_pages,
                                uint32_t& used_pages, uint32_t& reserved_bytes);

  // Gets the physical base heap.
  VirtualHeap* GetPhysicalHeap();

  // Dumps a map of all allocated memory to the log.
  void DumpMap();

  bool Save(stream::ByteStream* stream);
  bool Restore(stream::ByteStream* stream);

  //==========================================================================
  // Recompiled Code Function Table API
  //==========================================================================
  // Per-module function dispatch table at IMAGE_BASE + IMAGE_SIZE, indexed by
  // (guest_addr - code_base) * 2. Internally locked: callers may invoke from
  // any thread.
  bool InitializeFunctionTable(uint32_t code_base, uint32_t code_size, uint32_t image_base,
                               uint32_t image_size);
  bool DestroyFunctionTable(uint32_t code_base);
  // Returns false if guest_address is outside every registered module range.
  bool SetFunction(uint32_t guest_address, PPCFunc* host_function);
  bool HasAnyFunctionTable() const;

 private:
  static uint32_t HostToGuestVirtualThunk(const void* context, const void* host_address);

  static bool AccessViolationCallbackThunk(
      std::unique_lock<std::recursive_mutex> global_lock_locked_once, void* context,
      void* host_address, bool is_write);

  struct FunctionTableEntry {
    uint32_t table_base;
    uint32_t code_base;
    uint32_t code_size;
    uint32_t thunk_reserve;
  };
  std::vector<FunctionTableEntry> function_tables_;
  mutable std::mutex function_tables_mutex_;

  std::unique_ptr<gh::memory::MemoryService> service_;
  std::unique_ptr<runtime::MMIOHandler> mmio_handler_;
};

}  // namespace rex::memory
