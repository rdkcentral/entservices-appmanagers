# StorageManager Module

> Application Storage Allocation & Management

[← Back to Main](./README.md) | [← Previous: DownloadManager](./DownloadManager.md)

---

## Purpose & Role

The **StorageManager** provides application-specific persistent storage allocation, management, and cleanup. Each installed app gets a dedicated storage directory with configurable quotas.

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
    subgraph "StorageManager Module"
        SM[StorageManager<br/>Plugin]
        SMI[StorageManagerImplementation]
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

    PM -->|CreateStorage/DeleteStorage| SM
    AM -->|Clear/ClearAll| SM
    RTM -->|GetStorage| SM
    SM --> SMI
    SMI --> RH
    RH --> FS
```

---

## Class Diagram

```mermaid
classDiagram
    class StorageManagerImplementation {
        -string mStorageBasePath
        -map~string,StorageInfo~ mStorageMap
        +CreateStorage(appId, type) hresult
        +GetStorage(appId) StoragePath
        +DeleteStorage(appId) hresult
        +Clear(appId) hresult
        +ClearAll() hresult
    }

    class StorageInfo {
        +string appId
        +string storagePath
        +uint64_t quota
        +uint64_t usedBytes
    }

    class IStorageManager {
        <<interface>>
        +CreateStorage(appId, type) hresult
        +GetStorage(appId) string
        +DeleteStorage(appId) hresult
        +Clear(appId) hresult
        +ClearAll() hresult
    }

    StorageManagerImplementation ..|> IStorageManager
    StorageManagerImplementation --> StorageInfo: manages
```

---

## File Organization

```
StorageManager/
├── StorageManager.cpp             Plugin wrapper
├── StorageManager.h               Plugin class definition
├── StorageManagerImplementation.cpp Core implementation
├── StorageManagerImplementation.h   Implementation class
├── RequestHandler.cpp             File system operations
├── RequestHandler.h               Request handler class
├── Module.cpp/h                   Module registration
├── CMakeLists.txt                 Build configuration
└── StorageManager.config          Runtime configuration
```

---

## API Reference

### IStorageManager Interface

| Method | Purpose |
|--------|---------|
| `CreateStorage(appId, type)` | Create storage directory for an application |
| `GetStorage(appId)` | Get the storage path for an application |
| `DeleteStorage(appId)` | Remove all storage for an application |
| `Clear(appId)` | Clear app data but keep storage allocated |
| `ClearAll()` | Clear data for all applications |

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
    participant StorMgr as StorageManager
    participant FS as FileSystem

    Note over PkgMgr,FS: App Installation
    PkgMgr->>StorMgr: CreateStorage(appId)
    StorMgr->>FS: mkdir(basePath/appId)
    StorMgr->>FS: mkdir(basePath/appId/data)
    StorMgr->>FS: mkdir(basePath/appId/cache)
    StorMgr-->>PkgMgr: storagePath

    Note over PkgMgr,FS: App Running (uses storage)

    Note over PkgMgr,FS: Clear Data Request
    PkgMgr->>StorMgr: Clear(appId)
    StorMgr->>FS: rm -rf basePath/appId/*
    StorMgr->>FS: recreate subdirs
    StorMgr-->>PkgMgr: success

    Note over PkgMgr,FS: App Uninstall
    PkgMgr->>StorMgr: DeleteStorage(appId)
    StorMgr->>FS: rm -rf basePath/appId
    StorMgr-->>PkgMgr: success
```

---

## Configuration

Loaded from `StorageManager.config`:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `storagepath` | `/opt/appdata` | Base storage directory |
| `defaultQuota` | (unlimited) | Default storage quota per app |

---

[← Back to Main](./README.md) | [Next: PreinstallManager →](./PreinstallManager.md)

