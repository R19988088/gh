// Application lifecycle derived from ReXGlue (BSD-3-Clause).
// Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>.
#pragma once

#include <atomic>
#include <filesystem>
#include <memory>
#include <thread>
#include <rex/image_info.h>
#include <rex/runtime.h>
#include <rex/ui/imgui_dialog.h>
#include <rex/ui/imgui_drawer.h>
#include <rex/ui/immediate_drawer.h>
#include <rex/ui/overlay/debug_overlay.h>
#include <rex/ui/window.h>
#include <rex/ui/window_listener.h>
#include <rex/ui/windowed_app.h>

namespace rex {
class LogCaptureSink;
namespace ui {
class AchievementNotificationDialog;
class ConsoleDialog;
class SettingsDialog;
}
}

class GuardianHeroesApp final : public rex::ui::WindowedApp,
                               public rex::ui::WindowListener,
                               public rex::ui::WindowInputListener {
 public:
  static std::unique_ptr<rex::ui::WindowedApp> Create(rex::ui::WindowedAppContext& ctx);
  ~GuardianHeroesApp() override;

 private:
  struct Paths {
    std::filesystem::path game_data_root;
    std::filesystem::path user_data_root;
    std::filesystem::path update_data_root;
    std::filesystem::path cache_root;
    std::filesystem::path metadata_root;
    std::filesystem::path config_path;
  };
  GuardianHeroesApp(rex::ui::WindowedAppContext& ctx, rex::PPCImageInfo image);
  bool OnInitialize() override;
  void OnDestroy() override;
  bool SetupEnvironment();
  bool SetupPresentation();
  bool ConstructRuntime(const Paths& paths);
  void LaunchModule();
#ifdef GH_PROJECT_RUNTIME
  void RunSnapshotOperation(bool restore);
#endif
  void ConfigureRuntime(rex::RuntimeConfig& config);
  void InitializeProjectServices();
  void InitializeGuestImage();
  void ShutdownProjectServices();
  void SetupOverlays(rex::ui::Presenter* presenter, rex::ui::ImmediateDrawer* drawer);
  std::unique_ptr<rex::ui::ImGuiDialog> CreateAchievementsOverlay();
  std::unique_ptr<rex::ui::AchievementNotificationDialog> CreateAchievementNotificationDialog();
  rex::system::AchievementManager& achievements() const;
  void OnClosing(rex::ui::UIEvent& e) override;
  bool OnCloseRequested(rex::ui::UIEvent& e) override;
  void OnKeyDown(rex::ui::KeyEvent& e) override;

  rex::PPCImageInfo ppc_info_;
  Paths resolved_defaults_;
  rex::RuntimeConfig config_;
  std::filesystem::path game_data_root_;
  std::filesystem::path user_data_root_;
  std::filesystem::path update_data_root_;
  std::filesystem::path cache_root_;
  std::filesystem::path metadata_root_;
  std::filesystem::path config_path_;
  std::unique_ptr<rex::Runtime> runtime_;
  std::shared_ptr<rex::ui::Window> window_;
  std::thread module_thread_;
#ifdef GH_PROJECT_RUNTIME
  std::atomic<bool> snapshot_busy_{false};
  std::unique_ptr<uint8_t[]> snapshot_data_;
  size_t snapshot_size_ = 0;
  std::jthread snapshot_thread_;
#endif
  std::atomic<bool> shutting_down_{false};
  std::unique_ptr<rex::ui::ImmediateDrawer> immediate_drawer_;
  std::unique_ptr<rex::ui::ImGuiDrawer> imgui_drawer_;
  std::shared_ptr<rex::LogCaptureSink> log_sink_;
  std::unique_ptr<rex::ui::DebugOverlayDialog> debug_overlay_;
  std::unique_ptr<rex::ui::ConsoleDialog> console_overlay_;
  std::unique_ptr<rex::ui::SettingsDialog> settings_overlay_;
  std::unique_ptr<rex::ui::ImGuiDialog> achievements_overlay_;
  std::shared_ptr<rex::ui::AchievementNotificationDialog> achievement_notification_;
  uint64_t achievement_notification_listener_ = 0;
  rex::ui::DebugOverlayDialog::FrameStatsProvider frame_stats_provider_;
};
