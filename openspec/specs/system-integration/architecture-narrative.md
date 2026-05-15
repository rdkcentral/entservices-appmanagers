# RDK System Architecture - Narrative & Flow Diagrams

## Overview

This document provides high-level architecture narrative and flow diagrams for the integrated RDK application-management system.

## Description

It describes system layers, cross-component interactions, and operational flow visuals used for integration understanding.

## Requirements

### Requirement: Document role remains informational
This document SHALL be treated as supporting documentation for the specification set and SHALL NOT supersede normative requirements defined in companion `spec.md` artifacts.

#### Scenario: Reader uses supporting artifact
- **WHEN** this document is referenced during design, planning, or validation review
- **THEN** it is interpreted as contextual/supporting guidance aligned with the companion specification.

- _Not applicable — normative requirements are captured in component and integration specs; this file is architecture narrative support._

## Executive Summary

The RDK application management ecosystem is a layered architecture spanning display composition, service interfaces, lifecycle management, and package handling. This document provides narrative descriptions and visual flow diagrams for the key system scenarios.

## System Layers

### Layer 1: Application Interface (Top)
- **Applications** - Running instances of apps
- **JSON-RPC Protocol** - WebSocket-based method calls and events
- **PluginHost** - RPC routing, authentication, resource limits

### Layer 2: Service Implementations
- **AppManager** - App lifecycle control
- **PackageManager** - Installation and updates
- **RuntimeManager** - Execution environment
- **RDKWindowManager** - Display composition
- **And 56+ other services**

### Layer 3: Libraries & Components
- **LibPackage-Sky** - Package parsing and verification
- **LifecycleManager** - State machine for apps
- **Download Manager** - Async package downloads
- **Telemetry** - Event and metric reporting

### Layer 4: System (Bottom)
- **Linux Kernel** - Processes, filesystem, namespaces
- **Westeros Compositor** - GPU rendering
- **Hardware** - Display, input devices, storage

## Scenario 1: Fresh Installation and First Launch

```
User Action: Install Netflix from app store

                    ┌─────────────────┐
                    │ App Store       │
                    │ (Firebolt App)  │
                    └────────┬────────┘
                             │
                             │ JSON-RPC
                             ▼
                    ┌─────────────────────────────────────┐
                    │ AppManager Service                  │
                    │ (WPEFramework Plugin)               │
                    └──────┬──────────────────────────────┘
                           │
            ┌──────────────┼──────────────┐
            │              │              │
            ▼              ▼              ▼
    ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
    │Package       │ │LibPackage-   │ │RDK Window    │
    │Manager       │ │Sky           │ │Manager       │
    │(Install)     │ │(Verify,      │ │(Surface)     │
    │              │ │Extract)      │ │              │
    └──────┬───────┘ └──────┬───────┘ └──────────────┘
           │                │
           │ Download       │ Parse &
           │ from URL       │ Verify
           │                │
    ┌──────▼────────────────▼─────┐
    │ HTTP Client / Archive Parser │
    │ (Download: 100MB ZIP)        │
    │ (Verify: Signature OK)       │
    │ (Extract: /opt/apps/netflix/)│
    └──────┬─────────────────────┘
           │
           │ Success
           │
    ┌──────▼──────────────────┐
    │ AppInfoManager Registry │
    │ (New AppInfo added)      │
    │ (Storage allocated)      │
    └──────┬─────────────────┘
           │
           │ OnAppInstalled
           │ Event
           ▼
    ┌──────────────────┐
    │ App Store UI     │
    │ (Install button  │
    │  → "Launch")     │
    └──────────────────┘


Timeline:
  0s   - User clicks "Install"
  1s   - Download starts
  30s  - Download complete (100MB)
  32s  - Verification complete
  34s  - Extraction complete
  35s  - Registration complete, UI updates
  36s+ - User can click "Launch"
```

### Scenario 1b: Application Launch

