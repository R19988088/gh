# Optional explicit-PC title backend. Python is resolved only when this target
# is built; the normal app uses the checked-in generated bodies directly.
if(NOT GUARDIAN_HEROES_NATIVE_ONLY)
    return()
endif()

set(_gh_continuation_dir "${CMAKE_CURRENT_BINARY_DIR}/title-continuation")
file(GLOB _gh_continuation_inputs CONFIGURE_DEPENDS
    "${REXGLUE_ENTRYPOINT_INCLUDE_DIR}/guardian_heroes_recomp.*.cpp")
file(GLOB _gh_continuation_hooks CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
set(_gh_continuation_sources)
foreach(_gh_input IN LISTS _gh_continuation_inputs)
    get_filename_component(_gh_name "${_gh_input}" NAME)
    string(REPLACE "guardian_heroes_recomp" "gh_title_continuation" _gh_name "${_gh_name}")
    list(APPEND _gh_continuation_sources "${_gh_continuation_dir}/${_gh_name}")
endforeach()
list(APPEND _gh_continuation_sources "${_gh_continuation_dir}/gh_title_continuation_registry.cpp")
add_custom_command(OUTPUT "${_gh_continuation_dir}/codegen.stamp"
    BYPRODUCTS ${_gh_continuation_sources}
    "${_gh_continuation_dir}/gh_title_continuation_report.json"
    "${_gh_continuation_dir}/gh_title_continuation_sources.cmake"
    COMMAND ${CMAKE_COMMAND}
        "-DGH_TITLE_SOURCE=${REXGLUE_ENTRYPOINT_INCLUDE_DIR}"
        "-DGH_TITLE_HOOKS=${CMAKE_CURRENT_SOURCE_DIR}/src"
        "-DGH_TITLE_OUTPUT=${_gh_continuation_dir}"
        "-DGH_TITLE_CODEGEN=${CMAKE_CURRENT_SOURCE_DIR}/tools/gh_guest_continuation_title_codegen.py"
        -P "${CMAKE_CURRENT_LIST_DIR}/RunTitleContinuationCodegen.cmake"
    COMMAND ${CMAKE_COMMAND} -E touch "${_gh_continuation_dir}/codegen.stamp"
    DEPENDS ${_gh_continuation_inputs} ${_gh_continuation_hooks}
        "${REXGLUE_ENTRYPOINT_INCLUDE_DIR}/guardian_heroes_init.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/gh_guest_continuation_codegen.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/gh_guest_continuation_title_codegen.py"
        "${CMAKE_CURRENT_LIST_DIR}/RunTitleContinuationCodegen.cmake"
    COMMENT "Generating explicit-PC title continuation partitions" VERBATIM)
add_custom_target(guardian_heroes_title_continuation_codegen
    DEPENDS "${_gh_continuation_dir}/codegen.stamp")

# Keep the same strong project hooks and original fallback functions available
# to atomic entries. The objects are passed directly to consumers so weak
# generated symbols cannot prevent a strong hook from being linked.
set(_gh_continuation_support ${GUARDIANHEROES_SOURCES})
list(REMOVE_ITEM _gh_continuation_support src/main.cpp src/guardian_heroes_app.cpp)
add_library(guardian_heroes_title_continuation_support OBJECT EXCLUDE_FROM_ALL
    ${_gh_continuation_support})
add_library(guardian_heroes_title_continuation STATIC EXCLUDE_FROM_ALL
    ${_gh_continuation_sources})
get_target_property(_gh_recomp_options guardian_heroes_recomp COMPILE_OPTIONS)
get_target_property(_gh_recomp_definitions guardian_heroes_recomp COMPILE_DEFINITIONS)
foreach(_gh_target guardian_heroes_title_continuation guardian_heroes_title_continuation_support)
    target_include_directories(${_gh_target} PRIVATE
        "$<TARGET_PROPERTY:guardian_heroes_recomp,INCLUDE_DIRECTORIES>")
    target_link_libraries(${_gh_target} PRIVATE rex::runtime)
    if(_gh_recomp_options)
        target_compile_options(${_gh_target} PRIVATE ${_gh_recomp_options})
    endif()
    if(_gh_recomp_definitions)
        target_compile_definitions(${_gh_target} PRIVATE ${_gh_recomp_definitions})
    endif()
    target_precompile_headers(${_gh_target} REUSE_FROM guardian_heroes_recomp)
endforeach()
set_property(GLOBAL APPEND PROPERTY JOB_POOLS gh_title_continuation_compile=2)
set_property(TARGET guardian_heroes_title_continuation PROPERTY
    JOB_POOL_COMPILE gh_title_continuation_compile)
target_include_directories(guardian_heroes_title_continuation PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(guardian_heroes_title_continuation PUBLIC
    rex::runtime guardian_heroes_native guardian_heroes_filesystem
    guardian_heroes_clock guardian_heroes_threading
    "$<TARGET_OBJECTS:guardian_heroes_recomp>"
    "$<TARGET_OBJECTS:guardian_heroes_title_continuation_support>")
add_dependencies(guardian_heroes_title_continuation
    guardian_heroes_recomp guardian_heroes_title_continuation_support
    guardian_heroes_title_continuation_codegen)

add_executable(guardian_heroes_title_continuation_smoke EXCLUDE_FROM_ALL
    tests/gh_title_continuation_smoke.cpp)
target_include_directories(guardian_heroes_title_continuation_smoke PRIVATE
    "${REXGLUE_ENTRYPOINT_INCLUDE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(guardian_heroes_title_continuation_smoke PRIVATE
    guardian_heroes_title_continuation)
rexglue_apply_target_settings(guardian_heroes_title_continuation_smoke)
target_compile_options(guardian_heroes_title_continuation_smoke PRIVATE
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-UNDEBUG>
    $<$<CXX_COMPILER_ID:MSVC>:/UNDEBUG>)

# The same host app, window entry and strong hooks with explicit-PC execution.
# This executable is built only when requested for real title validation.
if(WIN32)
    add_executable(guardian_heroes_continuation_app WIN32 EXCLUDE_FROM_ALL
        src/main_continuation.cpp src/guardian_heroes_app.cpp)
else()
    add_executable(guardian_heroes_continuation_app EXCLUDE_FROM_ALL
        src/main_continuation.cpp src/guardian_heroes_app.cpp)
endif()
guardian_heroes_configure_app(guardian_heroes_continuation_app
    guardian_heroes_title_continuation)
