# AppManager Resource Monitoring Integration

## Overview

AppManager now coordinates with the App System Resource Monitor before launching or preloading applications and drives inactive applications through the lifecycle described by the App Resource Monitoring HLA.

The implementation covers three areas:

1. Foreground launches request resource reconciliation without waiting for completion.
2. Preloads are accepted after reconciliation submission and proceed only when the completion callback reports that the RAM target is achieved.
3. Confirmed PAUSED and SUSPENDED lifecycle states are advanced by a timer-driven state-transition worker.

ResourceMonitor remains responsible for observing platform-level signals such as `MemAvailable`, PSI, and swap usage. AppManager does not duplicate that monitoring. Each reconciliation request identifies the application and the maximum RAM target it needs.

## Feature Gate

All ResourceMonitor integration and inactive-state scheduling is compiled only when CMake receives `-DAPP_MANAGER_RESOURCE_MONITOR=ON`. The `entservices-rdkappmanagers` recipe sets this option only when `DISTRO_FEATURES` contains `resource-monitor`.

When the feature is absent, AppManager has no ResourceMonitor dependency: it does not include or query the interface, register callbacks, start the state-transition worker, alter launch or preload behavior, or add resource-monitoring configuration values.

## ResourceMonitor Contract

The COM-RPC interface is defined in `entservices-apis/apis/ResourceMonitor/IResourceMonitor.h`.

```cpp
struct IResourceMonitor : virtual public Core::IUnknown {
    struct INotification : virtual public Core::IUnknown {
      virtual void OnReconciliationComplete(const string& appId, bool targetRamAchieved) = 0;
    };

    virtual Core::hresult Register(INotification* notification) = 0;
    virtual Core::hresult Unregister(INotification* notification) = 0;
    virtual Core::hresult Reconcile(const string& appId, uint32_t ramTargetMB, bool allowTerminate) = 0;
};
```

The callsign used by AppManager is:

```text
org.rdk.ResourceMonitor
```

The interface and notification IDs are reserved in `entservices-apis/apis/Ids.h`.

The ResourceMonitor team's final interface must use the same callsign, method parameters, notification parameters, and interface IDs. If its final contract differs, both repositories must be updated together before integration.

## Initialization and Lifetime

During `AppManagerImplementation::Configure`:

1. AppManager queries `org.rdk.ResourceMonitor` through `QueryInterfaceByCallsign`.
2. It registers `ResourceMonitorNotification` for reconciliation completion events.
3. Failure to find ResourceMonitor does not prevent AppManager initialization.
4. Reconciliation methods retry interface discovery, allowing ResourceMonitor to activate after AppManager.
5. The inactive-state worker is configured and started.

During destruction:

1. The state-transition worker is stopped first.
2. LifecycleManager and ResourceMonitor interfaces are then unregistered and released.
3. This ordering prevents the worker from using released COM-RPC interfaces.

## Launch Flow

`LaunchApp` validates the request and then calls:

```cpp
Reconcile(appId, ramTargetMB, true);
```

`ramTargetMB` is derived from the application's configured system-memory limit. `allowTerminate=true` permits ResourceMonitor to invoke victim eviction if resource headroom is insufficient.

AppManager does not wait for `OnReconciliationComplete`. After the `Reconcile` method accepts the asynchronous request, the launch request is queued normally. Failure to start reconciliation is logged but does not reject the foreground launch.

```mermaid
sequenceDiagram
    participant Client
    participant AM as AppManager
    participant RM as ResourceMonitor
    participant LM as LifecycleManager

    Client->>AM: LaunchApp(appId, intent, args)
    AM->>RM: Reconcile(appId, ramTargetMB, true)
    RM-->>AM: Request accepted
    par Launch application
      AM->>AM: Queue launch request
      AM->>LM: Launch application
    and Reconciliation completes asynchronously
      RM-->>AM: OnReconciliationComplete(appId, targetRamAchieved)
    end
```

AppManager releases its administrative lock around the COM-RPC call so ResourceMonitor and VictimSelector can call back into AppManager without causing a cross-plugin deadlock.

## Preload Flow

`PreloadApp` calls:

```cpp
Reconcile(appId, ramTargetMB, false);
```

`allowTerminate=false` requests an availability check without evicting applications for an optional preload.

