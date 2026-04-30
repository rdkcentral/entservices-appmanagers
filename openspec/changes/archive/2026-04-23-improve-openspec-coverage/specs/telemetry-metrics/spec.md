## ADDED Requirements

### Requirement: Spec traceability comments in all TelemetryMetrics source files
All production `.cpp` and `.h` files in the TelemetryMetrics module SHALL contain a `// Spec: telemetry-metrics` comment at the top of the file to establish code-to-spec traceability.

#### Scenario: Source file contains spec reference
- **WHEN** any TelemetryMetrics `.cpp` or `.h` production file is opened
- **THEN** a `// Spec: telemetry-metrics` comment is present at the top of the file

### Requirement: TelemetryMetrics spec completeness
The `openspec/specs/telemetry-metrics/spec.md` SHALL contain all mandatory template sections.

#### Scenario: All required sections present
- **WHEN** the TelemetryMetrics spec.md is reviewed
- **THEN** all 13 mandatory sections are present and non-empty
