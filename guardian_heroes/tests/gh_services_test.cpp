#include "gh_clock.h"
#include "gh_exceptions.h"
#include "gh_logging.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <thread>

namespace {
uint64_t injected_guest_ticks = 0;
uint64_t injected_guest_frequency = 25000000;
uint64_t InjectedGuestTickCount() { return injected_guest_ticks; }
uint64_t InjectedGuestTickFrequency() { return injected_guest_frequency; }
}

int main() {
  assert(gh::clock::HostTickFrequency() == 1000000000ull);
  const auto first = gh::clock::HostTickCount();
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  const auto second = gh::clock::HostTickCount();
  assert(second > first);

  gh::clock::SetGuestTickCountSource(nullptr);
  const auto default_guest_tick = gh::clock::GuestTickCount();
  assert(default_guest_tick > 0);
  gh::clock::SetGuestTickFrequency(50000000);
  gh::clock::SetGuestTickFrequencySource(&InjectedGuestTickFrequency);
  assert(gh::clock::GuestTickFrequency() == 25000000);
  gh::clock::SetGuestTickCountSource(&InjectedGuestTickCount);
  assert(gh::clock::GuestTickCount() == 0);
  gh::clock::SetGuestSystemTime(1000);
  injected_guest_ticks += 25000000;
  assert(gh::clock::GuestSystemTime() == 10001000);
  assert(gh::clock::GuestUptimeMillis() == 1000);
  gh::clock::SetGuestSystemTime(2000);
  assert(gh::clock::GuestSystemTime() == 2000);
  assert(gh::clock::GuestUptimeMillis() == 1000);
  injected_guest_ticks += 12500000;
  assert(gh::clock::GuestSystemTime() == 5002000);
  assert(gh::clock::GuestUptimeMillis() == 1500);
  gh::clock::SetGuestTickCountSource(nullptr);
  gh::clock::SetGuestTickFrequencySource(nullptr);
  assert(gh::clock::GuestTickCount() != injected_guest_ticks);

  const auto guest_before = gh::clock::GuestSystemTime();
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  assert(gh::clock::GuestSystemTime() > guest_before);
  gh::clock::SetGuestSystemTime(123456789000000000ull);
  const auto anchored = gh::clock::GuestSystemTime();
  assert(anchored >= 123456789000000000ull && anchored - 123456789000000000ull < 100000ull);
  gh::clock::SetGuestSystemTime(0);
  assert(gh::clock::GuestSystemTime() < 100000ull);
  gh::clock::SetGuestSystemTime(1);
  assert(gh::clock::GuestSystemTime() >= 1 && gh::clock::GuestSystemTime() < 100001ull);
  gh::Exception error("test", 0x1234);
  assert(error.code() == 0x1234 && std::string(error.what()) == "test");

  const auto path = std::filesystem::temp_directory_path() / "guardian_heroes_gh_services_test.log";
  std::filesystem::remove(path);
  gh::logging::Initialize(path);
  gh::logging::Info("service test");
  gh::logging::Shutdown();
  std::ifstream input(path);
  const std::string contents((std::istreambuf_iterator<char>(input)), {});
  assert(contents.find("service test") != std::string::npos);
  std::filesystem::remove(path);
}
