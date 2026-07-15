# OpenSpec Coverage Report

Date: 2026-05-02
Scope: `entservices-appmanagers`, `entservices-apis`, `libpackage-sky`, `rdk-window-manager`
Method: Repository-local audit using the OpenSpec coverage rules from `.github/skills/openspec-coverage/SKILL.md`

## Executive Summary

Overall compliance score: **58.4 / 100**

The specification suite is structurally strong at the document level: the main component specs are present, readable, and generally rich in architecture, interface, and requirement detail. The dominant weakness is **code-to-spec traceability**. Under strict OpenSpec coverage rules, only `entservices-appmanagers/spec.md` currently provides machine-resolvable `## Covered Code` mappings. The other three component specs include a `## Covered Code` section, but the content is not in the required repo-root file mapping format, so an automated coverage tool cannot reliably resolve those references.

## Weighted Score Breakdown

| Category | Score | Weight | Notes |
|---|---:|---:|---|
| Code to Spec Coverage | 17.4 / 40 | 40% | Main gap. Only one component spec currently provides parseable, repo-root Covered Code mappings. |
| Architecture HLA Specification | 9.0 / 10 | 10% | Strong architecture narrative and flow diagrams; traceability from HLA to code is only partial. |
| Performance Specification | 6.0 / 10 | 10% | Good targets in multiple specs, but one component lacks a performance section and measured results are not tracked. |
| External Interface Specification | 8.0 / 10 | 10% | Strong interface detail and data definitions; examples/validation are present but uneven. |
| Security Specification | 6.0 / 10 | 10% | Security sections exist in most component specs, but threat modeling and validation evidence are limited. |
| Versioning & Compatibility | 6.0 / 10 | 10% | Present in most specs, but not all; migration guidance is partial. |
| Conformance Testing & Validation | 6.0 / 10 | 10% | Test plans exist in most specs, but actual validation results are not tracked. |
| **Total** | **58.4 / 100** | **100%** | |

## Code-to-Spec Coverage Calculation

### Raw Coverage Signals

- C/C++ files scanned across the four component repositories: **689**
- Heuristically extracted methods/functions: **10,921**
- Files currently covered by parseable `## Covered Code` mappings: **53**
- Methods inside those mapped files: **1,063**
- `// Spec: <spec_name>` comments found in code: **0**

### OpenSpec Coverage Subscores

| Sub-criterion | Score | Basis |
|---|---:|---|
| Reference Coverage | 1.9 / 20 | `1,063 / 10,921 = 9.73%` strict coverage |
| Spec Existence | 10.0 / 10 | All four audited component spec files exist |
| Spec Completeness | 5.0 / 5 | All four audited `spec.md` files contain `Overview`, `Description`, and `Requirements` |
| No Orphaned Code | 0.5 / 5 | Same strict traceability limitation as reference coverage |
| **Code to Spec Total** | **17.4 / 40** | |

## Key Findings

### Strengths

1. The four main component spec files are present and complete at the baseline template level.
2. The architecture narrative provides strong system-level context, layering, and flow diagrams.
3. External interfaces are documented in substantial detail, especially for `entservices-apis` and `entservices-appmanagers`.
4. `entservices-appmanagers/spec.md` now uses the correct OpenSpec `## Covered Code` structure and is machine-resolvable.

### Main Gaps

1. **Three `## Covered Code` sections are not in the required format.**
   - `entservices-apis/spec.md`, `libpackage-sky/spec.md`, and `rdk-window-manager/spec.md` use descriptive subsections like `### include/` and bold file names rather than repo-root entries in the required `- path/file:` format.
   - Result: automated coverage tooling resolves **0 files** from those three specs.

2. **Strict traceability is concentrated in one component only.**
   - `entservices-appmanagers`: 53 mapped files
   - `entservices-apis`: 0 mapped files under strict parsing
   - `libpackage-sky`: 0 mapped files under strict parsing
   - `rdk-window-manager`: 0 mapped files under strict parsing

3. **Some cross-cutting sections are missing from `libpackage-sky/spec.md`.**
   - `Performance`
   - `Security`
   - `Versioning & Compatibility`
   - `Conformance Testing & Validation`

4. **Validation evidence is mostly prospective, not historical.**
   - Multiple specs define targets and test plans, but there is no tracked benchmark output, conformance run history, or pass/fail evidence in the spec suite.

5. **No supplementary `// Spec:` comments are present in code.**
   - This leaves the suite entirely dependent on the `## Covered Code` sections for traceability.

## Evidence Highlights

### Architecture HLA

- Strong system architecture narrative and layered decomposition are present in `openspec/specs/system-integration/architecture-narrative.md`.
- Flow diagrams and end-to-end scenarios are explicitly documented there.

### External Interfaces

- `openspec/specs/entservices-apis/spec.md` includes a dedicated `## External Interfaces` section with interface signature patterns, I/O structures, notifications, and category breakdowns.
- `openspec/specs/entservices-appmanagers/spec.md` includes concrete `IAppManager` and `IPackageManager` method signatures.

### Performance / Security / Versioning / Conformance

- `openspec/specs/rdk-window-manager/spec.md` contains dedicated sections for all four categories, with explicit targets and test expectations.
- `openspec/specs/entservices-apis/spec.md` and `openspec/specs/entservices-appmanagers/spec.md` also include these sections.
- `openspec/specs/libpackage-sky/spec.md` currently stops after `Architecture / Design`, `External Interfaces`, `Covered Code`, `Open Queries`, `References`, and `Change History`.

## Missing / Mismatched Coverage

### Specs with strict, machine-readable Covered Code

- `openspec/specs/entservices-appmanagers/spec.md`

### Specs with non-compliant Covered Code formatting

- `openspec/specs/entservices-apis/spec.md`
- `openspec/specs/libpackage-sky/spec.md`
- `openspec/specs/rdk-window-manager/spec.md`

### Example Strict-Parsing Problem

Current non-compliant pattern:

```md
## Covered Code

### include/
- **Ids.h**
  - #define ID_APPMANAGER, ID_PACKAGEMANAGER, ...
```

Required OpenSpec pattern:

```md
## Covered Code
- entservices-apis/Ids.h:
    - ID_APPMANAGER
    - ID_PACKAGEMANAGER
```

## Orphaned Code Assessment

Under strict parsing, approximately **90.27%** of discovered methods/functions remain orphaned from any machine-resolvable spec mapping.

This is not primarily a documentation absence problem. It is a **traceability formatting problem**:

- the specs exist
- the covered code intent exists
- the mappings are not encoded in a format the coverage workflow can consume

## Recommendations

1. Rework `## Covered Code` in the three remaining component specs into strict repo-root mappings.
2. Add the missing `Performance`, `Security`, `Versioning & Compatibility`, and `Conformance Testing & Validation` sections to `libpackage-sky/spec.md`.
3. Decide whether to add supplementary `// Spec: <spec_name>` comments in high-value implementation files for secondary traceability.
4. Add measurable validation artifacts to the specs or adjacent reports:
   - benchmark results
   - conformance run summaries
   - security validation notes
5. Re-run coverage after those changes; the score should improve materially without requiring broad content rewrites.

## Audit Notes

- The code coverage calculation is **strict** and follows the OpenSpec skill’s requirement that `## Covered Code` be the primary signal.
- Function/method extraction from C/C++ is heuristic, so the absolute method count should be treated as an audit metric rather than a compiler-verified symbol inventory.
- The dominant result is robust despite that heuristic: only one component spec is currently machine-resolvable for coverage.
