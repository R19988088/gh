#!/usr/bin/env python3
"""Exercise SDL device lifecycle and XMA decoding without linking the runtime."""

from pathlib import Path
import argparse
import platform
import subprocess
import sys
import tempfile


def main():
    project = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser()
    parser.add_argument("libraries", type=Path, nargs="?",
                        default=project / "out/native-app/runtime/thirdparty")
    parser.add_argument("--system-output", action="store_true",
                        help="Exercise the real output device using silent frames")
    options = parser.parse_args()
    libraries = options.libraries
    sdl_library = libraries / "sdl3/libSDL3.a"
    if not sdl_library.is_file():
        sdl_library = libraries / "libSDL3.a"
    with tempfile.TemporaryDirectory(prefix="guardian-heroes-audio-output-") as directory:
        executable = Path(directory) / "gh_audio_output_test"
        command = [
            "clang++", "-std=c++23", "-Wall", "-Wextra", "-Werror", "-pthread",
            "-I", str(project / "src"),
            "-I", str(project / "runtime/thirdparty/sdl3/include"),
            "-I", str(project / "runtime/thirdparty/FFmpeg"),
            str(project / "src/gh_audio.cpp"),
            str(project / "src/gh_audio_sdl.cpp"),
            str(project / "src/gh_audio_xma.cpp"),
            str(project / "tests/gh_audio_output_test.cpp"),
            str(sdl_library),
            str(libraries / "liblibavcodec.a"),
            str(libraries / "liblibavutil.a"),
            "-lm", "-o", str(executable),
        ]
        if platform.machine().lower() in ("x86_64", "amd64"):
            command.append("-mssse3")
        if sys.platform == "darwin":
            for framework in ["CoreMedia", "CoreVideo", "Cocoa", "UniformTypeIdentifiers",
                              "IOKit", "ForceFeedback", "Carbon", "CoreAudio",
                              "AudioToolbox", "AVFoundation", "Foundation", "GameController",
                              "Metal", "QuartzCore", "CoreHaptics"]:
                command.extend(["-framework", framework])
        subprocess.run(command, check=True)
        command = [str(executable), str(project / "assets/media/sound/Wave Bank_BGM.xwb")]
        if options.system_output:
            command.append("--system-output")
        subprocess.run(command, check=True, timeout=15)


if __name__ == "__main__":
    main()
