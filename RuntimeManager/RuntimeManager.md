# RuntimeManager Plugin Documentation

> Container Runtime Management using Dobby OCI in RDK Infrastructure

## 1. High-Level Purpose & Architecture

### Role in ENT / RDK Infrastructure

The **RuntimeManager** plugin manages application execution through OCI-compliant containers using the Dobby container runtime. It handles container lifecycle operations including starting, suspending, hibernating, and terminating application containers.

### Responsibilities

- **Container Lifecycle**: Start, stop, suspend, resume, hibernate, and wake containers
- **OCI Spec Generation**: Generate Dobby-compatible OCI specifications via `DobbySpecGenerator`
- **Window Management**: Coordinate display creation with RDKWindowManager via `WindowManagerConnector`
- **Event Handling**: Process container events from Dobby via `DobbyEventListener`
- **User ID Management**: Manage container user/group IDs via `UserIdManager`
- **AI Configuration**: Load runtime configuration from YAML via `AIConfiguration`

The `RUNTIMEMANAGER_ENABLE_DIALCONFIG` CMake option (default `ON`) enables DIAL
environment-variable generation in `DobbySpecGenerator` for DIAL-enabled
applications. Set `-DRUNTIMEMANAGER_ENABLE_DIALCONFIG=ON` when configuring both
RuntimeManager and its standalone L0 tests to build and test this behavior.

### Interacting Subsystems

| Subsystem | Interaction Type | Purpose |
|-----------|-----------------|---------|
| LifecycleManager | COM-RPC (inbound) | Receives Run, Suspend, Resume, Hibernate, Wake, Terminate requests |
| RDKWindowManager | COM-RPC (outbound) | Creates displays for applications |
| OCIContainer (Dobby) | COM-RPC (outbound) | Executes container operations |
| AppStorageManager | COM-RPC (outbound) | Gets application storage paths |

### RALF Package Launch

When RALF support is built in, `Run()` selects the RALF flow only when the
runtime configuration contains a non-empty `ralfPkgPath`. The normal widget
flow remains the default: it uses `DobbySpecGenerator` and starts from the
legacy unpacked package path. RALF uses `RalfPackageBuilder` to read the
PackageManager mount manifest, assemble the package-layer overlay and generate
the RALF OCI configuration; the generated rootfs path is passed to
`IOCIContainer::StartContainer`. Both paths continue to share display setup,
container lifecycle notifications and AppStorageManager integration.
When compiled with `RALF_PACKAGE_SUPPORT`, RuntimeManager's plugin configuration
accepts `"ralfEnabled": false` to reject RALF launch requests explicitly
(default `true`). This launch-time switch does not disable package installation,
verification or the build-time libpackage RALF option; ordinary widgets remain
unaffected. Secure-debug signature fallback is not enabled by this switch.

PackageManager owns RALF verification, installation and package mounts. It
provides RuntimeManager a neutral manifest of package rootfs paths and the
associated RDK `config.json` files. The RALF builder unmounts its per-instance
overlay on stop/failure, while PackageManager releases its package/dependency
locks through the normal unlock lifecycle.

