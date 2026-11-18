## Context

RDKEMW-28030 provides internal C++ methods in ResourceMonitor to query an application's lifecycle capabilities and Suspended-state configuration from Package Manager. However, these capabilities are not yet enforced. The HLA defines timeouts for PAUSED and SUSPENDED states (60s for PAUSED→SUSPENDED, 10s GPU grace period in SUSPENDED) to prevent inactive apps from consuming system resources.

ResourceMonitor provides resource statistics APIs (RDKDEV-1479) that can be queried for GPU memory and other resource information during enforcement.

RDKEMW-28031 implements lifecycle enforcement in ResourceMonitor using Thunder's timer framework, with per-app deadlines served by a single shared timer. This replaces the earlier AppManager-owned timer path in `AppStateTransitionManager` (RDKEMW-17171), which lacked GPU grace period checking and used a custom thread with condition variables.

See proposal.md for motivation.

## Goals / Non-Goals

**Goals:**
- Implement lifecycle enforcement in ResourceMonitor using Thunder's timer framework
- Enforce PAUSED→SUSPENDED timeout (default 60s) with capability checking
- Enforce SUSPENDED GPU grace period (default 10s) with GPU memory checking
- Check hibernation conditions (device support, app support, memory floor) after SUSPENDED timeout
- Terminate apps that violate GPU or memory requirements after the grace period
- Query GPU memory usage via RuntimeManager/Dobby APIs
- Identify and terminate non-app workloads on PAUSED timeout
- Capture lifecycle capabilities per appInstanceId on first PAUSED/SUSPENDED entry and retain until unload

**Non-Goals:**
- Extending AppManager's `AppStateTransitionManager` (legacy, alternative enforcement path)
- Adding a new AppManager API for lifecycle enforcement
- Continuous CPU/memory monitoring in Suspended (only one-time deadline checks are in scope)
- Modifying the RDKEMW-28030 capability provider (it is used as-is)
- Adding a memory-floor deadline (this change uses the existing package metadata configuration)

## Decisions

### Architecture: ResourceMonitor owns timers, calls LifecycleManager directly
**Decision:** ResourceMonitor owns the PAUSED and SUSPENDED timers using Thunder's timer framework. ResourceMonitor subscribes to AppManager lifecycle state change events and requests lifecycle transitions via LifecycleManager using the exact `appInstanceId`. AppManager remains responsible for notifications, application information, and termination.

**Rationale:** Using Thunder's timer framework avoids per-app timer threads and leverages the framework's worker thread. Calling LifecycleManager directly removes the need for a new AppManager API and uses the exact instance ID already associated with ResourceMonitor's deadline. LifecycleManager implements the state machine, so direct calls keep lifecycle execution in the correct component.

**Alternatives considered:**
- Extend AppManager's `AppStateTransitionManager`: Would add GPU grace period checking to the legacy custom-thread implementation, which lacks Thunder timer benefits
- Add a new AppManager API: Would require new COM-RPC method, generated bindings, and coordinated API/client rebuilds for a simple forwarding operation
- Use AppManager as a proxy: Adds a forwarding hop and converts instance-specific enforcement into app-ID requests, which is less direct when instances are replaced

### Thunder timer framework usage
**Decision:** Use Thunder's `Core::TimerType` with a single shared callback identity to manage per-app deadlines. Store deadlines as data in an `_appTimers` map, scan for the earliest deadline, and arm the shared timer for that wake-up. Process all due deadlines in each callback pass.

**Rationale:** Thunder supplies the timer handler's internal worker thread. A single shared callback avoids per-application timer registrations or worker threads. Earliest-deadline scheduling with batch expiry handling is efficient and matches the framework's design.

**Alternatives considered:**
- Custom thread with condition variables: Would duplicate RDKEMW-17171's approach, which we chose not to extend
- Per-application timer registrations: Would scale poorly with many applications

### Capability snapshot lifetime
**Decision:** Capture lifecycle capabilities per `appInstanceId` on first PAUSED or SUSPENDED entry and retain the resolved snapshot until unload. Retry `UNAVAILABLE` results on later enforcement-state entry. Without a snapshot declaring support, enforcement follows the unsupported-state termination path.

**Rationale:** Capability queries involve PackageManager RPC calls. Capturing once per instance avoids repeated queries during timer expiries. Retaining snapshots through state changes (e.g., PAUSED→ACTIVE→PAUSED) avoids race conditions where PackageManager becomes unavailable between requests.

