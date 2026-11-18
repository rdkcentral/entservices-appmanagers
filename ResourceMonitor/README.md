-----------------
# ResourceMonitor

## Versions
`org.rdk.ResourceMonitor`

## Methods:
```
curl -X POST http://127.0.0.1:9998/jsonrpc -H "Content-Type: application/json" -d '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"org.rdk.ResourceMonitor.getMemInfo"
}'
 
curl -X POST http://127.0.0.1:9998/jsonrpc -H "Content-Type: application/json" -d '{
    "jsonrpc":"2.0",
    "id":2,
    "method":"org.rdk.ResourceMonitor.getSwapUsed"
}'
 
3. GetPsiMetrics
curl -X POST http://127.0.0.1:9998/jsonrpc -H "Content-Type: application/json" -d '{
    "jsonrpc":"2.0",
    "id":3,
    "method":"org.rdk.ResourceMonitor.getPsiMetrics",
    "params":{
        "metric":"full_avg60"
    }
}'
 
curl -X POST http://127.0.0.1:9998/jsonrpc -H "Content-Type: application/json" -d '{
    "jsonrpc":"2.0",
    "id":4,
    "method":"org.rdk.ResourceMonitor.getPsiMetrics",
    "params":{
        "metric":"some_avg10"
    }
}'
 
curl -X POST http://127.0.0.1:9998/jsonrpc -H "Content-Type: application/json" -d '{
    "jsonrpc":"2.0",
    "id":5,
    "method":"org.rdk.ResourceMonitor.getFlashSpace"
}'

4. GetStats:
curl -X POST http://127.0.0.1:9998/jsonrpc -H "Content-Type: application/json" -d '{
    "jsonrpc":"2.0",
    "id":5,
    "method":"org.rdk.ResourceMonitor.getStats"
}'

```

## Responses
```
GetFlashSpace:
{"jsonrpc":"2.0","id":5,"result":{"total":314572800,"used":8484,"available":314564316}}

GetMemInfo:
{"jsonrpc":"2.0","id":1,"result":{"memAvailable":7493700,"swapFree":7674528,"swapTotal":7674528}}

GetSwapUsed:
{"jsonrpc":"2.0","id":2,"result":{"swapUsed":4096,"memUsedTotal":12288}}

GetPsiMetrics:
{"jsonrpc":"2.0","id":3,"result":0}

GetStats:
{"jsonrpc":"2.0","id":2,"result":{"systemMemTotal":7858720,"systemMemUsed":366156,"systemMemAvailable":7492564,"peakSystemMemUsed":366156,"gpuMemTotal":0,"gpuMemUsed":0,"gpuMemAvailable":0,"peakGpuMemUsed":0,"swapTotal":7674528,"swapFree":7674528,"swapUsed":4096,"peakSwapUsed":4096,"flashSpaceTotal":1012768768,"flashSpaceUsed":495165440,"flashSpaceAvailable":517603328,"peakFlashSpaceUsed":495165440}}

```

## Events
```
onReconciliationComplete(success/failure)
```
## Lifecycle capabilities (internal)

ResourceMonitor can look up which lifecycle states an application supports, and its Suspended-state limits, from Package Manager. Lifecycle enforcement (RDKEMW-28031) uses these. They are C++ members of `ResourceMonitorImplementation` only; they are not exposed over COM-RPC or JSON-RPC.

```cpp
Core::hresult GetAppLifecycleCapabilities(const string& appId, AppLifecycleCapabilities& caps, const string& version = "");
Core::hresult IsAppLifecycleStateSupported(const string& appId, AppLifecycleState state, bool& supported, const string& version = "");
Core::hresult GetAppSuspendedStateConfig(const string& appId, SuspendedStateConfig& config, const string& version = "");
```

**Source.** Values come only from PackageManager. PackageManager copies the locked package's RALF metadata block `urn:rdk:config:application-lifecycle` into the runtime configuration payload as `applicationLifecycle`. ResourceMonitor reads that payload through `IPackageHandler::GetLockedInfo` on `org.rdk.AppPackageManager`.

**Interface lifetime.** Each package-list operation queries and releases `IPackageInstaller`; each locked-info operation queries and releases `IPackageHandler`, including error paths. Interfaces are not cached between requests, so later requests can reach a reactivated or replacement PackageManager implementation. An explicit version needs only the handler; it does not require the installer interface.

**Package resolution.** `appId` is used unchanged as the package id. Without a `version`, exactly one installed version must be locked.

**Status.**

| Status | Meaning | States reported |
|---|---|---|
| `CONFIGURED` | RALF package with the lifecycle block | `ACTIVE` plus the states in `supportedNonActiveStates` (case-insensitive) |
| `NOT_CONFIGURED` | RALF package without the block | `ACTIVE` only |
| `UNAVAILABLE` | Legacy (non-RALF) package, no or several locked versions, PackageManager unavailable, or malformed data | `ACTIVE` only; not a resolved capability snapshot or an automatic enforcement exemption |