```
User Action: Click "Launch" on Netflix

        ┌──────────────┐
        │ App Store UI │
        │ [Launch Btn] │
        └────┬─────────┘
             │ JSON-RPC: AppManager.Launch("com.netflix.app")
             ▼
    ┌─────────────────────────────────┐
    │ AppManager.Launch()             │
    │ 1. Lookup AppInfo (cached)      │
    │ 2. LifecycleManager.Transition  │
    │    Idle → Loading → Running     │
    └────┬────────────────────────────┘
         │
         ├─ onEntering(Loading)
         │  └─ Create ApplicationContext
         │
         ├─ RuntimeManager.Setup()
         │  ├─ Create cgroup
         │  ├─ Setup namespace
         │  ├─ Mount directories
         │  └─ Ready for exec
         │
         ├─ Fork/Exec process
         │  └─ PID=1234, UID=10001, GID=10001
         │
         ├─ RDKWindowManager.CreateDisplay()
         │  ├─ Create Westeros surface
         │  ├─ Allocate frame buffer (1920x1080)
         │  ├─ Setup input routing
         │  └─ Display handle = 0x12345
         │
         ├─ onEntering(Running)
         │  └─ Update ApplicationContext.state = Running
         │
         └─ Emit OnAppLaunched event
            ├─ appId = "com.netflix.app"
            ├─ instanceId = "instance_1"
            └─ Send via WebSocket to clients

Netflix Process (parallel):
  ├─ Initialize libraries
  ├─ Create EGL context
  ├─ Bind Wayland surface
  └─ Render frame 1
     └─ Present to shared buffer

RDKWindowManager (next frame):
  ├─ Detect new surface
  ├─ Composite Netflix surface
  ├─ Apply z-order (topmost)
  └─ Display on HDMI

Timeline:
  0s   - User clicks Launch
  0.1s - AppManager.Launch() invoked
  0.2s - RuntimeManager setup complete
  0.3s - Netflix process forked
  0.4s - Display created
  0.8s - Netflix renders first frame
  1.0s - Netflix visible on TV
  2.0s - Netflix fully loaded (< 2s goal)
```

## Scenario 2: Interrupted Installation with Recovery

```
User Action: Start install, network fails mid-way

    ┌─────────────┐
    │ Install     │
    │ Netflix 7.2 │
    └────┬────────┘
         │ Download 100MB ZIP
         ▼
    ┌──────────────────────┐
    │ DownloadManager      │
    │ Progress: 0/100MB    │
    │ Progress: 25/100MB   │
    │ Progress: 50/100MB   │
    │ Progress: 75/100MB   │
    │ ERROR: Network lost! │
    └────┬─────────────────┘
         │
         │ Return: ERROR_NETWORK_TIMEOUT
         │
    ┌────▼──────────────────────────┐
    │ PackageManager Error Handler   │
    │ ├─ Delete temp file            │
    │ ├─ Cleanup partial state       │
    │ └─ Registry unchanged          │
    └────┬──────────────────────────┘
         │
         │ Return to user: "Download failed, retry?"
         │
         ▼
    ┌──────────────────────┐
    │ User clicks "Retry"  │
    └────┬─────────────────┘
         │ Resume download (HTTP Range request)
         │ (Some clients support this)
         ▼
    ┌──────────────────────────────┐
    │ DownloadManager Resume        │
    │ Byte range: 75000000-100000000│
    │ Progress: 75/100MB            │
    │ Progress: 100/100MB ✓         │
    └────┬─────────────────────────┘
         │
         │ Download complete
         │
    ┌────▼──────────────────────────┐
    │ Verify signature              │
    │ ├─ Extract manifest.xml       │
    │ ├─ Verify XM-DSig             │
    │ ├─ Check dependencies         │
    │ └─ All OK ✓                   │
    └────┬──────────────────────────┘
         │
    ┌────▼──────────────────────────┐
    │ Extract to /opt/apps/netflix/ │
    │ ├─ File 1/500                 │
    │ ├─ File 250/500               │
    │ ├─ File 500/500 ✓             │
    │ └─ Extraction complete        │
    └────┬──────────────────────────┘
         │
    ┌────▼──────────────────────────┐
    │ Register in AppRegistry       │
    │ ├─ Create AppInfo             │
    │ ├─ Allocate storage           │
    │ ├─ Save registry              │
    │ └─ Success ✓                  │
    └────┬──────────────────────────┘
         │
         │ OnAppInstalled event
         │
         ▼
    Installation complete (retry succeeded)
```

