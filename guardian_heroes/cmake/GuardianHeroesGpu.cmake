# Project-owned GPU runtime; public rex::graphics names preserve the plugin ABI.
# Device/window/swapchain services remain in rexruntime's native UI layer.
set(_gh_gpu_source_dir "${CMAKE_CURRENT_LIST_DIR}/../src")
set(_gh_gpu_runtime_root "${CMAKE_CURRENT_LIST_DIR}/../runtime")
if(GUARDIAN_HEROES_NATIVE_ONLY)
    set(_gh_gpu_sdk_root "${GUARDIAN_HEROES_RUNTIME_ROOT}")
else()
    set(_gh_gpu_sdk_root "${CMAKE_CURRENT_LIST_DIR}/../../rexglue-sdk-main")
endif()
add_library(guardian_heroes_gpu_backend STATIC
    "${_gh_gpu_source_dir}/gh_gpu_flags.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_register_file.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_registers.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_ucode.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_sampler_info.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_graphics_system.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_command_processor.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_draw_extent_estimator.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_draw.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_packet_disassembler.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_primitive_processor.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_render_target_cache.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_shared_memory.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_texture_cache.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_shader_interpreter.cpp"
)

# The Switch renderer is a separate Xenos command processor.  Keep it out of
# the desktop backend: the deko3d implementation has to own its frame pools,
# EDRAM resolves and VI window, and cannot be made correct by selecting the
# Vulkan command processor at runtime.  The donor implementation is supplied
# outside the repository because it is platform/SDK specific.
if(REXGLUE_PLATFORM_SWITCH)
    set(GUARDIAN_HEROES_DEKO3D_BACKEND_SOURCE
        "${_gh_gpu_source_dir}/gpu/deko3d_backend.cpp" CACHE FILEPATH
        "Switch deko3d Xenos backend source")
    if(NOT EXISTS "${GUARDIAN_HEROES_DEKO3D_BACKEND_SOURCE}")
        message(FATAL_ERROR "Missing Guardian Heroes deko3d backend: ${GUARDIAN_HEROES_DEKO3D_BACKEND_SOURCE}")
    endif()
    target_sources(guardian_heroes_gpu_backend PRIVATE
        "${GUARDIAN_HEROES_DEKO3D_BACKEND_SOURCE}")
    target_compile_definitions(guardian_heroes_gpu_backend PRIVATE
        GH_HAS_DEKO3D_BACKEND=1)
endif()
if(NOT GUARDIAN_HEROES_NATIVE_ONLY)
    target_sources(guardian_heroes_gpu_backend PRIVATE "${_gh_gpu_source_dir}/gh_gpu_factory.cpp")
endif()
target_include_directories(guardian_heroes_gpu_backend PUBLIC "${_gh_gpu_source_dir}"
    PRIVATE "${_gh_gpu_runtime_root}" "${_gh_gpu_sdk_root}"
        "${_gh_gpu_sdk_root}/thirdparty/renderdoc")
target_compile_options(guardian_heroes_gpu_backend PRIVATE -Wno-unused-variable)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64)$")
    target_compile_options(guardian_heroes_gpu_backend PRIVATE
        $<$<CXX_COMPILER_ID:Clang,AppleClang>:-msse4.1>)
endif()
target_link_libraries(guardian_heroes_gpu_backend PRIVATE
    rex::runtime guardian_heroes_gpu_shader guardian_heroes_graphics xxHash::xxhash)
if(REXGLUE_PLATFORM_SWITCH)
    target_include_directories(guardian_heroes_gpu_backend PRIVATE
        "${_gh_gpu_source_dir}/gpu")
endif()
set_target_properties(guardian_heroes_gpu_backend PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
if(REXGLUE_USE_VULKAN)
    target_sources(guardian_heroes_gpu_backend PRIVATE
    "${_gh_gpu_source_dir}/gh_gpu_vulkan_graphics_system.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_vulkan_deferred_command_buffer.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_vulkan_command_processor.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_vulkan_pipeline_cache.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_vulkan_primitive_processor.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_vulkan_render_target_cache.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_vulkan_shader.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_vulkan_shared_memory.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_vulkan_texture_cache.cpp"
    )
    target_link_libraries(guardian_heroes_gpu_backend PRIVATE
        Vulkan::Headers glslang::SPIRV GPUOpen::VulkanMemoryAllocator
        glslang MachineIndependent GenericCodeGen OSDependent OGLCompiler)
endif()
if(REXGLUE_PLATFORM_SWITCH)
    target_link_libraries(guardian_heroes_gpu_backend PRIVATE deko3d)
endif()
if(REXGLUE_USE_D3D12)
    target_sources(guardian_heroes_gpu_backend PRIVATE
    "${_gh_gpu_source_dir}/gh_gpu_shader_dxbc.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_shader_dxbc_translator.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_shader_dxbc_translator_alu.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_shader_dxbc_translator_fetch.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_shader_dxbc_translator_memexport.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_shader_dxbc_translator_om.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_d3d12_graphics_system.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_d3d12_command_processor.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_d3d12_primitive_processor.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_d3d12_render_target_cache.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_d3d12_shader.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_d3d12_shared_memory.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_d3d12_texture_cache.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_d3d12_deferred_command_list.cpp"
    "${_gh_gpu_source_dir}/gh_gpu_d3d12_pipeline_cache.cpp"
        "${_gh_gpu_sdk_root}/thirdparty/dxbc/DXBCChecksum.cpp")
    set_source_files_properties("${_gh_gpu_sdk_root}/thirdparty/dxbc/DXBCChecksum.cpp"
        PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
    target_link_libraries(guardian_heroes_gpu_backend PRIVATE d3d12 dxgi dxguid)
endif()
target_precompile_headers(guardian_heroes_gpu_backend PRIVATE
    <mutex> <unordered_map> <rex/math.h> <rex/memory.h>
    <rex/thread/mutex.h> <rex/system/xmemory.h> <rex/graphics/xenos.h>
    <rex/graphics/registers.h> <rex/graphics/format/ucode.h>
    <rex/graphics/pipeline/shader/shader.h>)
target_link_libraries(rexgpu-xenos PRIVATE guardian_heroes_gpu_backend)

if(REXGLUE_USE_VULKAN)
    add_executable(guardian_heroes_gpu_snapshot_test EXCLUDE_FROM_ALL
        "${CMAKE_CURRENT_LIST_DIR}/../tests/gh_gpu_snapshot_test.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/../tests/gh_gpu_render_target_snapshot_test_support.cpp")
    target_link_libraries(guardian_heroes_gpu_snapshot_test PRIVATE
        rex::gpu-xenos guardian_heroes_gpu_backend guardian_heroes_gpu_shader rex::runtime Vulkan::Headers
        glslang::SPIRV xxHash::xxhash GPUOpen::VulkanMemoryAllocator)
    target_compile_options(guardian_heroes_gpu_snapshot_test PRIVATE
        $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-UNDEBUG>)
endif()
