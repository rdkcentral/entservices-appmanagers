# PackageManager Specification

## Overview
**Module Name**: PackageManager
**Type**: WPEFramework Plugin
**Purpose**: Application package lifecycle management including download, installation, locking, and uninstallation

## Description
PackageManager handles the full lifecycle of application packages on RDK-based devices. It provides HTTP download capabilities (with priority queuing, pause/resume, and rate limiting), package installation via `libpackage-sky`, lock/unlock semantics to prevent uninstall-during-execution races, metadata querying, and package removal. It integrates with AppStorageManager for storage allocation during installation and exposes a JSON-RPC API to AppManager, PreinstallManager, and direct clients.

## Requirements
- Provide JSON-RPC API for download, pause, resume, cancel, install, uninstall, lock, unlock, list, and rate-limit operations
- Download packages via HTTP using libcurl with configurable destination directory
- Support priority-based download queuing with configurable concurrency
- Support pause/resume using HTTP range requests (server must support)
- Support per-download bandwidth rate limiting
- Install packages by parsing manifests via `libpackage-sky`, validating signatures, extracting contents, and calling AppStorageManager for storage
- Maintain lock reference counts per package; prevent uninstall while locked
- Return `unpackedPath` and config metadata on `GetLockedInfo` for container launch
- Emit progress, completion, and error notifications to registered listeners
- Optionally emit telemetry metrics when `AIMANAGERS_TELEMETRY_METRICS_SUPPORT` is enabled
- Support RALF package format when `USE_LIBPACKAGE_RALF` is enabled

## Architecture / Design

### High-Level Architecture
```
Client / AppManager / PreinstallManager (JSON-RPC)
    ↓
PackageManagerImplementation
    ├→ HttpClient (libcurl)          →  HTTP download
    ├→ libpackage-sky                →  Package manifest parsing, extraction
    ├→ AppStorageManager (JSON-RPC)  →  Storage allocation on install
    └→ PackageManagerTelemetryReporting  →  (optional) metrics
```

### Key Components
- **PackageManagerImplementation**: Core logic — download queue, install, lock/unlock state machine
- **HttpClient**: libcurl-based HTTP download client with pause/resume and rate-limit support
- **PackageManagerTelemetryReporting**: Optional telemetry instrumentation
- **Lock State Machine**: Reference-counted lock per `(packageId, version)` pair

## External Interfaces

### Public APIs (JSON-RPC)
| Method | Description |
|--------|-------------|
| `Download(url, options, downloadId)` | Initiate package download |
| `Pause(downloadId)` | Pause active download |
| `Resume(downloadId)` | Resume paused download |
| `Cancel(downloadId)` | Cancel and discard download |
| `Progress(downloadId, percent)` | Query current download progress |
| `RateLimit(downloadId, limit)` | Throttle download bandwidth |
| `Install(packageId, version, additionalMetadata, fileLocator, failReason)` | Install downloaded package |
| `Uninstall(packageId, errorReason)` | Remove installed package |
| `Lock(packageId, version, lockReason, ...)` | Lock package for app execution |
| `Unlock(packageId, version)` | Unlock package |
| `GetLockedInfo(packageId, version, unpackedPath, configMetadata, ...)` | Query lock state and config |
| `ListPackages(packages)` | List installed packages |
| `PackageState(packageId, version, state)` | Query package install state |
| `Register / Unregister (INotification)` | Subscribe/unsubscribe download or install notifications |

### Configuration
- **Config File**: `AppPackageManager.conf.in` / `AppPackageManager.config`
- **Download Dir**: Configurable via `PLUGIN_PACKAGEMANAGER_DOWNLOAD_DIR`
- **DBus Service**: Registered as WPEFramework plugin

## Data Flow
```
Client::Download(url)
    ↓
PackageManagerImplementation::Download()
    ├→ Add to priority queue
    └→ HttpClient::downloadFile() via curl
        ├→ Report progress (OnProgressUpdate)
        └→ Save to PLUGIN_PACKAGEMANAGER_DOWNLOAD_DIR

Client::Install(packageId, version, fileLocator)
    ↓
PackageManagerImplementation::Install()
    ├→ Parse package manifest via libpackage-sky
    ├→ Validate signatures
    ├→ AppStorageManager::CreateStorage()
    ├→ Extract app files to storage path
    └→ Update package state → INSTALLED

Lock(packageId, version):
    ├→ Verify package is INSTALLED
    ├→ Increment lock reference count
    ├→ Return unpackedPath and config via GetLockedInfo
    └→ Block uninstall while locked

Unlock(packageId, version):
    ├→ Decrement lock reference count
    └→ If count == 0, allow uninstall / trigger pending operations
```

## Integration Points
- **Consumes from**:
  - AppStorageManager (storage allocation on install)
  - HTTP transport via libcurl (downloads)
- **Provides to**:
  - AppManager (package lock/unlock, metadata)
  - PreinstallManager (install requests)
  - Clients (via JSON-RPC API)

