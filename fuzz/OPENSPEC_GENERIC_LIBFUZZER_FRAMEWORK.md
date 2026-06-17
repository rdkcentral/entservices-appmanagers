# OpenSpec: Generic Automated libFuzzer Framework for C and C++ Projects

**Status:** Proposal  
**Applicability:** Reusable across C/C++ codebases  
**Execution Model:** Single-command automated fuzzing pipeline  
**Output Isolation:** All generated assets, build outputs, crashes, corpus, reports, and helper scripts remain under `fuzz/stats/`

---

## 1. Objective

Design and implement a fully automated, production-grade fuzzing framework for C and C++ projects using libFuzzer and sanitizers.

The framework must:
- automatically discover callable targets in configured source folders
- generate fuzz harnesses without hand-written target logic
- generate intelligent argument-aware and JSON-aware seeds
- safely fuzz nested API paths with generated stubs where required
- build and run without modifying production source trees
- validate only real crashes from in-scope production code
- minimize false positives from generated harness or stub logic
- generate actionable reports and reproducers
- run end-to-end through a single command

The framework is intended to behave consistently across projects once placeholders are filled in for source scope, dependency roots, and project-specific parsing rules.

---

## 2. Scope Placeholders

Replace the following placeholders when proposing this framework for a new project.

- `<TARGET_SOURCE_FOLDERS>`: comma-separated list of top-level folders to scan
- `<EXCLUDED_FOLDERS>`: folders to exclude from discovery, build, and reporting
- `<DEPENDENCIES_ROOT>`: external dependencies root or SDK location
- `<PRIMARY_BUILD_HINTS>`: project-specific dependency/build references if needed
- `<OPTIONAL_PLATFORM_HEADERS>`: headers unavailable on development platforms that require generated stubs
- `<PROJECT_INCLUDE_PATTERNS>`: include roots for production compilation
- `<PROJECT_SOURCE_PATTERNS>`: source patterns used for production compilation
- `<PROJECT_API_PATTERNS>`: parser patterns needed to capture callable targets in this project

Example placeholder block:

```text
TARGET_SOURCE_FOLDERS=<TARGET_SOURCE_FOLDERS>
EXCLUDED_FOLDERS=<EXCLUDED_FOLDERS>
DEPENDENCIES_ROOT=<DEPENDENCIES_ROOT>
PRIMARY_BUILD_HINTS=<PRIMARY_BUILD_HINTS>
OPTIONAL_PLATFORM_HEADERS=<OPTIONAL_PLATFORM_HEADERS>
PROJECT_INCLUDE_PATTERNS=<PROJECT_INCLUDE_PATTERNS>
PROJECT_SOURCE_PATTERNS=<PROJECT_SOURCE_PATTERNS>
PROJECT_API_PATTERNS=<PROJECT_API_PATTERNS>
```

---

## 3. Required Directory Layout

All generated content must remain isolated under `fuzz/stats/`.

```text
fuzz/
  stats/
    auto/
      generator.py
      stub_generator.py
      corpus_generator.py
      reporting.py
      generated/
      stubs/
      CMakeLists.txt

    corpus/
      <target_key_or_function_name>/

    crashes/
      <folder>/<target_key_or_function_name>/

    results/
    reports/
      <folder>/fuzz_report.md

    build/

    auto_fuzz.sh
    run_fuzz.sh
    fuzz_report.md
```

Constraints:
- do not write generated fuzz assets into production source folders
- do not modify production source files during generation
- do not depend on project autotools or existing production build scripts for the fuzz build itself
- keep cleanup scoped to generated fuzz assets and build outputs only

---

## 4. Single Command Execution

The framework must expose a single top-level command:

```bash
./fuzz/stats/auto_fuzz.sh
```

Canonical execution should support environment override patterns such as:

```bash
RUNS=${RUNS:-2000} \
MAX_TOTAL_TIME=${MAX_TOTAL_TIME:-0} \
PATH="<toolchain-bin>:$PATH" \
./fuzz/stats/auto_fuzz.sh
```

This command must automatically:
- discover callable targets
- generate harnesses
- generate corpus seeds
- generate platform/dependency stubs
- generate build files
- build fuzz binaries
- run libFuzzer with sanitizers
- collect crashes
- validate crashes
- generate final reports

---

## 5. Architecture Reference

A companion architecture document may be referenced for implementation flow, but the spec must remain generic. The architecture should describe the following phases:

