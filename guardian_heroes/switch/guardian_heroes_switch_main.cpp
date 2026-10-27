/**
 * @file guardian_heroes_switch_main.cpp
 * @brief libnx entry for the existing GuardianHeroesApp lifecycle.
 *
 * Adapted from nebadasSwifty/masseffect-nx, ReXGlue Horizon fork
 * (commit a85b0b419ec3e332165ae1617c980bd399856a44), under its BSD-3-Clause
 * license. This entry deliberately creates the project's normal WindowedApp;
 * it does not substitute a probe or alternate game implementation.
 */

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <switch.h>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ui/windowed_app.h>
#include <rex/ui/windowed_app_context_switch.h>

#include <switch/runtime/devices/romfs_dev.h>
#include <switch/types.h>

extern "C" {
extern u32 __nx_applet_exit_mode;
NX_NORETURN void __libnx_exit(int result);
}

namespace {

[[noreturn]] void ExitSwitchProcess(int result) {
  REXLOG_INFO("Switch: exiting Guardian Heroes ({})", result);
  rex::FlushLogging();
  std::fflush(nullptr);
  // Use libnx's applet-close path so its worker threads do not outlive the NRO
  // in a process that hbmenu may reuse.
  __nx_applet_exit_mode = 1;
  __libnx_exit(result);
}

}  // namespace

int main(int argc, char* argv[]) {
  const AppletType applet_type = appletGetAppletType();
  const bool application_mode = applet_type == AppletType_Application ||
                                applet_type == AppletType_SystemApplication;
  if (!application_mode) {
    std::fprintf(stderr,
                 "Guardian Heroes requires Application mode; launch hbmenu with title takeover (hold R)\n");
    return EXIT_FAILURE;
  }

  std::fprintf(stdout, "Guardian Heroes Switch: Application mode (applet type %u)\n",
               static_cast<unsigned>(applet_type));
  std::fflush(stdout);

  // Eden and hbmenu may provide no useful argv. Keep the cvar parser's argv[0]
  // contract intact while allowing the normal app factory to supply defaults.
  std::vector<char*> normalized_argv;
  if (argc == 0 || argv == nullptr) {
    static char program_name[] = "guardian_heroes";
    normalized_argv.push_back(program_name);
    argc = 1;
    argv = normalized_argv.data();
  }
  auto remaining = rex::cvar::Init(argc, argv);
  rex::cvar::ApplyEnvironment();
  rex::InitLoggingEarly();

  // Make packaged read-only assets available before SetupEnvironment probes
  // the RomFS root. SD development assets remain the higher-priority path.
  const Result romfs_result = romfsMountFromCurrentProcess("romfs");
  const bool romfs_mounted = R_SUCCEEDED(romfs_result);

  int result = EXIT_FAILURE;
  {
    rex::ui::SwitchWindowedAppContext app_context;
    if (!app_context.Initialize()) {
      REXLOG_ERROR("Switch: failed to initialize the Guardian Heroes app context");
      ExitSwitchProcess(EXIT_FAILURE);
    }

    const auto creator = rex::ui::GetWindowedAppCreator();
    if (!creator) {
      REXLOG_ERROR("Switch: Guardian Heroes app factory is not registered");
      ExitSwitchProcess(EXIT_FAILURE);
    }
    std::unique_ptr<rex::ui::WindowedApp> app = creator(app_context);
    if (!app) {
      REXLOG_ERROR("Switch: Guardian Heroes app factory returned no application");
      ExitSwitchProcess(EXIT_FAILURE);
    }

    std::map<std::string, std::string> parsed;
    const auto& option_names = app->GetPositionalOptions();
    const size_t count = std::min(remaining.size(), option_names.size());
    for (size_t i = 0; i < count; ++i) {
      parsed[option_names[i]] = remaining[i];
    }
    app->SetParsedArguments(std::move(parsed));

    const bool initialized = app->OnInitialize();
    if (!initialized) {
      REXLOG_ERROR("Switch: Guardian Heroes initialization failed");
    }
    result = initialized ? app_context.RunMainMessageLoop() : EXIT_FAILURE;
    app->InvokeOnDestroy();
  }

  if (romfs_mounted) {
    romfsUnmount("romfs");
  }

  ExitSwitchProcess(result);
}
