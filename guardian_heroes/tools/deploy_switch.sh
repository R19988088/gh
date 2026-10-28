#!/usr/bin/env bash
set -euo pipefail

# Deploy a validated NRO tree through the Switch FTP server. Uploading under a
# temporary name prevents the running title from observing a partial file.
project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${SWITCH_BUILD_DIR:-$project_dir/out/build/switch-aarch64-release}"
remote_root="${SWITCH_REMOTE_ROOT:-/switch/guardian-heroes}"
port="${SWITCH_FTP_PORT:-5000}"
ip="${1:-}"
action="${2:-push}"

if [[ -z "$ip" ]]; then
    echo "Usage: $0 SWITCH_IP [push|logs]" >&2
    exit 2
fi

nro="${SWITCH_NRO:-$build_dir/guardian_heroes_switch_game.nro}"
stage="${SWITCH_STAGE_DIR:-$build_dir/stage}"
if [[ "$action" == push ]]; then
    [[ -f "$nro" ]] || { echo "missing NRO: $nro" >&2; exit 1; }
    [[ -s "$stage/shaders/present.vert.dksh" ]] || { echo "missing staged vertex presentation shader" >&2; exit 1; }
    [[ -s "$stage/shaders/present.frag.dksh" ]] || { echo "missing staged fragment presentation shader" >&2; exit 1; }
    python3 "$project_dir/tools/nro_nacp.py" "$nro"
    tmp="$(mktemp -t guardian-heroes-nro.XXXXXX)"
    trap 'rm -f "$tmp"' EXIT
    sha_cmd="shasum -a 256"
    command -v shasum >/dev/null 2>&1 || sha_cmd="sha256sum"
    local_sha="$($sha_cmd "$nro" | awk '{print $1}')"
    curl --fail --silent --show-error --ftp-create-dirs \
        --upload-file "$nro" "ftp://$ip:$port$remote_root/guardian_heroes_switch_game.nro.tmp"
    curl --fail --silent --show-error \
        "ftp://$ip:$port$remote_root/guardian_heroes_switch_game.nro.tmp" -o "$tmp"
    remote_sha="$($sha_cmd "$tmp" | awk '{print $1}')"
    [[ "$local_sha" == "$remote_sha" ]] || { echo "NRO SHA-256 mismatch" >&2; exit 1; }
    curl --fail --silent --show-error "ftp://$ip:$port/" \
        -Q "-DELE $remote_root/guardian_heroes_switch_game.nro" || true
    curl --fail --silent --show-error "ftp://$ip:$port/" \
        -Q "-RNFR $remote_root/guardian_heroes_switch_game.nro.tmp" \
        -Q "-RNTO $remote_root/guardian_heroes_switch_game.nro"
    for shader in "$stage"/shaders/*.dksh; do
        [[ -f "$shader" ]] || continue
        curl --fail --silent --show-error --ftp-create-dirs \
            --upload-file "$shader" "ftp://$ip:$port$remote_root/shaders/$(basename "$shader")"
    done
    curl --fail --silent --show-error --ftp-create-dirs \
        --upload-file "$stage/deko3d" "ftp://$ip:$port$remote_root/deko3d" || true
    echo "deployed verified NRO $local_sha"
elif [[ "$action" == logs ]]; then
    mkdir -p "$project_dir/../logs/switch"
    for log in boot.log rex_crash.log rex_assert.log user/guardian_heroes.log; do
        local_log="${log//\//_}"
        curl --fail --silent --show-error \
            "ftp://$ip:$port$remote_root/$log" -o "$project_dir/../logs/switch/$local_log" || true
    done
else
    echo "unknown action: $action (push or logs)" >&2
    exit 2
fi
