# LifecycleManager Specification

## Overview
**Module Name**: LifecycleManager
**Type**: WPEFramework Plugin
**Purpose**: Application lifecycle state machine managing transitions between app states (UNLOADED → LOADING → ACTIVE/PAUSED → SUSPENDED/HIBERNATED → TERMINATING)

## Description
LifecycleManager is the central state machine for application lifecycle on RDK-based devices. It receives lifecycle requests from AppManager, validates and drives state transitions, coordinates the RuntimeManager for container operations and RDKWindowManager for display operations, and emits state change notifications to registered listeners. It maintains per-application context objects (`ApplicationContext`) encapsulating the current state and associated resources.

## Requirements
- Maintain a per-app state machine with valid transition enforcement
- Accept spawn, state-change, unload, and kill requests from AppManager
- Forward container operations (run, suspend, resume, hibernate, wake, terminate) to RuntimeManager
- Coordinate display creation and focus management with RDKWindowManager
- Emit state change notifications to all registered `ILifecycleManagerState::INotification` listeners
- Support `GetLoadedApps`, `IsAppLoaded`, `KillApp`, `SendIntentToActiveApp`, `AppReady`, and `StateChangeComplete` operations
- Reject invalid state transitions with a clear error response
- Hibernation support is conditional on CRIU availability in Dobby

## Architecture / Design

### Application Lifecycle States
```
UNLOADED (0)
    ↓ SpawnApp()
LOADING (1)
    ↓ container started
INITIALIZING (2)
    ↓ AppReady()
PAUSED / ACTIVE
    ↕ SetTargetAppState()
SUSPENDED / HIBERNATED
    ↕ SetTargetAppState()
TERMINATING (7)
    ↓ UnloadApp() / KillApp()
UNLOADED (0)
```

### Key Components
- **LifecycleManagerImplementation**: Core logic — state machine, request dispatch
- **ApplicationContext**: Per-app state and resource tracking
- **StateHandler / StateTransitionHandler**: Validates and executes state transitions
- **RequestHandler**: Queues and serialises state transition requests
- **RuntimeManagerHandler**: Connector to RuntimeManager over JSON-RPC
- **WindowManagerHandler**: Connector to RDKWindowManager over JSON-RPC
- **State / StateTransitionRequest**: State definitions and transition request objects
- **LifecycleManagerTelemetryReporting**: Optional telemetry instrumentation

## External Interfaces

### Public APIs (JSON-RPC)
| Method | Description |
|--------|-------------|
| `SpawnApp(appId, intent, targetState, runtimeConfig, launchArgs, ...)` | Initiate app launch |
| `SetTargetAppState(appInstanceId, targetState, intent)` | Request state transition |
| `UnloadApp(appInstanceId, ...)` | Request app termination |
| `KillApp(appInstanceId, ...)` | Force-kill app |
| `GetLoadedApps(verbose)` | List loaded apps with state |
| `IsAppLoaded(appId)` | Query whether an app is loaded |
| `SendIntentToActiveApp(appInstanceId, intent, ...)` | Send intent to active app |
| `AppReady(appId)` | Signal app has initialised |
| `StateChangeComplete(appId, stateChangedId, success)` | Acknowledge state change |
| `Register / Unregister (INotification)` | Subscribe / unsubscribe lifecycle notifications |

### Configuration
- **Config File**: `LifecycleManager.conf.in` / `LifecycleManager.config`
- **DBus Service**: Registered as WPEFramework plugin

## Data Flow
```
AppManager::LaunchApp()
    ↓
LifecycleManager::SpawnApp()
    ├→ Create ApplicationContext
    ├→ Transition: UNLOADED → LOADING
    ├→ RuntimeManagerHandler → RuntimeManager::Run()
    │   └→ Dobby container starts
    ├→ Notify: LOADING → INITIALIZING
    └→ Notify registered listeners of state changes

SetTargetAppState(ACTIVE):
    ├→ Validate current state
    ├→ WindowManagerHandler → RDKWindowManager::CreateDisplay() (if needed)
    ├→ RuntimeManagerHandler → RuntimeManager::Resume() (if suspended)
    └→ Notify: state → ACTIVE

UnloadApp():
    ├→ RuntimeManagerHandler → RuntimeManager::Terminate()
    ├→ WindowManagerHandler → RDKWindowManager::DestroyDisplay()
    ├→ Transition: ... → TERMINATING → UNLOADED
    └→ Notify listeners
```

## Integration Points
- **Consumes from**:
  - RuntimeManager (container state and events)
  - RDKWindowManager (display state and focus)
- **Provides to**:
  - AppManager (state change notifications via `ILifecycleManagerState::INotification`)
  - Clients (via JSON-RPC state queries)

## Performance
- State transitions are sequential per app (no parallel transitions for the same `appInstanceId`)
- RequestHandler queues transitions to serialise concurrent requests

