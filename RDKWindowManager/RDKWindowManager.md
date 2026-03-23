# RDKWindowManager Module

> Display & Window Management for RDK Platform

[← Back to Main](../README.md) | [← Previous: PreinstallManager](../PreinstallManager/PreinstallManager.md)


---

## Purpose & Role

The **RDKWindowManager** manages Wayland display creation, client interactions, key intercepts, focus control, and user inactivity detection for applications running on RDK platform.

### Core Responsibilities

- **Display Management:** Create/destroy Wayland displays for apps
- **Client Management:** Track connected Wayland clients
- **Key Intercepts:** Configure key interception for apps
- **Focus Control:** Manage window focus and visibility
- **Inactivity Detection:** Report user inactivity periods

### Dependencies

| Module | Purpose |
|--------|---------|
| Compositor (Essos/Westeros) | Wayland compositor backend |

---

## Architecture

```mermaid
graph TB
    subgraph "RDKWindowManager Module"
        WM[RDKWindowManager<br/>Plugin]
        WMI[RDKWindowManagerImplementation]
    end

    subgraph "Clients"
        RTM[RuntimeManager]
        LCM[LifecycleManager]
    end

    subgraph "System"
        Compositor[Wayland Compositor<br/>Essos/Westeros]
        Display[Physical Display]
    end

    RTM -->|CreateDisplay| WM
    LCM -->|Focus Control| WM
    WM --> WMI
    WMI --> Compositor
    Compositor --> Display
```

---

## Class Diagram

```mermaid
classDiagram
    class RDKWindowManagerImplementation {
        -CriticalSection mAdminLock
        -list~INotification*~ mRDKWindowManagerNotification
        -IShell* mService
        -shared_ptr~RdkWindowManagerEventListener~ mEventListener
        +CreateDisplay(displayParams) hresult
        +GetApps(appsIds) hresult
        +SetFocus(client) hresult
        +SetVisible(client, visible) hresult
        +GetVisibility(client, visible) hresult
        +AddKeyIntercept(intercept) hresult
        +AddKeyIntercepts(clientId, intercepts) hresult
        +RemoveKeyIntercept(clientId, keyCode, modifiers) hresult
        +AddKeyListener(keyListeners) hresult
        +RemoveKeyListener(keyListeners) hresult
        +InjectKey(keyCode, modifiers) hresult
        +SetInactivityInterval(interval) hresult
        +ResetInactivityTime() hresult
        +GetLastKeyInfo(keyCode, modifiers, timestampInSeconds) hresult
        +SetZOrder(appInstanceId, zOrder) hresult
        +GetZOrder(appInstanceId, zOrder) hresult
    }

    class IRDKWindowManager {
        <<interface>>
        +CreateDisplay(displayParams) hresult
        +GetApps(appsIds) hresult
        +SetFocus(client) hresult
        +SetVisible(client, visible) hresult
        +GetVisibility(client, visible) hresult
        +AddKeyIntercept(intercept) hresult
        +AddKeyIntercepts(clientId, intercepts) hresult
        +RemoveKeyIntercept(clientId, keyCode, modifiers) hresult
        +InjectKey(keyCode, modifiers) hresult
        +SetInactivityInterval(interval) hresult
        +ResetInactivityTime() hresult
        +GetLastKeyInfo(keyCode, modifiers, timestampInSeconds) hresult
        +SetZOrder(appInstanceId, zOrder) hresult
        +GetZOrder(appInstanceId, zOrder) hresult
    }

    RDKWindowManagerImplementation ..|> IRDKWindowManager
```

---

## File Organization

```
RDKWindowManager/
├── RDKWindowManager.cpp           Plugin wrapper
├── RDKWindowManager.h             Plugin class definition
├── RDKWindowManagerImplementation.cpp Core implementation
├── RDKWindowManagerImplementation.h   Implementation class
├── Module.cpp/h                   Module registration
├── CMakeLists.txt                 Build configuration
└── RDKWindowManager.config        Runtime configuration
```

---

## API Reference

### IRDKWindowManager Interface

| Method | Purpose |
|--------|---------|
| `CreateDisplay(displayParamsJson)` | Create a new Wayland display for an application (parameters encoded as JSON) |
| `DestroyDisplay(displayParamsJson)` | Destroy a Wayland display when app terminates (parameters encoded as JSON) |
| `SetFocus(focusParamsJson)` | Give focus to a specific display/application (parameters encoded as JSON) |
| `AddKeyIntercept(interceptParamsJson)` | Add key code to intercept list for an app (parameters encoded as JSON) |
| `RemoveKeyIntercept(interceptParamsJson)` | Remove key code from intercept list (parameters encoded as JSON) |
| `GetInactivityTime()` | Get time since last user input |

---

## Display Lifecycle

```mermaid
sequenceDiagram
    participant RTM as RuntimeManager
    participant WM as RDKWindowManager
    participant Compositor

    Note over RTM,Compositor: App Launch
    RTM->>WM: CreateDisplay(displayParamsJson)
    WM->>Compositor: Create Wayland Display
    Compositor-->>WM: display socket
    WM-->>RTM: "wayland-app-display"

    Note over RTM,Compositor: App Sets Focus
    RTM->>WM: SetFocus(focusParamsJson)
    WM->>Compositor: BringToFront()

    Note over RTM,Compositor: App Terminates
    RTM->>WM: DestroyDisplay(displayParamsJson)
    WM->>Compositor: Destroy Display
    WM-->>RTM: success
```

---

## Key Intercept

Allows applications to receive specific key events even when not focused:

| Key Code | Common Use |
|----------|------------|
| KEY_HOME | Return to launcher |
| KEY_BACK | Navigation back |
| KEY_MENU | Options menu |
| KEY_POWER | Power management |

---

## Inactivity Monitoring

Tracks time since last user input for:

- Screen saver activation
- Power saving mode
- Auto-suspend of background apps

---

## Client Connection Flow

```mermaid
flowchart TD
    A[Container Started] --> B[App Connects to Wayland]
    B --> C[WindowManager Receives Client]
    C --> D[Register Client Info]
    D --> E[App Renders to Display]
    E --> F[SetFocus Called]
    F --> G[Display Visible]
```

---

[← Back to Main](../README.md) | [Next: TelemetryMetrics →](../TelemetryMetrics/TelemetryMetrics.md)