## Scenario 3: Multi-App Composition with Input Routing

```
Display State:

        ┌─────────────────────────────────────┐
        │           1920 x 1080               │
        │                                     │
        │  ┌─────────────────────────────┐   │
        │  │  YouTube (z=2, full screen) │   │
        │  │  (1920x1080)                │   │
        │  │                             │   │
        │  │  ┌───────────────────────┐  │   │
        │  │  │ System UI   (z=10)    │  │   │
        │  │  │ (300x100, opacity=0.8)│  │   │
        │  │  └───────────────────────┘  │   │
        │  │                             │   │
        │  └─────────────────────────────┘   │
        │                                     │
        │  Netflix (z=1, suspended,           │
        │   not rendered)                     │
        │                                     │
        └─────────────────────────────────────┘

Input Event Flow:

    User presses RED button (remote)
         │
         ▼
    ┌─────────────────────────┐
    │ LinuxInput               │
    │ /dev/input/event0        │
    │ EV_KEY [KEY_RED] PRESS   │
    └────┬────────────────────┘
         │
         ▼
    ┌─────────────────────────────────────┐
    │ RdkCompositor.onKeyPress()          │
    │ keycode=406 (mapped RED)            │
    │ metadata=0x00000000                 │
    │ timestamp=1234567890000000          │
    └────┬────────────────────────────────┘
         │
         ├─ Check system shortcuts
         │  └─ RED = "Show menu" → System UI
         │     └─ Consume event
         │
         └─ Route to System UI listener (z=10, topmost)
            ├─ InputEvent event received
            ├─ System processes key
            ├─ Opens app switcher menu
            └─ Renders response (next frame)

Alternative (no system shortcut):
    onKeyPress() → Route to focused app (YouTube, z=2)
         ├─ InputEvent forwarded to Netflix listener
         ├─ Netflix processes key
         ├─ Netflix renders response
         └─ RDKWindowManager composites new frame

Rendering (next frame):
    ┌──────────────────────────────────┐
    │ RDKWindowManager.draw()          │
    │                                   │
    │ 1. Clear frame buffer             │
    │                                   │
    │ 2. Composite YouTube (z=1, skipped if z<2)
    │    └─ Not rendered (suspended)   │
    │                                   │
    │ 3. Composite YouTube (z=2)       │
    │    ├─ Fetch shared buffer        │
    │    ├─ Apply transforms           │
    │    │  └─ Position: (0,0)         │
    │    │  └─ Size: (1920,1080)       │
    │    │  └─ Opacity: 1.0            │
    │    ├─ Render to output           │
    │    └─ Mark region as opaque      │
    │                                   │
    │ 4. Composite System UI (z=10)   │
    │    ├─ Fetch shared buffer        │
    │    ├─ Apply transforms           │
    │    │  └─ Position: (200, 200)    │
    │    │  └─ Size: (300, 100)        │
    │    │  └─ Opacity: 0.8            │
    │    │  └─ Crop: (0,0, 300,100)    │
    │    └─ Render to output           │
    │                                   │
    │ 5. Calculate hole-punch regions  │
    │    └─ Opaque: (0,0, 1920,1080)   │
    │    └─ Pass to video decoder      │
    │                                   │
    │ 6. Present frame (vsync)         │
    │                                   │
    │ 7. Send frame via VNC (if active)│
    │                                   │
    └──────────────────────────────────┘
```

