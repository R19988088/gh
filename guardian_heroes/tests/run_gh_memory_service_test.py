#!/usr/bin/env python3
"""Compile and exercise actual project guest memory without ReX dependencies."""
from pathlib import Path
import platform
import subprocess
import tempfile

project = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="gh-memory-service-") as directory:
    executable = Path(directory) / "gh_memory_service_test"
    command = [
        "clang++", "-std=c++23", "-Wall", "-Wextra", "-Werror", "-pthread",
        "-I", str(project / "src"),
        str(project / "src/gh_heap.cpp"), str(project / "src/gh_guest_arena.cpp"),
        str(project / "src/gh_host_memory_posix.cpp"),
        str(project / "src/gh_memory_service.cpp"),
        str(project / "tests/gh_memory_service_test.cpp"), "-o", str(executable),
    ]
    if platform.system() == "Linux":
        command += ["-ldl", "-lrt"]
    subprocess.run(command, check=True, timeout=60)
    subprocess.run([str(executable)], check=True, timeout=30)