`ENTOS_RALF_SUPPORT` is a RuntimeManager build option enabled by default (requires
`RALF_PACKAGE_SUPPORT` and LibXml2 headers/libraries in the build sysroot).
When enabled, RALF reads
`/etc/rdk/vendor-gpu.json`, generates an instance-scoped
vendor layer under `/run/sky/bundles/<instance>/vendor`, and uses the Broadcom-style
device glob, bind, file-copy, and symlink entries instead of the preinstalled
`/usr/share/gpu-layer/rootfs`. With the option off, the existing static GPU
layer behavior is unchanged. RuntimeManager checks bundle-directory creation
and keeps the bundle tree owned by the RuntimeManager process with `0755`
directories before passing it to Dobby. The generated OCI config remains
process-owned with mode `0644`; app UID/GID values are applied through the OCI
mappings, not to host-side bundle directories. After generating the OCI config,
mount-target preparation creates non-bind OCI mountpoint directories, the
legacy standard `/home` directory, and bind targets inside the merged rootfs
before Dobby applies the read-only root setting. It does so without chowning
path ancestors (including `/run/sky/bundles/<instance>`); only newly created
file targets may receive the app UID/GID. RALF also passes the Rialto
connector's allocated socket path through as both `RIALTO_SOCKET_PATH` and the
OCI bind source/destination. A temporary EntOS-only workaround redirects the
missing Broadcom `libwayland-egl.so.1.20.0` host source to the device's
`libwayland-egl.so.1` soname while preserving the configured container path.
In this mode, a manifest containing a base package builds OCI `process.args`
with its init entrypoint first, followed by the runtime and app entrypoints and
the application's entryArgs. A RALF app with a W3C widget runtime instead reads
the widget's `config.xml` content executable and mounts from the RALF manifest,
then uses `DobbySpecGenerator` and `StartContainerFromDobbySpec` with `/runtime`
and `/package`. This matches legacy's traditional host-rootfs launch: the
two-layer RALF overlay lacks the ELF loader needed by `SkyBrowserLauncher`.
If a base-free runtime has no executable, the traditional launch runs the
app's entry point and omits the `/runtime` bind mount.
For this widget path, `Run()` uses the app package's `data.img` as the private
ext4 image when present, otherwise skips the `/home/private` loop mount. It
never treats the RALF package directory itself as an image. The classic spec
also enables the networking and AppServices plugins so the browser can discover
its local AS endpoints from inside the container.
RALF package identity and unlock handling remain unchanged. OCI packages with
a missing base init fail generation before Dobby starts. When
`RDK_APPMANAGERS_DEBUG` is enabled, the generated RALF OCI config is also
written to `/tmp/generatedRalfSpec` for inspection after bundle cleanup.

For launch-path diagnostics, debug logs trace `ralfPkgPath` from PackageManager's
lock result through AppManager, LifecycleManager, and RuntimeManagerHandler to
RuntimeManager `Run()`. A non-empty path selects RALF; an empty path selects the
legacy widget flow. These temporary diagnostics use the `[ralf-debug]` keyword
for easy filtering and removal. RuntimeManager also logs whether RALF support was
compiled in.

The Yocto recipe enables the `ralfsupport` PACKAGECONFIG only when the
`enable_ralf` distro feature is present and packages the installed
`/usr/share/ralf/oci-base-spec.json` with the application-managers package.
For legacyappserviced-main parity, current mount-layer differences, and
diagnosis of the EntOS overlay `ENOENT`, see
[RALF Mount Parity and Overlay Failure Analysis](RALF_Mount_Parity_Analysis.md).

### What It Does NOT Do

- Does not manage application lifecycle state machine (handled by LifecycleManager)
- Does not handle package installation (handled by PackageManager)
- Does not provide JSON-RPC API (internal service only)

---

## 2. Architectural Overview

### Major Components

```mermaid
graph TB
    subgraph "RuntimeManager Plugin"
        Shell[RuntimeManager<br/>Plugin Shell]
        Impl[RuntimeManagerImplementation<br/>Container Orchestration]
        DSG[DobbySpecGenerator<br/>OCI Spec Builder]
        DEL[DobbyEventListener<br/>Container Events]
        WMC[WindowManagerConnector<br/>Display Bridge]
        UIM[UserIdManager<br/>UID/GID Management]
        AIC[AIConfiguration<br/>YAML Config Loader]
        RPB[RalfPackageBuilder<br/>RALF OCI Rootfs/Spec]
    end

    subgraph "External Services"
        OCI[IOCIContainer<br/>Dobby Plugin]
        WM[RDKWindowManager]
        ASM[AppStorageManager]
    end

    Shell --> Impl
    Impl --> DSG
    Impl --> DEL
    Impl --> WMC
    Impl --> UIM
    Impl --> AIC
    Impl -->|non-empty ralfPkgPath| RPB
    DEL --> OCI
    WMC --> WM
    Impl --> ASM
```

### Container Lifecycle

