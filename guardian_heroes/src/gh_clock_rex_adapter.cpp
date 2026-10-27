#include "gh_clock_rex_adapter.h"

#include "gh_clock.h"

#include <rex/chrono/clock.h>

namespace gh::clock {

void BindReXGuestClock() noexcept {
  SetGuestTickFrequencySource(&rex::chrono::Clock::guest_tick_frequency);
  SetGuestTickCountSource(&rex::chrono::Clock::QueryGuestTickCount);
}

}  // namespace gh::clock
