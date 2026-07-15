# EntServices-AppManagers - Design Document

## Overview

This design document describes architecture and design choices for EntServices-AppManagers.

## Description

It complements the component specification with architecture, interactions, and implementation-level design narrative.

## Requirements

### Requirement: Document role remains informational
This document SHALL be treated as supporting documentation for the specification set and SHALL NOT supersede normative requirements defined in companion `spec.md` artifacts.

#### Scenario: Reader uses supporting artifact
- **WHEN** this document is referenced during design, planning, or validation review
- **THEN** it is interpreted as contextual/supporting guidance aligned with the companion specification.

- _Not applicable — normative requirements are defined in `openspec/specs/entservices-appmanagers/spec.md`; this document captures design realization._

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│           EntServices-AppManagers Architecture                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐│
│  │              AppManager Plugin (Main Service)               ││
│  │                                                             ││
│  │  ├─ AppInfo: app metadata (name, version, icon, etc)      ││
│  │  ├─ AppInfoManager: registry of installed apps             ││
│  │  ├─ JSON-RPC dispatch: method handlers                     ││
│  │  ├─ Notification system: event delivery                    ││
│  │  └─ Integration: LifecycleManager, RDKWindowManager        ││
│  └────────────────────────────────────────────────────────────┘│
│                         ▼                                        │
│  ┌────────────────────────────────────────────────────────────┐│
│  │            LifecycleManager State Machine                    ││
│  │                                                             ││
│  │  States:                                                   ││
│  │  ├─ Idle (app not running)                                ││
│  │  ├─ Loading (initializing)                                ││
│  │  ├─ Running (active)                                      ││
│  │  ├─ Suspended (backgrounded)                              ││
│  │  ├─ Stopping (shutdown in progress)                       ││
│  │  ├─ Error (failed state)                                  ││
│  │                                                             ││
│  │  Handlers:                                                 ││
│  │  ├─ StateHandler: process state entry/exit                ││
│  │  ├─ StateTransitionHandler: validate transitions          ││
│  │  ├─ WindowManagerHandler: surface coordination            ││
│  │  ├─ RuntimeManagerHandler: environment setup              ││
│  │  └─ ApplicationContext: per-app state tracking            ││
│  └────────────────────────────────────────────────────────────┘│
│                         ▼                                        │
│  ┌────────────────────────────────────────────────────────────┐│
│  │           PackageManager Plugin (Installation)              ││
│  │                                                             ││
│  │  ├─ Install: download → verify → extract → register       ││
│  │  ├─ Uninstall: deregister → cleanup → remove              ││
│  │  ├─ Verify: signature, integrity, dependencies            ││
│  │  ├─ Download: async file transfer with progress           ││
│  │  ├─ Reset: clear app state                                ││
│  │  └─ Storage: query/manage app storage quotas              ││
│  └────────────────────────────────────────────────────────────┘│
│                         ▼                                        │
│  ┌────────────────────────────────────────────────────────────┐│
│  │         PreinstallManager (Pre-installed Apps)              ││
│  │                                                             ││
│  │  ├─ Enumerate: scan pre-installed package directory        ││
│  │  ├─ Register: add to app registry                          ││
│  │  ├─ Update: handle version updates                         ││
│  │  └─ Cache: speed up discovery                              ││
│  └────────────────────────────────────────────────────────────┘│
│                         ▼                                        │
│  ┌────────────────────────────────────────────────────────────┐│
│  │        AppStorageManager (Persistent Storage)               ││
│  │                                                             ││
│  │  ├─ Allocate: reserve storage quota                        ││
│  │  ├─ Access: read/write persistent data                     ││
│  │  ├─ Query: get usage statistics                            ││
│  │  ├─ Reset: clear app data                                  ││
│  │  └─ Secure: encrypt sensitive data                         ││
│  └────────────────────────────────────────────────────────────┘│
│                         ▼                                        │
│  ┌────────────────────────────────────────────────────────────┐│
│  │        DownloadManager (Async Downloads)                    ││
│  │                                                             ││
│  │  ├─ Download: async file retrieval                         ││
│  │  ├─ Progress: track bytes transferred                      ││
│  │  ├─ Resume: partial resume support                         ││
│  │  ├─ Cache: HTTP caching                                    ││
│  │  └─ Bandwidth: rate limiting                               ││
│  └────────────────────────────────────────────────────────────┘│
│                         ▼                                        │
│  ┌────────────────────────────────────────────────────────────┐│
│  │      TelemetryMetrics (Event Reporting)                     ││
│  │                                                             ││
│  │  ├─ AppInstalled: package installed successfully           ││
│  │  ├─ AppUninstalled: app removed                            ││
│  │  ├─ AppLaunched: launch event                              ││
│  │  ├─ AppSuspended: background event                         ││
│  │  ├─ AppTerminated: shutdown event                          ││
│  │  ├─ AppCrashed: crash/error event                          ││
│  │  └─ AppMetrics: perf, memory, startup time                 ││
│  └────────────────────────────────────────────────────────────┘│
│                         ▼                                        │
│  ┌────────────────────────────────────────────────────────────┐│
│  │       External Service Integration                          ││
│  │                                                             ││
│  │  ├─ LibPackage-Sky: package parsing & verification         ││
│  │  ├─ RDKWindowManager: surface management                   ││
│  │  ├─ RuntimeManager: environment setup                      ││
│  │  └─ PersistentStore: long-term app data                    ││
│  └────────────────────────────────────────────────────────────┘│
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## Lifecycle State Machine

