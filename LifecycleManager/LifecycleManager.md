# LifecycleManager Plugin Documentation

> State Machine for Application Lifecycle Transitions in RDK Infrastructure

## 1. High-Level Purpose & Architecture

### Role in ENT / RDK Infrastructure

The **LifecycleManager** plugin implements the core state machine that governs application lifecycle transitions. It acts as the central coordinator between the AppManager (which initiates requests) and the RuntimeManager (which executes container operations).

### Responsibilities

- **State Machine Management**: Implement and enforce valid lifecycle state transitions
- **Application Context Tracking**: Maintain context for each loaded application
- **Runtime Coordination**: Delegate container operations to RuntimeManager
- **Window Management**: Coordinate with RDKWindowManager for display operations
- **State Change Notifications**: Propagate lifecycle state changes to subscribers

### Interacting Subsystems

| Subsystem | Interaction Type | Purpose |
|-----------|-----------------|---------|
| AppManager | COM-RPC (inbound) | Receives SpawnApp, SetTargetAppState, UnloadApp, KillApp requests |
| RuntimeManager | COM-RPC (outbound) | Delegates Run, Suspend, Resume, Hibernate, Wake, Terminate |
| RDKWindowManager | COM-RPC (outbound) | Coordinates display creation and focus |

### What It Does NOT Do

- Does not directly manage OCI containers (delegated to RuntimeManager)
- Does not handle JSON-RPC client communication directly (handled by AppManager)
- Does not manage package installation (handled by PackageManager)

---

## 2. Architectural Overview

### Major Components

```mermaid
graph TB
    subgraph "LifecycleManager Plugin"
        Shell[LifecycleManager<br/>JSON-RPC Shell]
        Impl[LifecycleManagerImplementation<br/>State Machine]
        SH[StateHandler<br/>Transition Logic]
        STH[StateTransitionHandler<br/>Transition Executor]
        AC[ApplicationContext<br/>App State Container]
        States[State Classes<br/>State Behaviors]
    end

    subgraph "Handlers"
        RMH[RuntimeManagerHandler]
        WMH[WindowManagerHandler]
        RH[RequestHandler]
    end

    subgraph "External Plugins"
        RTM[RuntimeManager]
        WM[RDKWindowManager]
    end

    Shell --> Impl
    Impl --> SH
    SH --> STH
    SH --> States
    Impl --> AC
    Impl --> RMH
    Impl --> WMH
    Impl --> RH
    RMH --> RTM
    WMH --> WM
```

### State Machine States

```mermaid
stateDiagram-v2
    [*] --> UNLOADED
    UNLOADED --> LOADING: SpawnApp
    LOADING --> INITIALIZING: Container Started
    INITIALIZING --> PAUSED: AppReady (background)
    INITIALIZING --> ACTIVE: AppReady (foreground)
    PAUSED --> ACTIVE: SetTargetState(ACTIVE)
    ACTIVE --> PAUSED: SetTargetState(PAUSED)
    PAUSED --> SUSPENDED: SetTargetState(SUSPENDED)
    SUSPENDED --> PAUSED: SetTargetState(PAUSED)
    PAUSED --> HIBERNATED: SetTargetState(HIBERNATED)
    HIBERNATED --> PAUSED: SetTargetState(PAUSED)
    PAUSED --> TERMINATING: UnloadApp/SetTargetState(TERMINATING)
    ACTIVE --> TERMINATING: UnloadApp
    SUSPENDED --> TERMINATING: UnloadApp
    HIBERNATED --> TERMINATING: UnloadApp
    TERMINATING --> UNLOADED: Container Stopped
    UNLOADED --> [*]
```

---

## 3. Code Organization

### Directory Structure

