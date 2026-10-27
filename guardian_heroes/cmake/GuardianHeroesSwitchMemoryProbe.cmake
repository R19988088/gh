# The same arena/host interfaces as the game, without the desktop runtime.
add_library(guardian_heroes_switch_memory_core STATIC EXCLUDE_FROM_ALL
    runtime/src/core/guest_memory_switch.cpp)
target_include_directories(guardian_heroes_switch_memory_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime/src/core"
    "${DEVKITPRO}/libnx/include")
target_compile_features(guardian_heroes_switch_memory_core PUBLIC cxx_std_23)
target_link_libraries(guardian_heroes_switch_memory_core PUBLIC "${GH_SWITCH_LIBNX}")
target_link_options(guardian_heroes_switch_memory_core PUBLIC -pthread)

add_library(guardian_heroes_switch_host_memory STATIC EXCLUDE_FROM_ALL
    src/gh_host_memory_switch.cpp)
target_include_directories(guardian_heroes_switch_host_memory PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(guardian_heroes_switch_host_memory PUBLIC guardian_heroes_switch_memory_core)

add_library(guardian_heroes_switch_guest_arena STATIC EXCLUDE_FROM_ALL src/gh_guest_arena.cpp)
target_include_directories(guardian_heroes_switch_guest_arena PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(guardian_heroes_switch_guest_arena PUBLIC guardian_heroes_switch_memory_core)

gh_switch_add_nro(guardian_heroes_switch_memory_probe
    SOURCES switch/memory_probe.cpp
    LIBRARIES guardian_heroes_switch_guest_arena guardian_heroes_switch_host_memory)
