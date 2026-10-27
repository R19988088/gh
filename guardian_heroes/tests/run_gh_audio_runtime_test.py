#!/usr/bin/env python3
"""Check guest XMA state and audio scheduling without the SDK or output device."""
from pathlib import Path
import argparse
import platform
import subprocess
import tempfile


def main():
    project = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=project / "out/native-app")
    options = parser.parse_args()
    libraries = options.build_dir / "runtime/thirdparty"
    with tempfile.TemporaryDirectory(prefix="guardian-heroes-audio-runtime-") as directory:
        executable = Path(directory) / "gh_audio_runtime_test"
        command = ["clang++", "-std=c++23", "-Wall", "-Wextra", "-Werror", "-pthread",
                   "-I", str(project / "src"), "-I", str(project / "runtime/thirdparty/FFmpeg")]
        for source in ["gh_audio_context", "gh_audio_runtime", "gh_audio_xma", "gh_xma_packet", "gh_kernel"]:
            command.append(str(project / "src" / (source + ".cpp")))
        command.extend([str(project / "tests/gh_audio_runtime_test.cpp"),
                        str(libraries / "liblibavcodec.a"),
                        str(libraries / "liblibavutil.a"), "-lm", "-o", str(executable)])
        if platform.machine().lower() in ("x86_64", "amd64"):
            command.append("-mssse3")
        subprocess.run(command, check=True)
        subprocess.run([str(executable), str(project / "assets/media/sound/Wave Bank_BGM.xwb")],
                       check=True, timeout=20)

if __name__ == "__main__":
    main()