```
LifecycleManager/
├── LifecycleManager.cpp              # Plugin shell
├── LifecycleManager.h                # Shell header
├── LifecycleManagerImplementation.cpp # Core state machine logic
├── LifecycleManagerImplementation.h   # Implementation header
├── ApplicationContext.cpp            # Application context container
├── ApplicationContext.h              # Context header
├── State.cpp                         # State base class and derived states
├── State.h                           # State class headers
├── StateHandler.cpp                  # State transition coordinator
├── StateHandler.h                    # StateHandler header
├── StateTransitionHandler.cpp        # Transition execution logic
├── StateTransitionHandler.h          # Transition header
├── StateTransitionRequest.h          # Transition request structure
├── RequestHandler.cpp                # Request processing
├── RequestHandler.h                  # RequestHandler header
├── RuntimeManagerHandler.cpp         # RuntimeManager bridge
├── RuntimeManagerHandler.h           # RuntimeManagerHandler header
├── WindowManagerHandler.cpp          # WindowManager bridge
├── WindowManagerHandler.h            # WindowManagerHandler header
├── IEventHandler.h                   # Event handler interface
├── LifecycleManagerTelemetryReporting.cpp # Telemetry
├── LifecycleManagerTelemetryReporting.h   # Telemetry header
├── Module.cpp                        # Plugin module
├── Module.h                          # Module header
├── CMakeLists.txt                    # Build configuration
├── LifecycleManager.config           # Plugin configuration
└── LifecycleManager.conf.in          # Configuration template
```

### File-by-File Breakdown

#### LifecycleManager.h / LifecycleManager.cpp

**Purpose**: JSON-RPC shell plugin exposing the LifecycleManager API.

**Key Elements**:
- Implements `PluginHost::IPlugin` and `PluginHost::JSONRPC`
- Contains `Notification` inner class for `ILifecycleManagerState::INotification`
- Aggregates both `ILifecycleManager` and `ILifecycleManagerState` interfaces

```cpp
// From LifecycleManager.h (lines 91-97)
BEGIN_INTERFACE_MAP(LifecycleManager)
INTERFACE_ENTRY(PluginHost::IPlugin)
INTERFACE_ENTRY(PluginHost::IDispatcher)
INTERFACE_AGGREGATE(Exchange::ILifecycleManager, mLifecycleManagerImplementation)
INTERFACE_AGGREGATE(Exchange::ILifecycleManagerState, mLifecycleManagerState)
END_INTERFACE_MAP
```

#### LifecycleManagerImplementation.h / LifecycleManagerImplementation.cpp

**Purpose**: Core state machine logic implementing `ILifecycleManager`, `ILifecycleManagerState`, and `IEventHandler`.

**Key Interfaces Implemented**:
- `Exchange::ILifecycleManager` - Main lifecycle control API
- `Exchange::ILifecycleManagerState` - State reporting and app-side callbacks
- `Exchange::IConfiguration` - Plugin configuration
- `IEventHandler` - Internal event handling

**Key Methods**:
```cpp
// ILifecycleManager methods
Core::hresult SpawnApp(const string& appId, const string& launchIntent, 
                       LifecycleState targetLifecycleState,
                       const RuntimeConfig& runtimeConfigObject,
                       const string& launchArgs, string& appInstanceId,
                       string& errorReason, bool& success);
Core::hresult SetTargetAppState(const string& appInstanceId, 
                                LifecycleState targetLifecycleState,
                                const string& launchIntent);
Core::hresult UnloadApp(const string& appInstanceId, string& errorReason, bool& success);
Core::hresult KillApp(const string& appInstanceId, string& errorReason, bool& success);
Core::hresult SendIntentToActiveApp(const string& appInstanceId, const string& intent,
                                     string& errorReason, bool& success);

// ILifecycleManagerState methods
Core::hresult AppReady(const string& appId);
Core::hresult StateChangeComplete(const string& appId, uint32_t stateChangedId, bool success);
Core::hresult CloseApp(const string& appId, AppCloseReason closeReason);
```

#### ApplicationContext.h / ApplicationContext.cpp

**Purpose**: Encapsulates all runtime context for a single application instance.

