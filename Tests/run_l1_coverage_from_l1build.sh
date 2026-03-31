#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

L1_ROOT="${REPO_ROOT}/l1build"
BUILD_ROOT="${L1_ROOT}/build"
COVERAGE_DIR="${L1_ROOT}/coverage"
LCOVRC_PATH="${REPO_ROOT}/Tests/L1Tests/.lcovrc_l1"
LCOV_CAPTURE_IGNORE_ERRORS="${LCOV_CAPTURE_IGNORE_ERRORS:-inconsistent,inconsistent,gcov,gcov}"

GCOV_CANDIDATES=()
GCOV_TOOL_SELECTED=""

add_gcov_candidate() {
    local candidate="$1"

    if [[ -z "${candidate}" ]]; then
        return 0
    fi

    if command -v "${candidate}" >/dev/null 2>&1; then
        local resolved
        resolved="$(command -v "${candidate}")"

        local existing
        for existing in "${GCOV_CANDIDATES[@]:-}"; do
            if [[ "${existing}" == "${resolved}" ]]; then
                return 0
            fi
        done

        GCOV_CANDIDATES+=("${resolved}")
    fi
}

discover_gcov_candidates() {
    if [[ -n "${GCOV_TOOL:-}" ]]; then
        add_gcov_candidate "${GCOV_TOOL}"
    fi

    add_gcov_candidate "gcov"
    add_gcov_candidate "gcov-15"
    add_gcov_candidate "gcov-14"
    add_gcov_candidate "gcov-13"
    add_gcov_candidate "gcov-12"
    add_gcov_candidate "gcov-11"
    add_gcov_candidate "gcov-10"
    add_gcov_candidate "gcov-9"
    add_gcov_candidate "gcov-8"
    add_gcov_candidate "gcov-7"

    if [[ "${#GCOV_CANDIDATES[@]}" -eq 0 ]]; then
        echo "ERROR: No gcov tool found in PATH."
        exit 1
    fi
}

capture_with_matching_gcov() {
    local coverage_info_path="$1"
    local lcov_err_log
    local tool

    lcov_err_log="$(mktemp)"
    trap 'rm -f "${lcov_err_log}"' RETURN

    for tool in "${GCOV_CANDIDATES[@]}"; do
        echo "Trying coverage capture with gcov tool: ${tool}"

        if lcov --config-file "${LCOVRC_PATH}" \
            --gcov-tool "${tool}" \
            --ignore-errors "${LCOV_CAPTURE_IGNORE_ERRORS}" \
            --rc geninfo_unexecuted_blocks=1 \
            -c \
            -o "${coverage_info_path}" \
            -d "${BUILD_ROOT}/entservices-appmanagers" \
            2> >(tee "${lcov_err_log}" >&2); then
            GCOV_TOOL_SELECTED="${tool}"
            return 0
        fi

        if grep -q "Incompatible GCC/GCOV version" "${lcov_err_log}"; then
            echo "gcov mismatch with ${tool}, trying next candidate..."
            : > "${lcov_err_log}"
            continue
        fi

        echo "ERROR: lcov capture failed with ${tool}."
        return 1
    done

    echo "ERROR: Could not find a compatible gcov tool for the generated .gcda/.gcno files."
    echo "Tried: ${GCOV_CANDIDATES[*]}"
    echo "Hint: export GCOV_TOOL=/usr/bin/gcov-<matching-version> and rerun."
    return 1
}

if ! command -v lcov >/dev/null 2>&1; then
    echo "ERROR: lcov is not installed. Install it (e.g., sudo apt install -y lcov)."
    exit 1
fi

if ! command -v genhtml >/dev/null 2>&1; then
    echo "ERROR: genhtml is not available. Install lcov package (contains genhtml)."
    exit 1
fi

if [[ ! -f "${LCOVRC_PATH}" ]]; then
    echo "ERROR: LCOV config file not found: ${LCOVRC_PATH}"
    exit 1
fi

if [[ ! -d "${BUILD_ROOT}/entservices-appmanagers" ]]; then
    echo "ERROR: Build directory not found: ${BUILD_ROOT}/entservices-appmanagers"
    echo "Run ./Tests/run_l1_from_l1build.sh first (or rebuild script), then rerun coverage."
    exit 1
fi

if [[ -z "$(find "${BUILD_ROOT}/entservices-appmanagers" -type f -name '*.gcda' -print -quit)" ]]; then
    echo "ERROR: No .gcda files found under ${BUILD_ROOT}/entservices-appmanagers"
    echo "Run L1 tests first so coverage data is generated, then rerun this script."
    exit 1
fi

discover_gcov_candidates

mkdir -p "${COVERAGE_DIR}"
rm -rf "${COVERAGE_DIR}"/*

COVERAGE_INFO="${L1_ROOT}/coverage.info"
FILTERED_COVERAGE_INFO="${L1_ROOT}/filtered_coverage.info"

rm -f "${COVERAGE_INFO}" "${FILTERED_COVERAGE_INFO}"

echo "Collecting coverage data..."
capture_with_matching_gcov "${COVERAGE_INFO}"
echo "Using gcov tool: ${GCOV_TOOL_SELECTED}"

echo "Filtering coverage data..."
lcov --config-file "${LCOVRC_PATH}" \
    --ignore-errors unused,unused \
    -r "${COVERAGE_INFO}" \
    '/usr/include/*' \
    '*/build/entservices-appmanagers/_deps/*' \
    '*/install/usr/include/*' \
    '*/Tests/headers/*' \
    '*/Tests/mocks/*' \
    '*/Tests/L1Tests/tests/*' \
    '*/Thunder/*' \
    -o "${FILTERED_COVERAGE_INFO}"

echo "Generating HTML report in ${COVERAGE_DIR}..."
genhtml \
    -o "${COVERAGE_DIR}" \
    -t "entservices-appmanagers coverage" \
    "${FILTERED_COVERAGE_INFO}"

echo "Coverage report ready: ${COVERAGE_DIR}/index.html"
