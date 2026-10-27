# The app owns the host lifecycle and uses the selected runtime's platform entry.
add_library(guardian_heroes_recomp OBJECT ${REXGLUE_ENTRYPOINT_GENERATED_SOURCES})
target_include_directories(guardian_heroes_recomp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
target_link_libraries(guardian_heroes_recomp PRIVATE rex::runtime)
rexglue_apply_target_settings(guardian_heroes_recomp)
rexglue_apply_recomp_settings(guardian_heroes_recomp
    "${REXGLUE_ENTRYPOINT_INCLUDE_DIR}/guardian_heroes_pch.h")
add_dependencies(guardian_heroes_recomp guardian_heroes_codegen)
function(guardian_heroes_configure_app target guest_backend)
    add_dependencies(${target} guardian_heroes_codegen)
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
    if(GUARDIAN_HEROES_NATIVE_ONLY)
        target_sources(${target} PRIVATE
            ${GUARDIAN_HEROES_RUNTIME_ROOT}/share/windowed_app_main_sdl.cpp)
    else()
        target_sources(${target} PRIVATE
            ${REXGLUE_SHARE_DIR}/windowed_app_main_sdl.cpp)
    endif()
    target_link_libraries(${target} PRIVATE ${guest_backend} rex::runtime)
    target_compile_definitions(${target} PRIVATE REXGLUE_BUILD_CONFIG="$<CONFIG>")
    rexglue_apply_target_settings(${target})

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/metadata/icons")
        rexglue_embed_metadata(${target}
            DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/metadata/icons" PREFIX "icons")
    endif()
    if(UNIX)
        if(APPLE)
            set(_gh_runtime_rpath "@executable_path")
        else()
            set(_gh_runtime_rpath "$ORIGIN")
        endif()
        set_target_properties(${target} PROPERTIES
            INSTALL_RPATH "${_gh_runtime_rpath}" BUILD_WITH_INSTALL_RPATH ON)
        if(GUARDIAN_HEROES_NATIVE_ONLY AND TARGET TracyClient)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:TracyClient> $<TARGET_FILE_DIR:${target}> VERBATIM)
        endif()
        if(NOT GUARDIAN_HEROES_NATIVE_ONLY)
        foreach(_gh_runtime_lib rex::runtime rex::TracyClient TracyClient)
            if(TARGET ${_gh_runtime_lib})
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        $<TARGET_FILE:${_gh_runtime_lib}>
                        $<TARGET_FILE_DIR:${target}> VERBATIM)
            endif()
        endforeach()
        endif()
    elseif(WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "$<$<BOOL:$<TARGET_RUNTIME_DLLS:${target}>>:${CMAKE_COMMAND};-E;copy_if_different;$<TARGET_RUNTIME_DLLS:${target}>;$<TARGET_FILE_DIR:${target}>>"
            COMMAND_EXPAND_LISTS VERBATIM)
        foreach(_gh_fx amd_fidelityfx_vk amd_fidelityfx_dx12)
            if(TARGET ${_gh_fx})
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        $<TARGET_FILE:${_gh_fx}> $<TARGET_FILE_DIR:${target}> VERBATIM)
            endif()
        endforeach()
    endif()
    if(TARGET rexgpu-xenos)
        set(_gh_gpu_target rexgpu-xenos)
    elseif(TARGET rex::gpu-xenos)
        set(_gh_gpu_target rex::gpu-xenos)
    else()
        message(FATAL_ERROR "The existing Xenos device backend is required")
    endif()
    if(GUARDIAN_HEROES_NATIVE_ONLY)
        target_link_libraries(${target} PRIVATE ${_gh_gpu_target})
    else()
    add_dependencies(${target} ${_gh_gpu_target})
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${_gh_gpu_target}> $<TARGET_FILE_DIR:${target}> VERBATIM)
    endif()
    if(APPLE AND REXGLUE_USE_VULKAN)
        if(GUARDIAN_HEROES_NATIVE_ONLY)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:${target}>/vulkan/lib"
                    "$<TARGET_FILE_DIR:${target}>/vulkan/share/vulkan/icd.d"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:Vulkan::Loader>" "$<TARGET_FILE_DIR:${target}>/vulkan/lib/libvulkan.1.dylib"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:MoltenVK::MoltenVK>" "$<TARGET_FILE_DIR:${target}>/vulkan/lib/libMoltenVK.dylib"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${GUARDIAN_HEROES_RUNTIME_ROOT}/cmake/MoltenVK_icd.json"
                    "$<TARGET_FILE_DIR:${target}>/vulkan/share/vulkan/icd.d/MoltenVK_icd.json" VERBATIM)
        else()
            _rexglue_stage_macos_vulkan_runtime(${target})
        endif()
    endif()
endfunction()

guardian_heroes_configure_app(guardian_heroes guardian_heroes_recomp)
