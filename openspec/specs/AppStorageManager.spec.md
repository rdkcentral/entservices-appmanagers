# AppStorageManager Specification

## Overview
**Module Name**: AppStorageManager
**Type**: WPEFramework Plugin
**Purpose**: Application-specific persistent storage allocation, management, and cleanup on the filesystem

## Description
AppStorageManager provides a dedicated per-application persistent storage layer on the local filesystem. It is responsible for creating, querying, clearing, and deleting storage directories for applications. It is called by PackageManager during installation to allocate storage, by RuntimeManager to resolve storage paths for container mounts, and by AppManager during app close to clear app data. Storage operations are synchronous and operate directly on the local filesystem.

## Requirements
- Provide JSON-RPC API for `CreateStorage`, `GetStorage`, `DeleteStorage`, `Clear`, and `ClearAll`
- Allocate a dedicated storage directory for each application with a configurable root path
- Return the allocated storage path, size, and usage via `GetStorage`
- Clear app data files while preserving the directory structure via `Clear`
- Support bulk clearing of all app data with an optional exemption list via `ClearAll`
- Delete the storage directory completely via `DeleteStorage`
- Enforce optional storage quotas if configured
- Expose a `RequestHandler` layer for processing storage requests asynchronously

## Architecture / Design

### High-Level Architecture
```
PackageManager / RuntimeManager / AppManager (JSON-RPC)
    ↓
AppStorageManager (plugin interface)
    ↓
AppStorageManagerImplementation
    ├→ RequestHandler       →  Async request dispatch
    ├→ Filesystem ops       →  mkdir, rmdir, stat, unlink
    └→ AppStorageManagerTelemetryReporting  →  (optional) metrics
```

### Key Components
- **AppStorageManagerImplementation**: Core logic — creates, queries, clears, and deletes storage
- **RequestHandler**: Processes storage requests, potentially async, with locking
- **AppStorageManagerTelemetryReporting**: Optional telemetry instrumentation

## External Interfaces

### Public APIs (JSON-RPC)
| Method | Description |
|--------|-------------|
| `CreateStorage(appId, size, path, errorReason)` | Allocate app storage directory |
| `GetStorage(appId, userId, groupId, path, size, used)` | Retrieve storage path and usage |
| `DeleteStorage(appId, errorReason)` | Remove storage directory completely |
| `Clear(appId, errorReason)` | Clear app data files, preserve directory structure |
| `ClearAll(exemptionAppIds, errorReason)` | Clear all app data with optional exemptions |

### Configuration
- **Config File**: `AppStorageManager.conf.in` / `AppStorageManager.config`
- **Storage Root**: Configurable via system settings
- **Thunder Service**: Exposed as a WPEFramework/Thunder plugin

## Data Flow
```
PackageManager::Install()
    ↓
AppStorageManagerImplementation::CreateStorage(appId, size)
    ├→ RequestHandler::CreateStorage()
    ├→ mkdir /storage/<appId>/
    ├→ Create subdirs (data, cache, etc.)
    └→ Return storagePath

RuntimeManager::Run()
    ↓
AppStorageManagerImplementation::GetStorage(appId, userId, groupId)
    ├→ RequestHandler::GetStorage()
    ├→ Retrieve storagePath, size, used
    └→ Return to RuntimeManager for container mount

AppManager::CloseApp()
    ↓
AppStorageManagerImplementation::Clear(appId)
    ├→ RequestHandler::Clear()
    ├→ Remove app data files under /storage/<appId>/
    ├→ Preserve directory structure
    └→ Reset storage usage counter

PackageManager::Uninstall()
    ↓
AppStorageManagerImplementation::DeleteStorage(appId)
    ├→ RequestHandler::DeleteStorage()
    ├→ Recursively remove /storage/<appId>/
    └→ Free filesystem space
```

## Integration Points
- **Consumes from**:
  - Local filesystem (storage operations)
