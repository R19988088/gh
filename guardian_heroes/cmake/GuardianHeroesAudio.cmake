# Project-owned guest callbacks, guest audio allocations, devices and XMA bridge.
if(NOT TARGET rexaudio)
    return()
endif()

set(GH_AUDIO_SRC "${CMAKE_CURRENT_LIST_DIR}/../src")
add_library(guardian_heroes_audio_service OBJECT
    "${GH_AUDIO_SRC}/gh_audio_driver.cpp"
    "${GH_AUDIO_SRC}/gh_audio_system.cpp"
    "${GH_AUDIO_SRC}/gh_audio_decoder.cpp"
    "${GH_AUDIO_SRC}/gh_audio_registers.cpp"
    "${GH_AUDIO_SRC}/gh_audio_sdl_driver.cpp"
    "${GH_AUDIO_SRC}/gh_audio_sdl_system.cpp"
    "${GH_AUDIO_SRC}/gh_audio_nop_system.cpp"
    "${GH_AUDIO_SRC}/gh_audio_factory.cpp")
target_include_directories(guardian_heroes_audio_service PUBLIC "${GH_AUDIO_SRC}")
target_link_libraries(guardian_heroes_audio_service
    PUBLIC guardian_heroes_audio_sdl guardian_heroes_audio_xma
    PRIVATE rexcore libavcodec libavutil)
set_target_properties(guardian_heroes_audio_service PROPERTIES POSITION_INDEPENDENT_CODE ON)
# Objects are incorporated once by the runtime; their host callbacks resolve
# against that same runtime and do not create a second runtime dependency.
target_link_libraries(rexruntime PRIVATE guardian_heroes_audio_service)
