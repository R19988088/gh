/**
 * @file        runtime/runtime.cpp
 * @brief       Runtime subsystem implementation
 *
 * @copyright   Copyright (c) 2026 Tom Clay <tomc@tctechstuff.com>
 *              All rights reserved.
 *
 * @license     BSD 3-Clause License
 *              See LICENSE file in the project root for full license text.
 */

#include <rex/chrono/clock.h>
#include <rex/cvar.h>
#include <rex/filesystem/devices/host_path_device.h>
#include <rex/filesystem/devices/null_device.h>
#include <rex/filesystem/vfs.h>
#include <rex/logging.h>
#include <rex/perf/counter.h>
#include <rex/ppc/context.h>          // PPCFuncMapping
#include <rex/platform/exceptions.h>  // SEH exception support
#include <rex/kernel/crt/heap.h>
#include "gh_system_runtime.h"
#include "gh_system_export_resolver.h"
#include "gh_system_kernel_state.h"
#include <rex/system/function_dispatcher.h>
#include "gh_system_user_module.h"
#include <rex/system/xmemory.h>
#include "gh_system_xthread.h"
#include <rex/thread.h>
#include <rex/stream.h>
#include <array>
#include "gh_clock.h"

REXCVAR_DEFINE_STRING(game_data_root, "", "Runtime", "Override game data path");
REXCVAR_DEFINE_STRING(user_data_root, "", "Runtime", "Override user data path");
REXCVAR_DEFINE_STRING(update_data_root, "", "Runtime", "Override update data path");
REXCVAR_DEFINE_STRING(cache_root, "", "Runtime", "Override shader cache path");
REXCVAR_DEFINE_STRING(metadata_root, "", "Runtime", "Override metadata path");

