#!/usr/bin/env python3
import argparse
import json
from pathlib import Path


def require_keys(obj, keys, ctx):
    for key in keys:
        if key not in obj:
            raise ValueError(f"Missing key '{key}' in {ctx}")


def validate_manifest(path: Path) -> None:
    data = json.loads(path.read_text(encoding="utf-8"))
    require_keys(data, ["targets", "coverage_gaps", "scope"], "discovery manifest")
    for idx, target in enumerate(data["targets"]):
        require_keys(
            target,
            [
                "function_name",
                "signature",
                "arguments",
                "source_file",
                "line_number",
                "folder_tag",
                "classification_tags",
                "json_touching",
                "stable_target_key",
            ],
            f"target[{idx}]",
        )


def validate_report(path: Path) -> None:
    text = path.read_text(encoding="utf-8")
    for token in [
        "total functions fuzzed",
        "total seeds generated",
        "total executions",
        "crashes found",
        "ignored false positives",
        "coverage summary",
    ]:
        if token not in text:
            raise ValueError(f"Report missing summary token: {token}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Validate fuzz output schemas")
    parser.add_argument("--repo-root", required=True)
    parser.add_argument("--manifest", default="fuzz/stats/results/discovery_manifest.json")
    parser.add_argument("--report", default="fuzz/stats/fuzz_report.md")
    args = parser.parse_args()

    repo = Path(args.repo_root).resolve()
    validate_manifest((repo / args.manifest).resolve())
    validate_report((repo / args.report).resolve())
    print("Output validation passed")


if __name__ == "__main__":
    main()
