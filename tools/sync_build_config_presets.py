#!/usr/bin/env python3
"""Synchronize Common's generated preset fragments from Bedrock's canonical copies."""
from __future__ import annotations

import argparse
import difflib
import pathlib
import shutil
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "cmake" / "_BedrockBuildConfig" / "presets"
NAMES = ("common.json", "windows.json", "linux.json")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--destination", required=True, type=pathlib.Path,
                        help="Common repository root receiving generated .cmake/presets files")
    parser.add_argument("--check", action="store_true", help="report drift without writing")
    args = parser.parse_args()
    destination = args.destination / ".cmake" / "presets"
    missing = False
    drift = False
    for name in NAMES:
        source = CANONICAL / name
        target = destination / name
        if not source.is_file():
            print(f"missing canonical preset: {source}", file=sys.stderr)
            missing = True
            continue
        expected = source.read_bytes()
        actual = target.read_bytes() if target.is_file() else b""
        if actual == expected:
            continue
        drift = True
        if args.check:
            print(f"preset drift: {target}")
            try:
                old = actual.decode("utf-8").splitlines(keepends=True)
                new = expected.decode("utf-8").splitlines(keepends=True)
                sys.stdout.writelines(difflib.unified_diff(old, new, fromfile=str(target), tofile=str(source)))
            except UnicodeDecodeError:
                pass
        else:
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(expected)
            print(f"updated {target}")
    if args.check and not (drift or missing):
        print("preset copies are synchronized")
    # A missing canonical fragment is an error in both modes; drift fails only --check.
    return 1 if missing or (args.check and drift) else 0


if __name__ == "__main__":
    raise SystemExit(main())
