#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
FUZZ_ROOT="${ROOT_DIR}/fuzz/stats"
AUTO_DIR="${FUZZ_ROOT}/auto"
BUILD_DIR="${FUZZ_ROOT}/build"
RESULTS_DIR="${FUZZ_ROOT}/results"
REPORTS_DIR="${RESULTS_DIR}/reports"
CRASHES_DIR="${RESULTS_DIR}/crashes"
CORPUS_DIR="${FUZZ_ROOT}/corpus"

RUNS="${RUNS:-2000}"
MAX_TOTAL_TIME="${MAX_TOTAL_TIME:-0}"
PAIRWISE_BUDGET="${PAIRWISE_BUDGET:-48}"
STATE_SEQUENCE_SEEDS="${STATE_SEQUENCE_SEEDS:-20}"
MAX_JSON_SEEDS="${MAX_JSON_SEEDS:-40}"
NON_INTERACTIVE="${NON_INTERACTIVE:-1}"
RUN_ID="${RUN_ID:-$(date -u +%Y%m%dT%H%M%SZ)}"
TARGET_SCOPE="${TARGET_SCOPE:-AppManager,AppStorageManager,DownloadManager,LifecycleManager,PackageManager,PreinstallManager,RDKWindowManager,TelemetryMetrics}"
DEPENDENCIES_ROOT="${DEPENDENCIES_ROOT:-/Users/mthiru270@apac.comcast.com/fuzz7/dependencies}"
FUZZ_CC="${FUZZ_CC:-${CC:-}}"
FUZZ_CXX="${FUZZ_CXX:-${CXX:-}}"

log() { printf '[auto_fuzz][%s] %s\n' "$(date -u +%H:%M:%S)" "$*"; }
fail() { echo "[auto_fuzz][ERROR] $*" >&2; exit 1; }

