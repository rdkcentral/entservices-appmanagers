# TelemetryMetrics Specification

## Overview
**Module Name**: TelemetryMetrics
**Type**: WPEFramework Plugin
**Purpose**: Centralized metrics recording and publishing for performance analytics and operational monitoring

## Description
TelemetryMetrics is a WPEFramework plugin that records JSON-formatted metrics in-memory and publishes them to the platform telemetry bus (T2) when `Publish` is invoked. Metrics are stored in an internal map keyed by `"<id>:<markerName>"` and are cleared after a successful publish. Other modules may optionally emit telemetry when built with `AIMANAGERS_TELEMETRY_METRICS_SUPPORT`, but the TelemetryMetrics plugin itself is enabled/disabled independently.

## Requirements
- Provide JSON-RPC API for `Record(id, metrics, name)` and `Publish(id, name)` operations
- Parse `metrics` as a JSON object and merge it into the stored record for the key `"<id>:<name>"`
- On `Publish`, filter/merge metrics according to marker filters and emit a T2 telemetry event (`t2_event_s`)
- Record/Publish failures must not crash or deadlock callers; failures are reported via the returned `hresult`

## Architecture / Design

### High-Level Architecture
```
AppManager / LifecycleManager / PackageManager /
RuntimeManager / DownloadManager
    │  (compiled with AIMANAGERS_TELEMETRY_METRICS_SUPPORT)
    ↓
TelemetryMetrics (JSON-RPC)
    ↓
TelemetryMetricsImplementation
    ├→ Record(id, metrics, markerName)
    │   └→ Buffer in unordered_map<recordId, metricsJson>
    └→ Publish(id, markerName)
        ├→ Filter metrics by recordId + markerName
        ├→ Merge other records with same appInstanceId
        └→ Emit T2 telemetry event via t2_event_s(markerName, publishMetrics)
```

### Metrics Recorded Per Module
| Source Module | Example Metrics |
|---------------|----------------|
| AppManager | app launch time, app close time, errors |
| LifecycleManager | state transition time, state duration |
| PackageManager | install time, download time |
| RuntimeManager | container start time, suspend time, wake time |
| DownloadManager | download speed, download duration, errors |

### Key Components
- **TelemetryMetricsImplementation**: Core logic — `Record`, `Publish`, in-memory metric buffer management
- **TelemetryFilters**: Filter helpers for extracting and merging metrics by `appInstanceId` and `markerName`
- **TelemetryMetrics**: Plugin entry point — `Initialize`, `Deinitialize`

## External Interfaces

### Public APIs (JSON-RPC)
| Method | Description |
|--------|-------------|
| `Record(id, metrics, name)` | Record a metrics payload identified by id and marker name |
| `Publish(id, name)` | Flush and publish buffered metrics for the given id and marker name |

### Publish Payload Format
```json
{
  "metrics": [
    {"name": "app_launch_time", "value": 2500, "unit": "ms", "timestamp": "..."},
    {"name": "container_start_time", "value": 1200, "unit": "ms", "timestamp": "..."}
  ]
}
```

### Configuration
- **Config File**: `TelemetryMetrics.conf.in` / `TelemetryMetrics.config`
- **Callsign**: `org.rdk.TelemetryMetrics`
- **Thunder Service**: Exposed via WPEFramework/Thunder JSON-RPC

## Data Flow
```
Module (e.g. AppManager) with AIMANAGERS_TELEMETRY_METRICS_SUPPORT:
    ├→ Before operation: TelemetryMetrics::Record(id, startMetrics, "start")
    └→ After operation:  TelemetryMetrics::Record(id, durationMetrics, "complete")

TelemetryMetricsImplementation::Record(id, metrics, markerName):
    ├→ Parse metrics JSON
    ├→ Store in mMetricsRecord map keyed by "<id>:<markerName>"
    └→ Return Core::ERROR_NONE on success

TelemetryMetricsImplementation::Publish(id, markerName):
    ├→ Filter mMetricsRecord for matching "<id>:<markerName>"
    ├→ Merge other records with same appInstanceId and markerName
    ├→ Serialize to publishMetrics JSON string
    └→ Emit T2 telemetry event via t2_event_s(markerName, publishMetrics)

Query (not a current API):
    _Not currently exposed via JSON-RPC — metrics are publish-only._
```

## Integration Points
- **Consumes from**:
  - AppManager (app lifecycle event metrics)
  - LifecycleManager (state transition metrics)
  - PackageManager (install and download metrics)
  - RuntimeManager (container operation metrics)
  - DownloadManager (download progress and error metrics)
- **Provides to**:
  - Platform telemetry bus (T2) via `t2_event_s`
  - Monitoring and analytics systems

