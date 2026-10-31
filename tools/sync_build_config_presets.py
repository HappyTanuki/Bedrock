#!/usr/bin/env python3
"""Synchronize Common's generated preset fragments from Bedrock's canonical copies.

A component that owns a fragment lists it in .cmake/presets/sync-exclude.json:

    {"version": 1, "exclude": [{"file": "windows.json", "reason": "..."}]}

Excluded fragments are never reported as drift or overwritten; every run
prints them with their reason. The other fragments are still compared byte for
byte, and CMakePresets.json must include every fragment, excluded ones as well.

Exit codes: 0 synchronized or written; 1 drift (--check only), a missing
canonical or excluded fragment, or a fragment CMakePresets.json does not
include; 2 an invalid sync-exclude.json (nothing is compared or written).
"""
from __future__ import annotations

import argparse
import difflib
import json
import pathlib
import posixpath
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "cmake" / "_BedrockBuildConfig" / "presets"
NAMES = ("common.json", "windows.json", "linux.json")
FRAGMENT_DIR = ".cmake/presets"
EXCLUDE_NAME = "sync-exclude.json"
EXCLUDE_VERSION = 1


class ExcludeError(Exception):
    """The component's sync-exclude.json is malformed or names an unknown fragment."""


def load_exclusions(path: pathlib.Path) -> dict[str, str]:
    """Return {fragment name: reason}; a component without the file excludes nothing."""
    if not path.exists():
        return {}
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise ExcludeError(f"{path}: cannot read JSON: {exc}") from exc
    if not isinstance(data, dict) or set(data) != {"version", "exclude"}:
        raise ExcludeError(f'{path}: expected an object with exactly the keys "version" and "exclude"')
    version = data["version"]
    if isinstance(version, bool) or version != EXCLUDE_VERSION:
        raise ExcludeError(f"{path}: unsupported version {version!r} (expected {EXCLUDE_VERSION})")
    if not isinstance(data["exclude"], list):
        raise ExcludeError(f'{path}: "exclude" must be a list of {{"file", "reason"}} objects')
    exclusions: dict[str, str] = {}
    for index, entry in enumerate(data["exclude"]):
        where = f"{path}: exclude[{index}]"
        if not isinstance(entry, dict) or set(entry) != {"file", "reason"}:
            raise ExcludeError(f'{where}: expected an object with exactly the keys "file" and "reason"')
        name, reason = entry["file"], entry["reason"]
        if not isinstance(name, str) or name not in NAMES:
            raise ExcludeError(f"{where}: unknown preset fragment {name!r} (expected one of {', '.join(NAMES)})")
        if not isinstance(reason, str) or not reason.strip():
            raise ExcludeError(f"{where}: {name} needs a non-empty reason")
        if name in exclusions:
            raise ExcludeError(f"{where}: {name} is listed more than once")
        exclusions[name] = reason.strip()
    return exclusions


def include_problems(root: pathlib.Path) -> list[str]:
    """Report fragments that the component's CMakePresets.json does not include."""
    path = root / "CMakePresets.json"
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        return [f"cannot read {path}: {exc}"]
    includes = data.get("include", []) if isinstance(data, dict) else None
    if not isinstance(includes, list):
        return [f'{path}: "include" must be a list']
    included = {posixpath.normpath(item.replace("\\", "/")) for item in includes if isinstance(item, str)}
    return [f"{path} does not include {FRAGMENT_DIR}/{name}"
            for name in NAMES if f"{FRAGMENT_DIR}/{name}" not in included]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--destination", required=True, type=pathlib.Path,
                        help="Common repository root receiving generated .cmake/presets files")
    parser.add_argument("--check", action="store_true", help="report drift without writing")
    args = parser.parse_args()
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(errors="backslashreplace")
    destination = args.destination / FRAGMENT_DIR
    try:
        exclusions = load_exclusions(destination / EXCLUDE_NAME)
    except ExcludeError as exc:
        print(f"invalid exclusion list: {exc}", file=sys.stderr)
        return 2
    missing = False
    drift = False
    for name in NAMES:
        source = CANONICAL / name
        target = destination / name
        if name in exclusions:
            print(f"skipped {target} (excluded by {EXCLUDE_NAME}): {exclusions[name]}")
            if not target.is_file():
                print(f"missing excluded preset: {target}", file=sys.stderr)
                missing = True
            elif source.is_file() and target.read_bytes() == source.read_bytes():
                print(f"note: {target} matches the canonical copy; its exclusion can be removed")
            continue
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
    problems = include_problems(args.destination)
    for problem in problems:
        print(problem, file=sys.stderr)
    if args.check and not (drift or missing or problems):
        excluded = f" ({len(exclusions)} excluded)" if exclusions else ""
        print(f"preset copies are synchronized{excluded}")
    # Missing or unincluded fragments fail both modes; drift fails only --check.
    return 1 if missing or problems or (args.check and drift) else 0


if __name__ == "__main__":
    raise SystemExit(main())
