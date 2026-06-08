# RuntimeManager Specification

## Overview
**Module Name**: RuntimeManager
**Type**: WPEFramework Plugin
**Purpose**: OCI container runtime management using Dobby for application execution, resource isolation, hibernation, and lifecycle control

## Description
RuntimeManager is responsible for all container-level operations on RDK devices. It uses Dobby (with crun as the OCI runtime) to launch, suspend, resume, hibernate, wake, terminate, and kill application containers. It generates per-application OCI bundle specifications via `DobbySpecGenerator`, allocates Wayland displays for GUI applications through RDKWindowManager, retrieves storage paths from AppStorageManager, and monitors container state changes via `DobbyEventListener`. Hibernation (checkpoint/restore) is an optional feature gated on CRIU availability.

## Requirements
- Launch application containers via Dobby using dynamically generated OCI specs
- Support suspend (SIGSTOP) and resume (SIGCONT) for pausing applications
- Support checkpoint-based hibernation via CRIU and subsequent wake (restore)
- Support graceful termination and force-kill of containers
- Allocate and bind-mount a Wayland display socket for GUI apps
- Retrieve app storage paths from AppStorageManager and mount them in the container
- Monitor container lifecycle events from Dobby and notify LifecycleManager
- Support optional RALF package format (`RALF_PACKAGE_SUPPORT`)
- Optionally emit telemetry metrics when `AIMANAGERS_TELEMETRY_METRICS_SUPPORT` is enabled

## Architecture / Design

### High-Level Architecture
```
LifecycleManager
    ↓
RuntimeManagerImplementation
    ├→ DobbySpecGenerator       →  Generate OCI JSON spec
    ├→ WindowManagerConnector   →  RDKWindowManager (Wayland display)
    ├→ AppStorageManager        →  Storage paths
    ├→ Dobby (IPC)              →  StartContainer / StopContainer / ...
    ├→ DobbyEventListener       →  Monitor container state events
    ├→ RialtoConnector          →  (optional) Rialto media session
    ├→ UserIdManager            →  Resolve user/group IDs
    └→ Gateway/ContainerUtils   →  Network filter, web inspector
```

### Key Components
- **RuntimeManagerImplementation**: Core plugin logic — dispatches all container operations
- **DobbySpecGenerator**: Generates OCI bundle JSON from app config and RuntimeConfig
- **DobbyEventListener**: Listens to Dobby container lifecycle events and propagates them
- **WindowManagerConnector**: Thin connector to RDKWindowManager for display allocation
- **RialtoConnector**: Optional connector for Rialto media session integration
- **UserIdManager**: Resolves user/group IDs for container isolation
- **Gateway/ContainerUtils, NetFilter, NetFilterLock**: Network isolation utilities
- **Gateway/WebInspector, Debugger**: Debug/inspection tooling

## External Interfaces

### Public APIs (JSON-RPC)
| Method | Description |
|--------|-------------|
| `Run(appId, appInstanceId, userId, groupId, ports, paths, debugSettings, runtimeConfig)` | Start container |
| `Suspend(appInstanceId)` | Pause container (SIGSTOP) |
| `Resume(appInstanceId)` | Resume container (SIGCONT) |
| `Hibernate(appInstanceId)` | Checkpoint to disk via CRIU |
| `Wake(appInstanceId, runtimeState)` | Restore from CRIU checkpoint |
| `Terminate(appInstanceId)` | Graceful container shutdown |
| `Kill(appInstanceId)` | Force-kill container |
| `Register / Unregister (INotification)` | Subscribe/unsubscribe container event notifications |

### OCI Specification
- Base spec: `resources/oci-base-spec.json` (configurable via `PLUGIN_RUNTIME_MANAGER_BASEOCISPEC`)
- Generated per-app with paths, networking, display socket, resource limits

### Configuration
- **Config File**: Runtime config YAML (if `ENABLE_RDKAPPMANAGERS_RUNTIMECONFIG`)
- **DBus Service**: Registered as WPEFramework plugin

