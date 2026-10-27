#!/usr/bin/env python3
"""Exercise the runtime POSIX signal bridge in isolated child processes."""
from pathlib import Path
import subprocess
import tempfile

project = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="gh-posix-exception-") as directory:
    executable = Path(directory) / "gh_posix_exception_test"
    subprocess.run([
        "clang++", "-std=c++23", "-Wall", "-Wextra", "-Werror",
        "-I", str(project / "runtime/include"),
        "-I", str(project / "runtime/thirdparty/fmt/include"),
        "-I", str(project / "runtime/thirdparty/spdlog/include"),
        str(project / "runtime/src/core/exception_handler.cpp"),
        str(project / "runtime/src/core/exception_handler_posix.cpp"),
        str(project / "runtime/src/core/math_gcc.cpp"),
        str(project / "tests/gh_posix_exception_test.cpp"), "-o", str(executable),
    ], check=True, timeout=60)
    subprocess.run([str(executable)], check=True, timeout=15)
