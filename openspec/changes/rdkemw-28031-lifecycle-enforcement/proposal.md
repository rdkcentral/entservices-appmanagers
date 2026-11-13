## Why

The HLA defines lifecycle state timeouts (PAUSED→SUSPENDED after 60s, SUSPENDED→HIBERNATED after 15s with GPU grace period) to prevent inactive apps from consuming system resources indefinitely. Currently these timeouts are not enforced, so apps can remain in PAUSED or SUSPENDED states without releasing GPU memory or reducing memory usage, risking platform resource exhaustion.

## What Changes

ResourceMonitor will enforce lifecycle state timers:
- Subscribe to AppManager lifecycle state change events (`onAppLifecycleStateChanged`)
- Maintain per-app timers for PAUSED and SUSPENDED states
- When PAUSED timer expires: check if app supports SUSPENDED (using RDKEMW-28030's `IsLifecycleStateSupported`), request SUSPENDED transition via LifecycleManager if supported, or request termination via AppManager if not
- When app enters SUSPENDED: start GPU grace period timer (`suspendedToHibernatedTimeout`, default 15s), sample GPU memory via RuntimeManager/Dobby
- After grace period: if GPU memory still above `maxGPUallowedInSuspended` (default 20 MiB), request termination; if GPU released, check hibernation conditions (device support, app support, memory floor below `maxSuspendedSystemMemory`)
- If hibernation conditions not met after SUSPENDED timeout: request termination
- Add configuration for `pausedToSuspendedTimeout` (default 60s, global setting)

## Capabilities

### New Capabilities
- `lifecycle-enforcement`: Per-app timer enforcement for PAUSED and SUSPENDED lifecycle states, including GPU grace period and hibernation condition checks

### Modified Capabilities
None (no existing spec requirements change)

## Impact

- ResourceMonitor plugin: new timer thread or timer-based event loop, event subscription to AppManager
- Dependencies: AppManager (lifecycle events), LifecycleManager (state transitions), RuntimeManager/Dobby (GPU/memory stats)
- Configuration: new global setting for `pausedToSuspendedTimeout` in ResourceMonitor config
- No new external dependencies
- No breaking changes to existing APIs
