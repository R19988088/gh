# Project-owned runtime, guest objects and title/thread orchestration.
# The rex namespaces retain the existing PPC and module ABI.
set(GUARDIAN_HEROES_SYSTEM_SDK_SOURCES
    achievement_manager.cpp
    achievements.cpp
    elf_module.cpp
    embedded_metadata.cpp
    entry_table.cpp
    export_resolver.cpp
    guest_path.cpp
    kernel_module.cpp
    kernel_state.cpp
    lzx.cpp
    map_parser.cpp
    module.cpp
    ppc_types.cpp
    runtime.cpp
    thread.cpp
    thread_state.cpp
    user_module.cpp
    util/gameinfo_utils.cpp
    util/native_list.cpp
    util/object_table.cpp
    util/xdbf_utils.cpp
    xam/app_manager.cpp
    xam/content_manager.cpp
    xam/user_profile.cpp
    xenumerator.cpp
    xevent.cpp
    xfile.cpp
    xiocompletion.cpp
    xmodule.cpp
    xmutant.cpp
    xnotifylistener.cpp
    xobject.cpp
    xsemaphore.cpp
    xsocket.cpp
    xsymboliclink.cpp
    xthread.cpp
    xtimer.cpp
)

set(GUARDIAN_HEROES_SYSTEM_SOURCES)
foreach(_gh_source IN LISTS GUARDIAN_HEROES_SYSTEM_SDK_SOURCES)
    string(REPLACE "/" "_" _gh_project_source "${_gh_source}")
    list(APPEND GUARDIAN_HEROES_SYSTEM_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/../src/gh_system_${_gh_project_source}")
endforeach()
list(APPEND GUARDIAN_HEROES_SYSTEM_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/../src/gh_guest_continuation.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../src/gh_guest_continuation_title.cpp")
list(APPEND GUARDIAN_HEROES_SYSTEM_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/../src/gh_guest_continuation_waits.cpp")

function(guardian_heroes_use_project_system target)
    get_target_property(_gh_sdk_source_dir ${target} SOURCE_DIR)
    get_target_property(_gh_existing_sources ${target} SOURCES)
    if(NOT _gh_existing_sources)
        set(_gh_existing_sources)
    endif()
    set(_gh_retained_sources)
    foreach(_gh_source IN LISTS _gh_existing_sources)
        if(_gh_source MATCHES "^\\$<")
            list(APPEND _gh_retained_sources "${_gh_source}")
            continue()
        endif()
        if(IS_ABSOLUTE "${_gh_source}")
            set(_gh_absolute_source "${_gh_source}")
        else()
            set(_gh_absolute_source "${_gh_sdk_source_dir}/${_gh_source}")
        endif()
        file(RELATIVE_PATH _gh_relative_source "${_gh_sdk_source_dir}" "${_gh_absolute_source}")
        if(NOT _gh_relative_source IN_LIST GUARDIAN_HEROES_SYSTEM_SDK_SOURCES)
            list(APPEND _gh_retained_sources "${_gh_absolute_source}")
        endif()
    endforeach()
    set_property(TARGET ${target} PROPERTY SOURCES "${_gh_retained_sources}")
    target_sources(${target} PRIVATE ${GUARDIAN_HEROES_SYSTEM_SOURCES})
    target_include_directories(${target} PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src>")
endfunction()
