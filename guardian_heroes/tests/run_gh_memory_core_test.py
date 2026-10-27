#!/usr/bin/env python3
"""Compile project page heaps and MMIO without ReX headers or libraries."""
from pathlib import Path
import subprocess
import tempfile

project = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="gh-memory-core-") as directory:
    executable = Path(directory) / "gh_memory_core_test"
    subprocess.run([
        "clang++", "-std=c++23", "-Wall", "-Wextra", "-Werror", "-pthread",
        "-I", str(project / "src"),
        str(project / "src/gh_heap.cpp"), str(project / "src/gh_mmio.cpp"),
        str(project / "tests/gh_memory_core_test.cpp"), "-o", str(executable),
    ], check=True, timeout=60)
    subprocess.run([str(executable)], check=True, timeout=10)
