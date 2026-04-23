# Telemetry Metrics

## Overview
Specification for the Telemetry Metrics capability, which provides per-metric-id/name recording and publishing of telemetry payloads to the configured backend integration, with support for concurrent clients and correlation context preservation.

## Description
The Telemetry Metrics plugin accepts metric record requests keyed by metric id and metric name, stores payloads in a buffer, and publishes them to the configured telemetry backend. It supports concurrent metric submissions from multiple components (e.g., AppManager, LifecycleManager, RuntimeManager) and preserves id/name association through publish operations. All failures are normalised to the canonical failure taxonomy.

## Requirements

### Requirement: Record metrics by id and name
The system SHALL record telemetry metric payloads keyed by metric id and metric name.

#### Scenario: Record metric
Given a caller provides metric id, metric data, and metric name
When record is requested
Then the metric data is stored for later publication

#### Scenario: Reject malformed record input
Given id or metric name is missing
When record is requested
Then the system returns invalid-parameter failure

### Requirement: Publish recorded metrics
The system SHALL publish recorded metrics to the configured telemetry backend path/integration.

#### Scenario: Publish metric set
Given one or more metrics are recorded for a metric id/name
When publish is requested
Then recorded metrics are emitted to telemetry backend integration

#### Scenario: Publish with no buffered metrics
Given no recorded data exists for provided id/name
When publish is requested
Then system returns deterministic empty-data result

### Requirement: Allow multiple clients to report metrics
The system SHALL support telemetry submissions from multiple app-manager components without data corruption.

#### Scenario: Concurrent metric recording
Given multiple callers record metrics concurrently
When record operations execute
Then each valid metric record is preserved without cross-request corruption

#### Scenario: Concurrent publish and record ordering
Given publish and record operations run concurrently for same id/name
When operations complete
Then published payload reflects a consistent and documented ordering rule

### Requirement: Preserve correlation context
The system SHALL preserve provided metric id/name context through record and publish operations.

#### Scenario: Publish keeps id/name association
Given metrics are recorded with a specific id and name
When publish is executed for that id/name
Then emitted telemetry can be correlated back to that same id/name context

#### Scenario: Mixed id records remain isolated
Given metrics are recorded under different ids
When one id is published
Then only metrics for the selected id are published

### Requirement: Return operation status for record and publish
The system SHALL return explicit success/failure status for record and publish operations.

#### Scenario: Backend publish failure
Given publish cannot be completed due to backend or transport error
When publish is requested
Then the system returns a failure status indicating publish did not complete successfully

#### Scenario: Backend transient failure and retry policy
Given publish fails transiently and retry policy is configured
When publish is requested
Then system retries per policy before final success or failure is returned

### Requirement: Use standardized failure taxonomy
The system SHALL classify operation failures using canonical categories: INVALID_ARGUMENT, NOT_FOUND, INVALID_STATE, DEPENDENCY_UNAVAILABLE, UNSUPPORTED_OPERATION, TIMEOUT, PERMISSION_DENIED, TRANSIENT_FAILURE, and INTERNAL_ERROR.

#### Scenario: Consistent category mapping
Given record or publish fails
When failure is returned
Then one canonical taxonomy category is included
And equivalent telemetry failures map consistently across APIs

#### Scenario: Preserve backend detail
Given telemetry backend returns native failure detail
When failure is propagated
Then canonical category is returned
And native detail is preserved as supplemental context

## Architecture / Design
Design is documented in [openspec/specs/telemetry-metrics/design.md](design.md). Key components:
- **Plugin facade** (`TelemetryMetrics.cpp/.h`) — Thunder RPC registration.
- **Implementation** (`TelemetryMetricsImplementation.cpp/.h`) — metric buffering and publish logic.
- **Filters** (`TelemetryFilters.h`) — filter configuration.

## External Interfaces
_The TelemetryMetrics plugin exposes the following Thunder JSON-RPC methods (confirm against generated stubs):_
- `recordMetric(id, name, data)` → success/failure
- `publishMetrics(id, name)` → success/failure

_Confirm exact parameter names and types against the generated Thunder interface definition._

## Performance
_Not yet defined — add record latency and publish latency targets when SLAs are established._

## Security
- Metric data should not contain credentials or PII — callers are responsible for sanitisation before calling `recordMetric`.
- Backend publish endpoint credentials and connection security are managed by the backend integration layer, not this plugin.
- _Privacy implications of telemetry data collection are partially addressed in design.md Security and Privacy section — formalise as follow-up._

## Versioning & Compatibility
_Not yet defined — add versioning scheme and backward-compatibility guarantees when the API is stabilised._

## Conformance Testing & Validation
_No dedicated test file found for TelemetryMetrics — test coverage needs to be added._

## Covered Code
- TelemetryMetrics/TelemetryMetrics.cpp — `TelemetryMetrics` (plugin facade)
- TelemetryMetrics/TelemetryMetrics.h — `TelemetryMetrics`
- TelemetryMetrics/TelemetryMetricsImplementation.cpp — `TelemetryMetricsImplementation`
- TelemetryMetrics/TelemetryMetricsImplementation.h — `TelemetryMetricsImplementation`
- TelemetryMetrics/TelemetryFilters.h — filter definitions
- TelemetryMetrics/Module.cpp, TelemetryMetrics/Module.h — Thunder module registration

---

## Open Queries
- No L0/L1/L2 test file exists for TelemetryMetrics — test coverage should be added.
- PII/credential sanitisation responsibility (caller vs. plugin) should be explicitly specified.
- Performance targets not yet defined.
- Formal versioning scheme not yet established.

## References
- [openspec/specs/telemetry-metrics/design.md](design.md)
- [openspec/specs/telemetry-metrics/requirements.md](requirements.md)

## Change History
- [2026-04-23] - openspec-templater - Restructured to match spec template.
