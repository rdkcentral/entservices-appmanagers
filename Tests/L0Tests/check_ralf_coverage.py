#!/usr/bin/env python3
"""Run RuntimeManager L0 tests and enforce 80% line coverage per Ralf file."""

import argparse
import json
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", required=True, type=Path)
    parser.add_argument("--source-dir", required=True, type=Path)
    parser.add_argument("--test-binary", required=True, type=Path)
    parser.add_argument("--gcov", default="gcov")
    args = parser.parse_args()
    source_dir = args.source_dir.resolve()
    sources = sorted(source_dir.glob("*.cpp"))
    if not sources:
        parser.error(f"No Ralf C++ sources found in {source_dir}")
    object_dir = args.build_dir / "CMakeFiles/runtimemanager_l0test.dir"
    notes = sorted(object_dir.rglob("*.gcno"))
    if not notes:
        parser.error("No coverage notes found; configure APPMGR_ENABLE_COVERAGE=ON")

    for data in object_dir.rglob("*.gcda"):
        data.unlink()
    subprocess.run([str(args.test_binary.resolve())], check=True)

    coverage = {}
    for note in notes:
        result = subprocess.run(
            [args.gcov, "--json-format", "--stdout", str(note.resolve())],
            check=True, capture_output=True, text=True,
        )
        if result.stderr:
            print(result.stderr, file=sys.stderr, end="")
            return 1
        report = json.loads(result.stdout)
        for entry in report["files"]:
            path = Path(entry["file"]).resolve()
            if path.parent != source_dir:
                continue
            lines = coverage.setdefault(path, {})
            for line in entry["lines"]:
                number = line["line_number"]
                lines[number] = lines.get(number, False) or line["count"] > 0

    failed = False
    for path in sources:
        if not coverage.get(path):
            print(f"FAIL: no executable coverage data for {path.name}", file=sys.stderr)
            failed = True
    for path, lines in sorted(coverage.items()):
        if not lines:
            continue
        hit = sum(lines.values())
        passed = hit * 100 >= 80 * len(lines)
        print(f"{path.name}: {hit}/{len(lines)} lines ({100 * hit / len(lines):.2f}%)"
              f" {'PASS' if passed else 'FAIL (minimum 80%)'}")
        failed |= not passed
    return int(failed)


if __name__ == "__main__":
    sys.exit(main())
