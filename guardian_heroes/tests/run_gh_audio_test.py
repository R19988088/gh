#!/usr/bin/env python3
"""Build and run project-owned PCM decode and mixer queue checks."""

from pathlib import Path
import platform
import subprocess
import tempfile


def main():
    project = Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix="guardian-heroes-audio-") as directory:
        executable = Path(directory) / "gh_audio_test"
        command = [
            "clang++", "-std=c++23", "-Wall", "-Wextra", "-Werror", "-pthread",
            "-I", str(project / "src"),
            str(project / "src/gh_audio.cpp"),
            str(project / "tests/gh_audio_test.cpp"),
            "-o", str(executable),
        ]
        if platform.machine().lower() in ("x86_64", "amd64"):
            command.append("-mssse3")
        subprocess.run(command, check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
