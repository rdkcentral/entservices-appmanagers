# Download Manager

## Overview
Specification for the Download Manager capability, which provides queued download execution, pause/resume/cancel controls, progress reporting, bandwidth configuration, and transient-failure retry for file transfers.

## Description
The Download Manager accepts download requests containing a URL and destination context, queues them, and executes them using supported network protocols via `DownloadManagerHttpClient`. It exposes controls to pause, resume, and cancel individual downloads, reports progress, supports configurable bandwidth rate limits, and automatically retries transient failures within a configured budget. All failures are normalised to a canonical failure taxonomy.

## Requirements

### Requirement: Queue and execute downloads
The system SHALL queue download requests and execute them using supported network protocols.

#### Scenario: Queue download
Given a download request with URL and destination context
When the request is submitted
Then the system adds the request to its managed queue
And begins execution according to scheduler policy

#### Scenario: Queue rejects malformed request
Given request omits mandatory URL or destination input
When queue submission is attempted
Then the system returns invalid-parameter failure

### Requirement: Support pause, resume, and cancel controls
The system SHALL support pausing, resuming, and canceling individual downloads.

#### Scenario: Pause and resume
Given a download is in progress
When pause is requested and later resume is requested
Then transfer progress pauses and later continues from the most recent supported offset/state

#### Scenario: Cancel queued request
Given a download is queued and not yet started
When cancel is requested
Then the request is removed from queue and reported as canceled

### Requirement: Report download progress and status
The system SHALL provide observable status/progress updates for download operations.

#### Scenario: Progress observation
Given an active download
When progress is queried or emitted
Then the system reports current transferred size and completion state

#### Scenario: Progress request for unknown download id
Given no download matches the provided id
When progress is requested
Then the system returns not-found failure

### Requirement: Support configurable bandwidth control
The system SHALL allow rate limits to be set for download operations.

#### Scenario: Apply rate limit
Given an active or queued download
When a rate limit is set
Then transfer behavior honors the configured limit within runtime/network constraints

#### Scenario: Invalid rate limit rejected
Given rate limit is zero or outside accepted bounds
When rate limit is requested
Then the system returns invalid-parameter failure

### Requirement: Retry transient failures
The system SHALL retry eligible transient download failures according to configured retry behavior.

#### Scenario: Transient network failure retry
Given a download encounters a transient network error
When retry budget remains
Then the system retries the download before marking it failed

#### Scenario: Retry budget exhausted
Given transient failures continue until retry budget is exhausted
When final retry fails
Then download is marked failed with final error reason

### Requirement: Use standardized failure taxonomy
The system SHALL classify operation failures using canonical categories: INVALID_ARGUMENT, NOT_FOUND, INVALID_STATE, DEPENDENCY_UNAVAILABLE, UNSUPPORTED_OPERATION, TIMEOUT, PERMISSION_DENIED, TRANSIENT_FAILURE, and INTERNAL_ERROR.

#### Scenario: Consistent category mapping
Given a download operation fails
When error is returned
Then one canonical taxonomy category is present
And equivalent failures map consistently across queue/control APIs

#### Scenario: Preserve transport detail
Given network/transport layer returns native failure detail
When failure is propagated
Then canonical category is returned
And native detail is preserved as supplemental context

## Architecture / Design
Design is documented in [openspec/specs/download-manager/design.md](design.md). Key components:
- **Plugin facade** (`DownloadManager.cpp/.h`) — Thunder RPC registration.
- **Implementation** (`DownloadManagerImplementation.cpp/.h`) — queue management, scheduler, and control logic.
- **HTTP client** (`DownloadManagerHttpClient.cpp/.h`) — network transfer layer.
- **Telemetry bridge** (`DownloadManagerTelemetryReporting.cpp/.h`) — timing markers.

## External Interfaces
_The DownloadManager plugin exposes the following Thunder JSON-RPC methods (confirm against generated stubs):_
- `download(url, destination, [options])` → `{downloadId}` or failure
- `pause(downloadId)` → success/failure
- `resume(downloadId)` → success/failure
- `cancel(downloadId)` → success/failure
- `getProgress(downloadId)` → `{transferred, total, state}` or failure
- `setRateLimit(downloadId, bytesPerSecond)` → success/failure
- **Events:** `onDownloadStateChanged(downloadId, state, reason)`

_Confirm exact parameter names and types against the generated Thunder interface definition._

## Performance
_Not yet defined — add latency and throughput targets when SLAs are established._

## Security
- URL inputs are validated at the API boundary to prevent path traversal in destination handling.
- Destination paths must be within allowed directories; callers cannot escape sandbox.
- _Threat model not yet formal — add as follow-up._

## Versioning & Compatibility
_Not yet defined — add versioning scheme and backward-compatibility guarantees when the API is stabilised._

## Conformance Testing & Validation
- **L1 tests:** [Tests/L1Tests/tests/test_DownloadManager.cpp](../../../Tests/L1Tests/tests/test_DownloadManager.cpp)
- _Test documentation not yet written — add as follow-up._

## Covered Code
- DownloadManager/DownloadManager.cpp — `DownloadManager` (plugin facade)
- DownloadManager/DownloadManager.h — `DownloadManager`
- DownloadManager/DownloadManagerImplementation.cpp — `DownloadManagerImplementation`
- DownloadManager/DownloadManagerImplementation.h — `DownloadManagerImplementation`
- DownloadManager/DownloadManagerHttpClient.cpp — `DownloadManagerHttpClient`
- DownloadManager/DownloadManagerHttpClient.h — `DownloadManagerHttpClient`
- DownloadManager/DownloadManagerTelemetryReporting.cpp — `DownloadManagerTelemetryReporting`
- DownloadManager/DownloadManagerTelemetryReporting.h — `DownloadManagerTelemetryReporting`
- DownloadManager/Module.cpp, DownloadManager/Module.h — Thunder module registration

---

## Open Queries
- External interface parameter names and types need verification against generated stubs.
- Destination path sandboxing rules need formal specification.
- Performance (download queue latency, throughput) targets not yet defined.
- Formal versioning scheme not yet established.

## References
- [openspec/specs/download-manager/design.md](design.md)
- [openspec/specs/download-manager/requirements.md](requirements.md)

## Change History
- [2026-04-23] - openspec-templater - Restructured to match spec template.
