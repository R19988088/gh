# Guardian Heroes recompiled title and project-owned host runtime.

cmake_minimum_required(VERSION 3.25)
project(guardian_heroes LANGUAGES C CXX)

if(REXGLUE_PLATFORM_SWITCH)
    add_compile_definitions(VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL=0 SPDLOG_NO_TZ_OFFSET=1)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GUARDIAN_HEROES_NATIVE_ONLY
       "Build the complete app with the project-owned host runtime" ON)

set(REXGLUE_CODEGEN_EXECUTABLE "" CACHE FILEPATH
    "Optional prebuilt ReXGlue codegen executable used as an offline tool")

set(GUARDIAN_HEROES_CODEGEN_EXECUTABLE "" CACHE FILEPATH
    "Optional prebuilt ReXGlue codegen executable used as an offline tool")

# Keep the project-facing option independent of the SDK's generic variable.
# The generated CMake fragment consumes REXGLUE_CODEGEN_EXECUTABLE.
if(GUARDIAN_HEROES_CODEGEN_EXECUTABLE)
    set(REXGLUE_CODEGEN_EXECUTABLE "${GUARDIAN_HEROES_CODEGEN_EXECUTABLE}")
endif()

if(GUARDIAN_HEROES_NATIVE_ONLY)
    add_compile_definitions(GH_PROJECT_RUNTIME=1)
    if(REXGLUE_PLATFORM_SWITCH)
        include(cmake/GuardianHeroesSwitch.cmake)
    else()
        include(cmake/GuardianHeroesStandaloneBootstrap.cmake)
    endif()
else()
    include(generated/rexglue.cmake)
endif()

# Sources
set(GUARDIANHEROES_NATIVE_SOURCES
    src/native_logic.cpp
    src/native_state.cpp
    src/native_bits.cpp
    src/native_arrays.cpp
    src/native_bytes.cpp
    src/native_queries.cpp
    src/native_links.cpp
    src/native_flags.cpp
    src/native_records.cpp
    src/native_constants.cpp
    src/native_accessors.cpp
    src/native_writers.cpp
    src/native_predicates.cpp
    src/native_addresses_ext.cpp
    src/native_writers_ext.cpp
    src/native_queries_ext.cpp
    src/native_float_ext.cpp
)