```mermaid
stateDiagram-v2
    [*] --> NotRunning
    NotRunning --> Starting: Run()
    Starting --> Running: Container Started
    Running --> Suspending: Suspend()
    Suspending --> Suspended: Suspended
    Suspended --> Resuming: Resume()
    Resuming --> Running: Resumed
    Running --> Hibernating: Hibernate()
    Hibernating --> Hibernated: Hibernated
    Hibernated --> Waking: Wake()
    Waking --> Running: Woke
    Running --> Terminating: Terminate()
    Suspended --> Terminating: Terminate()
    Hibernated --> Terminating: Terminate()
    Terminating --> Stopped: Container Stopped
    Stopped --> [*]
```

---

## 3. Code Organization (Folder & File-Level)

### Directory Structure

```
RuntimeManager/
├── RuntimeManager.cpp              # Plugin shell
├── RuntimeManager.h                # Shell header
├── RuntimeManagerImplementation.cpp # Core container management
├── RuntimeManagerImplementation.h   # Implementation header
├── DobbySpecGenerator.cpp          # OCI spec generation
├── DobbySpecGenerator.h            # Spec generator header
├── DobbyEventListener.cpp          # Container event handling
├── DobbyEventListener.h            # Event listener header
├── WindowManagerConnector.cpp      # RDKWindowManager bridge
├── WindowManagerConnector.h        # Connector header
├── UserIdManager.cpp               # UID/GID management
├── UserIdManager.h                 # UserIdManager header
├── AIConfiguration.cpp             # YAML configuration loader
├── AIConfiguration.h               # AI config header
├── ApplicationConfiguration.h      # App config structure
├── IEventHandler.h                 # Event handler interface
├── RialtoConnector.cpp             # Rialto integration (optional)
├── RialtoConnector.h               # Rialto header
├── RuntimeManagerTelemetryReporting.cpp # Telemetry
├── RuntimeManagerTelemetryReporting.h   # Telemetry header
├── Module.cpp                      # Plugin module
├── Module.h                        # Module header
├── Gateway/                        # Gateway components
├── ralf/                           # RALF package support
├── CMakeLists.txt                  # Build configuration
├── RuntimeManager.config           # Plugin configuration
└── RuntimeManager.conf.in          # Configuration template
```

### File-by-File Breakdown

#### RuntimeManager.h / RuntimeManager.cpp

**Purpose**: Thin plugin shell that exposes the RuntimeManager to Thunder.

```cpp
// From RuntimeManager.h (lines 35-43)
class RuntimeManager : public PluginHost::IPlugin {
    BEGIN_INTERFACE_MAP(RuntimeManager)
    INTERFACE_ENTRY(PluginHost::IPlugin)
    INTERFACE_AGGREGATE(Exchange::IRuntimeManager, mRuntimeManagerImpl)
    END_INTERFACE_MAP

    const string Initialize(PluginHost::IShell* service) override;
    void Deinitialize(PluginHost::IShell* service) override;
};
```

#### RuntimeManagerImplementation.h / RuntimeManagerImplementation.cpp

**Purpose**: Core container orchestration logic implementing `IRuntimeManager`.

**Key Types**:
`RuntimeEventType` values:

| Value |
|-------|
| `RUNTIME_MANAGER_EVENT_UNKNOWN` |
| `RUNTIME_MANAGER_EVENT_STATECHANGED` |
| `RUNTIME_MANAGER_EVENT_CONTAINERSTARTED` |
| `RUNTIME_MANAGER_EVENT_CONTAINERSTOPPED` |
| `RUNTIME_MANAGER_EVENT_CONTAINERFAILED` |

`RequestType` values:

| Value |
|-------|
| `REQUEST_TYPE_NONE` |
| `REQUEST_TYPE_LAUNCH` |
| `REQUEST_TYPE_SUSPEND` |
| `REQUEST_TYPE_RESUME` |
| `REQUEST_TYPE_HIBERNATE` |
| `REQUEST_TYPE_WAKE` |
| `REQUEST_TYPE_TERMINATE` |
| `REQUEST_TYPE_KILL` |

`RuntimeAppInfo` fields:

| Field | Type | Description |
|-------|------|-------------|
| `appId` | `std::string` | Application identifier |
| `appInstanceId` | `std::string` | Runtime instance identifier |
| `descriptor` | `uint32_t` | Runtime/container descriptor |
| `containerState` | `RuntimeState` | Current runtime state |
| `requestTime` | `time_t` | Timestamp for active request |
| `requestType` | `RequestType` | Request type currently being processed |