## Scenario 4: App Crash Detection and Recovery

```
Netflix Process:

    ├─ Running (normal)
    │  ├─ Render frame 1
    │  ├─ Render frame 2
    │  └─ Render frame 3
    │
    ├─ ERROR: Segmentation fault (SIGSEGV)
    │  └─ Process terminates
    │
    └─ Process no longer exists

LifecycleManager Detection:

    ┌────────────────────────────────────┐
    │ LifecycleManager                   │
    │ (Monitoring process with waitpid) │
    │                                    │
    │ waitpid(1234) → SIGSEGV status    │
    │ Signal=11 (SIGSEGV)               │
    └────┬───────────────────────────────┘
         │
         ├─ Call StateHandler::onExiting(Running)
         │  └─ Cleanup process resources
         │
         ├─ Transition to Error state
         │  └─ Update ApplicationContext.state = Error
         │
         └─ Emit OnAppCrashed event
            ├─ appId = "com.netflix.app"
            ├─ signal = SIGSEGV (11)
            ├─ backtrace = (from core dump)
            └─ Send to TelemetryMetrics

RDKWindowManager Response:

    ┌─────────────────────────────┐
    │ RDKWindowManager            │
    │ Receives app termination    │
    │                             │
    │ ├─ Close display for Netflix│
    │ ├─ Deallocate surface       │
    │ ├─ Reclaim frame buffer     │
    │ └─ Update compositor        │
    │    (Netflix surface removed)│
    └─────────────────────────────┘

Display Changes:

    Before: YouTube(z=2) + Netflix(z=3, z-order changed after crash)
    After:  YouTube(z=2) only → fullscreen

User Notification:

    ┌─────────────────────────────────┐
    │ Telemetry/Analytics             │
    │ ├─ Crash recorded               │
    │ ├─ Error code stored            │
    │ └─ Sent to backend for analysis │
    │                                  │
    │ App Store UI (listening)         │
    │ ├─ Receives OnAppCrashed event  │
    │ ├─ Shows "Netflix crashed"      │
    │ └─ Offers "Relaunch" button     │
    └─────────────────────────────────┘
```

## System Metrics & Health

```
Real-time Monitoring:

    ┌────────────────────────────────────┐
    │ System Health Dashboard            │
    ├────────────────────────────────────┤
    │                                     │
    │ Frame Rate:           59.8 FPS ✓   │
    │ Input Latency:        38ms ✓       │
    │ Memory Usage:         420MB ✓      │
    │ Storage Free:         2.3GB ✓      │
    │                                     │
    │ Running Apps:         3             │
    │ ├─ YouTube (45min, 125MB, Z=2)    │
    │ ├─ Netflix (1h 20min, 98MB, suspended)
    │ └─ Prime (30min, 110MB, Z=1)      │
    │                                     │
    │ Cached Packages:      8             │
    │ ├─ Netflix (extracted, 500MB)     │
    │ ├─ YouTube (extracted, 480MB)     │
    │ └─ ... (6 more)                    │
    │                                     │
    │ Network:              150Mbps ↓    │
    │ Thermals:             42°C ✓       │
    │                                     │
    └────────────────────────────────────┘

Telemetry Events (last 1 minute):

    ├─ OnAppLaunched: YouTube (5s ago)
    ├─ OnAppSuspended: Netflix (30s ago)
    ├─ OnAppInstalled: Prime (2min 15s ago)
    ├─ OnInputEvent: 143 events (keyboard, pointer)
    └─ OnFrameRendered: 3600 frames @ 60Hz
```

## Component-to-Code Traceability

This table maps every HLA component to its primary implementation files. All paths are relative to the repository root.

### Layer 2: Service Implementations