assert_in_scope() {
  local candidate="$1"
  local abs
  abs="$(cd "$(dirname "$candidate")" && pwd)/$(basename "$candidate")"
  case "$abs" in
    "${FUZZ_ROOT}"/*) ;;
    *) fail "Path out of scope: $abs" ;;
  esac
}

prepare_dirs() {
  for d in "${AUTO_DIR}/generated" "${AUTO_DIR}/stubs" "${BUILD_DIR}" "${RESULTS_DIR}" "${REPORTS_DIR}" "${CRASHES_DIR}" "${CORPUS_DIR}"; do
    mkdir -p "$d"
    assert_in_scope "$d"
  done
}

cleanup_stale() {
  # Only the per-run result JSONs are always stale.
  # Harnesses, stubs, and corpus are preserved and updated incrementally
  # via fingerprint checks below. Build outputs are wiped each run.
  log "Clearing stale run results and previous build artifacts (harnesses / stubs / corpus preserved)"
  rm -f "${RESULTS_DIR}"/*.json
  rm -rf "${BUILD_DIR}"/*
}

select_fuzz_toolchain() {
  if [[ -n "${FUZZ_CC}" && -n "${FUZZ_CXX}" ]]; then
    return
  fi

  if [[ "$(uname -s)" == "Darwin" ]]; then
    local brew_llvm_cxx="/opt/homebrew/opt/llvm/bin/clang++"
    local brew_llvm_cc="/opt/homebrew/opt/llvm/bin/clang"
    if [[ -x "${brew_llvm_cxx}" && -x "${brew_llvm_cc}" ]]; then
      local runtime
      runtime="$(${brew_llvm_cxx} -print-file-name=libclang_rt.fuzzer_osx.a 2>/dev/null || true)"
      if [[ -n "${runtime}" && "${runtime}" != "libclang_rt.fuzzer_osx.a" && -f "${runtime}" ]]; then
        FUZZ_CC="${brew_llvm_cc}"
        FUZZ_CXX="${brew_llvm_cxx}"
        return
      fi
    fi
  fi

  FUZZ_CC="${FUZZ_CC:-clang}"
  FUZZ_CXX="${FUZZ_CXX:-clang++}"
}

validate_libfuzzer_runtime() {
  if [[ "$(uname -s)" != "Darwin" ]]; then
    return
  fi

  local runtime
  runtime="$(${FUZZ_CXX} -print-file-name=libclang_rt.fuzzer_osx.a 2>/dev/null || true)"
  if [[ -z "${runtime}" || "${runtime}" == "libclang_rt.fuzzer_osx.a" || ! -f "${runtime}" ]]; then
    fail "No macOS libFuzzer runtime found for compiler '${FUZZ_CXX}'. Install Homebrew llvm (brew install llvm) or set FUZZ_CC/FUZZ_CXX to a clang toolchain that provides libclang_rt.fuzzer_osx.a"
  fi

  log "Using libFuzzer runtime: ${runtime}"
}

# ---------------------------------------------------------------------------
# Fingerprint helpers
#
# Two independent fingerprints control regeneration:
#
#   _compute_stub_fingerprint   — hashes only #include directives + deps path
#                                  (stubs depend solely on what headers are
#                                   pulled in, not on implementation code)
#
#   _compute_source_fingerprint — hashes full content of scoped .h/.cpp files
#                                  (harnesses and corpus depend on API
#                                   signatures and code structure)
#
# Each fingerprint is stored in a dot-file inside its output directory.
# If the stored value matches the computed one the generation step is skipped.
# ---------------------------------------------------------------------------
_compute_stub_fingerprint() {
  # Hash #include lines in scoped sources + deps root + source fingerprint.
  # This ensures stubs are refreshed when code changes imply updated fallback
  # needs even if include directives remain unchanged.
  local scope_dirs content
  local stub_generator_hash
  scope_dirs=$(echo "${TARGET_SCOPE}" | tr ',' ' ')
  stub_generator_hash=$(sha256sum "${AUTO_DIR}/stub_generator.py" | awk '{print $1}')
  content="deps=${DEPENDENCIES_ROOT}\n"
  for dir in ${scope_dirs}; do
    [[ -d "${ROOT_DIR}/${dir}" ]] || continue
    content+=$(grep -r '^\ *#\ *include' "${ROOT_DIR}/${dir}" \
      --include='*.h' --include='*.cpp' 2>/dev/null | sort)
  done
  content+="\nsource_fp=$(_compute_source_fingerprint)"
  content+="\nstub_generator_fp=${stub_generator_hash}"
  printf '%s' "${content}" | \
    python3 -c "import hashlib,sys; print(hashlib.sha256(sys.stdin.read().encode()).hexdigest())"
}

_needs_stub_regen() {
  local stubs_dir="${AUTO_DIR}/stubs"
  local fp_file="${stubs_dir}/.stub_fingerprint"

  # Regenerate if the stubs directory is empty or fingerprint file is absent.
  if [[ ! -d "${stubs_dir}" ]] || \
     [[ -z "$(ls -A "${stubs_dir}" 2>/dev/null | grep -v '^\.stub_fingerprint$')" ]]; then
    log "Stubs directory empty — will generate stubs"
    return 0
  fi

  local current_fp
  current_fp=$(_compute_stub_fingerprint)

  if [[ ! -f "${fp_file}" ]] || [[ "$(cat "${fp_file}")" != "${current_fp}" ]]; then
    log "Include directives changed (fingerprint mismatch) — will regenerate stubs"
    # Keep existing stubs and update in place so PR runs can merge generated
    # assets with previously cached outputs.
    printf '%s' "${current_fp}" > "${fp_file}"
    return 0
  fi

  log "Stub fingerprint unchanged — skipping stub generation"
  return 1
}

# Source fingerprint: full content of all scoped .h/.cpp files.
# Result is cached in _SOURCE_FP_CACHE so the hash is computed at most once
# per invocation even when both harness and corpus checks call it.
_SOURCE_FP_CACHE=""
_compute_source_fingerprint() {
  if [[ -n "${_SOURCE_FP_CACHE}" ]]; then
    echo "${_SOURCE_FP_CACHE}"
    return
  fi
  local scope_dirs
  scope_dirs=$(echo "${TARGET_SCOPE}" | tr ',' ' ')
  _SOURCE_FP_CACHE=$(
    { for dir in ${scope_dirs}; do
        [[ -d "${ROOT_DIR}/${dir}" ]] || continue
        find "${ROOT_DIR}/${dir}" \( -name '*.h' -o -name '*.cpp' \) \
          | sort | xargs cat 2>/dev/null
      done; } | \
    python3 -c "import hashlib,sys; print(hashlib.sha256(sys.stdin.read().encode()).hexdigest())"
  )
  echo "${_SOURCE_FP_CACHE}"
}

_needs_harness_regen() {
  local fp_file="${AUTO_DIR}/generated/.harness_fingerprint"
  local harness_count
  harness_count=$(find "${AUTO_DIR}/generated" -name 'fuzz_*.cpp' 2>/dev/null | wc -l)

  if [[ "${harness_count}" -eq 0 ]]; then
    log "No harnesses found — will generate"
    return 0
  fi

  local current_fp
  current_fp=$(_compute_source_fingerprint)

  if [[ ! -f "${fp_file}" ]] || [[ "$(cat "${fp_file}")" != "${current_fp}" ]]; then
    log "Source changed (harness fingerprint mismatch) — will regenerate harnesses"
    return 0
  fi

  log "Source fingerprint unchanged — skipping harness generation"
  return 1
}

_needs_corpus_regen() {
  local fp_file="${CORPUS_DIR}/.corpus_fingerprint"
  local seed_count
  seed_count=$(find "${CORPUS_DIR}" -name 'seed_*.json' 2>/dev/null | wc -l)

  if [[ "${seed_count}" -eq 0 ]]; then
    log "No corpus seeds found — will generate"
    return 0
  fi

  local current_fp
  current_fp=$(_compute_source_fingerprint)

  if [[ ! -f "${fp_file}" ]] || [[ "$(cat "${fp_file}")" != "${current_fp}" ]]; then
    log "Source changed (corpus fingerprint mismatch) — will regenerate corpus"
    return 0
  fi

  log "Corpus fingerprint unchanged — skipping corpus generation"
  return 1
}

run_generators() {
  log "Running discovery"
  python3 "${AUTO_DIR}/generator.py" discover --repo-root "${ROOT_DIR}" --scope "${TARGET_SCOPE}"

  if _needs_stub_regen; then
    log "Generating stubs (dependency lookup: ${DEPENDENCIES_ROOT})"
    python3 "${AUTO_DIR}/stub_generator.py" \
      --repo-root "${ROOT_DIR}" \
      --dependencies-root "${DEPENDENCIES_ROOT}" \
      --scope "${TARGET_SCOPE}"
    _compute_stub_fingerprint > "${AUTO_DIR}/stubs/.stub_fingerprint"
  fi

  if _needs_harness_regen; then
    log "Generating harnesses"
    python3 "${AUTO_DIR}/generator.py" generate-harnesses --repo-root "${ROOT_DIR}"
    _compute_source_fingerprint > "${AUTO_DIR}/generated/.harness_fingerprint"
  fi

  if _needs_corpus_regen; then
    log "Generating corpus"
    python3 "${AUTO_DIR}/corpus_generator.py" \
      --repo-root "${ROOT_DIR}" \
      --pairwise-budget "${PAIRWISE_BUDGET}" \
      --state-sequence-seeds "${STATE_SEQUENCE_SEEDS}" \
      --max-json-seeds "${MAX_JSON_SEEDS}"
    _compute_source_fingerprint > "${CORPUS_DIR}/.corpus_fingerprint"
  fi

  # CMakeLists.txt is always regenerated — it is fast and reflects the exact
  # set of harness .cpp files currently in generated/.
  log "Generating CMake build file"
  python3 "${AUTO_DIR}/generator.py" generate-cmake \
    --repo-root "${ROOT_DIR}" \
    --dependencies-root "${DEPENDENCIES_ROOT}"
}

build_targets() {
  select_fuzz_toolchain
  validate_libfuzzer_runtime

  log "Using compilers: CC=${FUZZ_CC} CXX=${FUZZ_CXX}"
  log "Configuring CMake in ${BUILD_DIR}"
  cmake -S "${AUTO_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_C_COMPILER="${FUZZ_CC}" \
    -DCMAKE_CXX_COMPILER="${FUZZ_CXX}" \
    -DFUZZ_INCLUDE_APP_SOURCES=ON \
    || fail "CMake configure failed"

  log "Building fuzz targets"
  NCPUS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
  cmake --build "${BUILD_DIR}" -j "${NCPUS}" || fail "Build failed in full mode (FUZZ_INCLUDE_APP_SOURCES=ON)"
}

run_fuzzing() {
  log "Executing fuzz targets"
  "${FUZZ_ROOT}/run_fuzz.sh" \
    --repo-root "${ROOT_DIR}" \
    --build-dir "${BUILD_DIR}" \
    --runs "${RUNS}" \
    --max-total-time "${MAX_TOTAL_TIME}" \
    --run-id "${RUN_ID}"
}

triage_and_report() {
  log "Triaging crashes"
  python3 "${AUTO_DIR}/reporting.py" triage --repo-root "${ROOT_DIR}"

  log "Generating markdown reports"
  python3 "${AUTO_DIR}/reporting.py" report \
    --repo-root "${ROOT_DIR}" \
    --runs "${RUNS}" \
    --max-total-time "${MAX_TOTAL_TIME}" \
    --pairwise-budget "${PAIRWISE_BUDGET}" \
    --state-sequence-seeds "${STATE_SEQUENCE_SEEDS}" \
    --max-json-seeds "${MAX_JSON_SEEDS}"

  log "Validating output schemas"
  python3 "${AUTO_DIR}/validate_outputs.py" --repo-root "${ROOT_DIR}"
}

main() {
  [[ -d "${ROOT_DIR}" ]] || fail "Run from repository root"

  log "Run id: ${RUN_ID}"
  log "Target scope: ${TARGET_SCOPE}"
  prepare_dirs
  cleanup_stale
  run_generators
  build_targets
  run_fuzzing
  triage_and_report

  log "Done. Report: ${FUZZ_ROOT}/fuzz_report.md"
}

main "$@"
