# RuntimeManager Module

> Container Runtime & Execution Management

[← Back to Main](../README.md) | [← Previous: LifecycleManager](./LifecycleManager.md)

---

## Purpose & Role

The **RuntimeManager** manages OCI container lifecycle using Dobby. It handles application execution, suspension, hibernation (CRIU checkpoint), wake (restore), and termination.

### Core Responsibilities

- **Container Execution:** Start apps in isolated OCI containers
- **Resource Management:** CPU, memory, network namespace isolation
- **Suspend/Resume:** SIGSTOP/SIGCONT for pausing containers
- **Hibernate/Wake:** CRIU checkpoint/restore for memory efficiency
- **Display Integration:** Coordinate with WindowManager for Wayland

### Dependencies

| Module | Purpose |
|--------|---------|
| Dobby (IOCIContainer) | OCI container runtime |
| RDKWindowManager | Wayland display creation |
| StorageManager | App storage paths |

---

## Architecture

```mermaid
graph TB
    subgraph "RuntimeManager Module"
        RTM[RuntimeManager<br/>Plugin]
        RTMI[RuntimeManagerImplementation]
        DSG[DobbySpecGenerator]
        WMC[WindowManagerConnector]
        DEL[DobbyEventListener]
        UID[UserIdManager]
    end

    subgraph "External Services"
        Dobby[Dobby<br/>IOCIContainer]
        WinMgr[RDKWindowManager]
        StorMgr[StorageManager]
    end

    RTM --> RTMI
    RTMI --> DSG
    RTMI --> WMC
    RTMI --> DEL
    RTMI --> UID
    RTMI --> Dobby
    WMC --> WinMgr
    DSG --> StorMgr
```

---

## Class Diagram

```mermaid
classDiagram
    class RuntimeManagerImplementation {
        -map~string,RuntimeAppInfo~ mRuntimeAppInfo
        -IOCIContainer* mOciContainerRemoteObject
        -WindowManagerConnector* mWindowManagerConnector
        -DobbyEventListener* mDobbyEventListener
        +Run(appInstanceId, config) hresult
        +Suspend(appInstanceId) hresult
        +Resume(appInstanceId) hresult
        +Hibernate(appInstanceId) hresult
        +Wake(appInstanceId) hresult
        +Terminate(appInstanceId) hresult
        +Kill(appInstanceId) hresult
    }

    class DobbySpecGenerator {
        +generateSpec(config, appStorage) string
        +getDefaultSpec() string
    }

    class WindowManagerConnector {
        -IRDKWindowManager* mWindowManager
        +createDisplay(displayName) bool
        +destroyDisplay(displayName) void
    }

    class DobbyEventListener {
        -INotification* mCallback
        +onContainerStarted(containerId) void
        +onContainerStopped(containerId) void
    }

    RuntimeManagerImplementation --> DobbySpecGenerator
    RuntimeManagerImplementation --> WindowManagerConnector
    RuntimeManagerImplementation --> DobbyEventListener
```

---

## File Organization

```
RuntimeManager/
├── RuntimeManager.cpp             Plugin wrapper
├── RuntimeManager.h               Plugin class definition
├── RuntimeManagerImplementation.cpp Core implementation
├── RuntimeManagerImplementation.h   Implementation class
├── DobbySpecGenerator.cpp         OCI spec generation
├── DobbySpecGenerator.h           Spec generator class
├── WindowManagerConnector.cpp     WindowManager integration
├── WindowManagerConnector.h       Connector class
├── DobbyEventListener.cpp         Dobby event handling
├── DobbyEventListener.h           Event listener class
├── UserIdManager.cpp              User ID allocation
├── UserIdManager.h                User ID manager class
├── IEventHandler.h                Event handler interface
├── Module.cpp/h                   Module registration
├── CMakeLists.txt                 Build configuration
└── RuntimeManager.config          Runtime configuration
```

---

## Key Data Structures

```cpp
typedef struct _RuntimeAppInfo {
    string appId;
    string appInstanceId;
    int32_t containerId;
    string containerName;
    string displayName;
    RuntimeConfig config;
    ContainerState state;
} RuntimeAppInfo;
```

