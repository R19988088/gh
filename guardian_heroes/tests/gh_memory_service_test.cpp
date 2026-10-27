#include "gh_memory_service.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

using namespace gh::memory;

class Bytes final : public HeapStream {
 public:
  std::vector<uint8_t> bytes;
  size_t offset = 0;
  void Read(void* data, size_t size) override {
    assert(offset + size <= bytes.size());
    std::memcpy(data, bytes.data() + offset, size);
    offset += size;
  }
  void Write(const void* data, size_t size) override {
    const auto* p = static_cast<const uint8_t*>(data);
    bytes.insert(bytes.end(), p, p + size);
  }
};

int main() {
  std::recursive_mutex mutex;
  unsigned errors = 0, invalidations = 0;
  MemoryService service(mutex, [] { return false; }, [] { return false; },
      [&](int level, std::string_view message) {
        if (level == 3) {
          std::fprintf(stderr, "%.*s\n", int(message.size()), message.data());
          ++errors;
        }
      });
  bool installed = false;
  assert(service.Initialize(true, [&] {
    // The exception adapter is installed after the physical view is committed,
    // and before the title's initial XEX reservation is allocated.
    PageAccess access;
    size_t length = 0;
    assert(host::QueryProtect(service.physical_membase(), length, access));
    assert(access == PageAccess::kReadWrite);
    assert(host::QueryProtect(service.virtual_membase(), length, access));
    assert(access == PageAccess::kNoAccess);
    HeapAllocationInfo info{};
    assert(service.LookupHeap(0x80000000)->QueryRegionInfo(0x80000000, &info));
    assert(info.state == 0);
    installed = true;
    return true;
  }));
  assert(installed);
  assert(service.physical_membase() == service.virtual_membase() + 0x100000000ull);
  assert(!service.LookupHeap(0x7F000000) && !service.LookupHeap(0xFFD00000));
  const std::array<uint8_t, 4> title_constant{0x2A, 0x6E, 0x3F, 0x38};
  assert(std::memcmp(service.TranslateVirtual(0x8000001C), title_constant.data(), 4) == 0);
  auto* e_heap = service.LookupHeapByType(true, 0x1000);
  assert(e_heap->heap_base() == 0xE0000000);
#if defined(_WIN32) || (defined(__APPLE__) && defined(__aarch64__))
  assert(e_heap->host_address_offset() == 0x1000);
#endif
  const uint32_t virtual_address = service.SystemHeapAlloc(0x8000, 0x4000, false);
  assert(virtual_address);
  auto* virtual_pointer = service.TranslateVirtual(virtual_address);
  virtual_pointer[0] = 0x47;
  virtual_pointer[0x4000] = 0x48;
  auto* virtual_heap = service.LookupHeap(virtual_address);
  assert(virtual_heap->Protect(virtual_address, 0x1000, kMemoryProtectRead));
  uint32_t guest_access;
  assert(virtual_heap->QueryProtect(virtual_address, &guest_access));
  assert(guest_access == kMemoryProtectRead);
  // On 16KiB hosts the writable neighbour shares the first host page.
  virtual_pointer[0x1000] = 0x49;

  const uint32_t physical_address = service.SystemHeapAlloc(0x10000, 0x10000, true);
  assert(physical_address);
  const uint32_t canonical = service.GetPhysicalAddress(physical_address);
  auto* physical_pointer = service.TranslateVirtual(physical_address);
  physical_pointer[0] = 0x64;
  assert(service.physical_membase()[canonical] == 0x64);
  assert(service.HostToGuestVirtual(physical_pointer) == physical_address);
  auto* callback = service.RegisterPhysicalMemoryInvalidationCallback(
      [](void* p, uint32_t, uint32_t, bool) {
        ++*static_cast<unsigned*>(p);
        return std::pair<uint32_t, uint32_t>{0, UINT32_MAX};
      }, &invalidations);
  service.EnablePhysicalMemoryAccessCallbacks(canonical, 0x1000, true, false);
  assert(service.AccessViolationCallback(std::unique_lock(mutex), physical_pointer, true));
  assert(invalidations == 1);
  physical_pointer[0] = 0x65;
  service.UnregisterPhysicalMemoryInvalidationCallback(callback);

  // Snapshot encoding and restore use the service-owned virtual and canonical
  // heaps; physical aliases continue to address the restored shared bytes.
  Bytes snapshot;
  assert(service.Save(&snapshot));
  virtual_pointer[0x4000] = 0;
  physical_pointer[0] = 0;
  assert(service.Restore(&snapshot));
  assert(virtual_pointer[0x4000] == 0x48 && physical_pointer[0] == 0x65);
  assert(snapshot.offset == snapshot.bytes.size());

  assert(service.PrepareMappedRange(0x7FC80000, 0xFFFF));
  PageAccess mmio_access;
  size_t mmio_length = 0;
  assert(host::QueryProtect(service.TranslateVirtual(0x7FC80000), mmio_length, mmio_access));
  assert(mmio_access == PageAccess::kNoAccess && mmio_length >= 0x10000);

  // Shared host mappings retain aliases and report old page access correctly.
  const auto page = host::page_size();
  const auto name = service.file_name().string() + "_page";
  auto handle = host::CreateFileMappingHandle(name, page, PageAccess::kReadWrite, true);
  assert(handle != host::kFileMappingHandleInvalid);
  void* first = host::MapFileView(handle, nullptr, page, PageAccess::kReadWrite, 0);
  void* second = host::MapFileView(handle, nullptr, page, PageAccess::kReadOnly, 0);
  assert(first && second);
  *static_cast<uint8_t*>(first) = 0xA7;
  assert(*static_cast<uint8_t*>(second) == 0xA7);
  PageAccess old_access;
  assert(host::Protect(first, page, PageAccess::kReadOnly, &old_access));
  assert(old_access == PageAccess::kReadWrite);
  assert(host::UnmapFileView(handle, second, page));
  assert(host::UnmapFileView(handle, first, page));
  host::CloseFileMappingHandle(handle, name);
  service.SystemHeapFree(virtual_address);
  service.SystemHeapFree(physical_address);
  assert(errors == 0);
  std::puts("Project memory service: actual OS arena, initialization order, aliases, page protection, watches, snapshots and shared mappings passed");
}