**Key Methods**:
```cpp
// IRuntimeManager interface implementation
Core::hresult Run(const string& appId, const string& appInstanceId,
                  uint32_t userId, uint32_t groupId,
                  IValueIterator* const& ports,
                  IStringIterator* const& paths,
                  IStringIterator* const& debugSettings,
                  const string& runtimeConfigPayload);
Core::hresult Hibernate(const string& appInstanceId);
Core::hresult Wake(const string& appInstanceId, RuntimeState runtimeState);
Core::hresult Suspend(const string& appInstanceId);
Core::hresult Resume(const string& appInstanceId);
Core::hresult Terminate(const string& appInstanceId);
Core::hresult Kill(const string& appInstanceId);
Core::hresult GetInfo(const string& appInstanceId, string& info);
Core::hresult Annotate(const string& appInstanceId, const string& key, const string& value);
```

#### DobbySpecGenerator.h / DobbySpecGenerator.cpp

**Purpose**: Generates OCI-compliant container specifications for Dobby.

`RuntimeManager::Run` is the only manager boundary that validates and decodes the flat opaque JSON payload into the private `RuntimeConfiguration` type. The decoder consumes known fields for OCI generation; upstream managers do not share that structure. JSON arrays remain arrays throughout the payload flow, and enrichment preserves unknown properties.

**Key Methods**:
```cpp
class DobbySpecGenerator {
public:
    DobbySpecGenerator(AIConfiguration& aiConfiguration);
    bool generate(const ApplicationConfiguration& config,
                  const RuntimeConfiguration& runtimeConfig,
                  string& outputJsonString);

private:
    Json::Value createEnvVars(const ApplicationConfiguration& config,
                              const RuntimeConfiguration& runtimeConfig,
                              const std::vector<std::pair<std::string, std::string>>& capabilities) const;
    Json::Value createMounts(const ApplicationConfiguration& config,
                             const RuntimeConfiguration& runtimeConfig) const;
    Json::Value createRdkPlugins(const ApplicationConfiguration& config,
                                 const RuntimeConfiguration& runtimeConfig,
                                 const std::vector<std::pair<std::string, std::string>>& capabilities) const;
    Json::Value createNetworkPlugin(...) const;
    Json::Value createThunderPlugin(...) const;
    Json::Value createOpenCDMPlugin(...) const;
    // ... additional plugin creation methods
};
```

#### DobbyEventListener.h / DobbyEventListener.cpp

**Purpose**: Listens for container events from the Dobby OCIContainer plugin.

```cpp
class DobbyEventListener {
    class OCIContainerNotification : public Exchange::IOCIContainer::INotification {
        void OnContainerStarted(const string& containerId, const string& name) override;
        void OnContainerStopped(const string& containerId, const string& name) override;
        void OnContainerFailed(const string& containerId, const string& name, uint32_t error) override;
        void OnContainerStateChanged(const string& containerId, ContainerState state) override;
    };

public:
    bool initialize(PluginHost::IShell* service, IEventHandler* eventHandler,
                    Exchange::IOCIContainer* ociContainerObject);
    bool deinitialize();
};
```

#### WindowManagerConnector.h / WindowManagerConnector.cpp

**Purpose**: Bridge to RDKWindowManager for display creation.

**Key Functionality**:
- Creates displays for applications before container start
- Monitors window manager events
- Handles display disconnection cleanup

#### UserIdManager.h / UserIdManager.cpp

**Purpose**: Manages unique user/group IDs for container isolation.

**Key Functionality**:
- Allocates unique UID/GID pairs for containers
- Tracks active allocations
- Releases IDs when containers stop

#### AIConfiguration.h / AIConfiguration.cpp

**Purpose**: Loads runtime configuration from YAML files.

**Configuration Sources**:
- Platform-wide runtime configuration
- Per-application overrides
- Memory limits, GPU settings, network configuration

---

## 4. Class & Interface Documentation

### Exchange::IRuntimeManager Interface

