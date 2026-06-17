# Fuzz Testing Framework — Architecture & Flow

## What is this framework?

This is an **automated libFuzzer framework** that:
1. Scans C++ source code to find callable function targets in scope (public APIs plus internal/helper/member methods that match discovery patterns)
2. Auto-generates a fuzz harness (a small C++ test driver) for each API
3. Compiles all harnesses with AddressSanitizer (ASan) + UndefinedBehaviorSanitizer (UBSan)
4. Runs every harness with random + seeded inputs to find crashes
5. Collects, triages, and reports results

The goal is to find memory-safety bugs (buffer overflows, use-after-free, null dereferences, etc.) in the plugin implementations before they reach production.

---

## High-Level Flow

```
┌─────────────────────────────────────────────────────────────────────┐
│                        PHASE 1 — DISCOVER                           │
│                                                                     │
│  Source folders:                                                    │
│  AppManager / AppStorageManager / DownloadManager /                 │
│  LifecycleManager / PackageManager / PreinstallManager /            │
│  RDKWindowManager / TelemetryMetrics                                │
│                         │                                           │
│                         ▼                                           │
│              generator.py  discover                                 │
│   (scans .cpp/.h files with regex, extracts callable signatures)    │
│                         │                                           │
│                         ▼                                           │
│  results/discovery_manifest.json  ←── discovered callable targets   │
│          results/dispatch_coverage_report.json ←── mapped vs not   │
└─────────────────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────────┐
│                        PHASE 2 — STUBS                              │
│                                                                     │
│              stub_generator.py                                      │
│   (generates lightweight .h stubs for headers that pull in          │
│    heavy platform/Thunder dependencies unavailable on macOS)        │
│                         │                                           │
│                         ▼                                           │
│               auto/stubs/  (*.h stub files)                         │
└─────────────────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      PHASE 3 — HARNESS GENERATION                   │
│                                                                     │
│            generator.py  generate-harnesses                         │
│   (for each discovered API, writes a C++ file:                      │
│    - reads raw bytes via InputReader                                 │
│    - creates the real implementation object                          │
│    - dispatches fuzzer bytes as typed arguments                      │
│    - calls the actual production function)                           │
│                         │                                           │
│                         ▼                                           │
│            auto/generated/fuzz_<hash>.cpp  (one per API)            │
│            auto/generated/fuzz_harness_common.h                     │
└─────────────────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      PHASE 4 — CMAKE BUILD                          │
│                                                                     │
│           generator.py  generate-cmake                              │
│   (writes auto/CMakeLists.txt that compiles every harness with:     │
│    -fsanitize=address,undefined                                      │
│    -fsanitize-coverage=trace-pc-guard                                │
│    -DFUZZ_FOLDER_<PLUGINNAME>                                         │
│    real plugin .cpp sources included directly)                       │
│                         │                                           │
│                         ▼                                           │
│              cmake + clang → build/bin/fuzz_<hash>                  │
│              (one binary per API, ~639 binaries)                    │
└─────────────────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      PHASE 5 — CORPUS SEEDS                         │
│                                                                     │
│               corpus_generator.py                                   │
│   (generates structured JSON seed files per target:                  │
│    - boundary integer values (0, MAX, -1, etc.)                      │
│    - string edge cases (empty, huge, special chars)                  │
│    - pointer-mode seeds: null / invalid / valid pointer)            │
│                         │                                           │
│                         ▼                                           │
│            corpus/<FolderName>/<key>/seed_*.json                    │
└─────────────────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      PHASE 6 — FUZZ RUN                             │
│                                                                     │
│                  run_fuzz.sh                                        │
│   (loops over every binary in build/bin/:                            │
│    - runs: ./fuzz_<hash> -runs=N -artifact_prefix=<crashdir>        │
│    - feeds corpus/<folder> as initial seed corpus                   │
│    - libFuzzer mutates inputs and tracks coverage                   │
│    - if a crash is found, libFuzzer saves the triggering input      │
│    - stdout/stderr captured to results/fuzz_<hash>.log)             │
│                         │                                           │
│                         ▼                                           │
│            results/run_results.json   (per-target outcomes)         │
│            crashes/<folder>/<key>_crash-<hash>  (crash inputs)      │
└─────────────────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      PHASE 7 — TRIAGE & REPORT                      │
│                                                                     │
│               reporting.py  triage                                  │
│   (reads every log, looks for ASan/UBSan signatures,                │
│    classifies crashes as valid (in-scope) or false_positive,        │
│    copies reproducer inputs to crashes/<folder>/<key>/)             │
│                         │                                           │
│               reporting.py  report                                  │
│   (renders fuzz_report.md: crash summary, per-folder breakdown,     │
│    stack frames, suggested fix guidance)                            │
│                         │                                           │
│                         ▼                                           │
│         results/triage_results.json                                 │
│         fuzz_report.md   ← human-readable final report             │
└─────────────────────────────────────────────────────────────────────┘
```

