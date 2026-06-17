#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: run_fuzz.sh --repo-root <path> --build-dir <path> --runs <n> --max-total-time <sec> --run-id <id> [--with-report <0|1>] [--pairwise-budget <n>] [--state-sequence-seeds <n>] [--max-json-seeds <n>]
EOF
}

REPO_ROOT=""
BUILD_DIR=""
RUNS=2000
MAX_TOTAL_TIME=0
RUN_ID="manual"
WITH_REPORT=1
PAIRWISE_BUDGET=48
STATE_SEQUENCE_SEEDS=20
MAX_JSON_SEEDS=40

while [[ $# -gt 0 ]]; do
  case "$1" in
    --repo-root) REPO_ROOT="$2"; shift 2 ;;
    --build-dir) BUILD_DIR="$2"; shift 2 ;;
    --runs) RUNS="$2"; shift 2 ;;
    --max-total-time) MAX_TOTAL_TIME="$2"; shift 2 ;;
    --run-id) RUN_ID="$2"; shift 2 ;;
    --with-report) WITH_REPORT="$2"; shift 2 ;;
    --pairwise-budget) PAIRWISE_BUDGET="$2"; shift 2 ;;
    --state-sequence-seeds) STATE_SEQUENCE_SEEDS="$2"; shift 2 ;;
    --max-json-seeds) MAX_JSON_SEEDS="$2"; shift 2 ;;
    *) usage; exit 1 ;;
  esac
done

[[ -n "$REPO_ROOT" && -n "$BUILD_DIR" ]] || { usage; exit 1; }

RESULTS_DIR="${REPO_ROOT}/fuzz/stats/results"
CRASH_DIR="${RESULTS_DIR}/crashes"
mkdir -p "$RESULTS_DIR" "$CRASH_DIR"

targets=()
while IFS= read -r target; do
  [[ -n "$target" ]] || continue
  targets+=("$target")
done < <(find "${BUILD_DIR}/bin" -type f -perm -111 2>/dev/null | sort)

if [[ ${#targets[@]} -eq 0 ]]; then
  echo "No fuzz targets found in ${BUILD_DIR}/bin" >&2
  printf '{"targets": []}\n' > "${RESULTS_DIR}/run_results.json"
  exit 0
fi

MANIFEST="${REPO_ROOT}/fuzz/stats/results/discovery_manifest.json"
METADATA="${REPO_ROOT}/fuzz/stats/results/target_metadata_map.json"

python3 - "$MANIFEST" "$METADATA" "${RESULTS_DIR}/run_results.json" "$RUNS" "$MAX_TOTAL_TIME" "$RUN_ID" "${CRASH_DIR}" "${targets[@]}" <<'PY'
import json
import os
import subprocess
import sys
from pathlib import Path

manifest_path = Path(sys.argv[1])
metadata_path = Path(sys.argv[2])
out_path = Path(sys.argv[3])
runs = int(sys.argv[4])
max_total_time = int(sys.argv[5])
run_id = sys.argv[6]
crash_root = Path(sys.argv[7])
executables = [Path(x) for x in sys.argv[8:]]

manifest = json.loads(manifest_path.read_text(encoding="utf-8")) if manifest_path.exists() else {"targets": []}
metadata = json.loads(metadata_path.read_text(encoding="utf-8")) if metadata_path.exists() else {}

targets = manifest.get("targets", [])
by_key = {t["stable_target_key"]: t for t in targets}

results = {"run_id": run_id, "targets": []}

for exe in executables:
    if not exe.name.startswith("fuzz_"):
        continue
    key = exe.name.replace("fuzz_", "")
    if by_key and (key not in by_key):
        # Skip stale binaries from previous generations.
        continue
    target = by_key.get(key, {})
    log_file = Path("fuzz/stats/results") / f"{exe.name}.log"
    crash_dir = crash_root / target.get("folder_tag", "AppManager")
    crash_dir.mkdir(parents=True, exist_ok=True)
    # libFuzzer writes files as <prefix>crash-<hash>; set prefix to <crash_dir>/<key>_
    artifact_prefix = crash_dir / f"{key}_"

    # Try target-key-specific corpus first; fallback to folder_tag
    corpus_root = Path(sys.argv[7]).parent.parent / "corpus"
    corpus_dir = corpus_root / key  # Use target_key for per-target corpus
    if not corpus_dir.is_dir():
        corpus_dir = corpus_root / target.get("folder_tag", "AppManager")  # Fallback to folder_tag
    
    cmd = [str(exe), f"-runs={runs}"]
    if max_total_time > 0:
        cmd.append(f"-max_total_time={max_total_time}")
    cmd.append(f"-artifact_prefix={artifact_prefix}")
    if corpus_dir.is_dir():
        cmd.append(str(corpus_dir))

    with (Path(os.getcwd()) / log_file).open("w", encoding="utf-8") as fh:
        proc = subprocess.run(cmd, stdout=fh, stderr=subprocess.STDOUT, text=True)

    crash_files = sorted(
        str(p.relative_to(Path(os.getcwd())))
        for p in crash_dir.glob(f"{key}_crash-*")
        if p.is_file()
    )

    results["targets"].append(
        {
            "target_key": key,
            "binary": str(exe),
            "executions": runs,
            "max_total_time": max_total_time,
            "exit_code": proc.returncode,
            "log_file": str(log_file),
            "crash_files": crash_files,
            "folder_tag": target.get("folder_tag", "AppManager"),
            "function_name": target.get("function_name", metadata.get(key, {}).get("function_name", "unknown")),
            "source_file": target.get("source_file", metadata.get(key, {}).get("source_file", "unknown")),
            "line_number": target.get("line_number", metadata.get(key, {}).get("line_number", 0)),
            "crash_type": "sanitizer" if crash_files else ("exit" if proc.returncode != 0 else "none"),
        }
    )

out_path.write_text(json.dumps(results, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY

echo "Run results written to ${RESULTS_DIR}/run_results.json"

if [[ "${WITH_REPORT}" == "1" ]]; then
  python3 "${REPO_ROOT}/fuzz/stats/auto/reporting.py" triage --repo-root "${REPO_ROOT}"
  python3 "${REPO_ROOT}/fuzz/stats/auto/reporting.py" report \
    --repo-root "${REPO_ROOT}" \
    --runs "${RUNS}" \
    --max-total-time "${MAX_TOTAL_TIME}" \
    --pairwise-budget "${PAIRWISE_BUDGET}" \
    --state-sequence-seeds "${STATE_SEQUENCE_SEEDS}" \
    --max-json-seeds "${MAX_JSON_SEEDS}"
  echo "Reports updated: ${REPO_ROOT}/fuzz/stats/fuzz_report.md"
fi
