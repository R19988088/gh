#include "gh_kernel_guest.h"
#include "native_memory.h"

#include <bit>
#include <atomic>
#include <cstring>
#include <limits>

namespace gh::kernel {
namespace {
constexpr std::uint32_t kApcLists = 0x74;
constexpr std::uint32_t kMayQueueApcs = 0x8B;
constexpr std::uint32_t kApcDisableCount = 0xB0;

std::atomic_ref<std::uint32_t> ApcNext(const GuestMemory& memory, std::uint32_t entry) {
  return std::atomic_ref<std::uint32_t>(
      *reinterpret_cast<std::uint32_t*>(memory.Address(entry, 4)));
}

void InsertBefore(const GuestMemory& memory, std::uint32_t before, std::uint32_t entry) {
  const auto previous = memory.Read32(before + 4);
  memory.Write32(entry, before);
  memory.Write32(entry + 4, previous);
  ApcNext(memory, previous).store(__builtin_bswap32(entry), std::memory_order_release);
  memory.Write32(before + 4, entry);
}
}  // namespace

void GuestApcQueue::Initialize(const GuestMemory& memory, std::uint32_t apc,
                               std::uint32_t thread, std::uint32_t kernel,
                               std::uint32_t rundown, std::uint32_t normal,
                               std::uint32_t mode, std::uint32_t context) {
  const std::uint16_t type = 18;
  std::memcpy(memory.Address(apc, 2), &type, sizeof(type));
  memory.Write8(apc + 2, normal ? static_cast<std::uint8_t>(mode) : 0);
  memory.Write8(apc + 3, 0);
  memory.Write32(apc + 4, thread);
  memory.Write32(apc + 16, kernel);
  memory.Write32(apc + 20, rundown);
  memory.Write32(apc + 24, normal);
  memory.Write32(apc + 28, normal ? context : 0);
}

bool GuestApcQueue::Insert(const GuestMemory& memory, std::uint32_t apc,
                           std::uint32_t arg1, std::uint32_t arg2) {
  const auto thread = memory.Read32(apc + 4);
  if (!thread || !memory.Read8(thread + kMayQueueApcs) || memory.Read8(apc + 3)) return false;
  const auto mode = memory.Read8(apc + 2);
  if (mode > 1) return false;
  const auto head = thread + kApcLists + mode * 8;
  auto before = head;
  if (!memory.Read32(apc + 24)) {
    // Preserve the guest queue's existing kernel-APC insertion order.
    for (auto entry = memory.Read32(head); entry != head; entry = memory.Read32(entry)) {
      before = entry;
      if (memory.Read32(entry - 8 + 24)) break;
    }
  }
  memory.Write32(apc + 32, arg1);
  memory.Write32(apc + 36, arg2);
  InsertBefore(memory, before, apc + 8);
  memory.Write8(apc + 3, 1);
  return true;
}

bool GuestApcQueue::Remove(const GuestMemory& memory, std::uint32_t apc) {
  if (!memory.Read32(apc + 4) || !memory.Read8(apc + 3)) return false;
  const auto next = memory.Read32(apc + 8);
  const auto previous = memory.Read32(apc + 12);
  ApcNext(memory, previous).store(__builtin_bswap32(next), std::memory_order_release);
  memory.Write32(next + 4, previous);
  memory.Write8(apc + 3, 0);
  return true;
}

bool GuestApcQueue::Pending(const GuestMemory& memory, std::uint32_t thread,
                            std::uint32_t mode) {
  const auto head = thread + kApcLists + mode * 8;
  return __builtin_bswap32(ApcNext(memory, head).load(std::memory_order_acquire)) != head;
}

bool GuestApcQueue::Deliverable(const GuestMemory& memory, std::uint32_t thread) {
  return Pending(memory, thread, 1) && std::atomic_ref<std::int32_t>(
      *reinterpret_cast<std::int32_t*>(memory.Address(thread + kApcDisableCount, 4)))
      .load(std::memory_order_acquire) == 0;
}

std::uint32_t GuestApcQueue::Pop(const GuestMemory& memory, std::uint32_t thread,
                                std::uint32_t mode) {
  if (!Pending(memory, thread, mode)) return 0;
  const auto apc = memory.Read32(thread + kApcLists + mode * 8) - 8;
  Remove(memory, apc);
  return apc;
}

void GuestTls::Initialize(const GuestMemory& memory, std::uint32_t bitmap,
                           std::uint32_t slots) {
  const auto padded = (slots + 3) & ~std::uint32_t{3};
  for (std::uint32_t i = 0; i < 8; ++i) {
    const auto available = padded > i * 32 ? padded - i * 32 : 0;
    memory.Write32(bitmap + i * 4, available >= 32 ? ~0u :
                   available ? ~0u << (32 - available) : 0);
  }
}

std::uint32_t GuestTls::Allocate(const GuestMemory& memory, std::uint32_t bitmap) {
  for (std::uint32_t i = 0; i < 8; ++i) {
    const auto bits = memory.Read32(bitmap + i * 4);
    if (!bits) continue;
    const auto leading = static_cast<std::uint32_t>(std::countl_zero(bits));
    memory.Write32(bitmap + i * 4, bits & ~(1u << (31 - leading)));
    return i * 32 + leading;
  }
  return std::numeric_limits<std::uint32_t>::max();
}

bool GuestTls::Free(const GuestMemory& memory, std::uint32_t bitmap,
                     std::uint32_t slot, const std::function<void()>& clear_thread_values) {
  if (slot >= 256) return false;
  const auto word = bitmap + slot / 32 * 4;
  const auto mask = 1u << (31 - slot % 32);
  const auto bits = memory.Read32(word);
  if (bits & mask) return false;
  clear_thread_values();
  memory.Write32(word, bits | mask);
  return true;
}

bool GuestTls::Read(const GuestMemory& memory, GuestTlsBlock block,
                     std::uint32_t slot, std::uint32_t* value) {
  if (slot >= block.size / 4) return false;
  *value = memory.Read32(block.address + slot * 4);
  return true;
}

bool GuestTls::Write(const GuestMemory& memory, GuestTlsBlock block,
                      std::uint32_t slot, std::uint32_t value) {
  if (slot >= block.size / 4) return false;
  memory.Write32(block.address + slot * 4, value);
  return true;
}
}  // namespace gh::kernel
