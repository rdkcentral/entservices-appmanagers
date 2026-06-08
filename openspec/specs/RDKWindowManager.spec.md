# RDKWindowManager Specification

## Overview
**Module Name**: RDKWindowManager
**Type**: WPEFramework Plugin
**Purpose**: Display and window management for RDK platform, providing Wayland display creation, client management, focus control, and key interception

## Description
RDKWindowManager is the display and window management layer for RDK-based devices. It wraps the `rdk-window-manager` library (Essos/Westeros compositor) and exposes a JSON-RPC API for creating and destroying Wayland displays, managing window focus and visibility, configuring key interception, tracking connected Wayland clients, and reporting user inactivity. It is consumed primarily by RuntimeManager (for display allocation during container launch) and LifecycleManager (for focus and visibility management during state transitions).

## Requirements
- Provide JSON-RPC API for `CreateDisplay`, `DestroyDisplay`, `SetFocus`, `GetFocus`, `SetVisibility`, `GetClients`, `SetKeyInterceptor`, and `ReportInactivity`
- Allocate a Wayland display socket for each application via the underlying compositor (Essos/Westeros)
- Support virtual displays with configurable width and height
- Manage window focus and stacking order across multiple applications
- Configure per-app key interception (prevent app from receiving intercepted keys)
- Track connected Wayland clients via `CompositorController::getClients`
- Show/hide individual application windows via `SetVisibility`
- Report user inactivity periods to the system
- Coordinate display lifecycle (create on app launch, destroy on app termination)
- Optionally emit telemetry metrics when `AIMANAGERS_TELEMETRY_METRICS_SUPPORT` is enabled

## Architecture / Design

### High-Level Architecture
```
RuntimeManager / LifecycleManager (JSON-RPC)
    ↓
RDKWindowManager (plugin interface)
    ↓
RDKWindowManagerImplementation
    ├→ CreateDisplayRequest (async dispatch)
    ├→ CompositorController (rdk-window-manager)
    │   ├→ Essos/Westeros compositor
    │   └→ Wayland protocol
    └→ RDKWindowManagerTelemetryReporting  →  (optional) metrics
```

### Key Components
- **RDKWindowManagerImplementation**: Core plugin logic — all display and window operations
- **CreateDisplayRequest**: Encapsulates an async display creation request
- **CompositorController**: Interface to the underlying `rdk-window-manager` compositor
- **RDKWindowManagerTelemetryReporting**: Optional telemetry instrumentation

## External Interfaces

### Public APIs (JSON-RPC)
| Method | Description |
|--------|-------------|
| `CreateDisplay(clientId, displayName, displayWidth, displayHeight, virtualDisplay, virtualWidth, virtualHeight, ownerId, groupId, topmost, focus)` | Create Wayland display |
| `DestroyDisplay(displayName)` | Destroy display and release socket |
| `SetFocus(client)` | Set window focus |
| `GetFocus()` | Query currently focused window |
| `SetVisibility(client, visible)` | Show or hide a window |
| `GetClients(displayName)` | List connected Wayland clients |
| `SetKeyInterceptor(displayName, keys)` | Configure key interception |
| `ReportInactivity(inactiveSeconds)` | Notify system of user inactivity |

### Configuration
- **Config File**: `RDKWindowManager.conf.in` / `RDKWindowManager.config`
- **Display Settings**: Wayland socket paths, compositor connection parameters
- **DBus Service**: Registered as WPEFramework plugin

## Data Flow
```
RuntimeManager::Run()
    ↓
RDKWindowManagerImplementation::CreateDisplay(clientId, displayName, ...)
    ├→ CreateDisplayRequest created and enqueued
    ├→ CompositorController::createDisplay()
    │   ├→ Allocate Wayland socket via Essos
    │   └→ Register with compositor
    └→ Return waylandDisplay socket path to RuntimeManager

RuntimeManager mounts display:
    ├→ Bind-mount Wayland socket into OCI container
    ├→ App connects to Wayland socket
    └→ RDKWindowManagerImplementation tracks client via CompositorController::getClients()

LifecycleManager::SetTargetAppState(ACTIVE)
    ↓
RDKWindowManagerImplementation::SetFocus(client)
    ├→ CompositorController::setFocus(client)
    ├→ Update stacking order
    └→ Notify connected clients

LifecycleManager::UnloadApp()
    ↓
RDKWindowManagerImplementation::DestroyDisplay(displayName)
    ├→ CompositorController: release display
    └→ Free Wayland socket

Key Intercept:
    RDKWindowManagerImplementation::SetKeyInterceptor(displayName, keys)
    ├→ Configure key codes to intercept at compositor level
    └→ Intercepted keys routed to system handler, not app

Inactivity:
    User inactivity timer fires
        ↓
    RDKWindowManagerImplementation::ReportInactivity(seconds)
        └→ Notify registered system listeners
```