- **Provides to**:
  - PackageManager (storage allocation on install)
  - RuntimeManager (storage paths for container mounts)
  - AppManager (storage cleanup on app close)

## Performance
- Storage operations are synchronous; large `Clear` or `ClearAll` operations may block
- Quota enforcement depends on filesystem support (e.g., `quotactl` or directory-size stat)

## Security
- No encryption support for sensitive app data
- Storage directories are scoped per `appId` to prevent cross-app access
- `userId` and `groupId` are accepted on `GetStorage` to ensure correct ownership for container mounts

## Versioning & Compatibility
- **API Version**: 1.0.0
- **Plugin Execution Mode**: Configurable via `PLUGIN_APP_STORAGE_MANAGER_MODE`
- No cross-device storage sync; storage is local-only

## Conformance Testing & Validation
- **Unit Tests**: `Tests/L1Tests`
- **Integration Tests**: `Tests/L2Tests`
- **Test Dependencies**: Mock filesystem, sandbox directories

## Dependencies

### External Repositories
| Repository | Purpose | Optional |
|------------|---------|----------|
| entservices-apis | API interface definitions (IAppStorageManager) | No |

### System Libraries
| Library | Version | Purpose | Optional |
|---------|---------|---------|----------|
| WPEFramework | Latest | Plugin framework | No |

### Internal Dependencies
| Module | Type | Purpose |
|--------|------|---------|
| AppManagersHelpers | Library | Shared utilities, logging |

### CMake Configuration
| Option | Type | Default | Purpose |
|--------|------|---------|---------|
| PLUGIN_APP_STORAGE_MANAGER_MODE | String | Off | Plugin execution mode |
| PLUGIN_APP_STORAGE_MANAGER_AUTOSTART | Boolean | false | Auto-start on boot |

## Build & Installation
- **Compiled Artifacts**: `lib${NAMESPACE}AppStorageManager.so`, `lib${NAMESPACE}AppStorageManagerImplementation.so`
- **Install Path**: `${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins`
- **Required Build Flags**: C++11
- **API Version**: 1.0.0

## Constraints & Limitations
- Depends on local filesystem (not network storage)
- No encryption support for sensitive data
- Storage operations are synchronous (may block on large clear operations)
- Quota enforcement depends on filesystem support
- No backup/restore or cross-device sync

## Covered Code
- AppStorageManager/AppStorageManagerImplementation.cpp:
    - StorageManagerImplementation::CreateStorage
    - StorageManagerImplementation::GetStorage
    - StorageManagerImplementation::DeleteStorage
    - StorageManagerImplementation::Clear
    - StorageManagerImplementation::ClearAll
- AppStorageManager/AppStorageManagerImplementation.h:
    - StorageManagerImplementation
- AppStorageManager/RequestHandler.cpp:
    - RequestHandler::CreateStorage
    - RequestHandler::GetStorage
    - RequestHandler::DeleteStorage
    - RequestHandler::Clear
    - RequestHandler::ClearAll
- AppStorageManager/RequestHandler.h:
    - RequestHandler
- AppStorageManager/AppStorageManager.cpp:
    - AppStorageManager
- AppStorageManager/AppStorageManager.h:
    - AppStorageManager
- AppStorageManager/AppStorageManagerTelemetryReporting.cpp:
    - AppStorageManagerTelemetryReporting

## Open Queries
- What is the configurable storage root path and how is it set (CMake option, config file, or runtime config)?
- Is quota enforcement implemented — if so, what mechanism is used (filesystem quotas or manual stat tracking)?
- Does `ClearAll` apply to all apps including system apps, or only to user-installed apps?

## References
- Architecture: [AppStorageManager.md](../../AppStorageManager/AppStorageManager.md)
- Related Spec: [PackageManager.spec.md](./PackageManager.spec.md)
- Related Spec: [RuntimeManager.spec.md](./RuntimeManager.spec.md)

## Change History
- [2026-06-06] - openspec-templater - Restructured to match spec template.
