#include <rex/chrono/clock.h>
#include <switch.h>
namespace rex::chrono {
uint64_t Clock::host_tick_frequency_platform(){ return 1000000000ull; }
uint64_t Clock::host_tick_count_platform(){ return armGetSystemTick() * 625ull / 12ull; }
uint64_t Clock::QueryHostSystemTime(){ return 11644473600ull*10000000ull + host_tick_count_platform()/100; }
uint64_t Clock::QueryHostUptimeMillis(){ return host_tick_count_platform()/1000000ull; }
}
