# PackageManager Module

> Application Package Lifecycle Management

[← Back to Main](../README.md) | [← Previous: RuntimeManager](../RuntimeManager/RuntimeManager.md)


---

## Purpose & Role

The **PackageManager** handles the complete lifecycle of application packages including download, installation, extraction, metadata parsing, locking during execution, and uninstallation.

### Core Responsibilities

- **Package Download:** Coordinate with DownloadManager for HTTP downloads
- **Installation:** Extract packages, parse manifests, setup storage
- **Package Locking:** Prevent uninstall while app is running
- **Metadata Management:** Parse and serve app configuration
- **Uninstallation:** Clean removal of packages and storage

### Dependencies

| Module | Purpose |
|--------|---------|
| DownloadManager | HTTP download operations |
| AppStorageManager | App storage allocation |

---

## Architecture

```mermaid
graph TB
    subgraph "PackageManager Module"
        PM[PackageManager<br/>Plugin]
        PMI[PackageManagerImplementation]
        DL[IPackageDownloader]
        INS[IPackageInstaller]
        HDL[IPackageHandler]
    end

    subgraph "External Services"
        DLMgr[DownloadManager]
        StorMgr[AppStorageManager]
        FS[FileSystem]
    end

    PM --> PMI
    PMI -.-> DL
    PMI -.-> INS
    PMI -.-> HDL
    PMI --> DLMgr
    PMI --> StorMgr
    PMI --> FS
```

---

## Class Diagram

```mermaid
classDiagram
    class PackageManagerImplementation {
        -map~string,PackageState~ mPackages
        -map~uint32_t,LockInfo~ mLocks
        -IDownloadManager* mDownloadManager
        -Exchange::IAppStorageManager* mStorageManager
        +Download(url, appId) hresult
        +Install(appId) hresult
        +Uninstall(appId) hresult
        +Lock(appId) hresult
        +Unlock(lockId) hresult
        +GetMetadata(appId) string
    }

    class IPackageDownloader {
        <<interface>>
        +Download(url, appId) hresult
        +CancelDownload(appId) hresult
        +GetDownloadStatus(appId) Status
    }

    class IPackageInstaller {
        <<interface>>
        +Install(appId, path) hresult
        +Uninstall(appId) hresult
        +IsInstalled(appId) bool
    }

    class IPackageHandler {
        <<interface>>
        +Lock(appId) lockId
        +Unlock(lockId) hresult
        +GetPackageInfo(appId) PackageInfo
        +ListPackages() list
    }

    PackageManagerImplementation ..|> IPackageDownloader
    PackageManagerImplementation ..|> IPackageInstaller
    PackageManagerImplementation ..|> IPackageHandler
```

---

## File Organization

```
PackageManager/
├── PackageManager.cpp             Plugin wrapper
├── PackageManager.h               Plugin class definition
├── PackageManagerImplementation.cpp Core implementation
├── PackageManagerImplementation.h   Implementation class
├── HttpClient.cpp                 HTTP download client
├── HttpClient.h                   HTTP client class
├── Module.cpp/h                   Module registration
├── CMakeLists.txt                 Build configuration
└── PackageManager.conf.in         Configuration template
```

---

## Key Data Structures

```cpp
struct PackageState {
    string appId;
    string version;
    string installedPath;
    string downloadPath;
    PackageStatus status; // DOWNLOADING, INSTALLED, etc.
    RuntimeConfig config;
    string appMetadata;
};

struct LockInfo {
    uint32_t lockId;
    string appId;
    uint32_t refCount;
};
```

---

## API Reference

### IPackageDownloader Interface

| Method | Purpose |
|--------|---------|
| `Download(url, options, downloadId)` | Download package from URL with specified options, returning a download handle |
| `CancelDownload(downloadId)` | Cancel in-progress download identified by handle |
| `GetDownloadStatus(downloadId)` | Get download progress for a given download handle |

### IPackageInstaller Interface

| Method | Purpose |
|--------|---------|
| `Install(appId, packagePath, appMetadata)` | Install package: extract, parse manifest, create storage |
| `Uninstall(appId)` | Uninstall package: remove files, cleanup storage |
| `IsInstalled(appId)` | Check if package is installed |

### IPackageHandler Interface

| Method | Purpose |
|--------|---------|
| `Lock(packageId, version, lockOptions, lockHandle)` | Lock a specific package version to prevent uninstall during app execution, returning a lock handle and related info |
| `Unlock(packageId, version, lockHandle)` | Release a previously acquired package lock after app terminates |
| `GetMetadata(packageId, version)` | Get application/package metadata JSON for a specific package version |
| `ListPackages()` | List all installed packages |

---

## Workflows

### Package Installation Flow

```mermaid
sequenceDiagram
    participant Client
    participant PkgMgr as PackageManager
    participant DLMgr as DownloadManager
    participant StorMgr as StorageManager
    participant FS as FileSystem

    Client->>PkgMgr: Download(url, appId)
    PkgMgr->>DLMgr: Download(url, destPath)
    DLMgr-->>PkgMgr: OnDownloadComplete
    PkgMgr->>FS: Extract(tarball, destPath)
    PkgMgr->>PkgMgr: Parse manifest.json
    PkgMgr->>StorMgr: CreateStorage(appId)
    StorMgr-->>PkgMgr: storagePath
    PkgMgr->>PkgMgr: Update PackageState
    PkgMgr-->>Client: OnInstalled(appId)
```

### Package Lock/Unlock Flow

```mermaid
sequenceDiagram
    participant AppMgr as AppManager
    participant PkgMgr as PackageManager

    Note over AppMgr,PkgMgr: App Launch
    AppMgr->>PkgMgr: Lock(packageId, version, lockOptions, lockHandle)
    PkgMgr->>PkgMgr: Create/Update LockInfo for (packageId, version), refCount=1
    PkgMgr-->>AppMgr: lockHandle, unpackedPath, config

    Note over AppMgr,PkgMgr: App Running...

    Note over AppMgr,PkgMgr: App Terminate
    AppMgr->>PkgMgr: Unlock(packageId, version, lockHandle)
    PkgMgr->>PkgMgr: refCount--, if 0 remove LockInfo for (packageId, version)
    PkgMgr-->>AppMgr: success
```

### Package Uninstall Flow

```mermaid
flowchart TD
    A[Uninstall Request] --> B{Package Locked?}
    B -->|Yes| C[Return ERROR_LOCKED]
    B -->|No| D[Remove Package Files]
    D --> E[Delete App Storage]
    E --> F[Remove from mPackages]
    F --> G[Emit OnUninstalled]
```

---

## Package Locking

Package locking prevents uninstallation while an application is running. When AppManager launches an app, it first locks the corresponding package (identified by `packageId` and `version`). The lock is released using the returned `lockHandle` when the app terminates.

### Lock States

| Scenario | Lock Count | Uninstall Allowed |
|----------|------------|-------------------|
| No app running | 0 | Yes |
| One app instance | 1 | No |
| Multiple instances | N | No |

> **Important:** Always ensure `Unlock(packageId, version, lockHandle)` is called in all termination paths (normal, crash, kill) to prevent package lock leaks.

---

[← Back to Main](../README.md) | [Next: DownloadManager →](../DownloadManager/DownloadManager.md)


