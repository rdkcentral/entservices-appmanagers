# RDK System Integration - Specification

## Overview

This specification defines the integration points, data flows, and interactions between the four major RDK components:
- **RDK Window Manager** - Display composition and UI management
- **EntServices-APIs** - Service interface contracts
- **EntServices-AppManagers** - Service implementations
- **LibPackage-Sky** - Package management library

## Description

This integration document explains inter-component data flows, orchestration boundaries, and end-to-end operational behavior across the RDK stack.

## Requirements

### Requirement: Document role remains informational
This document SHALL be treated as supporting documentation for the specification set and SHALL NOT supersede normative requirements defined in companion `spec.md` artifacts.

#### Scenario: Reader uses supporting artifact
- **WHEN** this document is referenced during design, planning, or validation review
- **THEN** it is interpreted as contextual/supporting guidance aligned with the companion specification.

- _Not applicable — component-level normative requirements are defined in each component `spec.md`; this file describes integration behavior._

## System Data Flows

### 1. Application Installation Flow

```
User/CLI: Install App Request
  │
  ▼
AppManager (EntServices-AppManagers)
  ├─ Create PackageManager task
  ├─ Call PackageManager::Install(url, appId, version)
  │  ├─ DownloadManager::Download(url)
  │  │  └─ Uses HTTPClient (LibPackage-Sky)
  │  │     └─ Downloads .zip/.tar to temp location
  │  │
  │  ├─ Verify: LibPackage-Sky::VerifyPackage()
  │  │  ├─ Parse manifest via PackageMetadata
  │  │  ├─ Check XML-DSig via XmlDSigVerifier
  │  │  └─ Validate checksums
  │  │
  │  ├─ Extract: LibPackage-Sky::ExtractPackage()
  │  │  ├─ Unzip/untar to /opt/apps/appId/
  │  │  ├─ Set ownership (userId, groupId)
  │  │  └─ Set permissions (755, 644)
  │  │
  │  └─ Register: AppInfoManager::Register()
  │     ├─ Create AppInfo
  │     ├─ Create storage directory (/data/app_storage/appId/)
  │     ├─ Save to registry
  │     └─ Emit OnAppInstalled notification
  │
  └─ Return: Success or error code

Timeline: 30-300 seconds (depends on size)
```

### 2. Application Launch Flow

```
User/System: Launch App (appId)
  │
  ▼
AppManager::Launch(appId, args)
  ├─ Look up AppInfo (in-memory registry)
  │  └─ Found: AppInfo with execPath, userId, groupId, capabilities
  │
  ├─ LifecycleManager::RequestStateChange(appId, Running)
  │  ├─ Validate transition (Idle → Loading → Running)
  │  ├─ Call StateHandler::onEntering(Loading)
  │  │  └─ Create ApplicationContext
  │  │
  │  ├─ Call RuntimeManager::Setup(appId)
  │  │  ├─ Create execution environment
  │  │  ├─ Mount app directories
  │  │  ├─ Setup cgroups/namespaces
  │  │  └─ Set LD_LIBRARY_PATH, etc.
  │  │
  │  ├─ Fork/exec application
  │  │  ├─ Run as userId/groupId
  │  │  └─ Inherit capabilities
  │  │
  │  ├─ Call RDKWindowManager::CreateDisplay(appId)
  │  │  ├─ Create Westeros surface
  │  │  ├─ Initialize compositor
  │  │  ├─ Setup input routing
  │  │  └─ Return display handle
  │  │
  │  ├─ App sends first frame
  │  ├─ RDKWindowManager receives and renders
  │  │
  │  └─ Call StateHandler::onEntering(Running)
  │     └─ Update ApplicationContext
  │
  └─ Emit OnAppLaunched(appId, instanceId)

Timeline: <2 seconds (typical)
States: Idle → Loading → Running
```

### 3. Surface Rendering Flow (Per Frame)

```
RDK Window Manager Main Loop (60 Hz):

update()
  ├─ Poll input devices
  ├─ Collect input events
  ├─ Check app state changes
  ├─ Process Firebolt command queue
  └─ Update surface dirty regions

draw()
  ├─ For each RdkCompositor:
  │  ├─ Fetch app-rendered surfaces from shared buffers
  │  │
  │  ├─ For each surface (sorted by z-order):
  │  │  ├─ Transform (crop, scale, position)
  │  │  ├─ Apply opacity
  │  │  ├─ Render to output
  │  │  └─ Track opaque regions (hole-punch)
  │  │
  │  ├─ Render overlays (UI, HUD, etc.)
  │  │
  │  ├─ Calculate hole-punch rectangles
  │  │  └─ Notify video decoder of transparent regions
  │  │
  │  └─ Present frame (vsync, buffer swap)
  │
  └─ Send frame via VNC (if enabled)

Timeline: 16.7ms (for 60 FPS)
Input: App-rendered buffers (via Wayland/EGL)
Output: Display output + video hole-punch rectangles
```

