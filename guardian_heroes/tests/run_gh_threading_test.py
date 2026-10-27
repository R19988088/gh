#!/usr/bin/env python3
"""Build and run the project-owned guest critical-region test."""

from pathlib import Path
import subprocess
import tempfile


def main():
    project = Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix="guardian-heroes-threading-") as directory:
        executable = Path(directory) / "gh_threading_test"
        subprocess.run([
            "clang++", "-std=c++23", "-Wall", "-Wextra", "-Werror",
            "-I", str(project / "src"),
            str(project / "src/gh_threading.cpp"),
            str(project / "tests/gh_threading_test.cpp"),
            "-o", str(executable),
        ], check=True)
        subprocess.run([str(executable)], check=True)
        unmatched = subprocess.run([str(executable), "unmatched-leave"],
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if unmatched.returncode == 0:
            raise SystemExit("unmatched Leave did not trigger its assertion")

        release_executable = Path(directory) / "gh_threading_test_release"
        subprocess.run([
            "clang++", "-std=c++23", "-DNDEBUG", "-Wall", "-Wextra", "-Werror",
            "-I", str(project / "src"),
            str(project / "src/gh_threading.cpp"),
            str(project / "tests/gh_threading_test.cpp"),
            "-o", str(release_executable),
        ], check=True)
        release_unmatched = subprocess.run([str(release_executable), "unmatched-leave"],
                                           stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if release_unmatched.returncode == 0:
            raise SystemExit("unmatched Leave did not fail in release mode")


if __name__ == "__main__":
    main()