**Key Members**:
```cpp
class ApplicationContext : public std::enable_shared_from_this<ApplicationContext> {
private:
    std::string mAppInstanceId;      // Unique instance ID
    std::string mAppId;              // Application ID
    timespec mLastLifecycleStateChangeTime;
    std::string mActiveSessionId;
    LifecycleState mTargetLifecycleState;
    std::string mMostRecentIntent;
    void* mState;                    // Current State object
    uint32_t mStateChangeId;         // State change counter
    ApplicationLaunchParams mLaunchParams;
    ApplicationKillParams mKillParams;
    time_t mRequestTime;
    RequestType mRequestType;

public:
    sem_t mReachedLoadingStateSemaphore;
    sem_t mFirstFrameAfterResumeSemaphore;
    bool mPendingStateTransition;
    std::vector<LifecycleState> mPendingStates;
    LifecycleState mPendingOldState;
    std::string mPendingEventName;
};
```

#### State.h / State.cpp

**Purpose**: State pattern implementation with base class and derived state classes.

**State Classes**:
```cpp
class State {
public:
    State(ApplicationContext* context, LifecycleState state);
    virtual bool handle(string& errorReason);
    virtual LifecycleState getValue();
    virtual ApplicationContext* getContext();
protected:
    LifecycleState mState;
    ApplicationContext* mContext;
};

class UnloadedState : public State { /* ... */ };
class LoadingState : public State { /* ... */ };
class InitializingState : public State { /* ... */ };
class PausedState : public State { /* ... */ };
class ActiveState : public State { /* ... */ };
class SuspendedState : public State { /* ... */ };
class HibernatedState : public State { /* ... */ };
class TerminatingState : public State { /* ... */ };
```

#### StateHandler.h / StateHandler.cpp

**Purpose**: Coordinates state transitions, validates transition paths, and dispatches events.

**Key Methods**:
```cpp
class StateHandler {
public:
    static void initialize();
    static bool changeState(StateTransitionRequest& request, string& errorReason);

private:
    static State* createState(ApplicationContext* context, LifecycleState state);
    static bool isValidTransition(LifecycleState start, LifecycleState target,
                                  std::map<LifecycleState, bool>& pathSequence,
                                  std::vector<LifecycleState>& foundPath);
    static bool updateState(ApplicationContext* context, LifecycleState state, 
                           string& errorReason);
    static void sendEvent(ApplicationContext* context, LifecycleState oldState,
                         LifecycleState newState, string& errorReason);
    static bool getStatePath(ApplicationContext* context, LifecycleState target,
                            std::vector<LifecycleState>& statePath, string& errorReason);
    
    static std::map<LifecycleState, std::list<LifecycleState>> mPossibleStateTransitions;
};
```

---

## 4. Class & Interface Documentation

### Exchange::ILifecycleManager Interface

Primary interface for lifecycle control:

```cpp
interface ILifecycleManager {
    // Notification interface for state changes
    interface INotification {
        void OnAppStateChanged(const string& appId, LifecycleState state, 
                              const string& errorReason);
    };

    hresult Register(INotification* notification);
    hresult Unregister(INotification* notification);
    hresult GetLoadedApps(bool verbose, string& apps);
    hresult IsAppLoaded(const string& appId, bool& loaded);
    hresult SpawnApp(const string& appId, const string& launchIntent,
                     LifecycleState targetState, const RuntimeConfig& config,
                     const string& launchArgs, string& appInstanceId,
                     string& errorReason, bool& success);
    hresult SetTargetAppState(const string& appInstanceId, LifecycleState target,
                              const string& launchIntent);
    hresult UnloadApp(const string& appInstanceId, string& errorReason, bool& success);
    hresult KillApp(const string& appInstanceId, string& errorReason, bool& success);
    hresult SendIntentToActiveApp(const string& appInstanceId, const string& intent,
                                   string& errorReason, bool& success);
};
```

### Exchange::ILifecycleManagerState Interface

Interface for app-side state reporting:

