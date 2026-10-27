#!/usr/bin/env python3
"""Build and run focused project clock, exception, and logging service tests."""

from pathlib import Path
import subprocess
import tempfile


def main():
    project = Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix="guardian-heroes-services-") as directory:
        executable = Path(directory) / "gh_services_test"
        subprocess.run([
            "clang++", "-std=c++23", "-Wall", "-Wextra", "-Werror",
            "-I", str(project / "src"),
            str(project / "src/gh_clock.cpp"),
            str(project / "src/gh_exceptions.cpp"),
            str(project / "src/gh_logging.cpp"),
            str(project / "tests/gh_services_test.cpp"),
            "-o", str(executable),
        ], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
