#include <cstdint>
#include <cstdio>

extern "C" {
volatile uint8_t rex_nx_dirty[0x20000] = {};
volatile uint8_t rex_nx_dirty_coarse[0x200] = {};
volatile int rex_nx_menu_open = 0;
volatile int rex_nx_menu_cursor = 0;
volatile int rex_nx_show_fps = 0;

const char* rex_nx_version(void) { return "0.1.0"; }
const char* RexSwitchLogDir(void) { return "sdmc:/switch/gh/"; }
void RexLogEmergencyDrain(void) {}
void RexSwitchPerfCount(unsigned) {}
void RexSwitchPerfSetThreadName(uint32_t, const char*) {}
}
