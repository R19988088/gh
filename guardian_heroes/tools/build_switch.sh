#!/usr/bin/env bash
set -euo pipefail

# Build one explicit Switch target. The game target is opt-in and fails during
# configure when a real runtime, renderer, FFmpeg, or shader dependency is absent.
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
local_env="$project_dir/../toolchains/switch-local/env.sh"
if [[ ! -f "$local_env" ]]; then
    local_env="$project_dir/../toolchains/switch/env.sh"
fi
if [[ -f "$local_env" ]]; then
    source "$local_env"
fi
if [[ "${REXGLUE_SWITCH_SDK_HOST_INCOMPATIBLE:-0}" == 1 ]]; then
    cat >&2 <<'EOF'
The local devkitPro SDK is Linux ARM64 ELF and cannot execute on macOS.
Run this build inside an ARM64 Linux/container environment with the same
project-local toolchain mounted at toolchains/switch-local.
EOF
    exit 1
fi
target="${1:-guardian_heroes_switch_bootstrap_nro}"
if (( $# > 1 )); then
    echo "Usage: $0 [guardian_heroes_switch_{bootstrap,memory_probe,vulkan_probe,game}_nro|guardian_heroes_switch_guest_abi_probe]" >&2
    exit 2
fi
case "$target" in
    guardian_heroes_switch_bootstrap_nro|guardian_heroes_switch_memory_probe_nro|guardian_heroes_switch_vulkan_probe_nro|guardian_heroes_switch_game_nro|guardian_heroes_switch_guest_abi_probe) ;;
    *) echo "Unsupported Switch probe target: $target" >&2; exit 2 ;;
esac
if [[ "$target" == guardian_heroes_switch_vulkan_probe_nro && -z "${REXGLUE_SWITCH_NVK_SDK:-}" ]]; then
    echo "Set REXGLUE_SWITCH_NVK_SDK to the Horizon Mesa/NVK SDK before building the Vulkan probe." >&2
    exit 1
fi
game_flag=OFF
if [[ "$target" == guardian_heroes_switch_game_nro ]]; then
    game_flag=ON
    : "${REXGLUE_SWITCH_FFMPEG:?Set REXGLUE_SWITCH_FFMPEG to the target FFmpeg SDK for the game NRO}"
    : "${REXGLUE_SWITCH_NVK_SDK:?Set REXGLUE_SWITCH_NVK_SDK to the Horizon GPU SDK for the game NRO}"
fi
cmake --preset switch-aarch64-release \
    -DGH_SWITCH_BUILD_GAME="$game_flag" \
    "-DREXGLUE_SWITCH_NVK_SDK=${REXGLUE_SWITCH_NVK_SDK:-}"
cmake --build "$project_dir/out/build/switch-aarch64-release" --target "$target" --parallel "${CMAKE_BUILD_PARALLEL_LEVEL:-2}"

build_dir="$project_dir/out/build/switch-aarch64-release"
nro="$build_dir/${target%_nro}.nro"
if [[ -f "$nro" && -f "$build_dir/${target%_nro}.nacp" ]]; then
    version="$(sed -n 's/^set(GH_SWITCH_APP_VERSION "\(.*\)")/\1/p' "$project_dir/switch/metadata.cmake" | head -1)"
    python3 "$project_dir/tools/nro_nacp.py" "$nro" "${version:-0.1.0}"

    # Keep the S0 inputs and produced payload tied to the same build directory.
    # This is deliberately plain text so it can be attached with the NRO.
    sha256_file() {
        if command -v sha256sum >/dev/null 2>&1; then
            sha256sum "$1" | awk '{print $1}'
        else
            shasum -a 256 "$1" | awk '{print $1}'
        fi
    }
    manifest="$build_dir/${target%_nro}.build-manifest.txt"
    {
        printf 'target=%s\n' "$target"
        printf 'app_version=%s\n' "${version:-0.1.0}"
        printf 'xex_sha256=%s\n' "$(sha256_file "$project_dir/assets/default.xex")"
        printf 'project_manifest_sha256=%s\n' "$(sha256_file "$project_dir/guardian_heroes_manifest.toml")"
        printf 'codegen_descriptor_sha256=%s\n' "$(sha256_file "$project_dir/generated/default/codegen.d")"
        printf 'nro_sha256=%s\n' "$(sha256_file "$nro")"
        printf 'nacp_sha256=%s\n' "$(sha256_file "$build_dir/${target%_nro}.nacp")"
        printf 'switch_nvk_sdk=%s\n' "${REXGLUE_SWITCH_NVK_SDK:-}"
        printf 'switch_ffmpeg=%s\n' "${REXGLUE_SWITCH_FFMPEG:-}"
    } > "$manifest"
fi

if [[ "$target" == guardian_heroes_switch_game_nro ]]; then
    stage="$build_dir/stage"
    shader_dir="${GH_SHADER_DUMPS:-$project_dir/out/shaders_trace}"
    rm -rf "$stage"
    mkdir -p "$stage/shaders"
    command -v uam >/dev/null 2>&1 || { echo "uam is required to build the Switch presentation shaders" >&2; exit 1; }
    for stage_name in vert frag; do
        shader_source="$project_dir/src/gpu/shaders/present.$stage_name"
        [[ -f "$shader_source" ]] || { echo "missing presentation shader: $shader_source" >&2; exit 1; }
        uam -s "$stage_name" -o "$stage/shaders/present.$stage_name.dksh" "$shader_source"
        [[ -s "$stage/shaders/present.$stage_name.dksh" ]] || { echo "uam did not produce $stage/shaders/present.$stage_name.dksh" >&2; exit 1; }
    done
    if [[ -d "$shader_dir" ]]; then
        python3 "$project_dir/tools/xenos2glsl.py" --all "$shader_dir" --out-dir "$build_dir/glsl" --check
        for shader in "$build_dir"/glsl/*.glsl.dksh; do
            [[ -f "$shader" ]] || continue
            cp "$shader" "$stage/shaders/$(basename "$shader" .glsl.dksh).dksh"
        done
    fi
    cp "$nro" "$stage/$(basename "$nro")"
    : > "$stage/deko3d"
fi