### 4. Input Event Routing Flow

```
Linux Input Device
  │ (/dev/input/event0, /dev/input/event1, etc.)
  ▼
RDKWindowManager::LinuxInput
  ├─ Poll device
  ├─ Parse event (type, code, value)
  ├─ Map to RDK keycode
  └─ Create InputEvent struct
  │
  ▼
RdkCompositor::onKeyPress/onKeyRelease/onPointerMotion
  ├─ Add to event queue
  ├─ Wake input listener thread
  └─ Process on next frame
  │
  ▼
InputEventListener callbacks (registered by app)
  ├─ App receives InputEvent
  ├─ App processes and responds
  └─ App renders response in next frame

Timeline: <50ms latency (device to app)
Priority: System shortcuts > active app > background apps
```

### 5. Application Lifecycle State Machine Integration

```
RequestStateChange(appId, targetState)
  │
  ├─ Check valid transition with StateTransitionHandler
  │  └─ Validate: current → target is allowed
  │
  ├─ Call StateHandler::onExiting(currentState)
  │  └─ Cleanup: close network connections, flush state, etc.
  │
  ├─ Call StateHandler::onEntering(targetState)
  │  ├─ For Loading:
  │  │  └─ Setup environment (RuntimeManager)
  │  │
  │  ├─ For Running:
  │  │  └─ Create display (RDKWindowManager)
  │  │
  │  ├─ For Suspended:
  │  │  ├─ Pause rendering
  │  │  └─ Clear input routing
  │  │
  │  └─ For Stopping:
  │     ├─ Close display (RDKWindowManager)
  │     ├─ Terminate process
  │     └─ Cleanup environment (RuntimeManager)
  │
  ├─ Update ApplicationContext
  │
  └─ Emit OnAppLifecycleStateChanged(appId, newState, oldState)

Transitions:
  Idle → Loading → Running ⇄ Suspended → Stopping → Idle
  
  Any state → Error (on failure)
  Any state → Stopping (force terminate)
```

## Cross-Component Dependencies

### RDKWindowManager ← EntServices-APIs
- Uses IAppManager interface to query app info
- Listens to OnAppLaunched/OnAppTerminated events
- Queries RDKWindowManager API from AppManager (via Firebolt)

### EntServices-AppManagers → EntServices-APIs
- Implements IAppManager, IPackageManager, etc.
- Inherits interface contracts
- Uses error codes from ErrorCodes enum

### EntServices-AppManagers → LibPackage-Sky
- Uses IPackage interface to load packages
- Uses IPackageManager for installation
- Uses IExtractedPackage for file access

### EntServices-AppManagers → RDKWindowManager
- Calls RDKWindowManager API on app launch/terminate
- Passes appId, capabilities to window manager
- Receives surface handles

### LibPackage-Sky (standalone)
- No dependencies on other RDK components
- Pure library, no RPC communication

## Service Layer Interactions