The method returns after ResourceMonitor accepts the asynchronous request. AppManager retains the preload request and waits for `OnReconciliationComplete` before acting on it:

- `targetRamAchieved=true`: queue and execute the preload.
- `targetRamAchieved=false`: report failure through `onAppLifecycleStateChanged` with `APP_ERROR_UNKNOWN`.
- ResourceMonitor unavailable or request failure: return `Core::ERROR_GENERAL` and report failure through `onAppLifecycleStateChanged` with `APP_ERROR_UNKNOWN`.

The output `error` string reports only immediate validation or reconciliation-submission failures. The lifecycle event is the result of an accepted preload request.

```mermaid
sequenceDiagram
    participant Client
    participant AM as AppManager
    participant RM as ResourceMonitor
    participant LM as LifecycleManager

    Client->>AM: PreloadApp(appId, intent, args)
    AM->>RM: Reconcile(appId, ramTargetMB, false)
    RM-->>AM: Request accepted
    AM-->>Client: Request accepted
    RM-->>AM: OnReconciliationComplete(appId, targetRamAchieved)
    alt targetRamAchieved
        AM->>AM: Queue preload request
        AM->>LM: Preload application
    else target not achieved or request failed
      AM-->>Client: onAppLifecycleStateChanged(APP_ERROR_UNKNOWN)
    end
```

## Reconciliation Synchronization

The ResourceMonitor contract does not include a separate request ID. Foreground reconciliation is fire-and-forget: ResourceMonitor uses `appId` for telemetry and AppManager does not wait for or process its completion event. AppManager retains pending preload requests until their callback arrives:

- `mPendingPreloads`: retains accepted preload requests until their callback queues the preload or reports a lifecycle error.

Foreground launches and preloads invoke `Reconcile` directly and do not block. Preload callbacks enqueue the preload after a successful resource check or send an error lifecycle event otherwise. The callback `appId` identifies the pending preload request.

A request identifier in the future ResourceMonitor contract would allow safe correlation if same-app reconciliation requests can overlap.

## Inactive Application State Flow

`AppStateTransitionManager` runs on a dedicated worker thread. Lifecycle callbacks only update timer state; they do not sleep or perform blocking transitions.

```mermaid
flowchart TD
    I[Application enters inactive state] --> P{PAUSED accepted?}
    P -- No or timeout --> T[Terminate]
    P -- Yes --> PS[PAUSED]
    PS --> E{Paused timer expires or another app becomes ACTIVE/PAUSED}
    E --> S{SUSPENDED accepted?}
    S -- No --> T
    S -- Yes --> SS[SUSPENDED]
    SS --> H{Suspended timer expires}
    H --> C{App supports hibernation and flash space is available?}
    C -- No --> SS
    C -- Yes --> HH[Request HIBERNATED]
```

### PAUSED behavior

When AppManager receives a confirmed PAUSED event:

- It records a deadline using `pausedToSuspendedTimeout`.
- Only one application is tracked as the current paused application.
- If another app becomes ACTIVE or PAUSED, the previous paused application's deadline is advanced immediately.
- At the deadline, AppManager requests SUSPENDED through LifecycleManager.
- If PAUSED was rejected or not confirmed by the existing close timeout, AppManager terminates the app.
- If SUSPENDED is rejected, AppManager terminates the app.

### SUSPENDED behavior

When AppManager receives a confirmed SUSPENDED event:

- It records a deadline using `suspendedToHibernatedTimeout`.
- At the deadline, AppManager checks the app's `APPLICATION_CAN_RUN_IN_HIBERNATE_MODE` property and available flash space.
- HIBERNATED is requested when the app supports hibernation and enough flash space is available for its configured `dataImageSize`.
- If flash space is unavailable, the app remains suspended and is checked again after `suspendedToHibernatedTimeout`.
- If hibernation is unsupported, the app remains suspended without another hibernation retry.
- If hibernation is rejected, the app remains suspended and is retried after `suspendedToHibernatedTimeout`.

### Other states

ACTIVE, HIBERNATED, TERMINATING, UNLOADED, and unrelated states remove the application from timer tracking. App removal also explicitly clears any scheduler entry.

## LifecycleManager Integration

`LifecycleInterfaceConnector::setTargetAppState` provides the scheduler with a generic transition path:

