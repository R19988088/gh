#!/usr/bin/env python3
"""Build and check native backing/view ownership without linking ReXGlue."""

from pathlib import Path
import platform
import subprocess
import tempfile


def main():
    project = Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix="guardian-heroes-arena-") as directory:
        executable = Path(directory) / "gh_guest_arena_test"
        command = [
            "clang++", "-std=c++23", "-Wall", "-Wextra", "-Werror", "-pthread",
            "-I", str(project / "src"),
            str(project / "src/gh_guest_arena.cpp"),
            str(project / "tests/gh_guest_arena_test.cpp"),
            "-o", str(executable),
        ]
        if platform.system() == "Linux":
            command.append("-lrt")
        subprocess.run(command, check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