```cpp
interface IRuntimeManager {
    enum RuntimeState {
        UNKNOWN = 0,
        STARTING,
        RUNNING,
        SUSPENDING,
        SUSPENDED,
        RESUMING,
        HIBERNATING,
        HIBERNATED,
        WAKING,
        TERMINATING,
        STOPPED
    };

    interface INotification {
        void OnStateChanged(const string& appInstanceId, RuntimeState oldState,
                           RuntimeState newState, const string& errorReason);
    };

    hresult Register(INotification* notification);
    hresult Unregister(INotification* notification);
    hresult Run(const string& appId, const string& appInstanceId,
                uint32_t userId, uint32_t groupId,
                IValueIterator* ports, IStringIterator* paths,
                IStringIterator* debugSettings,
                const string& runtimeConfigPayload);
    hresult Hibernate(const string& appInstanceId);
    hresult Wake(const string& appInstanceId, RuntimeState state);
    hresult Suspend(const string& appInstanceId);
    hresult Resume(const string& appInstanceId);
    hresult Terminate(const string& appInstanceId);
    hresult Kill(const string& appInstanceId);
    hresult GetInfo(const string& appInstanceId, string& info);
    hresult Annotate(const string& appInstanceId, const string& key, const string& value);
    hresult Mount();
    hresult Unmount();
};
```

### IEventHandler Interface

Internal interface for event handling:

```cpp
class IEventHandler {
public:
    virtual void onOCIContainerStartedEvent(std::string name, JsonObject& data) = 0;
    virtual void onOCIContainerStoppedEvent(std::string name, JsonObject& data) = 0;
    virtual void onOCIContainerFailureEvent(std::string name, JsonObject& data) = 0;
    virtual void onOCIContainerStateChangedEvent(std::string name, JsonObject& data) = 0;
};
```

### Class Relationships

```mermaid
classDiagram
    class RuntimeManager {
        -IRuntimeManager* mRuntimeManagerImpl
        +Initialize()
        +Deinitialize()
    }

    class RuntimeManagerImplementation {
        -IOCIContainer* mOciContainerObject
        -IAppStorageManager* mStorageManagerObject
        -WindowManagerConnector* mWindowManagerConnector
        -DobbyEventListener* mDobbyEventListener
        -UserIdManager* mUserIdManager
        -AIConfiguration* mAIConfiguration
        -map~string,RuntimeAppInfo~ mRuntimeAppInfo
        +Run()
        +Hibernate()
        +Wake()
        +Suspend()
        +Resume()
        +Terminate()
        +Kill()
    }

    class DobbySpecGenerator {
        -AIConfiguration* mAIConfiguration
        +generate()
        -createEnvVars()
        -createMounts()
        -createRdkPlugins()
    }

    class DobbyEventListener {
        -IOCIContainer* mOCIContainerObject
        -IEventHandler* mEventHandler
        +initialize()
        +deinitialize()
    }

    class WindowManagerConnector {
        +createDisplay()
        +destroyDisplay()
    }

    class UserIdManager {
        +allocateId()
        +releaseId()
    }

    class AIConfiguration {
        +load()
        +getConfig()
    }

    RuntimeManager --> RuntimeManagerImplementation
    RuntimeManagerImplementation --> DobbySpecGenerator
    RuntimeManagerImplementation --> DobbyEventListener
    RuntimeManagerImplementation --> WindowManagerConnector
    RuntimeManagerImplementation --> UserIdManager
    RuntimeManagerImplementation --> AIConfiguration
    DobbySpecGenerator --> AIConfiguration
```

---

## 5. Configuration & Build Integration

### Plugin Configuration (RuntimeManager.config)

```cmake
set (autostart ${PLUGIN_RUNTIME_MANAGER_AUTOSTART})
set (preconditions Platform)
set (callsign "org.rdk.RuntimeManager")

map()
   key(root)
   map()
       kv(mode ${PLUGIN_RUNTIME_MANAGER_MODE})
       kv(locator lib${PLUGIN_IMPLEMENTATION}.so)
   end()
end()
ans(configuration)
```

