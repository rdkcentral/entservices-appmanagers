# RDKWindowManager Module

> Display & Window Management for RDK Platform

[← Back to Main](./README.md) | [← Previous: PreinstallManager](./PreinstallManager.md)

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
        -map~string,DisplayInfo~ mDisplays
        -list~ClientInfo~ mClients
        -InactivityMonitor mInactivityMonitor
        +CreateDisplay(displayName, config) hresult
        +DestroyDisplay(displayName) hresult
        +SetFocus(displayName) hresult
        +AddKeyIntercept(displayName, keyCode) hresult
        +RemoveKeyIntercept(displayName, keyCode) hresult
        +GetInactivityTime() uint32_t
    }

    class DisplayInfo {
        +string displayName
        +string waylandDisplay
        +bool visible
        +list~uint32_t~ keyIntercepts
    }

    class ClientInfo {
        +string clientId
        +string displayName
        +uint32_t pid
    }

    class IRDKWindowManager {
        <<interface>>
        +CreateDisplay() hresult
        +DestroyDisplay() hresult
        +SetFocus() hresult
        +AddKeyIntercept() hresult
        +GetInactivityTime() uint32_t
    }

    RDKWindowManagerImplementation ..|> IRDKWindowManager
    RDKWindowManagerImplementation --> DisplayInfo
    RDKWindowManagerImplementation --> ClientInfo
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
| `CreateDisplay(displayName, config)` | Create a new Wayland display for an application |
| `DestroyDisplay(displayName)` | Destroy a Wayland display when app terminates |
| `SetFocus(displayName)` | Give focus to a specific display/application |
| `AddKeyIntercept(displayName, keyCode)` | Add key code to intercept list for app |
| `RemoveKeyIntercept(displayName, keyCode)` | Remove key code from intercept list |
| `GetInactivityTime()` | Get time since last user input |

---

## Display Lifecycle

```mermaid
sequenceDiagram
    participant RTM as RuntimeManager
    participant WM as RDKWindowManager
    participant Compositor

    Note over RTM,Compositor: App Launch
    RTM->>WM: CreateDisplay("app-display", config)
    WM->>Compositor: Create Wayland Display
    Compositor-->>WM: display socket
    WM-->>RTM: "wayland-app-display"

    Note over RTM,Compositor: App Sets Focus
    RTM->>WM: SetFocus("app-display")
    WM->>Compositor: BringToFront()

    Note over RTM,Compositor: App Terminates
    RTM->>WM: DestroyDisplay("app-display")
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

[← Back to Main](./README.md) | [Next: TelemetryMetrics →](./TelemetryMetrics.md)

