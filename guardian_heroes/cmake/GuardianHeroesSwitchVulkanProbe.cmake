# Static Horizon NVK, isolated from the desktop loader and game runtime.
# Keep configuration usable while the optional SDK is being built. The full
# game target performs its own REQUIRED checks when explicitly requested.
set(_gh_nvk_missing)
foreach(_gh_candidate vulkan expat zstd z)
    find_library(GH_SWITCH_${_gh_candidate}_LIBRARY NAMES ${_gh_candidate}
        PATHS "${REXGLUE_SWITCH_NVK_SDK}/lib" "${DEVKITPRO}/portlibs/switch/lib"
        NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH NO_CACHE)
    if(NOT GH_SWITCH_${_gh_candidate}_LIBRARY)
        list(APPEND _gh_nvk_missing ${_gh_candidate})
    endif()
endforeach()
find_path(GH_SWITCH_VULKAN_INCLUDE_DIR vulkan/vulkan.h
    PATHS "${REXGLUE_SWITCH_NVK_SDK}/include" "${DEVKITPRO}/portlibs/switch/include"
    NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH NO_CACHE)
if(_gh_nvk_missing OR NOT GH_SWITCH_VULKAN_INCLUDE_DIR)
    message(STATUS "Skipping Switch Vulkan probe: NVK SDK is incomplete")
    return()
endif()

set(_gh_nvk_libs)
foreach(_gh_lib vulkan expat zstd z)
    list(APPEND _gh_nvk_libs "${GH_SWITCH_${_gh_lib}_LIBRARY}")
endforeach()
string(JOIN "," _gh_nvk_group ${_gh_nvk_libs})

gh_switch_add_nro(guardian_heroes_switch_vulkan_probe
    SOURCES switch/vulkan_probe.cpp switch/nvk_elf_stubs.c
    LIBRARIES "$<LINK_GROUP:RESCAN,${_gh_nvk_group}>" m)
target_include_directories(guardian_heroes_switch_vulkan_probe SYSTEM PRIVATE
    "${GH_SWITCH_VULKAN_INCLUDE_DIR}")
target_compile_definitions(guardian_heroes_switch_vulkan_probe PRIVATE VK_USE_PLATFORM_VI_NN)
target_link_options(guardian_heroes_switch_vulkan_probe PRIVATE -pthread)