```cpp
interface ILifecycleManagerState {
    interface INotification {
        void OnAppLifecycleStateChanged(const string& appId, const string& appInstanceId,
                                        LifecycleState oldState, LifecycleState newState,
                                        const string& navigationIntent);
    };

    hresult Register(INotification* notification);
    hresult Unregister(INotification* notification);
    hresult AppReady(const string& appId);
    hresult StateChangeComplete(const string& appId, uint32_t stateChangedId, bool success);
    hresult CloseApp(const string& appId, AppCloseReason closeReason);
};
```

### LifecycleState Enumeration

```cpp
enum LifecycleState {
    UNLOADED = 0,     // App not loaded
    LOADING,          // Container starting
    INITIALIZING,     // App initializing
    PAUSED,           // App in background
    ACTIVE,           // App in foreground
    SUSPENDED,        // App suspended (low memory)
    HIBERNATED,       // App state persisted to disk
    TERMINATING       // App shutting down
};
```

### Class Relationships

```mermaid
classDiagram
    class LifecycleManager {
        -ILifecycleManager* mLifecycleManagerImplementation
        -ILifecycleManagerState* mLifecycleManagerState
        +Initialize()
        +Deinitialize()
    }

    class LifecycleManagerImplementation {
        -list~ApplicationContext~ mLoadedApplications
        -map~string,PendingRespawnRequest~ mPendingRespawns
        -PluginHost::IShell* mService
        +SpawnApp()
        +SetTargetAppState()
        +UnloadApp()
        +KillApp()
    }

    class ApplicationContext {
        -string mAppInstanceId
        -string mAppId
        -LifecycleState mTargetLifecycleState
        -void* mState
        +getState()
        +setState()
        +getTargetLifecycleState()
    }

    class StateHandler {
        +changeState()$
        +isValidTransition()$
        -mPossibleStateTransitions$
    }

    class State {
        #LifecycleState mState
        #ApplicationContext* mContext
        +handle()
        +getValue()
    }

    class LoadingState {
        +handle()
    }

    class ActiveState {
        +handle()
    }

    LifecycleManager --> LifecycleManagerImplementation
    LifecycleManagerImplementation --> ApplicationContext
    LifecycleManagerImplementation --> StateHandler
    StateHandler --> State
    State <|-- LoadingState
    State <|-- ActiveState
    State <|-- PausedState
    State <|-- SuspendedState
    State <|-- HibernatedState
    State <|-- TerminatingState
    ApplicationContext --> State
```

---

## 5. Configuration & Build Integration

### Plugin Configuration (LifecycleManager.config)

```cmake
set (autostart false)
set (preconditions Platform)
set (callsign "org.rdk.LifecycleManager")

map()
   key(root)
   map()
       kv(mode ${PLUGIN_LIFECYCLE_MANAGER_MODE})
       kv(locator lib${MODULE_NAME}.so)
   end()
end()
ans(configuration)
```

### CMake Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `PLUGIN_LIFECYCLE_MANAGER_MODE` | Execution mode (Off/Local) | Off |
| `PLUGIN_LIFECYCLE_MANAGER_AUTOSTART` | Auto-start on boot | false |
| `ENABLE_UNIT_TESTS` | Enable unit test compilation | OFF |

### Source Files

```cmake
set(PLUGIN_LIFECYCLE_MANAGER_SOURCES
    LifecycleManager.cpp
    LifecycleManagerImplementation.cpp
    Module.cpp
    LifecycleManagerTelemetryReporting.cpp
    ApplicationContext.cpp
    RequestHandler.cpp
    RuntimeManagerHandler.cpp
    WindowManagerHandler.cpp
    State.cpp
    StateHandler.cpp
    StateTransitionHandler.cpp)
```

---

## 6. Internal Workflows & Execution Flow

### SpawnApp Flow