### CMake Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `PLUGIN_RUNTIME_MANAGER_MODE` | Execution mode (Off/Local) | Off |
| `PLUGIN_RUNTIME_MANAGER_AUTOSTART` | Auto-start on boot | false |
| `PLUGIN_RUNTIME_APP_PORTAL` | Runtime app portal identifier | "" |
| `PLUGIN_RUNTIME_CONFIG_FILE` | Path to runtime config YAML | "" |
| `RALF_PACKAGE_SUPPORT` | Enable RALF package support | ON |
| `RIALTO_IN_DAC_FEATURE` | Enable Rialto in DAC | OFF |

### Dependencies

- `yaml-cpp` - For YAML configuration parsing
- `jsoncpp` - For JSON/OCI spec generation
- `WPEFramework` - Thunder framework
- `RialtoServerManager` (optional) - For Rialto integration

---

## 6. Internal Workflows & Execution Flow

### Container Start (Run) Flow

```mermaid
sequenceDiagram
    participant LCM as LifecycleManager
    participant RTM as RuntimeManagerImpl
    participant DSG as DobbySpecGenerator
    participant WMC as WindowManagerConnector
    participant UIM as UserIdManager
    participant OCI as OCIContainer

    LCM->>RTM: Run(appId, appInstanceId, config)
    RTM->>UIM: allocateId()
    UIM-->>RTM: userId, groupId
    RTM->>WMC: createDisplay(appInstanceId)
    WMC-->>RTM: displayCreated
    RTM->>DSG: generate(appConfig, runtimeConfig)
    DSG-->>RTM: dobbySpec (JSON)
    RTM->>OCI: StartContainerFromSpec(containerId, spec)
    OCI-->>RTM: descriptor
    RTM->>RTM: mRuntimeAppInfo[appInstanceId] = info
    RTM-->>LCM: Core::ERROR_NONE

    Note over OCI: Container starts...

    OCI->>RTM: OnContainerStarted(containerId)
    RTM->>LCM: OnStateChanged(RUNNING)
```

### Container Suspend Flow

```mermaid
sequenceDiagram
    participant LCM as LifecycleManager
    participant RTM as RuntimeManagerImpl
    participant OCI as OCIContainer

    LCM->>RTM: Suspend(appInstanceId)
    RTM->>RTM: getContainerId(appInstanceId)
    RTM->>OCI: PauseContainer(containerId)
    OCI-->>RTM: success
    RTM->>RTM: updateState(SUSPENDED)
    RTM-->>LCM: Core::ERROR_NONE
    RTM->>LCM: OnStateChanged(SUSPENDING -> SUSPENDED)
```

### Container Hibernate Flow

```mermaid
sequenceDiagram
    participant LCM as LifecycleManager
    participant RTM as RuntimeManagerImpl
    participant OCI as OCIContainer

    LCM->>RTM: Hibernate(appInstanceId)
    RTM->>RTM: getContainerId(appInstanceId)
    RTM->>OCI: HibernateContainer(containerId, options)
    OCI-->>RTM: success
    RTM->>RTM: updateState(HIBERNATED)
    RTM-->>LCM: Core::ERROR_NONE
    RTM->>LCM: OnStateChanged(HIBERNATING -> HIBERNATED)
```

### OCI Spec Generation Flow

```mermaid
flowchart TD
    A[generate] --> B[Load base OCI spec]
    B --> C[Parse capabilities]
    C --> D[createEnvVars]
    D --> E[createMounts]
    E --> F[createRdkPlugins]
    F --> G{GPU Enabled?}
    G -->|Yes| H[Add GPU mounts]
    G -->|No| I[Skip GPU]
    H --> J[createNetworkPlugin]
    I --> J
    J --> K[createThunderPlugin]
    K --> L[createOpenCDMPlugin]
    L --> M[Add memory limits]
    M --> N[Serialize to JSON]
    N --> O[Return spec string]
```

---

## 7. Diagrams & Visual Aids

### Complete Runtime State Diagram

