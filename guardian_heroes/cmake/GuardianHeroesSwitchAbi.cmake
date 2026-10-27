include_guard(GLOBAL)

function(guardian_heroes_switch_add_guest_abi_probe)
    if(NOT REXGLUE_PLATFORM_SWITCH)
        message(FATAL_ERROR "The guest ABI probe requires the Switch toolchain")
    endif()

    set(_gh_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/..")
    # Use the project's ABI compatibility headers, not a donor SDK context.
    add_library(guardian_heroes_switch_guest_abi_probe OBJECT EXCLUDE_FROM_ALL
        "${_gh_root}/src/guest_abi_check.cpp"
        "${_gh_root}/switch/guest_abi_probe.cpp")
    target_compile_features(guardian_heroes_switch_guest_abi_probe PRIVATE cxx_std_23)
    target_include_directories(guardian_heroes_switch_guest_abi_probe PRIVATE
        "${_gh_root}"
        "${_gh_root}/src"
        "${_gh_root}/runtime/include"
        "${_gh_root}/runtime/thirdparty/simde"
        "${_gh_root}/runtime/thirdparty/fmt/include"
        "${_gh_root}/runtime/thirdparty/spdlog/include")
    target_compile_definitions(guardian_heroes_switch_guest_abi_probe PRIVATE
        GH_PROJECT_RUNTIME=1 SPDLOG_FMT_EXTERNAL=1 SPDLOG_NO_TZ_OFFSET=1)
    target_compile_options(guardian_heroes_switch_guest_abi_probe PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-fno-fast-math;-frounding-math;-ffp-contract=off>
        $<$<CXX_COMPILER_ID:Clang>:-ffp-model=strict>)
endfunction()
