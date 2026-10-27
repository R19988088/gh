#include "gh_heap.h"
#include <algorithm>
#include <bit>
#include <cassert>
#include <climits>
#include <limits>
#include <format>

#define GH_HEAP_ERROR(...) host_->HeapLog(3, std::format(__VA_ARGS__))
#define GH_HEAP_WARN(...) host_->HeapLog(2, std::format(__VA_ARGS__))
#define GH_HEAP_DEBUG(...) host_->HeapLog(0, std::format(__VA_ARGS__))

namespace gh::memory {
namespace {
template <typename T> constexpr T RoundUp(T value, T alignment) {
  if (!value) return alignment;
  return (value + alignment - 1) / alignment * alignment;
}
template <typename T> constexpr T Align(T value, T alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}
constexpr uint32_t SatSub(uint32_t a, uint32_t b) { return a > b ? a - b : 0; }
uint32_t get_page_count(uint32_t value, uint32_t page_size, uint32_t shift) {
  return RoundUp(value, page_size) >> shift;
}
}  // namespace

PageAccess ToPageAccess(uint32_t protect) {
  bool is_writable =
      (protect & kMemoryProtectWrite) || (protect & kMemoryProtectWriteCombine);

  if ((protect & kMemoryProtectRead) && !is_writable) {
    return PageAccess::kReadOnly;
  } else if ((protect & kMemoryProtectRead) && is_writable) {
    return PageAccess::kReadWrite;
  } else {
    return PageAccess::kNoAccess;
  }
}

uint32_t FromPageAccess(PageAccess protect) {
  switch (protect) {
    case PageAccess::kNoAccess:
      return kMemoryProtectNoAccess;
    case PageAccess::kReadOnly:
      return kMemoryProtectRead;
    case PageAccess::kReadWrite:
      return kMemoryProtectRead | kMemoryProtectWrite;
    case PageAccess::kExecuteReadOnly:
      // Guest memory cannot be executable - this should never happen :)
      assert(false);
      return kMemoryProtectRead;
    case PageAccess::kExecuteReadWrite:
      // Guest memory cannot be executable - this should never happen :)
      assert(false);
      return kMemoryProtectRead | kMemoryProtectWrite;
  }

  return kMemoryProtectNoAccess;
}

BaseHeap::BaseHeap() : membase_(nullptr), heap_base_(0), heap_size_(0), page_size_(0) {}

BaseHeap::~BaseHeap() = default;

std::unique_lock<std::recursive_mutex> BaseHeap::AcquireHostPageReconcileLock() const {
  if (page_size_ >= host_->HostPageSize()) {
    return {};
  }
  return std::unique_lock<std::recursive_mutex>(host_->GlobalMutex());
}

bool BaseHeap::SyncHostPageAccess(uint32_t start_page_number, uint32_t end_page_number) {
  const uint32_t host_page_size = host_->HostPageSize();
  if (page_size_ >= host_page_size) {
    return true;
  }

  // The global critical region guards the watch flags read below and must
  // already be held by the caller (AcquireHostPageReconcileLock), acquired
  // ahead of heap_mutex_ so this never inverts against PhysicalHeap.

  // Multiple guest pages may share one host page. Keep the guest page table as
  // the source of truth, then grant the host page the union of the access needed
  // by its committed guest pages. This can't enforce stricter access for one
  // guest page while a neighbor needs broader access, but it keeps neighboring
  // committed pages usable and allows guest protection metadata to stay exact.
  const auto get_host_page_access = [&](uint32_t host_page_number) {
    const uint64_t host_page_start = uint64_t(host_page_number) * host_page_size;
    const uint64_t host_page_end = host_page_start + host_page_size;
    const uint64_t heap_host_start = host_address_offset_;
    const uint64_t heap_host_end = heap_host_start + heap_size_;
    const uint64_t overlap_start = std::max(host_page_start, heap_host_start);
    const uint64_t overlap_end = std::min(host_page_end, heap_host_end);
    if (overlap_start >= overlap_end) {
      return PageAccess::kNoAccess;
    }

    const uint32_t guest_page_start = uint32_t(overlap_start - heap_host_start) >> page_size_shift_;
    const uint32_t guest_page_end = uint32_t(overlap_end - heap_host_start - 1) >> page_size_shift_;

    bool has_read = false;
    bool has_write = false;
    bool has_committed = false;
    for (uint32_t page_number = guest_page_start; page_number <= guest_page_end; ++page_number) {
      const auto& page_entry = page_table_[page_number];
      if (!(page_entry.state & kMemoryAllocationCommit)) {
        continue;
      }
      has_committed = true;
      const PageAccess page_access = ToPageAccess(page_entry.current_protect);
      if (page_access == PageAccess::kReadWrite) {
        has_read = true;
        has_write = true;
        break;
      }
      if (page_access == PageAccess::kReadOnly) {
        has_read = true;
      }
    }

    PageAccess access;
    // Physical aliases are intentionally left accessible after release unless
    // protect_on_release is enabled because GPU users may still reference them.
    if (!has_committed && heap_type_ == HeapType::kGuestPhysical &&
        !host_->ProtectOnRelease()) {
      access = PageAccess::kReadWrite;
    } else if (!has_read) {
      access = PageAccess::kNoAccess;
    } else {
      access = has_write ? PageAccess::kReadWrite : PageAccess::kReadOnly;
    }

    // The guest page table is not the whole truth: a physical heap page may
    // also be write-watched for GPU invalidation. EnableAccessCallbacks records
    // that in notify_on_invalidation and skips any page whose bit is already
    // set, so handing write access back here would stop the watch firing
    // permanently. One host page covers several guest pages, so an unrelated
    // operation on a neighbour reaches this code constantly - clamp instead.
    // Leaving the page read-only can only cause a spurious invalidation, which
    // is safe; restoring write access loses invalidations, which is not.
    if (access == PageAccess::kReadWrite && IsHostPageWriteWatched(host_page_number)) {
      access = PageAccess::kReadOnly;
    }
    return access;
  };

  const uint64_t guest_byte_start = uint64_t(start_page_number) << page_size_shift_;
  const uint64_t guest_byte_end = ((uint64_t(end_page_number) + 1) << page_size_shift_) - 1;
  const uint32_t host_page_first =
      uint32_t((guest_byte_start + host_address_offset_) / host_page_size);
  const uint32_t host_page_last =
      uint32_t((guest_byte_end + host_address_offset_) / host_page_size);

  // Coalesce neighbouring host pages that resolve to the same access into one
  // Protect call. A bulk commit spans thousands of host pages and almost always
  // wants a single uniform access for all of them, so protecting page by page
  // would cost thousands of syscalls where one suffices.
  uint8_t* const heap_host_base = membase_ + heap_base_;
  uint32_t run_first_page = host_page_first;
  PageAccess run_access = get_host_page_access(host_page_first);
  const auto flush_run = [&](uint32_t run_last_page) {
    const size_t run_length = (size_t(run_last_page - run_first_page) + 1) * host_page_size;
    uint8_t* const run_pointer = heap_host_base + size_t(run_first_page) * host_page_size;
    if (!host_->ProtectHost(run_pointer, run_length, run_access, nullptr)) {
      GH_HEAP_ERROR("BaseHeap::SyncHostPageAccess failed for heap {:08X} host pages {}-{}",
                   heap_base_, run_first_page, run_last_page);
      return false;
    }
    return true;
  };

  for (uint32_t host_page = host_page_first + 1; host_page <= host_page_last; ++host_page) {
    const PageAccess access = get_host_page_access(host_page);
    if (access == run_access) {
      continue;
    }
    if (!flush_run(host_page - 1)) {
      return false;
    }
    run_first_page = host_page;
    run_access = access;
  }
  return flush_run(host_page_last);
}

void BaseHeap::Initialize(HeapHost* host, uint8_t* membase, HeapType heap_type,
                          uint32_t heap_base, uint32_t heap_size, uint32_t page_size,
                          uint32_t host_address_offset) {
  host_ = host;
  membase_ = membase;
  heap_type_ = heap_type;
  heap_base_ = heap_base;
  heap_size_ = heap_size;
  page_size_ = page_size;
  assert(std::has_single_bit(page_size_));
  page_size_shift_ = std::countr_zero(page_size_);
  host_address_offset_ = host_address_offset;
  page_table_.resize(heap_size / page_size);
  unreserved_page_count_ = uint32_t(page_table_.size());
}

void BaseHeap::Dispose() {
  // Walk table and release all regions.
  for (uint32_t page_number = 0; page_number < page_table_.size(); ++page_number) {
    auto& page_entry = page_table_[page_number];
    if (page_entry.state) {
      host_->ReleaseHost(TranslateRelative(page_number << page_size_shift_), 0,
                                DeallocationType::kRelease);
      page_number += page_entry.region_page_count;
    }
  }
}

void BaseHeap::DumpMap() {
  std::lock_guard<std::recursive_mutex> heap_lock(heap_mutex_);
  GH_HEAP_ERROR("------------------------------------------------------------------");
  GH_HEAP_ERROR("Heap: {:08X}-{:08X}", heap_base_, heap_base_ + (heap_size_ - 1));
  GH_HEAP_ERROR("------------------------------------------------------------------");
  GH_HEAP_ERROR("            Heap Base: {:08X}", heap_base_);
  GH_HEAP_ERROR("            Heap Size: {0} ({0:08X})", heap_size_);
  GH_HEAP_ERROR("            Page Size: {0} ({0:08X})", page_size_);
  GH_HEAP_ERROR("           Page Count: {}", page_table_.size());
  GH_HEAP_ERROR("  Host Address Offset: {0} ({0:08X})", host_address_offset_);
  bool is_empty_span = false;
  uint32_t empty_span_start = 0;
  for (uint32_t i = 0; i < uint32_t(page_table_.size()); ++i) {
    auto& page = page_table_[i];
    if (!page.state) {
      if (!is_empty_span) {
        is_empty_span = true;
        empty_span_start = i;
      }
      continue;
    }
    if (is_empty_span) {
      GH_HEAP_ERROR("  {:08X}-{:08X} {:6d}p {:10d}b unreserved",
                   heap_base_ + (empty_span_start << page_size_shift_),
                   heap_base_ + (i << page_size_shift_), i - empty_span_start,
                   (i - empty_span_start) << page_size_shift_);
      is_empty_span = false;
    }
    const char* state_name = "   ";
    if (page.state & kMemoryAllocationCommit) {
      state_name = "COM";
    } else if (page.state & kMemoryAllocationReserve) {
      state_name = "RES";
    }
    char access_r = (page.current_protect & kMemoryProtectRead) ? 'R' : ' ';
    char access_w = (page.current_protect & kMemoryProtectWrite) ? 'W' : ' ';
    uint32_t region_pages = page.region_page_count;
    GH_HEAP_ERROR("  {:08X}-{:08X} {:6d}p {:10d}b {} {}{}", heap_base_ + (i << page_size_shift_),
                 heap_base_ + ((i + region_pages) << page_size_shift_), region_pages,
                 region_pages << page_size_shift_, state_name, access_r, access_w);
    i += region_pages - 1;
  }
  if (is_empty_span) {
    GH_HEAP_ERROR("  {:08X}-{:08X} - {} unreserved pages)",
                 heap_base_ + (empty_span_start << page_size_shift_), heap_base_ + (heap_size_ - 1),
                 page_table_.size() - empty_span_start);
  }
}

uint32_t BaseHeap::GetTotalPageCount() {
  return uint32_t(page_table_.size());
}

uint32_t BaseHeap::GetUnreservedPageCount() {
  std::lock_guard<std::recursive_mutex> heap_lock(heap_mutex_);
  uint32_t count = 0;
  bool is_empty_span = false;
  uint32_t empty_span_start = 0;
  uint32_t size = uint32_t(page_table_.size());
  for (uint32_t i = 0; i < size; ++i) {
    auto& page = page_table_[i];
    if (!page.state) {
      if (!is_empty_span) {
        is_empty_span = true;
        empty_span_start = i;
      }
      continue;
    }
    if (is_empty_span) {
      is_empty_span = false;
      count += i - empty_span_start;
    }
    i += page.region_page_count - 1;
  }
  if (is_empty_span) {
    count += size - empty_span_start;
  }
  return count;
}

bool BaseHeap::Save(HeapStream* stream) {
  GH_HEAP_DEBUG("Heap {:08X}-{:08X}", heap_base_, heap_base_ + (heap_size_ - 1));
  auto global_lock = AcquireHostPageReconcileLock();
  std::lock_guard<std::recursive_mutex> heap_lock(heap_mutex_);
  const size_t host_page_size = host_->HostPageSize();

  for (size_t i = 0; i < page_table_.size(); i++) {
    auto& page = page_table_[i];
    stream->Write(page.qword);
    if (!page.state) {
      // Unallocated.
      continue;
    }

    // TODO(DrChat): write compressed with snappy.
    if (page.state & kMemoryAllocationCommit) {
      void* addr = TranslateRelative(i << page_size_shift_);

      // A 4KiB guest page can share a 16KiB host page. mprotect must receive
      // the whole aligned host page, including when reading the protected zero
      // page. Restore the exact host access after each serialized guest page.
      void* protection_address = addr;
      size_t protection_length = page_size_;
      if (page_size_ < host_page_size) {
        protection_address = reinterpret_cast<void*>(
            reinterpret_cast<uintptr_t>(addr) & ~(uintptr_t(host_page_size) - 1));
        protection_length = host_page_size;
      }
      PageAccess old_access;
      if (!host_->ProtectHost(protection_address, protection_length,
                              PageAccess::kReadWrite, &old_access)) {
        return false;
      }

      stream->Write(addr, page_size_);

      if (!host_->ProtectHost(protection_address, protection_length, old_access, nullptr)) {
        return false;
      }
    }
  }

  return true;
}

bool BaseHeap::Restore(HeapStream* stream) {
  GH_HEAP_DEBUG("Heap {:08X}-{:08X}", heap_base_, heap_base_ + (heap_size_ - 1));

  const uint32_t host_page_size = host_->HostPageSize();
  const bool reconcile_host_pages = page_size_ < host_page_size;
  uint32_t writable_host_page = UINT32_MAX;

  // Restore doesn't take heap_mutex_, but SyncHostPageAccess below still needs
  // the global critical region held by its caller.
  auto global_lock = AcquireHostPageReconcileLock();

  for (size_t i = 0; i < page_table_.size(); i++) {
    auto& page = page_table_[i];
    page.qword = stream->Read<uint64_t>();
    if (!page.state) {
      // Unallocated.
      continue;
    }

    PageAccess page_access = PageAccess::kNoAccess;
    if ((page.current_protect & kMemoryProtectRead) &&
        (page.current_protect & kMemoryProtectWrite)) {
      page_access = PageAccess::kReadWrite;
    } else if (page.current_protect & kMemoryProtectRead) {
      page_access = PageAccess::kReadOnly;
    }

    if (!(page.state & kMemoryAllocationCommit)) {
      continue;
    }

    void* addr = TranslateRelative(i << page_size_shift_);
    if (!reconcile_host_pages) {
      // Commit the memory if it isn't already. We do not need to reserve any
      // memory, as the mapping has already taken care of that.
      host_->AllocateHost(addr, page_size_, AllocationType::kCommit,
                              PageAccess::kReadWrite);
      // Read into memory with R/W protection, then restore the saved
      // protection.
      // TODO(DrChat): read compressed with snappy.
      host_->ProtectHost(addr, page_size_, PageAccess::kReadWrite, nullptr);
      stream->Read(addr, page_size_);
      host_->ProtectHost(addr, page_size_, page_access, nullptr);
      continue;
    }

    // Guest pages are smaller than host pages, so a per-guest-page commit is
    // rejected and a per-guest-page protect is misaligned - either way the
    // target would never become writable and the read below would fault or be
    // silently lost. Make the containing host page writable once instead;
    // guest pages are visited in ascending order, so one commit covers the
    // whole run that shares a host page. Real protections are reconciled from
    // the restored page table after the loop.
    const uint32_t host_page =
        uint32_t(((uint64_t(i) << page_size_shift_) + host_address_offset_) / host_page_size);
    if (host_page != writable_host_page) {
      uint8_t* host_page_pointer = membase_ + heap_base_ + size_t(host_page) * host_page_size;
      if (!host_->AllocateHost(host_page_pointer, host_page_size,
                                   AllocationType::kCommit,
                                   PageAccess::kReadWrite)) {
        GH_HEAP_ERROR("BaseHeap::Restore failed to make host page {} writable", host_page);
        return false;
      }
      writable_host_page = host_page;
    }
    stream->Read(addr, page_size_);
  }

  if (reconcile_host_pages && !page_table_.empty()) {
    if (!SyncHostPageAccess(0, uint32_t(page_table_.size()) - 1)) {
      return false;
    }
  }

  return true;
}

void BaseHeap::Reset() {
  // TODO(DrChat): protect pages.
  std::memset(page_table_.data(), 0, sizeof(PageEntry) * page_table_.size());
  // TODO(Triang3l): Remove access callbacks from pages if this is a physical
  // memory heap.
}

bool BaseHeap::Alloc(uint32_t size, uint32_t alignment, uint32_t allocation_type, uint32_t protect,
                     bool top_down, uint32_t* out_address) {
  *out_address = 0;
  size = RoundUp(size, page_size_);
  alignment = RoundUp(alignment, page_size_);

  // Reserve address space at the top for thread stacks.
  // 0x3XXXXXXX is for system threads, 0x7XXXXXXX is for title threads.
  uint32_t heap_virtual_guest_offset = 0;
  if (heap_type_ == HeapType::kGuestVirtual) {
    heap_virtual_guest_offset = 0x10000000;
    if (page_size_ == 0x10000) {
      heap_virtual_guest_offset = 0x0F000000;
    }
  }

  uint32_t low_address = heap_base_;
  uint32_t high_address = heap_base_ + (heap_size_ - 1) - heap_virtual_guest_offset;
  return AllocRange(low_address, high_address, size, alignment, allocation_type, protect, top_down,
                    out_address);
}

bool BaseHeap::AllocSystemHeap(uint32_t size, uint32_t alignment, uint32_t allocation_type,
                               uint32_t protect, bool top_down, uint32_t* out_address) {
  *out_address = 0;
  size = RoundUp(size, page_size_);
  alignment = RoundUp(alignment, page_size_);

  uint32_t low_address = heap_base_;
  if (heap_type_ == HeapType::kGuestVirtual) {
    low_address = heap_base_ + heap_size_ - 0x10000000;
  }
  uint32_t high_address = heap_base_ + (heap_size_ - 1);
  return AllocRange(low_address, high_address, size, alignment, allocation_type, protect, top_down,
                    out_address);
}

bool BaseHeap::AllocFixed(uint32_t base_address, uint32_t size, uint32_t alignment,
                          uint32_t allocation_type, uint32_t protect) {
  alignment = RoundUp(alignment, page_size_);
  if (base_address % alignment != 0) {
    if (base_address % page_size_ != 0) {
      GH_HEAP_ERROR(
          "BaseHeap::AllocFixed invalid base alignment: base={:08X} page_size={:08X} "
          "requested_alignment={:08X} heap={:08X}-{:08X}",
          base_address, page_size_, alignment, heap_base_, heap_base_ + (heap_size_ - 1));
      return false;
    }
    // Fixed allocations can only be guaranteed page-aligned. If callers provide
    // a stricter alignment for an already-fixed address, fall back to page size.
    GH_HEAP_WARN(
        "BaseHeap::AllocFixed clamping alignment from {:08X} to page size {:08X} for "
        "base={:08X}",
        alignment, page_size_, base_address);
    alignment = page_size_;
  }
  size = Align(size, alignment);
  uint32_t page_count = get_page_count(size, page_size_, page_size_shift_);
  uint32_t start_page_number = (base_address - heap_base_) >> page_size_shift_;
  uint32_t end_page_number = start_page_number + page_count - 1;
  if (start_page_number >= page_table_.size() || end_page_number > page_table_.size()) {
    GH_HEAP_ERROR("BaseHeap::AllocFixed passed out of range address range");
    return false;
  }

  // Global critical region before heap_mutex_ - see
  // AcquireHostPageReconcileLock for why the order matters.
  auto global_lock = AcquireHostPageReconcileLock();
  std::lock_guard<std::recursive_mutex> heap_lock(heap_mutex_);

  // - If we are reserving the entire range requested must not be already
  //   reserved.
  // - If we are committing it's ok for pages within the range to already be
  //   committed.
  for (uint32_t page_number = start_page_number; page_number <= end_page_number; ++page_number) {
    uint32_t state = page_table_[page_number].state;
    if ((allocation_type == kMemoryAllocationReserve) && state) {
      // Already reserved.
      GH_HEAP_ERROR(
          "BaseHeap::AllocFixed attempting to reserve an already reserved "
          "range");
      return false;
    }
    if ((allocation_type == kMemoryAllocationCommit) &&
        !(state & kMemoryAllocationReserve)) {
      // Attempting a commit-only op on an unreserved page.
      // This may be OK.
      GH_HEAP_WARN("BaseHeap::AllocFixed attempting commit on unreserved page");
      allocation_type |= kMemoryAllocationReserve;
      break;
    }
  }

  // Allocate from host.
  if (allocation_type == kMemoryAllocationReserve) {
    // Reserve is not needed, as we are mapped already.
  } else if (page_size_ >= host_->HostPageSize()) {
    auto alloc_type = (allocation_type & kMemoryAllocationCommit)
                          ? AllocationType::kCommit
                          : AllocationType::kReserve;
    void* result =
        host_->AllocateHost(TranslateRelative(start_page_number << page_size_shift_),
                                page_count << page_size_shift_, alloc_type, ToPageAccess(protect));
    if (!result) {
      GH_HEAP_ERROR("BaseHeap::AllocFixed failed to alloc range from host");
      return false;
    }
  }

  // Set page state.
  for (uint32_t page_number = start_page_number; page_number <= end_page_number; ++page_number) {
    auto& page_entry = page_table_[page_number];
    if (allocation_type & kMemoryAllocationReserve) {
      if (!page_entry.state) {
        unreserved_page_count_--;
      }
      // Region is based on reservation.
      page_entry.base_address = start_page_number;
      page_entry.region_page_count = page_count;
    }
    page_entry.allocation_protect = protect;
    page_entry.current_protect = protect;
    page_entry.state = kMemoryAllocationReserve | allocation_type;
  }

  if (!SyncHostPageAccess(start_page_number, end_page_number)) {
    return false;
  }

  if (allocation_type != kMemoryAllocationReserve && host_->ScribbleHeap() &&
      (protect & kMemoryProtectWrite)) {
    std::memset(TranslateRelative(start_page_number << page_size_shift_), 0xCD,
                page_count << page_size_shift_);
  }

  return true;
}

bool BaseHeap::AllocRange(uint32_t low_address, uint32_t high_address, uint32_t size,
                          uint32_t alignment, uint32_t allocation_type, uint32_t protect,
                          bool top_down, uint32_t* out_address) {
  *out_address = 0;

  alignment = RoundUp(alignment, page_size_);
  uint32_t page_count = get_page_count(size, page_size_, page_size_shift_);
  low_address = std::max(heap_base_, Align(low_address, alignment));
  high_address = std::min(heap_base_ + (heap_size_ - 1), Align(high_address, alignment));
  if (high_address < low_address) {
    GH_HEAP_ERROR("BaseHeap::Alloc invalid requested range");
    return false;
  }
  uint32_t low_page_number = (low_address - heap_base_) >> page_size_shift_;
  uint32_t high_page_number = (high_address - heap_base_) >> page_size_shift_;
  low_page_number = std::min(uint32_t(page_table_.size()) - 1, low_page_number);
  high_page_number = std::min(uint32_t(page_table_.size()) - 1, high_page_number);
  if (high_page_number < low_page_number) {
    GH_HEAP_ERROR("BaseHeap::Alloc invalid requested page range");
    return false;
  }

  uint32_t available_page_count = high_page_number - low_page_number + 1;
  if (!page_count || page_count > available_page_count) {
    GH_HEAP_ERROR("BaseHeap::Alloc page count too big for requested range");
    return false;
  }

  // Global critical region before heap_mutex_ - see
  // AcquireHostPageReconcileLock for why the order matters.
  auto global_lock = AcquireHostPageReconcileLock();
  std::lock_guard<std::recursive_mutex> heap_lock(heap_mutex_);

  // Find a free page range.
  // The base page must match the requested alignment, so we first scan for
  // a free aligned page and only then check for continuous free pages.
  // TODO(benvanik): optimized searching (free list buckets, bitmap, etc).
  uint32_t start_page_number = UINT_MAX;
  uint32_t end_page_number = UINT_MAX;
  uint32_t page_scan_stride = alignment >> page_size_shift_;
  uint32_t max_base_page_number = high_page_number + 1 - page_count;
  if (top_down) {
    max_base_page_number -= max_base_page_number % page_scan_stride;
    for (int64_t base_page_number = max_base_page_number; base_page_number >= low_page_number;
         base_page_number -= page_scan_stride) {
      if (page_table_[base_page_number].state != 0) {
        // Base page not free, skip to next usable page.
        continue;
      }
      // Check requested range to ensure free.
      start_page_number = uint32_t(base_page_number);
      end_page_number = uint32_t(base_page_number) + page_count - 1;
      assert(end_page_number < page_table_.size());
      bool any_taken = false;
      for (uint32_t page_number = uint32_t(base_page_number);
           !any_taken && page_number <= end_page_number; ++page_number) {
        bool is_free = page_table_[page_number].state == 0;
        if (!is_free) {
          // At least one page in the range is used, skip to next.
          // We know we'll be starting at least before this page.
          any_taken = true;
          if (page_count > page_number) {
            // Not enough space left to fit entire page range. Breaks outer
            // loop.
            base_page_number = -1;
          } else {
            base_page_number = page_number - page_count;
            base_page_number -= base_page_number % page_scan_stride;
            base_page_number += page_scan_stride;  // cancel out loop logic
          }
          break;
        }
      }
      if (!any_taken) {
        // Found our place.
        break;
      }
      // Retry.
      start_page_number = end_page_number = UINT_MAX;
    }
  } else {
    for (uint32_t base_page_number = low_page_number; base_page_number <= max_base_page_number;
         base_page_number += page_scan_stride) {
      if (page_table_[base_page_number].state != 0) {
        // Base page not free, skip to next usable page.
        continue;
      }
      // Check requested range to ensure free.
      start_page_number = base_page_number;
      end_page_number = base_page_number + page_count - 1;
      bool any_taken = false;
      for (uint32_t page_number = base_page_number; !any_taken && page_number <= end_page_number;
           ++page_number) {
        bool is_free = page_table_[page_number].state == 0;
        if (!is_free) {
          // At least one page in the range is used, skip to next.
          // We know we'll be starting at least after this page.
          any_taken = true;
          base_page_number = RoundUp(page_number + 1, page_scan_stride);
          base_page_number -= page_scan_stride;  // cancel out loop logic
          break;
        }
      }
      if (!any_taken) {
        // Found our place.
        break;
      }
      // Retry.
      start_page_number = end_page_number = UINT_MAX;
    }
  }
  if (start_page_number == UINT_MAX || end_page_number == UINT_MAX) {
    // Out of memory.
    GH_HEAP_ERROR("BaseHeap::Alloc failed to find contiguous range");
    assert(false && "Heap exhausted!");
    return false;
  }

  // Allocate from host.
  if (allocation_type == kMemoryAllocationReserve) {
    // Reserve is not needed, as we are mapped already.
  } else if (page_size_ >= host_->HostPageSize()) {
    auto alloc_type = (allocation_type & kMemoryAllocationCommit)
                          ? AllocationType::kCommit
                          : AllocationType::kReserve;
    void* result =
        host_->AllocateHost(TranslateRelative(start_page_number << page_size_shift_),
                                page_count << page_size_shift_, alloc_type, ToPageAccess(protect));
    if (!result) {
      GH_HEAP_ERROR("BaseHeap::Alloc failed to alloc range from host");
      return false;
    }
  }

  // Set page state.
  for (uint32_t page_number = start_page_number; page_number <= end_page_number; ++page_number) {
    auto& page_entry = page_table_[page_number];
    if (!page_entry.state) {
      unreserved_page_count_--;
    }
    page_entry.base_address = start_page_number;
    page_entry.region_page_count = page_count;
    page_entry.allocation_protect = protect;
    page_entry.current_protect = protect;
    page_entry.state = kMemoryAllocationReserve | allocation_type;
  }

  if (!SyncHostPageAccess(start_page_number, end_page_number)) {
    return false;
  }

  if (allocation_type != kMemoryAllocationReserve && host_->ScribbleHeap() &&
      (protect & kMemoryProtectWrite)) {
    std::memset(TranslateRelative(start_page_number << page_size_shift_), 0xCD,
                page_count << page_size_shift_);
  }

  *out_address = heap_base_ + (start_page_number << page_size_shift_);
  return true;
}

bool BaseHeap::Decommit(uint32_t address, uint32_t size) {
  uint32_t page_count = get_page_count(size, page_size_, page_size_shift_);
  uint32_t start_page_number = (address - heap_base_) >> page_size_shift_;
  uint32_t end_page_number = start_page_number + page_count - 1;
  start_page_number = std::min(uint32_t(page_table_.size()) - 1, start_page_number);
  end_page_number = std::min(uint32_t(page_table_.size()) - 1, end_page_number);

  // Global critical region before heap_mutex_ - see
  // AcquireHostPageReconcileLock for why the order matters.
  auto global_lock = AcquireHostPageReconcileLock();
  std::lock_guard<std::recursive_mutex> heap_lock(heap_mutex_);

  // Release from host.
  // TODO(benvanik): find a way to actually decommit memory;
  //     mapped memory cannot be decommitted.
  /*BOOL result =
      VirtualFree(TranslateRelative(start_page_number << page_size_shift_),
                  page_count << page_size_shift_, MEM_DECOMMIT);
  if (!result) {
    PLOGW("BaseHeap::Decommit failed due to host VirtualFree failure");
    return false;
  }*/

  // Perform table change.
  for (uint32_t page_number = start_page_number; page_number <= end_page_number; ++page_number) {
    auto& page_entry = page_table_[page_number];
    page_entry.state &= ~kMemoryAllocationCommit;
  }

  if (!SyncHostPageAccess(start_page_number, end_page_number)) {
    return false;
  }

  return true;
}

bool BaseHeap::Release(uint32_t base_address, uint32_t* out_region_size) {
  // Global critical region before heap_mutex_ - see
  // AcquireHostPageReconcileLock for why the order matters.
  auto global_lock = AcquireHostPageReconcileLock();
  std::lock_guard<std::recursive_mutex> heap_lock(heap_mutex_);

  // Given address must be a region base address.
  uint32_t base_page_number = (base_address - heap_base_) >> page_size_shift_;
  auto base_page_entry = page_table_[base_page_number];
  if (base_page_entry.base_address != base_page_number) {
    GH_HEAP_ERROR("BaseHeap::Release failed because address is not a region start");
    return false;
  }

  if (heap_base_ == 0x00000000 && base_page_number == 0) {
    GH_HEAP_ERROR("BaseHeap::Release: Attempt to free 0!");
    return false;
  }

  if (out_region_size) {
    *out_region_size = (base_page_entry.region_page_count << page_size_shift_);
  }

  // Release from host not needed as mapping reserves the range for us.
  // TODO(benvanik): protect with NOACCESS?
  /*BOOL result = VirtualFree(
      TranslateRelative(base_page_number << page_size_shift_), 0, MEM_RELEASE);
  if (!result) {
    PLOGE("BaseHeap::Release failed due to host VirtualFree failure");
    return false;
  }*/
  // Instead, we just protect it, if we can.
  if (page_size_ == host_->HostPageSize() ||
      ((base_page_entry.region_page_count << page_size_shift_) % host_->HostPageSize() == 0 &&
       ((base_page_number << page_size_shift_) % host_->HostPageSize() == 0))) {
    // TODO(benvanik): figure out why games are using memory after releasing
    // it. It's possible this is some virtual/physical stuff where the GPU
    // still can access it.
    if (host_->ProtectOnRelease()) {
      if (!host_->ProtectHost(TranslateRelative(base_page_number << page_size_shift_),
                                base_page_entry.region_page_count << page_size_shift_,
                                PageAccess::kNoAccess, nullptr)) {
        GH_HEAP_WARN("BaseHeap::Release failed due to host VirtualProtect failure");
      }
    }
  }

  // Perform table change.
  uint32_t end_page_number = base_page_number + base_page_entry.region_page_count - 1;
  for (uint32_t page_number = base_page_number; page_number <= end_page_number; ++page_number) {
    auto& page_entry = page_table_[page_number];
    page_entry.qword = 0;
    unreserved_page_count_++;
  }

  if (!SyncHostPageAccess(base_page_number, end_page_number)) {
    return false;
  }

  return true;
}

bool BaseHeap::Protect(uint32_t address, uint32_t size, uint32_t protect, uint32_t* old_protect) {
  if (!size) {
    GH_HEAP_ERROR("BaseHeap::Protect failed due to zero size");
    return false;
  }

  // From the VirtualProtect MSDN page:
  //
  // "The region of affected pages includes all pages containing one or more
  //  bytes in the range from the lpAddress parameter to (lpAddress+dwSize).
  //  This means that a 2-byte range straddling a page boundary causes the
  //  protection attributes of both pages to be changed."
  //
  // "The access protection value can be set only on committed pages. If the
  //  state of any page in the specified region is not committed, the function
  //  fails and returns without modifying the access protection of any pages in
  //  the specified region."

  uint32_t start_page_number = (address - heap_base_) >> page_size_shift_;
  if (start_page_number >= page_table_.size()) {
    GH_HEAP_ERROR("BaseHeap::Protect failed due to out-of-bounds base address {:08X}", address);
    return false;
  }
  uint32_t end_page_number =
      uint32_t((uint64_t(address) + size - 1 - heap_base_) >> page_size_shift_);
  if (end_page_number >= page_table_.size()) {
    GH_HEAP_ERROR(
        "BaseHeap::Protect failed due to out-of-bounds range ({:08X} bytes "
        "from {:08x})",
        size, address);
    return false;
  }

  // Global critical region before heap_mutex_ - see
  // AcquireHostPageReconcileLock for why the order matters.
  auto global_lock = AcquireHostPageReconcileLock();
  std::lock_guard<std::recursive_mutex> heap_lock(heap_mutex_);

  // Ensure all pages are in the same reserved region and all are committed.
  uint32_t first_base_address = UINT_MAX;
  for (uint32_t page_number = start_page_number; page_number <= end_page_number; ++page_number) {
    auto page_entry = page_table_[page_number];
    if (first_base_address == UINT_MAX) {
      first_base_address = page_entry.base_address;
    } else if (first_base_address != page_entry.base_address) {
      GH_HEAP_ERROR("BaseHeap::Protect failed due to request spanning regions");
      return false;
    }
    if (!(page_entry.state & kMemoryAllocationCommit)) {
      GH_HEAP_ERROR("BaseHeap::Protect failed due to uncommitted page");
      return false;
    }
  }

  // Change host protection directly when guest pages are at least as large as
  // host pages. Smaller guest pages are reconciled after the page table update.
  if (page_size_ >= host_->HostPageSize()) {
    const uint32_t page_count = end_page_number - start_page_number + 1;
    PageAccess old_protect_access;
    if (!host_->ProtectHost(TranslateRelative(start_page_number << page_size_shift_),
                              page_count << page_size_shift_, ToPageAccess(protect),
                              old_protect ? &old_protect_access : nullptr)) {
      GH_HEAP_ERROR("BaseHeap::Protect failed due to host VirtualProtect failure");
      return false;
    }

    if (old_protect) {
      *old_protect = FromPageAccess(old_protect_access);
    }
  } else {
    if (old_protect) {
      *old_protect = page_table_[start_page_number].current_protect;
    }
  }

  // Perform table change.
  for (uint32_t page_number = start_page_number; page_number <= end_page_number; ++page_number) {
    auto& page_entry = page_table_[page_number];
    page_entry.current_protect = protect;
  }

  if (!SyncHostPageAccess(start_page_number, end_page_number)) {
    return false;
  }

  return true;
}

bool BaseHeap::QueryRegionInfo(uint32_t base_address, HeapAllocationInfo* out_info) {
  uint32_t start_page_number = (base_address - heap_base_) >> page_size_shift_;
  if (start_page_number > page_table_.size()) {
    GH_HEAP_ERROR("BaseHeap::QueryRegionInfo base page out of range");
    return false;
  }

  std::lock_guard<std::recursive_mutex> heap_lock(heap_mutex_);

  auto start_page_entry = page_table_[start_page_number];
  out_info->base_address = base_address;
  out_info->allocation_base = 0;
  out_info->allocation_protect = 0;
  out_info->region_size = 0;
  out_info->state = 0;
  out_info->protect = 0;
  if (start_page_entry.state) {
    // Committed/reserved region.
    out_info->allocation_base = heap_base_ + (start_page_entry.base_address << page_size_shift_);
    out_info->allocation_protect = start_page_entry.allocation_protect;
    out_info->allocation_size = start_page_entry.region_page_count << page_size_shift_;
    out_info->state = start_page_entry.state;
    out_info->protect = start_page_entry.current_protect;

    // Scan forward and report the size of the region matching the initial
    // base address's attributes.
    for (uint32_t page_number = start_page_number;
         page_number < start_page_entry.base_address + start_page_entry.region_page_count;
         ++page_number) {
      auto page_entry = page_table_[page_number];
      if (page_entry.base_address != start_page_entry.base_address ||
          page_entry.state != start_page_entry.state ||
          page_entry.current_protect != start_page_entry.current_protect) {
        // Different region or different properties within the region; done.
        break;
      }
      out_info->region_size += page_size_;
    }
  } else {
    // Free region.
    for (uint32_t page_number = start_page_number; page_number < page_table_.size();
         ++page_number) {
      auto page_entry = page_table_[page_number];
      if (page_entry.state) {
        // First non-free page; done with region.
        break;
      }
      out_info->region_size += page_size_;
    }
  }
  return true;
}

bool BaseHeap::QuerySize(uint32_t address, uint32_t* out_size) {
  uint32_t page_number = (address - heap_base_) >> page_size_shift_;
  if (page_number > page_table_.size()) {
    GH_HEAP_ERROR("BaseHeap::QuerySize base page out of range");
    *out_size = 0;
    return false;
  }
  std::lock_guard<std::recursive_mutex> heap_lock(heap_mutex_);
  auto page_entry = page_table_[page_number];
  *out_size = (page_entry.region_page_count << page_size_shift_);
  return true;
}

bool BaseHeap::QueryBaseAndSize(uint32_t* in_out_address, uint32_t* out_size) {
  uint32_t page_number = (*in_out_address - heap_base_) >> page_size_shift_;
  if (page_number > page_table_.size()) {
    GH_HEAP_ERROR("BaseHeap::QuerySize base page out of range");
    *out_size = 0;
    return false;
  }
  std::lock_guard<std::recursive_mutex> heap_lock(heap_mutex_);
  auto page_entry = page_table_[page_number];
  *in_out_address = (page_entry.base_address << page_size_shift_);
  *out_size = (page_entry.region_page_count << page_size_shift_);
  return true;
}

bool BaseHeap::QueryProtect(uint32_t address, uint32_t* out_protect) {
  uint32_t page_number = (address - heap_base_) >> page_size_shift_;
  if (page_number > page_table_.size()) {
    GH_HEAP_ERROR("BaseHeap::QueryProtect base page out of range");
    *out_protect = 0;
    return false;
  }
  std::lock_guard<std::recursive_mutex> heap_lock(heap_mutex_);
  auto page_entry = page_table_[page_number];
  *out_protect = page_entry.current_protect;
  return true;
}

PageAccess BaseHeap::QueryRangeAccess(uint32_t low_address, uint32_t high_address) {
  if (low_address > high_address || low_address < heap_base_ ||
      (high_address - heap_base_) >= heap_size_) {
    return PageAccess::kNoAccess;
  }
  uint32_t low_page_number = (low_address - heap_base_) >> page_size_shift_;
  uint32_t high_page_number = (high_address - heap_base_) >> page_size_shift_;
  bool all_readable = true;
  bool all_writable = true;
  {
    std::lock_guard<std::recursive_mutex> heap_lock(heap_mutex_);
    for (uint32_t i = low_page_number; i <= high_page_number; ++i) {
      uint32_t page_protect = page_table_[i].current_protect;
      if (!(page_protect & kMemoryProtectRead)) {
        all_readable = false;
      }
      if (!(page_protect & kMemoryProtectWrite) &&
          !(page_protect & kMemoryProtectWriteCombine)) {
        all_writable = false;
      }
    }
  }
  if (all_readable && all_writable) {
    return PageAccess::kReadWrite;
  } else if (all_readable) {
    return PageAccess::kReadOnly;
  } else {
    return PageAccess::kNoAccess;
  }
}

VirtualHeap::VirtualHeap() = default;

VirtualHeap::~VirtualHeap() = default;

void VirtualHeap::Initialize(HeapHost* host, uint8_t* membase, HeapType heap_type,
                             uint32_t heap_base, uint32_t heap_size, uint32_t page_size) {
  BaseHeap::Initialize(host, membase, heap_type, heap_base, heap_size, page_size);
}

PhysicalHeap::PhysicalHeap() : parent_heap_(nullptr) {}

PhysicalHeap::~PhysicalHeap() = default;

void PhysicalHeap::Initialize(HeapHost* host, uint8_t* membase, HeapType heap_type,
                              uint32_t heap_base, uint32_t heap_size, uint32_t page_size,
                              VirtualHeap* parent_heap) {
  uint32_t host_address_offset;
  if (heap_base >= 0xE0000000 && host->HostAllocationGranularity() > 0x1000) {
    host_address_offset = 0x1000;
  } else {
    host_address_offset = 0;
  }

  BaseHeap::Initialize(host, membase, heap_type, heap_base, heap_size, page_size,
                       host_address_offset);
  parent_heap_ = parent_heap;
  system_page_size_ = uint32_t(host_->HostPageSize());

  system_page_count_ =
      (size_t(heap_size_) + host_address_offset + (system_page_size_ - 1)) / system_page_size_;
  system_page_flags_.resize((system_page_count_ + 63) / 64);
}

bool PhysicalHeap::Alloc(uint32_t size, uint32_t alignment, uint32_t allocation_type,
                         uint32_t protect, bool top_down, uint32_t* out_address) {
  *out_address = 0;

  // Default top-down. Since parent heap is bottom-up this prevents
  // collisions.
  top_down = true;

  // Adjust alignment size our page size differs from the parent.
  size = RoundUp(size, page_size_);
  alignment = RoundUp(alignment, page_size_);

  // Global critical region before heap_mutex_ - see
  // AcquireHostPageReconcileLock for why the order matters.
  auto global_lock = AcquireHostPageReconcileLock();
  std::lock_guard<std::recursive_mutex> heap_lock(heap_mutex_);

  // Allocate from parent heap (gets our physical address in 0-512mb).
  uint32_t parent_heap_start = GetPhysicalAddress(heap_base_);
  uint32_t parent_heap_end = GetPhysicalAddress(heap_base_ + (heap_size_ - 1));
  uint32_t parent_address;
  if (!parent_heap_->AllocRange(parent_heap_start, parent_heap_end, size, alignment,
                                allocation_type, protect, top_down, &parent_address)) {
    GH_HEAP_ERROR("PhysicalHeap::Alloc unable to alloc physical memory in parent heap");
    return false;
  }

  // Given the address we've reserved in the parent heap, pin that here.
  // Shouldn't be possible for it to be allocated already.
  uint32_t address = heap_base_ + parent_address - parent_heap_start;
  if (!BaseHeap::AllocFixed(address, size, alignment, allocation_type, protect)) {
    GH_HEAP_ERROR("PhysicalHeap::Alloc unable to pin physical memory in physical heap");
    // TODO(benvanik): don't leak parent memory.
    return false;
  }
  *out_address = address;
  return true;
}

bool PhysicalHeap::AllocFixed(uint32_t base_address, uint32_t size, uint32_t alignment,
                              uint32_t allocation_type, uint32_t protect) {
  // Adjust alignment size our page size differs from the parent.
  size = RoundUp(size, page_size_);
  alignment = RoundUp(alignment, page_size_);

  // Global critical region before heap_mutex_ - see
  // AcquireHostPageReconcileLock for why the order matters.
  auto global_lock = AcquireHostPageReconcileLock();
  std::lock_guard<std::recursive_mutex> heap_lock(heap_mutex_);

  // Allocate from parent heap (gets our physical address in 0-512mb).
  // NOTE: this can potentially overwrite heap contents if there are already
  // committed pages in the requested physical range.
  // TODO(benvanik): flag for ensure-not-committed?
  uint32_t parent_base_address = GetPhysicalAddress(base_address);
  if (!parent_heap_->AllocFixed(parent_base_address, size, alignment, allocation_type, protect)) {
    GH_HEAP_ERROR("PhysicalHeap::Alloc unable to alloc physical memory in parent heap");
    return false;
  }

  // Given the address we've reserved in the parent heap, pin that here.
  // Shouldn't be possible for it to be allocated already.
  uint32_t address = heap_base_ + parent_base_address - GetPhysicalAddress(heap_base_);
  if (!BaseHeap::AllocFixed(address, size, page_size_, allocation_type, protect)) {
    GH_HEAP_ERROR("PhysicalHeap::Alloc unable to pin physical memory in physical heap");
    // TODO(benvanik): don't leak parent memory.
    return false;
  }

  return true;
}

bool PhysicalHeap::AllocRange(uint32_t low_address, uint32_t high_address, uint32_t size,
                              uint32_t alignment, uint32_t allocation_type, uint32_t protect,
                              bool top_down, uint32_t* out_address) {
  *out_address = 0;

  // Adjust alignment size our page size differs from the parent.
  size = RoundUp(size, page_size_);
  alignment = RoundUp(alignment, page_size_);

  // Global critical region before heap_mutex_ - see
  // AcquireHostPageReconcileLock for why the order matters.
  auto global_lock = AcquireHostPageReconcileLock();
  std::lock_guard<std::recursive_mutex> heap_lock(heap_mutex_);

  // Allocate from parent heap (gets our physical address in 0-512mb).
  low_address = std::max(heap_base_, low_address);
  high_address = std::min(heap_base_ + (heap_size_ - 1), high_address);
  uint32_t parent_low_address = GetPhysicalAddress(low_address);
  uint32_t parent_high_address = GetPhysicalAddress(high_address);
  uint32_t parent_address;
  if (!parent_heap_->AllocRange(parent_low_address, parent_high_address, size, alignment,
                                allocation_type, protect, top_down, &parent_address)) {
    GH_HEAP_ERROR("PhysicalHeap::Alloc unable to alloc physical memory in parent heap");
    return false;
  }

  // Given the address we've reserved in the parent heap, pin that here.
  // Shouldn't be possible for it to be allocated already.
  uint32_t address = heap_base_ + parent_address - GetPhysicalAddress(heap_base_);
  if (!BaseHeap::AllocFixed(address, size, page_size_, allocation_type, protect)) {
    GH_HEAP_ERROR("PhysicalHeap::Alloc unable to pin physical memory in physical heap");
    // TODO(benvanik): don't leak parent memory.
    return false;
  }
  *out_address = address;
  return true;
}

bool PhysicalHeap::AllocSystemHeap(uint32_t size, uint32_t alignment, uint32_t allocation_type,
                                   uint32_t protect, bool top_down, uint32_t* out_address) {
  return Alloc(size, alignment, allocation_type, protect, top_down, out_address);
}

bool PhysicalHeap::Decommit(uint32_t address, uint32_t size) {
  auto global_lock = std::unique_lock<std::recursive_mutex>(host_->GlobalMutex());

  uint32_t parent_address = GetPhysicalAddress(address);
  if (!parent_heap_->Decommit(parent_address, size)) {
    GH_HEAP_ERROR("PhysicalHeap::Decommit failed due to parent heap failure");
    return false;
  }

  // Not caring about the contents anymore.
  TriggerCallbacks(std::move(global_lock), address, size, true, true);

  return BaseHeap::Decommit(address, size);
}

bool PhysicalHeap::Release(uint32_t base_address, uint32_t* out_region_size) {
  auto global_lock = std::unique_lock<std::recursive_mutex>(host_->GlobalMutex());

  uint32_t parent_base_address = GetPhysicalAddress(base_address);
  if (!parent_heap_->Release(parent_base_address, out_region_size)) {
    GH_HEAP_ERROR("PhysicalHeap::Release failed due to parent heap failure");
    return false;
  }

  // Must invalidate here because the range being released may be reused in
  // another mapping of physical memory - but callback flags are set in each
  // heap separately (https://github.com/xenia-project/xenia/issues/1559 -
  // dynamic vertices in 4D5307F2 start screen and menu allocated in 0xA0000000
  // at addresses that overlap intro video textures in 0xE0000000, with the
  // state of the allocator as of February 24th, 2020). If memory is invalidated
  // in Alloc instead, Alloc won't be aware of callbacks enabled in other heaps,
  // thus callback handlers will keep considering this range valid forever.
  uint32_t region_size;
  if (QuerySize(base_address, &region_size)) {
    TriggerCallbacks(std::move(global_lock), base_address, region_size, true, true);
  }

  return BaseHeap::Release(base_address, out_region_size);
}

bool PhysicalHeap::Protect(uint32_t address, uint32_t size, uint32_t protect,
                           uint32_t* old_protect) {
  auto global_lock = std::unique_lock<std::recursive_mutex>(host_->GlobalMutex());

  // Only invalidate if making writable again, for simplicity - not when simply
  // marking some range as immutable, for instance.
  if (protect & kMemoryProtectWrite) {
    TriggerCallbacks(std::move(global_lock), address, size, true, true, false);
  }

  if (!parent_heap_->Protect(GetPhysicalAddress(address), size, protect, old_protect)) {
    GH_HEAP_ERROR("PhysicalHeap::Protect failed due to parent heap failure");
    return false;
  }

  return BaseHeap::Protect(address, size, protect);
}

void PhysicalHeap::EnableAccessCallbacks(uint32_t physical_address, uint32_t length,
                                         bool enable_invalidation_notifications,
                                         bool enable_data_providers) {
  // TODO(Triang3l): Implement data providers.
  assert(!enable_data_providers);
  if (!enable_invalidation_notifications && !enable_data_providers) {
    return;
  }
  uint32_t physical_address_offset = GetPhysicalAddress(heap_base_);
  if (physical_address < physical_address_offset) {
    if (physical_address_offset - physical_address >= length) {
      return;
    }
    length -= physical_address_offset - physical_address;
    physical_address = physical_address_offset;
  }
  uint32_t heap_relative_address = physical_address - physical_address_offset;
  if (heap_relative_address >= heap_size_) {
    return;
  }
  length = std::min(length, heap_size_ - heap_relative_address);
  if (length == 0) {
    return;
  }

  uint32_t system_page_first = (heap_relative_address + host_address_offset()) / system_page_size_;
  uint32_t system_page_last =
      (heap_relative_address + length - 1 + host_address_offset()) / system_page_size_;
  system_page_last = std::min(system_page_last, system_page_count_ - 1);
  assert(system_page_first <= system_page_last);

  // Update callback flags for system pages and make their protection stricter
  // if needed.
  PageAccess protect_access = enable_data_providers
                                               ? PageAccess::kNoAccess
                                               : PageAccess::kReadOnly;
  uint8_t* protect_base = membase_ + heap_base_;
  uint32_t protect_system_page_first = UINT32_MAX;
  auto global_lock = std::unique_lock<std::recursive_mutex>(host_->GlobalMutex());
  for (uint32_t i = system_page_first; i <= system_page_last; ++i) {
    // Check if need to enable callbacks for the page and raise its protection.
    //
    // If enabling invalidation notifications:
    // - Page writable and not watched for changes yet - protect and enable
    //   invalidation notifications.
    // - Page seen as writable by the guest, but only needs data providers -
    //   just set the bits to enable invalidation notifications (already has
    //   even stricter protection than needed).
    // - Page not writable as requested by the game - don't do anything (need
    //   real access violations here).
    // If enabling data providers:
    // - Page accessible (either read/write or read-only) and didn't need data
    //   providers initially - protect and enable data providers.
    // - Otherwise - do nothing.
    //
    // It's safe not to await data provider completion here before protecting as
    // this never makes protection lighter, so it can't interfere with page
    // faults that await data providers.
    //
    // Enabling data providers doesn't need to be deferred - providers will be
    // polled for the last time without releasing the lock.
    SystemPageFlagsBlock& page_flags_block = system_page_flags_[i >> 6];
    uint64_t page_flags_bit = uint64_t(1) << (i & 63);
    uint32_t guest_page_number =
        SatSub(i * system_page_size_, host_address_offset()) >> page_size_shift_;
    if (guest_page_number >= page_table_.size()) {
      GH_HEAP_ERROR("Access callback page OOB: system_page={} guest_page={} offset=0x{:X}", i,
                   guest_page_number, host_address_offset());
      assert(false);
      continue;
    }
    PageAccess current_page_access =
        ToPageAccess(page_table_[guest_page_number].current_protect);
    bool protect_system_page = false;
    // Don't do anything with inaccessible pages - don't protect, don't enable
    // callbacks - because real access violations are needed there. And don't
    // enable invalidation notifications for read-only pages for the same
    // reason.
    if (current_page_access != PageAccess::kNoAccess) {
      // TODO(Triang3l): Enable data providers.
      if (enable_invalidation_notifications) {
        if (current_page_access != PageAccess::kReadOnly &&
            (page_flags_block.notify_on_invalidation & page_flags_bit) == 0) {
          // TODO(Triang3l): Check if data providers are already enabled.
          // If data providers are already enabled for the page, it has even
          // stricter protection.
          protect_system_page = true;
          page_flags_block.notify_on_invalidation |= page_flags_bit;
        }
      }
    }
    if (protect_system_page) {
      if (protect_system_page_first == UINT32_MAX) {
        protect_system_page_first = i;
      }
    } else {
      if (protect_system_page_first != UINT32_MAX) {
        host_->ProtectHost(protect_base + protect_system_page_first * system_page_size_,
                             (i - protect_system_page_first) * system_page_size_, protect_access);
        protect_system_page_first = UINT32_MAX;
      }
    }
  }
  if (protect_system_page_first != UINT32_MAX) {
    host_->ProtectHost(protect_base + protect_system_page_first * system_page_size_,
                         (system_page_last + 1 - protect_system_page_first) * system_page_size_,
                         protect_access);
  }
}

bool PhysicalHeap::TriggerCallbacks(std::unique_lock<std::recursive_mutex> global_lock_locked_once,
                                    uint32_t virtual_address, uint32_t length, bool is_write,
                                    bool unwatch_exact_range, bool unprotect) {
  (void)global_lock_locked_once;  // Holds the shared lock throughout callback dispatch.
  // TODO(Triang3l): Support read watches.
  assert(is_write);
  if (!is_write) {
    return false;
  }

  if (virtual_address < heap_base_) {
    if (heap_base_ - virtual_address >= length) {
      return false;
    }
    length -= heap_base_ - virtual_address;
    virtual_address = heap_base_;
  }
  uint32_t heap_relative_address = virtual_address - heap_base_;
  if (heap_relative_address >= heap_size_) {
    return false;
  }
  length = std::min(length, heap_size_ - heap_relative_address);
  if (length == 0) {
    return false;
  }

  uint32_t system_page_first = (heap_relative_address + host_address_offset()) / system_page_size_;
  uint32_t system_page_last =
      (heap_relative_address + length - 1 + host_address_offset()) / system_page_size_;
  system_page_last = std::min(system_page_last, system_page_count_ - 1);
  assert(system_page_first <= system_page_last);
  uint32_t block_index_first = system_page_first >> 6;
  uint32_t block_index_last = system_page_last >> 6;

  // Check if watching any page, whether need to call the callback at all.
  bool any_watched = false;
  for (uint32_t i = block_index_first; i <= block_index_last; ++i) {
    uint64_t block = system_page_flags_[i].notify_on_invalidation;
    if (i == block_index_first) {
      block &= ~((uint64_t(1) << (system_page_first & 63)) - 1);
    }
    if (i == block_index_last && (system_page_last & 63) != 63) {
      block &= (uint64_t(1) << ((system_page_last & 63) + 1)) - 1;
    }
    if (block) {
      any_watched = true;
      break;
    }
  }
  if (!any_watched) {
    return false;
  }

  // Trigger callbacks.
  if (!unprotect) {
    // If not doing anything with protection, no point in unwatching excess
    // pages.
    unwatch_exact_range = true;
  }
  uint32_t physical_address_offset = GetPhysicalAddress(heap_base_);
  uint32_t physical_address_start =
      SatSub(system_page_first * system_page_size_, host_address_offset()) +
      physical_address_offset;
  uint32_t physical_length =
      std::min(SatSub(system_page_last * system_page_size_ + system_page_size_,
                            host_address_offset()) +
                   physical_address_offset - physical_address_start,
               heap_size_ - (physical_address_start - physical_address_offset));
  uint32_t unwatch_first = 0;
  uint32_t unwatch_last = UINT32_MAX;
  const auto allowed = host_->InvalidatePhysicalRange(
      physical_address_start, physical_length, unwatch_exact_range);
  unwatch_first = allowed.first;
  unwatch_last = allowed.second;
  if (!unwatch_exact_range) {
    // Always unwatch at least the requested pages.
    unwatch_first = std::min(unwatch_first, physical_address_start);
    unwatch_last = std::max(unwatch_last, physical_address_start + physical_length - 1);
    // Don't unprotect too much if not caring much about the region (limit to
    // 4 MB - somewhat random, but max 1024 iterations of the page loop).
    const uint32_t kMaxUnwatchExcess = 4 * 1024 * 1024;
    unwatch_first = std::max(unwatch_first, physical_address_start & ~(kMaxUnwatchExcess - 1));
    unwatch_last = std::min(
        unwatch_last, (physical_address_start + physical_length - 1) | (kMaxUnwatchExcess - 1));
    // Convert to heap-relative addresses.
    unwatch_first = SatSub(unwatch_first, physical_address_offset);
    unwatch_last = SatSub(unwatch_last, physical_address_offset);
    // Clamp to the heap upper bound.
    unwatch_first = std::min(unwatch_first, heap_size_ - 1);
    unwatch_last = std::min(unwatch_last, heap_size_ - 1);
    // Convert to system pages and update the range.
    unwatch_first += host_address_offset();
    unwatch_last += host_address_offset();
    assert(unwatch_first <= unwatch_last);
    system_page_first = unwatch_first / system_page_size_;
    system_page_last = unwatch_last / system_page_size_;
    block_index_first = system_page_first >> 6;
    block_index_last = system_page_last >> 6;
  }

  // Unprotect ranges that need unprotection.
  if (unprotect) {
    uint8_t* protect_base = membase_ + heap_base_;
    uint32_t unprotect_system_page_first = UINT32_MAX;
    for (uint32_t i = system_page_first; i <= system_page_last; ++i) {
      // Check if need to allow writing to this page.
      bool unprotect_page =
          (system_page_flags_[i >> 6].notify_on_invalidation & (uint64_t(1) << (i & 63))) != 0;
      if (unprotect_page) {
        uint32_t guest_page_number =
            SatSub(i * system_page_size_, host_address_offset()) >> page_size_shift_;
        if (ToPageAccess(page_table_[guest_page_number].current_protect) !=
            PageAccess::kReadWrite) {
          unprotect_page = false;
        }
      }
      if (unprotect_page) {
        if (unprotect_system_page_first == UINT32_MAX) {
          unprotect_system_page_first = i;
        }
      } else {
        if (unprotect_system_page_first != UINT32_MAX) {
          host_->ProtectHost(protect_base + unprotect_system_page_first * system_page_size_,
                               (i - unprotect_system_page_first) * system_page_size_,
                               PageAccess::kReadWrite);
          unprotect_system_page_first = UINT32_MAX;
        }
      }
    }
    if (unprotect_system_page_first != UINT32_MAX) {
      host_->ProtectHost(protect_base + unprotect_system_page_first * system_page_size_,
                           (system_page_last + 1 - unprotect_system_page_first) * system_page_size_,
                           PageAccess::kReadWrite);
    }
  }

  // Mark pages as not write-watched.
  for (uint32_t i = block_index_first; i <= block_index_last; ++i) {
    uint64_t mask = 0;
    if (i == block_index_first) {
      mask |= (uint64_t(1) << (system_page_first & 63)) - 1;
    }
    if (i == block_index_last && (system_page_last & 63) != 63) {
      mask |= ~((uint64_t(1) << ((system_page_last & 63) + 1)) - 1);
    }
    system_page_flags_[i].notify_on_invalidation &= mask;
  }

  return true;
}

std::unique_lock<std::recursive_mutex> PhysicalHeap::AcquireHostPageReconcileLock() const {
  auto lock = BaseHeap::AcquireHostPageReconcileLock();
  if (!lock.owns_lock() && parent_heap_ && parent_heap_->page_size() < host_->HostPageSize()) {
    lock = std::unique_lock<std::recursive_mutex>(host_->GlobalMutex());
  }
  return lock;
}

bool PhysicalHeap::IsHostPageWriteWatched(uint32_t host_page_number) const {
  // Indices match EnableAccessCallbacks: both count host pages from
  // membase_ + heap_base_, including host_address_offset().
  if (host_page_number >= system_page_count_) {
    return false;
  }
  return (system_page_flags_[host_page_number >> 6].notify_on_invalidation &
          (uint64_t(1) << (host_page_number & 63))) != 0;
}

uint32_t PhysicalHeap::GetPhysicalAddress(uint32_t address) const {
  assert(address >= heap_base_);
  address -= heap_base_;
  assert(address < heap_size_);
  if (heap_base_ >= 0xE0000000) {
    address += 0x1000;
  }
  return address;
}

}  // namespace gh::memory
