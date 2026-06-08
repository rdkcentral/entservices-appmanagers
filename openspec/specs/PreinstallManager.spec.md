# PreinstallManager Specification

## Overview
**Module Name**: PreinstallManager
**Type**: WPEFramework Plugin
**Purpose**: Scan and install pre-installed application packages from device image directories

## Description
PreinstallManager handles the detection and installation of pre-installed application packages that are baked into the device image. On system startup, it scans configured preinstall directories for app packages, compares them against what is already installed, and triggers PackageManager to install any new or updated packages. It can also be triggered manually via the `Scan` API. Installation is executed asynchronously in a dedicated install thread (`installPackages`) to avoid blocking system startup.

## Requirements
- Automatically scan configured preinstall directories on system startup (`Initialize`)
- Detect new and updated packages by comparing current scan against previously installed packages
- Trigger `PackageManager::Install` for each detected package
- Provide JSON-RPC API for `Scan`, `ListPreinstalled`, `Install`, and `GetStatus`
- Execute installations asynchronously in a dedicated thread to avoid blocking startup
- Emit `OnPackageDetected`, `OnInstallStarted`, and `OnInstallComplete` notifications
- Only process packages whose install state is `INSTALLED` when checking for already-installed packages
- Support configurable scan directory via `PLUGIN_PREINSTALL_MANAGER_APP_PREINSTALL_DIRECTORY`
- Auto-start on boot via `PLUGIN_PREINSTALL_MANAGER_AUTOSTART`

## Architecture / Design

### High-Level Architecture
```
System Startup / Client (JSON-RPC)
    ↓
PreinstallManager::Initialize()
    ↓
PreinstallManagerImplementation
    ├→ Scan preinstall directories → build PackageInfo list
    ├→ Filter: exclude already-INSTALLED packages
    ├→ installPackages() (async thread)
    │   └→ PackageManager::Install() (JSON-RPC) for each package
    └→ Emit OnPackageDetected, OnInstallStarted, OnInstallComplete
```

### Key Components
- **PreinstallManagerImplementation**: Core logic — directory scan, install coordination, state tracking
- **installPackages**: Background thread method that processes the install queue sequentially
- **PreinstallManager**: Plugin entry point — handles `Initialize` and plugin lifecycle

## External Interfaces

### Public APIs (JSON-RPC)
| Method | Description |
|--------|-------------|
| `Scan()` | Initiate preinstall directory scan |
| `ListPreinstalled()` | List detected preinstalled packages |
| `Install(packageId)` | Install a specific preinstalled package |
| `GetStatus()` | Query scan/install status |

### Notifications
| Event | Description |
|-------|-------------|
| `OnPackageDetected` | New preinstalled package found during scan |
| `OnInstallStarted` | Installation of a package initiated |
| `OnInstallComplete` | Installation completed (success or failure) |

### Configuration
- **Config File**: `PreinstallManager.conf.in` / `PreinstallManager.config`
- **Scan Directories**: Configured via `PLUGIN_PREINSTALLMANAGER_SCAN_DIRECTORIES`
- **DBus Service**: Registered as WPEFramework plugin

## Data Flow
```
System Startup
    ↓
PreinstallManager::Initialize(service)
    ├→ PreinstallManagerImplementation created and configured
    └→ Scan preinstall directories

For each detected package:
    ├→ Check if already INSTALLED via PackageManager::PackageState
    ├→ If not installed: add to preinstallPackages list
    └→ Notify: OnPackageDetected

mInstallThread = std::thread(installPackages, preinstallPackages):
    For each package in list:
        ├→ Notify: OnInstallStarted
        ├→ PackageManager::Install(packageId, version, fileLocator, ...)
        └→ Notify: OnInstallComplete (success/failure)

Manual Scan (Client::Scan()):
    ├→ Re-scan directories
    ├→ Detect new packages vs. previous scan
    └→ Notify: OnPackageDetected for each new package
```

## Integration Points
- **Consumes from**:
  - Filesystem (preinstall directories; read-only device image)
- **Provides to**:
  - PackageManager (install requests)
  - System startup (ensures preinstalled apps are available before use)

## Performance
- Scanning is performed at startup and may delay readiness if directories are large
- Installation is executed asynchronously in a dedicated thread; failures do not block system startup
- Only packages not already in `INSTALLED` state are processed to avoid redundant reinstalls

## Security
- Preinstall directories are part of the device image (read-only); packages cannot be modified
- Version updates to preinstalled apps require a device image update

## Versioning & Compatibility
- **API Version**: 1.0.0
- **Plugin Execution Mode**: Configurable via `PLUGIN_PREINSTALL_MANAGER_MODE`
- `PLUGIN_PREINSTALL_MANAGER_AUTOSTART`: defaults to `false` (auto-start on boot)
- Preinstalled packages are immutable; the install copies them to mutable storage via PackageManager

## Conformance Testing & Validation
- **Unit Tests**: `Tests/L1Tests`
- **Integration Tests**: `Tests/L2Tests` (with mock preinstall directories)
- **Test Dependencies**: Mock PackageManager, sandbox preinstall directories

## Dependencies

### External Repositories
| Repository | Purpose | Optional |
|------------|---------|----------|
| entservices-apis | API interface definitions | No |

### System Libraries
| Library | Version | Purpose | Optional |
|---------|---------|---------|----------|
| WPEFramework | Latest | Plugin framework | No |

### Internal Dependencies
| Module | Type | Purpose |
|--------|------|---------|
| PackageManager | Plugin | Trigger package installation |
| AppManagersHelpers | Library | Shared utilities, logging |

### CMake Configuration
| Option | Type | Default | Purpose |
|--------|------|---------|---------|
| PLUGIN_PREINSTALLMANAGER_MODE | String | Off | Plugin execution mode |
| PLUGIN_PREINSTALLMANAGER_AUTOSTART | Boolean | true | Auto-start on boot |
| PLUGIN_PREINSTALLMANAGER_SCAN_DIRECTORIES | String | "" | Preinstall directories to scan |

## Build & Installation
- **Compiled Artifact**: `RdkCppPlugin_PreinstallManager.so`
- **Install Path**: `${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins`
- **Required Build Flags**: C++11
- **API Version**: 1.0.0

## Constraints & Limitations
- Depends on preinstall directories being mounted at boot
- Scanning is blocking per directory and may delay startup for large image directories
- Cannot modify preinstalled packages (read-only device image)
- Installation failures do not block system startup
- Version updates require a full device image update

## Covered Code
- PreinstallManager/PreinstallManagerImplementation.cpp:
    - PreinstallManagerImplementation::installPackages
- PreinstallManager/PreinstallManagerImplementation.h:
    - PreinstallManagerImplementation
- PreinstallManager/PreinstallManager.cpp:
    - PreinstallManager::Initialize
- PreinstallManager/PreinstallManager.h:
    - PreinstallManager

## Open Queries
- Does `PreinstallManagerImplementation` expose a `Scan`, `ListPreinstalled`, `Install`, or `GetStatus` method in the implementation, or are these stubs only in the plugin interface?
- What format are the packages in the preinstall directories — raw OCI bundles, tar archives, or a libpackage-sky format?
- Is the install thread joined or detached on plugin `Deinitialize` — what happens if shutdown occurs mid-install?

## References
- Architecture: [PreinstallManager.md](./PreinstallManager.md)
- Related Spec: [PackageManager.spec.md](./PackageManager.spec.md)

## Change History
- [2026-06-06] - openspec-templater - Restructured to match spec template.
