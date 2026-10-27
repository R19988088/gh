# Directly linked project backend; no runtime GPU plugin loader.
add_library(rexgpu-xenos STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/gh_gpu_factory.cpp)
if(REXGLUE_USE_VULKAN)
    target_sources(rexgpu-xenos PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../src/gh_gpu_shader_spirv_device.cpp)
    target_link_libraries(rexgpu-xenos PRIVATE glslang::SPIRV renderdoc)
endif()
add_library(rex::gpu-xenos ALIAS rexgpu-xenos)
set_target_properties(rexgpu-xenos PROPERTIES OUTPUT_NAME guardian_heroes_gpu
    CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
if(REXGLUE_PLATFORM_SWITCH)
    # The platform deko3d implementation is linked from the project GPU
    # backend. Keep the backend name explicit so Switch cannot silently select
    # the desktop Vulkan command processor.
    target_compile_definitions(rexgpu-xenos PRIVATE GH_HAS_DEKO3D_BACKEND=1)
endif()
target_include_directories(rexgpu-xenos PUBLIC ${REXGLUE_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../src)
target_link_libraries(rexgpu-xenos PRIVATE rexruntime xxHash::xxhash)
