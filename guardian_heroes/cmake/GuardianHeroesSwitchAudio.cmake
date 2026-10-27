# Guardian Heroes' Switch XMA decoder, backed by the FFmpeg targets built by
# the ReXGlue SDK (which carries the Switch/AArch64 FFmpeg configuration).
include_guard(GLOBAL)

function(guardian_heroes_switch_add_audio)
    if(NOT REXGLUE_PLATFORM_SWITCH)
        message(FATAL_ERROR "Guardian Heroes Switch audio requires the devkitA64 toolchain")
    endif()

    foreach(_target guardian_heroes_kernel guardian_heroes_xma_packet libavcodec libavutil)
        if(NOT TARGET "${_target}")
            message(FATAL_ERROR
                "Cannot define Guardian Heroes Switch XMA audio: missing '${_target}'. "
                "Add the ReXGlue SDK (including its thirdparty directory) before calling "
                "guardian_heroes_switch_add_audio().")
        endif()
    endforeach()

    if(TARGET guardian_heroes_audio_xma)
        return()
    endif()

    set(_gh_audio_src "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../src")
    add_library(guardian_heroes_audio_xma STATIC
        "${_gh_audio_src}/gh_audio_decoder.cpp"
        "${_gh_audio_src}/gh_audio_xma.cpp"
        "${_gh_audio_src}/gh_audio_context.cpp"
        "${_gh_audio_src}/gh_audio_runtime.cpp")
    target_compile_features(guardian_heroes_audio_xma PUBLIC cxx_std_23)
    target_include_directories(guardian_heroes_audio_xma PUBLIC
        "${_gh_audio_src}"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../runtime/include"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../runtime/thirdparty/fmt/include"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../runtime/thirdparty/simde"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../runtime/thirdparty/spdlog/include")
    target_compile_definitions(guardian_heroes_audio_xma PRIVATE
        HAVE_AV_CONFIG_H _USE_MATH_DEFINES)
    target_link_libraries(guardian_heroes_audio_xma
        PUBLIC guardian_heroes_kernel guardian_heroes_xma_packet
        PRIVATE libavcodec libavutil)
    set_target_properties(guardian_heroes_audio_xma PROPERTIES
        POSITION_INDEPENDENT_CODE ON)
endfunction()
