#!/usr/bin/env python3
"""Standalone XEX payload differential and length checks without ReX linkage."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
project = root / "guardian_heroes"
sdk = root / "rexglue-sdk-main"
mspack = sdk / "thirdparty/libmspack/cabextract/mspack"
with tempfile.TemporaryDirectory(prefix="gh-image-test-") as temporary:
    obj = Path(temporary) / "lzxd.o"
    binary = Path(temporary) / "image-test"
    subprocess.run(["clang", "-O2", "-w", "-I" + str(mspack),
                    "-I" + str(mspack.parent), "-c", str(mspack / "lzxd.c"),
                    "-o", str(obj)], check=True)
    subprocess.run(["clang++", "-std=c++23", "-O2", "-I" + str(project / "src"),
                    "-I" + str(sdk / "thirdparty"), "-I" + str(mspack),
                    str(project / "src/gh_image.cpp"),
                    str(project / "tests/gh_image_test.cpp"), str(obj),
                    "-o", str(binary)], check=True)
    subprocess.run([str(binary), str(project / "assets/default.xex")], check=True)
