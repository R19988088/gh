#include "gh_heap.h"
#include "gh_mmio.h"

#include <array>
#include <cassert>
#include <cstdio>
#include <vector>

using namespace gh::memory;

class Host final : public HeapHost, public FaultHost {
 public:
  uint32_t HostPageSize() const override { return 0x4000; }
  uint32_t HostAllocationGranularity() const override { return 0x4000; }
  void* AllocateHost(void* p, size_t, AllocationType, PageAccess) override { return p; }
  bool ReleaseHost(void*, size_t, DeallocationType) override { return true; }
  bool ProtectHost(void*, size_t, PageAccess access, PageAccess* old) override {
    if (old) *old = last_access;
    last_access = access;
    return true;
  }
  std::recursive_mutex& GlobalMutex() override { return mutex; }
  bool ProtectOnRelease() const override { return false; }
  bool ScribbleHeap() const override { return false; }
  void HeapLog(int, std::string_view) override {}
  void FaultLog(std::string_view) override {}
  std::pair<uint32_t, uint32_t> InvalidatePhysicalRange(uint32_t p, uint32_t n, bool) override {
    ++invalidations;
    assert(p == 0 && n == 0x4000);
    return {0, 0x3FFF};
  }
  bool QueryProtection(void*, size_t&, PageAccess& access) override {
    access = last_access;
    return true;
  }
  PageAccess last_access = PageAccess::kNoAccess;
  unsigned invalidations = 0;
  std::recursive_mutex mutex;
};

struct HeapBytes final : HeapStream {
  std::vector<uint8_t> bytes;
  size_t offset = 0;
  void Read(void* output, size_t size) override {
    assert(offset + size <= bytes.size());
    std::memcpy(output, bytes.data() + offset, size);
    offset += size;
  }
  void Write(const void* input, size_t size) override {
    auto* begin = static_cast<const uint8_t*>(input);
    bytes.insert(bytes.end(), begin, begin + size);
  }
};

int main() {
  constexpr auto allocate = kMemoryAllocationReserve | kMemoryAllocationCommit;
  constexpr auto rw = kMemoryProtectRead | kMemoryProtectWrite;
  // Four guest pages share one host page. Protecting one page must keep its
  // writable neighbour usable while retaining the exact guest metadata.
  Host host;
  alignas(0x4000) std::array<uint8_t, 0x10000> backing{};
  VirtualHeap heap;
  heap.Initialize(&host, backing.data(), HeapType::kGuestXex, 0, backing.size(), 0x1000);
  assert(heap.AllocFixed(0x4000, 0x2000, 0x1000, allocate, rw));
  assert(heap.Protect(0x4000, 0x1000, kMemoryProtectRead));
  assert(host.last_access == PageAccess::kReadWrite);
  uint32_t protect = 0, size = 0;
  assert(heap.QueryProtect(0x4000, &protect) && protect == kMemoryProtectRead);
  assert(heap.QuerySize(0x4000, &size) && size == 0x2000);
  backing[0x5000] = 0x47;
  HeapBytes stream;
  assert(heap.Save(&stream));
  backing[0x5000] = 0;
  assert(heap.Restore(&stream));
  assert(backing[0x5000] == 0x47);
  assert(heap.Release(0x4000));
  // Existing zero-size and non-power-of-two alignment rounding is retained.
  uint32_t address = 0;
  assert(heap.Alloc(0, 0x3000, allocate, rw, false, &address));
  assert(heap.QuerySize(address, &size) && size == 0x1000);

  // Watch state belongs to the project physical heap. Reconciliation cannot
  // silently restore writes, and callback dispatch removes the watch once.
  VirtualHeap parent;
  parent.Initialize(&host, backing.data(), HeapType::kGuestPhysical, 0, backing.size(), 0x1000);
  PhysicalHeap physical;
  physical.Initialize(&host, backing.data(), HeapType::kGuestPhysical, 0, backing.size(), 0x1000, &parent);
  assert(physical.AllocFixed(0, 0x4000, 0x1000, allocate, rw));
  physical.EnableAccessCallbacks(0, 0x1000, true, false);
  assert(host.last_access == PageAccess::kReadOnly);
  assert(physical.AllocFixed(0x2000, 0x1000, 0x1000, allocate, rw));
  assert(host.last_access == PageAccess::kReadOnly);
  assert(physical.TriggerCallbacks(std::unique_lock(host.mutex), 0, 1, true, true));
  assert(host.invalidations == 1 && host.last_access == PageAccess::kReadWrite);
  assert(!physical.TriggerCallbacks(std::unique_lock(host.mutex), 0, 1, true, true));

  // The E alias starts one guest page into a 16KiB host mapping. Initialize
  // must query the incoming host before BaseHeap has stored that host pointer.
  PhysicalHeap e_alias;
  e_alias.Initialize(&host, backing.data(), HeapType::kGuestPhysical,
                     0xE0000000, backing.size(), 0x1000, &parent);
  assert(e_alias.host_address_offset() == 0x1000);
  assert(e_alias.heap_base() == 0xE0000000 && e_alias.page_size() == 0x1000);

  uint32_t stored = 0;
  MMIO mmio(host, backing.data(), backing.data() + 0x8000, backing.data() + backing.size() - 1,
      [](const void* base, const void* p) { return uint32_t(static_cast<const uint8_t*>(p) - static_cast<const uint8_t*>(base)); },
      backing.data(), nullptr, nullptr);
  assert(mmio.RegisterRange(0x2000, 0xFFFFF000, 0x1000, &stored,
      [](void*, void*, uint32_t) { return 0x12345678u; },
      [](void*, void* context, uint32_t, uint32_t value) { *static_cast<uint32_t*>(context) = value; }));
  uint32_t loaded = 0;
  assert(mmio.CheckLoad(0x2004, &loaded) && loaded == 0x12345678);
  assert(mmio.CheckStore(0x2004, 0x76543210) && stored == 0x76543210);
  assert(!mmio.CheckLoad(0x3000, &loaded));
  FaultContext fault;
  fault.address = uint64_t(backing.data() + 0x2004);
#if defined(__aarch64__) || defined(_M_ARM64)
  // LDR W0, [X1], #4: a fault must advance PC, update W0 and write back X1.
  const uint32_t load = 0xB8404420;
  fault.x[1] = fault.address;
  fault.instruction = uint64_t(&load);
  assert(mmio.HandleFault(fault));
  assert(fault.x[0] == 0x78563412 && fault.x[1] == fault.address + 4);
  assert(fault.changed_x == 3 && fault.resume_pc == uint64_t(&load) + 4);
#elif defined(__x86_64__) || defined(_M_X64)
  const std::array<uint8_t, 2> load{0x8B, 0x01};  // MOV EAX, [RCX].
  fault.instruction = uint64_t(load.data());
  assert(mmio.HandleFault(fault));
  assert(fault.x[0] == 0x78563412 && fault.changed_x == 1);
  assert(fault.resume_pc == uint64_t(load.data()) + load.size());
#endif
  std::puts("Project memory core: allocation, subpage protection, snapshot bytes, write watches and MMIO fault context passed");
}