```mermaid
stateDiagram-v2
    [*] --> UNKNOWN: Initial
    UNKNOWN --> STARTING: Run()
    STARTING --> RUNNING: Container Started
    STARTING --> STOPPED: Start Failed
    
    RUNNING --> SUSPENDING: Suspend()
    SUSPENDING --> SUSPENDED: Pause Complete
    SUSPENDED --> RESUMING: Resume()
    RESUMING --> RUNNING: Resume Complete
    
    RUNNING --> HIBERNATING: Hibernate()
    HIBERNATING --> HIBERNATED: Checkpoint Complete
    HIBERNATED --> WAKING: Wake()
    WAKING --> RUNNING: Restore Complete
    
    RUNNING --> TERMINATING: Terminate()
    SUSPENDED --> TERMINATING: Terminate()
    HIBERNATED --> TERMINATING: Terminate()
    
    TERMINATING --> STOPPED: Container Stopped
    
    STOPPED --> [*]
```

### Component Integration Diagram

```mermaid
graph TB
    subgraph "RuntimeManager"
        Impl[Implementation]
        DSG[DobbySpecGenerator]
        DEL[DobbyEventListener]
        WMC[WindowManagerConnector]
    end

    subgraph "External Plugins"
        OCI[OCIContainer/Dobby]
        WM[RDKWindowManager]
        ASM[AppStorageManager]
    end

    subgraph "System"
        Kernel[Linux Kernel]
        CGroups[cgroups]
        Namespaces[namespaces]
    end

    Impl --> DSG
    Impl --> DEL
    Impl --> WMC
    DSG -.->|OCI Spec| OCI
    DEL <-.->|Events| OCI
    WMC --> WM
    Impl --> ASM
    OCI --> Kernel
    OCI --> CGroups
    OCI --> Namespaces
```

### OCI Spec Structure

```mermaid
graph TD
    subgraph "Dobby OCI Spec"
        Root[Root Spec]
        Process[process]
        Mounts[mounts]
        Env[env]
        Plugins[rdkPlugins]
    end

    Root --> Process
    Root --> Mounts
    Root --> Env
    Root --> Plugins

    Plugins --> Network[networking]
    Plugins --> Thunder[thunder]
    Plugins --> OpenCDM[opencdm]
    Plugins --> AppServicesSDK[appservices-sdk]
    Plugins --> Minidump[minidump]
    Plugins --> EthanLog[ethanlog]
```

---

## 8. Testing & Quality Analysis

### Existing Tests

Located in `Tests/L1Tests/tests/test_RunTimeManager.cpp`:

| Test Category | Description |
|---------------|-------------|
| Run Tests | Container start with various configurations |
| Suspend/Resume Tests | Container pause/unpause |
| Hibernate/Wake Tests | Container checkpoint/restore |
| Terminate Tests | Graceful container shutdown |
| Kill Tests | Forced container termination |
| Event Tests | Container event handling |

### Test Coverage Gaps

1. **OCI Spec Generation**: Various capability combinations
2. **Error Recovery**: Container failure handling
3. **Resource Limits**: Memory/CPU limit enforcement
4. **Concurrent Operations**: Multiple container operations

### Suggested Test Cases

```cpp
// Test GPU-enabled container spec
TEST(RuntimeManagerTest, GenerateSpecWithGPU) {
    // Verify GPU mounts and environment variables
}

// Test memory limit configuration
TEST(RuntimeManagerTest, MemoryLimitEnforcement) {
    // Verify cgroup memory limits in spec
}

// Test container failure recovery
TEST(RuntimeManagerTest, ContainerFailureRecovery) {
    // Verify proper cleanup on container crash
}
```

---

## 9. Beginner-to-Expert Teaching Mode

### Must Know First

1. **OCI Containers**: Understand OCI runtime specification
2. **Dobby**: RDK's OCI-compliant container runtime
3. **Linux Namespaces/cgroups**: Container isolation fundamentals

### Beginner Level

1. Read `RuntimeManager.h` for the plugin structure
2. Examine a generated OCI spec to understand container configuration
3. Trace a simple Run() call through the code

### Intermediate Level

1. Study `DobbySpecGenerator` for OCI spec construction
2. Understand `DobbyEventListener` event flow
3. Examine `WindowManagerConnector` for display coordination

### Advanced Level

1. Study `AIConfiguration` YAML loading and merging
2. Understand capability-based plugin selection in spec generation
3. Analyze memory/GPU resource management

### Expert Level

1. Add new Dobby RDK plugins to spec generation
2. Implement custom container isolation features
3. Optimize container startup time
4. Add new runtime states for advanced lifecycle management
