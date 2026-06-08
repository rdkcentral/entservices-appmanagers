# DownloadManager Specification

## Overview
**Module Name**: DownloadManager
**Type**: WPEFramework Plugin
**Purpose**: HTTP download service with priority queuing, pause/resume, rate limiting, and progress reporting

## Description
DownloadManager is a standalone HTTP download service for RDK-based devices. It provides a JSON-RPC API for initiating, pausing, resuming, cancelling, and rate-limiting HTTP downloads using libcurl. Downloads are processed from a priority queue by a dedicated downloader thread (`downloaderRoutine`). It reports real-time progress, completion, and error events to registered listeners via notifications. It is consumed primarily by PackageManager as an optional delegate for HTTP package downloads.

## Requirements
- Provide JSON-RPC API for `Download`, `Pause`, `Resume`, `Cancel`, `RateLimit`, `Progress`, and `Delete`
- Download files via HTTP/HTTPS using libcurl (`DownloadManagerHttpClient`)
- Process downloads sequentially from a priority queue via a dedicated background thread
- Support pause via `curl_easy_pause` and resume with continuation
- Support per-download bandwidth rate limiting via `CURLOPT_MAX_RECV_SPEED_LARGE`
- Emit `OnProgressUpdate`, `OnComplete`, `OnError`, and `OnPaused` notifications
- Implement automatic retry with exponential backoff on transient failures
- Report detailed error codes and messages on failure
- Validate that a network connection is available before starting a download
- Validate that the download URL is non-empty before queuing
- Optionally emit telemetry metrics when `AIMANAGERS_TELEMETRY_METRICS_SUPPORT` is enabled

## Architecture / Design

### High-Level Architecture
```
Client / PackageManager (JSON-RPC)
    ↓
DownloadManagerImplementation
    ├→ Priority Queue (mDownloadQueue)
    ├→ downloaderRoutine (background thread)
    │   └→ DownloadManagerHttpClient (libcurl)
    │       ├→ curl_easy_perform()
    │       ├→ progressCb() → OnProgressUpdate notifications
    │       └→ Save to downloadPath
    └→ DownloadManagerTelemetryReporting  →  (optional) metrics
```

### Key Components
- **DownloadManagerImplementation**: Core logic — queue management, control operations, notification dispatch
- **DownloadManagerHttpClient**: libcurl wrapper providing `downloadFile`, `progressCb`, pause/resume, and rate-limit support
- **downloaderRoutine**: Background thread that dequeues and executes download jobs
- **DownloadManagerTelemetryReporting**: Optional telemetry instrumentation

## External Interfaces

### Public APIs (JSON-RPC)
| Method | Description |
|--------|-------------|
| `Download(url, headers, downloadId, downloadPath, timeoutMs)` | Start HTTP download |
| `Pause(downloadId)` | Pause active download |
| `Resume(downloadId)` | Resume paused download |
| `Cancel(downloadId)` | Cancel and discard download |
| `RateLimit(downloadId, bytesPerSecond)` | Throttle download speed |
| `Progress(downloadId, percent)` | Query download progress percentage |
| `Delete(fileLocator)` | Delete a downloaded file |

### Notifications
| Event | Description |
|-------|-------------|
| `OnProgressUpdate` | Download progress (size, percentage, speed) |
| `OnComplete` | Download completed successfully |
| `OnError` | Download failed with error code |
| `OnPaused` | Download paused |

### Configuration
- **Config File**: `DownloadManager.conf.in` / `DownloadManager.config`
- **Download Directory**: Configurable destination path
- **Thunder Service**: Registered as a WPEFramework/Thunder plugin

## Data Flow
```
Client::Download(url, downloadId, downloadPath)
    ↓
DownloadManagerImplementation::Download()
    ├→ Validate: network available, URL non-empty
    ├→ Add DownloadInfo to priority queue
    └→ Signal downloaderRoutine thread

downloaderRoutine (background thread):
    ├→ pickDownloadJob() — dequeue next job
    ├→ DownloadManagerHttpClient::downloadFile(url, fileName, rateLimit)
    │   ├→ curl_easy_perform()
    │   ├→ progressCb() → notify OnProgressUpdate
    │   └→ On complete: notify OnComplete
    └→ On failure: notify OnError (with DownloadReason::DOWNLOAD_FAILURE)

Pause(downloadId):
    DownloadManagerImplementation::Pause()
    ├→ curl_easy_pause(CURLPAUSE_RECV)
    └→ Notify: OnPaused

Resume(downloadId):
    DownloadManagerImplementation::Resume()
    ├→ curl_easy_pause(CURLPAUSE_RECV_CONT)
    └→ Notify: OnProgressUpdate (resumed)

RateLimit(downloadId, bytesPerSec):
    DownloadManagerImplementation::RateLimit()
    └→ DownloadManagerHttpClient: set CURLOPT_MAX_RECV_SPEED_LARGE
```