## Data Flow
```
LifecycleManager::SpawnApp()
    ↓
RuntimeManagerImplementation::Run()
    ├→ DobbySpecGenerator::generate()
    │   ├→ Apply base OCI spec
    │   ├→ createMounts() — app paths and storage
    │   ├→ Add networking config
    │   └→ Output JSON spec string
    ├→ WindowManagerConnector → RDKWindowManager::CreateDisplay()
    │   └→ Allocate Wayland socket
    ├→ Dobby::StartContainer(ociSpec)
    │   └→ crun executes OCI spec
    └→ DobbyEventListener monitors container events → notify LifecycleManager

Suspend:
    RuntimeManagerImplementation::Suspend()
        └→ Dobby: send SIGSTOP → notify LifecycleManager

Hibernate:
    RuntimeManagerImplementation::Hibernate()
        ├→ CRIU checkpoint via Dobby
        └→ Container stopped; checkpoint files saved

Wake:
    RuntimeManagerImplementation::Wake()
        ├→ WindowManagerConnector → restore display
        ├→ CRIU restore via Dobby
        └→ Container resumes; notify LifecycleManager
```

## Integration Points
- **Consumes from**:
  - RDKWindowManager (Wayland display allocation)
  - AppStorageManager (storage paths)
  - Dobby (container lifecycle IPC)
- **Provides to**:
  - LifecycleManager (container state events)
  - Monitoring systems (container metrics via telemetry)

## Performance
- Container operations are asynchronous; Dobby events drive state change callbacks
- OCI spec generation is synchronous and performed inline during `Run()`
- Resource limits (CPU, memory) are enforced at container creation and cannot be changed post-launch

## Security
- Each container runs with a resolved user/group ID via UserIdManager
- Network isolation enforced via Gateway/NetFilter and NetFilterLock
- Container capabilities and mounts are defined in the OCI spec and cannot be changed at runtime

## Versioning & Compatibility
- **API Version**: 1.0.0
- **Plugin Execution Mode**: Configurable via `PLUGIN_RUNTIME_MANAGER_MODE`
- `RALF_PACKAGE_SUPPORT`: optional build flag for RALF package format
- Hibernation is optional and requires CRIU; absent CRIU, `Hibernate` calls return an error

## Conformance Testing & Validation
- **Unit Tests**: `Tests/L1Tests`
- **Integration Tests**: `Tests/L2Tests` (with Dobby mock)
- **Test Dependencies**: Mock Dobby, Mock RDKWindowManager, Mock AppStorageManager

## Dependencies

### External Repositories
| Repository | Purpose | Optional |
|------------|---------|----------|
| Dobby | OCI container runtime with CRIU support | No |
| rdk-window-manager | Display and Wayland management | No |
| libpackage-sky | Package metadata handling | No |

### System Libraries
| Library | Version | Purpose | Optional |
|---------|---------|---------|----------|
| WPEFramework | Latest | Plugin framework | No |
| crun | >=0.13 | OCI runtime | No |
| criu | Latest | Checkpoint/restore (hibernation) | Yes |
| jsoncpp | - | OCI spec JSON generation | No |

### Internal Dependencies
| Module | Type | Purpose |
|--------|------|---------|
| RDKWindowManager | Plugin | Create display, allocate Wayland socket |
| AppStorageManager | Plugin | Get app storage paths |
| AppManagersHelpers | Library | Shared utilities, logging |

### CMake Configuration
| Option | Type | Default | Purpose |
|--------|------|---------|---------|
| PLUGIN_RUNTIME_MANAGER_MODE | String | Off | Plugin execution mode |
| PLUGIN_RUNTIME_MANAGER_AUTOSTART | Boolean | false | Auto-start on boot |
| PLUGIN_RUNTIME_APP_PORTAL | String | "" | App portal identifier |
| PLUGIN_RUNTIME_CONFIG_FILE | String | "" | Runtime config YAML path |
| RALF_PACKAGE_SUPPORT | ON/OFF | OFF | Enable RALF package support |
| AIMANAGERS_TELEMETRY_METRICS_SUPPORT | ON/OFF | OFF | Telemetry collection |
| ENABLE_RDKAPPMANAGERS_RUNTIMECONFIG | ON/OFF | OFF | Enable YAML runtime config parsing |

