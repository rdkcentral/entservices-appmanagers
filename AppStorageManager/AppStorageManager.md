# AppStorageManager Module

> Application Storage Allocation & Management

[← Back to Main](../README.md) | [← Previous: DownloadManager](../DownloadManager/DownloadManager.md)

---

## Purpose & Role

The **AppStorageManager** provides application-specific persistent storage allocation, management, and cleanup. Each installed app gets a dedicated storage directory with configurable quotas.

### Core Responsibilities

- **Storage Creation:** Allocate app-specific storage directories
- **Storage Query:** Get storage paths for apps
- **Data Clear:** Clear app data while preserving structure
- **Storage Deletion:** Remove storage on app uninstall
- **Quota Management:** Enforce storage limits (if configured)

---

## Architecture

```mermaid
graph TB
    subgraph "AppStorageManager Module"
        ASM[AppStorageManager<br/>Plugin]
        ASMI[StorageManagerImplementation]
        RH[RequestHandler]
    end

    subgraph "Clients"
        PM[PackageManager]
        AM[AppManager]
        RTM[RuntimeManager]
    end

    subgraph "System"
        FS[FileSystem]
    end

    PM -->|CreateStorage/DeleteStorage| ASM
    AM -->|Clear/ClearAll| ASM
    RTM -->|GetStorage| ASM
    ASM --> ASMI
    ASMI --> RH
    RH --> FS
```

---

## Class Diagram

```mermaid
classDiagram
    class StorageManagerImplementation {
        +CreateStorage(...)
        +GetStorage(...)
        +DeleteStorage(...)
        +Clear(...)
        +ClearAll(...)
    }

    class IAppStorageManager {
        <<interface>>
        +CreateStorage(...)
        +GetStorage(...)
        +DeleteStorage(...)
        +Clear(...)
        +ClearAll(...)
    }

    StorageManagerImplementation ..|> IAppStorageManager
```

---

## File Organization

```
AppStorageManager/
├── AppStorageManager.cpp             Plugin wrapper
├── AppStorageManager.h               Plugin class definition
├── AppStorageManagerImplementation.cpp Core implementation
├── AppStorageManagerImplementation.h   Implementation class
├── RequestHandler.cpp                File system operations
├── RequestHandler.h                  Request handler class
├── Module.cpp/h                      Module registration
├── CMakeLists.txt                    Build configuration
└── AppStorageManager.config          Runtime configuration
```

---

## API Reference

### IAppStorageManager Interface

| Method | Purpose |
|--------|---------|
| `CreateStorage(appId, quota)` | Create a storage directory for an application with the specified quota |
| `GetStorage(appId)` | Get the storage path for an application |
| `DeleteStorage(appId)` | Remove all storage for an application |
| `Clear(appId)` | Clear app data but keep storage allocated |
| `ClearAll(exemptionAppIds)` | Clear data for all applications, optionally exempting those listed in `exemptionAppIds` |

---

## Storage Structure

```mermaid
graph TD
    A["/opt/appdata/"] --> B["com.app.one/"]
    A --> C["com.app.two/"]
    A --> D["com.app.three/"]

    B --> B1["cache/"]
    B --> B2["data/"]
    B --> B3["logs/"]

    C --> C1["cache/"]
    C --> C2["data/"]
    C --> C3["logs/"]
```

### Storage Types

| Type | Path | Purpose |
|------|------|---------|
| DATA | `{appId}/data/` | Persistent app data |
| CACHE | `{appId}/cache/` | Cacheable data (may be cleared) |
| LOGS | `{appId}/logs/` | Application logs |

---

## Storage Lifecycle

```mermaid
sequenceDiagram
    participant PkgMgr as PackageManager
    participant AppStorMgr as AppStorageManager
    participant FS as FileSystem

    Note over PkgMgr,FS: App Installation
    PkgMgr->>AppStorMgr: CreateStorage(appId)
    AppStorMgr->>FS: mkdir(basePath/appId)
    AppStorMgr->>FS: mkdir(basePath/appId/data)
    AppStorMgr->>FS: mkdir(basePath/appId/cache)
    AppStorMgr-->>PkgMgr: storagePath

    Note over PkgMgr,FS: App Running (uses storage)

    Note over PkgMgr,FS: Clear Data Request
    PkgMgr->>AppStorMgr: Clear(appId)
    AppStorMgr->>FS: rm -rf basePath/appId/*
    AppStorMgr->>FS: recreate subdirs
    AppStorMgr-->>PkgMgr: success

    Note over PkgMgr,FS: App Uninstall
    PkgMgr->>AppStorMgr: DeleteStorage(appId)
    AppStorMgr->>FS: rm -rf basePath/appId
    AppStorMgr-->>PkgMgr: success
```

---

## Configuration

Loaded from `AppStorageManager.config`:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `path` | `/opt/appdata` | Base storage directory for application data |

---

[← Back to Main](../README.md) | [Next: PreinstallManager →](../PreinstallManager/PreinstallManager.md)

