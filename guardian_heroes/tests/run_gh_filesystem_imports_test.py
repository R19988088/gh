#!/usr/bin/env python3
"""Build and run the project-owned guest filesystem import tests."""

from pathlib import Path
import subprocess
import tempfile


def main():
    project = Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix="guardian-heroes-filesystem-imports-") as directory:
        executable = Path(directory) / "gh_filesystem_imports_test"
        subprocess.run([
            "clang++", "-std=c++23", "-Wall", "-Wextra", "-Werror",
            "-I", str(project / "src"),
            str(project / "src/gh_filesystem_imports.cpp"),
            str(project / "tests/gh_filesystem_imports_test.cpp"),
            "-o", str(executable),
        ], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
