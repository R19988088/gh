#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
target="${1:-guardian_heroes_switch_bootstrap_nro}"
export PATH="/opt/homebrew/bin:/opt/homebrew/opt/lima/bin:${PATH}"

if ! command -v colima >/dev/null 2>&1 || ! command -v docker >/dev/null 2>&1; then
    echo "需要先安装 colima 和 docker（brew install lima docker colima）" >&2
    exit 1
fi

if ! colima status 2>/dev/null | grep -q '^colima is running'; then
    colima start --arch aarch64 --vm-type=vz --mount-type=virtiofs
fi

docker pull devkitpro/devkita64:latest
docker run --rm \
    --platform linux/arm64 \
    -v "$project_dir:/workspace" \
    -w /workspace \
    -e DEVKITPRO=/opt/devkitpro \
    -e DEVKITA64=/opt/devkitpro/devkitA64 \
    -e DEVKITPRO_TOOLS=/opt/devkitpro/tools \
    -e REXGLUE_SWITCH_FFMPEG=/workspace/toolchains/switch-local/ffmpeg-switch \
    devkitpro/devkita64:latest \
    bash -lc "source /opt/devkitpro/switchvars.sh; bash guardian_heroes/tools/build_switch.sh '$target'"
