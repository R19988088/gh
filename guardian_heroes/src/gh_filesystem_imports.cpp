#include "gh_filesystem_imports.h"
#include "native_memory.h"

// NtFlushBuffersFile has no host-side state beyond the guest IO status block.
// Keep this narrow import project-owned while file handles remain runtime-owned.
namespace gh::filesystem {

uint32_t FlushGuestFile(uint8_t* base, uint32_t io_status_block) noexcept {
  if (io_status_block != 0) {
    GuestMemory memory(base);
    memory.WriteRam32(io_status_block, 0);
    memory.WriteRam32(io_status_block + 4, 0);
  }
  return 0;  // X_STATUS_SUCCESS
}


}  // namespace gh::filesystem