namespace rex {

// Static instance for global access
Runtime* Runtime::instance_ = nullptr;

Runtime* Runtime::instance() {
  return instance_;
}

Runtime::Runtime(const std::filesystem::path& game_data_root,
                 const std::filesystem::path& user_data_root,
                 const std::filesystem::path& update_data_root,
                 const std::filesystem::path& cache_root,
                 const std::filesystem::path& metadata_root)
    : game_data_root_(game_data_root),
      user_data_root_(user_data_root.empty() ? game_data_root : user_data_root),
      update_data_root_(update_data_root),
      cache_root_(cache_root),
      metadata_root_(metadata_root) {}

Runtime::~Runtime() {
  Shutdown();
}

std::optional<std::filesystem::path> Runtime::FindMetadataPath(
    const std::filesystem::path& relative_path) const {
  if (!metadata_root_.empty()) {
    std::filesystem::path candidate = metadata_root_ / relative_path;
    std::error_code ec;
    if (std::filesystem::exists(candidate, ec)) {
      return candidate;
    }
    return std::nullopt;
  }

  const std::filesystem::path candidates[] = {
      game_data_root_ / "metadata" / relative_path,
      game_data_root_.parent_path() / "metadata" / relative_path,
      game_data_root_ / relative_path,
  };
  for (const auto& candidate : candidates) {
    std::error_code ec;
    if (std::filesystem::exists(candidate, ec)) {
      return candidate;
    }
  }
  return std::nullopt;
}

std::optional<EmbeddedMetadataAsset> Runtime::FindEmbeddedMetadata(
    const std::filesystem::path& relative_path) const {
  return FindEmbeddedMetadataAsset(relative_path);
}

X_STATUS Runtime::Setup(RuntimeConfig config) {
  if (instance_ != nullptr) {
    REXSYS_ERROR("Runtime::Setup() called but global instance already exists");
    return X_STATUS_UNSUCCESSFUL;
  }
  instance_ = this;

  auto fail = [this](X_STATUS status, std::string_view reason) {
    REXSYS_ERROR("Runtime::Setup failed: {}", reason);
    Shutdown();
    return status;
  };

  // Initialize SEH exception support for hardware exception handling
  rex::initialize_seh();

  // Initialize clock
  chrono::Clock::set_guest_tick_frequency(50000000);
  chrono::Clock::set_guest_system_time_base(chrono::Clock::QueryHostSystemTime());
  chrono::Clock::set_guest_time_scalar(1.0);

  // Enable threading affinity configuration
  thread::EnableAffinityConfiguration();

  tool_mode_ = config.tool_mode;

  // Create memory system first
  memory_ = std::make_unique<memory::Memory>();
  if (!memory_->Initialize()) {
    return fail(X_STATUS_UNSUCCESSFUL, "memory init failed");
  }

  export_resolver_ = std::make_unique<runtime::ExportResolver>();

  function_dispatcher_ =
      std::make_unique<runtime::FunctionDispatcher>(memory_.get(), export_resolver_.get());

  // Create virtual file system
  file_system_ = std::make_unique<rex::filesystem::VirtualFileSystem>();

  // Create kernel state - this sets the global singleton
  kernel_state_ = std::make_unique<system::KernelState>(this);

  // Initialize input from injected config
  if (config.input_factory) {
    input_system_ = config.input_factory(tool_mode_);
    if (input_system_) {
      X_STATUS input_status = input_system_->Setup();
      if (XFAILED(input_status)) {
        REXSYS_WARN("Failed to initialize input system (status {:08X}) - input disabled",
                    input_status);
        input_system_.reset();
      } else {
        REXSYS_DEBUG("Input system initialized");
      }
    }
  }

  // HLE kernel modules and apps.
  if (config.kernel_init) {
    config.kernel_init(this, kernel_state_.get());
  }

  // Initialize the APU (Audio Processing Unit) from injected config
  if (config.audio_factory) {
    audio_system_ = config.audio_factory(function_dispatcher_.get());
    if (audio_system_) {
      X_STATUS audio_status = audio_system_->Setup(kernel_state_.get());
      if (XFAILED(audio_status)) {
        REXSYS_WARN("Failed to initialize audio system (status {:08X}) - audio disabled",
                    audio_status);
        audio_system_.reset();
      } else {
        REXSYS_DEBUG("Audio system initialized");
      }
    }
  }

  // Set up VFS: game_data_root as game:/d:, update_data_root as update:
  if (!SetupVfs()) {
    return fail(X_STATUS_UNSUCCESSFUL, "VFS setup failed");
  }

  // Skip GPU initialization in tool mode (for analysis tools like codegen)
  if (tool_mode_) {
    REXSYS_DEBUG("Runtime initialized in tool mode (no GPU)");
    setup_complete_ = true;
    return X_STATUS_SUCCESS;
  }

  // Initialize GPU from injected config
  if (config.graphics) {
    graphics_system_ = std::move(config.graphics);
    bool with_presentation = (app_context_ != nullptr);
    X_STATUS gpu_status = graphics_system_->Setup(function_dispatcher_.get(), kernel_state_.get(),
                                                  app_context_, with_presentation);
    if (XFAILED(gpu_status)) {
      return fail(gpu_status, "GPU setup failed");
    }
    REXSYS_DEBUG("GPU system initialized (presentation={})", with_presentation);
  } else {
    REXSYS_DEBUG("Runtime initialized without graphics system (native rendering mode)");
  }

  REXSYS_DEBUG("Runtime initialized successfully");
  setup_complete_ = true;
  return X_STATUS_SUCCESS;
}

X_STATUS Runtime::Setup(const rex::PPCImageInfo& image_info, RuntimeConfig config) {
  X_STATUS status = Setup(std::move(config));
  if (status != X_STATUS_SUCCESS) {
    return status;
  }

  codegen_flags_ = image_info.codegen_flags;

  if (!function_dispatcher_->InitializeFunctionTable(image_info.code_base, image_info.code_size,
                                                     image_info.image_base, image_info.image_size,
                                                     /*is_entrypoint=*/true)) {
    REXSYS_ERROR("Failed to initialize function table");
    Shutdown();
    return X_STATUS_UNSUCCESSFUL;
  }

  if (image_info.func_mappings) {
    int count = 0;
    int duplicates = 0;
    int rejected = 0;
    for (int i = 0; image_info.func_mappings[i].guest != 0; ++i) {
      uint32_t guest = static_cast<uint32_t>(image_info.func_mappings[i].guest);
      auto* host = image_info.func_mappings[i].host;
      if (!host) {
        continue;
      }
      if (function_dispatcher_->GetFunction(guest)) {
        REXSYS_WARN("func_mappings: duplicate guest address {:08X}", guest);
        ++duplicates;
      }
      if (!function_dispatcher_->SetFunction(guest, host)) {
        ++rejected;
      } else {
        ++count;
      }
    }
    REXSYS_DEBUG("Registered {} recompiled functions ({} duplicates, {} rejected)", count,
                 duplicates, rejected);
    if (rejected > 0) {
      REXSYS_ERROR("PPCImageInfo registration: {} func_mappings entries rejected", rejected);
      Shutdown();
      return X_STATUS_UNSUCCESSFUL;
    }
  }

  REXSYS_DEBUG("Runtime setup complete (code: {:08X}-{:08X}, image: {:08X}-{:08X})",
               image_info.code_base, image_info.code_base + image_info.code_size,
               image_info.image_base, image_info.image_base + image_info.image_size);

  const auto& flags = image_info.codegen_flags;
  REXSYS_DEBUG(
      "Codegen config: skip_lr={} skip_msr={} ctr={} xer={} cr={} reserved={} "
      "non_argument={} non_volatile={}",
      flags.skip_lr, flags.skip_msr, flags.ctr_as_local, flags.xer_as_local, flags.cr_as_local,
      flags.reserved_as_local, flags.non_argument_as_local, flags.non_volatile_as_local);

  return X_STATUS_SUCCESS;
}

void Runtime::Shutdown() {
  std::lock_guard operation(snapshot_mutex_);
  if (!instance_ && !setup_complete_ && !memory_) {
    return;
  }

  if (instance_ == this) {
    instance_ = nullptr;
  }
  {
    std::lock_guard lock(title_mutex_);
    shutting_down_ = true;
    title_main_thread_.reset();
    title_changed_.notify_all();
  }

  // Snapshot failures retain stopped hosts and device ownership until teardown.
  // Drain the hosts while their kernel, audio callbacks and guest heap exist.
  if (snapshot_restore_failed_ && kernel_state_) {
    kernel_state_->TerminateTitle();
    failed_snapshot_stop_.reset();
  }
  if (failed_snapshot_clock_paused_) {
    chrono::Clock::ResumeGuestClock();
    failed_snapshot_clock_paused_ = false;
  }

  if (graphics_system_) {
    graphics_system_->Shutdown();
    graphics_system_.reset();
  }
  if (audio_system_) {
    audio_system_->Shutdown();
    audio_system_.reset();
  }
  if (input_system_) {
    input_system_->Shutdown();
    input_system_.reset();
  }
  kernel_state_.reset();
  function_dispatcher_.reset();
  export_resolver_.reset();
  file_system_.reset();
  memory_.reset();

  rex::perf::Profiler::Shutdown();
  setup_complete_ = false;
}

uint8_t* Runtime::virtual_membase() const {
  return memory_ ? memory_->virtual_membase() : nullptr;
}

bool Runtime::SetupVfs() {
  if (game_data_root_.empty()) {
    REXSYS_WARN("Runtime::SetupVfs: No game_data_root specified, skipping VFS setup");
    return true;
  }

  // libnx's sdmc:/ prefix is a mounted filesystem name, not a POSIX
  // relative path. std::filesystem::absolute would incorrectly prepend the
  // process working directory on Switch.
  auto abs_game_root = game_data_root_;
#if !defined(__SWITCH__)
  abs_game_root = std::filesystem::absolute(abs_game_root);
#endif
  if (!std::filesystem::exists(abs_game_root)) {
    REXSYS_ERROR("Runtime::SetupVfs: game_data_root does not exist: {}", abs_game_root.string());
    return false;
  }

  // Mount game_data_root as \Device\Harddisk0\Partition1
  auto mount_path = "\\Device\\Harddisk0\\Partition1";
  auto device = std::make_unique<rex::filesystem::HostPathDevice>(
      mount_path, abs_game_root, !REXCVAR_GET(allow_game_relative_writes));
  if (!device->Initialize()) {
    REXSYS_ERROR("Runtime::SetupVfs: Failed to initialize host path device");
    return false;
  }
  if (!file_system_->RegisterDevice(std::move(device))) {
    REXSYS_ERROR("Runtime::SetupVfs: Failed to register host path device");
    return false;
  }
  REXSYS_DEBUG("  Mounted {} at {}", abs_game_root.string(), mount_path);

  // Register symbolic links for game: and D:
  file_system_->RegisterSymbolicLink("game:", mount_path);
  file_system_->RegisterSymbolicLink("d:", mount_path);
  REXSYS_DEBUG("  Registered symbolic links: game:, d:");

  // Mount update_data_root as update:\ if provided
  if (!update_data_root_.empty()) {
    auto abs_update_root = update_data_root_;
#if !defined(__SWITCH__)
    abs_update_root = std::filesystem::absolute(abs_update_root);
#endif
    if (std::filesystem::exists(abs_update_root)) {
      auto update_mount = "\\Device\\Harddisk0\\PartitionUpdate";
      auto update_device =
          std::make_unique<rex::filesystem::HostPathDevice>(update_mount, abs_update_root, true);
      if (update_device->Initialize() && file_system_->RegisterDevice(std::move(update_device))) {
        file_system_->RegisterSymbolicLink("update:", update_mount);
        REXSYS_DEBUG("  Mounted {} at update:", abs_update_root.string());
      }
    }
  }

  // Setup NullDevice for raw HDD partition accesses
  // Cache/STFC code baked into games tries reading/writing to these
  // Using a NullDevice returns success to all IO requests, allowing games
  // to believe cache/raw disk was accessed successfully.
  // NOTE: Must be registered AFTER Partition1 so Partition1 requests don't
  // go to NullDevice (VFS resolves devices in registration order)
  auto null_paths = {std::string("\\Partition0"), std::string("\\Cache0"), std::string("\\Cache1")};
  auto null_device =
      std::make_unique<rex::filesystem::NullDevice>("\\Device\\Harddisk0", null_paths);
  if (null_device->Initialize()) {
    file_system_->RegisterDevice(std::move(null_device));
    REXSYS_DEBUG("  Registered NullDevice for \\Device\\Harddisk0\\{{Partition0,Cache0,Cache1}}");
  }

  // NOTE: Do NOT register a device for cache: paths
  // Games handle "device not found" gracefully but don't handle actual device
  // errors (like NAME_COLLISION) well. Let cache: fail cleanly.

  return true;
}

X_STATUS Runtime::LoadXexImage(const std::string_view module_path) {
  REXSYS_DEBUG("Loading XEX image: {}", std::string(module_path));

  auto module = system::object_ref<system::UserModule>(new system::UserModule(kernel_state_.get()));
  X_STATUS status = module->LoadFromFile(module_path);
  if (XFAILED(status)) {
    REXSYS_ERROR("Runtime::LoadXexImage: Failed to load module, status {:08X}", status);
    return status;
  }

  kernel_state_->SetExecutableModule(module);
  REXSYS_DEBUG("  XEX image loaded successfully");
  return X_STATUS_SUCCESS;
}

system::object_ref<system::XThread> Runtime::PrepareModuleLaunch() {
  auto executable = kernel_state_->GetExecutableModule();
  if (!executable) {
    REXSYS_ERROR("Runtime::PrepareModuleLaunch: No executable module loaded");
    return nullptr;
  }

  auto thread = kernel_state_->PrepareModuleLaunch(executable);
  if (!thread) {
    REXSYS_ERROR("Runtime::PrepareModuleLaunch: Failed to prepare module");
    return nullptr;
  }

  REXSYS_DEBUG("  Module prepared on thread '{}'", thread->name());
  {
    std::lock_guard lock(title_mutex_);
    title_main_thread_ = thread;
  }
  return thread;
}

system::object_ref<system::XThread> Runtime::LaunchModule() {
  auto thread = PrepareModuleLaunch();
  if (thread) {
    thread->Resume();
    REXSYS_DEBUG("  Module launched on thread '{}'", thread->name());
  }
  return thread;
}

void Runtime::WaitForTitleCompletion() {
  std::unique_lock lock(title_mutex_);
  for (;;) {
    title_changed_.wait(lock, [this] {
      return shutting_down_ || (!restoring_snapshot_ && !snapshot_restore_failed_);
    });
    if (shutting_down_ || !title_main_thread_) return;
    auto thread = title_main_thread_;
    lock.unlock();
    thread->Wait(0, 0, 0, nullptr);
    lock.lock();
    if (!restoring_snapshot_ && !snapshot_restore_failed_ && thread.get() == title_main_thread_.get())
      return;
  }
}

namespace {
constexpr uint32_t kRuntimeSnapshotMagic = 0x31535247;  // GRS1
enum SnapshotSection : size_t { kGpuState, kAudioState, kKernelState, kMemoryState, kStateCount };
struct RuntimeSnapshotHeader {
  uint32_t magic = kRuntimeSnapshotMagic;
  uint32_t version = 1;
  uint32_t main_thread_id = 0;
  uint32_t devices = 0;
  uint32_t title_id = 0;
  uint32_t reserved = 0;
  chrono::Clock::GuestState clock{};
  gh::clock::GuestState project_clock{};
  std::array<uint64_t, kStateCount> lengths{};
};
}  // namespace

bool Runtime::Save(stream::ByteStream* stream, std::chrono::milliseconds timeout) {
  if (!stream || system::XThread::IsInThread() || !kernel_state_ || !memory_) return false;
  std::lock_guard operation(snapshot_mutex_);
  {
    std::lock_guard lock(title_mutex_);
    if (snapshot_restore_failed_ || shutting_down_) return false;
  }
  const auto start = stream->offset();
  if (start > stream->data_length() || sizeof(RuntimeSnapshotHeader) > stream->data_length()-start)
    return false;
  gh::guest::continuation::TitleCoordinator::PauseResult paused;
  bool audio_paused = false, timers_paused = false, gpu_paused = false, clock_paused = false, dispatch_paused = false;
  auto resume = [&] {
    if (clock_paused) chrono::Clock::ResumeGuestClock();
    if (dispatch_paused) kernel_state_->ResumeDispatchAfterSnapshot();
    if (timers_paused) kernel_state_->ResumeTimersAfterSnapshot();
    if (gpu_paused) graphics_system_->Resume();
    const bool resumed = !audio_paused || audio_system_->Resume();
    if (!resumed) {
      if (dispatch_paused) kernel_state_->PauseDispatchForSnapshot(std::chrono::milliseconds(0));
      if (timers_paused) kernel_state_->PauseTimersForSnapshot();
      if (gpu_paused) graphics_system_->Pause();
      if (clock_paused) failed_snapshot_clock_paused_ = chrono::Clock::PauseGuestClock();
      failed_snapshot_stop_ = std::move(paused.stop);
      std::lock_guard lock(title_mutex_);
      snapshot_restore_failed_ = true;
    }
    return resumed;
  };
  try {
    if (audio_system_) { audio_system_->Pause(); audio_paused = true; }
    kernel_state_->PauseTimersForSnapshot();
    timers_paused = true;
    if (graphics_system_) { graphics_system_->Pause(); gpu_paused = true; }
    dispatch_paused = true;
    if (!kernel_state_->PauseDispatchForSnapshot(timeout)) { resume(); return false; }
    // Drain complete audio callbacks before parking guest Machines. Parking a
    // callback first would prevent ClientScheduler::Pause from draining it.
    paused = kernel_state_->PauseGuestContinuations(timeout);
    if (!paused.stop) {
      REXSYS_WARN("Runtime snapshot refused: {}", paused.reason);
      resume(); return false;
    }
    if (!kernel_state_->PauseDispatchForSnapshot(std::chrono::milliseconds(0))) { resume(); return false; }
    clock_paused = chrono::Clock::PauseGuestClock();
    if (!clock_paused) { resume(); return false; }
    RuntimeSnapshotHeader header;
    header.devices = (graphics_system_ ? 1u : 0u) | (audio_system_ ? 2u : 0u);
    header.title_id = kernel_state_->GetExecutableModule() ? kernel_state_->title_id() : 0;
    {
      std::lock_guard lock(title_mutex_);
      if (title_main_thread_) header.main_thread_id = title_main_thread_->thread_id();
    }
    if (!chrono::Clock::CaptureGuestState(header.clock)) { resume(); return false; }
    header.project_clock = gh::clock::CaptureGuestState();
    stream->Write(header);
    const auto save = [&](size_t section, auto&& callback) {
      const auto before = stream->offset();
      if (!callback()) return false;
      header.lengths[section] = stream->offset() - before;
      return true;
    };
    // GPU readback can make GPU-authoritative pages visible in guest RAM.
    // Capture memory only after all device and kernel records are stable.
    const bool saved = save(kGpuState, [&] { return !graphics_system_ || graphics_system_->Save(stream); }) &&
        save(kAudioState, [&] { return !audio_system_ || audio_system_->Save(stream); }) &&
        save(kKernelState, [&] { return kernel_state_->Save(stream, *paused.stop); }) &&
        save(kMemoryState, [&] { return memory_->Save(stream); });
    const auto end = stream->offset();
    stream->set_offset(start);
    if (saved) { stream->Write(header); stream->set_offset(end); }
    const bool resumed = resume();
    return saved && resumed;
  } catch (const std::exception& error) {
    stream->set_offset(start);
    REXSYS_ERROR("Runtime snapshot failed: {}", error.what());
    resume();
    return false;
  }
}

bool Runtime::Restore(stream::ByteStream* stream, std::chrono::milliseconds timeout) {
  if (!stream || system::XThread::IsInThread() || !kernel_state_ || !memory_) return false;
  std::lock_guard operation(snapshot_mutex_);
  {
    std::lock_guard lock(title_mutex_);
    if (snapshot_restore_failed_ || shutting_down_) return false;
  }
  const auto start = stream->offset();
  if (start > stream->data_length() || sizeof(RuntimeSnapshotHeader) > stream->data_length()-start)
    return false;
  const auto header = stream->Read<RuntimeSnapshotHeader>();
  const auto device_mask = (graphics_system_ ? 1u : 0u) | (audio_system_ ? 2u : 0u);
  std::array<size_t, kStateCount> offsets{};
  size_t end = stream->offset();
  bool valid = header.magic == kRuntimeSnapshotMagic && header.version == 1 &&
      header.devices == device_mask &&
      header.title_id == (kernel_state_->GetExecutableModule() ? kernel_state_->title_id() : 0);
  for (size_t i = 0; i < kStateCount; ++i) {
    offsets[i] = end;
    if (header.lengths[i] > stream->data_length() - end) { valid = false; break; }
    end += static_cast<size_t>(header.lengths[i]);
  }
  stream->set_offset(start);
  if (!valid) return false;
  gh::guest::continuation::TitleCoordinator::PauseResult paused;
  bool audio_paused = false, timers_paused = false, gpu_paused = false, clock_paused = false, dispatch_paused = false, destructive = false;
  const auto resume = [&] {
    if (clock_paused) chrono::Clock::ResumeGuestClock();
    if (dispatch_paused) kernel_state_->ResumeDispatchAfterSnapshot();
    if (timers_paused) kernel_state_->ResumeTimersAfterSnapshot();
    if (gpu_paused) graphics_system_->Resume();
    const bool resumed = !audio_paused || audio_system_->Resume();
    if (!resumed) {
      if (dispatch_paused) kernel_state_->PauseDispatchForSnapshot(std::chrono::milliseconds(0));
      if (timers_paused) kernel_state_->PauseTimersForSnapshot();
      if (gpu_paused) graphics_system_->Pause();
      if (clock_paused) failed_snapshot_clock_paused_ = chrono::Clock::PauseGuestClock();
      failed_snapshot_stop_ = std::move(paused.stop);
      std::lock_guard lock(title_mutex_);
      snapshot_restore_failed_ = true;
    }
    return resumed;
  };
  const auto fail = [&] {
    if (!destructive) { resume(); return; }
    kernel_state_->CancelRestoredContinuations();
    failed_snapshot_clock_paused_ = failed_snapshot_clock_paused_ || clock_paused;
    std::lock_guard lock(title_mutex_);
    restoring_snapshot_ = false;
    snapshot_restore_failed_ = true;
    title_changed_.notify_all();
  };
  try {
    if (audio_system_) { audio_system_->Pause(); audio_paused = true; }
    kernel_state_->PauseTimersForSnapshot();
    timers_paused = true;
    if (graphics_system_) { graphics_system_->Pause(); gpu_paused = true; }
    dispatch_paused = true;
    if (!kernel_state_->PauseDispatchForSnapshot(timeout)) { resume(); return false; }
    paused = kernel_state_->PauseGuestContinuations(timeout);
    if (!paused.stop) {
      REXSYS_WARN("Runtime restore refused: {}", paused.reason);
      fail(); return false;
    }
    if (!kernel_state_->PauseDispatchForSnapshot(std::chrono::milliseconds(0))) { resume(); return false; }
    clock_paused = chrono::Clock::PauseGuestClock();
    if (!clock_paused) { fail(); return false; }
    // Validate clock configuration before retiring any thread or guest heap.
    chrono::Clock::GuestState current_clock;
    chrono::Clock::CaptureGuestState(current_clock);
    if (!chrono::Clock::RestoreGuestState(header.clock)) { fail(); return false; }
    chrono::Clock::RestoreGuestState(current_clock);
    {
      std::lock_guard lock(title_mutex_);
      restoring_snapshot_ = true;
    }
    destructive = true;
    kernel_state_->TerminateTitle();
    paused.stop.reset();
    for (const auto& thread : kernel_state_->object_table()->GetObjectsByType<system::XThread>())
      if (thread->is_guest_thread() && thread->is_alive()) { fail(); return false; }
    if (audio_system_ && !audio_system_->PrepareRestore()) { fail(); return false; }
    const auto restore = [&](size_t section, auto&& callback) {
      stream::ByteStream section_stream(stream->data() + offsets[section], header.lengths[section]);
      return callback(&section_stream) && section_stream.offset() == header.lengths[section];
    };
    if (!restore(kMemoryState, [&](auto* part) { return memory_->Restore(part); }) ||
        !restore(kKernelState, [&](auto* part) { return kernel_state_->Restore(part, false); }) ||
        !restore(kGpuState, [&](auto* part) { return !graphics_system_ || graphics_system_->Restore(part); }) ||
        !restore(kAudioState, [&](auto* part) { return !audio_system_ || audio_system_->Restore(part); }) ||
        !chrono::Clock::RestoreGuestState(header.clock)) { fail(); return false; }
    gh::clock::RestoreGuestState(header.project_clock);
    if (!kernel_state_->PrepareRestoredContinuations()) { fail(); return false; }
    auto main = header.main_thread_id ? kernel_state_->GetThreadByID(header.main_thread_id) : nullptr;
    if (header.main_thread_id && !main) { fail(); return false; }
    // Restored hosts remain behind the kernel gate until memory and both
    // devices are ready. The app's completion waiter follows the new main host.
    if (!resume()) { fail(); return false; }
    audio_paused = timers_paused = gpu_paused = clock_paused = dispatch_paused = false;
    if (!kernel_state_->ResumeRestoredContinuations()) {
      if (audio_system_) audio_system_->Pause();
      kernel_state_->PauseDispatchForSnapshot(std::chrono::milliseconds(0));
      kernel_state_->PauseTimersForSnapshot();
      if (graphics_system_) graphics_system_->Pause();
      clock_paused = chrono::Clock::PauseGuestClock();
      fail(); return false;
    }
    {
      std::lock_guard lock(title_mutex_);
      title_main_thread_ = std::move(main);
      restoring_snapshot_ = snapshot_restore_failed_ = false;
      title_changed_.notify_all();
    }
    stream->set_offset(end);
    return true;
  } catch (const std::exception& error) {
    REXSYS_ERROR("Runtime restore failed: {}", error.what());
    fail();
    return false;
  }
}

}  // namespace rex