## Security
- State machine enforces strict transition rules; invalid transitions are rejected with an error response
- AppInstanceId is used as the key for all post-spawn operations, preventing cross-app interference

## Versioning & Compatibility
- **API Version**: 1.0.0
- **Plugin Execution Mode**: Configurable via `PLUGIN_LIFECYCLE_MANAGER_MODE`
- Hibernation requires CRIU support in Dobby (optional feature; absent CRIU, hibernate requests are rejected)

## Conformance Testing & Validation
- **Unit Tests**: `Tests/L1Tests` — state machine transition coverage, mock RuntimeManager, mock RDKWindowManager
- **Integration Tests**: `Tests/L2Tests` — multi-module coordination
- **Test Dependencies**: Mock RuntimeManager, Mock RDKWindowManager

## Dependencies

### External Repositories
| Repository | Purpose | Optional |
|------------|---------|----------|
| entservices-apis | API interfaces for ILifecycleManager, ILifecycleManagerState | No |

### System Libraries
| Library | Version | Purpose | Optional |
|---------|---------|---------|----------|
| WPEFramework | Latest | Plugin framework | No |
| jsoncpp | - | JSON serialization | No |

### Internal Dependencies
| Module | Type | Purpose |
|--------|------|---------|
| RuntimeManager | Plugin | Run, suspend, resume, hibernate, wake, terminate container |
| RDKWindowManager | Plugin | Create/destroy display, manage focus |
| AppManagersHelpers | Library | Shared utilities |

### CMake Configuration
| Option | Type | Default | Purpose |
|--------|------|---------|---------|
| PLUGIN_LIFECYCLE_MANAGER_MODE | String | Off | Plugin execution mode |
| PLUGIN_LIFECYCLE_MANAGER_AUTOSTART | Boolean | false | Auto-start on boot |

## Build & Installation
- **Compiled Artifact**: `RdkCppPlugin_LifecycleManager.so`
- **Install Path**: `${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins`
- **Required Build Flags**: C++11
- **API Version**: 1.0.0

## Constraints & Limitations
- State transitions are sequential (no parallel transitions for same app)
- Requires RuntimeManager and RDKWindowManager to be available
- State machine enforces strict transition rules (invalid transitions rejected)
- Hibernation requires CRIU support in Dobby

## Covered Code
- LifecycleManager/LifecycleManagerImplementation.cpp:
    - LifecycleManagerImplementation::Register
    - LifecycleManagerImplementation::Unregister
    - LifecycleManagerImplementation::GetLoadedApps
    - LifecycleManagerImplementation::IsAppLoaded
    - LifecycleManagerImplementation::SpawnApp
    - LifecycleManagerImplementation::SetTargetAppState
    - LifecycleManagerImplementation::UnloadApp
    - LifecycleManagerImplementation::KillApp
    - LifecycleManagerImplementation::SendIntentToActiveApp
    - LifecycleManagerImplementation::AppReady
    - LifecycleManagerImplementation::StateChangeComplete
    - LifecycleManagerImplementation::CloseApp
- LifecycleManager/LifecycleManager.cpp:
    - LifecycleManager
- LifecycleManager/LifecycleManager.h:
    - LifecycleManager
- LifecycleManager/ApplicationContext.cpp:
    - ApplicationContext
- LifecycleManager/ApplicationContext.h:
    - ApplicationContext
- LifecycleManager/StateHandler.h:
    - StateHandler
- LifecycleManager/StateTransitionHandler.cpp:
    - StateTransitionHandler
- LifecycleManager/RequestHandler.cpp:
    - RequestHandler
- LifecycleManager/RuntimeManagerHandler.cpp:
    - RuntimeManagerHandler
- LifecycleManager/RuntimeManagerHandler.h:
    - RuntimeManagerHandler
- LifecycleManager/WindowManagerHandler.cpp:
    - WindowManagerHandler
- LifecycleManager/WindowManagerHandler.h:
    - WindowManagerHandler
- LifecycleManager/State.cpp:
    - State
- LifecycleManager/State.h:
    - State
- LifecycleManager/StateTransitionRequest.h:
    - StateTransitionRequest
- LifecycleManager/LifecycleManagerTelemetryReporting.cpp:
    - LifecycleManagerTelemetryReporting

## Open Queries
- What is the defined behaviour when both RuntimeManager and RDKWindowManager fail during a state transition — is a rollback attempted?
- Is `StateChangeComplete` called by the container/app process directly, or proxied through AppManager?
- Are there timeout mechanisms for transitions that do not complete (e.g., app stuck in LOADING)?

## References
- State Diagram: [LifecycleManager.md](../../LifecycleManager/LifecycleManager.md#state-diagram)
- Architecture: [README.md](../../README.md#application-lifecycle-states)
- Related Spec: [RuntimeManager.spec.md](./RuntimeManager.spec.md)
- Related Spec: [RDKWindowManager.spec.md](./RDKWindowManager.spec.md)

## Change History
- [2026-06-06] - openspec-templater - Restructured to match spec template.