## Integration Points
- **Consumes from**:
  - HTTP/HTTPS network (downloads via libcurl)
  - Filesystem (save downloaded files to `downloadPath`)
- **Provides to**:
  - PackageManager (optional delegate for package HTTP downloads)
  - Clients (via JSON-RPC API and event notifications)

## Performance
- Downloads are processed sequentially per the queue (configurable concurrency limit)
- `progressCb` is invoked by libcurl on each data chunk; notification dispatch frequency depends on chunk size
- Rate limiting is applied per-download via `CURLOPT_MAX_RECV_SPEED_LARGE`

## Security
- No global rate limit — per-download only
- Network availability is checked before queuing to fail-fast on offline scenarios
- Download file path is caller-controlled; callers should validate destination paths

## Versioning & Compatibility
- **API Version**: 1.0.0
- **Plugin Execution Mode**: Configurable via `PLUGIN_DOWNLOADMANAGER_MODE`
- Pause/resume requires HTTP range request support on the server (`Accept-Ranges: bytes`)
- Paused downloads are lost on daemon restart (no persistent queue state)

## Conformance Testing & Validation
- **Unit Tests**: `Tests/L1Tests`
- **Integration Tests**: `Tests/L2Tests` (with mock HTTP server)
- **Test Dependencies**: Mock HTTP server, network simulation

## Dependencies

### External Repositories
| Repository | Purpose | Optional |
|------------|---------|----------|
| entservices-apis | API interface definitions (IDownloadManager) | No |

### System Libraries
| Library | Version | Purpose | Optional |
|---------|---------|---------|----------|
| WPEFramework | Latest | Plugin framework | No |
| curl | Latest | HTTP client library | No |
| jsoncpp | - | JSON serialization | No |

### Internal Dependencies
| Module | Type | Purpose |
|--------|------|---------|
| AppManagersHelpers | Library | Shared utilities, logging |

### CMake Configuration
| Option | Type | Default | Purpose |
|--------|------|---------|---------|
| PLUGIN_DOWNLOADMANAGER_MODE | String | Off | Plugin execution mode |
| PLUGIN_DOWNLOADMANAGER_AUTOSTART | Boolean | false | Auto-start on boot |
| AIMANAGERS_TELEMETRY_METRICS_SUPPORT | ON/OFF | OFF | Telemetry collection |

## Build & Installation
- **Compiled Artifact**: `RdkCppPlugin_DownloadManager.so`
- **Install Path**: `${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins`
- **Required Build Flags**: C++11, CURL support
- **API Version**: 1.0.0

## Constraints & Limitations
- Downloads are processed sequentially (configurable concurrency limit)
- Pause/resume uses HTTP range requests (server must support `Accept-Ranges`)
- Rate limiting applies per-download (no global rate limit)
- Network timeouts are configurable but may vary by system
- Paused downloads are lost if daemon restarts (no persistent queue)

## Covered Code
- DownloadManager/DownloadManagerImplementation.cpp:
    - DownloadManagerImplementation::DownloadManagerImplementation
    - DownloadManagerImplementation::Download
    - DownloadManagerImplementation::Pause
    - DownloadManagerImplementation::Resume
    - DownloadManagerImplementation::Cancel
    - DownloadManagerImplementation::Delete
    - DownloadManagerImplementation::Progress
    - DownloadManagerImplementation::RateLimit
    - DownloadManagerImplementation::downloaderRoutine
    - DownloadManagerImplementation::pickDownloadJob
- DownloadManager/DownloadManagerImplementation.h:
    - DownloadManagerImplementation
- DownloadManager/DownloadManagerHttpClient.cpp:
    - DownloadManagerHttpClient::DownloadManagerHttpClient
    - DownloadManagerHttpClient::downloadFile
    - DownloadManagerHttpClient::progressCb
- DownloadManager/DownloadManagerHttpClient.h:
    - DownloadManagerHttpClient
- DownloadManager/DownloadManager.cpp:
    - DownloadManager::DownloadManager
- DownloadManager/DownloadManager.h:
    - DownloadManager
- DownloadManager/DownloadManagerTelemetryReporting.cpp:
    - DownloadManagerTelemetryReporting

## Open Queries
- What is the configurable concurrency limit for the download queue — is it a CMake option, a config file setting, or hardcoded?
- Is there a mechanism to persist the download queue across daemon restarts?
- What retry policy is used on transient failures — fixed delay, exponential backoff, or configurable?

## References
- Architecture: [DownloadManager.md](../../DownloadManager/DownloadManager.md)
- libcurl: [Documentation](https://curl.se/libcurl/c/)
- Related Spec: [PackageManager.spec.md](./PackageManager.spec.md)

## Change History
- [2026-06-06] - openspec-templater - Restructured to match spec template.
