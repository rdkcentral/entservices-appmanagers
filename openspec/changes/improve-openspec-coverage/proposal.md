## Why

The Openspec coverage audit (2026-04-23) scored the repository at 28/100. The lowest-scoring areas are code-to-spec traceability (0/20 — no inline spec references), spec completeness (1/9 specs have all required sections), and missing specification categories (External Interface, Versioning, Performance all score 0). Improving these gaps now establishes a solid traceability baseline before the codebase grows further.

## What Changes

- Add `// Spec: <spec-name>` reference comments to all production `.cpp` and `.h` files in each plugin module (AppManager, AppStorageManager, DownloadManager, LifecycleManager, PackageManager, PreinstallManager, RDKWindowManager, RuntimeManager, TelemetryMetrics, helpers).
- Apply the openspec-templater to the 8 remaining `spec.md` files that are missing Overview, Description, Covered Code, Open Queries, and Change History sections.
- Add `## Covered Code` sections to each `spec.md` mapping files and key classes/methods.
- Add `## External Interfaces` sections documenting Thunder plugin API surface (methods, parameters, return types) to each spec.
- Add `## Versioning & Compatibility` sections to each spec.
- Add `## Performance` sections to each spec.
- Extend `## Security` sections to the 5 plugin specs currently missing them (AppStorageManager, DownloadManager, LifecycleManager, PreinstallManager, RuntimeManager).
- Add test documentation (`## Conformance Testing & Validation`) to each spec referencing the existing L0/L1/L2 test files.

## Capabilities

### New Capabilities

_(None — this change improves existing spec coverage only, no new plugin capabilities are introduced.)_

### Modified Capabilities

- `app-manager`: Add missing template sections (Overview, Description, External Interfaces, Versioning, Performance, Covered Code, Conformance Testing, Change History).
- `app-storage-manager`: Add missing External Interfaces, Versioning, Performance, and Covered Code mapping; extend Security section.
- `download-manager`: Add all missing template sections and Security section.
- `lifecycle-manager`: Add all missing template sections and Security section.
- `package-manager-rdkems`: Add all missing template sections.
- `preinstall-manager`: Add all missing template sections and Security section.
- `rdk-window-manager`: Add all missing template sections.
- `runtime-manager`: Add all missing template sections and Security section.
- `telemetry-metrics`: Add all missing template sections.

## Impact

- All production `.cpp` and `.h` files in plugin modules gain a spec reference comment header.
- All 9 `spec.md` files are updated (no new spec files created — only existing ones completed).
- No code logic changes; comments and spec documents only.
- Coverage score target: raise from 28/100 to ≥65/100 after implementation.
