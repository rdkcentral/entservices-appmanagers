## Context

The repository contains 9 plugin modules (AppManager, AppStorageManager, DownloadManager, LifecycleManager, PackageManager, PreinstallManager, RDKWindowManager, RuntimeManager, TelemetryMetrics) plus a shared `helpers/` module. Each plugin has a corresponding `openspec/specs/<plugin>/spec.md`, but as of the 2026-04-23 coverage audit:

- No production `.cpp` or `.h` file contains a `// Spec: <spec-name>` reference comment.
- 8 of 9 `spec.md` files are missing Overview, Description, Covered Code, Open Queries, and Change History sections (only `app-storage-manager` was recently updated).
- External Interfaces, Performance, and Versioning sections are absent from all specs.
- Security is only present in 4 of 9 design files, not in spec files.
- Test files exist at L0/L1/L2 levels but are not cross-referenced in spec documents.

This is a documentation-and-traceability change only. No production logic is modified.

## Goals / Non-Goals

**Goals:**
- Add inline `// Spec: <spec-name>` comments to all production `.cpp` and `.h` files per the Openspec convention.
- Complete all 9 `spec.md` files to the standard template (Overview, Description, Requirements, Architecture/Design, External Interfaces, Performance, Security, Versioning, Conformance Testing, Covered Code, Open Queries, References, Change History).
- Populate Covered Code mappings pointing to the actual implementation files and key classes/methods.
- Raise the Openspec coverage score from 28/100 to ≥65/100.

**Non-Goals:**
- Modifying any production logic, build scripts, or interfaces.
- Writing new test cases (existing tests are referenced, not authored here).
- Defining formal threat models or performance benchmarks (stubs are sufficient for this pass).
- Updating `helpers/` spec (deferred — helpers module spans many cross-cutting concerns).

## Decisions

### Decision 1: Use file-level `// Spec:` comments, not per-method

**Choice:** Place a single `// Spec: <spec-name>` comment at the top of each file (after license header if present), not inline per method.

**Rationale:** Per-method comments are labour-intensive and drift quickly. A file-level comment establishes the primary spec association with minimal noise. The Covered Code section in the spec provides method-level traceability.

**Alternatives considered:**
- Per-method `@spec` annotations — rejected: too verbose, no tooling to enforce.
- Separate traceability matrix file — rejected: duplicates information already in spec Covered Code.

### Decision 2: Add spec sections to existing `spec.md` files rather than create new files

**Choice:** Extend each `spec.md` in-place with the missing template sections rather than splitting into separate `interfaces.md`, `security.md`, etc.

**Rationale:** Keeps all spec information discoverable in one place per module. The template already accommodates all required sections.

**Alternatives considered:**
- Separate `interfaces.md` per module — rejected: increases navigation burden without benefit.

### Decision 3: Use `_Not applicable_ / _Not yet defined_` stubs for unknown section content

**Choice:** Sections with insufficient information are populated with placeholder stubs and Open Queries entries rather than left empty or omitted.

**Rationale:** A populated-but-flagged section scores higher on completeness checks and explicitly signals what is missing to reviewers.

## Risks / Trade-offs

- **Risk: Spec comments become stale if files are moved or refactored** → Mitigation: add spec reference validation to future CI as follow-up work.
- **Risk: Covered Code mappings are incomplete on first pass** → Mitigation: list primary implementation files at minimum; flag gaps in Open Queries.
- **Risk: Interface stubs may be misleading if incorrect** → Mitigation: use hedged language ("the plugin exposes the following methods — confirm against generated Thunder stubs") and mark as open query.
- **Trade-off:** Stub sections improve coverage score immediately but require follow-up enrichment to provide real value to readers. This is acceptable for the first traceability pass.
