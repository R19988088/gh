#!/usr/bin/env python3
"""Verify XEX/PE/import parsing independently from the SDK and guest runtime."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
project = root / "guardian_heroes"
sdk = root / "rexglue-sdk-main"
mspack = sdk / "thirdparty/libmspack/cabextract/mspack"
with tempfile.TemporaryDirectory(prefix="gh-image-metadata-") as temporary:
    obj = Path(temporary) / "lzxd.o"
    binary = Path(temporary) / "metadata-test"
    subprocess.run(["clang", "-O2", "-w", "-I" + str(mspack),
                    "-c", str(mspack / "lzxd.c"), "-o", str(obj)], check=True)
    subprocess.run(["clang++", "-std=c++23", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-I" + str(project / "src"), "-I" + str(sdk / "thirdparty"),
                    "-I" + str(mspack), str(project / "src/gh_image.cpp"),
                    str(project / "src/gh_image_metadata.cpp"),
                    str(project / "tests/gh_image_metadata_test.cpp"), str(obj),
                    "-o", str(binary)], check=True)
    subprocess.run([str(binary), str(project / "assets/default.xex")], check=True)
