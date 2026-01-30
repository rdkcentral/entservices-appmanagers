# PreinstallManager Module

> Pre-installed Application Management

[← Back to Main](../README.md) | [← Previous: StorageManager](./StorageManager.md)

---

## Purpose & Role

The **PreinstallManager** handles applications that are pre-installed on the device image. It scans configured directories for preinstalled packages and triggers their installation through PackageManager on system startup.

### Core Responsibilities

- **Directory Scanning:** Scan preinstall directories for app packages
- **Package Detection:** Identify new preinstalled packages
- **Installation Trigger:** Request PackageManager to install packages
- **Version Management:** Handle preinstalled app updates

### Dependencies

| Module | Purpose |
|--------|---------|
| PackageManager | Package installation |

---

## Architecture

```mermaid
graph TB
    subgraph "PreinstallManager Module"
        PIM[PreinstallManager<br/>Plugin]
        PIMI[PreinstallManagerImplementation]
    end

    subgraph "External"
        PkgMgr[PackageManager]
        FS[FileSystem<br/>Preinstall Dirs]
    end

    PIM --> PIMI
    PIMI -->|Scan| FS
    PIMI -->|Install| PkgMgr
```

---

## Class Diagram

```mermaid
classDiagram
    class PreinstallManagerImplementation {
        -list~string~ mPreinstallPaths
        -IPackageInstaller* mPackageInstaller
        +Configure(config) hresult
        +ScanAndInstall() hresult
        +GetPreinstalledApps() list
    }

    class IPreinstallManager {
        <<interface>>
        +ScanAndInstall() hresult
        +GetPreinstalledApps() list
    }

    PreinstallManagerImplementation ..|> IPreinstallManager
```

---

## File Organization

```
PreinstallManager/
├── PreinstallManager.cpp          Plugin wrapper
├── PreinstallManager.h            Plugin class definition
├── PreinstallManagerImplementation.cpp Core implementation
├── PreinstallManagerImplementation.h   Implementation class
├── Module.cpp/h                   Module registration
├── CMakeLists.txt                 Build configuration
└── PreinstallManager.config       Runtime configuration
```

---

## API Reference

### IPreinstallManager Interface

| Method | Purpose |
|--------|---------|
| `ScanAndInstall()` | Scan preinstall directories and install found packages |
| `GetPreinstalledApps()` | Get list of preinstalled application IDs |

---

## Startup Scan Flow

```mermaid
sequenceDiagram
    participant System
    participant PIM as PreinstallManager
    participant FS as FileSystem
    participant PkgMgr as PackageManager

    System->>PIM: Plugin Activated
    PIM->>PIM: Configure(paths)

    loop For each preinstall path
        PIM->>FS: ScanDirectory(path)
        FS-->>PIM: List of .tar.gz packages

        loop For each package
            PIM->>PkgMgr: IsInstalled(appId)?
            PkgMgr-->>PIM: installed/not installed

            alt Not Installed
                PIM->>PkgMgr: Install(appId, packagePath)
                PkgMgr-->>PIM: success
            end
        end
    end

    PIM-->>System: Scan Complete
```

---

## Preinstall Decision Flow

```mermaid
flowchart TD
    A[Scan Preinstall Directory] --> B[Found Package?]
    B -->|No| C[Done]
    B -->|Yes| D[Parse Manifest]
    D --> E{Already Installed?}
    E -->|No| F[Install Package]
    E -->|Yes| G{Version Higher?}
    G -->|No| H[Skip]
    G -->|Yes| I[Update Package]
    F --> J[Next Package]
    H --> J
    I --> J
    J --> B
```

---

## Preinstall Directory Structure

```
/opt/preinstall/
├── com.app.one/
│   ├── manifest.json
│   ├── bundle.tar.gz
│   └── metadata.json
├── com.app.two/
│   ├── manifest.json
│   ├── bundle.tar.gz
│   └── metadata.json
└── ...
```

---

## Configuration

Loaded from `PreinstallManager.config`:

| Parameter | Description |
|-----------|-------------|
| `preinstallpaths` | List of directories containing preinstalled packages |

---

[← Back to Main](../README.md) | [Next: RDKWindowManager →](./RDKWindowManager.md)