add_library(guardian_heroes_native STATIC ${GUARDIANHEROES_NATIVE_SOURCES})
target_include_directories(guardian_heroes_native PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(guardian_heroes_native PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-ffp-model=strict>
)

add_library(guardian_heroes_filesystem STATIC src/gh_filesystem.cpp)
target_include_directories(guardian_heroes_filesystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(guardian_heroes_clock STATIC src/gh_clock.cpp)
target_include_directories(guardian_heroes_clock PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # MSVC's CRT does not provide the helpers emitted for unsigned __int128.
    execute_process(
        COMMAND "${CMAKE_CXX_COMPILER}" --rtlib=compiler-rt --print-libgcc-file-name
        OUTPUT_VARIABLE _gh_compiler_builtins
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    target_link_libraries(guardian_heroes_clock PUBLIC "${_gh_compiler_builtins}")
endif()

add_library(guardian_heroes_threading STATIC src/gh_threading.cpp)
target_include_directories(guardian_heroes_threading PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(NOT REXGLUE_PLATFORM_SWITCH)
    add_library(guardian_heroes_guest_arena STATIC src/gh_guest_arena.cpp)
    target_include_directories(guardian_heroes_guest_arena PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
    set_target_properties(guardian_heroes_guest_arena PROPERTIES POSITION_INDEPENDENT_CODE ON)
    if(UNIX AND NOT APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
        target_link_libraries(guardian_heroes_guest_arena PRIVATE ${CMAKE_DL_LIBS})
        if(NOT ANDROID)
            target_link_libraries(guardian_heroes_guest_arena PRIVATE rt)
        endif()
    endif()
endif()

add_library(guardian_heroes_heap STATIC src/gh_heap.cpp)
target_include_directories(guardian_heroes_heap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_target_properties(guardian_heroes_heap PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(guardian_heroes_mmio STATIC src/gh_mmio.cpp)
target_include_directories(guardian_heroes_mmio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_target_properties(guardian_heroes_mmio PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(REXGLUE_PLATFORM_SWITCH)
    set(GUARDIAN_HEROES_HOST_MEMORY_SOURCE src/gh_host_memory_switch.cpp)
elseif(WIN32)
    set(GUARDIAN_HEROES_HOST_MEMORY_SOURCE src/gh_host_memory_win.cpp)
else()
    set(GUARDIAN_HEROES_HOST_MEMORY_SOURCE src/gh_host_memory_posix.cpp)
endif()
if(NOT REXGLUE_PLATFORM_SWITCH)
    add_library(guardian_heroes_host_memory STATIC ${GUARDIAN_HEROES_HOST_MEMORY_SOURCE})
    target_include_directories(guardian_heroes_host_memory PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
    set_target_properties(guardian_heroes_host_memory PROPERTIES POSITION_INDEPENDENT_CODE ON)
    if(UNIX AND NOT APPLE)
        target_link_libraries(guardian_heroes_host_memory PRIVATE ${CMAKE_DL_LIBS})
        if(NOT ANDROID AND NOT CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
            target_link_libraries(guardian_heroes_host_memory PRIVATE rt)
        endif()
    endif()
endif()
add_library(guardian_heroes_memory_service STATIC src/gh_memory_service.cpp)
target_include_directories(guardian_heroes_memory_service PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_target_properties(guardian_heroes_memory_service PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(guardian_heroes_memory_service PUBLIC
    guardian_heroes_heap guardian_heroes_guest_arena guardian_heroes_host_memory)

add_library(guardian_heroes_image_metadata STATIC src/gh_image_metadata.cpp)
target_include_directories(guardian_heroes_image_metadata PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_target_properties(guardian_heroes_image_metadata PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(guardian_heroes_kernel STATIC
    src/gh_kernel.cpp src/gh_kernel_guest.cpp src/gh_kernel_objects.cpp)
target_include_directories(guardian_heroes_kernel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
find_package(Threads REQUIRED)
target_link_libraries(guardian_heroes_kernel PUBLIC Threads::Threads)
target_link_libraries(guardian_heroes_memory_service PUBLIC Threads::Threads)

add_library(guardian_heroes_xenos_math STATIC src/gh_xenos_math.cpp)
target_include_directories(guardian_heroes_xenos_math PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(guardian_heroes_xma_packet STATIC src/gh_xma_packet.cpp)
target_include_directories(guardian_heroes_xma_packet PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(guardian_heroes_audio STATIC
    src/gh_audio.cpp src/gh_audio_driver.cpp src/gh_audio_system.cpp)
target_include_directories(guardian_heroes_audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/runtime/include ${CMAKE_CURRENT_SOURCE_DIR}/runtime/thirdparty/fmt/include ${CMAKE_CURRENT_SOURCE_DIR}/runtime/thirdparty/simde ${CMAKE_CURRENT_SOURCE_DIR}/runtime/thirdparty/spdlog/include)

add_library(guardian_heroes_graphics STATIC src/gh_graphics.cpp src/gh_gpu_commands.cpp)
target_include_directories(guardian_heroes_graphics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
set_target_properties(guardian_heroes_kernel guardian_heroes_audio guardian_heroes_graphics
    PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Preserve the compiler semantics of the runtime algorithms moved out of the SDK.
foreach(_gh_core_target guardian_heroes_guest_arena guardian_heroes_heap
        guardian_heroes_host_memory guardian_heroes_memory_service
        guardian_heroes_image_metadata guardian_heroes_xenos_math
        guardian_heroes_xma_packet guardian_heroes_audio guardian_heroes_graphics)
    get_target_property(_gh_aliased_target ${_gh_core_target} ALIASED_TARGET)
    if(NOT _gh_aliased_target)
        target_compile_options(${_gh_core_target} PRIVATE
            $<$<CXX_COMPILER_ID:Clang,AppleClang>:-fno-strict-aliasing;-ffp-model=strict;-fno-char8_t>
            $<$<CXX_COMPILER_ID:GNU>:-fno-strict-aliasing;-ffp-contract=off;-fno-fast-math;-fno-char8_t>)
    endif()
endforeach()

if(NOT REXGLUE_PLATFORM_SWITCH)
set(GUARDIANHEROES_SOURCES
    src/main.cpp
    src/guardian_heroes_app.cpp
    src/localization.cpp
    src/localization_options.cpp
    src/cheats.cpp
    src/cheats_damage.cpp
    src/gpu_wait.cpp
    src/guest_abi_check.cpp
    src/gh_clock_rex_adapter.cpp
    src/gh_logging.cpp
    src/gh_exceptions.cpp
    src/gh_filesystem_imports.cpp
    src/gh_filesystem_imports_rex_adapter.cpp
    src/gh_controller_assignment.cpp
    src/gh_controller_assignment_adapter.cpp
    src/gh_memory_rex_adapter.cpp
)

if(WIN32)
    add_executable(guardian_heroes WIN32 ${GUARDIANHEROES_SOURCES})
else()
    add_executable(guardian_heroes ${GUARDIANHEROES_SOURCES})
endif()

include(cmake/GuardianHeroesApp.cmake)
target_link_libraries(guardian_heroes PRIVATE guardian_heroes_native guardian_heroes_filesystem guardian_heroes_clock guardian_heroes_threading)
endif()
if(TARGET rexcore AND TARGET rexruntime)
    if(NOT GUARDIAN_HEROES_NATIVE_ONLY)
        include(cmake/GuardianHeroesSystem.cmake)
        guardian_heroes_use_project_system(rexruntime)
        target_sources(rexruntime PRIVATE
            src/gh_system_xmemory.cpp src/gh_system_mmio_handler.cpp
            src/gh_system_xex_module.cpp src/gh_system_function_dispatcher.cpp)
    endif()
    target_include_directories(rexcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_include_directories(rexruntime PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(rexcore PUBLIC guardian_heroes_host_memory)
    target_link_libraries(rexruntime PRIVATE
        guardian_heroes_clock
        guardian_heroes_threading
        guardian_heroes_memory_service
        guardian_heroes_kernel
        guardian_heroes_guest_arena
        guardian_heroes_heap
        guardian_heroes_mmio
        guardian_heroes_xenos_math
        guardian_heroes_xma_packet
    )
endif()
if(REXGLUE_PLATFORM_SWITCH AND REXGLUE_SWITCH_FFMPEG)
    # The Switch FFmpeg build is an external target-side archive set. Import
    # it explicitly so the audio adapter cannot silently fall back to a host
    # FFmpeg target or claim that C++ compilation alone is sufficient.
    set(_gh_ffmpeg_include "${REXGLUE_SWITCH_FFMPEG}/include")
    foreach(_gh_ffmpeg_name IN ITEMS avcodec avutil)
        set(_gh_ffmpeg_archive "${REXGLUE_SWITCH_FFMPEG}/lib/lib${_gh_ffmpeg_name}.a")
        if(EXISTS "${_gh_ffmpeg_archive}" AND NOT TARGET "lib${_gh_ffmpeg_name}")
            add_library("lib${_gh_ffmpeg_name}" STATIC IMPORTED GLOBAL)
            set_target_properties("lib${_gh_ffmpeg_name}" PROPERTIES
                IMPORTED_LOCATION "${_gh_ffmpeg_archive}"
                INTERFACE_INCLUDE_DIRECTORIES
                    "${_gh_ffmpeg_include};${CMAKE_CURRENT_SOURCE_DIR}/runtime/thirdparty/FFmpeg;${CMAKE_CURRENT_SOURCE_DIR}/runtime/thirdparty/FFmpeg/libavutil")
        endif()
    endforeach()
endif()
if(TARGET rexaudio AND REXGLUE_PLATFORM_SWITCH)
    include(cmake/GuardianHeroesSwitchAudio.cmake)
    if(TARGET libavcodec AND TARGET libavutil)
        guardian_heroes_switch_add_audio()
        target_link_libraries(rexruntime PRIVATE guardian_heroes_audio_xma)
    endif()
elseif(TARGET rexaudio)
    if(GUARDIAN_HEROES_NATIVE_ONLY)
        target_include_directories(rexaudio INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    else()
        target_include_directories(rexaudio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    endif()
    add_library(guardian_heroes_audio_sdl STATIC src/gh_audio_sdl.cpp)
    target_include_directories(guardian_heroes_audio_sdl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(guardian_heroes_audio_sdl PUBLIC guardian_heroes_audio PRIVATE SDL3::SDL3)
    add_library(guardian_heroes_audio_xma STATIC
        src/gh_audio_xma.cpp src/gh_audio_context.cpp src/gh_audio_runtime.cpp)
    target_include_directories(guardian_heroes_audio_xma PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(guardian_heroes_audio_xma
        PUBLIC guardian_heroes_kernel guardian_heroes_xma_packet
        PRIVATE libavcodec libavutil)
    set_target_properties(guardian_heroes_audio_sdl guardian_heroes_audio_xma
        PROPERTIES POSITION_INDEPENDENT_CODE ON)
    if(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64" OR
       (NOT APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$"))
        target_compile_options(guardian_heroes_audio_sdl PRIVATE
            $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-mssse3>)
        target_compile_options(guardian_heroes_audio_xma PRIVATE
            $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-mssse3>)
    endif()
    if(GUARDIAN_HEROES_NATIVE_ONLY)
        target_link_libraries(rexaudio INTERFACE guardian_heroes_audio_sdl guardian_heroes_audio_xma)
    else()
        target_link_libraries(rexaudio PRIVATE guardian_heroes_audio_sdl guardian_heroes_audio_xma)
    endif()
    target_link_libraries(rexruntime PRIVATE guardian_heroes_audio_sdl guardian_heroes_audio_xma)
    include(cmake/GuardianHeroesAudio.cmake)
endif()
if(TARGET mspack)
    add_library(guardian_heroes_image STATIC src/gh_image.cpp src/gh_image_loader.cpp)
    target_include_directories(guardian_heroes_image PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
    if(GUARDIAN_HEROES_NATIVE_ONLY)
        target_include_directories(guardian_heroes_image PRIVATE ${GUARDIAN_HEROES_RUNTIME_ROOT}/thirdparty)
    else()
        target_include_directories(guardian_heroes_image PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../rexglue-sdk-main/thirdparty)
    endif()
    target_link_libraries(guardian_heroes_image PRIVATE mspack guardian_heroes_image_metadata)
    set_target_properties(guardian_heroes_image PROPERTIES POSITION_INDEPENDENT_CODE ON)
    target_link_libraries(rexruntime PRIVATE guardian_heroes_image)
endif()
if(TARGET rexgpu-xenos)
    add_library(guardian_heroes_gpu_shader STATIC
        src/gh_gpu_shader.cpp
        src/gh_gpu_shader_translator.cpp
        src/gh_gpu_shader_translator_disasm.cpp)
    if(REXGLUE_USE_VULKAN)
        target_sources(guardian_heroes_gpu_shader PRIVATE
            src/gh_gpu_shader_spirv.cpp
            src/gh_gpu_shader_spirv_builder.cpp
            src/gh_gpu_shader_spirv_translator.cpp
            src/gh_gpu_shader_spirv_translator_alu.cpp
            src/gh_gpu_shader_spirv_translator_fetch.cpp
            src/gh_gpu_shader_spirv_translator_memexport.cpp
            src/gh_gpu_shader_spirv_translator_rb.cpp)
        target_link_libraries(guardian_heroes_gpu_shader PRIVATE glslang::SPIRV)
    endif()
    target_include_directories(guardian_heroes_gpu_shader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(guardian_heroes_gpu_shader PRIVATE rex::runtime fmt::fmt)
    set_target_properties(guardian_heroes_gpu_shader PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_target_properties(guardian_heroes_gpu_shader PROPERTIES
        CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
    target_link_libraries(rexgpu-xenos PRIVATE guardian_heroes_graphics guardian_heroes_gpu_shader)
    include(cmake/GuardianHeroesGpu.cmake)
endif()

foreach(_gh_device_target guardian_heroes_audio_sdl guardian_heroes_audio_xma
        guardian_heroes_image guardian_heroes_gpu_shader
        guardian_heroes_audio_service guardian_heroes_gpu_backend)
    if(TARGET ${_gh_device_target})
        target_compile_options(${_gh_device_target} PRIVATE
            $<$<CXX_COMPILER_ID:Clang,AppleClang>:-fno-strict-aliasing;-ffp-model=strict;-fno-char8_t>
            $<$<CXX_COMPILER_ID:GNU>:-fno-strict-aliasing;-ffp-contract=off;-fno-fast-math;-fno-char8_t>)
    endif()
endforeach()

if(REXGLUE_PLATFORM_SWITCH)
    if(GH_SWITCH_BUILD_GAME)
        include(cmake/GuardianHeroesSwitchGame.cmake)
        guardian_heroes_switch_add_game()
    endif()
    return()
endif()

# Exercise the real kernel adapters without loading an XEX or starting the game.
add_executable(guardian_heroes_kernel_adapter_test EXCLUDE_FROM_ALL
    tests/gh_kernel_adapter_test.cpp)
target_link_libraries(guardian_heroes_kernel_adapter_test PRIVATE rex::runtime)
target_include_directories(guardian_heroes_kernel_adapter_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(guardian_heroes_kernel_adapter_test PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)

add_executable(guardian_heroes_audio_system_state_test EXCLUDE_FROM_ALL
    tests/gh_audio_system_state_test.cpp)
target_link_libraries(guardian_heroes_audio_system_state_test PRIVATE rex::runtime)
target_include_directories(guardian_heroes_audio_system_state_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

include(cmake/GuardianHeroesTitleContinuation.cmake)

add_executable(guardian_heroes_continuation_restore_test EXCLUDE_FROM_ALL
    tests/gh_guest_continuation_restore_test.cpp)
target_link_libraries(guardian_heroes_continuation_restore_test PRIVATE rex::runtime)
target_include_directories(guardian_heroes_continuation_restore_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(guardian_heroes_continuation_restore_test PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)

add_executable(guardian_heroes_continuation_waits_test EXCLUDE_FROM_ALL
    tests/gh_guest_continuation_waits_test.cpp)
target_link_libraries(guardian_heroes_continuation_waits_test PRIVATE rex::runtime)
target_include_directories(guardian_heroes_continuation_waits_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(guardian_heroes_continuation_waits_test PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)

add_executable(guardian_heroes_runtime_snapshot_test EXCLUDE_FROM_ALL tests/gh_runtime_snapshot_test.cpp)
target_link_libraries(guardian_heroes_runtime_snapshot_test PRIVATE rex::runtime)
target_include_directories(guardian_heroes_runtime_snapshot_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(guardian_heroes_runtime_snapshot_test PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)

add_executable(guardian_heroes_xfile_continuation_test EXCLUDE_FROM_ALL
    tests/gh_xfile_continuation_test.cpp)
target_link_libraries(guardian_heroes_xfile_continuation_test PRIVATE rex::runtime)
target_include_directories(guardian_heroes_xfile_continuation_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(guardian_heroes_xfile_continuation_test PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)