---

## Component Map

```
fuzz/
├── stats/
│   ├── auto/
│   │   ├── generator.py            ← MAIN BRAIN: discover / generate-harnesses / generate-cmake
│   │   ├── stub_generator.py       ← generates stub headers for missing platform dependencies
│   │   ├── corpus_generator.py     ← generates semantic seed inputs per API
│   │   ├── reporting.py            ← triage crashes, render fuzz_report.md
│   │   ├── dispatch_mapping.json   ← allowlist of 97 APIs that get real dispatch (editable)
│   │   ├── generated/              ← auto-generated C++ harness files (one per API)
│   │   │   ├── fuzz_harness_common.h
│   │   │   └── fuzz_<hash>.cpp  (×639)
│   │   └── stubs/                  ← auto-generated stub headers
│   ├── run_fuzz.sh                 ← orchestrates the fuzz run loop
│   ├── build/
│   │   └── bin/fuzz_<hash>         ← compiled fuzz binaries (×639)
│   ├── corpus/
│   │   └── <FolderName>/<key>/     ← seed corpus per target
│   ├── crashes/
│   │   └── <FolderName>/<key>/     ← crash reproducer inputs
│   ├── results/
│   │   ├── discovery_manifest.json       ← all discovered APIs
│   │   ├── target_metadata_map.json      ← metadata (file, line, args) per key
│   │   ├── dispatch_coverage_report.json ← mapped vs unmapped API analysis
│   │   ├── run_results.json              ← per-target run outcomes
│   │   └── triage_results.json           ← crash classification results
│   └── fuzz_report.md              ← final human-readable report
```

---

## Step-by-Step: What Happens When You Run It

### Step 1 — API Discovery

`generator.py discover` walks through all 8 plugin folders. For each `.cpp` and `.h` file it:
- Applies a regex to extract function declarations (return type, name, arguments)
- Filters out false positives (`if`, `for`, `while`, macros, etc.)
- Records each function with its source file path and line number
- Assigns a stable hash key (`SHA-1` of `file:function:line`) so the key never changes across re-runs
- Outputs `discovery_manifest.json` with every target found (~639 targets)
- Also writes `dispatch_coverage_report.json` explaining which APIs are mapped and why others are not

### Step 2 — Stub Generation

The production code includes Thunder/WPEFramework headers that don't exist on macOS developer machines. `stub_generator.py` scans the `#include` directives in each source file and creates minimal stub headers under `auto/stubs/` so the code compiles without the full platform SDK.

### Step 3 — Harness Generation

`generator.py generate-harnesses` creates one `.cpp` file per discovered API. Each harness:

```cpp
// Example: fuzz_abc123.cpp  (for AppManagerImplementation::LaunchApp)
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
    fuzzauto::InputReader in(Data, Size);
    std::string target = "AppManagerImplementation::LaunchApp";
    fuzzauto::RunTargetApi(target, in);
    return 0;
}
```

Inside `RunTargetApi`, the fuzzer bytes are read as typed values and passed to the real API:

```cpp
// Inside fuzz_harness_common.h → RunTargetApi()
auto& impl = *new AppManagerImplementation();
(void)impl.LaunchApp(appId, intent, arg);
```

The `InputReader` class slices the raw bytes into:
- strings (length-prefixed)
- int32 / bool values (4-byte / 1-byte reads)
- pointer mode byte: 0 = null, 1 = invalid pointer, 2 = valid pointer

### Step 4 — Build with Sanitizers

CMake compiles every harness with real plugin source files included directly (`FUZZ_INCLUDE_APP_SOURCES=ON`). Compiler flags include:
- `-fsanitize=address,undefined` — AddressSanitizer + UBSan
- `-fsanitize-coverage=trace-pc-guard` — edge coverage for libFuzzer guidance
- `-DFUZZ_FOLDER_<PLUGIN>` — selects the right plugin header in `fuzz_harness_common.h`

This means the fuzzer exercises the **real production code**, not mocked stubs.

### Step 5 — Corpus Seeds

`corpus_generator.py` creates structured seed files for each API under `corpus/<Folder>/<key>/`. These give libFuzzer a head start with meaningful inputs:
- Integer boundary values: `0`, `1`, `-1`, `INT_MAX`, `INT_MIN`
- String edge cases: empty string, very long string (4096 chars), null-containing strings
- Pointer mode seeds: `seed_ptrmode_0.json` (null), `seed_ptrmode_1.json` (invalid), `seed_ptrmode_2.json` (valid)
- Pairwise combination seeds (field combinations likely to expose interaction bugs)

### Step 6 — Fuzz Execution

`run_fuzz.sh` loops over all binaries in `build/bin/` and runs each one:

```bash
./build/bin/fuzz_abc123 -runs=N -artifact_prefix=crashes/AppManager/abc123_ corpus/AppManager/abc123/
```

