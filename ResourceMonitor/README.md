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

**Source.** Values come only from Package Manager. Package Manager copies the locked package's RALF metadata block `urn:rdk:config:application-lifecycle` into the runtime configuration payload as `applicationLifecycle`. ResourceMonitor reads that payload through `IPackageHandler::GetLockedInfo` on `org.rdk.AppPackageManager`, which it connects to on first use.

**Package resolution.** `appId` is used unchanged as the package id. Without a `version`, exactly one installed version must be locked.

**Status.**

| Status | Meaning | States reported |
|---|---|---|
| `CONFIGURED` | RALF package with the lifecycle block | `ACTIVE` plus the states in `supportedNonActiveStates` (case-insensitive) |
| `NOT_CONFIGURED` | RALF package without the block | `ACTIVE` only |
| `UNAVAILABLE` | Legacy (non-RALF) package, no or several locked versions, Package Manager unavailable, or malformed data | `ACTIVE` only; callers should not enforce lifecycle |

**Suspended configuration.** Each value is reported as present or unavailable on its own:
- `maxSuspendedSystemMemory`, for example `"16M"`, converted to bytes;
- `maxTimeToSuspendMemoryState`, in seconds;
- a CPU threshold, which is always unavailable until a RALF metadata member is defined for it.

**Usage.**
- The methods are safe to call from several threads.
- Call them without holding ResourceMonitor's admin, reconcile or reactive-monitor locks.
- Take a snapshot of the result per `appInstanceId` at launch.
- No outcome is treated as a failure: every case returns `ERROR_NONE` with a status, except an empty `appId`, which returns `ERROR_INVALID_PARAMETER`.