**Alternatives considered:**
- Query capabilities on every timer expiry: Would add latency and fail if PackageManager is temporarily unavailable
- Cache globally by appId: Would not handle instance replacement correctly

### GPU and memory checking location
**Decision:** Perform GPU grace period and hibernation condition checks in ResourceMonitor's expiry handlers. Query GPU memory via RuntimeManager/Dobby APIs. Use package metadata's `maxSuspendedSystemMemory` for memory floor comparison.

**Rationale:** Expiry handlers already have the `appInstanceId` and can query external services without holding the timer mutex. RuntimeManager provides GPU stats via container inspection. Package metadata already defines the memory floor threshold.

**Alternatives considered:**
- Implement checks in AppManager: Would require new COM-RPC APIs and add latency
- Create a separate enforcement component: Adds new component with unclear ownership

### Configuration
**Decision:** Add lifecycle timeout configuration to ResourceMonitor's config: `paused_to_suspended_timeout` (default 60s) and `suspended_gpu_grace_period` (default 10s). The GPU threshold (`maxGPUallowedInSuspended`, 20MiB) and memory floor (`maxSuspendedSystemMemory`) come from package metadata.

**Rationale:** ResourceMonitor owns the timers, so its configuration should control the timeouts. GPU and memory thresholds are per-app settings from package metadata, not global platform settings.

**Alternatives considered:**
- Using AppManager's existing configuration: Would split timer configuration from timer implementation
- Hardcoding all timeouts: Would not allow tuning for different device profiles

## Risks / Trade-offs

### RuntimeManager or AppManager unavailable
**Risk:** If RuntimeManager or AppManager is unavailable when a timer expires, GPU checks or state verification cannot be performed, and enforcement may be incomplete.

**Mitigation:** Log the unavailability and skip the unavailable check. If state cannot be verified, skip enforcement for that expiry. Do not crash or block on the enforcement action.

### GPU stats unavailable on some platforms
**Risk:** Some platforms may not support GPU memory statistics via Dobby/ResourceMonitor. The GPU grace period check cannot be performed on such platforms.

**Mitigation:** If GPU stats are unavailable, skip the GPU check and proceed to hibernation condition checks. Log a warning that GPU enforcement is not available on this platform.

### Memory floor unavailable in package metadata
**Risk:** If package metadata does not define `maxSuspendedSystemMemory`, the memory floor check cannot be performed.

**Mitigation:** If the memory floor is not defined, treat it as not meeting the condition and request termination if GPU check passed but hibernation is unsupported.

### Non-app workload identification
**Risk:** The values for `LoadedAppInfo.type` may not be standardized. Misidentifying a non-app workload could lead to incorrect enforcement.

**Mitigation:** Use documented type values (e.g., "SYSTEM_APP"). If identification is ambiguous, treat the workload as an app and enforce lifecycle capabilities normally.

### Concurrent enforcement actions
**Risk:** If multiple apps have timers that expire simultaneously, the enforcement actions may contend for COM-RPC calls to LifecycleManager, AppManager, and RuntimeManager.

**Mitigation:** Expiry handlers execute sequentially on the timer dispatch path. The timer mutex protects deadline collection and scheduling, but inter-plugin calls occur without holding it. A slow call delays later handlers but does not deadlock.

### Configuration inconsistency
**Risk:** If `paused_to_suspended_timeout` or `suspended_gpu_grace_period` are configured differently across devices, enforcement behavior may vary unexpectedly.

**Mitigation:** These are ResourceMonitor configuration settings with documented defaults. Document the settings in ResourceMonitor README and provide validation in configuration loading.

### Package Manager interface caching
**Risk:** PackageManager interfaces may be cached between requests. If the service is deactivated and reactivated with a new implementation, stale interfaces could return incorrect results.

**Mitigation:** Acquire and release PackageManager interfaces per request rather than caching them persistently. Capability snapshots are retained, but the interface references are request-scoped.

## Migration Plan

No migration is required for existing deployments. This adds new enforcement in ResourceMonitor without modifying existing APIs. The feature is always enabled in the ResourceMonitor plugin; no feature flag is needed.

**Deployment steps:**
1. Deploy the updated ResourceMonitor plugin with lifecycle enforcement
2. Monitor logs for timer scheduling, expiry handling, and enforcement actions
3. Adjust ResourceMonitor configuration (`paused_to_suspended_timeout`, `suspended_gpu_grace_period`) if needed based on device characteristics

