/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <rex/graphics/pipeline/shader/spirv_translator.h>
#include <rex/ui/vulkan/device.h>

namespace rex::graphics {

SpirvShaderTranslator::Features::Features(const ui::vulkan::VulkanDevice* const vulkan_device)
    : max_storage_buffer_range(vulkan_device->properties().maxStorageBufferRange),
      full_draw_index_uint32(vulkan_device->properties().fullDrawIndexUint32),
      vertex_pipeline_stores_and_atomics(
          vulkan_device->properties().vertexPipelineStoresAndAtomics),
      fragment_stores_and_atomics(vulkan_device->properties().fragmentStoresAndAtomics),
      clip_distance(vulkan_device->properties().shaderClipDistance),
      cull_distance(vulkan_device->properties().shaderCullDistance),
      image_view_format_swizzle(vulkan_device->properties().imageViewFormatSwizzle),
      signed_zero_inf_nan_preserve_float32(
          vulkan_device->properties().shaderSignedZeroInfNanPreserveFloat32),
      denorm_flush_to_zero_float32(vulkan_device->properties().shaderDenormFlushToZeroFloat32),
      rounding_mode_rte_float32(vulkan_device->properties().shaderRoundingModeRTEFloat32),
      fragment_shader_sample_interlock(vulkan_device->properties().fragmentShaderSampleInterlock),
      demote_to_helper_invocation(vulkan_device->properties().shaderDemoteToHelperInvocation),
      sample_rate_shading(vulkan_device->properties().sampleRateShading) {
  const uint32_t vulkan_api_version = vulkan_device->properties().apiVersion;
  if (vulkan_api_version >= VK_MAKE_API_VERSION(0, 1, 2, 0)) {
    spirv_version = spv::Spv_1_5;
  } else if (vulkan_device->extensions().ext_1_2_KHR_spirv_1_4) {
    spirv_version = spv::Spv_1_4;
  } else if (vulkan_api_version >= VK_MAKE_API_VERSION(0, 1, 1, 0)) {
    spirv_version = spv::Spv_1_3;
  } else {
    spirv_version = spv::Spv_1_0;
  }
}

}  // namespace rex::graphics