---

## API Reference

### IRuntimeManager Interface

| Method | Purpose |
|--------|---------|
| `Run(appInstanceId, config)` | Start application in OCI container |
| `Suspend(appInstanceId)` | Pause container (SIGSTOP to all processes) |
| `Resume(appInstanceId)` | Resume suspended container (SIGCONT) |
| `Hibernate(appInstanceId)` | Checkpoint container to disk using CRIU |
| `Wake(appInstanceId)` | Restore container from CRIU checkpoint |
| `Terminate(appInstanceId)` | Graceful container shutdown (SIGTERM + timeout) |
| `Kill(appInstanceId)` | Force kill container (SIGKILL) |

---

## Container Lifecycle

```mermaid
stateDiagram-v2
    [*] --> NotRunning
    NotRunning --> Starting: Run()
    Starting --> Running: Container Started
    Running --> Paused: Suspend()
    Paused --> Running: Resume()
    Running --> Checkpointing: Hibernate()
    Checkpointing --> Hibernated: Checkpoint Complete
    Hibernated --> Restoring: Wake()
    Restoring --> Running: Restore Complete
    Running --> Stopping: Terminate()
    Paused --> Stopping: Terminate()
    Hibernated --> Stopping: Terminate()
    Stopping --> NotRunning: Container Stopped
    Running --> NotRunning: Kill()
    NotRunning --> [*]
```

---

## Run Container Sequence

```mermaid
sequenceDiagram
    participant LCM as LifecycleManager
    participant RTM as RuntimeManager
    participant DSG as DobbySpecGenerator
    participant WMC as WindowManagerConnector
    participant Dobby

    LCM->>RTM: Run(appInstanceId, config)
    RTM->>DSG: generateSpec(config)
    DSG-->>RTM: dobbySpec JSON
    RTM->>WMC: createDisplay(displayName)
    WMC-->>RTM: display created
    RTM->>Dobby: startContainerFromSpec(spec)
    Dobby-->>RTM: containerId
    RTM->>RTM: Store RuntimeAppInfo
    RTM-->>LCM: success
    Note over Dobby: Container Running
    Dobby->>RTM: onContainerStarted(containerId)
    RTM->>LCM: OnRuntimeManagerEvent(STARTED)
```

---

## Hibernate/Wake Flow

```mermaid
sequenceDiagram
    participant LCM as LifecycleManager
    participant RTM as RuntimeManager
    participant Dobby
    participant CRIU

    Note over LCM,CRIU: Hibernate Flow
    LCM->>RTM: Hibernate(appInstanceId)
    RTM->>Dobby: hibernateContainer(containerId)
    Dobby->>CRIU: checkpoint(pid)
    CRIU-->>Dobby: checkpoint files saved
    Dobby-->>RTM: hibernate complete
    RTM-->>LCM: success

    Note over LCM,CRIU: Wake Flow
    LCM->>RTM: Wake(appInstanceId)
    RTM->>Dobby: wakeContainer(containerId)
    Dobby->>CRIU: restore(checkpoint)
    CRIU-->>Dobby: process restored
    Dobby-->>RTM: wake complete
    RTM-->>LCM: success
```

---

## Dobby Integration

### Dobby Spec Generation

RuntimeManager generates OCI-compliant container specifications including:

| Component | Description |
|-----------|-------------|
| `rootfs` | Application bundle path from PackageManager |
| `mounts` | Bind mounts for storage, Wayland socket, device nodes |
| `env` | WAYLAND_DISPLAY, XDG_RUNTIME_DIR, app-specific vars |
| `cgroups` | Memory limits, CPU shares |
| `namespaces` | PID, network, mount isolation |

### Event Handling

| Dobby Event | RuntimeManager Action |
|-------------|----------------------|
| Container Started | Notify LifecycleManager → LOADING complete |
| Container Stopped | Cleanup display, notify LifecycleManager |
| Container Crashed | Notify with error, cleanup resources |

---

[← Back to Main](../README.md) | [Next: PackageManager →](./PackageManager.md)

