/**
 * @file        guardian_heroes_app.cpp
 * @brief       Guardian Heroes application and guest lifecycle
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#include "guardian_heroes_app.h"
#include "generated/default/guardian_heroes_init.h"
#include "localization.h"
#include "gh_exceptions.h"
#include "gh_logging.h"
#include "gh_clock_rex_adapter.h"
#include "gh_threading.h"
#include "gh_controller_assignment_adapter.h"
#include "gh_memory_rex_adapter.h"
#include "gh_audio_factory.h"
#if defined(__SWITCH__)
#include "gh_switch_audio.h"
#include "gh_switch_input_driver.h"
extern "C" void RexSwitchBootLog(const char* stage);
#endif
#ifdef GH_PROJECT_RUNTIME
#include "gh_gpu_factory.h"
#include "gh_guest_continuation_title.h"
#include <rex/stream.h>
#endif
#include <rex/thread/mutex.h>

#include <cstdlib>

#include <rex/assert.h>
#include <rex/cvar.h>
#include <rex/ui/flags.h>
#include <rex/kernel/crt/heap.h>
#include <rex/filesystem.h>
#include <rex/logging/sink.h>
#include <rex/logging.h>
#include <rex/ui/overlay/achievement_toast.h>
#include <rex/ui/overlay/achievements_overlay.h>
#include <rex/ui/overlay/console_overlay.h>
#include <rex/ui/overlay/debug_overlay.h>
#include <rex/ui/overlay/settings_overlay.h>
#include <rex/audio/audio_system.h>
#if !defined(__SWITCH__)
#include <rex/audio/sdl/sdl_audio_system.h>
#endif
#include <rex/input/input_system.h>
#include <rex/kernel/init.h>
#include <rex/string/numeric.h>
#include <rex/system.h>
#include <rex/system/achievement_manager.h>
#include <rex/system/gpu_plugin.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xthread.h>
#include <rex/ui/graphics_provider.h>
#include <rex/ui/keybinds.h>
#include <rex/version.h>

#include <fmt/format.h>
#include <imgui.h>

#include <algorithm>
#include <filesystem>
#include <string_view>

REXCVAR_DEFINE_STRING(gpu_plugin, "", "GPU",
                      "GPU emulation plugin to load at startup (e.g. 'xenos'); empty disables "
                      "GPU emulation")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

namespace {

void SwitchBootLog(const char* stage) {
#if defined(__SWITCH__)
  RexSwitchBootLog(stage);
#else
  (void)stage;
#endif
}

}  // namespace

GuardianHeroesApp::~GuardianHeroesApp() = default;

GuardianHeroesApp::GuardianHeroesApp(rex::ui::WindowedAppContext& ctx, rex::PPCImageInfo image)
    : WindowedApp(ctx, "guardian_heroes", ""), ppc_info_(image) {}

std::unique_ptr<rex::ui::WindowedApp> GuardianHeroesApp::Create(rex::ui::WindowedAppContext& ctx) {
  return std::unique_ptr<GuardianHeroesApp>(new GuardianHeroesApp(ctx, PPCImageConfig));
}

std::unique_ptr<rex::ui::ImGuiDialog> GuardianHeroesApp::CreateAchievementsOverlay() {
  if (!runtime_ || !runtime_->kernel_state() || !imgui_drawer_ || !immediate_drawer_) {
    return nullptr;
  }
  return std::make_unique<rex::ui::AchievementsOverlayDialog>(
      imgui_drawer_.get(), immediate_drawer_.get(), runtime_.get(), &achievements());
}

std::unique_ptr<rex::ui::AchievementNotificationDialog> GuardianHeroesApp::CreateAchievementNotificationDialog() {
  if (!imgui_drawer_ || !immediate_drawer_ || !runtime_) {
    return nullptr;
  }
  return std::make_unique<rex::ui::AchievementToastDialog>(imgui_drawer_.get(), immediate_drawer_.get(),
                                                      runtime_.get());
}

rex::system::AchievementManager& GuardianHeroesApp::achievements() const {
  assert_not_null(runtime_);
  assert_not_null(runtime_->kernel_state());
  return runtime_->kernel_state()->achievements();
}

bool GuardianHeroesApp::OnInitialize() {
  SwitchBootLog("SetupEnvironment begin");
  if (!SetupEnvironment()) {
    SwitchBootLog("SetupEnvironment failed");
    return false;
  }
  SwitchBootLog("SetupEnvironment succeeded");
  SwitchBootLog("SetupPresentation begin");
  if (!SetupPresentation()) {
    SwitchBootLog("SetupPresentation failed");
    return false;
  }
  SwitchBootLog("SetupPresentation succeeded");

  SwitchBootLog("ConstructRuntime begin");
  if (!ConstructRuntime(resolved_defaults_)) {
    SwitchBootLog("ConstructRuntime failed");
    return false;
  }
  SwitchBootLog("ConstructRuntime succeeded");
  SwitchBootLog("LaunchModule begin");
  LaunchModule();
  SwitchBootLog("LaunchModule returned");
  return true;
}

bool GuardianHeroesApp::SetupEnvironment() {
  auto exe_dir = rex::filesystem::GetExecutableFolder();

  std::filesystem::path game_dir;
  std::string game_data_cvar = REXCVAR_GET(game_data_root);
  if (!game_data_cvar.empty()) {
    game_dir = game_data_cvar;
  } else {
#if defined(__SWITCH__)
    // The NRO is launched from the Switch SD card. Keep immutable game data
    // beside it while reserving a separate directory for saves and caches.
    const std::filesystem::path switch_root = "sdmc:/switch/guardian-heroes";
    const auto switch_assets = switch_root / "assets";
    std::error_code error;
    if (std::filesystem::is_regular_file(switch_assets / "default.xex", error)) {
      game_dir = switch_assets;
    }
    // Installed NROs carry the assets directory as the RomFS root. Prefer an
    // explicitly deployed SD tree for development, then fall back to the
    // packaged copy.
    if (game_dir.empty()) {
      const std::filesystem::path romfs_assets = "romfs:/";
      error.clear();
      if (std::filesystem::is_regular_file(romfs_assets / "default.xex", error)) {
        game_dir = romfs_assets;
      }
    }
#else
    // Resolve bundled resources relative to the executable, so launching from
    // Explorer or a shortcut does not depend on the working directory.
    const auto bundled_assets = exe_dir / "assets";
    std::error_code error;
    if (std::filesystem::is_regular_file(bundled_assets / "default.xex", error)) {
      game_dir = bundled_assets;
    }
#endif
  }

  // User data: cvar override, or platform user directory
  std::filesystem::path user_dir;
  std::string user_data_cvar = REXCVAR_GET(user_data_root);
  if (!user_data_cvar.empty()) {
    user_dir = user_data_cvar;
  } else {
#if defined(__SWITCH__)
    user_dir = std::filesystem::path("sdmc:/switch/guardian-heroes/user");
#else
    user_dir = rex::filesystem::GetUserFolder() / GetName();
#endif
  }

  // Update data: cvar override, or empty (opt-in)
  std::filesystem::path update_dir;
  std::string update_data_cvar = REXCVAR_GET(update_data_root);
  if (!update_data_cvar.empty()) {
    update_dir = update_data_cvar;
  }

  // Cache: cvar override, or user_dir/cache
  std::filesystem::path cache_dir;
  std::string cache_root_cvar = REXCVAR_GET(cache_root);
  if (!cache_root_cvar.empty()) {
    cache_dir = cache_root_cvar;
  } else {
    cache_dir = user_dir / "cache";
  }

  std::filesystem::path metadata_dir;
  std::string metadata_root_cvar = REXCVAR_GET(metadata_root);
  if (!metadata_root_cvar.empty()) {
    metadata_dir = metadata_root_cvar;
  }

#if defined(__SWITCH__)
  const auto config_path = std::filesystem::path("sdmc:/switch/guardian-heroes") /
                           (std::string(GetName()) + ".toml");
#else
  const auto config_path = exe_dir / (std::string(GetName()) + ".toml");
#endif
  Paths path_config{game_dir,  user_dir,     update_dir,
                         cache_dir, metadata_dir, config_path};
  game_data_root_ = path_config.game_data_root;
  user_data_root_ = path_config.user_data_root;
  update_data_root_ = path_config.update_data_root;
  cache_root_ = path_config.cache_root;
  metadata_root_ = path_config.metadata_root;
  config_path_ = path_config.config_path;
  resolved_defaults_ = std::move(path_config);

  // Load config FIRST so log cvars have final values
  if (std::filesystem::exists(config_path_))
    rex::cvar::LoadConfig(config_path_);

  // Late-phase logging
  std::string log_file_cvar = REXCVAR_GET(log_file);
  std::string log_level_str = REXCVAR_GET(log_level);
  if (REXCVAR_GET(log_verbose) && log_level_str == "info")
    log_level_str = "trace";

  auto category_levels = rex::ParseCategoryLevelsFromConfig(config_path_);
  auto log_config = rex::BuildLogConfig(log_file_cvar.empty() ? nullptr : log_file_cvar.c_str(),
                                        log_level_str, category_levels);
  if (log_file_cvar.empty()) {
    log_config.app_name = std::string(GetName());
    log_config.log_dir = (exe_dir / "logs").string();
  }

  rex::InitLogging(log_config);
  rex::RegisterLogLevelCallback();

  log_sink_ = std::make_shared<rex::LogCaptureSink>();
  rex::AddSink(log_sink_);

  InitializeProjectServices();

  if (std::filesystem::exists(config_path_))
    REXLOG_DEBUG("Loaded config: {}", config_path_.filename().string());

  REXLOG_DEBUG("{} starting", GetName());
  if (!game_data_root_.empty()) {
    REXLOG_DEBUG("  Game directory: {}", game_data_root_.string());
  }
  if (!user_data_root_.empty()) {
    REXLOG_DEBUG("  User data:      {}", user_data_root_.string());
  }
  if (!update_data_root_.empty()) {
    REXLOG_DEBUG("  Update data:    {}", update_data_root_.string());
  }
  REXLOG_DEBUG("  Cache root:     {}", cache_root_.string());
  if (!metadata_root_.empty()) {
    REXLOG_DEBUG("  Metadata root:  {}", metadata_root_.string());
  }

  return true;
}

bool GuardianHeroesApp::ConstructRuntime(const Paths& paths) {
  if (paths.game_data_root.empty()) {
    auto msg = std::string("Game data not found. Place the assets folder (containing "
                           "default.xex) next to guardian_heroes.exe, or specify "
                           "--game_data_root.");
    REXLOG_ERROR("{}", msg);
    rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error, msg);
    return false;
  }
  if (!std::filesystem::is_directory(paths.game_data_root)) {
    auto msg = fmt::format("--game_data_root does not exist: {}", paths.game_data_root.string());
    REXLOG_ERROR("{}", msg);
    rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error, msg);
    return false;
  }

  game_data_root_ = paths.game_data_root;
  user_data_root_ = paths.user_data_root;
  update_data_root_ = paths.update_data_root;
  cache_root_ = paths.cache_root;
  metadata_root_ = paths.metadata_root;

  runtime_ =
      std::make_unique<rex::Runtime>(paths.game_data_root, paths.user_data_root,
                                     paths.update_data_root, paths.cache_root, paths.metadata_root);
  runtime_->set_app_context(&app_context());

  // Window and ImGui drawer already exist from SetupPresentation; publish them
  // to the runtime before Setup so hooks and native rendering see them.
  if (window_) {
    runtime_->set_display_window(window_.get());
  }
  if (imgui_drawer_) {
    runtime_->set_imgui_drawer(imgui_drawer_.get());
  }

  auto status = runtime_->Setup(ppc_info_, std::move(config_));
  if (XFAILED(status)) {
    REXLOG_ERROR("Runtime setup failed: {:08X}", status);
    return false;
  }

  if (window_ && runtime_->input_system()) {
    static_cast<rex::input::InputSystem*>(runtime_->input_system())->AttachWindow(window_.get());
  }

  if (ppc_info_.register_modules) {
    ppc_info_.register_modules(runtime_->kernel_state());
  }

  if (imgui_drawer_) {
    auto* input_sys = static_cast<rex::input::InputSystem*>(runtime_->input_system());
    if (input_sys) {
      input_sys->SetActiveCallback([this]() {
        if (!debug_overlay_ && !console_overlay_ && !settings_overlay_ && !achievements_overlay_)
          return true;
        return !imgui_drawer_->GetIO().WantCaptureMouse;
      });
    }
  }

  std::string xex_image = "game:\\default.xex";

  // Mirrors the game:\ / d:\ -> game_data_root mapping in Runtime::SetupVfs.
  {
    constexpr std::string_view kGameDevice = "game:\\";
    constexpr std::string_view kDDevice = "d:\\";
    std::string_view tail = xex_image;
    if (tail.starts_with(kGameDevice)) {
      tail.remove_prefix(kGameDevice.size());
    } else if (tail.starts_with(kDDevice)) {
      tail.remove_prefix(kDDevice.size());
    }
    std::string host_tail{tail};
    std::replace(host_tail.begin(), host_tail.end(), '\\', '/');
    auto xex_host = paths.game_data_root / host_tail;
    if (!std::filesystem::is_regular_file(xex_host)) {
      auto msg = fmt::format("Entrypoint XEX not found: {}", xex_host.string());
      REXLOG_ERROR("{}", msg);
      rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error, msg);
      return false;
    }
  }

  status = runtime_->LoadXexImage(xex_image);
  if (XFAILED(status)) {
    auto msg = fmt::format("Failed to load XEX ({}): {:08X}", xex_image, status);
    REXLOG_ERROR("{}", msg);
    rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error, msg);
    return false;
  }

  InitializeGuestImage();

  if (ppc_info_.rexcrt_heap) {
    if (!rex::kernel::crt::InitHeap(REXCVAR_GET(rexcrt_heap_size_mb), runtime_->memory())) {
      REXLOG_ERROR("Failed to initialize rexcrt heap");
      return false;
    }
  }

  return true;
}

bool GuardianHeroesApp::SetupPresentation() {
  config_.gpu_plugin = REXCVAR_GET(gpu_plugin);
#if defined(__SWITCH__)
  config_.audio_factory = gh::audio::switch_::CreateSwitchAudioSystem;
#else
  config_.audio_factory = gh::audio::CreateSdlAudioSystem;
#endif
  config_.input_factory = REX_INPUT_BACKEND(rex::input::CreateDefaultInputSystem);
  config_.kernel_init = rex::kernel::InitializeKernel;

  ConfigureRuntime(config_);

  if (!config_.graphics && !config_.gpu_plugin.empty()) {
#ifdef GH_PROJECT_RUNTIME
    config_.graphics = gh::graphics::CreateGraphicsSystem(config_.gpu_plugin, "any");
#else
    config_.graphics = rex::system::LoadGpuPlugin(config_.gpu_plugin);
#endif
    if (!config_.graphics) {
      // Fatal by design: no silent headless fallback.
      auto msg =
          fmt::format("Failed to load GPU plugin '{}'. See log for details.", config_.gpu_plugin);
      REXLOG_ERROR("{}", msg);
      rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error, msg);
      return false;
    }
  }

  if (config_.graphics) {
    rex::X_STATUS status = config_.graphics->SetupPresentation(&app_context());
    if (XFAILED(status)) {
      REXLOG_ERROR("Graphics presentation setup failed: {:08X}", status);
      return false;
    }
  }

  // Create window
  window_ = rex::ui::Window::Create(app_context(), "Guardian Heroes HD", 1280, 720);
  if (!window_) {
    REXLOG_ERROR("Failed to create window");
    return false;
  }

  window_->AddListener(this);
  window_->AddInputListener(this, 0);

  if (REXCVAR_GET(fullscreen)) {
    window_->SetFullscreen(true);
  }
  rex::cvar::RegisterChangeCallback("fullscreen", [this](std::string_view, std::string_view value) {
    if (window_) {
      window_->SetFullscreen(rex::string::from_string<bool>(value, false));
    }
  });
  window_->Open();

  auto* graphics_system = config_.graphics.get();
  if (graphics_system && graphics_system->presenter()) {
    // SDK mode: the emulated-Xenos presenter drives the overlays.
    auto* presenter = graphics_system->presenter();
    auto* provider = graphics_system->provider();
    if (provider) {
      immediate_drawer_ = provider->CreateImmediateDrawer();
      if (immediate_drawer_) {
        immediate_drawer_->SetPresenter(presenter);
        SetupOverlays(presenter, immediate_drawer_.get());
      }
    }
    window_->SetPresenter(presenter);
  }

  return true;
}

void GuardianHeroesApp::SetupOverlays(rex::ui::Presenter* presenter, rex::ui::ImmediateDrawer* drawer) {
  imgui_drawer_ = std::make_unique<rex::ui::ImGuiDrawer>(
      window_.get(), 64);
  imgui_drawer_->SetPresenterAndImmediateDrawer(presenter, drawer);
  rex::ui::RegisterBind("bind_debug_overlay", "F2", "Toggle debug overlay", [this] {
    if (debug_overlay_) {
      debug_overlay_.reset();
    } else {
      debug_overlay_ =
          std::make_unique<rex::ui::DebugOverlayDialog>(imgui_drawer_.get(), frame_stats_provider_);
    }
  });
  rex::ui::RegisterBind("bind_console", "Backtick", "Toggle console overlay", [this] {
    if (console_overlay_) {
      console_overlay_.reset();
    } else {
      console_overlay_ = std::make_unique<rex::ui::ConsoleDialog>(imgui_drawer_.get(), log_sink_);
    }
  });
  rex::ui::RegisterBind("bind_settings", "F4", "Toggle settings overlay", [this] {
    if (settings_overlay_) {
      settings_overlay_.reset();
    } else {
      settings_overlay_ = std::make_unique<rex::ui::SettingsDialog>(imgui_drawer_.get(), config_path_);
    }
  });
  rex::ui::RegisterBind("bind_achievements", "F7", "Toggle achievements overlay", [this] {
    if (achievements_overlay_) {
      achievements_overlay_.reset();
    } else {
      achievements_overlay_ = CreateAchievementsOverlay();
    }
  });
#ifdef GH_PROJECT_RUNTIME
  if (gh::guest::continuation::TitleProgram()) {
    rex::ui::RegisterBind("bind_snapshot_save", "F5", "Save running state", [this] {
      RunSnapshotOperation(false);
    });
    rex::ui::RegisterBind("bind_snapshot_restore", "F9", "Restore running state", [this] {
      RunSnapshotOperation(true);
    });
  }
#endif

}

#ifdef GH_PROJECT_RUNTIME
void GuardianHeroesApp::RunSnapshotOperation(bool restore) {
  if (shutting_down_.load(std::memory_order_acquire) ||
      snapshot_busy_.exchange(true, std::memory_order_acq_rel)) return;
  if (snapshot_thread_.joinable()) snapshot_thread_.join();
  snapshot_thread_ = std::jthread([this, restore] {
    struct Complete {
      std::atomic<bool>& busy;
      ~Complete() { busy.store(false, std::memory_order_release); }
    } complete{snapshot_busy_};
    try {
      if (restore) {
        if (!snapshot_size_) {
          REXLOG_WARN("No running-state snapshot has been saved");
          return;
        }
        rex::stream::ByteStream input(snapshot_data_.get(), snapshot_size_);
        if (runtime_->Restore(&input))
          REXLOG_INFO("Running-state snapshot restored ({} bytes)", snapshot_size_);
        else
          REXLOG_ERROR("Running-state snapshot restore refused or failed");
      } else {
        // Reserve address space without touching unused pages. The stream
        // checks bounds and refuses an oversized capture before copying it.
        constexpr size_t capacity = size_t{2} << 30;
        auto data = std::unique_ptr<uint8_t[]>(new uint8_t[capacity]);
        rex::stream::ByteStream output(data.get(), capacity);
        if (runtime_->Save(&output)) {
          snapshot_size_ = output.offset();
          snapshot_data_ = std::move(data);
          REXLOG_INFO("Running-state snapshot saved ({} bytes)", snapshot_size_);
        } else {
          REXLOG_WARN("Running-state snapshot refused; previous snapshot retained");
        }
      }
    } catch (const std::exception& error) {
      REXLOG_ERROR("Running-state snapshot operation failed: {}", error.what());
    }
  });
}
#endif

void GuardianHeroesApp::LaunchModule() {
  app_context().CallInUIThreadDeferred([this]() {
    // KernelState is available only after runtime construction.
    if (!achievement_notification_) {
      achievement_notification_ =
          std::shared_ptr<rex::ui::AchievementNotificationDialog>(CreateAchievementNotificationDialog());
    }
    if (achievement_notification_ && achievement_notification_listener_ == 0 && runtime_ &&
        runtime_->kernel_state()) {
      std::weak_ptr<rex::ui::AchievementNotificationDialog> notification = achievement_notification_;
      achievement_notification_listener_ = achievements().RegisterNotificationCallback(
          [notification](const rex::system::AchievementEvent& event) {
            if (auto dialog = notification.lock()) {
              dialog->Push(event);
            }
          });
    }

    auto main_thread = runtime_->PrepareModuleLaunch();
    if (!main_thread) {
      REXLOG_ERROR("Failed to launch module");
      app_context().QuitFromUIThread();
      return;
    }

    auto* graphics_system = runtime_->graphics_system();
    if (graphics_system && !runtime_->cache_root().empty()) {
      uint32_t title_id = runtime_->kernel_state()->title_id();
      if (title_id != 0) {
        REXLOG_INFO("Initializing shader storage for title {:08X}...", title_id);
        graphics_system->InitializeShaderStorage(runtime_->cache_root(), title_id, true);
      }
    }

    main_thread->Resume();

    module_thread_ = std::thread([this, main_thread = std::move(main_thread)]() mutable {
#ifdef GH_PROJECT_RUNTIME
      runtime_->WaitForTitleCompletion();
#else
      main_thread->Wait(0, 0, 0, nullptr);
#endif
      REXLOG_INFO("Execution complete");
      if (!shutting_down_.load(std::memory_order_acquire)) {
        app_context().CallInUIThread([this]() { app_context().QuitFromUIThread(); });
      }
    });
  });
}

void GuardianHeroesApp::OnKeyDown(rex::ui::KeyEvent& e) {
  rex::ui::ProcessKeyEvent(e);
}

void GuardianHeroesApp::OnClosing(rex::ui::UIEvent& e) {
  (void)e;
  REXLOG_INFO("Window closing, shutting down...");
  shutting_down_.store(true, std::memory_order_release);
  if (runtime_ && runtime_->kernel_state()) {
    runtime_->kernel_state()->TerminateTitle();
  }
  // Hard-exit rather than run subsystem teardown, which can deadlock on a host
  // lock still held by a straggler TerminateTitle left running. Flush (not
  // ShutdownLogging, which frees loggers a straggler may still use); the OS
  // reclaims the rest.
  REXLOG_INFO("Title terminated; hard-exiting process.");
  rex::FlushLogging();
  std::_Exit(0);
}

bool GuardianHeroesApp::OnCloseRequested(rex::ui::UIEvent& e) {
  (void)e;
  return true;
}

void GuardianHeroesApp::OnDestroy() {
#ifdef GH_PROJECT_RUNTIME
  shutting_down_.store(true, std::memory_order_release);
  if (snapshot_thread_.joinable()) snapshot_thread_.join();
#endif
  ShutdownProjectServices();

  // Unregister overlay keybinds before destroying dialogs
  rex::ui::UnregisterBind("bind_debug_overlay");
  rex::ui::UnregisterBind("bind_console");
  rex::ui::UnregisterBind("bind_settings");
  rex::ui::UnregisterBind("bind_achievements");
#ifdef GH_PROJECT_RUNTIME
  rex::ui::UnregisterBind("bind_snapshot_save");
  rex::ui::UnregisterBind("bind_snapshot_restore");
#endif

  // ImGui cleanup (reverse of setup)
  if (achievement_notification_listener_ != 0) {
    if (runtime_ && runtime_->kernel_state()) {
      achievements().UnregisterCallback(achievement_notification_listener_);
    }
    achievement_notification_listener_ = 0;
  }
  achievement_notification_.reset();
  achievements_overlay_.reset();
  settings_overlay_.reset();
  console_overlay_.reset();
  debug_overlay_.reset();
  if (imgui_drawer_) {
    imgui_drawer_->SetPresenterAndImmediateDrawer(nullptr, nullptr);
    imgui_drawer_.reset();
  }
  // Unlink the drawer before disposing its presenter.
  if (immediate_drawer_) {
    immediate_drawer_->SetPresenter(nullptr);
    immediate_drawer_.reset();
  }
  if (runtime_) {
    runtime_->set_display_window(nullptr);
    runtime_->set_imgui_drawer(nullptr);
  }
  // Window/runtime cleanup
  if (window_) {
    window_->SetPresenter(nullptr);
  }
  if (module_thread_.joinable()) {
    module_thread_.join();
  }
  if (window_) {
    window_->RemoveInputListener(this);
    window_->RemoveListener(this);
  }
  window_.reset();
  runtime_.reset();
}

void GuardianHeroesApp::ConfigureRuntime(rex::RuntimeConfig& config) {
    gh::thread::GlobalCriticalRegion::BindRuntimeMutex(
        &rex::thread::global_critical_region::mutex());
    gh::clock::BindReXGuestClock();
    if (!config.graphics && config.gpu_plugin.empty() &&
        rex::cvar::GetFlagSource("gpu_plugin") == rex::cvar::Source::kDefault) {
      config.gpu_plugin = "xenos";
    }
#if defined(__SWITCH__)
    config.input_factory = REX_INPUT_BACKEND(rex::input::switch_::CreateSwitchInputSystem);
#else
    config.input_factory = [](bool tool_mode) -> std::unique_ptr<rex::system::IInputSystem> {
      auto input = rex::input::CreateDefaultInputSystem(tool_mode);
      input->SetDeviceAssignment(std::make_unique<gh::input::ControllerAssignment>());
      return input;
    };
#endif
  }

void GuardianHeroesApp::InitializeProjectServices() {
    gh::logging::Initialize(user_data_root_ / "guardian_heroes.log");
    gh::exceptions::InstallTerminateHandler();
    gh::logging::Info("Project clock, exception, and logging services initialized");
  }

void GuardianHeroesApp::InitializeGuestImage() {
    gh::memory::BindReXGuestMemory();
    gh::localization::Initialize(
        game_data_root_, user_data_root_, runtime_->memory(),
        [context = &app_context(), window = std::weak_ptr(window_)](bool chinese) {
          context->CallInUIThread([window, chinese]() {
            if (const auto target = window.lock()) {
              target->SetTitle(chinese ? "守护英雄 HD" : "Guardian Heroes HD");
            }
          });
        });
  }

void GuardianHeroesApp::ShutdownProjectServices() {
    gh::memory::UnbindGuestMemory();
    gh::logging::Shutdown();
  }
