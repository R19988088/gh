#!/usr/bin/env python3
"""Build and run the standalone project graphics-interface contract test."""

from pathlib import Path
import subprocess
import tempfile


def main():
    project = Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix="guardian-heroes-graphics-") as directory:
        executable = Path(directory) / "gh_graphics_test"
        subprocess.run([
            "clang++", "-std=c++23", "-Wall", "-Wextra", "-Werror",
            "-I", str(project / "src"),
            str(project / "src/gh_graphics.cpp"),
            str(project / "tests/gh_graphics_test.cpp"),
            "-o", str(executable),
        ], check=True)
        subprocess.run([str(executable)], check=True)
        commands_executable = Path(directory) / "gh_gpu_commands_test"
        subprocess.run([
            "clang++", "-std=c++23", "-Wall", "-Wextra", "-Werror",
            "-I", str(project / "src"),
            str(project / "src/gh_gpu_commands.cpp"),
            str(project / "tests/gh_gpu_commands_test.cpp"),
            "-o", str(commands_executable),
        ], check=True)
        subprocess.run([str(commands_executable)], check=True)


if __name__ == "__main__":
    main()