```text
DISCOVER -> STUBS -> HARNESS GENERATION -> BUILD -> CORPUS -> EXECUTION -> TRIAGE -> REPORTING
```

Generic architectural expectations:
- **Discovery** identifies callable targets in configured scope
- **Stub generation** compensates for missing platform or external headers
- **Harness generation** produces one harness per target using real production code
- **Build generation** compiles fuzz binaries with sanitizers under isolated output directories
- **Corpus generation** creates meaningful, bounded, structured seed inputs
- **Execution** runs each target with seed-driven and mutation-driven fuzzing
- **Triage** filters false positives and validates crash ownership
- **Reporting** emits machine-readable and human-readable outputs

---

## 6. Automatic Callable Discovery

### 6.1 Discovery Goal

The discovery stage must automatically identify **callable targets in configured scope**.

This includes, when matched by parser rules:
- exported APIs
- public methods
- static functions
- helper functions
- event handlers
- scheduler functions
- validation functions
- parsing functions
- memory-related handlers
- nested API helpers
- additional callable entities identified by parser logic

It must not be limited to public APIs unless a project explicitly asks for that narrower mode.

### 6.2 Required Discovery Outputs

For each discovered target, capture:
- function name
- signature
- arguments
- source file
- line number
- folder tag
- classification tags
- JSON-touching indicator
- stable target key

Emit:
- `fuzz/stats/results/discovery_manifest.json`
- `fuzz/stats/results/target_metadata_map.json`
- coverage gaps for callable-like lines not matched by the primary parser

### 6.3 Parsing Expectations

The generator must support C and C++ callable discovery patterns including:
- free functions
- scoped member functions
- static methods
- overloaded functions
- constructors/destructors when explicitly included by project rules
- macro-heavy code paths where callable-like gaps should still be reported

The parser should be configurable through `<PROJECT_API_PATTERNS>` and should support project-specific refinement where needed.

### 6.4 Dispatch or Reachability Analysis

If a project uses a dispatch, RPC, registration, or external-call mapping layer, emit an additional reachability report, such as:
- dispatch-mapped targets
- unmapped internal/helper targets
- reasons for unmapped classification

This report is informative only. Unmapped internal targets may still be fuzzed if they are in scope.

---

## 7. Intelligent Argument-Aware Fuzzing

The framework must not rely on random bytes alone.

It must analyze argument shapes and generate structured inputs using type-aware heuristics.

### 7.1 Integer-Like Parameters

Generate bounded semantic values such as:
- zero
- one
- negative one where signed
- minimum and maximum values by type width
- common sentinel values
- overflow-adjacent boundaries

Support concrete widths including:
- int8 / uint8
- int16 / uint16
- int32 / uint32
- int64 / uint64

### 7.2 String-Like Parameters

Generate:
- empty strings
- whitespace-only values
- normal short strings
- long strings
- special characters
- newline and tab variants
- path-like values
- invalid identifier-like values
- malformed format-specific strings when type hints suggest them

### 7.3 Boolean-Like Parameters

Generate:
- true
- false
- numeric equivalents
- sentinel values when parsing logic may coerce them

### 7.4 Enum, Mode, and State Parameters

Generate:
- known valid enum-like values
- boundary and unknown mode values
- state transition values when lifecycle behavior is implied

### 7.5 Struct and Pointer Parameters

Automatically generate seeds that:
- zero initialize shapes
- partially initialize fields
- vary fields independently
- corrupt nested values in a bounded way
- test null pointers
- test logically missing subfields
- exercise ownership-sensitive code without invalid harness cleanup

---

## 8. JSON-Aware Seed Generation

If a target consumes, builds, parses, or mutates JSON-like payloads, the corpus generator must create structured seeds rather than random garbage.

For each JSON-touching target, generate bounded meaningful seeds, for example:
- valid objects
- missing keys
- empty values
- null values
- invalid types
- oversized values
- nested object mutations
- array mutations
- duplicate-like fields where representable
- malformed nested structures

Field-wise mutation should include:
- removing the key
- replacing with empty string
- whitespace-only value
- null value
- type mismatch
- boundary numeric value
- oversized content
- malformed nested child

The goal is to test resilience to:
- missing required fields
- malformed structures
- partial payloads
- nested corruption
- semantic edge cases

The output location must remain generic:

```text
fuzz/stats/corpus/<target_key_or_function_name>/
```

---

## 9. Seed Strategy and Corpus Pickup

### 9.1 Per-Target Corpus

