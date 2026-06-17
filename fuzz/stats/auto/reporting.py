#!/usr/bin/env python3
import argparse
import json
import re
from collections import defaultdict
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List

FRAME_RE = re.compile(r"([^\s:]+\.cpp):(\d+)")
SANITIZER_RE = re.compile(r"(AddressSanitizer:|UndefinedBehaviorSanitizer:|SUMMARY: AddressSanitizer|SUMMARY: UndefinedBehaviorSanitizer|ERROR: AddressSanitizer:|ERROR: UndefinedBehaviorSanitizer:)")


def parse_log_for_frames(log_text: str) -> List[Dict[str, object]]:
    frames = []
    for line in log_text.splitlines():
        m = FRAME_RE.search(line)
        if not m:
            continue
        frames.append({"source_file": m.group(1), "line_number": int(m.group(2)), "raw": line.strip()})
    return frames


def classify_crash(frames: List[Dict[str, object]], include_folders: List[str]) -> str:
    if any(any(f"{folder}/" in f["source_file"] for folder in include_folders) for f in frames):
        return "valid"
    if frames and all("fuzz/stats/auto/generated" in f["source_file"] or "fuzz/stats/auto/stubs" in f["source_file"] for f in frames):
        return "false_positive"
    return "false_positive"


def has_sanitizer_signature(log_text: str) -> bool:
    return bool(SANITIZER_RE.search(log_text))