## Performance
- Downloads are queued (sequential by default; configurable concurrency limit)
- Installation is synchronous and may be slow for large packages
- Lock/unlock operations are O(1) reference count updates

## Security
- Package removal requires all lock references to be released
- Signature validation is performed during install (mechanism depends on libpackage-sky configuration)
- Download directory permissions should restrict access to the PackageManager process

## Versioning & Compatibility
- **API Version**: 1.0.0
- **Plugin Execution Mode**: Configurable via `PLUGIN_PACKAGEMANAGER_MODE`
- Multiple package versions are allowed simultaneously (no version conflict enforcement)
- `USE_LIBPACKAGE_RALF`: optional flag for RALF package format support

## Conformance Testing & Validation
- **Unit Tests**: `Tests/L1Tests`
- **Integration Tests**: `Tests/L2Tests`
- **Test Dependencies**: Mock AppStorageManager, Mock HTTP server

## Dependencies

### External Repositories
| Repository | Purpose | Optional |
|------------|---------|----------|
| libpackage-sky | Package installation and metadata handling | No |

### System Libraries
| Library | Version | Purpose | Optional |
|---------|---------|---------|----------|
| WPEFramework | Latest | Plugin framework | No |
| curl | Latest | HTTP client for downloads | No |
| jsoncpp | - | JSON serialization | No |

### Internal Dependencies
| Module | Type | Purpose |
|--------|------|---------|
| AppStorageManager | Plugin | Create/manage app storage directories |
| AppManagersHelpers | Library | Shared utilities, logging |
| DownloadManager | Plugin | (Optional) delegate HTTP downloads | Yes |

### CMake Configuration
| Option | Type | Default | Purpose |
|--------|------|---------|---------|
| PLUGIN_PACKAGEMANAGER_MODE | String | Off | Plugin execution mode |
| PLUGIN_PACKAGEMANAGER_AUTOSTART | Boolean | false | Auto-start on boot |
| PLUGIN_PACKAGEMANAGER_DOWNLOAD_DIR | String | "" | Download directory path |
| USE_LIBPACKAGE_RALF | ON/OFF | OFF | Use RALF package format |
| AIMANAGERS_TELEMETRY_METRICS_SUPPORT | ON/OFF | OFF | Telemetry collection |

## Build & Installation
- **Compiled Artifact**: `lib${NAMESPACE}AppPackageManager.so`
- **Install Path**: `${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins`
- **Required Build Flags**: C++11
- **API Version**: 1.0.0
- **Build Dependencies**: `libPackage.so` (from libpackage-sky)

## Constraints & Limitations
- Downloads are sequential (queued, not parallel by default)
- Package removal requires all locks to be released
- No package versioning conflict enforcement (multiple versions allowed)
- Download interruption requires manual retry
- Metadata parsing depends on libpackage-sky format
- Paused downloads may be lost if daemon restarts

## Covered Code
- PackageManager/PackageManagerImplementation.cpp:
    - PackageManagerImplementation::Download
    - PackageManagerImplementation::Pause
    - PackageManagerImplementation::Resume
    - PackageManagerImplementation::Cancel
    - PackageManagerImplementation::Progress
    - PackageManagerImplementation::RateLimit
    - PackageManagerImplementation::Install
    - PackageManagerImplementation::Uninstall
    - PackageManagerImplementation::ListPackages
    - PackageManagerImplementation::PackageState
    - PackageManagerImplementation::Lock
    - PackageManagerImplementation::Unlock
    - PackageManagerImplementation::GetLockedInfo
    - PackageManagerImplementation::LockRuntime
    - PackageManagerImplementation::LockPackage
    - PackageManagerImplementation::UnlockPackage
    - PackageManagerImplementation::processBlockedPackage
- PackageManager/PackageManagerImplementation.h:
    - PackageManagerImplementation
- PackageManager/HttpClient.cpp:
    - HttpClient
- PackageManager/HttpClient.h:
    - HttpClient
- PackageManager/PackageManager.cpp:
    - PackageManager
- PackageManager/PackageManager.h:
    - PackageManager
- PackageManager/PackageManagerTelemetryReporting.cpp:
    - PackageManagerTelemetryReporting

## Open Queries
- When `USE_LIBPACKAGE_RALF` is enabled, does the Install API signature change, or is it transparent to callers?
- Is the DownloadManager plugin integration a strict delegation or an optional fallback when PackageManager's internal HttpClient is insufficient?
- What happens to in-progress downloads if the daemon crashes — is there a recovery mechanism?

## References
- Architecture: [PackageManager.md](./PackageManager.md)
- libpackage-sky: [Repository](https://github.com/rdk-e/libpackage-sky)
- Related Spec: [AppStorageManager.spec.md](./AppStorageManager.spec.md)
- Related Spec: [DownloadManager.spec.md](./DownloadManager.spec.md)

## Change History
- [2026-06-06] - openspec-templater - Restructured to match spec template.