The preferred runtime strategy is **per-target corpus first**.

Each target should consume:
- `fuzz/stats/corpus/<target_key_or_function_name>/`

### 9.2 Shared Folder-Level Corpus Fallback

If a project also generates shared folder-level or module-level seeds, the runner should:
- prefer target-specific corpus first
- optionally merge or fall back to folder-level seeds when per-target corpus is missing or incomplete

### 9.3 Practical Hardening Requirement

To prevent seed loss:
- any shared folder-level seeds must not become unreachable
- if both folder-level and target-level seeds exist, ensure runner behavior does not ignore one set unintentionally
- the framework may merge shared seeds into each target corpus or use multi-path pickup logic

The design goal is simple: **no meaningful generated seed should be silently skipped at runtime**.

### 9.4 Bounded Seed Budgets

Support budget controls such as:
- `RUNS`
- `MAX_TOTAL_TIME`
- `PAIRWISE_BUDGET`
- `STATE_SEQUENCE_SEEDS`
- `MAX_JSON_SEEDS`

The implementation must prioritize meaningful bounded seeds over exhaustive combinatorial explosion.

---

## 10. Nested API Safety and Stub Generation

Some projects cannot compile production code directly on every development platform because of unavailable SDKs, system frameworks, RPC headers, or hardware headers.

The framework must automatically generate stubs into:

```text
fuzz/stats/auto/stubs/
```

Stub generation should support:
- missing headers
- missing types
- placeholder constants and enums
- placeholder interfaces needed only for compilation
- failure profiles for nested dependencies

Supported failure simulations may include:
- null returns
- allocation failures
- timeout-like conditions
- invalid states
- malformed nested responses
- cross-component failure results

Hard requirement:
- generated stubs must enable compilation and controlled negative-path fuzzing
- generated stubs must not become the source of reported product defects

---

## 11. Real Code Execution and False Positive Prevention

Harnesses must exercise real in-scope production code.

Rules:
- do not replace core target logic with mocks
- compile real project source files into fuzz binaries
- use stubs only to satisfy unavailable dependencies or boundary simulation requirements
- report only crashes rooted in in-scope production code

Do not report as product defects:
- generated harness cleanup errors
- ownership bugs isolated to generated fuzz code
- invalid free or double free caused only by generated assets
- crashes originating solely from generated stubs or generated harnesses

Harness ownership rules:
- free only locally allocated memory
- avoid double free
- track generated allocations safely
- never free memory owned by production code

---

## 12. Crash Validation

A crash is valid only if the fault origin is in configured in-scope production paths.

Ignore crashes that originate only from:
- `fuzz/stats/auto/generated/`
- `fuzz/stats/auto/stubs/`
- generated-only cleanup logic

For valid crashes, capture:
- reproducer
- triggering input
- sanitizer output
- stack trace
- source file and line number when available
- crash classification
- root cause explanation
- suggested fix direction

Store validated crashes under:

```text
fuzz/stats/crashes/<folder>/<target_key_or_function_name>/
```

Do not create empty crash directories.

---

## 13. Build System Requirements

Auto-generate:

```text
fuzz/stats/auto/CMakeLists.txt
```

Use:
- `clang`
- `clang++`
- libFuzzer integration
- AddressSanitizer
- optional UndefinedBehaviorSanitizer if project-compatible

Expected sanitizer baseline:

```text
-fsanitize=fuzzer,address
```

Optional extension:

```text
-fsanitize=undefined
```

The fuzz build must:
- avoid dependency on autotools or project-specific production build files
- compile isolated fuzz binaries under `fuzz/stats/build/`
- use `<DEPENDENCIES_ROOT>` and `<PROJECT_INCLUDE_PATTERNS>` placeholders for external includes and libraries
- allow environment overrides such as `FUZZ_CC`, `FUZZ_CXX`, and dependency-root options

### 13.1 Dependency Handling

The framework must allow a placeholder-driven dependency model:
- `<DEPENDENCIES_ROOT>` for external SDKs or vendored libraries
- `<PRIMARY_BUILD_HINTS>` for project-specific dependency references
- optional parsing of existing project build hints to infer include or library paths

This is especially important when the production code depends on platform SDKs, cross-compiled headers, or environment-specific third-party libraries.

---

## 14. macOS and Cross-Platform Hardening

The framework must be usable on macOS and Linux where feasible.

### 14.1 macOS Challenges to Handle

