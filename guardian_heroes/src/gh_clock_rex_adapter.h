#pragma once

namespace gh::clock {

// Bind project guest-time queries to the clock used by ReX runtime services.
void BindReXGuestClock() noexcept;

}  // namespace gh::clock