```
┌─────────────────────────────────────────────────────────┐
│                   Applications                           │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  ┌───────────────────────────────────────────────────┐  │
│  │ JSON-RPC Client Layer (via PluginHost)           │  │
│  ├───────────────────────────────────────────────────┤  │
│  │                                                    │  │
│  │  AppManager.launch      ← IAppManager interface   │  │
│  │  PackageManager.install ← IPackageManager iface   │  │
│  │  DeviceInfo.getInfo     ← IDeviceInfo interface   │  │
│  │  ... (60+ services)                               │  │
│  │                                                    │  │
│  └───────────────────────────────────────────────────┘  │
│          ▼ (RPC dispatch)                               │
│  ┌───────────────────────────────────────────────────┐  │
│  │ WPEFramework PluginHost                           │  │
│  │ (RPC routing, authentication, resource mgmt)      │  │
│  └───────────────────────────────────────────────────┘  │
│          ▼ (method invocation)                          │
│  ┌───────────────────────────────────────────────────┐  │
│  │ Plugin Implementations (AppManager, etc.)         │  │
│  ├───────────────────────────────────────────────────┤  │
│  │                                                    │  │
│  │  AppManager plugin                               │  │
│  │  ├─ Calls LifecycleManager                       │  │
│  │  ├─ Calls RDKWindowManager API                   │  │
│  │  └─ Calls LibPackage-Sky                         │  │
│  │                                                    │  │
│  │  PackageManager plugin                            │  │
│  │  └─ Calls LibPackage-Sky functions                │  │
│  │                                                    │  │
│  │  ... (other plugins)                              │  │
│  │                                                    │  │
│  └───────────────────────────────────────────────────┘  │
│          ▼ (native API calls)                           │
│  ┌───────────────────────────────────────────────────┐  │
│  │ Core Systems                                       │  │
│  ├───────────────────────────────────────────────────┤  │
│  │                                                    │  │
│  │  RDK Window Manager     (display, input, VNC)    │  │
│  │  Runtime Manager        (environment, cgroups)    │  │
│  │  LibPackage-Sky         (packages, verification)  │  │
│  │  Linux Kernel           (processes, filesystem)   │  │
│  │                                                    │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

## Error Code Propagation

```
Error Generation (inside component):
  └─ Return error code (enum Core::hresult)

Error Propagation (through layers):
  RDK Component
    ├─ Detects error condition
    ├─ Returns specific error code
    └─ Logs detailed info for debugging

RPC Marshaling (JSON-RPC):
  ├─ Convert error code to JSON response
  │  { "error": {"code": -32601, "message": "..."},
  │    "result": null }
  └─ Send to client

Client Handling:
  ├─ Receive JSON error
  ├─ Convert to local error enum
  ├─ Log and report to user
  └─ Retry or fail operation
```

## Event Notification Flow

```
Event Generation (in service):
  └─ OnAppInstalled(appId, version)

Internal Dispatch (PluginHost):
  ├─ Collect all registered listeners
  ├─ For each listener:
  │  ├─ Construct event JSON
  │  └─ Send via WebSocket
  └─ Async delivery (non-blocking)

Listener Reception (application):
  ├─ Receive WebSocket message
  ├─ Parse JSON event
  ├─ Call event handler
  └─ Process as needed

Example Event:
  {
    "id": "com.example.app:events",
    "method": "onAppInstalled",
    "params": {
      "appId": "com.netflix.app",
      "version": "7.2.1"
    }
  }
```

## Performance Integration Points

### Critical Path (App Launch)
```
User triggers launch
  ├─ AppManager lookup: <1ms (in-memory)
  ├─ LifecycleManager transition: <50ms (state change)
  ├─ RuntimeManager setup: <500ms (cgroups, mount)
  ├─ Fork/exec: <100ms
  ├─ Display creation: <100ms
  ├─ App renders first frame: 500-1000ms
  └─ Total: <2 seconds (typical)
```

### Frame Rendering Critical Path
```
Each frame (16.7ms budget @ 60 FPS):
  ├─ RDKWindowManager::update(): <5ms
  │  └─ Poll input, check state
  ├─ RDKWindowManager::draw(): <10ms
  │  ├─ Composite surfaces
  │  ├─ Apply transforms
  │  └─ Present frame
  └─ Overhead: <1.7ms
```

## Configuration & Defaults

### Installation Size Limits
- Max app size: 2GB
- Max concurrent installations: 5
- Installation timeout: 10 minutes

### Memory Limits (per app)
- Default memory limit: 256MB
- Memory-intensive capability raises limit to 512MB
- Soft limit with warning; hard limit kills app

### Storage Quotas
- Default app storage: 100MB
- Pre-installed apps: unlimited
- System reserved: 500MB

## Testing & Validation

### Integration Tests Required
1. **End-to-end Install & Launch**
   - Install app → launch → render → terminate → uninstall
   - Verify all notifications emitted
   - Verify storage cleanup

2. **Concurrent Operations**
   - Install app A while launching app B
   - Verify no resource contention
   - Verify state consistency

3. **Error Recovery**
   - Interrupt installation mid-process
   - Verify rollback works correctly
   - Verify app registry consistent

4. **Performance Under Load**
   - 50 concurrent apps running
   - 10 simultaneous downloads
   - Verify frame rate ≥55 FPS
   - Verify input latency <50ms

---

## Change History

- **May 2, 2026** - Initial specification created
