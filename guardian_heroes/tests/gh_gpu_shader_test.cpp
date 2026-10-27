// Guardian Heroes runtime shader-storage fixtures (title 58410B2B).
// Guest ucode words are normalized to host endian before analysis.
#include "gh_gpu_shader_spirv.h"
#include "gh_gpu_shader_spirv_translator.h"
#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <span>
#include <stdexcept>
using namespace rex::graphics;
namespace {
void Require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
constexpr uint32_t kUcode0[] = {
  0x30052003u, 0x00001200u, 0xC4000000u, 0x00001005u, 0x00001200u, 0xC2000000u,
  0x00001006u, 0x10071200u, 0x22000000u, 0x30081000u, 0x00393A88u, 0x00000007u,
  0x00080000u, 0x40263688u, 0x00000300u, 0xC80F8000u, 0x00000000u, 0xE2000000u,
  0xC80F803Eu, 0x00000000u, 0xE2010100u, 0xC8000000u, 0x00000000u, 0xE2000000u,
  0x00000000u, 0x00000000u, 0x00000000u,
};

constexpr uint32_t kUcode1[] = {
  0x00000000u, 0x1001C400u, 0x22000000u, 0xC80F8000u, 0x00000000u, 0xE2000000u,
  0x00000000u, 0x00000000u, 0x00000000u,
};

constexpr uint32_t kUcode2[] = {
  0x30052003u, 0x00001200u, 0xC2000000u, 0x00001005u, 0x00001200u, 0xC4000000u,
  0x00001006u, 0x00002200u, 0x00000000u, 0x15F81000u, 0x00253B08u, 0x00000003u,
  0x05F80000u, 0x4006060Au, 0x00000203u, 0xC80F803Eu, 0x00000000u, 0xE2010100u,
  0xC80F8000u, 0x00000000u, 0xE2000000u, 0x00000000u, 0x00000000u, 0x00000000u,
};

constexpr uint32_t kUcode3[] = {
  0x00000000u, 0x1001C400u, 0x22000000u, 0xC80F8000u, 0x00000000u, 0xA1000000u,
  0x00000000u, 0x00000000u, 0x00000000u,
};

constexpr uint32_t kUcode4[] = {
  0x30052003u, 0x00001200u, 0xC2000000u, 0x00001005u, 0x00001200u, 0xC4000000u,
  0x00001006u, 0x00002200u, 0x00000000u, 0x1DF81000u, 0x00253B08u, 0x00000004u,
  0x05F80000u, 0x40253FC8u, 0x00000204u, 0xC80F803Eu, 0x00000000u, 0xE2010100u,
  0xC8038000u, 0x00B0B000u, 0xE2000000u, 0x00000000u, 0x00000000u, 0x00000000u,
};

constexpr uint32_t kUcode5[] = {
  0x00011002u, 0x00001200u, 0xC4000000u, 0x00002003u, 0x00002200u, 0x00000000u,
  0x10080001u, 0x1F1FF688u, 0x00004000u, 0xC80F0000u, 0x001B0000u, 0xAB000100u,
  0xC80F8000u, 0x00000000u, 0xA1000000u, 0x00000000u, 0x00000000u, 0x00000000u,
};

constexpr uint32_t kUcode6[] = {
  0x70153003u, 0x00001200u, 0xC2000000u, 0x00001006u, 0x00001200u, 0xC4000000u,
  0x00003007u, 0x00002200u, 0x00000000u, 0x1DF81000u, 0x0025323Fu, 0x00000004u,
  0x05F81000u, 0x40192FC1u, 0x00000204u, 0x05F80000u, 0x4006060Au, 0x00000304u,
  0xC88BC03Eu, 0x001A6C00u, 0xA001FF00u, 0xC8038000u, 0x00B0B000u, 0xA1010200u,
  0xC80F8001u, 0x00000000u, 0xE2000000u, 0xC80F8002u, 0x00000000u, 0x22010100u,
  0x00000000u, 0x00000000u, 0x00000000u,
};

constexpr uint32_t kUcode7[] = {
  0x00103002u, 0x00001000u, 0xC4000000u, 0x00006005u, 0x200B5600u, 0x22000000u,
  0xC8040200u, 0x00FFC600u, 0xAF01FF00u, 0x70000202u, 0x000000C6u, 0xE2020200u,
  0x10080001u, 0x1F1FF688u, 0x00004000u, 0xC8010200u, 0x18A7A700u, 0xEF000100u,
  0xC8080200u, 0x186CB100u, 0xA500FF00u, 0xC8020200u, 0x1C1B6CC6u, 0xCD8000FFu,
  0xC8050200u, 0x18B0B000u, 0xE0000000u, 0xB8200002u, 0x190000C0u, 0xC20202FFu,
  0xC8020200u, 0x1C1B6CB1u, 0xAD80FF00u, 0xC80F8000u, 0x18F10000u, 0xE1000200u,
  0xC80F8000u, 0x10000000u, 0xE1000200u, 0x00000000u, 0x00000000u, 0x00000000u,
};

struct Fixture { xenos::ShaderType type; std::span<const uint32_t> words; };
const Fixture kFixtures[] = {
{xenos::ShaderType::kVertex, kUcode0},
{xenos::ShaderType::kPixel, kUcode1},
{xenos::ShaderType::kVertex, kUcode2},
{xenos::ShaderType::kPixel, kUcode3},
{xenos::ShaderType::kVertex, kUcode4},
{xenos::ShaderType::kPixel, kUcode5},
{xenos::ShaderType::kVertex, kUcode6},
{xenos::ShaderType::kPixel, kUcode7}
};
}
int main(int argc, char** argv) {
  Require(argc == 2, "Provide the output directory");
  const std::filesystem::path output(argv[1]);
  std::filesystem::create_directories(output);
  size_t translated = 0;
  for (bool all_features : {false, true}) {
    SpirvShaderTranslator::Features features(all_features);
    for (bool interlock : {false, true}) {
      features.fragment_shader_sample_interlock = interlock;
      SpirvShaderTranslator translator(features, true, true, interlock);
      for (size_t i = 0; i < std::size(kFixtures); ++i) {
        const auto& fixture = kFixtures[i];
        SpirvShader shader(fixture.type, i, fixture.words.data(), fixture.words.size(), std::endian::native);
        rex::string::StringBuffer disassembly;
        shader.AnalyzeUcode(disassembly);
        Require(shader.is_ucode_analyzed(), "Ucode analysis failed");
        SpirvShaderTranslator::Modification modification(fixture.type == xenos::ShaderType::kVertex
            ? translator.GetDefaultVertexShaderModification(64)
            : translator.GetDefaultPixelShaderModification(64));
        if (fixture.type == xenos::ShaderType::kVertex) modification.vertex.interpolator_mask = 0xFFFF;
        else modification.pixel.interpolator_mask = 0xFFFF;
        auto* translation = shader.GetOrCreateTranslation(modification.value);
        Require(translator.TranslateAnalyzedShader(*translation), "Ucode translation failed");
        const auto binary = translation->GetTranslatedBinaryString();
        Require(binary.size() >= 20 && binary.size() % 4 == 0, "Invalid SPIR-V size");
        uint32_t magic;
        std::memcpy(&magic, binary.data(), 4);
        Require(magic == 0x07230203, "Invalid SPIR-V magic");
        const std::string name = std::to_string(all_features) + "-" + std::to_string(interlock) + "-" + std::to_string(i);
        std::ofstream(output / (name + ".spv"), std::ios::binary).write(binary.data(), binary.size());
        const auto ucode_disasm = shader.ucode_disassembly();
        std::ofstream(output / (name + ".txt"), std::ios::binary).write(ucode_disasm.data(), ucode_disasm.size());
        ++translated;
      }
      for (uint32_t mode = 0; mode <= 3; ++mode) {
        const auto binary = translator.CreateDepthOnlyFragmentShader(
            static_cast<SpirvShaderTranslator::Modification::DepthStencilMode>(mode));
        Require(!binary.empty(), "Depth-only translation failed");
        const std::string name = std::to_string(all_features) + "-" + std::to_string(interlock) + "-depth-" + std::to_string(mode);
        std::ofstream(output / (name + ".spv"), std::ios::binary).write(reinterpret_cast<const char*>(binary.data()), binary.size());
        ++translated;
      }
    }
  }
  std::printf("Shader core: %zu real-ucode/depth translations passed\n", translated);
}
