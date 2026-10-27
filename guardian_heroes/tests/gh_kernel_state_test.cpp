#include "gh_kernel_guest.h"
#include "gh_kernel_objects.h"
#include "native_memory.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>

namespace {
void TestApcGuestOwnership() {
  std::array<std::uint8_t, 2048> bytes{};
  GuestMemory memory(bytes.data(), bytes.size());
  constexpr std::uint32_t thread = 0x100;
  for (std::uint32_t mode : {0u, 1u}) {
    const auto head = thread + 0x74 + mode * 8;
    memory.Write32(head, head);
    memory.Write32(head + 4, head);
  }
  memory.Write8(thread + 0x8B, 1);
  using Queue = gh::kernel::GuestApcQueue;
  Queue::Initialize(memory, 0x400, thread, 1, 2, 3, 1, 4);
  Queue::Initialize(memory, 0x440, thread, 1, 2, 5, 1, 6);
  assert(Queue::Insert(memory, 0x400, 7, 8));
  assert(!Queue::Insert(memory, 0x400, 99, 99));
  assert(memory.Read32(0x400 + 32) == 7);
  assert(Queue::Insert(memory, 0x440, 9, 10));
  memory.Write32(thread + 0xB0, 1);
  assert(Queue::Pending(memory, thread, 1));
  assert(!Queue::Deliverable(memory, thread));
  memory.Write32(thread + 0xB0, 0);
  const auto saved = bytes;
  assert(Queue::Pop(memory, thread, 1) == 0x400);
  assert(memory.Read8(0x400 + 3) == 0);
  assert(Queue::Remove(memory, 0x440));
  assert(!Queue::Remove(memory, 0x440));
  assert(!Queue::Pending(memory, thread, 1));
  bytes = saved;
  assert(Queue::Deliverable(memory, thread));
  assert(Queue::Pop(memory, thread, 1) == 0x400);
  assert(Queue::Pop(memory, thread, 1) == 0x440);
  assert(Queue::Pop(memory, thread, 1) == 0);

  Queue::Initialize(memory, 0x480, thread, 1, 2, 3, 0, 4);
  Queue::Initialize(memory, 0x4C0, thread, 1, 2, 0, 1, 4);
  assert(memory.Read8(0x4C0 + 2) == 0 && memory.Read32(0x4C0 + 28) == 0);
  assert(Queue::Insert(memory, 0x480, 0, 0));
  assert(Queue::Insert(memory, 0x4C0, 0, 0));
  assert(Queue::Pop(memory, thread, 0) == 0x4C0);
  assert(Queue::Pop(memory, thread, 0) == 0x480);
  memory.Write8(thread + 0x8B, 0);
  assert(!Queue::Insert(memory, 0x400, 0, 0));
}

void TestGuestTlsAllocationAndBounds() {
  std::array<std::uint8_t, 2048> bytes{};
  GuestMemory memory(bytes.data(), bytes.size());
  constexpr std::uint32_t bitmap = 0x40;
  using Tls = gh::kernel::GuestTls;
  const gh::kernel::GuestTlsBlock first{0x100, 256}, second{0x300, 256};
  Tls::Initialize(memory, bitmap, 64);
  for (std::uint32_t slot = 0; slot < 64; ++slot)
    assert(Tls::Allocate(memory, bitmap) == slot);
  assert(Tls::Allocate(memory, bitmap) == std::numeric_limits<std::uint32_t>::max());
  assert(Tls::Write(memory, first, 63, 0x12345678));
  assert(Tls::Write(memory, second, 63, 0xABCDEF01));
  std::uint32_t value = 0;
  assert(Tls::Read(memory, first, 63, &value) && value == 0x12345678);
  int cleared = 0;
  const auto clear = [&] {
    ++cleared;
    assert(Tls::Write(memory, first, 63, 0));
    assert(Tls::Write(memory, second, 63, 0));
  };
  assert(Tls::Free(memory, bitmap, 63, clear));
  assert(!Tls::Free(memory, bitmap, 63, clear));
  assert(cleared == 1 && Tls::Allocate(memory, bitmap) == 63);
  assert(Tls::Read(memory, first, 63, &value) && value == 0);
  assert(Tls::Read(memory, second, 63, &value) && value == 0);
  memory.Write32(first.address + first.size, 0xDEADBEEF);
  assert(!Tls::Read(memory, first, 64, &value));
  assert(!Tls::Write(memory, first, 64, 0));
  assert(!Tls::Write(memory, first, 0x40000000, 0));
  assert(memory.Read32(first.address + first.size) == 0xDEADBEEF);
}

void TestProjectHandleOwnership() {
  using Table = gh::kernel::HandleTable;
  struct Object {
    explicit Object(int& destroyed) : destroyed(destroyed) {}
    ~Object() { ++destroyed; }
    int& destroyed;
    bool host = false;
  };
  int destroyed = 0;
  auto guest = std::make_shared<Object>(destroyed);
  auto host = std::make_shared<Object>(destroyed);
  Table table;
  const auto first = table.Add(guest);
  const auto duplicate = table.Add(guest);
  const auto host_handle = table.Add(host);
  host->host = true;  // XHostThread marks itself after base-object insertion.
  assert(first == Table::kHandleBase + 4);
  assert(table.Retain(first) && table.References(first) == 2);
  assert(table.Release(first).valid && table.Lookup(first) == guest.get());
  auto released = table.Release(first);
  assert(released.valid && released.removed.get() == guest.get());
  assert(!table.Lookup(first) && !table.Retain(first));
  assert(!table.Release(first).valid);
  released.removed.reset();
  auto removed = table.Purge([](void* value) { return static_cast<Object*>(value)->host; });
  assert(removed.size() == 1 && removed[0].handle == duplicate);
  assert(!table.Lookup(duplicate) && table.Lookup(host_handle) == host.get());
  guest.reset();
  assert(destroyed == 0);
  removed.clear();
  assert(destroyed == 1);

  assert(table.Retain(host_handle));
  const auto counts = table.SaveReferences();
  Table restored;
  assert(restored.RestoreReferences(counts).empty());
  assert(!restored.Lookup(host_handle));
  assert(!restored.Restore(host_handle, host));
  assert(restored.References(host_handle) == 2);
  assert(restored.Release(host_handle).valid && restored.Lookup(host_handle));
  auto final_release = restored.Release(host_handle);
  assert(final_release.removed.get() == host.get());
  final_release.removed.reset();
  auto old = table.Reset();
  assert(old.size() == 1 && !table.Lookup(host_handle));
  host.reset();
  assert(destroyed == 1);
  old.clear();
  assert(destroyed == 2);
  const auto next = table.Add(std::make_shared<int>(1));
  assert(next == first);
}
}  // namespace

int main() {
  TestApcGuestOwnership();
  TestGuestTlsAllocationAndBounds();
  TestProjectHandleOwnership();
}
