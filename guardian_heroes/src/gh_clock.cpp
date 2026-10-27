#include "gh_clock.h"

#include <atomic>
#include <chrono>
#include <limits>
#include <mutex>

namespace gh::clock {
namespace {
constexpr uint64_t kFileTimeEpochOffset = 11644473600ull * 10000000ull;
constexpr uint64_t kDefaultGuestFrequency = 50000000;
const auto kHostStart = std::chrono::steady_clock::now();
const uint64_t kGuestBase = [] {
  const auto now = std::chrono::system_clock::now().time_since_epoch();
  const auto ticks = std::chrono::duration_cast<std::chrono::duration<uint64_t, std::ratio<1, 10000000>>>(now).count();
  return ticks + kFileTimeEpochOffset;
}();
std::atomic<uint64_t> guest_frequency{kDefaultGuestFrequency};
std::atomic<GuestTickFrequencySource> guest_frequency_source{nullptr};
std::atomic<uint64_t> guest_base{kGuestBase};
std::atomic<uint64_t> guest_anchor_tick{0};
std::atomic<uint64_t> guest_uptime_anchor_tick{0};
std::atomic<GuestTickCountSource> guest_tick_source{nullptr};
std::mutex guest_time_mutex;

uint64_t Scale(uint64_t value, uint64_t numerator, uint64_t denominator) noexcept {
  return static_cast<uint64_t>((static_cast<unsigned __int128>(value) * numerator) /
                               denominator);
}

uint64_t GuestSystemTimeAt(uint64_t tick) noexcept {
  const auto anchor = guest_anchor_tick.load(std::memory_order_relaxed);
  const auto elapsed = tick > anchor ? tick - anchor : 0;
  return guest_base.load(std::memory_order_relaxed) +
         Scale(elapsed, 10000000ull, GuestTickFrequency());
}
}  // namespace

uint64_t HostTickFrequency() noexcept { return 1000000000ull; }

uint64_t HostTickCount() noexcept {
  const auto elapsed = std::chrono::steady_clock::now() - kHostStart;
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count());
}

uint64_t HostSystemTime() noexcept {
  const auto now = std::chrono::system_clock::now().time_since_epoch();
  const auto ticks = std::chrono::duration_cast<std::chrono::duration<uint64_t, std::ratio<1, 10000000>>>(now).count();
  return ticks + kFileTimeEpochOffset;
}

uint64_t GuestTickFrequency() noexcept {
  if (const auto source = guest_frequency_source.load(std::memory_order_acquire)) {
    const auto frequency = source();
    if (frequency != 0) return frequency;
  }
  return guest_frequency.load(std::memory_order_relaxed);
}

void SetGuestTickFrequency(uint64_t frequency) noexcept {
  if (frequency != 0) guest_frequency.store(frequency, std::memory_order_relaxed);
}

void SetGuestTickFrequencySource(GuestTickFrequencySource source) noexcept {
  guest_frequency_source.store(source, std::memory_order_release);
}

void SetGuestTickCountSource(GuestTickCountSource source) noexcept {
  std::lock_guard lock(guest_time_mutex);
  const auto current_time = GuestSystemTimeAt(GuestTickCount());
  guest_tick_source.store(source, std::memory_order_release);
  const auto new_anchor = GuestTickCount();
  guest_base.store(current_time, std::memory_order_relaxed);
  guest_anchor_tick.store(new_anchor, std::memory_order_relaxed);
  guest_uptime_anchor_tick.store(new_anchor, std::memory_order_relaxed);
}

uint64_t GuestTickCount() noexcept {
  if (const auto source = guest_tick_source.load(std::memory_order_acquire)) {
    return source();
  }
  const auto host = HostTickCount();
  return Scale(host, GuestTickFrequency(), HostTickFrequency());
}

uint64_t GuestSystemTime() noexcept {
  std::lock_guard lock(guest_time_mutex);
  return GuestSystemTimeAt(GuestTickCount());
}

void SetGuestSystemTime(uint64_t file_time) noexcept {
  std::lock_guard lock(guest_time_mutex);
  guest_base.store(file_time, std::memory_order_relaxed);
  guest_anchor_tick.store(GuestTickCount(), std::memory_order_relaxed);
}

uint32_t GuestUptimeMillis() noexcept {
  std::lock_guard lock(guest_time_mutex);
  const auto now = GuestTickCount();
  const auto anchor = guest_uptime_anchor_tick.load(std::memory_order_relaxed);
  const auto elapsed = now > anchor ? now - anchor : 0;
  const auto millis = Scale(elapsed, 1000ull, GuestTickFrequency());
  return millis > std::numeric_limits<uint32_t>::max()
             ? std::numeric_limits<uint32_t>::max()
             : static_cast<uint32_t>(millis);
}

GuestState CaptureGuestState() noexcept {
  std::lock_guard lock(guest_time_mutex);
  return {guest_frequency.load(std::memory_order_relaxed),
          guest_base.load(std::memory_order_relaxed),
          guest_anchor_tick.load(std::memory_order_relaxed),
          guest_uptime_anchor_tick.load(std::memory_order_relaxed)};
}

void RestoreGuestState(const GuestState& state) noexcept {
  std::lock_guard lock(guest_time_mutex);
  guest_frequency.store(state.frequency, std::memory_order_relaxed);
  guest_base.store(state.system_time_base, std::memory_order_relaxed);
  guest_anchor_tick.store(state.system_time_anchor, std::memory_order_relaxed);
  guest_uptime_anchor_tick.store(state.uptime_anchor, std::memory_order_relaxed);
}

}  // namespace gh::clock