## Integration Points
- **Consumes from**:
  - Wayland compositor (Essos/Westeros) via `rdk-window-manager` / `CompositorController`
  - System event handlers (key, touch, inactivity events)
- **Provides to**:
  - RuntimeManager (Wayland display socket allocation)
  - LifecycleManager (focus and visibility management)

## Performance
- Display creation is dispatched via `CreateDisplayRequest` for async processing to avoid blocking the JSON-RPC thread
- Focus and visibility operations are synchronous via `CompositorController`

## Security
- Key interception is applied at the compositor level (system-wide policy); intercepted keys are never delivered to the app
- Display socket permissions should restrict access to the owning container (`ownerId`/`groupId` parameters)
- Focus is exclusive; only one application holds focus at a time

## Versioning & Compatibility
- **API Version**: 1.0.0
- **Plugin Execution Mode**: Configurable via `PLUGIN_RDKWINDOWMANAGER_MODE`
- One Wayland display per application (Wayland protocol limitation)
- Requires a functioning Essos/Westeros compositor; absent compositor, all operations fail

## Conformance Testing & Validation
- **Unit Tests**: `Tests/L1Tests`
- **Integration Tests**: `Tests/L2Tests` (with mock compositor)
- **Test Dependencies**: Mock Wayland compositor, event injection framework

## Dependencies

### External Repositories
| Repository | Purpose | Optional |
|------------|---------|----------|
| rdk-window-manager | RDK window management reference implementation | No |

### System Libraries
| Library | Version | Purpose | Optional |
|---------|---------|---------|----------|
| WPEFramework | Latest | Plugin framework | No |
| Wayland | Latest | Wayland protocol client/server | No |
| Essos/Westeros | - | Wayland compositor | No |

### Internal Dependencies
| Module | Type | Purpose |
|--------|------|---------|
| AppManagersHelpers | Library | Shared utilities, logging |

### CMake Configuration
| Option | Type | Default | Purpose |
|--------|------|---------|---------|
| PLUGIN_RDKWINDOWMANAGER_MODE | String | Off | Plugin execution mode |
| PLUGIN_RDKWINDOWMANAGER_AUTOSTART | Boolean | false | Auto-start on boot |

## Build & Installation
- **Compiled Artifact**: `RdkCppPlugin_RDKWindowManager.so`
- **Install Path**: `${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins`
- **Required Build Flags**: C++11
- **API Version**: 1.0.0

## Constraints & Limitations
- One Wayland display per application
- Requires functioning Wayland compositor (Essos/Westeros)
- Key interception is global (system-wide policy)
- Display socket permissions depend on container capabilities
- Focus is exclusive (only one focused app at a time)

## Covered Code
- RDKWindowManager/RDKWindowManagerImplementation.cpp:
    - RDKWindowManagerImplementation::CreateDisplay
    - RDKWindowManagerImplementation::SetFocus
    - RDKWindowManagerImplementation::createDisplay
- RDKWindowManager/RDKWindowManagerImplementation.h:
    - RDKWindowManagerImplementation
- RDKWindowManager/RDKWindowManager.cpp:
    - RDKWindowManager
- RDKWindowManager/RDKWindowManager.h:
    - RDKWindowManager
- RDKWindowManager/RDKWindowManagerTelemetryReporting.cpp:
    - RDKWindowManagerTelemetryReporting

## Open Queries
- Is `DestroyDisplay` explicitly called by LifecycleManager/RuntimeManager on app termination, or is cleanup automatic when the Wayland client disconnects?
- How is `SetKeyInterceptor` persisted across display destroy/recreate cycles for the same app?
- Is there a maximum number of concurrent displays supported by the compositor?

## References
- Architecture: [RDKWindowManager.md](../../RDKWindowManager/RDKWindowManager.md)
- rdk-window-manager: [Repository](https://github.com/rdkcentral/rdk-window-manager)
- Related Spec: [RuntimeManager.spec.md](./RuntimeManager.spec.md)
- Related Spec: [LifecycleManager.spec.md](./LifecycleManager.spec.md)

## Change History
- [2026-06-06] - openspec-templater - Restructured to match spec template.
