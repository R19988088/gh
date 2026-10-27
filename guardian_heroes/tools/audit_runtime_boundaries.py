#!/usr/bin/env python3
"""Audit the project-owned runtime fork without launching the game.

Source markers describe ownership and wiring, never runtime acceptance.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import shlex


def audit_compile_commands(project: Path, build_directory: Path) -> dict:
    path = build_directory / "compile_commands.json"
    result = {"compile_commands": str(path.relative_to(project)) if path.is_relative_to(project)
              else str(path), "present": path.is_file()}
    if not path.is_file():
        result["zero_sdk_source_and_include_references"] = None
        return result
    commands = json.loads(path.read_text())
    sdk_sources, sdk_includes, external_sources = set(), set(), set()
    continuation_sources = set()
    for entry in commands:
        directory = Path(entry["directory"])
        source_path = Path(entry["file"])
        if not source_path.is_absolute():
            source_path = directory / source_path
        if "rexglue-sdk-main" in source_path.parts:
            sdk_sources.add(str(source_path))
        if not source_path.is_relative_to(project):
            external_sources.add(str(source_path))
        if source_path.name.startswith("gh_title_continuation"):
            continuation_sources.add(str(source_path))
        args = entry.get("arguments") or shlex.split(entry["command"])
        for index, argument in enumerate(args):
            include = None
            if argument in ("-I", "-isystem", "-iquote", "-include", "-F", "-iframework"):
                include = args[index + 1] if index + 1 < len(args) else None
            elif argument.startswith(("-I", "-isystem", "-iquote", "/I")):
                include = re.sub(r"^(?:-isystem|-iquote|-I|/I)", "", argument)
            if include and "rexglue-sdk-main" in Path(include).parts:
                sdk_includes.add(include)
    result.update({
        "translation_units": len(commands),
        "sdk_source_references": sorted(sdk_sources),
        "sdk_include_references": sorted(sdk_includes),
        "source_paths_outside_project": sorted(external_sources),
        "title_continuation_translation_units": len(continuation_sources),
        "zero_sdk_source_and_include_references": not (sdk_sources or sdk_includes),
        "build_or_runtime_success_proven": False,
    })
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path,
                        help="Configured standalone build directory (default: out/native-app)")
    args = parser.parse_args()
    project = Path(__file__).resolve().parents[1]
    build_directory = (args.build_dir or project / "out/native-app").absolute()
    native = sorted((project / "src").glob("native_*.cpp"))
    native_text = "\n".join(path.read_text() for path in native)
    mappings = (project / "generated/default/guardian_heroes_init.cpp").read_text()
    pch = (project / "generated/default/guardian_heroes_pch.h").read_text()
    cmake = (project / "CMakeLists.txt").read_text()
    app = "\n".join((project / path).read_text() for path in (
        "src/guardian_heroes_app.h", "src/guardian_heroes_app.cpp"))
    host_cmake = (project / "cmake/GuardianHeroesApp.cmake").read_text()
    source = {
        path.name: path.read_text()
        for path in (project / "src").glob("*")
        if path.is_file()
    }
    rexglue_cmake = (project / "generated/rexglue.cmake").read_text()
    runtime = project / "runtime"
    runtime_cmake = (runtime / "CMakeLists.txt").read_text()
    system_cmake = (project / "cmake/GuardianHeroesSystem.cmake").read_text()
    audio_cmake = (project / "cmake/GuardianHeroesAudio.cmake").read_text()
    gpu_cmake = (project / "cmake/GuardianHeroesGpu.cmake").read_text()
    runtime_system_cmake = (runtime / "src/system/CMakeLists.txt").read_text()
    runtime_gpu_cmake = (runtime / "src/graphics/CMakeLists.txt").read_text()
    bootstrap = (project / "cmake/GuardianHeroesStandaloneBootstrap.cmake").read_text()
    runtime_xobject = source["gh_system_xobject.cpp"]
    runtime_xevent = source["gh_system_xevent.cpp"]
    runtime_xsemaphore = source["gh_system_xsemaphore.cpp"]
    runtime_xtimer = source["gh_system_xtimer.cpp"]
    runtime_xmutant = source["gh_system_xmutant.cpp"]
    runtime_xthread = source["gh_system_xthread.cpp"]
    runtime_xfile = source["gh_system_xfile.cpp"]
    runtime_xnotify = source["gh_system_xnotifylistener.cpp"]
    runtime_kernel_state = source["gh_system_kernel_state.cpp"]
    runtime_kernel_header = source["gh_system_kernel_state.h"]
    runtime_object_table = source["gh_system_util_object_table.cpp"]
    runtime_delay = runtime_xthread.partition("X_STATUS XThread::Delay(")[2].split(
        "struct ThreadSavedState", 1
    )[0]
    waitable_sources = {
        "event": runtime_xevent, "semaphore": runtime_xsemaphore,
        "mutant": runtime_xmutant, "timer": runtime_xtimer,
        "thread": runtime_xthread, "file": runtime_xfile,
        "notification": runtime_xnotify,
    }
    runtime_xrtl = (runtime / "src/kernel/xboxkrnl/xboxkrnl_rtl.cpp").read_text()
    runtime_threading = (runtime / "src/kernel/xboxkrnl/xboxkrnl_threading.cpp").read_text()
    runtime_memory = source["gh_system_xmemory.cpp"]
    runtime_image = source["gh_system_xex_module.cpp"]
    runtime_lzx = source["gh_system_lzx.cpp"]
    runtime_sdl = source["gh_audio_sdl_driver.cpp"]
    runtime_mmio = source["gh_system_mmio_handler.cpp"]
    runtime_xma = source["gh_audio_context.cpp"]
    runtime_xma_decoder = source["gh_audio_decoder.cpp"]
    runtime_xma_decoder_header = source["gh_audio_decoder.h"]
    runtime_audio = source["gh_audio_system.cpp"]
    runtime_xma_header = (runtime / "include/rex/audio/xma/context.h").read_text()
    runtime_gpu = source["gh_gpu_command_processor.cpp"]
    runtime_gpu_header = source["gh_gpu_command_processor.h"]
    ppc_func = (runtime / "include/rex/ppc/func.h").read_text()
    unified_context = "using PPCContext = GuestContext;" in ppc_func
    native_gpu_factory = (
        "#ifdef GH_PROJECT_RUNTIME" in app
        and "gh::graphics::CreateGraphicsSystem(config_.gpu_plugin" in app
        and "GH_PROJECT_RUNTIME=1" in cmake
    )

    def project_library(target: str, *sources: str) -> bool:
        declaration = re.search(
            r"add_library\(\s*" + re.escape(target) + r"\s+STATIC\b([^)]*)\)",
            cmake, re.DOTALL,
        )
        return bool(declaration) and all(
            path in declaration.group(1).split() and (project / path).is_file()
            for path in sources
        )

    def linked(target: str, dependency: str) -> bool:
        return any(
            dependency in body.split()
            for body in re.findall(
                r"target_link_libraries\(\s*" + re.escape(target) + r"\b([^)]*)\)",
                cmake, re.DOTALL,
            )
        )

    addresses = re.findall(r"RegisterFunction<GuestContext>\(static_cast<uint32_t>\(0x([0-9A-Fa-f]+)\),", mappings)
    import_addresses = re.findall(
        r"\{ 0x([0-9A-Fa-f]+), REX_GUEST_FUNCTION_POINTER\(__imp__", mappings
    )
    result = {
        "native_source_count": len(native),
        "native_rex_ppc_includes": native_text.count("<rex/ppc"),
        "native_guest_context_uses": native_text.count("GuestContext"),
        "registered_entries": len(addresses),
        "registered_unique_addresses": len(set(addresses)),
        "import_entries": len(import_addresses),
        "mapping_entries": len(re.findall(r"\{ 0x[0-9A-Fa-f]+,", mappings)),
        "generated_context": "GuestContext" if "REX_GUEST_CONTEXT_TYPE GuestContext" in pch else "PPCContext",
        "rexglue_target": "rexglue_setup_target" in cmake,
        "xenos_plugin": not native_gpu_factory and "LoadGpuPlugin(config_.gpu_plugin)" in app,
        "legacy_sdk_plugin_fallback_present": "LoadGpuPlugin(config_.gpu_plugin)" in app,
        "project_gpu_factory_selected": native_gpu_factory,
        "codegen_command": " codegen " in (project / "generated/rexglue.cmake").read_text(),
        "codegen_offline_tool_hook": (
            "REXGLUE_CODEGEN_EXECUTABLE" in cmake
            and "REXGLUE_CODEGEN_COMMAND" in rexglue_cmake
            and "$<TARGET_FILE:rex::rexglue>" in rexglue_cmake
        ),
    }
    result["native_is_runtime_free"] = result["native_rex_ppc_includes"] == 0
    result["registration_addresses_match"] = (
        result["registered_entries"] == result["registered_unique_addresses"]
    )
    result["all_imports_have_project_entries"] = set(import_addresses).issubset(set(addresses))
    result["unregistered_trap_updates_context"] = (
        "context.last_indirect_target = address" in source.get("guest_dispatch.h", "")
        and "throw InvalidFunctionTrap(address," in source.get("guest_dispatch.h", "")
    )
    result["project_dispatch_registered"] = "guest::RegisterFunction<GuestContext>" in mappings
    result["project_indirect_dispatch_hooked"] = "guest::DispatchReXIndirect" in pch
    result["project_indirect_runtime_fallback_bounded"] = (
        "ResolveIndirectFunction" in pch
        and "REX_CODE_SIZE + REX_THUNK_RESERVE_SIZE" in pch
    )
    result["project_generated_guest_memory"] = (
        '#include "../../src/native_memory.h"' in pch
        and "GuestMemory(base).ReadRam32" in pch
        and "GuestMemory::WithGlobalMmio(base).Read32" in pch
    )
    result["generated_pch_legacy_mmio_access"] = "MMIOHandler::global_handler" in pch
    result["project_mmio_runtime_adapter"] = (
        "src/gh_memory_rex_adapter.cpp" in cmake
        and "gh::memory::BindReXGuestMemory()" in app
    )
    result["native_only_build_option"] = "GUARDIAN_HEROES_NATIVE_ONLY" in cmake
    result.update({
        "guest_context_generated_abi": "REX_GUEST_CONTEXT_TYPE GuestContext" in pch,
        "guest_context_rex_entry_adapter": "ReXGuestEntry" in source.get("rex_guest_entry_adapter.h", ""),
        "guest_context_entry_roundtrip_copy": not unified_context and (
            "std::memcpy(&guest_context, &runtime_context" in source.get("rex_guest_entry_adapter.h", "")
            and "std::memcpy(&runtime_context, &guest_context" in source.get("rex_guest_entry_adapter.h", "")
        ),
        "native_sources_use_ppc_context": any("PPCContext" in path.read_text() for path in native),
        "ppc_context_references_confined_to_adapter_and_abi_check": all(
            "PPCContext" not in path.read_text()
            for name, path in ((path.name, path) for path in (project / "src").glob("*"))
            if path.is_file() and name not in {"rex_guest_entry_adapter.h", "guest_abi_check.cpp"}
        ),
        "ppc_context_adapter_only": (
            all("PPCContext" not in path.read_text() for path in native)
            and "ReXRuntimeContext" in source.get("rex_guest_entry_adapter.h", "")
            and "using ReXRuntimeContext = PPCContext" in pch
            and "PPCContext" in source.get("guest_abi_check.cpp", "")
        ),
        "nonvolatile_save_restore_api": (
            "SaveNonVolatiles" in source.get("guest_context.h", "")
            and "RestoreNonVolatiles" in source.get("guest_context.h", "")
            and "kNonVolatileSaveSize" in source.get("guest_context.h", "")
        ),
        "nonvolatile_save_layout_assertions": all(
            marker in (project / "tests/guest_context_layout_test.cpp").read_text()
            for marker in ("GuestContext::kNonVolatileSaveSize == 1616",
                           "context.SaveNonVolatiles(saved.data())",
                           "context.RestoreNonVolatiles(saved.data())")
        ),
        "guest_memory_project_impl": (
            (project / "src/native_memory.h").exists()
            and "GuestMemory(base).ReadRam32" in pch
            and "GuestMemory(base).WriteRam32" in pch
        ),
        "guest_memory_runtime_mmio_adapter": (
            (project / "src/gh_memory_rex_adapter.cpp").exists()
            and "SetGlobalMmioHandler" in source.get("gh_memory_rex_adapter.cpp", "")
        ),
        "project_dispatch_registry": "class FunctionRegistry" in source.get("guest_dispatch.h", ""),
        "project_dispatch_invalid_trap": "class InvalidFunctionTrap" in source.get("guest_dispatch.h", ""),
        "project_dispatch_registration_authoritative": "guest::RegisterFunction<GuestContext>" in mappings,
        "rex_mapping_compatibility_retained": "PPCFuncMapping PPCFuncMappings[]" in mappings,
        "services": {
            "critical_region": {
                "project_logic": (project / "src/gh_threading.cpp").exists(),
                "runtime_mutex_adapter": "BindRuntimeMutex" in app,
                "guest_rtl_adapter": all(
                    "critical_sections()." + entry in runtime_xrtl
                    for entry in ("Initialize", "Enter", "TryEnter", "Leave")
                ),
                "runtime_owned_address_table": (
                    "std::unique_ptr<gh::kernel::CriticalSections>" in runtime_kernel_header
                    and "std::make_unique<gh::kernel::CriticalSections>()" in runtime_kernel_state
                    and "CriticalSections::Impl" in source.get("gh_kernel.cpp", "")
                ),
                "guest_rtl_rex_wait_fallback": any(
                    entry in runtime_xrtl for entry in ("xeKeWaitForSingleObject", "xeKeSetEvent")
                ),
                "termination_cancels_waits": "critical_sections_->CancelWaits()" in runtime_kernel_state
                    and "CriticalSections::CancelWaits()" in source.get("gh_kernel.cpp", "")
                    and "XThread::CheckTitleTermination()" in runtime_xrtl,
                "title_restart_resets_address_table": "critical_sections_->Reset()" in runtime_kernel_state,
                "restore_rebuilds_guest_ownership":
                    "Reset(/*restore_guest_ownership=*/true)" in runtime_kernel_state
                    and "memory.Read32(address + 24)" in source.get("gh_kernel.cpp", ""),
            },
            "kernel_threads_events_timers": {
                "project_logic": all((project / path).is_file() for path in (
                    "src/gh_kernel.h", "src/gh_kernel.cpp",
                    "tests/gh_kernel_test.cpp", "tests/run_gh_kernel_test.py",
                )) and project_library("guardian_heroes_kernel", "src/gh_kernel.cpp",
                                      "src/gh_kernel_guest.cpp", "src/gh_kernel_objects.cpp"),
                "runtime_adapter": (
                    "#include \"gh_kernel.h\"" in runtime_xobject
                    and "gh::kernel::WaitMultiple" in runtime_xobject
                    and "gh::kernel::NotifyWaiters()" in runtime_xthread
                    and "GetKernelWaitable" in runtime_xevent
                    and "GetKernelWaitable" in runtime_xsemaphore
                    and "GetKernelWaitable" in runtime_xmutant
                    and "GetKernelWaitable" in runtime_xtimer
                    and all("GetKernelWaitable" in text for text in waitable_sources.values())
                    and "guardian_heroes_kernel" in cmake
                ),
                "mixed_wait_fallback": "rex::thread::WaitAny" in runtime_xobject,
                "project_waitable_types": [name for name, text in waitable_sources.items()
                                           if "GetKernelWaitable" in text],
                "legacy_wait_handle_dispatch": any(
                    marker in runtime_xobject for marker in (
                        "GetWaitHandle", "rex::thread::Wait(", "rex::thread::WaitAny",
                        "rex::thread::WaitAll", "rex::thread::SignalAndWait",
                    )
                ),
                "native_wait_mirrors": any(
                    re.search(r"rex::thread::(?:Event|Semaphore|Mutant|Timer)::Create", text)
                    is not None for name, text in waitable_sources.items() if name != "thread"
                ),
                "alertable_apc_wakeup": "NotifyWaiters" in runtime_xthread
                    and "HasDeliverableAPCs" in runtime_xobject,
                "title_termination_wakeup": "gh::kernel::NotifyWaiters()" in runtime_kernel_state
                    and "is_terminating_title()" in runtime_xobject,
                "event_pulse_dispatch": "Event::Pulse()" in source.get("gh_kernel.cpp", "")
                    and "kernel_event_->Pulse()" in runtime_xevent,
                "timer_retains_callback_thread": "target = routine ? retain_object" in runtime_xtimer
                    and "callback_thread_ = std::move(target)" in runtime_xtimer
                    and "target = std::move(callback_thread_)" in runtime_xtimer,
                "mutant_owner_token": "AbandonOwner(thread->handle())" in runtime_xmutant
                    and "gh::kernel::SetThreadLocalValue(handle())" in runtime_xthread,
                "thread_creation_runtime_owned": "rex::thread::Thread::Create" in runtime_xthread,
                "project_host_thread_adapter":
                    "gh::kernel::HostThread::Create" in runtime_xthread
                    and "gh::kernel::HostThread::Exit" in runtime_xthread
                    and "HostThread::Impl" in source.get("gh_kernel.cpp", ""),
                "project_host_callback_dispatch":
                    "gh::kernel::HostThread::DispatchUserCallbacks()" in runtime_delay
                    and "thread_->QueueUserCallback" in runtime_xthread,
                "project_guest_apc_queue": all(
                    "gh::kernel::GuestApcQueue::" + operation in runtime_threading
                    for operation in ("Initialize", "Insert", "Remove")
                ) and all(
                    "gh::kernel::GuestApcQueue::" + operation in runtime_xthread
                    for operation in ("Pending", "Deliverable", "Pop")
                ),
                "project_guest_tls": all(
                    "gh::kernel::GuestTls::" + operation in runtime_kernel_state
                    for operation in ("Initialize", "Allocate", "Free")
                ) and all(
                    "gh::kernel::GuestTls::" + operation in runtime_xthread
                    for operation in ("Read", "Write")
                ),
                "runtime_guest_thread_lifecycle":
                    "void KernelState::TerminateTitle()" in runtime_kernel_state
                    and "kernel_state_->OnThreadExit(this)" in runtime_xthread,
                "coordinated_thread_continuation_restore_wired": all(
                    marker in runtime_xthread for marker in (
                        "kThreadContinuationSignature", "validated.Restore(execution)",
                        "XThread::PrepareContinuationRestore(", "continuation::ResumeTitle(")),
                "uncoordinated_unfinished_thread_restore_rejected":
                    "Cannot restore unfinished XThread without continuation payload" in runtime_xthread
                    and "if (state.is_running || !completed)" in runtime_xthread,
                "thread_lifetime_uses_completion_event":
                    "bool XThread::is_alive() const" in runtime_xthread
                    and "thread_ && !kernel_thread_event_->signaled()" in runtime_xthread
                    and "thread->is_alive()" in runtime_kernel_state,
                "title_termination_resumes_suspended_threads":
                    "thread->WakeForTitleTermination()" in runtime_kernel_state
                    and "void XThread::WakeForTitleTermination()" in runtime_xthread
                    and "suspend_cv_.notify_all()" in runtime_xthread,
                "project_delay_wait": "gh::kernel::WaitMultiple" in runtime_delay,
                "delay_absolute_timeout": "GuestTimeoutToDuration" in runtime_delay
                    and "QueryGuestSystemTime" in runtime_delay,
                "delay_host_callbacks_runtime_owned": "rex::thread::AlertableSleep" in runtime_delay,
                "object_table_runtime_owned":
                    "util::ObjectTable object_table_" in runtime_kernel_header
                    and "gh::kernel::HandleTable" not in runtime_object_table,
                "object_table_runtime_adapter":
                    "util::ObjectTable object_table_" in runtime_kernel_header
                    and "std::make_unique<gh::kernel::HandleTable>()" in runtime_object_table,
                "project_handle_table": project_library(
                    "guardian_heroes_kernel", "src/gh_kernel_objects.cpp"
                ) and all("handles_->" + operation in runtime_object_table
                          for operation in ("Add", "Retain", "Release", "Lookup", "Reset",
                                            "Purge", "SaveReferences", "RestoreReferences")),
                "object_table_detaches_handles": "std::erase(object->handles(), handle)" in runtime_object_table
                    and "UnlinkHandle(" in runtime_object_table
                    and "name_table_.clear()" in runtime_object_table
                    and "std::erase_if(name_table_" in runtime_object_table,
                "mutant_waitable": (
                    "class Mutant : public Waitable" in (project / "src/gh_kernel.h").read_text()
                    and "TestMutant" in (project / "tests/gh_kernel_test.cpp").read_text()
                    and "kernel_mutant_" in runtime_xmutant
                ),
                "mutant_abandonment": (
                    "WaitResult::Abandoned" in (project / "src/gh_kernel.cpp").read_text()
                    and "TestMutantAbandonment" in (project / "tests/gh_kernel_test.cpp").read_text()
                    and "AbandonOwnedBy" in runtime_xmutant
                    and "AbandonMutantsOwnedBy" in runtime_xthread
                    and "X_STATUS_ABANDONED_WAIT_0" in runtime_xobject
                ),
                "thread_exit_waitable": (
                    "kernel_thread_event_" in runtime_xthread
                    and "GetKernelWaitable" in runtime_xthread
                ),
            },
            "clock": {
                "project_logic": (project / "src/gh_clock.cpp").exists(),
                "runtime_adapter": (project / "src/gh_clock_rex_adapter.cpp").exists(),
            },
            "exceptions_logging": {
                "project_logic": (project / "src/gh_exceptions.cpp").exists()
                    and (project / "src/gh_logging.cpp").exists(),
                "app_wired": "InstallTerminateHandler" in app and "logging::Initialize" in app,
            },
            "filesystem": {
                "project_logic": (project / "src/gh_filesystem.cpp").exists(),
                "runtime_supplies_roots": "Initialize(game_data_root_, user_data_root_" in app,
            },
            "input_assignment": {
                "project_logic": (project / "src/gh_controller_assignment.cpp").exists(),
                "runtime_adapter": (project / "src/gh_controller_assignment_adapter.cpp").exists(),
                "runtime_input_backend": "CreateDefaultInputSystem" in app,
            },
            "guest_arena": {
                "project_logic": project_library(
                    "guardian_heroes_guest_arena", "src/gh_guest_arena.cpp"
                ),
                "runtime_adapter": linked("rexruntime", "guardian_heroes_memory_service")
                    and "service_->Initialize(" in runtime_memory
                    and "std::make_unique<GuestArena>()" in source.get("gh_memory_service.cpp", "")
                    and "virtual_membase_ = arena_->virtual_base()" in source.get("gh_memory_service.cpp", ""),
                "project_layout_heaps_watches_and_snapshots": project_library(
                    "guardian_heroes_memory_service", "src/gh_memory_service.cpp"
                ) and all(token in source.get("gh_memory_service.cpp", "") for token in (
                    "MemoryService::Initialize(", "MemoryService::Save(", "MemoryService::Restore(",
                    "MemoryService::TriggerPhysicalMemoryCallbacks(")),
                "project_os_page_operations": linked("rexcore", "guardian_heroes_host_memory")
                    and "host::Protect(" in source.get("gh_memory_service.cpp", "")
                    and "mprotect(" in source.get("gh_host_memory_posix.cpp", "")
                    and "VirtualProtect(" in source.get("gh_host_memory_win.cpp", ""),
                "project_heap_page_tables_and_protection": project_library(
                    "guardian_heroes_heap", "src/gh_heap.cpp"
                ) and linked("rexruntime", "guardian_heroes_heap")
                    and "bool BaseHeap::AllocFixed(" in source.get("gh_heap.cpp", "")
                    and "bool BaseHeap::SyncHostPageAccess(" in source.get("gh_heap.cpp", ""),
                "project_mmio_ranges_and_fault_decode": project_library(
                    "guardian_heroes_mmio", "src/gh_mmio.cpp"
                ) and linked("rexruntime", "guardian_heroes_mmio")
                    and "bool MMIO::HandleFault(" in source.get("gh_mmio.cpp", "")
                    and "core_.HandleFault(fault)" in runtime_mmio,
                "runtime_heap_allocator": "bool BaseHeap::AllocFixed(" in runtime_memory,
                "runtime_exception_registration": "ExceptionHandler::Install(" in runtime_mmio,
                "runtime_page_protection": "rex::memory::Protect(" in runtime_memory,
                "runtime_mmio_handler": "runtime::MMIOHandler::Install(" in runtime_memory,
            },
            "image_payload": {
                "project_logic": project_library("guardian_heroes_image", "src/gh_image.cpp"),
                "runtime_adapter": linked("rexruntime", "guardian_heroes_image")
                    and "gh::image::DecodePayload(payload," in runtime_image,
                "project_session_key": "gh::image::SessionKey(" in runtime_image,
                "project_lzx_shared_with_patch": "gh::image::DecompressLzx(" in runtime_lzx
                    and "DecompressLzx(" in source.get("gh_image_loader.cpp", ""),
                "project_import_binding_and_delta_execution": project_library(
                    "guardian_heroes_image", "src/gh_image_loader.cpp"
                ) and "gh::image::BindImports(" in runtime_image
                    and "gh::image::ApplyDeltaPatch(" in runtime_image
                    and "int ApplyDeltaRecords(" in source.get("gh_image_loader.cpp", ""),
                "project_xex_pe_import_metadata": project_library(
                    "guardian_heroes_image_metadata", "src/gh_image_metadata.cpp"
                ) and linked("guardian_heroes_image", "guardian_heroes_image_metadata")
                    and "gh::image::FindOptionalHeader(" in runtime_image
                    and "gh::image::ParsePe(" in runtime_image
                    and "metadata_->ParseImports(" in runtime_image,
                "runtime_xex_metadata": "optional_header_count" in runtime_image,
                "runtime_pe_headers": "IMAGE_NT_HEADERS" in runtime_image,
                "runtime_metadata_adapter": "XexModule::GetOptHeader(" in runtime_image,
                "runtime_import_resolver_adapter": "bool XexModule::SetupLibraryImports(" in runtime_image,
                "runtime_guest_allocation": "heap->AllocFixed(" in runtime_image,
                "runtime_delta_patch_record_traversal": "int lzxdelta_apply_patch(" in runtime_lzx,
            },
            "audio_xma": {
                "project_logic": project_library("guardian_heroes_audio", "src/gh_audio.cpp")
                    and project_library("guardian_heroes_audio_sdl", "src/gh_audio_sdl.cpp")
                    and project_library("guardian_heroes_audio_xma", "src/gh_audio_xma.cpp"),
                "runtime_adapter": linked("rexaudio", "guardian_heroes_audio_sdl")
                    and linked("rexaudio", "guardian_heroes_audio_xma")
                    and "std::make_unique<gh::audio::SdlOutput>" in runtime_sdl
                    and "using XmaContext = gh::audio::XmaContext" in runtime_xma_header
                    and "gh::audio::XmaRuntime runtime_" in runtime_xma_decoder_header
                    and "runtime_.Initialize(" in runtime_xma_decoder,
                "project_sdl_device_and_callback": "SdlOutput::Open(" in source.get("gh_audio_sdl.cpp", "")
                    and "static void Callback(" in source.get("gh_audio_sdl.cpp", ""),
                "project_pcm_conversion":
                    "conversion::sequential_6_BE_to_interleaved_2_LE" in source.get("gh_audio_sdl.cpp", "")
                    and "ConvertFrame(channels," in source.get("gh_audio_xma.cpp", ""),
                "project_ffmpeg_frame_decoder": "avcodec_send_packet(" in source.get("gh_audio_xma.cpp", "")
                    and "avcodec_receive_frame(" in source.get("gh_audio_xma.cpp", ""),
                "project_guest_packet_loop_and_output_ring": project_library(
                    "guardian_heroes_audio_xma", "src/gh_audio_context.cpp", "src/gh_audio_runtime.cpp"
                ) and "XmaContext::GetNextPacket(" in source.get("gh_audio_context.cpp", "")
                    and "XmaContext::PrepareOutputRingBuffer(" in source.get("gh_audio_context.cpp", "")
                    and "loop_frame_output_limit_" in source.get("gh_audio_context.cpp", ""),
                "project_context_worker_and_client_credits":
                    "XmaRuntime::Run()" in source.get("gh_audio_runtime.cpp", "")
                    and "ClientScheduler::Run(" in source.get("gh_audio_runtime.cpp", "")
                    and "clients_.FrameConsumed(" in runtime_audio,
                "runtime_guest_packet_traversal_and_output_ring":
                    "GetNextPacket(" in runtime_xma and "PrepareOutputRingBuffer(" in runtime_xma
                    and "current_frame_remaining_subframes_" in runtime_xma,
                "runtime_client_and_credit_adapter": "semaphore->Release(1, nullptr)" in runtime_sdl,
                "runtime_audio_api_references": sum(
                    text.count("XmaDecoder") + text.count("AudioSystem")
                    for name, text in source.items()
                    if name.startswith("native_") or name.endswith(".h")
                ),
                "registered_audio_imports": len(re.findall(
                    r"\{ 0x[0-9A-Fa-f]+, REX_GUEST_FUNCTION_POINTER\(__imp__(?:XMA|XAudio)",
                    mappings,
                )),
            },
            "gpu_commands": {
                "project_logic": project_library(
                    "guardian_heroes_graphics", "src/gh_gpu_commands.cpp"
                ),
                "runtime_adapter": linked("rexgpu-xenos", "guardian_heroes_graphics")
                    and "gh::graphics::GuestCommandProcessor guest_commands_" in runtime_gpu_header
                    and "guest_commands_.Execute(packet, project_reader, target)" in runtime_gpu,
                "project_packet_decode_and_predication":
                    "GuestCommandProcessor::Execute(" in source.get("gh_gpu_commands.cpp", "")
                    and "bin_select_ & bin_mask_" in source.get("gh_gpu_commands.cpp", ""),
                "runtime_device_command_fallback":
                    "PacketResult::kDeviceCommand: return ExecutePacketType3(reader, packet)" in runtime_gpu,
                "project_draw_and_shader_packet_decode":
                    "GuestCommandProcessor::Draw(" in source.get("gh_gpu_commands.cpp", "")
                    and "target.SubmitDraw(command)" in source.get("gh_gpu_commands.cpp", "")
                    and "target.LoadShader(type, address, code, words)" in source.get("gh_gpu_commands.cpp", ""),
                "runtime_draw_shader_backend": "owner_.IssueDraw(" in runtime_gpu
                    and "owner_.LoadShader(" in runtime_gpu,
                "project_shader_ir_analysis_and_spirv_translation": project_library(
                    "guardian_heroes_gpu_shader", "src/gh_gpu_shader.cpp", "src/gh_gpu_shader_translator.cpp"
                ) and linked("rexgpu-xenos", "guardian_heroes_gpu_shader")
                    and "Shader::AnalyzeUcode(" in source.get("gh_gpu_shader_translator.cpp", "")
                    and "SpirvShaderTranslator::StartTranslation(" in source.get("gh_gpu_shader_spirv_translator.cpp", ""),
                "inherited_format_and_common_utilities_retained":
                    "<rex/graphics/format/ucode.h>" in source.get("gh_gpu_shader_translator.h", ""),
                "runtime_graphics_context_and_presenter": "WindowedApp" in app,
            },
            "window_presentation": {
                "project_logic": (runtime / "src/ui/presenter.cpp").is_file(),
                "project_app_lifecycle": "GuardianHeroesApp final : public rex::ui::WindowedApp" in app
                    and "GuardianHeroesApp::LaunchModule()" in app
                    and "GuardianHeroesApp::OnDestroy()" in app,
                "rex_app_base_retained": "public rex::ReXApp" in app,
                "runtime_host": "WindowedApp" in app,
            },
        },
        "xenos_project_backend_present": all(
            marker in source.get(name, "")
            for name, marker in (
                ("gh_gpu_command_processor.cpp", "CommandProcessor::"),
                ("gh_gpu_vulkan_command_processor.cpp", "VulkanCommandProcessor::"),
                ("gh_gpu_texture_cache.cpp", "TextureCache::"),
                ("gh_gpu_render_target_cache.cpp", "RenderTargetCache::"),
            )
        ) and "guardian_heroes_gpu_backend" in gpu_cmake,
        "rexglue_build_chain": {
            "generated_cmake_present": (project / "generated/rexglue.cmake").exists(),
            "setup_target_present": "rexglue_setup_target" in cmake,
            "codegen_runtime_command_present": " codegen " in rexglue_cmake,
            "codegen_offline_tool_hook": (
                "REXGLUE_CODEGEN_EXECUTABLE" in cmake
                and "REXGLUE_CODEGEN_COMMAND" in rexglue_cmake
            ),
            "runtime_library_referenced": "rex::runtime" in host_cmake,
            "xenos_plugin_argument_present": "GPU_PLUGINS xenos" in cmake,
            "project_host_build_assembly": "include(cmake/GuardianHeroesApp.cmake)" in cmake,
        },
    })
    # A complete project build is distinct from full title/runtime acceptance.
    result["full_runtime_replaced"] = False
    result["audit_scope"] = "Project source ownership and configured build graph; no game execution"
    result["runtime_provenance"] = {
        "kind": "Project-maintained fork of retained ReXGlue/Xenia runtime sources",
        "original_implementation_from_scratch": False,
        "inherited_rex_namespaces_and_abi_retained": True,
        "source_roots": ["src/gh_system_*", "src/gh_audio_*", "src/gh_gpu_*",
                         "runtime/src", "runtime/include", "runtime/thirdparty"],
        "legacy_sdk_build_available": "include(generated/rexglue.cmake)" in cmake,
        "native_build_uses_sdk_setup_target": "rexglue_setup_target(" in bootstrap,
    }
    result["native_only_builds_complete_app"] = (
        "add_subdirectory(runtime)" in bootstrap
        and "include(cmake/GuardianHeroesStandaloneBootstrap.cmake)" in cmake
        and "add_executable(guardian_heroes" in cmake
        and "add_library(rexruntime STATIC" in runtime_system_cmake
        and all("add_subdirectory(src/" + domain + ")" in runtime_cmake
                for domain in ("core", "filesystem", "ui", "input", "audio",
                               "system", "kernel", "graphics"))
    )
    result["native_only_libraries_only"] = not result["native_only_builds_complete_app"]
    result["native_only_default_enabled"] = re.search(
        r'option\(GUARDIAN_HEROES_NATIVE_ONLY\s+"[^"]*"\s+ON\s*\)', cmake
    ) is not None
    result["native_codegen_is_optional_offline_tool"] = (
        "if(GUARDIAN_HEROES_CODEGEN_EXECUTABLE)" in bootstrap
        and "include(${CMAKE_CURRENT_SOURCE_DIR}/generated/default/sources.cmake)" in bootstrap
    )
    result["guest_context_unified_runtime_abi"] = unified_context
    result["ppc_context_adapter_only"] = False
    title_header = source.get("gh_guest_continuation_title.h", "")
    title_source = source.get("gh_guest_continuation_title.cpp", "")
    title_codegen_path = project / "tools/gh_guest_continuation_title_codegen.py"
    title_codegen = title_codegen_path.read_text() if title_codegen_path.is_file() else ""
    dispatcher = source.get("gh_system_function_dispatcher.cpp", "")
    runtime_lifecycle = source.get("gh_system_runtime.cpp", "")
    configured_build = audit_compile_commands(project, build_directory)
    result["continuation"] = {
        "project_instruction_budget_machine_present": all(
            (project / path).is_file() for path in (
                "src/gh_guest_continuation.cpp", "tools/gh_guest_continuation_codegen.py",
                "tests/gh_guest_continuation_test.cpp")),
        "title_partition_codegen_present": all(marker in title_codegen for marker in (
            "guardian_heroes_recomp.*.cpp", "CreateTitleContinuationProgram()",
            "gh_title_continuation_sources.cmake", "gh_title_continuation_report.json")),
        "title_codegen_classifies_atomic_and_unsupported_entries": all(
            marker in title_codegen for marker in (
                "program.AddAtomicHost(", "program.AddUnsupported(", "FORBIDDEN_CONTROL")),
        "title_xthread_entry_bridge": "continuation::TryExecuteTitle(" in runtime_xthread,
        "title_function_dispatcher_bridge": "continuation::TryExecuteTitle(" in dispatcher,
        "title_xthread_dispatch_integrated": all(
            "continuation::TryExecuteTitle(" in text
            for text in (runtime_xthread, dispatcher)),
        "title_program_default_is_null": re.search(
            r"title_program\s*\{\s*nullptr\s*\}", title_source) is not None,
        "app_or_runtime_installs_title_program": any(
            re.search(r"\bInstallTitleProgram\s*\(", text) is not None
            for text in (app, runtime_lifecycle)),
        "optional_continuation_app_installs_title_program":
            "InstallTitleProgram(" in source.get("main_continuation.cpp", ""),
        "generated_title_partitions_in_configured_build": (
            configured_build["title_continuation_translation_units"] > 0
            if configured_build["present"] else None),
        "nested_atomic_callbacks_keep_original_host_stack":
            "!program || CurrentTitleExecution()" in title_header,
        "cooperative_coordinator_declared": "class TitleCoordinator" in title_header,
        "cooperative_coordinator_implemented": all(marker in title_source for marker in (
            "TitleCoordinator::Pause(", "TitleCoordinator::Cancel(",
            "TitleCoordinator::Stop::Restore(")),
        "cooperative_thread_images_include_execution_and_incarnation": all(
            marker in title_header for marker in (
                "struct ThreadImage", "uint64_t incarnation", "std::vector<uint8_t> execution")),
        "cooperative_xthread_registration_wired": all(
            marker in runtime_xthread for marker in (
                "TitleCoordinator::Registration", "continuation_thread.emplace(",
                "kernel_state_->title_continuations()", "&XThread::CanPauseTitleContinuation")),
        "cooperative_instruction_budget_poll_wired": all(
            marker in title_source for marker in (
                "machine.Run(base, 4096)",
                "coordinator->Poll(current_thread_id, current_can_pause)")),
        "cooperative_unsafe_guest_states_excluded": all(
            marker in runtime_xthread for marker in (
                "thread->apc_disable_count", "thread->executing_kernel_apc",
                "thread->fiber_ptr", "pcr->current_irql", "pcr->prcb_data.dpc_active",
                "GlobalCriticalRegion::depth() == 0", "critical_sections().IsOwnedBy(")),
        "cooperative_kernel_pause_includes_unfinished_threads": all(
            marker in runtime_kernel_state for marker in (
                "KernelState::PauseGuestContinuations(",
                "thread->is_guest_thread() && thread->is_alive()",
                "title_continuations_.Pause(unfinished, timeout)")),
        "cooperative_title_termination_cancels_stop":
            "title_continuations_.Cancel()" in runtime_kernel_state,
        "cooperative_restore_same_live_threads_only": all(
            marker in title_source for marker in (
                "TitleCoordinator::RestoreStopped(",
                "state->generation != generation", "!state->Ready({})",
                "threads.size() != state->entries.size()",
                "it->second.incarnation != image.incarnation")),
        "cooperative_capture_is_full_kernel_memory_device_snapshot": False,
        "runtime_aggregate_snapshot_wired": all(marker in runtime_lifecycle for marker in (
            "kRuntimeSnapshotMagic = 0x31535247", "Runtime::Save(", "Runtime::Restore(",
            "PauseTimersForSnapshot()", "PauseGuestContinuations(timeout)",
            "PauseGuestClock()", "PrepareRestoredContinuations()", "snapshot_restore_failed_")),
        "deferred_dispatch_snapshot_gate_wired": all(marker in runtime_kernel_state for marker in (
            "KernelState::PauseDispatchForSnapshot(", "KernelState::ResumeDispatchAfterSnapshot()",
            "dispatch_in_flight_", "dispatch_paused_for_snapshot_"))
            and runtime_lifecycle.count("PauseDispatchForSnapshot(") >= 4,
        "queued_blocking_imports_wired": all(marker in source["gh_guest_continuation_waits.cpp"]
            for marker in ("BeginKeDelay", "BeginKeWaitSingle", "BeginNtWaitSingle",
                           "BeginKeWaitMultiple", "BeginNtWaitMultiple", "BeginSignalAndWait"))
            and "program.AddQueuedHost(" in title_codegen
            and "QueuedWait::QueuedWait(" in source["gh_kernel.cpp"],
        "queued_import_snapshot_retains_order_outcomes_and_deadlines": all(
            marker in source["gh_guest_continuation_waits.cpp"] for marker in (
                "state.order", "state.completed", "deadline_", "AddHandle(object.get()",
                "QueryGuestTickCount() >= deadline_"))
            and "PreparePendingHost(memory()->virtual_membase(), true)" in runtime_xthread,
        "in_place_frame_restore_rejects_queued_imports":
            "queued imports require a full kernel and memory restore" in title_source,
        "created_suspended_thread_snapshot_wired": all(marker in runtime_xthread for marker in (
            "kThreadCreatedSuspendedSignature", "is_created_suspended()",
            "CreateNativeHostThread(")),
        "timer_snapshot_wired": all(marker in runtime_xtimer for marker in (
            "XTimer::Save(", "XTimer::Restore(",
            "UpdatePausedDeadline", "callback_thread_")),
        "quiescent_file_continuation_snapshot_source_wiring": all(
            marker in runtime_xfile for marker in (
                "XFile::SaveContinuation(", "XFile::RestoreContinuation(",
                "0x46494C31", "async_event_->signaled()", "find_pattern_",
                "stream->Write<uint64_t>(find_index_)",
                "std::scoped_lock lock(file_lock_, completion_port_lock_)",
                "!completion_ports_.empty()", "file_->entry()->delete_on_close()"))
            and all(marker in runtime_kernel_state for marker in (
                "static_cast<XFile*>(object.get())->SaveContinuation(stream)",
                "XFile::RestoreContinuation(this, stream)")),
        "file_continuation_mounted_root_reopen_source_wiring": all(
            marker in runtime_xfile for marker in (
                "file_system()->ResolvePath(abs_path)",
                "entry->Open(access, &vfs_file)",
                "kFileAttributeDirectory) != 0) ==",
                "is_directory")),
        "in_flight_import_and_seh_restore_supported": False,
        "running_or_suspended_title_restore_supported": False,
        "uncoordinated_unfinished_thread_save_and_restore_rejected": (
            "Cannot save unfinished XThread" in runtime_xthread
            and "without a coordinated continuation stop" in runtime_xthread
            and "Cannot restore unfinished XThread without continuation payload" in runtime_xthread),
        "coordinated_kernel_continuation_save_wired": all(
            marker in runtime_kernel_state for marker in (
                "stop->IsActiveFor(title_continuations_)", "kKernelContinuationSignature",
                "thread->SaveContinuation(stream, *image)", "can_save_continuation()",
                "Kernel object type {} has no resumable serializer")),
        "kernel_continuation_rebuilds_new_native_threads": all(
            marker in runtime_xthread for marker in (
                "new XThread(nullptr)", "XThread::PrepareContinuationRestore(",
                "gh::kernel::HostThread::Create(params", "params.create_suspended = true",
                "continuation::ResumeTitle(")),
        "kernel_continuation_delayed_start_and_failure_gate": all(
            marker in runtime_kernel_state for marker in (
                "KernelState::CancelRestoredContinuations(",
                "KernelState::ResumeRestoredContinuations(",
                "return !resume_continuations || ResumeRestoredContinuations()",
                "gate->start = true", "thread->thread()->Join()")),
        "kernel_continuation_reuses_loaded_modules":
            "XModule::RestoreContinuation(" in source["gh_system_xmodule.cpp"]
            and "kernel_state->GetModule(path, true)" in source["gh_system_xmodule.cpp"],
        "kernel_continuation_primary_and_auxiliary_module_reuse_source_wiring": all(
            marker in source["gh_system_xmodule.cpp"] for marker in (
                "kernel_state->GetExecutableModule()",
                "rex::string::utf8_equal_case(module->path(), path)",
                "kernel_state->GetModule(path, true)",
                "module->hmodule_ptr_ != hmodule", "module->RestoreObject(stream)")),
        "retired_guest_storage_ownership_is_abandoned_before_kernel_restore":
            "object->AbandonGuestStorageForRestore()" in runtime_kernel_state
            and "XThread::AbandonGuestStorageForRestore()" in runtime_xthread,
        "mutant_continuation_retains_recursion_and_abandonment": all(
            marker in runtime_xmutant for marker in (
                "XMutant::SaveContinuation(", "state.recursion", "state.abandoned",
                "AcquireMutantOnStartup(mutant, recursion)")),
    }
    services = result["services"]
    services["kernel_threads_events_timers"]["project_runtime_and_guest_lifecycle"] = (
        "gh_system_" in system_cmake
        and "void KernelState::TerminateTitle()" in runtime_kernel_state)
    services["audio_xma"].update({
        "project_guest_callback_worker_and_allocation": all(
            marker in runtime_audio for marker in (
                "new rex::system::XHostThread", "function_dispatcher_->Execute(",
                "memory_->SystemHeapAlloc(4)", "memory_->SystemHeapFree(pointer)")),
        "project_client_registration_and_snapshots": all(
            marker in runtime_audio for marker in (
                "AudioSystem::RegisterClient(", "AudioSystem::UnregisterClient(",
                "AudioSystem::Save(", "AudioSystem::Restore(")),
        "project_device_factory": "gh::audio::CreateSdlAudioSystem" in app
            and "sdl::SDLAudioSystem::Create(dispatcher)" in source["gh_audio_factory.cpp"],
        "service_objects_linked_to_project_runtime":
            "target_link_libraries(rexruntime PRIVATE guardian_heroes_audio_service)" in audio_cmake,
        "bounded_xma_frame_decoder_snapshot": all(
            marker in source["gh_audio_xma.cpp"] for marker in (
                "XmaFrameDecoder::SaveState()", "XmaFrameDecoder::RestoreState(",
                "ff_xmaframes_save_state", "ff_xmaframes_restore_state"))
            and (runtime / "thirdparty/FFmpeg/libavcodec/xmaframes_state.h").is_file(),
        "xma_context_and_register_snapshot": all(
            marker in source["gh_audio_runtime.cpp"] for marker in (
                "XmaRuntime::SaveState(", "XmaRuntime::RestoreState(",
                "!worker_parked_", "saved.registers = registers_", "saved.allocated = allocated_"))
            and "XmaContext::SaveState()" in source["gh_audio_context.cpp"]
            and "XmaDecoder::RestoreState(" in source["gh_audio_decoder.cpp"],
        "legacy_ffmpeg_without_state_extension_preserves_decode":
            "__has_include(<libavcodec/xmaframes_state.h>)" in source["gh_audio_xma.cpp"],
        "complete_audio_device_queue_and_credit_snapshot": all(
            marker in runtime_audio for marker in (
                "kAudioCompleteSaveSignature", "clients_.SaveState(clients)",
                "clients_.RestoreState(clients)", "AudioSystem::PrepareRestore()"))
            and all(marker in source["gh_audio_sdl.cpp"] for marker in (
                "GH_SDL_SaveAudioStreamState", "GH_SDL_RestoreAudioStreamState",
                "SdlOutput::SaveState(", "SdlOutput::RestoreState(")),
        "already_delivered_hardware_samples_rewindable": False,
    })
    services["gpu_commands"]["project_backend_and_factory"] = (
        result["xenos_project_backend_present"]
        and "gh_gpu_factory.cpp" in runtime_gpu_cmake
        and result["project_gpu_factory_selected"])
    acceptance_path = project.parent / "docs/validation/native-runtime-acceptance-2026-10-17.json"
    acceptance = json.loads(acceptance_path.read_text()) if acceptance_path.is_file() else {}
    title_roundtrip_recorded = acceptance.get("full_device_and_thread_snapshot_accepted") is True
    result["runtime_acceptance_record"] = str(acceptance_path.relative_to(project.parent))
    services["gpu_commands"]["snapshot"] = {
        "command_processor_state_wired": "CommandProcessor::Save(" in runtime_gpu
            and "CommandProcessor::Restore(" in runtime_gpu,
        "vulkan_backend_state_wired": all(marker in source["gh_gpu_vulkan_command_processor.cpp"]
            for marker in ("VulkanCommandProcessor::Save(", "VulkanCommandProcessor::Restore(")),
        "shared_memory_and_render_targets_wired": all(
            marker in source[name] for name, marker in (
                ("gh_gpu_shared_memory.cpp", "SharedMemory::GpuWrittenRanges()"),
                ("gh_gpu_vulkan_render_target_cache.cpp", "VulkanRenderTargetCache::SaveState("),
                ("gh_gpu_vulkan_render_target_cache.cpp", "VulkanRenderTargetCache::RestoreState("))),
        "d3d12_snapshot_supported": False,
        "presented_swapchain_image_serialized": False,
        "title_snapshot_restore_acceptance_verified": title_roundtrip_recorded,
    }
    services["window_presentation"]["project_runtime_ui_and_input_sources"] = all(
        (runtime / path).is_file() for path in (
            "src/ui/window_sdl.cpp", "src/ui/windowed_app.cpp", "src/input/input_system.cpp"))
    result["configured_build"] = configured_build
    result["remaining_boundaries"] = [
        "The optional title continuation bridge requires an installed program; the default title path still uses compiled host call stacks",
        "Runtime GRS1 coordinates kernel queues, memory, clocks and devices; title roundtrip acceptance is reported separately from source wiring",
        "In-flight atomic imports, nested guest callbacks and SEH continuation restore remain unsupported",
        "Inherited rex namespaces, PPC function mappings, kernel imports and platform UI/device APIs remain in the project fork",
        "SDL, FFmpeg, Vulkan/MoltenVK and other vendored platform dependencies remain required",
        "Complete game-save reload, stable audio, visual/input parity and target-hardware acceptance remain unverified",
    ]
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