macOS commonly introduces these issues:
- non-system LLVM installation paths
- platform SDK headers missing for Linux-targeted code
- dependency paths installed outside default system locations
- differing shell environments and path resolution

### 14.2 Required Mitigations

The framework should support:
- LLVM toolchain injection via `PATH`
- compiler override via `FUZZ_CC` and `FUZZ_CXX`
- generated stubs for unavailable platform headers
- explicit `<DEPENDENCIES_ROOT>` configuration
- portable shell scripting compatible with standard macOS environments
- build fallback modes when high parallelism causes instability

### 14.3 Generic macOS Execution Pattern

```bash
RUNS=${RUNS:-2000} \
PATH="<llvm-bin>:$PATH" \
FUZZ_CC=${FUZZ_CC:-clang} \
FUZZ_CXX=${FUZZ_CXX:-clang++} \
./fuzz/stats/auto_fuzz.sh
```

---

## 15. Run Stability and Cleanup

Before each run, the framework must clean stale generated assets that can poison the next run.

Clean only generated fuzz artifacts such as:
- generated harnesses
- generated stubs
- generated build files
- fuzz build outputs
- stale reports when appropriate

Do not delete production project assets.

The runner must ensure:
- stale binaries do not survive target regeneration
- build failures are fixed via dependency resolution or stub generation, not by silently skipping targets
- old artifacts do not create misleading run results

---

## 16. Execution Controls

Support the following controls:
- `RUNS`: per-target libFuzzer iteration budget
- `MAX_TOTAL_TIME`: per-target time budget passed as `-max_total_time`
- `PAIRWISE_BUDGET`: cap pairwise argument-combination seeds
- `STATE_SEQUENCE_SEEDS`: cap state-sequence seeds for stateful targets
- `MAX_JSON_SEEDS`: cap JSON seed count per JSON-touching target

Execution guidance:
- use low budgets for setup validation
- use larger budgets for real fuzz campaigns
- use `MAX_TOTAL_TIME` when a project contains slow or hanging targets
- permit `0` when no per-target time cap is desired

---

## 17. Reporting Requirements

Automatically generate:
- `fuzz/stats/fuzz_report.md`
- `fuzz/stats/reports/<folder>/fuzz_report.md`

Per-folder reports are required for all discovered top-level folders except excluded folders.

Reports must include:
- total functions fuzzed
- total seeds generated
- total executions
- crashes found
- ignored false positives
- coverage or discovery summary
- discovery-to-run reconciliation
- dispatch/reachability summary when available

For each valid crash include:
- function name
- source file
- line number when available
- crash type
- sanitizer summary
- triggering input or reproducer path
- why the crash occurred
- suggested fix direction

---

## 18. Output Expectations

A successful propose-to-implementation flow should result in behavior like this, without using project-specific constants in the spec:
- discovery produces a machine-readable manifest of callable targets in scope
- generation produces one fuzz harness per stable target key
- corpus generation creates bounded meaningful seeds per target
- build generation produces isolated fuzz binaries
- execution consumes generated corpus without silently skipping relevant seeds
- crash triage filters out generated-only failures
- reporting produces a root summary plus per-folder summaries

The spec should be simple enough that when propose runs on a new project, the generated framework behaves consistently and emits the same categories of outputs.

---

## 19. Constraints

- everything remains under `fuzz/stats/`
- must support large codebase operation
- must be CI/CD compatible
- must run via single-command execution
- avoid excessive seed explosion
- prefer bounded meaningful structured fuzzing over raw random bytes
- use real source code from configured folders without silently missing files
- keep the design generic and placeholder-driven

---

## 20. Optional Project Hooks

If the target project already has dependency/bootstrap scripts, the framework may reference them as hints only.

Examples of acceptable hook categories:
- dependency bootstrap scripts
- project include path generators
- local SDK setup helpers
- coverage-build helper scripts

These hooks may inform dependency discovery, but the fuzz framework should remain self-contained in generation, build, execution, triage, and reporting.

---

## 21. Deliverables

The propose stage should produce a framework plan and implementation shape that includes:
- generator for callable discovery
- stub generator for dependency and platform abstraction
- corpus generator for argument-aware and JSON-aware seeds
- reporting/triage pipeline
- generated build system under `fuzz/stats/auto/`
- `run_fuzz.sh`
- `auto_fuzz.sh`
- isolated corpus, build, crash, result, and report directories

This spec intentionally omits project-specific counts, folder names, and metrics so it can be reused as a clean template across projects.