## Performance
- Metrics are buffered in-memory (`unordered_map`); publishing is triggered on `Publish` calls
- Publishing failures are non-blocking and do not affect calling module operations
- Metric buffer is currently unbounded (in-memory `unordered_map`); records are cleared when successfully published.

## Security
_Not applicable — TelemetryMetrics operates on internal performance data only; no user PII is expected in metrics._

## Versioning & Compatibility
- **API Version**: 1.0.0
- **Plugin Execution Mode**: Configurable via `PLUGIN_TELEMETRY_METRICS_MODE`
- Entire module is optional; absent `AIMANAGERS_TELEMETRY_METRICS_SUPPORT`, no metrics code is compiled into any module
- Metrics are not persisted across daemon restarts (in-memory only)

## Conformance Testing & Validation
- **Unit Tests**: None for TelemetryMetrics plugin itself; TelemetryMetricsClient utilities are exercised in `Tests/L0Tests/LifecycleManager/LifecycleManager_ContextTests.cpp`
- **Integration Tests**: None currently in `Tests/L2Tests/`
- **Test Dependencies**: N/A

## Dependencies

### External Repositories
| Repository | Purpose | Optional |
|------------|---------|----------|
| entservices-apis | API interface definitions (ITelemetryMetrics) | No |

### System Libraries
| Library | Version | Purpose | Optional |
|---------|---------|---------|----------|
| WPEFramework | Latest | Plugin framework | No |
| jsoncpp | - | JSON serialization for metrics payloads | No |

### Internal Dependencies
| Module | Type | Purpose |
|--------|------|---------|
| AppManagersHelpers | Library | Shared utilities, logging |

### CMake Configuration
| Option | Type | Default | Purpose |
|--------|------|---------|---------|
| AIMANAGERS_TELEMETRY_METRICS_SUPPORT | ON/OFF | OFF | Enable telemetry reporting in consumer modules (adds ENABLE_AIMANAGERS_TELEMETRY_METRICS) |
| PLUGIN_TELEMETRY_METRICS_MODE | String | Off | Plugin execution mode |
| PLUGIN_TELEMETRY_METRICS_AUTOSTART | Boolean | false | Auto-start on boot |
| APPMANAGERS_ENABLE_T2_INIT | ON/OFF | OFF | Enable T2 init/uninit in TelemetryMetricsImplementation |

## Build & Installation
- **Compiled Artifacts**: `lib${NAMESPACE}TelemetryMetrics.so`, `lib${NAMESPACE}TelemetryMetricsImplementation.so`
- **Install Path**: `${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins`
- **Required Build Flags**: C++11
- **API Version**: 1.0.0
- **Build Notes**: `AIMANAGERS_TELEMETRY_METRICS_SUPPORT` controls telemetry emission in consumer modules; the TelemetryMetrics plugin is enabled separately (e.g., via `PLUGIN_TELEMETRYMETRICS`).
## Constraints & Limitations
- Metrics are not persisted across daemon restarts (in-memory buffer only)
- Telemetry is opt-in per build (requires compile-time flag)
- Publishing failures do not block module operations
- Metric buffer has a size limit; oldest metrics are discarded if exceeded
- Network unavailability does not prevent metric recording, only publishing

## Covered Code
- TelemetryMetrics/TelemetryMetricsImplementation.cpp:
    - TelemetryMetricsImplementation::Record
    - TelemetryMetricsImplementation::Publish
- TelemetryMetrics/TelemetryMetricsImplementation.h:
    - TelemetryMetricsImplementation
- TelemetryMetrics/TelemetryMetrics.cpp:
    - TelemetryMetrics::Deinitialize
- TelemetryMetrics/TelemetryMetrics.h:
    - TelemetryMetrics
- TelemetryMetrics/TelemetryFilters.h:
    - TelemetryFilters

## Open Queries
- What is the maximum metric buffer size before oldest entries are discarded — is this configurable?
- Is `Publish` called automatically on a timer interval, or only when explicitly invoked by a module?
- How are marker filters configured/updated (e.g., compile-time constants vs config file vs runtime API)?
- Are there plans to expose a `GetMetrics` or `ClearMetrics` JSON-RPC API in a future version?

## References
- Architecture: [TelemetryMetrics.md](../../TelemetryMetrics/TelemetryMetrics.md)
- Build Configuration: [CMakeLists.txt](../../TelemetryMetrics/CMakeLists.txt)
- Related Spec: [AppManager.spec.md](./AppManager.spec.md)
- Related Spec: [LifecycleManager.spec.md](./LifecycleManager.spec.md)

## Change History
- [2026-06-06] - openspec-templater - Restructured to match spec template.
