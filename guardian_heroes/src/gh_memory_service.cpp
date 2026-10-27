// Guest memory service derived from Xenia, Copyright 2020 Ben Vanik, BSD license.
#include "gh_memory_service.h"
#include <algorithm>
#include <bit>
#include <cassert>
#include <format>
#include <thread>

#define GH_MEMORY_ERROR(...) HeapLog(3, std::format(__VA_ARGS__))
#define GH_MEMORY_WARN(...) HeapLog(2, std::format(__VA_ARGS__))
#define GH_MEMORY_DEBUG(...) HeapLog(0, std::format(__VA_ARGS__))
#define GH_MEMORY_INFO(...) HeapLog(1, std::format(__VA_ARGS__))

namespace gh::memory {
namespace {
size_t RoundUp(size_t value, size_t multiple) {
  return value ? ((value + multiple - 1) / multiple) * multiple : multiple;
}
}
MemoryService::MemoryService(std::recursive_mutex& mutex, std::function<bool()> protect_on_release,
                             std::function<bool()> scribble_heap, Log log)
    : global_mutex_(mutex), protect_on_release_(std::move(protect_on_release)),
      scribble_heap_(std::move(scribble_heap)), log_(std::move(log)) {
  system_page_size_ = uint32_t(host::page_size());
  system_allocation_granularity_ = uint32_t(host::allocation_granularity());
}

MemoryService::~MemoryService() {
  for (auto invalidation_callback : physical_memory_invalidation_callbacks_) {
    delete invalidation_callback;
  }

  heaps_.v00000000.Dispose();
  heaps_.v40000000.Dispose();
  heaps_.v80000000.Dispose();
  heaps_.v90000000.Dispose();
  heaps_.vA0000000.Dispose();
  heaps_.vC0000000.Dispose();
  heaps_.vE0000000.Dispose();
  heaps_.physical.Dispose();

  // The arena outlives the heaps and MMIO handler that address its views.
  arena_.reset();

  virtual_membase_ = nullptr;
  physical_membase_ = nullptr;
}

void* MemoryService::AllocateHost(void* address, size_t size, AllocationType type, PageAccess access) {
  return host::AllocFixed(address, size, type, access);
}

bool MemoryService::ReleaseHost(void* address, size_t size, DeallocationType type) {
  return host::DeallocFixed(address, size, type);
}

bool MemoryService::ProtectHost(void* address, size_t size, PageAccess access, PageAccess* old) {
  return host::Protect(address, size, access, old);
}

std::pair<uint32_t, uint32_t> MemoryService::InvalidatePhysicalRange(
    uint32_t start, uint32_t length, bool exact) {
  uint32_t first = 0, last = UINT32_MAX;
  for (auto callback : physical_memory_invalidation_callbacks_) {
    const auto allowed = callback->first(callback->second, start, length, exact);
    if (!exact) {
      first = std::max(first, allowed.first);
      last = std::min(last, uint32_t(std::min(uint64_t(allowed.first) + std::max(allowed.second, uint32_t(1)) - 1, uint64_t(UINT32_MAX))));
    }
  }
  return {first, last};
}

bool MemoryService::Initialize(bool protect_zero, const std::function<bool()>& install_mmio) {
  arena_ = std::make_unique<GuestArena>();
  if (!arena_->Initialize()) {
    GH_MEMORY_ERROR("Unable to map guest memory arena: {}", arena_->error());
    assert(false);
    return false;
  }
  file_name_ = arena_->name();
  virtual_membase_ = arena_->virtual_base();
  physical_membase_ = arena_->physical_base();

  GH_MEMORY_INFO("Guest memory arena mapped: virtual base 0x{:016X}, physical base 0x{:016X}",
              reinterpret_cast<uintptr_t>(virtual_membase_),
              reinterpret_cast<uintptr_t>(physical_membase_));

  // Prepare virtual heaps.
  heaps_.v00000000.Initialize(this, virtual_membase_, HeapType::kGuestVirtual, 0x00000000,
                              0x40000000, 4096);
  heaps_.v40000000.Initialize(this, virtual_membase_, HeapType::kGuestVirtual, 0x40000000,
                              0x40000000 - 0x01000000, 64 * 1024);
  heaps_.v80000000.Initialize(this, virtual_membase_, HeapType::kGuestXex, 0x80000000,
                              0x10000000, 64 * 1024);
  heaps_.v90000000.Initialize(this, virtual_membase_, HeapType::kGuestXex, 0x90000000,
                              0x10000000, 4096);

  // Prepare physical heaps.
  heaps_.physical.Initialize(this, physical_membase_, HeapType::kGuestPhysical, 0x00000000,
                             0x20000000, 4096);
  heaps_.vA0000000.Initialize(this, virtual_membase_, HeapType::kGuestPhysical, 0xA0000000,
                              0x20000000, 64 * 1024, &heaps_.physical);
  heaps_.vC0000000.Initialize(this, virtual_membase_, HeapType::kGuestPhysical, 0xC0000000,
                              0x20000000, 16 * 1024 * 1024, &heaps_.physical);
  heaps_.vE0000000.Initialize(this, virtual_membase_, HeapType::kGuestPhysical, 0xE0000000,
                              0x1FD00000, 4096, &heaps_.physical);

  // Protect the first and last 64kb of memory.
  heaps_.v00000000.AllocFixed(0x00000000, 0x10000, 0x10000,
                              kMemoryAllocationReserve | kMemoryAllocationCommit,
                              !protect_zero
                                  ? kMemoryProtectRead | kMemoryProtectWrite
                                  : kMemoryProtectNoAccess);
  heaps_.physical.AllocFixed(0x1FFF0000, 0x10000, 0x10000, kMemoryAllocationReserve,
                             kMemoryProtectNoAccess);

  // GPU writeback.
  // 0xC... is physical, 0x7F... is virtual. We may need to overlay these.
  heaps_.vC0000000.AllocFixed(0xC0000000, 0x01000000, 32,
                              kMemoryAllocationReserve | kMemoryAllocationCommit,
                              kMemoryProtectRead | kMemoryProtectWrite);

  // Pre-commit the physical memory range so the GPU can access it
  // without page faults. Reference: xenia-canary 5f5be0668.
  host::AllocFixed(heaps_.physical.TranslateRelative(0), heaps_.physical.heap_size(),
                          AllocationType::kCommit,
                          PageAccess::kReadWrite);

  if (!install_mmio()) {
    GH_MEMORY_ERROR("Unable to install MMIO handlers");
    return false;
  }
  GH_MEMORY_DEBUG("Installed MMIO handler for physical address translation");

  // ?
  uint32_t unk_phys_alloc;
  heaps_.vA0000000.Alloc(0x340000, 64 * 1024, kMemoryAllocationReserve,
                         kMemoryProtectNoAccess, true, &unk_phys_alloc);

  // Allocate region at start of XEX range. Title 544307D5 explicitly
  // accesses 0x8000001C and expects a specific constant value.
  // Reference: xenia-canary 78f97f8ff.
  uint32_t unknown_xex_range;
  heaps_.v80000000.Alloc(0x40000, 4 * 1024, kMemoryAllocationCommit,
                         kMemoryProtectRead | kMemoryProtectWrite, false,
                         &unknown_xex_range);

  uint32_t value_to_write = std::byteswap(uint32_t(0x2a6e3f38));
  std::memcpy(TranslateVirtual(0x80000000 + 0x1C), &value_to_write, sizeof(uint32_t));

  return true;
}

void MemoryService::Reset() {
  heaps_.v00000000.Reset();
  heaps_.v40000000.Reset();
  heaps_.v80000000.Reset();
  heaps_.v90000000.Reset();
  heaps_.physical.Reset();
}

const BaseHeap* MemoryService::LookupHeap(uint32_t address) const {
  if (address < 0x40000000) {
    return &heaps_.v00000000;
  } else if (address < 0x7F000000) {
    return &heaps_.v40000000;
  } else if (address < 0x80000000) {
    return nullptr;
  } else if (address < 0x90000000) {
    return &heaps_.v80000000;
  } else if (address < 0xA0000000) {
    return &heaps_.v90000000;
  } else if (address < 0xC0000000) {
    return &heaps_.vA0000000;
  } else if (address < 0xE0000000) {
    return &heaps_.vC0000000;
  } else if (address < 0xFFD00000) {
    return &heaps_.vE0000000;
  } else {
    return nullptr;
  }
}

BaseHeap* MemoryService::LookupHeapByType(bool physical, uint32_t page_size) {
  if (physical) {
    if (page_size <= 4096) {
      return &heaps_.vE0000000;
    } else if (page_size <= 64 * 1024) {
      return &heaps_.vA0000000;
    } else {
      return &heaps_.vC0000000;
    }
  } else {
    if (page_size <= 4096) {
      return &heaps_.v00000000;
    } else {
      return &heaps_.v40000000;
    }
  }
}

VirtualHeap* MemoryService::GetPhysicalHeap() {
  return &heaps_.physical;
}

uint32_t MemoryService::HostToGuestVirtual(const void* host_address) const {
  size_t virtual_address =
      reinterpret_cast<size_t>(host_address) - reinterpret_cast<size_t>(virtual_membase_);
  uint32_t vE0000000_host_offset = heaps_.vE0000000.host_address_offset();
  size_t vE0000000_host_base = size_t(heaps_.vE0000000.heap_base()) + vE0000000_host_offset;
  if (virtual_address >= vE0000000_host_base &&
      virtual_address <= (vE0000000_host_base + (heaps_.vE0000000.heap_size() - 1))) {
    virtual_address -= vE0000000_host_offset;
  }
  return uint32_t(virtual_address);
}

uint32_t MemoryService::GetPhysicalAddress(uint32_t address) const {
  const BaseHeap* heap = LookupHeap(address);
  if (!heap || heap->heap_type() != HeapType::kGuestPhysical) {
    return UINT32_MAX;
  }
  return static_cast<const PhysicalHeap*>(heap)->GetPhysicalAddress(address);
}

void MemoryService::Zero(uint32_t address, uint32_t size) {
  std::memset(TranslateVirtual(address), 0, size);
}

void MemoryService::Fill(uint32_t address, uint32_t size, uint8_t value) {
  std::memset(TranslateVirtual(address), value, size);
}

void MemoryService::Copy(uint32_t dest, uint32_t src, uint32_t size) {
  uint8_t* pdest = TranslateVirtual(dest);
  const uint8_t* psrc = TranslateVirtual(src);
  std::memcpy(pdest, psrc, size);
}

uint32_t MemoryService::SearchAligned(uint32_t start, uint32_t end, const uint32_t* values,
                               size_t value_count) {
  assert(start <= end);
  auto p = TranslateVirtual<const uint32_t*>(start);
  auto pe = TranslateVirtual<const uint32_t*>(end);
  while (p != pe) {
    if (*p == values[0]) {
      const uint32_t* pc = p + 1;
      size_t matched = 1;
      for (size_t n = 1; n < value_count; n++, pc++) {
        if (*pc != values[n]) {
          break;
        }
        matched++;
      }
      if (matched == value_count) {
        return HostToGuestVirtual(p);
      }
    }
    p++;
  }
  return 0;
}

bool MemoryService::AccessViolationCallback(std::unique_lock<std::recursive_mutex> global_lock_locked_once,
                                     void* host_address, bool is_write) {
  // Access via physical_membase_ is special, when need to bypass everything
  // (for instance, for a data provider to actually write the data) so only
  // triggering callbacks on virtual memory regions.
  if (reinterpret_cast<size_t>(host_address) < reinterpret_cast<size_t>(virtual_membase_) ||
      reinterpret_cast<size_t>(host_address) >= reinterpret_cast<size_t>(physical_membase_)) {
    return false;
  }
  uint32_t virtual_address = HostToGuestVirtual(host_address);
  BaseHeap* heap = LookupHeap(virtual_address);
  if (!heap || heap->heap_type() != HeapType::kGuestPhysical) {
    GH_MEMORY_ERROR(
        "Unhandled guest access violation: {} of guest 0x{:08X} (host 0x{:016X}) on thread 0x{:X}",
        is_write ? "write" : "read", virtual_address, reinterpret_cast<uintptr_t>(host_address),
        std::hash<std::thread::id>{}(std::this_thread::get_id()));
    return false;
  }

  // Access violation callbacks from the guest are triggered when the global
  // critical region mutex is locked once.
  //
  // Will be rounded to physical page boundaries internally, so just pass 1 as
  // the length - guranteed not to cross page boundaries also.
  auto physical_heap = static_cast<PhysicalHeap*>(heap);
  if (physical_heap->TriggerCallbacks(std::move(global_lock_locked_once), virtual_address, 1,
                                      is_write, false)) {
    return true;
  }

  // Recovery path for stale host protection state in physical memory:
  // if guest metadata says the page is writable but host protection is still
  // read-only / no-access, restore write access and resume execution.
  if (is_write) {
    constexpr uint32_t kWriteProtectMask =
        kMemoryProtectWrite | kMemoryProtectWriteCombine;
    uint32_t guest_protect = 0;
    bool allow_write = false;
    if (heap->QueryProtect(virtual_address, &guest_protect) &&
        (guest_protect & kWriteProtectMask)) {
      allow_write = true;
    } else {
      // If write-watch left current protect stale, trust committed allocation
      // metadata first for this alias.
      HeapAllocationInfo guest_info{};
      if (heap->QueryRegionInfo(virtual_address, &guest_info) &&
          (guest_info.state & kMemoryAllocationCommit) &&
          (guest_info.allocation_protect & kWriteProtectMask)) {
        guest_protect = guest_info.protect;
        allow_write = true;
      } else {
        // Alias-aware fallback: consult canonical 0x00000000 physical heap
        // metadata when this alias has stale tracking.
        uint32_t physical_address = GetPhysicalAddress(virtual_address);
        if (physical_address != UINT32_MAX) {
          uint32_t physical_protect = 0;
          if (heaps_.physical.QueryProtect(physical_address, &physical_protect) &&
              (physical_protect & kWriteProtectMask)) {
            guest_protect = physical_protect;
            allow_write = true;
          } else {
            HeapAllocationInfo physical_info{};
            if (heaps_.physical.QueryRegionInfo(physical_address, &physical_info) &&
                (physical_info.state & kMemoryAllocationCommit) &&
                (physical_info.allocation_protect & kWriteProtectMask)) {
              guest_protect = physical_info.protect;
              allow_write = true;
            }
          }
        }
      }
    }
    if (allow_write) {
      size_t host_page_size = host::page_size();
      uintptr_t page_base =
          reinterpret_cast<uintptr_t>(host_address) & ~(uintptr_t(host_page_size - 1));
      if (host::Protect(reinterpret_cast<void*>(page_base), host_page_size,
                               PageAccess::kReadWrite, nullptr)) {
        GH_MEMORY_WARN(
            "Recovered stale physical page protection for guest {:08X} (host {:016X}, "
            "guest_protect {:08X})",
            virtual_address, static_cast<uint64_t>(page_base), guest_protect);
        return true;
      }
    }

    uint32_t current_guest_protect = 0;
    heap->QueryProtect(virtual_address, &current_guest_protect);
    uint32_t physical_address = GetPhysicalAddress(virtual_address);
    uint32_t physical_protect = 0;
    if (physical_address != UINT32_MAX) {
      heaps_.physical.QueryProtect(physical_address, &physical_protect);
    }
    GH_MEMORY_ERROR(
        "Unhandled guest physical write fault: guest={:08X} host={:016X} phys={:08X} "
        "guest_protect={:08X} physical_protect={:08X}",
        virtual_address, static_cast<uint64_t>(reinterpret_cast<uintptr_t>(host_address)),
        physical_address, current_guest_protect, physical_protect);
  }

  return false;
}

bool MemoryService::TriggerPhysicalMemoryCallbacks(
    std::unique_lock<std::recursive_mutex> global_lock_locked_once, uint32_t virtual_address,
    uint32_t length, bool is_write, bool unwatch_exact_range, bool unprotect) {
  BaseHeap* heap = LookupHeap(virtual_address);
  if (!heap) {
    return false;
  }
  if (heap->heap_type() == HeapType::kGuestPhysical) {
    auto physical_heap = static_cast<PhysicalHeap*>(heap);
    return physical_heap->TriggerCallbacks(std::move(global_lock_locked_once), virtual_address,
                                           length, is_write, unwatch_exact_range, unprotect);
  }
  return false;
}

void* MemoryService::RegisterPhysicalMemoryInvalidationCallback(
    PhysicalMemoryInvalidationCallback callback, void* callback_context) {
  auto entry = new std::pair<PhysicalMemoryInvalidationCallback, void*>(callback, callback_context);
  auto lock = std::unique_lock(global_mutex_);
  physical_memory_invalidation_callbacks_.push_back(entry);
  return entry;
}

void MemoryService::UnregisterPhysicalMemoryInvalidationCallback(void* callback_handle) {
  auto entry =
      reinterpret_cast<std::pair<PhysicalMemoryInvalidationCallback, void*>*>(callback_handle);
  {
    auto lock = std::unique_lock(global_mutex_);
    auto it = std::find(physical_memory_invalidation_callbacks_.begin(),
                        physical_memory_invalidation_callbacks_.end(), entry);
    assert(it != physical_memory_invalidation_callbacks_.end());
    if (it != physical_memory_invalidation_callbacks_.end()) {
      physical_memory_invalidation_callbacks_.erase(it);
    }
  }
  delete entry;
}

void MemoryService::EnablePhysicalMemoryAccessCallbacks(uint32_t physical_address, uint32_t length,
                                                 bool enable_invalidation_notifications,
                                                 bool enable_data_providers) {
  heaps_.vA0000000.EnableAccessCallbacks(physical_address, length,
                                         enable_invalidation_notifications, enable_data_providers);
  heaps_.vC0000000.EnableAccessCallbacks(physical_address, length,
                                         enable_invalidation_notifications, enable_data_providers);
  heaps_.vE0000000.EnableAccessCallbacks(physical_address, length,
                                         enable_invalidation_notifications, enable_data_providers);
}

uint32_t MemoryService::SystemHeapAlloc(uint32_t size, uint32_t alignment, bool is_physical) {
  // TODO(benvanik): lightweight pool.
  auto heap = LookupHeapByType(is_physical, 4096);
  uint32_t address;
  if (!heap->AllocSystemHeap(
          size, alignment, kMemoryAllocationReserve | kMemoryAllocationCommit,
          kMemoryProtectRead | kMemoryProtectWrite, false, &address)) {
    return 0;
  }
  Zero(address, size);
  return address;
}

void MemoryService::SystemHeapFree(uint32_t address, uint32_t* out_region_size) {
  if (!address) {
    return;
  }
  // TODO(benvanik): lightweight pool.
  auto heap = LookupHeap(address);
  heap->Release(address, out_region_size);
}

void MemoryService::GetHeapsPageStatsSummary(const BaseHeap* const* provided_heaps, size_t heaps_count,
                                      uint32_t& unreserved_pages, uint32_t& reserved_pages,
                                      uint32_t& used_pages, uint32_t& reserved_bytes) {
  // Heap array is fixed after init; page counts are approximate statistics.
  for (size_t i = 0; i < heaps_count; i++) {
    const BaseHeap* heap = provided_heaps[i];
    uint32_t heap_unreserved = heap->unreserved_page_count();
    uint32_t heap_reserved = heap->reserved_page_count();

    unreserved_pages += heap_unreserved;
    reserved_pages += heap_reserved;
    used_pages += ((heap->total_page_count() - heap_unreserved) * heap->page_size()) / 4096;
    reserved_bytes += heap_reserved * heap->page_size();
  }
}

void MemoryService::DumpMap() {
  GH_MEMORY_ERROR("==================================================================");
  GH_MEMORY_ERROR("Memory Dump");
  GH_MEMORY_ERROR("==================================================================");
  GH_MEMORY_ERROR("               System Page Size: {0} ({0:08X})", system_page_size_);
  GH_MEMORY_ERROR("  System Allocation Granularity: {0} ({0:08X})", system_allocation_granularity_);
  GH_MEMORY_ERROR("                Virtual Membase: {}", reinterpret_cast<uintptr_t>(virtual_membase_));
  GH_MEMORY_ERROR("               Physical Membase: {}", reinterpret_cast<uintptr_t>(physical_membase_));
  GH_MEMORY_ERROR("");
  GH_MEMORY_ERROR("------------------------------------------------------------------");
  GH_MEMORY_ERROR("Virtual Heaps");
  GH_MEMORY_ERROR("------------------------------------------------------------------");
  GH_MEMORY_ERROR("");
  heaps_.v00000000.DumpMap();
  heaps_.v40000000.DumpMap();
  heaps_.v80000000.DumpMap();
  heaps_.v90000000.DumpMap();
  GH_MEMORY_ERROR("");
  GH_MEMORY_ERROR("------------------------------------------------------------------");
  GH_MEMORY_ERROR("Physical Heaps");
  GH_MEMORY_ERROR("------------------------------------------------------------------");
  GH_MEMORY_ERROR("");
  heaps_.physical.DumpMap();
  heaps_.vA0000000.DumpMap();
  heaps_.vC0000000.DumpMap();
  heaps_.vE0000000.DumpMap();
  GH_MEMORY_ERROR("");
}

bool MemoryService::Save(HeapStream* stream) {
  GH_MEMORY_DEBUG("Serializing memory...");
  return heaps_.v00000000.Save(stream) && heaps_.v40000000.Save(stream) &&
      heaps_.v80000000.Save(stream) && heaps_.v90000000.Save(stream) &&
      heaps_.physical.Save(stream);
}

bool MemoryService::Restore(HeapStream* stream) {
  GH_MEMORY_DEBUG("Restoring memory...");
  return heaps_.v00000000.Restore(stream) && heaps_.v40000000.Restore(stream) &&
      heaps_.v80000000.Restore(stream) && heaps_.v90000000.Restore(stream) &&
      heaps_.physical.Restore(stream);
}

bool MemoryService::PrepareMappedRange(uint32_t virtual_address, uint32_t size) {
  // AllocFixed rejects requests that don't cover whole host pages, so that a
  // guest heap can never widen an mprotect over a neighbouring guest page that
  // shares the host page. An MMIO window has no such neighbours - it owns its
  // entire range - so rounding out to host page bounds is safe here, and it is
  // required because callers describe these windows with mask-style sizes such
  // as 0xFFFF, which is one byte short of 64 KB and therefore never a multiple
  // of the 16 KB host page on Apple Silicon.
  const size_t host_page_size = host::page_size();
  uint8_t* const range_start = TranslateVirtual(virtual_address);
  uint8_t* const aligned_start = reinterpret_cast<uint8_t*>(
      reinterpret_cast<uintptr_t>(range_start) & ~(uintptr_t(host_page_size) - 1));
  const size_t aligned_length =
      RoundUp(size_t(range_start - aligned_start) + size, host_page_size);
  if (!host::AllocFixed(aligned_start, aligned_length, AllocationType::kCommit,
                               PageAccess::kNoAccess)) {
    GH_MEMORY_ERROR("Unable to map range; commit/protect failed");
    return false;
  }
  return true;
}

}  // namespace gh::memory
