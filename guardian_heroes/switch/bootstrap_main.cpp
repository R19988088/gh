#include <switch.h>

#include <cstdarg>
#include <cstdio>
#include <sys/stat.h>

namespace {
FILE* log_file = nullptr;

void log_stage(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    if (log_file) {
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        fflush(log_file);
    }
}

void log_memory(InfoType type, const char* name) {
    u64 bytes = 0;
    const Result result = svcGetInfo(&bytes, type, CUR_PROCESS_HANDLE, 0);
    if (R_SUCCEEDED(result)) {
        log_stage("%s: %llu bytes (%llu MiB)\n", name,
                  static_cast<unsigned long long>(bytes),
                  static_cast<unsigned long long>(bytes / (1024 * 1024)));
    } else {
        log_stage("%s query failed: 0x%08x\n", name, result);
    }
}
}  // namespace

int main(int, char**) {
    consoleInit(nullptr);
    mkdir("sdmc:/switch", 0777);
    mkdir("sdmc:/switch/guardian-heroes", 0777);
    mkdir("sdmc:/switch/guardian-heroes/logs", 0777);
    log_file = fopen("sdmc:/switch/guardian-heroes/logs/bootstrap.log", "a");
    log_stage("\nGuardian Heroes Switch startup probe %s\n", GH_SWITCH_VERSION);
    log_stage("stage: libnx console initialized\n");
    log_stage("SD log: %s\n", log_file ? "ready" : "unavailable");
    const AppletType type = appletGetAppletType();
    log_stage("Applet type: %u; mode: %s\n", static_cast<unsigned>(type),
              type == AppletType_Application ? "Application" : "Applet / other");
    log_memory(InfoType_TotalMemorySize, "Process memory budget");
    log_memory(InfoType_UsedMemorySize, "Process memory used");
    log_stage("stage: lifecycle ready\nThis probe does not execute the game.\n");
    log_stage("Waiting for Horizon lifecycle events.\n");
    while (appletMainLoop()) {
        consoleUpdate(nullptr);
    }
    // Only a lifecycle shutdown request ends the probe; no exit key is bound.
    log_stage("stage: Horizon requested shutdown\n");
    if (log_file) fclose(log_file);
    consoleExit(nullptr);
    return 0;
}
