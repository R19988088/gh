# Isolated probes: no desktop runtime, generated guest code, SDL or GPU SDK.
include("${CMAKE_CURRENT_LIST_DIR}/../switch/metadata.cmake")
find_program(GH_SWITCH_NACPTOOL nacptool HINTS "${DEVKITPRO}/tools/bin" REQUIRED)
find_program(GH_SWITCH_ELF2NRO elf2nro HINTS "${DEVKITPRO}/tools/bin" REQUIRED)
find_library(GH_SWITCH_LIBNX nx PATHS "${DEVKITPRO}/libnx/lib" NO_DEFAULT_PATH REQUIRED)

option(GH_SWITCH_BUILD_GAME
    "Assemble the full Guardian Heroes game NRO (requires project Switch runtime and Mesa/NVK)"
    OFF)

function(gh_switch_add_nro target)
    cmake_parse_arguments(PARSE_ARGV 1 GH "" "ROMFS_DIR" "SOURCES;LIBRARIES")
    if(GH_UNPARSED_ARGUMENTS OR NOT GH_SOURCES)
        message(FATAL_ERROR "gh_switch_add_nro requires SOURCES and optional LIBRARIES")
    endif()
    add_executable(${target} EXCLUDE_FROM_ALL ${GH_SOURCES})
    set_target_properties(${target} PROPERTIES SUFFIX ".elf")
    target_compile_features(${target} PRIVATE cxx_std_17)
    target_include_directories(${target} SYSTEM PRIVATE "${DEVKITPRO}/libnx/include")
    target_link_libraries(${target} PRIVATE ${GH_LIBRARIES} "${GH_SWITCH_LIBNX}")
    target_link_options(${target} PRIVATE "LINKER:-Map,${CMAKE_CURRENT_BINARY_DIR}/${target}.map")
    set(_nacp "${CMAKE_CURRENT_BINARY_DIR}/${target}.nacp")
    set(_nro "${CMAKE_CURRENT_BINARY_DIR}/${target}.nro")
    add_custom_command(OUTPUT "${_nacp}"
        COMMAND "${GH_SWITCH_NACPTOOL}" --create
            "${GH_SWITCH_APP_NAME}" "${GH_SWITCH_APP_AUTHOR}" "${GH_SWITCH_APP_VERSION}" "${_nacp}"
        DEPENDS "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../switch/metadata.cmake"
        VERBATIM)
    set(_romfs_args)
    if(GH_ROMFS_DIR)
        if(NOT IS_DIRECTORY "${GH_ROMFS_DIR}")
            message(FATAL_ERROR "ROMFS_DIR does not exist: ${GH_ROMFS_DIR}")
        endif()
        list(APPEND _romfs_args "--romfsdir=${GH_ROMFS_DIR}")
    endif()
    add_custom_command(OUTPUT "${_nro}"
        COMMAND "${GH_SWITCH_ELF2NRO}" "$<TARGET_FILE:${target}>" "${_nro}" "--nacp=${_nacp}" ${_romfs_args}
        DEPENDS ${target} "${_nacp}"
        VERBATIM)
    add_custom_target(${target}_nro DEPENDS "${_nro}")
endfunction()

gh_switch_add_nro(guardian_heroes_switch_bootstrap SOURCES switch/bootstrap_main.cpp)
target_compile_definitions(guardian_heroes_switch_bootstrap PRIVATE
    GH_SWITCH_VERSION="${GH_SWITCH_APP_VERSION}")

include("${CMAKE_CURRENT_LIST_DIR}/GuardianHeroesSwitchMemoryProbe.cmake" OPTIONAL)
if(NOT TARGET guardian_heroes_guest_arena AND TARGET guardian_heroes_switch_guest_arena)
    add_library(guardian_heroes_guest_arena ALIAS guardian_heroes_switch_guest_arena)
endif()
if(NOT TARGET guardian_heroes_host_memory AND TARGET guardian_heroes_switch_host_memory)
    add_library(guardian_heroes_host_memory ALIAS guardian_heroes_switch_host_memory)
endif()
include("${CMAKE_CURRENT_LIST_DIR}/GuardianHeroesSwitchAbi.cmake" OPTIONAL)
if(COMMAND guardian_heroes_switch_add_guest_abi_probe)
    guardian_heroes_switch_add_guest_abi_probe()
endif()
if(REXGLUE_SWITCH_NVK_SDK)
    include("${CMAKE_CURRENT_LIST_DIR}/GuardianHeroesSwitchVulkanProbe.cmake")
endif()

# Keep probe-only configurations usable without the large NVK SDK. Runtime
# targets are needed only for the explicitly requested game build.
if(GH_SWITCH_BUILD_GAME AND NOT REXGLUE_SWITCH_NVK_SDK)
    message(FATAL_ERROR "Switch game build requires REXGLUE_SWITCH_NVK_SDK")
endif()
set(REXGLUE_PLATFORM_SWITCH ON)
if(REXGLUE_SWITCH_NVK_SDK)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../runtime" runtime)
endif()

