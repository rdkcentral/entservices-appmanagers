# AppStorageManager Plugin Documentation

> Application-Specific Storage Allocation and Management for RDK Infrastructure

## 1. High-Level Purpose & Architecture

### Role in ENT / RDK Infrastructure

The **AppStorageManager** plugin provides dedicated storage management for applications, handling creation, access, and cleanup of application-specific storage directories with proper ownership and permissions.

### Responsibilities

- **Storage Creation**: Create dedicated storage directories for applications
- **Storage Access**: Provide storage paths with proper UID/GID ownership
- **Storage Cleanup**: Clear individual or all application storage
- **Quota Management**: Track storage usage per application

### Interacting Subsystems

| Subsystem | Interaction Type | Purpose |
|-----------|-----------------|---------|
| AppManager | COM-RPC (inbound) | Clear app data requests |
| RuntimeManager | COM-RPC (inbound) | Get app storage info |
| PackageManager | COM-RPC (inbound) | Storage allocation |

---

## 2. Architectural Overview

```mermaid
graph TB
    subgraph "AppStorageManager Plugin"
        Shell[AppStorageManager<br/>Plugin Shell]
        Impl[StorageManagerImplementation<br/>Core Logic]
    end

    subgraph "Consumers"
        AM[AppManager]
        RTM[RuntimeManager]
        PKG[PackageManager]
    end

    subgraph "System"
        FS[FileSystem]
    end

    AM --> Shell
    RTM --> Shell
    PKG --> Shell
    Shell --> Impl
    Impl --> FS
```

---

## 3. Code Organization (Folder & File-Level)

### Directory Structure

```
AppStorageManager/
├── AppStorageManager.cpp              # Plugin shell
├── AppStorageManager.h                # Shell header
├── AppStorageManagerImplementation.cpp # Core implementation
├── AppStorageManagerImplementation.h   # Implementation header
├── RequestHandler.cpp                 # Request processing
├── RequestHandler.h                   # RequestHandler header
├── AppStorageManagerTelemetryReporting.cpp # Telemetry
├── AppStorageManagerTelemetryReporting.h   # Telemetry header
├── Module.cpp                         # Plugin module
├── Module.h                           # Module header
├── CMakeLists.txt                     # Build configuration
├── AppStorageManager.config           # Plugin configuration
└── AppStorageManager.conf.in          # Configuration template
```

---

## 4. Class & Interface Documentation

### Exchange::IAppStorageManager Interface

```cpp
interface IAppStorageManager {
    hresult CreateStorage(const string& appId, uint32_t size,
                          string& path, string& errorReason);
    hresult GetStorage(const string& appId, int32_t userId, int32_t groupId,
                       string& path, uint32_t& size, uint32_t& used);
    hresult DeleteStorage(const string& appId, string& errorReason);
    hresult Clear(const string& appId, string& errorReason);
    hresult ClearAll(const string& exemptionAppIds, string& errorReason);
};
```

### StorageManagerImplementation

```cpp
// From AppStorageManagerImplementation.h
class StorageManagerImplementation : public Exchange::IAppStorageManager,
                                     public Exchange::IConfiguration {
private:
    class Config : public Core::JSON::Container {
    public:
        Core::JSON::String Path;  // Base storage path
    };

    Config _config;
    PluginHost::IShell* mCurrentservice;
    std::string mBaseStoragePath;

public:
    Core::hresult CreateStorage(const string& appId, const uint32_t& size,
                                string& path, string& errorReason) override;
    Core::hresult GetStorage(const string& appId, const int32_t& userId,
                             const int32_t& groupId, string& path,
                             uint32_t& size, uint32_t& used) override;
    Core::hresult DeleteStorage(const string& appId, string& errorReason) override;
    Core::hresult Clear(const string& appId, string& errorReason) override;
    Core::hresult ClearAll(const string& exemptionAppIds, string& errorReason) override;
    uint32_t Configure(PluginHost::IShell* service) override;
};
```

---

## 5. Configuration & Build Integration

The implementation configuration key is `path`; the plugin-level configuration also contains `mode`, `locator`, `autostart`, and optional `startuporder`. [AppStorageManager.conf.in](AppStorageManager.conf.in) is the generated template and [AppStorageManager.config](AppStorageManager.config) is the checked-in helper form. The build optionally enables `RALF_PACKAGE_SUPPORT` and wraps filesystem calls for L1 tests.

The effective platform default for an empty configured path is not established by this subsystem.

### Plugin Configuration

```cmake
set (autostart false)
set (preconditions Platform)
set (callsign "org.rdk.AppStorageManager")
```

### Runtime Configuration

```json
{
    "path": "/opt/persistent/apps"
}
```

### Storage Structure

/opt/persistent/apps/
├── com.example.app1/
├── com.example.app2/
└── ...
```

## 6. Internal Workflows & Execution Flow

### Storage Creation Flow

```mermaid
sequenceDiagram
    participant Client
    participant ASM as AppStorageManager
    participant FS as FileSystem

    Client->>ASM: CreateStorage(appId, size)
    ASM->>ASM: Calculate path: baseStoragePath/appId
    ASM->>FS: mkdir(path)
    ASM->>FS: Set permissions
    ASM-->>Client: path, success
```

### Storage Clear Flow

```mermaid
sequenceDiagram
    participant AM as AppManager
    participant ASM as AppStorageManager
    participant FS as FileSystem

    AM->>ASM: Clear(appId)
    ASM->>FS: Get storage path
    ASM->>FS: Remove contents (preserve directory)
    ASM-->>AM: success/error
```

### ClearAll with Exemptions

```mermaid
flowchart TD
    A[ClearAll called] --> B[Parse exemption list]
    B --> C[List all app directories]
    C --> D{For each directory}
    D --> E{In exemption list?}
    E -->|Yes| F[Skip]
    E -->|No| G[Clear contents]
    F --> D
    G --> D
    D -->|Done| H[Return success]
```

---

## 7. Diagrams & Visual Aids

```mermaid
classDiagram
    class StorageManagerImplementation
    class RequestHandler
    StorageManagerImplementation --> RequestHandler : delegates filesystem work
    StorageManagerImplementation ..|> IAppStorageManager
    StorageManagerImplementation ..|> IConfiguration
```

```mermaid
stateDiagram-v2
    [*] --> Unconfigured
    Unconfigured --> Ready: Configure(service)
    Ready --> Operating: storage request
    Operating --> Ready: completed
    Operating --> Error: filesystem or store failure
    Ready --> Stopped: teardown
```

## 8. Testing & Quality Analysis

### Existing Tests

Located in `Tests/L1Tests/tests/test_AppStorageManager.cpp`

| Test | Description |
|------|-------------|
| CreateStorage | Storage directory creation |
| GetStorage | Storage info retrieval |
| Clear | Individual app storage clear |
| ClearAll | Clear all with exemptions |
| DeleteStorage | Storage deletion |

---

The repository also has L0 lifecycle, implementation, and component tests under [Tests/L0Tests/AppStorageManager](../Tests/L0Tests/AppStorageManager), plus L1 and L2 coverage. Add tests for quota boundaries, ownership/path validation, partial filesystem failure, empty configured paths, and RALF-enabled behavior.

## 9. Beginner-to-Expert Teaching Mode

**Must know first:** distinguish persistent application data from package contents and runtime state. Learn how `path` becomes app-specific storage and how quota/ownership outputs are returned.

**Advanced path:** trace `Configure` through cache and PersistentStore setup, inspect `RequestHandler`, then compare normal and `RALF_PACKAGE_SUPPORT` builds.
