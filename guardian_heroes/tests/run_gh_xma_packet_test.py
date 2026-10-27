#!/usr/bin/env python3
"""Build and run exhaustive differential checks for the XMA header helpers."""

from pathlib import Path
import subprocess
import tempfile


def main():
    project = Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix="guardian-heroes-xma-packet-") as directory:
        executable = Path(directory) / "gh_xma_packet_test"
        subprocess.run([
            "clang++", "-std=c++23", "-Wall", "-Wextra", "-Werror",
            "-I", str(project / "src"),
            str(project / "src/gh_xma_packet.cpp"),
            str(project / "tests/gh_xma_packet_test.cpp"),
            "-o", str(executable),
        ], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