```
                    ┌─────────────┐
                    │    Idle     │◄─────┐
                    │ (not running)│     │
                    └──────┬──────┘     │
                           │            │
                           │ Launch     │ Error/Terminate
                           ▼            │
                    ┌──────────────┐    │
                    │   Loading    │    │
                    │(initializing)│────┤
                    └──────┬───────┘    │
                           │            │
                    (env ready)          │
                           ▼            │
                    ┌──────────────┐    │
                    │   Running    │    │
                    │  (active)    │────┤
                    └──────┬───────┘    │
                           │            │
                    (background)         │
                           ▼            │
                    ┌──────────────┐    │
                    │  Suspended   │    │
                    │(backgrounded)│────┤
                    └──────┬───────┘    │
                           │            │
                    (terminate req)      │
                           ▼            │
                    ┌──────────────┐    │
                    │  Stopping    │────┘
                    │ (shutdown)   │
                    └──────┬───────┘
                           │
                           ▼
                    (back to Idle)
```

## AppInfo & Registry

```
AppInfo Structure:
├─ appId: unique identifier (e.g., "com.netflix.app")
├─ name: display name
├─ version: version string
├─ icon: icon URL/path
├─ execPath: executable location
├─ runtime: runtime type (native, web, etc)
├─ userId: Unix UID
├─ groupId: Unix GID
├─ storageQuota: max persistent storage
├─ capabilities: feature set
└─ metadata: custom key-value pairs

AppInfoManager:
├─ Registry (in-memory + persistent)
│  └─ Map<appId, AppInfo>
├─ Load: from disk at startup
├─ Save: on changes
├─ Query: by appId, category, capability
└─ Update: when new app installed/removed
```

## Installation Process

```
Install(url, appId, version)
  ├─ Step 1: Download
  │  ├─ AsyncDownload(url)
  │  └─ Save to temporary location
  │
  ├─ Step 2: Verify
  │  ├─ Parse manifest
  │  ├─ Verify signature
  │  ├─ Check integrity
  │  ├─ Validate dependencies
  │  └─ Fail if any check fails
  │
  ├─ Step 3: Extract
  │  ├─ Unzip to /opt/apps/appId/
  │  ├─ Set permissions (userId, groupId)
  │  └─ Setup directories
  │
  ├─ Step 4: Register
  │  ├─ Parse metadata
  │  ├─ Create AppInfo
  │  ├─ Add to registry
  │  ├─ Save registry
  │  └─ Emit OnAppInstalled event
  │
  └─ Step 5: Cleanup
     ├─ Remove temporary files
     └─ Return success/failure
```

## Download Manager

```
AsyncDownload(url, appId)
  ├─ Create HTTP client
  ├─ Check cache (if EnableHttpCache)
  ├─ Set rate limit (if DisableRateLimit = false)
  ├─ Open output file
  ├─ Loop:
  │  ├─ Fetch chunk (512KB typical)
  │  ├─ Write to file
  │  ├─ Emit progress callback
  │  ├─ Check for cancellation
  │  └─ Retry on network error (3x)
  ├─ Close file
  └─ Return success/failure

Progress Tracking:
├─ bytesTotal: total bytes to download
├─ bytesDownloaded: current progress
├─ status: Downloading, Downloaded, Failed, Cancelled
└─ estimatedTimeRemaining: calculated from rate
```

## Storage Management

```
Persistent Storage:
├─ Path: /data/app_storage/appId/
├─ Max size: configurable per app
├─ Access: AppStorageManager APIs
├─ Quota tracking:
│  ├─ storageAllocated: reserved bytes
│  ├─ storageUsed: current usage
│  └─ storageFree: available bytes
└─ Operations:
   ├─ Read: file access
   ├─ Write: create/overwrite
   ├─ Delete: cleanup
   └─ Reset: clear all app data

Volatile Storage (Temp):
├─ Path: /tmp/app_cache/appId/
├─ Cleared on app terminate
└─ Used for temporary data

Secure Storage:
├─ Encrypted with device key
├─ Limited to sensitive credentials
└─ Access controlled per app
```

## Telemetry Integration

```
Telemetry Events:

OnAppInstalled(appId, version)
  └─ Reported: timestamp, appId, version, success flag

OnAppUninstalled(appId)
  └─ Reported: timestamp, appId

OnAppLaunched(appId, reason)
  └─ Reported: timestamp, appId, reason (user, system, api)

OnAppSuspended(appId)
  └─ Reported: timestamp, appId, foreground time

OnAppTerminated(appId, reason)
  └─ Reported: timestamp, appId, reason (user, crash, memory)

OnAppCrashed(appId, signal)
  └─ Reported: timestamp, appId, signal, backtrace

Metrics:
├─ Startup time (ms)
├─ Memory peak (MB)
├─ CPU time (ms)
└─ Disk I/O (bytes)
```

## Thread Safety

- Main plugin thread processes RPC requests
- Background threads for async operations (download, extraction)
- Mutex protection for app registry
- Thread-safe event delivery

## Configuration

```
AppManager.conf
├─ appDirectory: /opt/apps/
├─ storageRoot: /data/app_storage/
├─ preinstallPath: /opt/preinstall/
├─ downloadTimeout: 300s
├─ installTimeout: 600s
├─ defaultStorageQuota: 100MB
└─ enableTelemetry: true
```