## Build & Installation
- **Compiled Artifact**: `RdkCppPlugin_RuntimeManager.so`
- **Install Path**: `${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins`
- **Required Build Flags**: C++11, exceptions enabled
- **API Version**: 1.0.0

## Constraints & Limitations
- Requires Dobby service to be running
- Hibernation requires CRIU support (optional feature)
- Container operations are asynchronous; events drive state changes
- One Wayland display per app (Wayland limitation)
- Resource limits enforced at container creation (cannot change post-launch)

## Covered Code
- RuntimeManager/RuntimeManagerImplementation.cpp:
    - RuntimeManagerImplementation::Register
    - RuntimeManagerImplementation::Unregister
    - RuntimeManagerImplementation::Run
    - RuntimeManagerImplementation::Hibernate
    - RuntimeManagerImplementation::Wake
    - RuntimeManagerImplementation::Suspend
    - RuntimeManagerImplementation::Resume
    - RuntimeManagerImplementation::Terminate
    - RuntimeManagerImplementation::Kill
    - RuntimeManagerImplementation::createOCIContainerPluginObject
    - RuntimeManagerImplementation::createStorageManagerPluginObject
    - RuntimeManagerImplementation::getAppStorageInfo
- RuntimeManager/RuntimeManagerImplementation.h:
    - RuntimeManagerImplementation
- RuntimeManager/DobbySpecGenerator.cpp:
    - DobbySpecGenerator::generate
    - DobbySpecGenerator::getWorkingDir
    - DobbySpecGenerator::createMounts
    - DobbySpecGenerator::getSysMemoryLimit
    - DobbySpecGenerator::getGPUMemoryLimit
    - DobbySpecGenerator::getVpuEnabled
    - DobbySpecGenerator::populateClassicPlugins
    - DobbySpecGenerator::createEthanLogPlugin
    - DobbySpecGenerator::createMulticastSocketPlugin
    - DobbySpecGenerator::createOpenCDMPlugin
    - DobbySpecGenerator::createPrivateDataMount
    - DobbySpecGenerator::createFkpsMounts
- RuntimeManager/DobbyEventListener.cpp:
    - DobbyEventListener
- RuntimeManager/WindowManagerConnector.h:
    - WindowManagerConnector
- RuntimeManager/RialtoConnector.cpp:
    - RialtoConnector
- RuntimeManager/UserIdManager.cpp:
    - UserIdManager
- RuntimeManager/Gateway/ContainerUtils.cpp:
    - ContainerUtils
- RuntimeManager/Gateway/NetFilter.cpp:
    - NetFilter
- RuntimeManager/Gateway/NetFilterLock.cpp:
    - NetFilterLock
- RuntimeManager/Gateway/WebInspector.cpp:
    - WebInspector

## Open Queries
- What is the exact Dobby IPC mechanism used — D-Bus, Unix socket, or shared memory?
- Is `Wake` guaranteed to restore to the exact display state prior to `Hibernate`, or does display re-creation require coordination with LifecycleManager?
- What happens if the CRIU checkpoint directory is corrupted or missing at `Wake` time?

## References
- Architecture: [RuntimeManager.md](../../RuntimeManager/RuntimeManager.md)
- Dobby Integration: [Dobby Documentation](https://github.com/rdkcentral/Dobby)
- Related Spec: [LifecycleManager.spec.md](./LifecycleManager.spec.md)
- Related Spec: [RDKWindowManager.spec.md](./RDKWindowManager.spec.md)

## Change History
- [2026-06-06] - openspec-templater - Restructured to match spec template.
