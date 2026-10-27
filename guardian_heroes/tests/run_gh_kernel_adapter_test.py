#!/usr/bin/env python3
"""Build and exercise the real kernel adapters without launching the game."""

from pathlib import Path
import shutil
import subprocess
import sys


def main():
    project = Path(__file__).resolve().parents[1]
    build = Path(sys.argv[1]) if len(sys.argv) > 1 else project / "out/build/mac-arm64-release"
    cmake = shutil.which("cmake") or "/opt/homebrew/bin/cmake"
    subprocess.run([cmake, "--build", str(build), "--target",
                    "guardian_heroes_kernel_adapter_test", "-j", "6"], check=True)
    executable = build / "guardian_heroes_kernel_adapter_test"
    if sys.platform == "win32":
        executable = executable.with_suffix(".exe")
    subprocess.run([str(executable)], check=True, timeout=15)


if __name__ == "__main__":
    main()