**Rollback strategy:**
- Uninstall the updated ResourceMonitor plugin to revert to the previous version
- No data migration or schema changes are involved

## Thunder Timer Scheduling

ResourceMonitor uses Thunder's timer framework with a single shared callback to manage per-app deadlines, rather than creating one timer thread per application.

### Core Design

- **Per-app deadlines:** Each application instance maintains its own PAUSED and SUSPENDED deadline stored in `_appTimers` map, keyed by `appInstanceId`
- **Shared timer handler:** One `Core::TimerType<LifecycleTimer>` and one callback identity serve all tracked applications
- **Earliest-deadline scheduling:** ResourceMonitor scans all entries, selects the earliest nonzero deadline, and arms the shared timer for that wake-up
- **Batch expiry handling:** When the timer fires, all due deadlines are collected and processed sequentially in the same callback pass
- **Re-arming without reset:** Re-arming the timer changes the pending wake-up without resetting other applications' stored deadlines

### Deadline Storage and Configuration

| Member | Purpose |
|---|---|
| `_appTimers` | Map keyed by `appInstanceId`, holding deadlines, expected states, capability snapshots, and instance generations |
| `pausedTimerExpiryMs` | Absolute monotonic PAUSED deadline; `0` means inactive |
| `suspendedTimerExpiryMs` | Absolute monotonic SUSPENDED GPU-grace deadline; `0` means inactive |
| `_appTimersMutex` | Protects the map and scheduling decisions |
| `_timer` | Shared `Core::ProxyType<LifecycleTimer>` callback object |
| `_timerHandler` | `Core::TimerType<LifecycleTimer>` managing the pending framework wake-up |
| `_timerStopping` | Prevents further timer arming during shutdown |

Configuration keys:

| Configuration key | Default | Deadline purpose |
|---|---|---|
| `paused_to_suspended_timeout` | 60 seconds | Evaluate whether a PAUSED instance should be suspended or terminated |
| `suspended_gpu_grace_period` | 10 seconds | Evaluate GPU release and the next lifecycle action for a SUSPENDED instance |

### Scheduling Flow

When an instance enters PAUSED or SUSPENDED:

1. ResourceMonitor captures capabilities if it has no resolved snapshot for that instance (outside the timer-map mutex)
2. Under `_appTimersMutex`, it stores the expected state and the appropriate deadline using `std::chrono::steady_clock`
3. `ScheduleNextTimerLocked()` scans both deadline fields of every tracked instance and selects the earliest nonzero deadline
4. It converts the remaining monotonic delay to the framework timestamp and calls `Trigger()` with the shared callback

Re-arming uses the same callback identity. Thunder's `Trigger()` removes an existing matching pending entry before scheduling its replacement, so the wake-up changes without affecting other stored deadlines.

### Expiry Processing

Each callback:

1. Locks `_appTimersMutex` and finds **all** nonzero deadlines at or before the current monotonic time
2. Copies `(appInstanceId, deadlineType)` pairs into a local list and clears the corresponding deadline fields to `0`
3. Releases the mutex before invoking the stage-specific expiry handlers
4. Each handler checks that the instance record still exists and that the application remains in the expected state before enforcing an action
5. Reacquires the mutex and schedules the next earliest remaining deadline, or revokes the pending wake-up when none remain

Handlers execute sequentially on the timer dispatch path, not on parallel per-app workers. A scheduled deadline is an evaluation time, not a guarantee of an instantaneous completed lifecycle transition.

### Cancellation and Shutdown

| Situation | Scheduler behavior |
|---|---|
| Exit PAUSED | Clear that instance's PAUSED deadline and recalculate the next wake-up |
| Exit SUSPENDED | Clear that instance's GPU-grace deadline and recalculate the next wake-up |
| Reactivation, hibernation, or termination | Cancel outstanding deadlines while retaining an established capability snapshot |
| Confirmed unload | Erase the instance's map entry, including its capability snapshot |
| No active deadlines found during scheduling | Revoke the pending callback |
| Shutdown | Set `_timerStopping` under the mutex, call `Flush()` to drain in-flight callbacks, then revoke the shared callback before plugin-interface cleanup |

Resolved capability snapshots survive timer cancellation until unload. `UNAVAILABLE` is not retained as a resolved snapshot and can be retried on a later PAUSED/SUSPENDED entry.

## Open Questions

None. The design decisions are based on the HLA requirements and the architectural clarifications provided. All implementation details can be resolved during coding without changing the approach or the spec.
