# EntServices-AppManagers Plugin Spec - DownloadManager

## Overview
DownloadManager provides asynchronous package download control, progress reporting, and download-state lifecycle operations for application package flows.

## Description
This plugin encapsulates HTTP download operations and client-facing controls such as start/pause/resume/cancel/delete/progress/rate-limit. It integrates telemetry reporting for download outcomes and durations.

## Requirements
### Requirement: DownloadManager supports asynchronous download operations
DownloadManager MUST perform non-blocking downloads and expose operation handles for subsequent control actions.

#### Scenario: Start download
- **WHEN** a client requests package download
- **THEN** plugin schedules asynchronous transfer and returns status/handle information

### Requirement: DownloadManager supports operational controls
DownloadManager SHALL support pause, resume, cancel, delete, progress, and bandwidth rate-limit operations.

#### Scenario: Pause and resume transfer
- **WHEN** a client pauses and later resumes an active download
- **THEN** plugin transitions state coherently and continues transfer from persisted progress state

### Requirement: DownloadManager publishes telemetry and status events
DownloadManager MUST emit status/telemetry signals for success and failure paths.

#### Scenario: Completion and error telemetry
- **WHEN** a download completes or fails
- **THEN** plugin records telemetry and emits corresponding status event

## Architecture / Design (if applicable)
- DownloadManagerImplementation coordinates request lifecycle and worker threads.
- DownloadManagerHttpClient encapsulates network callbacks and write/progress handling.

## External Interfaces (if applicable)
- Download events (`Exchange::JDownloadManager::Event::OnAppDownloadStatus`)
- Control APIs (`Download`, `Pause`, `Resume`, `Cancel`, `Delete`, `Progress`, `RateLimit`)

## Performance (if applicable)
- Concurrent downloads SHOULD maintain responsive control operations and progress updates.

## Security (if applicable)
- Download requests MUST validate source/target constraints and avoid unsafe file write paths.

## Versioning & Compatibility (if applicable)
- Control operation semantics SHALL remain compatible for existing clients across minor revisions.

## Conformance Testing & Validation (if applicable)
- Validate async control transitions, progress accounting, and error handling with integration tests.

## Covered Code
- entservices-appmanagers/DownloadManager/DownloadManager.cpp:
    - DownloadManager::Initialize
    - DownloadManager::Deinitialize
- entservices-appmanagers/DownloadManager/DownloadManagerImplementation.cpp:
    - DownloadManagerImplementation::Download
    - DownloadManagerImplementation::Pause
    - DownloadManagerImplementation::Resume
    - DownloadManagerImplementation::Cancel
    - DownloadManagerImplementation::Delete
    - DownloadManagerImplementation::Progress
    - DownloadManagerImplementation::RateLimit
- entservices-appmanagers/DownloadManager/DownloadManagerHttpClient.cpp:
    - DownloadManagerHttpClient::downloadFile
    - DownloadManagerHttpClient::progressCb
    - DownloadManagerHttpClient::write_data

---

## Open Queries
- Should DownloadManager persist resumable state across process restarts by default?

## References
- entservices-appmanagers/DownloadManager/DownloadManager.cpp
- entservices-appmanagers/DownloadManager/DownloadManagerImplementation.cpp
- entservices-appmanagers/DownloadManager/DownloadManagerHttpClient.cpp

## Change History
- [2026-05-03] - openspec-explore - Added per-plugin DownloadManager specification.
