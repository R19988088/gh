#pragma once

namespace gh::memory {

// Bind generated guest accesses to the runtime MMIO table after Runtime setup.
void BindReXGuestMemory() noexcept;
void UnbindGuestMemory() noexcept;

}  // namespace gh::memory