1. Resolve `appId` to `appInstanceId` through `AppInfoManager`.
2. Call `ILifecycleManager::SetTargetAppState`.
3. Update AppManager's target-state bookkeeping after a successful request.
4. Wait for normal lifecycle notifications to confirm the actual state.

The scheduler never writes lifecycle state directly.

## Configuration

The following CMake cache variables are available:

| Variable | Default | Meaning |
|---|---:|---|
| `PLUGIN_APP_MANAGER_PAUSED_TO_SUSPENDED_TIMEOUT` | `60` | Seconds an app may remain PAUSED |
| `PLUGIN_APP_MANAGER_SUSPENDED_TO_HIBERNATED_TIMEOUT` | `15` | Seconds before evaluating a SUSPENDED app |
| `PLUGIN_APP_MANAGER_HIBERNATION_STORAGE_PATH` | `/media/apps/memcr` | Filesystem path used to check available hibernation flash space |

They are emitted into the generated AppManager configuration as:

```json
{
  "pausedToSuspendedTimeout": 60,
  "suspendedToHibernatedTimeout": 15,
  "hibernationStoragePath": "/media/apps/memcr"
}
```

Both modern `AppManager.conf.in` and legacy `AppManager.config` generation paths contain these values.

## Files Changed

### entservices-apis

- `apis/ResourceMonitor/IResourceMonitor.h`
  - Defines reconciliation and completion notification COM-RPC contracts.
- `apis/Ids.h`
  - Reserves ResourceMonitor interface IDs.

### entservices-appmanagers

- `AppManager/AppManagerImplementation.h/.cpp`
  - Owns ResourceMonitor connection, notification sink, synchronization, launch/preload integration, and scheduler lifetime.
- `AppManager/AppStateTransitionManager.h/.cpp`
  - Implements timer-driven inactive-state progression.
- `AppManager/LifecycleInterfaceConnector.h/.cpp`
  - Adds generic target-state requests and termination on failed/unconfirmed PAUSED transitions.
- `AppManager/AppManager.conf.in`
  - Adds modern configuration values.
- `AppManager/AppManager.config`
  - Adds legacy configuration values.
- `AppManager/CMakeLists.txt`
  - Adds state-flow configuration defaults and builds the scheduler source.
- `Tests/L0Tests/CMakeLists.txt`
  - Includes the scheduler source in the AppManager L0 target.

## Validation and Test Coverage

Completed locally:

- Editor diagnostics for new scheduler and modified lifecycle files.
- CMake diagnostics check.
- `git diff --check` in both repositories.

A full build was not available because the local environment has no configured CMake target and lacks the Linux/Thunder development include path.

Focused tests should cover:

1. Launch calls `Reconcile(appId, ramTargetMB, true)` and does not wait for completion.
2. Preload returns after reconciliation submission and proceeds after `targetRamAchieved=true`.
3. Preload failure after `targetRamAchieved=false` is reported through `onAppLifecycleStateChanged`.
4. Preload reconciliation-submission failure reports `Core::ERROR_GENERAL` and `onAppLifecycleStateChanged`.
5. A duplicate pending preload for the same app is rejected.
6. PAUSED advances after its timeout.
7. A new ACTIVE or PAUSED app immediately advances the prior paused app.
8. Failed PAUSED or SUSPENDED requests terminate the app.
9. SUSPENDED advances to HIBERNATED when hibernation support and flash-space checks succeed.
10. Disabled or failed hibernation leaves the app suspended.
11. Plugin destruction stops the worker before releasing COM-RPC dependencies.

## Known Limitations

- The ResourceMonitor API currently has no request correlation ID. AppManager relies on the contract that same-app reconciliation requests do not overlap.
- Application support for PAUSED, SUSPENDED, and HIBERNATED is inferred from LifecycleManager request/confirmation behavior. Explicit capability metadata is not currently available through AppManager's package data.
- Hibernation support is read from the AppManager property `APPLICATION_CAN_RUN_IN_HIBERNATE_MODE`; the legacy `supportsHibernation` property is accepted as a fallback. If neither property is present, the app is not hibernation-capable.
- Flash requirement is currently estimated from `RuntimeConfig.dataImageSize`, and free space is checked with `statvfs` on `hibernationStoragePath`.
- Unit tests for the newly added flows still need to be implemented.
