# TelemetryMetrics Specification

## Overview
**Module Name**: TelemetryMetrics
**Type**: WPEFramework Plugin
**Purpose**: Centralized metrics recording and publishing for performance analytics and operational monitoring

## Description
TelemetryMetrics provides a centralized service for recording, buffering, and publishing performance metrics from across all AppManagers modules. It is an optional, compile-time-gated component controlled by the `AIMANAGERS_TELEMETRY_METRICS_SUPPORT` build flag. When enabled, each module (AppManager, LifecycleManager, RuntimeManager, PackageManager, DownloadManager) instruments key operations with `Record` and `Publish` calls. Metrics are stored in-memory and published to a configurable backend endpoint. The core API exposes `Record` (store a metric entry) and `Publish` (flush buffered metrics for a given record ID and marker name).

## Requirements
- Provide JSON-RPC API for `Record(id, metrics, name)` and `Publish(id, name)` operations
- Buffer recorded metrics in-memory keyed by record ID and marker name
- Merge metrics with the same `appInstanceId` and `markerName` on publish
- Publish buffered metrics to a configurable telemetry backend endpoint
- Entire module is optional and gated by `AIMANAGERS_TELEMETRY_METRICS_SUPPORT` compile flag
- Auto-start on boot when enabled (`PLUGIN_TELEMETRY_METRICS_AUTOSTART`)
- All consumer modules (AppManager, LifecycleManager, etc.) must be compiled with `AIMANAGERS_TELEMETRY_METRICS_SUPPORT` to emit metrics
- Publishing failures must not block or affect module operations
- Metric buffer has a finite size; oldest metrics are discarded when the limit is exceeded

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
        ├→ Merge metrics with same appInstanceId
        └→ POST to TELEMETRY_PUBLISH_ENDPOINT
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
POST {TELEMETRY_PUBLISH_ENDPOINT}
{
  "metrics": [
    {"name": "app_launch_time", "value": 2500, "unit": "ms", "timestamp": "..."},
    {"name": "container_start_time", "value": 1200, "unit": "ms", "timestamp": "..."}
  ]
}
```

### Configuration
- **Config File**: `TelemetryMetrics.conf.in` / `TelemetryMetrics.config`
- **Publish Endpoint**: Configurable via `TELEMETRY_PUBLISH_ENDPOINT`
- **DBus Service**: Registered as WPEFramework plugin (when enabled)

## Data Flow
```
Module (e.g. AppManager) with AIMANAGERS_TELEMETRY_METRICS_SUPPORT:
    ├→ Before operation: TelemetryMetrics::Record(id, startMetrics, "start")
    └→ After operation:  TelemetryMetrics::Record(id, durationMetrics, "complete")

TelemetryMetricsImplementation::Record(id, metrics, markerName):
    ├→ Parse metrics JSON
    ├→ Store in mRecordMetrics map keyed by (id, markerName)
    └→ Return Core::ERROR_NONE on success

TelemetryMetricsImplementation::Publish(id, markerName):
    ├→ Filter mRecordMetrics for matching (id, markerName)
    ├→ Extract appInstanceId from filtered entries
    ├→ Merge other records with same appInstanceId and markerName
    ├→ Serialize to publishMetrics JSON string
    └→ POST to TELEMETRY_PUBLISH_ENDPOINT

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
  - Telemetry backend (metric publishing via HTTP POST)
  - Monitoring and analytics systems

## Performance
- Metrics are buffered in-memory (`unordered_map`); publishing is triggered on `Publish` calls
- Publishing failures are non-blocking and do not affect calling module operations
- Metric buffer has a finite size; oldest entries are discarded on overflow

## Security
_Not applicable — TelemetryMetrics operates on internal performance data only; no user PII is expected in metrics._

## Versioning & Compatibility
- **API Version**: 1.0.0
- **Plugin Execution Mode**: Configurable via `PLUGIN_TELEMETRY_METRICS_MODE`
- Entire module is optional; absent `AIMANAGERS_TELEMETRY_METRICS_SUPPORT`, no metrics code is compiled into any module
- Metrics are not persisted across daemon restarts (in-memory only)

## Conformance Testing & Validation
- **Unit Tests**: `Tests/L1Tests` (metric recording and buffer management)
- **Integration Tests**: `Tests/L2Tests` (end-to-end with mock telemetry backend)
- **Test Dependencies**: Mock telemetry backend, network simulation

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
| AIMANAGERS_TELEMETRY_METRICS_SUPPORT | ON/OFF | OFF | Enable telemetry support globally (affects all modules) |
| PLUGIN_TELEMETRY_METRICS_MODE | String | Off | Plugin execution mode |
| PLUGIN_TELEMETRY_METRICS_AUTOSTART | Boolean | true | Auto-start on boot when enabled |
| TELEMETRY_PUBLISH_ENDPOINT | String | "" | Telemetry backend URL for publishing |

## Build & Installation
- **Compiled Artifact**: `RdkCppPlugin_TelemetryMetrics.so`
- **Install Path**: `${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins`
- **Required Build Flags**: C++11
- **API Version**: 1.0.0
- **Build Dependency**: `AIMANAGERS_TELEMETRY_METRICS_SUPPORT` must be ON; affects all consumer modules

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
- How is `TELEMETRY_PUBLISH_ENDPOINT` set at runtime — CMake option, config file, or JSON-RPC call?
- Are there plans to expose a `GetMetrics` or `ClearMetrics` JSON-RPC API in a future version?

## References
- Architecture: [TelemetryMetrics.md](./TelemetryMetrics.md)
- Build Configuration: [CMakeLists.txt](../TelemetryMetrics/CMakeLists.txt)
- Related Spec: [AppManager.spec.md](./AppManager.spec.md)
- Related Spec: [LifecycleManager.spec.md](./LifecycleManager.spec.md)

## Change History
- [2026-06-06] - openspec-templater - Restructured to match spec template.