| HLA Component | Primary Header | Primary Implementation | Spec |
|---|---|---|---|
| AppManager | `entservices-appmanagers/AppManager/AppManager.h` | `entservices-appmanagers/AppManager/AppManagerImplementation.cpp` | `openspec/specs/entservices-appmanagers/spec.md` |
| PackageManager | `entservices-appmanagers/PackageManager/PackageManager.h` | `entservices-appmanagers/PackageManager/PackageManagerImplementation.cpp` | `openspec/specs/entservices-appmanagers/spec.md` |
| LifecycleManager | `entservices-appmanagers/LifecycleManager/LifecycleManager.h` | `entservices-appmanagers/LifecycleManager/LifecycleManagerImplementation.cpp` | `openspec/specs/entservices-appmanagers/spec.md` |
| PreinstallManager | `entservices-appmanagers/PreinstallManager/PreinstallManager.h` | `entservices-appmanagers/PreinstallManager/PreinstallManagerImplementation.cpp` | `openspec/specs/entservices-appmanagers/spec.md` |
| AppStorageManager | `entservices-appmanagers/AppStorageManager/AppStorageManager.h` | `entservices-appmanagers/AppStorageManager/AppStorageManagerImplementation.cpp` | `openspec/specs/entservices-appmanagers/spec.md` |
| DownloadManager | `entservices-appmanagers/DownloadManager/DownloadManager.h` | `entservices-appmanagers/DownloadManager/DownloadManagerImplementation.cpp` | `openspec/specs/entservices-appmanagers/spec.md` |
| RuntimeManager | `entservices-appmanagers/RuntimeManager/RuntimeManager.h` | `entservices-appmanagers/RuntimeManager/RuntimeManagerImplementation.cpp` | `openspec/specs/entservices-appmanagers/spec.md` |
| TelemetryMetrics | `entservices-appmanagers/TelemetryMetrics/TelemetryMetrics.h` | `entservices-appmanagers/TelemetryMetrics/TelemetryMetricsImplementation.cpp` | `openspec/specs/entservices-appmanagers/spec.md` |
| RDKWindowManager | `rdk-window-manager/rdkwindowmanager.h` | `rdk-window-manager/rdkwindowmanager.cpp` | `openspec/specs/rdk-window-manager/spec.md` |

### Layer 3: Libraries & Components

| HLA Component | Primary Header | Primary Implementation | Spec |
|---|---|---|---|
| LibPackage-Sky | `libpackage-sky/AppId.h` | `libpackage-sky/AppId.cpp` | `openspec/specs/libpackage-sky/spec.md` |
| Service Interface Definitions | `entservices-apis/apis/AppManager/IAppManager.h` | _(interfaces only — no implementation)_ | `openspec/specs/entservices-apis/spec.md` |
| Westeros Compositor (external) | `rdk-window-manager/rdkcompositor.h` | `rdk-window-manager/rdkcompositor.cpp` | `openspec/specs/rdk-window-manager/spec.md` |

### Layer 1: JSON-RPC Protocol Stubs (Generated)

| Generated Artifact | Source Interface | Generator Tool |
|---|---|---|
| `entservices-apis/apis/AppManager/JAppManager.h` | `IAppManager.h` | `entservices-apis/tools/` |
| `entservices-apis/apis/PackageManager/JPackageManager.h` | `IPackageManager.h` | `entservices-apis/tools/` |
| `entservices-apis/apis/LifecycleManager/JLifecycleManager.h` | `ILifecycleManager.h` | `entservices-apis/tools/` |
| `entservices-apis/apis/RuntimeManager/JRuntimeManager.h` | `IRuntimeManager.h` | `entservices-apis/tools/` |

---

## Summary

The RDK ecosystem provides:
- **Robust Installation** - Atomic operations with rollback capability
- **Reliable Lifecycle** - State machine ensures consistent app behavior
- **Fast Rendering** - 60 FPS composition with optimized rendering paths
- **Responsive Input** - <50ms latency from device to app
- **Observable System** - Complete telemetry and metrics
- **Secure Execution** - Per-app isolation with capability-based access control

---

## Change History

- **May 2, 2026** - Initial narrative and flow diagrams created
