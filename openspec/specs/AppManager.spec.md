# AppManager Specification

## Overview
**Module Name**: AppManager
**Type**: WPEFramework Plugin
**Purpose**: Primary API for application lifecycle management and orchestration on RDK-based devices

## Description
AppManager is the primary entry point for application lifecycle management on RDK-based devices. It acts as the orchestrator that coordinates all other modules — packaging, lifecycle, storage, and display — to provide a unified JSON-RPC API to clients. It tracks loaded applications, propagates state change events, and delegates specialised operations (container runtime, display, package management) to the appropriate sub-plugins.

## Requirements
- Provide JSON-RPC API for launching, closing, terminating, and killing applications
- Maintain an in-memory map of loaded applications and their current lifecycle states
- Lock packages via PackageManager before launching an app and unlock on close/terminate
- Forward launch/close/terminate/kill requests to LifecycleManager
- Propagate state change notifications received from LifecycleManager to JSON-RPC clients
- Support querying of loaded apps and per-app state via `GetLoadedApps` and `GetAppState`
- Support get/set of application-specific properties via PersistentStore
- Clear app-specific storage via AppStorageManager on unload
- Optionally emit telemetry metrics when `AIMANAGERS_TELEMETRY_METRICS_SUPPORT` is enabled

## Architecture / Design

### High-Level Architecture
```
Client (JSON-RPC)
    ↓
AppManager (plugin interface)
    ↓
AppManagerImplementation
    ├→ LifecycleInterfaceConnector  →  LifecycleManager (JSON-RPC)
    ├→ PackageManager (JSON-RPC)     →  Lock / Unlock packages
    ├→ AppStorageManager (JSON-RPC)  →  Clear storage
    ├→ PersistentStore (Framework)   →  Get / Set properties
    └→ AppInfoManager                →  In-memory app state map
```

### Key Components
- **AppManagerImplementation**: Core logic — handles all JSON-RPC method dispatch
- **LifecycleInterfaceConnector**: Thin connector to LifecycleManager over JSON-RPC; also forwards state-change notifications back to AppManagerImplementation
- **AppInfoManager**: In-memory registry of loaded apps and their metadata
- **AppInfo / AppManagerTypes**: Data structures for app metadata and types
- **AppManagerTelemetryReporting**: Optional telemetry instrumentation

## External Interfaces

### Public APIs (JSON-RPC)
| Method | Description |
|--------|-------------|
| `LaunchApp(appId, intent, launchArgs)` | Launch an application |
| `CloseApp(appId)` | Request graceful app closure |
| `TerminateApp(appId)` | Forcefully terminate app |
| `KillApp(appId)` | Kill app without cleanup |
| `GetLoadedApps()` | List all loaded applications |
| `GetAppState(appId)` | Query current app state |

### Configuration
- **Config File**: `AppManager.conf.in` / `AppManager.config`
- **DBus Service**: Registered as WPEFramework plugin

## Data Flow
```
Client (JSON-RPC)
    ↓
AppManager::Notify()
    ↓
AppManagerImplementation::LaunchApp()
    ├→ PackageManager::Lock()         →  Get unpackedPath, config
    ├→ LifecycleInterfaceConnector::Launch()
    │   └→ LifecycleManager::SpawnApp()
    │       └→ RuntimeManager::Run()
    │           └→ Dobby (container started)
    └→ Track app state in AppInfoManager

State Changes:
    LifecycleManager → LifecycleInterfaceConnector (ILifecycleManagerState listener)
    LifecycleInterfaceConnector → AppManagerImplementation
    AppManagerImplementation → Clients (via JSON-RPC notifications)
```

## Integration Points
- **Consumes from**:
  - LifecycleManager (lifecycle state changes)
  - PackageManager (package paths, configs)
  - AppStorageManager (storage cleanup)
  - PersistentStore (app properties)
- **Provides to**:
  - Client applications via JSON-RPC API
  - System telemetry (if `AIMANAGERS_TELEMETRY_METRICS_SUPPORT` enabled)

## Performance
- Single-threaded JSON-RPC message handling; long-running operations are delegated asynchronously to LifecycleManager
- App state is held in-memory (`AppInfoManager`) for O(1) lookups

## Security
- Package must be locked before launch to prevent uninstall-during-execution race conditions
- App properties stored in PersistentStore are scoped per `appId`