```mermaid
sequenceDiagram
    participant AM as AppManager
    participant LCM as LifecycleManagerImpl
    participant SH as StateHandler
    participant RMH as RuntimeManagerHandler
    participant RTM as RuntimeManager
    participant AC as ApplicationContext

    AM->>LCM: SpawnApp(appId, intent, targetState, config)
    LCM->>AC: new ApplicationContext(appId)
    LCM->>AC: setApplicationLaunchParams(...)
    LCM->>SH: changeState(LOADING)
    SH->>SH: isValidTransition(UNLOADED, LOADING)
    SH->>AC: setState(LoadingState)
    SH->>LCM: LoadingState.handle()
    LCM->>RMH: Run(appId, appInstanceId, config)
    RMH->>RTM: Run(...)
    RTM-->>RMH: success
    RMH-->>LCM: success
    LCM-->>AM: appInstanceId, success=true

    Note over RTM: Container starts...

    RTM->>LCM: onRuntimeManagerEvent(CONTAINERSTARTED)
    LCM->>SH: changeState(INITIALIZING)
    SH->>AC: setState(InitializingState)
    
    Note over LCM: App calls AppReady...

    LCM->>LCM: AppReady(appId)
    LCM->>SH: changeState(targetState)
    SH->>AC: setState(ActiveState or PausedState)
    LCM->>AM: OnAppLifecycleStateChanged
```

### SetTargetAppState Flow

```mermaid
sequenceDiagram
    participant AM as AppManager
    participant LCM as LifecycleManagerImpl
    participant SH as StateHandler
    participant AC as ApplicationContext
    participant RMH as RuntimeManagerHandler

    AM->>LCM: SetTargetAppState(appInstanceId, SUSPENDED)
    LCM->>LCM: getContext(appInstanceId)
    LCM->>AC: setTargetLifecycleState(SUSPENDED)
    LCM->>SH: getStatePath(current, SUSPENDED)
    SH-->>LCM: [PAUSED, SUSPENDED]
    
    loop For each state in path
        LCM->>SH: changeState(nextState)
        SH->>AC: setState(nextState)
        SH->>SH: State.handle()
        alt State is SUSPENDED
            SH->>RMH: Suspend(appInstanceId)
            RMH-->>SH: success
        end
        SH->>LCM: sendEvent(oldState, newState)
        LCM->>AM: OnAppLifecycleStateChanged
    end
    
    LCM-->>AM: Core::ERROR_NONE
```

### State Transition Validation

The `StateHandler` maintains a map of valid transitions:

```cpp
// Conceptual representation of mPossibleStateTransitions
{
    UNLOADED:     [LOADING],
    LOADING:      [INITIALIZING, TERMINATING],
    INITIALIZING: [PAUSED, ACTIVE, TERMINATING],
    PAUSED:       [ACTIVE, SUSPENDED, HIBERNATED, TERMINATING],
    ACTIVE:       [PAUSED, TERMINATING],
    SUSPENDED:    [PAUSED, TERMINATING],
    HIBERNATED:   [PAUSED, TERMINATING],
    TERMINATING:  [UNLOADED]
}
```

### Event Handling Flow

```mermaid
flowchart TD
    A[RuntimeManager Event] --> B{Event Type}
    B -->|CONTAINERSTARTED| C[handleRuntimeManagerEvent]
    B -->|CONTAINERSTOPPED| D[handleRuntimeManagerEvent]
    B -->|CONTAINERFAILED| E[notifyOnFailure]
    
    C --> F[changeState to INITIALIZING]
    D --> G[changeState to UNLOADED]
    E --> H[Notify error to listeners]
    
    I[WindowManager Event] --> J{Event Type}
    J -->|READY| K[handleWindowManagerEvent]
    J -->|DISCONNECTED| L[Handle disconnect]
    
    K --> M[Signal first frame ready]
```

---

## 7. Diagrams & Visual Aids

### Complete State Transition Diagram

