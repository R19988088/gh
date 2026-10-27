#!/usr/bin/env python3
"""Check that codegen's build edge is separate from the runtime edge.

The generated source files are owned by ReXGlue, but the project can use a
prebuilt host codegen executable.  This check inspects Ninja's authoritative
dependency graph instead of inferring edges from CMake source text.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys


def query_stamp(build_dir: Path, ninja: str, stamp: Path) -> list[str]:
    result = subprocess.run(
        [ninja, "-C", str(build_dir), "-t", "query", str(stamp)],
        check=True,
        capture_output=True,
        text=True,
    )
    dependencies: list[str] = []
    in_inputs = False
    for line in result.stdout.splitlines():
        if line == "  input: CUSTOM_COMMAND":
            in_inputs = True
            continue
        if in_inputs and line == "  outputs:":
            break
        if in_inputs and line.startswith("    || "):
            dependencies.append(line.removeprefix("    || "))
    return dependencies


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("build_dir", type=Path)
    parser.add_argument("--expect-offline", action="store_true")
    parser.add_argument("--expect-sdk", action="store_true")
    args = parser.parse_args()
    if args.expect_offline == args.expect_sdk:
        parser.error("choose exactly one of --expect-offline or --expect-sdk")

    build_dir = args.build_dir.resolve()
    stamp = Path(__file__).resolve().parents[1] / "generated/default/codegen.build.stamp"
    ninja = shutil.which("ninja") or "/opt/homebrew/bin/ninja"
    dependencies = query_stamp(build_dir, ninja, stamp)
    names = {Path(path).name for path in dependencies}
    runtime_edges = sorted(
        path for path in dependencies
        if Path(path).name == "rexglue"
        or "rexruntime" in Path(path).name
        or "rexcodegen" in Path(path).name
    )

    if args.expect_offline and runtime_edges:
        print("offline codegen edge still depends on SDK runtime:", file=sys.stderr)
        print("\n".join(runtime_edges), file=sys.stderr)
        return 1
    if args.expect_sdk and not ({"rexglue"} <= names):
        print("SDK codegen edge is missing the rexglue tool dependency", file=sys.stderr)
        return 1
    print(f"codegen dependencies: {len(dependencies)}")
    print(f"runtime edge dependencies: {len(runtime_edges)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
