#!/usr/bin/env python3
"""Build and run the project-owned controller assignment tests."""

from pathlib import Path
import subprocess
import tempfile


def main():
    project = Path(__file__).resolve().parents[1]
    repository = project.parent
    sdk_include = repository / "rexglue-sdk-main/include"
    with tempfile.TemporaryDirectory(prefix="guardian-heroes-controller-") as directory:
        core = Path(directory) / "gh_controller_assignment_test"
        subprocess.run([
            "clang++", "-std=c++23", "-Wall", "-Wextra", "-Werror",
            "-I", str(project / "src"),
            str(project / "src/gh_controller_assignment.cpp"),
            str(project / "tests/gh_controller_assignment_test.cpp"),
            "-o", str(core),
        ], check=True)
        subprocess.run([str(core)], check=True)

        adapter = Path(directory) / "gh_controller_assignment_adapter_test"
        subprocess.run([
            "clang++", "-std=c++23", "-Wall", "-Wextra", "-Werror",
            "-I", str(project / "src"), "-I", str(sdk_include),
            str(project / "src/gh_controller_assignment.cpp"),
            str(project / "src/gh_controller_assignment_adapter.cpp"),
            str(project / "tests/gh_controller_assignment_adapter_test.cpp"),
            "-o", str(adapter),
        ], check=True)
        subprocess.run([str(adapter)], check=True)


if __name__ == "__main__":
    main()
