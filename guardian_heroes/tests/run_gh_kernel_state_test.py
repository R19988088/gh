#!/usr/bin/env python3
"""Verify project guest APC/TLS and handle ownership without the runtime."""

from pathlib import Path
import subprocess
import tempfile


def main():
    project = Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix="guardian-heroes-kernel-state-") as directory:
        executable = Path(directory) / "gh_kernel_state_test"
        subprocess.run([
            "clang++", "-std=c++23", "-Wall", "-Wextra", "-Werror", "-pthread",
            "-I", str(project / "src"),
            str(project / "src/gh_kernel_guest.cpp"),
            str(project / "src/gh_kernel_objects.cpp"),
            str(project / "tests/gh_kernel_state_test.cpp"),
            "-o", str(executable),
        ], check=True, timeout=60)
        subprocess.run([str(executable)], check=True, timeout=15)


if __name__ == "__main__":
    main()