## Versioning & Compatibility
- **API Version**: 1.0.0
- **Plugin Execution Mode**: Configurable via `PLUGIN_APP_MANAGER_MODE` (Off / InProcess / Remote)
- App state is in-memory; not persisted across daemon restarts — clients must re-query on reconnect

## Conformance Testing & Validation
- **Unit Tests**: `Tests/L1Tests` — mock LifecycleManager, PackageManager, AppStorageManager
- **Integration Tests**: `Tests/L2Tests` — full multi-module coordination
- **Test Dependencies**: Mock implementations for all external plugins

## Dependencies

### External Repositories
| Repository | Purpose | Optional |
|------------|---------|----------|
| entservices-apis | API interface definitions (IAppManager) | No |
| Dobby | Container runtime platform (indirect via LifecycleManager) | No |

### System Libraries
| Library | Version | Purpose | Optional |
|---------|---------|---------|----------|
| WPEFramework | Latest | Plugin framework | No |
| jsoncpp | - | JSON-RPC serialization | No |

### Internal Dependencies
| Module | Type | Purpose |
|--------|------|---------|
| LifecycleManager | Plugin | Forward app lifecycle requests; receive state notifications |
| PackageManager | Plugin | Lock/unlock packages during app lifecycle |
| AppStorageManager | Plugin | Clear app-specific storage on unload |
| PersistentStore | Framework | Store/retrieve app properties |
| AppManagersHelpers | Library | Shared utilities and logging |

### CMake Configuration
| Option | Type | Default | Purpose |
|--------|------|---------|---------|
| PLUGIN_APP_MANAGER_MODE | String | Off | Plugin execution mode (Off/InProcess/Remote) |
| PLUGIN_APP_MANAGER_AUTOSTART | Boolean | false | Auto-start on system boot |
| AIMANAGERS_TELEMETRY_METRICS_SUPPORT | ON/OFF | OFF | Enable telemetry collection |

## Build & Installation
- **Compiled Artifact**: `RdkCppPlugin_AppManager.so` / `RdkCppPlugin_AppManagerImplementation.so`
- **Install Path**: `${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins`
- **Required Build Flags**: C++11, exceptions enabled

## Constraints & Limitations
- Single-threaded JSON-RPC message handling
- Requires LifecycleManager to be available for app launch operations
- Package must be locked before app can be launched
- App state is in-memory; not persisted across daemon restarts

## Covered Code
- AppManager/AppManagerImplementation.cpp:
    - AppManagerImplementation::LaunchApp
    - AppManagerImplementation::CloseApp
    - AppManagerImplementation::TerminateApp
    - AppManagerImplementation::KillApp
    - AppManagerImplementation::GetLoadedApps
- AppManager/AppManagerImplementation.h:
    - AppManagerImplementation
- AppManager/LifecycleInterfaceConnector.cpp:
    - LifecycleInterfaceConnector::closeApp
    - LifecycleInterfaceConnector::terminateApp
    - LifecycleInterfaceConnector::killApp
    - LifecycleInterfaceConnector::getLoadedApps
- AppManager/LifecycleInterfaceConnector.h:
    - LifecycleInterfaceConnector
- AppManager/AppInfoManager.cpp:
    - AppInfoManager
- AppManager/AppInfoManager.h:
    - AppInfoManager
- AppManager/AppInfo.cpp:
    - AppInfo
- AppManager/AppInfo.h:
    - AppInfo
- AppManager/AppManagerTypes.h:
    - AppManagerTypes
- AppManager/AppManagerTelemetryReporting.cpp:
    - AppManagerTelemetryReporting
- AppManager/AppManagerTelemetryReporting.h:
    - AppManagerTelemetryReporting
- AppManager/AppManager.cpp:
    - AppManager
- AppManager/AppManager.h:
    - AppManager

## Open Queries
- What is the exact retry/fallback behaviour when LifecycleManager is unavailable at launch time?
- Is `GetAppState` exposed as a direct JSON-RPC method on AppManager, or only via LifecycleManager?
- Are app properties in PersistentStore namespaced to prevent cross-app access?

## References
- Architecture: [README.md](../../README.md#system-architecture)
- Full Module Doc: [AppManager.md](../../AppManager/AppManager.md)
- Related Spec: [LifecycleManager.spec.md](./LifecycleManager.spec.md)
- Related Spec: [PackageManager.spec.md](./PackageManager.spec.md)

## Change History
- [2026-06-06] - openspec-templater - Restructured to match spec template.
