#!/usr/bin/env python3
"""Build and run the exhaustive pre-extraction Xenos 7e3 differential test."""

from pathlib import Path
import subprocess
import tempfile


def main():
    project = Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix="guardian-heroes-xenos-math-") as directory:
        executable = Path(directory) / "gh_xenos_math_test"
        subprocess.run([
            "clang++", "-std=c++23", "-Wall", "-Wextra", "-Werror",
            "-I", str(project / "src"),
            str(project / "src/gh_xenos_math.cpp"),
            str(project / "tests/gh_xenos_math_test.cpp"),
            "-o", str(executable),
        ], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