**Suspended configuration.** Each value is reported as present or unavailable on its own:
- `maxSuspendedSystemMemory`, for example `"16M"`, converted to bytes;
- `maxTimeToSuspendMemoryState`, in seconds;
- a CPU threshold, which is always unavailable until a RALF metadata member is defined for it.

**Usage.**
- The methods are safe to call from several threads.
- Call them without holding ResourceMonitor's admin, reconcile or reactive-monitor locks.
- Capture capabilities per `appInstanceId` before the first PAUSED or SUSPENDED enforcement entry, including first-observed SUSPENDED, rather than at launch.
- Retain a resolved snapshot across later state changes and metadata unavailability until confirmed unload. Do not retain `UNAVAILABLE` as a resolved snapshot; a later enforcement-state entry can retry acquisition. A generation check prevents an in-flight lookup from restoring an unloaded instance's entry.
- An unresolved lookup does not stop a deadline from starting. Without a snapshot declaring the required SUSPENDED/HIBERNATED capability, expiry takes the corresponding unsupported-state termination path. Provider status and enforcement policy are separate concerns.
- No outcome is treated as a failure: every case returns `ERROR_NONE` with a status, except an empty `appId`, which returns `ERROR_INVALID_PARAMETER`.

## Lifecycle enforcement (RDKEMW-28031)

ResourceMonitor observes AppManager lifecycle notifications and owns the deadline decisions. It requests SUSPENDED/HIBERNATED directly through the existing `ILifecycleManager::SetTargetAppState(appInstanceId, targetState, launchIntent)`, acquiring and releasing LifecycleManager for each request. The intent is empty, preserving LifecycleManager's existing intent. No new public `IAppManager::SetTargetAppState` method or target-state metric is required.

AppManager remains the application-information/notification source and handles termination through its existing API. Its `GetLoadedApps()` path refreshes observed and target state from LifecycleManager. ResourceMonitor monitors AppManager/RuntimeManager service changes to refresh its subscriptions and synchronized interface references.

| Configuration key | Default | Action at expiry, after checking the expected state |
|---|---|---|
| `paused_to_suspended_timeout` | 60 s | `SYSTEM_APP`/`SYSTEM` workloads request termination. Other apps request SUSPENDED only if the snapshot declares support; otherwise request termination. |
| `suspended_gpu_grace_period` | 10 s | Query per-container GPU usage through RuntimeManager `GetInfo`. Nonzero usage requests termination. Zero usage requests HIBERNATED if supported, otherwise termination. An unavailable or malformed GPU query skips the expiry's action. |

A failed/unavailable transition request is logged; it does not introduce automatic retry or a new termination-on-RPC-failure policy. An unavailable GPU query also does not automatically re-arm the deadline. The reported Suspended memory metadata is not a memory-floor enforcement gate in this change. No Dobby changes, new device-hibernation API, CPU enforcement, or HLA 20 MiB GPU allowance were added.

### One Thunder timer, multiple application deadlines

- `_appTimers` stores separate PAUSED and SUSPENDED monotonic deadlines for every tracked `appInstanceId`; a deadline of `0` is inactive. Expected states and capability snapshots are stored with the instance.
- `_timerHandler` is one `Core::TimerType<LifecycleTimer>`, with one shared `_timer` callback object per ResourceMonitor implementation. Thunder supplies its internal timer worker; there is no custom `TimerWorkerLoop` or per-app timer thread.
- `ScheduleNextTimerLocked()` scans all nonzero deadlines and arms the earliest one. `Trigger()` replaces the matching pending callback entry, so starting another app's deadline does not create another framework timer registration or reset the other apps' deadlines.
- `OnLifecycleTimerExpired()` collects all due deadlines under `_appTimersMutex` and clears their fields. It invokes the handlers sequentially outside that mutex; handlers recheck the live state before acting. It then schedules the next earliest deadline. `Timed()` returns `0`, so repetition is explicit re-arming rather than a fixed polling interval.
- Example: A is PAUSED until t=60 s while B and C are SUSPENDED until t=10 s. One wake-up at t=10 s handles both B and C, then selects A's t=60 s deadline. If D enters SUSPENDED at t=20 s, its t=30 s deadline moves the shared wake-up earlier without changing A's stored deadline.
- State exits cancel the corresponding deadlines; reactivation, hibernation, or termination cancels outstanding deadlines while retaining a resolved snapshot. Unload erases the instance entry. An already queued wake-up may still occur after unload, but the erased entry is absent from the scan.
- Shutdown sets `_timerStopping` and revokes the callback before plugin-interface cleanup. The reactive-monitor and reconciliation workers remain separate from this lifecycle scheduler.

### Planning-document status

The older RDKEMW-28030/28031 OpenSpec artifacts still include cached-interface guidance and superseded AppManager-owned timer/memory-floor assumptions. They require a separately confirmed reconciliation; they are not an accurate completed-task checklist for this implementation. This README describes the current behavior without silently changing those planning requirements.
