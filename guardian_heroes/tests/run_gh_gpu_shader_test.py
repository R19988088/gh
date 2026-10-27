#!/usr/bin/env python3
"""Compile the real shader core; optionally compare a pre-migration snapshot."""
import argparse
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference-source", type=Path,
                        help="Directory of the original shader .cpp/.h files")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sdk = root / "rexglue-sdk-main"
    libraries = sdk / "out/mac-arm64"
    runtime = root / "guardian_heroes/out/build/mac-arm64-release"
    project = root / "guardian_heroes/src"
    with tempfile.TemporaryDirectory(prefix="gh-gpu-shader-") as directory:
        work = Path(directory)
        includes = [project, sdk / "include", sdk / "thirdparty/glslang",
                    sdk / "thirdparty/fmt/include", sdk / "thirdparty/spdlog/include",
                    sdk / "thirdparty/xxHash"]
        flags = ["/usr/bin/clang++", "-std=c++23", "-O1", "-DNDEBUG",
                 "-DSPDLOG_FMT_EXTERNAL", "-DREX_PLATFORM_MAC=1", "-DREX_HAS_VULKAN=1",
                 "-Wno-unused-variable", "-Wno-nontrivial-memcall", "-Wl,-dead_strip"]
        helpers = [project / "gh_gpu_ucode.cpp", project / "gh_gpu_flags.cpp",
                   project / "gh_gpu_draw.cpp", Path(__file__).with_name("gh_gpu_shader_test.cpp")]
        links = ["-L" + str(libraries), "-lSPIRV", "-lglslang", "-lMachineIndependent",
                 "-lGenericCodeGen", "-lOSDependent", "-lOGLCompiler", "-lfmt",
                 "-L" + str(runtime), "-lrexruntime", "-Wl,-rpath," + str(runtime)]

        def run(name, sources, include_paths):
            command = flags.copy()
            for path in include_paths:
                command.extend(["-I", str(path)])
            command.extend(map(str, [*sources, *helpers]))
            command.extend([*links, "-o", str(work / name)])
            subprocess.run(command, check=True)
            output = work / (name + "-output")
            subprocess.run([str(work / name), str(output)], check=True)
            return output

        actual = run("project", [project / (name + ".cpp") for name in (
            "gh_gpu_shader", "gh_gpu_shader_translator", "gh_gpu_shader_translator_disasm",
            "gh_gpu_shader_spirv", "gh_gpu_shader_spirv_builder", "gh_gpu_shader_spirv_translator",
            "gh_gpu_shader_spirv_translator_alu", "gh_gpu_shader_spirv_translator_fetch",
            "gh_gpu_shader_spirv_translator_memexport", "gh_gpu_shader_spirv_translator_rb")], includes)
        if args.reference_source:
            source = work / "reference-source"
            source.mkdir()
            headers = source / "include/rex/graphics/pipeline/shader"
            headers.mkdir(parents=True)
            for path in args.reference_source.glob("*.h"):
                text = path.read_text().replace(
                    "#include <rex/ui/vulkan/device.h>",
                    "namespace rex::ui::vulkan { class VulkanDevice; }")
                (headers / path.name).write_text(text)
            for path in args.reference_source.glob("*.cpp"):
                text = path.read_text()
                if path.stem == "spirv_translator":
                    # Device capability discovery is outside the shader algorithm.
                    begin = text.index("SpirvShaderTranslator::Features::Features(const ui::vulkan::VulkanDevice*")
                    end = text.index("\nuint64_t SpirvShaderTranslator::GetDefaultVertexShaderModification", begin)
                    text = text[:begin] + text[end:]
                (source / path.name).write_text(text)
            expected = run("reference", sorted(source.glob("*.cpp")), [source / "include", *includes])
            files = sorted(path.name for path in actual.iterdir())
            assert files == sorted(path.name for path in expected.iterdir())
            for name in files:
                assert (actual / name).read_bytes() == (expected / name).read_bytes(), name
            print(f"Shader migration: {len(files)} SPIR-V/disassembly outputs match byte-for-byte")


if __name__ == "__main__":
    main()