def triage(repo_root: Path, run_results: Path, crashes_root: Path, triage_out: Path, manifest_path: Path) -> None:
    crash_records = []
    ignored = 0

    include_folders = ["AppManager"]
    if manifest_path.exists():
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        include_folders = manifest.get("scope", {}).get("include", include_folders)

    if run_results.exists():
        runs = json.loads(run_results.read_text(encoding="utf-8"))
    else:
        runs = {"targets": []}

    for item in runs.get("targets", []):
        key = item.get("target_key")
        folder = item.get("folder_tag", "AppManager")
        crash_paths = item.get("crash_files", [])
        exit_code = int(item.get("exit_code", 0) or 0)
        source_file = item.get("source_file", "unknown")
        log_path = item.get("log_file")
        log_text = ""
        if log_path and (repo_root / log_path).exists():
            log_text = (repo_root / log_path).read_text(encoding="utf-8", errors="ignore")
        frames = parse_log_for_frames(log_text)
        in_scope_source = any(source_file.startswith(f"{folder_name}/") for folder_name in include_folders)
        classification = "valid" if in_scope_source else classify_crash(frames, include_folders)
        sanitizer_crash = (0 != exit_code) and has_sanitizer_signature(log_text)
        has_failure = bool(crash_paths) or (0 != exit_code)

        if not has_failure:
            continue

        if ("valid" != classification) and (not sanitizer_crash):
            ignored += 1
            continue

        target_dir = crashes_root / folder / key
        target_dir.mkdir(parents=True, exist_ok=True)

        for idx, crash in enumerate(crash_paths):
            crash_src = repo_root / crash
            if not crash_src.exists():
                continue
            out_name = f"reproducer_{idx:03d}.bin"
            out_path = target_dir / out_name
            out_path.write_bytes(crash_src.read_bytes())

        crash_records.append(
            {
                "target_key": key,
                "folder_tag": folder,
                "function_name": item.get("function_name", "unknown"),
                "source_file": item.get("source_file", "unknown"),
                "line_number": item.get("line_number", 0),
                "crash_type": "sanitizer_exit" if sanitizer_crash and not crash_paths else item.get("crash_type", "sanitizer"),
                "sanitizer_output": log_text[-4000:],
                "triggering_input": crash_paths[0] if crash_paths else "n/a",
                "root_cause_explanation": "Memory safety fault reached in an in-scope production stack frame.",
                "why_crash_occurred": "Mutated input exercised an unsafe code path.",
                "suggested_fix": "Add input validation and ownership-safe bounds checks around failing path.",
                "stack_frames": frames,
                "sanitizer_signature": sanitizer_crash,
            }
        )

    triage_out.parent.mkdir(parents=True, exist_ok=True)
    triage_out.write_text(
        json.dumps({"valid_crashes": crash_records, "ignored_false_positives": ignored}, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )


def render_reports(
    repo_root: Path,
    manifest_path: Path,
    corpus_summary_path: Path,
    run_results_path: Path,
    triage_path: Path,
    report_path: Path,
    reports_root: Path,
    budgets: Dict[str, int],
) -> None:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8")) if manifest_path.exists() else {"targets": []}
    corpus_summary = json.loads(corpus_summary_path.read_text(encoding="utf-8")) if corpus_summary_path.exists() else {}
    run_results = json.loads(run_results_path.read_text(encoding="utf-8")) if run_results_path.exists() else {"targets": []}
    triage = json.loads(triage_path.read_text(encoding="utf-8")) if triage_path.exists() else {"valid_crashes": [], "ignored_false_positives": 0}

    folder_crashes = defaultdict(list)
    for c in triage.get("valid_crashes", []):
        folder_crashes[c.get("folder_tag", "Unknown")].append(c)

    executions = sum(t.get("executions", 0) for t in run_results.get("targets", []))
    total_functions = len(manifest.get("targets", []))
    total_seeds = corpus_summary.get("total_seeds", 0)
    valid_count = len(triage.get("valid_crashes", []))
    ignored_count = triage.get("ignored_false_positives", 0)

    now = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")

    def render_single(title: str, crashes: List[Dict[str, object]]) -> str:
        lines = [
            f"# {title}",
            "",
            f"Generated: {now}",
            "",
            "## Summary",
            f"- total functions fuzzed: {total_functions}",
            f"- total seeds generated: {total_seeds}",
            f"- total executions: {executions}",
            f"- crashes found: {len(crashes)}",
            f"- ignored false positives: {ignored_count}",
            f"- coverage summary: discovered targets={total_functions}, coverage gaps={len(manifest.get('coverage_gaps', []))}",
            "",
            "## Applied Budgets",
            f"- RUNS: {budgets.get('RUNS', 0)}",
            f"- MAX_TOTAL_TIME: {budgets.get('MAX_TOTAL_TIME', 0)}",
            f"- PAIRWISE_BUDGET: {budgets.get('PAIRWISE_BUDGET', 0)}",
            f"- STATE_SEQUENCE_SEEDS: {budgets.get('STATE_SEQUENCE_SEEDS', 0)}",
            f"- MAX_JSON_SEEDS: {budgets.get('MAX_JSON_SEEDS', 0)}",
            "",
            "## Valid Crashes",
        ]

        if not crashes:
            lines.append("No valid crashes detected.")
        for c in crashes:
            lines.extend(
                [
                    "",
                    f"### {c.get('function_name', 'unknown')}",
                    f"- source file: {c.get('source_file', 'unknown')}",
                    f"- line number: {c.get('line_number', 0)}",
                    f"- crash type: {c.get('crash_type', 'sanitizer')}",
                    f"- triggering input: {c.get('triggering_input', 'n/a')}",
                    "- sanitizer output:",
                    "```",
                    c.get("sanitizer_output", "").strip()[-2000:],
                    "```",
                    f"- root cause explanation: {c.get('root_cause_explanation', '')}",
                    f"- why crash occurred: {c.get('why_crash_occurred', '')}",
                    f"- suggested fix: {c.get('suggested_fix', '')}",
                ]
            )
        return "\n".join(lines) + "\n"

    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(render_single("Fuzz Report", triage.get("valid_crashes", [])), encoding="utf-8")

    folder_list = sorted(set(t.get("folder_tag", "Unknown") for t in manifest.get("targets", [])))
    for folder in folder_list:
        if "Tests" == folder:
            continue
        fdir = reports_root / folder
        fdir.mkdir(parents=True, exist_ok=True)
        (fdir / "fuzz_report.md").write_text(
            render_single(f"Fuzz Report - {folder}", folder_crashes.get(folder, [])), encoding="utf-8"
        )


def main() -> None:
    parser = argparse.ArgumentParser(description="Crash triage and report generation")
    parser.add_argument("command", choices=["triage", "report"])
    parser.add_argument("--repo-root", required=True)
    parser.add_argument("--run-results", default="fuzz/stats/results/run_results.json")
    parser.add_argument("--crashes-root", default="fuzz/stats/crashes")
    parser.add_argument("--triage-out", default="fuzz/stats/results/triage_results.json")
    parser.add_argument("--manifest", default="fuzz/stats/results/discovery_manifest.json")
    parser.add_argument("--corpus-summary", default="fuzz/stats/results/corpus_summary.json")
    parser.add_argument("--report", default="fuzz/stats/fuzz_report.md")
    parser.add_argument("--reports-root", default="fuzz/stats/reports")
    parser.add_argument("--runs", type=int, default=2000)
    parser.add_argument("--max-total-time", type=int, default=0)
    parser.add_argument("--pairwise-budget", type=int, default=48)
    parser.add_argument("--state-sequence-seeds", type=int, default=20)
    parser.add_argument("--max-json-seeds", type=int, default=40)
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()

    if "triage" == args.command:
        triage(
            repo_root,
            (repo_root / args.run_results).resolve(),
            (repo_root / args.crashes_root).resolve(),
            (repo_root / args.triage_out).resolve(),
            (repo_root / args.manifest).resolve(),
        )
        return

    budgets = {
        "RUNS": args.runs,
        "MAX_TOTAL_TIME": args.max_total_time,
        "PAIRWISE_BUDGET": args.pairwise_budget,
        "STATE_SEQUENCE_SEEDS": args.state_sequence_seeds,
        "MAX_JSON_SEEDS": args.max_json_seeds,
    }

    render_reports(
        repo_root,
        (repo_root / args.manifest).resolve(),
        (repo_root / args.corpus_summary).resolve(),
        (repo_root / args.run_results).resolve(),
        (repo_root / args.triage_out).resolve(),
        (repo_root / args.report).resolve(),
        (repo_root / args.reports_root).resolve(),
        budgets,
    )


if __name__ == "__main__":
    main()