```mermaid
stateDiagram-v2
    [*] --> UNLOADED
    
    UNLOADED --> LOADING: SpawnApp()
    
    LOADING --> INITIALIZING: Container Started
    LOADING --> TERMINATING: Container Failed
    
    INITIALIZING --> PAUSED: AppReady (target=PAUSED)
    INITIALIZING --> ACTIVE: AppReady (target=ACTIVE)
    INITIALIZING --> TERMINATING: Timeout/Error
    
    PAUSED --> ACTIVE: SetTargetState(ACTIVE)
    ACTIVE --> PAUSED: SetTargetState(PAUSED)
    
    PAUSED --> SUSPENDED: SetTargetState(SUSPENDED)
    SUSPENDED --> PAUSED: SetTargetState(PAUSED/ACTIVE)
    
    PAUSED --> HIBERNATED: SetTargetState(HIBERNATED)
    HIBERNATED --> PAUSED: SetTargetState(PAUSED/ACTIVE)
    
    PAUSED --> TERMINATING: UnloadApp/CloseApp
    ACTIVE --> TERMINATING: UnloadApp
    SUSPENDED --> TERMINATING: UnloadApp/KillApp
    HIBERNATED --> TERMINATING: UnloadApp/KillApp
    
    TERMINATING --> UNLOADED: Container Stopped
    
    UNLOADED --> [*]
```

### Handler Interaction Diagram

```mermaid
graph LR
    subgraph "LifecycleManagerImplementation"
        Impl[Implementation]
    end

    subgraph "Handlers"
        RH[RequestHandler]
        RMH[RuntimeManagerHandler]
        WMH[WindowManagerHandler]
    end

    subgraph "External"
        RTM[RuntimeManager]
        WM[RDKWindowManager]
    end

    Impl --> RH
    RH --> RMH
    RH --> WMH
    RMH --> RTM
    WMH --> WM

    RTM -.->|Events| Impl
    WM -.->|Events| Impl
```

---

## 8. Testing & Quality Analysis

### Existing Tests

Located in `Tests/L1Tests/tests/test_LifecycleManager.cpp`:

| Test Category | Description |
|---------------|-------------|
| SpawnApp Tests | Verify app spawning with various configurations |
| State Transition Tests | Verify valid state transitions |
| Invalid Transition Tests | Verify rejection of invalid transitions |
| UnloadApp Tests | Verify app unloading |
| KillApp Tests | Verify forced termination |
| Event Tests | Verify event delivery |

### Test Coverage Gaps

1. **Concurrent State Changes**: Multiple simultaneous SetTargetAppState calls
2. **Pending State Queue**: Verification of pending state handling
3. **Respawn Logic**: Testing of pending respawn mechanism
4. **Timeout Handling**: State transition timeout scenarios

### Suggested Test Cases

```cpp
// Test rapid state transitions
TEST(LifecycleManagerTest, RapidStateTransitions) {
    // ACTIVE -> PAUSED -> SUSPENDED -> PAUSED -> ACTIVE in quick succession
}

// Test pending respawn
TEST(LifecycleManagerTest, PendingRespawn) {
    // Spawn during termination should queue respawn
}

// Test state path calculation
TEST(LifecycleManagerTest, StatePathCalculation) {
    // ACTIVE to HIBERNATED should go through PAUSED
}
```

---

## 9. Beginner-to-Expert Learning Path

### Must Know First

1. **State Pattern**: Understand the Gang of Four State design pattern
2. **WPEFramework Plugins**: Basic Thunder plugin architecture
3. **COM-RPC**: Inter-plugin communication mechanism

### Beginner Level

1. Read `State.h` to understand the state class hierarchy
2. Examine `LifecycleManager.config` for plugin configuration
3. Trace a simple SpawnApp call from entry to container start

### Intermediate Level

1. Study `StateHandler` for transition validation logic
2. Understand `ApplicationContext` lifecycle
3. Trace the event flow from RuntimeManager back to AppManager

### Advanced Level

1. Study the pending state queue mechanism in `ApplicationContext`
2. Analyze respawn handling in `mPendingRespawns`
3. Understand handler separation (RuntimeManagerHandler, WindowManagerHandler)

### Expert Level

1. Add new lifecycle states to the state machine
2. Implement custom state transition behaviors
3. Optimize state path calculation algorithm
4. Add new event types and handlers
