# Assemble the actual GuardianHeroesApp and checked-in guest recompilation for
# Switch. This intentionally requires the platform/runtime targets to exist;
# it never turns a bootstrap or graphics probe into a nominal game NRO.
include_guard(GLOBAL)

function(guardian_heroes_switch_add_game)
    if(NOT REXGLUE_PLATFORM_SWITCH)
        message(FATAL_ERROR "Guardian Heroes Switch game target requires the devkitA64 toolchain")
    endif()
    if(NOT COMMAND gh_switch_add_nro)
        message(FATAL_ERROR "Include GuardianHeroesSwitch.cmake before GuardianHeroesSwitchGame.cmake")
    endif()

    # These targets are the current app's concrete runtime, Xenos/Vulkan,
    # memory, XMA decode, and Horizon platform dependencies. They must be built
    # from Guardian Heroes sources/adapters, not supplied by a generic probe.
    set(_gh_required_targets
        guardian_heroes_native
        guardian_heroes_filesystem
        guardian_heroes_clock
        guardian_heroes_threading
        guardian_heroes_kernel
        guardian_heroes_memory_service
        guardian_heroes_heap
        guardian_heroes_mmio
        guardian_heroes_xenos_math
        guardian_heroes_xma_packet
        guardian_heroes_audio
        guardian_heroes_audio_xma
        guardian_heroes_image
        guardian_heroes_gpu_shader
        guardian_heroes_gpu_backend
        guardian_heroes_switch_guest_arena
        guardian_heroes_switch_host_memory
        rexruntime
        rexui
        rexinput
        rexaudio
        rexgpu-xenos
    )
    set(_gh_missing_targets)
    foreach(_gh_target IN LISTS _gh_required_targets)
        if(NOT TARGET "${_gh_target}")
            list(APPEND _gh_missing_targets "${_gh_target}")
        endif()
    endforeach()
    if(_gh_missing_targets)
        string(JOIN ", " _gh_missing_text ${_gh_missing_targets})
        message(FATAL_ERROR
            "Cannot assemble the Guardian Heroes game NRO; missing real Switch runtime targets: "
            "${_gh_missing_text}. The full game target is withheld until these are provided.")
    endif()
    if(NOT REXGLUE_SWITCH_NVK_SDK)
        message(FATAL_ERROR
            "Guardian Heroes game NRO requires the Horizon Mesa/NVK SDK; "
            "REXGLUE_SWITCH_NVK_SDK is empty.")
    endif()
    if(NOT REXGLUE_SWITCH_FFMPEG)
        message(FATAL_ERROR
            "Guardian Heroes game NRO requires Switch-target FFmpeg archives; "
            "REXGLUE_SWITCH_FFMPEG is empty.")
    endif()
    foreach(_gh_ffmpeg_file include/libavcodec/avcodec.h include/libavutil/avutil.h
            lib/libavcodec.a lib/libavutil.a)
        if(NOT EXISTS "${REXGLUE_SWITCH_FFMPEG}/${_gh_ffmpeg_file}")
            message(FATAL_ERROR
                "Switch FFmpeg SDK is incomplete: missing "
                "${REXGLUE_SWITCH_FFMPEG}/${_gh_ffmpeg_file}")
        endif()
    endforeach()

    set(_gh_nvk_libraries)
    foreach(_gh_nvk_library vulkan expat zstd z)
        find_library(GH_SWITCH_GAME_${_gh_nvk_library}_LIBRARY NAMES ${_gh_nvk_library}
            PATHS "${REXGLUE_SWITCH_NVK_SDK}/lib" "${DEVKITPRO}/portlibs/switch/lib"
            NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH NO_CACHE REQUIRED)
        list(APPEND _gh_nvk_libraries "${GH_SWITCH_GAME_${_gh_nvk_library}_LIBRARY}")
    endforeach()
    string(JOIN "," _gh_nvk_link_group ${_gh_nvk_libraries})
    find_path(GH_SWITCH_GAME_VULKAN_INCLUDE_DIR vulkan/vulkan.h
        PATHS "${REXGLUE_SWITCH_NVK_SDK}/include" "${DEVKITPRO}/portlibs/switch/include"
        NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH NO_CACHE REQUIRED)

    set(_gh_generated_cmake "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../generated/default/sources.cmake")
    if(NOT EXISTS "${_gh_generated_cmake}")
        message(FATAL_ERROR "Checked-in Guardian Heroes guest sources are missing: ${_gh_generated_cmake}")
    endif()
    include("${_gh_generated_cmake}")
    if(NOT GENERATED_SOURCES)
        message(FATAL_ERROR "generated/default/sources.cmake contains no Guardian Heroes guest code")
    endif()
    foreach(_gh_source IN LISTS GENERATED_SOURCES)
        if(NOT EXISTS "${_gh_source}")
            message(FATAL_ERROR "Checked-in Guardian Heroes guest source is missing: ${_gh_source}")
        endif()
    endforeach()

    set(_gh_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/..")
    add_library(guardian_heroes_switch_recomp OBJECT ${GENERATED_SOURCES})
    target_compile_features(guardian_heroes_switch_recomp PRIVATE cxx_std_23)
    target_compile_definitions(guardian_heroes_switch_recomp PRIVATE
        GH_PROJECT_RUNTIME=1 SPDLOG_FMT_EXTERNAL=1 SPDLOG_NO_TZ_OFFSET=1)
    target_include_directories(guardian_heroes_switch_recomp PRIVATE
        "${_gh_root}"
        "${_gh_root}/src"
        "${_gh_root}/generated/default"
        "${_gh_root}/runtime/include"
        "${_gh_root}/runtime/thirdparty/simde"
        "${_gh_root}/runtime/thirdparty/fmt/include"
        "${_gh_root}/runtime/thirdparty/spdlog/include")
    target_compile_options(guardian_heroes_switch_recomp PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-fno-fast-math;-frounding-math;-ffp-contract=off;-fno-strict-aliasing;-fno-char8_t>
        $<$<CXX_COMPILER_ID:Clang>:-ffp-model=strict;-fno-strict-aliasing;-fno-char8_t>)
    target_link_libraries(guardian_heroes_switch_recomp PRIVATE rexruntime)
    if(TARGET guardian_heroes_codegen)
        add_dependencies(guardian_heroes_switch_recomp guardian_heroes_codegen)
    endif()

    set(GH_SWITCH_APP_NAME "Guardian Heroes")
    set(GH_SWITCH_APP_AUTHOR "Guardian Heroes contributors")
    set(GH_SWITCH_APP_VERSION "0.1.0")
    gh_switch_add_nro(guardian_heroes_switch_game
        ROMFS_DIR "${_gh_root}/assets"
        SOURCES
            "${_gh_root}/switch/guardian_heroes_switch_main.cpp"
            $<TARGET_OBJECTS:rexcore>
            $<TARGET_OBJECTS:rexfilesystem>
            "${_gh_root}/src/main.cpp"
            "${_gh_root}/src/guardian_heroes_app.cpp"
            "${_gh_root}/src/localization.cpp"
            "${_gh_root}/src/localization_options.cpp"
            "${_gh_root}/src/cheats.cpp"
            "${_gh_root}/src/cheats_damage.cpp"
            "${_gh_root}/src/gpu_wait.cpp"
            "${_gh_root}/src/guest_abi_check.cpp"
            "${_gh_root}/src/gh_clock_rex_adapter.cpp"
            "${_gh_root}/src/gh_logging.cpp"
            "${_gh_root}/src/gh_exceptions.cpp"
            "${_gh_root}/src/gh_filesystem_imports.cpp"
            "${_gh_root}/src/gh_filesystem_imports_rex_adapter.cpp"
            "${_gh_root}/src/gh_controller_assignment.cpp"
            "${_gh_root}/src/gh_controller_assignment_adapter.cpp"
            "${_gh_root}/src/gh_memory_rex_adapter.cpp"
            "${_gh_root}/src/gh_switch_input_driver.cpp"
            "${_gh_root}/src/gh_switch_audio.cpp"
            "${_gh_root}/switch/nvk_elf_stubs.c"
            "${_gh_root}/switch/switch_support.cpp"
        LIBRARIES
            guardian_heroes_switch_recomp
            guardian_heroes_native
            guardian_heroes_filesystem
            guardian_heroes_clock
            guardian_heroes_threading
            guardian_heroes_kernel
            guardian_heroes_memory_service
            guardian_heroes_heap
            guardian_heroes_mmio
            guardian_heroes_xenos_math
            guardian_heroes_xma_packet
            guardian_heroes_audio
            guardian_heroes_audio_xma
            guardian_heroes_image
            guardian_heroes_gpu_shader
            guardian_heroes_gpu_backend
            guardian_heroes_switch_guest_arena
            guardian_heroes_switch_host_memory
            rexruntime
            rexui
            rexinput
            rexgpu-xenos
            rexcore
            rexfilesystem
            rexruntime
            rexcore
            rexfilesystem
            guardian_heroes_audio_xma
            guardian_heroes_threading
            guardian_heroes_kernel
            guardian_heroes_clock
            guardian_heroes_memory_service
            guardian_heroes_heap
            guardian_heroes_mmio
            guardian_heroes_xma_packet
            guardian_heroes_switch_host_memory
            rexruntime
            "$<LINK_GROUP:RESCAN,${_gh_nvk_link_group}>"
            pthread m)
    target_link_options(guardian_heroes_switch_game PRIVATE
        "LINKER:--whole-archive,${DEVKITPRO}/libnx/lib/libnx.a,--no-whole-archive")

    target_compile_features(guardian_heroes_switch_game PRIVATE cxx_std_23)
    target_compile_definitions(guardian_heroes_switch_game PRIVATE
        GH_PROJECT_RUNTIME=1 SPDLOG_FMT_EXTERNAL=1 SPDLOG_NO_TZ_OFFSET=1
        VK_USE_PLATFORM_VI_NN)
    target_include_directories(guardian_heroes_switch_game PRIVATE
        "${_gh_root}"
        "${_gh_root}/src"
        "${_gh_root}/generated/default"
        "${_gh_root}/runtime/include"
        "${_gh_root}/runtime/thirdparty/simde"
        "${_gh_root}/runtime/thirdparty/fmt/include"
        "${_gh_root}/runtime/thirdparty/spdlog/include")
    target_include_directories(guardian_heroes_switch_game SYSTEM PRIVATE
        "${GH_SWITCH_GAME_VULKAN_INCLUDE_DIR}")
    target_compile_options(guardian_heroes_switch_game PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-fno-fast-math;-frounding-math;-ffp-contract=off;-fno-strict-aliasing;-fno-char8_t>
        $<$<CXX_COMPILER_ID:Clang>:-ffp-model=strict;-fno-strict-aliasing;-fno-char8_t>)

    # The runtime and device plugin contain registration/override translation
    # units that must survive --gc-sections in the final NRO.
    set(_gh_force_link_targets rexruntime rexgpu-xenos)
    foreach(_gh_target IN LISTS _gh_force_link_targets)
        get_target_property(_gh_target_type "${_gh_target}" TYPE)
        if(_gh_target_type STREQUAL "OBJECT_LIBRARY")
            target_link_libraries(guardian_heroes_switch_game PRIVATE "${_gh_target}")
        elseif(_gh_target_type STREQUAL "STATIC_LIBRARY")
            list(APPEND _gh_force_link_files "$<TARGET_FILE:${_gh_target}>")
            set_property(TARGET guardian_heroes_switch_game APPEND PROPERTY
                LINK_DEPENDS "$<TARGET_FILE:${_gh_target}>")
        else()
            message(FATAL_ERROR
                "${_gh_target} must be OBJECT or STATIC for a static Switch title (got ${_gh_target_type})")
        endif()
    endforeach()
    if(_gh_force_link_files)
        string(JOIN "," _gh_force_link_csv ${_gh_force_link_files})
        target_link_options(guardian_heroes_switch_game PRIVATE
            "LINKER:--whole-archive,${_gh_force_link_csv},--no-whole-archive")
    endif()
    target_link_options(guardian_heroes_switch_game PRIVATE -pthread)
endfunction()