- libFuzzer mutates inputs and tracks which code edges are newly covered
- If any input causes a crash (ASan/UBSan abort), libFuzzer saves the triggering input file
- All output is captured to `results/fuzz_<hash>.log`
- When finished, all outcomes are written to `results/run_results.json`

### Step 7 — Triage and Report

`reporting.py triage` reads every log file:
- Detects ASan/UBSan crash signatures
- Parses stack frames to identify if the crash originated in production code (in-scope) vs the harness itself (false positive)
- Copies crash reproducer inputs to `crashes/<folder>/<key>/reproducer_000.bin`
- Writes `results/triage_results.json`

`reporting.py report` renders `fuzz_report.md` with:
- Total targets run, crash count, per-folder breakdown
- Per-crash entry: function name, source file/line, crash type, stack frames, suggested fix

---

## Dispatch Mapping — Why It Exists

When a harness is generated for an API, the harness must know **how to call** that specific API (what object to construct, what arguments to pass). This semantic knowledge cannot be inferred from the function signature alone.

`dispatch_mapping.json` is the **allowlist** of 97 APIs that have an explicit dispatch branch in `RunTargetApi`. Only these APIs get a real call into production code.

APIs not in the allowlist are still compiled (so build errors are caught) but the call is skipped at runtime and classified by `unmapped_reason()`:

| Reason | Meaning |
|---|---|
| `internal_helper_method` | Private/helper methods (lower-camelCase, `on*`, `handle*`, etc.) — not public APIs |
| `constructor_or_destructor_like` | Constructors, destructors, class-level setup |
| `accessor_or_mutator` | Simple `get*` / `set*` property accessors |
| `plugin_lifecycle_wrapper` | Thunder lifecycle callbacks (`Initialize`, `Deinitialize`) — managed by the framework |
| `singleton_or_registration_helper` | `Register`, `getInstance`, etc. — infrastructure, not testable in isolation |
| `event_callback` | `On*` event notification methods — triggered by platform events, not direct calls |

To add a new API to real dispatch: add it to `dispatch_mapping.json` and add a `#elif` branch in `RunTargetApi` inside `generator.py`.

---

## Data Flow Summary

```
 .cpp / .h files
      │
      ▼ (regex scan)
 discovery_manifest.json  ──►  dispatch_coverage_report.json
      │
      ▼ (code generation)
 generated/fuzz_<hash>.cpp  (×639)
      │
      ▼ (clang + ASan/UBSan)
 build/bin/fuzz_<hash>  (×639)
      │
      ├── corpus seeds ──► libFuzzer
      │
      ▼ (run_fuzz.sh loops all binaries)
 results/fuzz_<hash>.log  (per-target)
 crashes/<folder>/<key>_crash-<hash>  (on crash)
      │
      ▼ (reporting.py triage + report)
 results/triage_results.json
 fuzz_report.md
```

---

## Key Numbers (Latest Run)

| Metric | Value |
|---|---|
| Plugin folders scanned | 8 |
| APIs discovered | ~639 |
| APIs with real dispatch | 97 |
| Sanitizer-detected crashes | 10 |
| Crash folders | DownloadManager, AppManager, AppStorageManager, PreinstallManager, PackageManager, RDKWindowManager |
| Build result | `EXIT:0` (clean) |

---

## How to Run the Full Pipeline

```bash
REPO="$(pwd)"
DEPS="/path/to/dependencies"
SCOPE="AppManager,AppStorageManager,DownloadManager,LifecycleManager,PackageManager,PreinstallManager,RDKWindowManager,TelemetryMetrics"

# Step 1: Discover APIs
python3 fuzz/stats/auto/generator.py discover --repo-root "$REPO" --scope "$SCOPE"

# Step 2: Generate stubs
python3 fuzz/stats/auto/stub_generator.py --repo-root "$REPO" --dependencies-root "$DEPS" --scope "$SCOPE"

# Step 3: Generate harnesses
python3 fuzz/stats/auto/generator.py generate-harnesses --repo-root "$REPO"

# Step 4: Generate CMakeLists
python3 fuzz/stats/auto/generator.py generate-cmake --repo-root "$REPO" --dependencies-root "$DEPS"

# Step 5: Build
cmake -S fuzz/stats/auto -B fuzz/stats/build -DCMAKE_CXX_COMPILER=clang++ -DFUZZ_INCLUDE_APP_SOURCES=ON
cmake --build fuzz/stats/build -j$(nproc)

# Step 6+7: Run, triage, report
./fuzz/stats/run_fuzz.sh --repo-root "$REPO" --build-dir "$REPO/fuzz/stats/build" \
  --runs 1000 --max-total-time 0 --run-id "my_run_$(date -u +%Y%m%dT%H%M%SZ)"

# Results
cat fuzz/stats/fuzz_report.md
```
