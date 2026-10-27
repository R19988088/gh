#pragma once

#include <cstdint>
#include <functional>

class GuestMemory;

namespace gh::kernel {

// Guest ABI queues remain in guest memory; mutation/traversal holds the guest
// APC spinlock. Pending/Deliverable are read-only hints; delivery rechecks locked.
class GuestApcQueue {
 public:
  static void Initialize(const GuestMemory&, std::uint32_t apc, std::uint32_t thread,
                         std::uint32_t kernel, std::uint32_t rundown,
                         std::uint32_t normal, std::uint32_t mode,
                         std::uint32_t context);
  static bool Insert(const GuestMemory&, std::uint32_t apc,
                     std::uint32_t arg1, std::uint32_t arg2);
  static bool Remove(const GuestMemory&, std::uint32_t apc);
  static bool Pending(const GuestMemory&, std::uint32_t thread, std::uint32_t mode);
  static bool Deliverable(const GuestMemory&, std::uint32_t thread);
  static std::uint32_t Pop(const GuestMemory&, std::uint32_t thread, std::uint32_t mode);
};

struct GuestTlsBlock {
  std::uint32_t address;
  std::uint32_t size;
};

// Bitmap bits and slots use the existing guest byte order and high-bit-first ABI.
class GuestTls {
 public:
  static void Initialize(const GuestMemory&, std::uint32_t bitmap, std::uint32_t slots);
  static std::uint32_t Allocate(const GuestMemory&, std::uint32_t bitmap);
  static bool Free(const GuestMemory&, std::uint32_t bitmap, std::uint32_t slot,
                   const std::function<void()>& clear_thread_values);
  static bool Read(const GuestMemory&, GuestTlsBlock, std::uint32_t slot,
                   std::uint32_t* value);
  static bool Write(const GuestMemory&, GuestTlsBlock, std::uint32_t slot,
                    std::uint32_t value);
};

}  // namespace gh::kernel
