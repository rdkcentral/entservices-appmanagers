Branch: `RDKEMW-28031` (from `RDKEMW-28030` commit `7d27b9ec`). All commands run from `../local-build/` through `docker-run.sh` on `linux/amd64`, per `AGENTS.md`.

## 1. ResourceMonitor: Add Thunder timer infrastructure

- [ ] 1.1 Add timer handler and callback members to `ResourceMonitorImplementation.h`: `_timerHandler` (Core::TimerType), `_timer` (Core::ProxyType<LifecycleTimer>), `_appTimersMutex`, `_timerStopping`. Add `LifecycleTimer` class inheriting from `Core::IDispatch` with `Timed()` and `Dispatch()` methods. Acceptance: Timer infrastructure compiles, callback is registered.
- [ ] 1.2 Add per-app deadline storage to `ResourceMonitorImplementation.h`: `AppTimerState` struct with `pausedTimerExpiryMs`, `suspendedTimerExpiryMs`, `expectedState`, `capabilities` snapshot, and `generation`. Add `_appTimers` map keyed by `appInstanceId`. Acceptance: Deadline storage compiles, map is accessible.

## 2. ResourceMonitor: Implement deadline scheduling

- [ ] 2.1 Implement `StartPausedTimerLocked()` and `StartSuspendedTimerLocked()` in `ResourceMonitorImplementation.cpp`. Calculate absolute monotonic deadlines using `std::chrono::steady_clock`, store in `AppTimerState`, and call `ScheduleNextTimerLocked()`. Acceptance: Deadlines are stored correctly, scheduling is triggered.
- [ ] 2.2 Implement `ScheduleNextTimerLocked()` to scan all entries in `_appTimers`, select the earliest nonzero deadline, convert to Thunder timestamp, and call `_timerHandler.Trigger()` with the shared callback. If no active deadlines, call `Revoke()`. Acceptance: Earliest deadline is selected, timer is armed or revoked correctly.
- [ ] 2.3 Implement timer cancellation helpers: `CancelPausedTimer()`, `CancelSuspendedTimer()`, and `CancelAllTimers()`. Clear deadline fields and reschedule the next wake-up. Acceptance: Deadlines are cleared, next wake-up is recalculated.

## 3. ResourceMonitor: Implement expiry handling

- [ ] 3.1 Implement `LifecycleTimer::Timed()` and `Dispatch()` to call `OnLifecycleTimerExpired()`. Return `0` to disable automatic repetition. Acceptance: Callback executes on timer expiry.
- [ ] 3.2 Implement `OnLifecycleTimerExpired()` to collect all due deadlines under `_appTimersMutex`, clear them, release the mutex, and invoke stage-specific handlers. Reacquire the mutex and schedule the next wake-up. Acceptance: Due deadlines are processed sequentially, next wake-up is scheduled.
- [ ] 3.3 Implement `OnPausedTimerExpired()` handler. Check that the app remains in PAUSED, query lifecycle capabilities (if not already captured), request SUSPENDED via LifecycleManager if supported, or request termination via AppManager if not. Acceptance: PAUSED expiry enforces SUSPENDED or termination correctly.
- [ ] 3.4 Implement `OnSuspendedTimerExpired()` handler. Check that the app remains in SUSPENDED, query GPU memory via RuntimeManager/Dobby, compare against `maxGPUallowedInSuspended`. If GPU exceeds threshold, request termination. If GPU is within limits, check hibernation conditions (device support, app support, memory floor) and request HIBERNATED or termination accordingly. Acceptance: SUSPENDED expiry enforces GPU check and hibernation conditions correctly.

## 4. ResourceMonitor: Subscribe to lifecycle state changes

- [ ] 4.1 Add lifecycle state change subscription to `ResourceMonitorImplementation.cpp`. Register for AppManager's `onAppLifecycleStateChanged` notification. Acceptance: Notification handler is registered, state changes are received.
- [ ] 4.2 Implement `OnAppLifecycleStateChanged()` handler. On PAUSED entry, call `StartPausedTimerLocked()`. On SUSPENDED entry, call `StartSuspendedTimerLocked()`. On exit from PAUSED/SUSPENDED, call the appropriate cancellation helper. Acceptance: Timers are started and cancelled correctly on state changes.

## 5. ResourceMonitor: Capability snapshot lifetime

- [ ] 5.1 Implement capability capture on first PAUSED/SUSPENDED entry. Query `PackageLifecycleConfigProvider` for lifecycle capabilities, store in `AppTimerState.capabilities` if successful. Use a generation counter to guard against restoring an unloaded/replaced entry. Acceptance: Capabilities are captured once per instance, generation guard prevents race conditions.
- [ ] 5.2 Implement snapshot retention and retry. Retain resolved snapshots until unload. If capability query returns `UNAVAILABLE`, do not retain the snapshot and retry on later PAUSED/SUSPENDED entry. Acceptance: Successful snapshots persist, `UNAVAILABLE` is retried.

## 6. ResourceMonitor: LifecycleManager interface acquisition

- [ ] 6.1 Add request-scoped LifecycleManager interface acquisition in `RequestSuspendedTransition()` and `RequestHibernatedTransition()`. Query `ILifecycleManager` per request, call `SetTargetAppState()` with the exact `appInstanceId`, then release the interface. Acceptance: LifecycleManager is acquired and released per request, transitions use instance ID.

## 7. ResourceMonitor: PackageManager interface lifetime

- [ ] 7.1 Modify `PackageLifecycleConfigProvider` to acquire and release PackageManager interfaces per request. Acquire `IPackageInstaller` for `ListPackages()` and `IPackageHandler` for `GetLockedInfo()` within each method, then release them. Clear cached interfaces on connection errors or destruction. Acceptance: Interfaces are request-scoped, stale interfaces are cleared.

## 8. ResourceMonitor: GPU and memory checking

- [ ] 8.1 Implement GPU memory query in `OnSuspendedTimerExpired()` via RuntimeManager/Dobby. Use container inspection to get GPU memory usage. Log error if query fails. Acceptance: GPU memory is queried, errors are logged gracefully.
- [ ] 8.2 Implement hibernation condition checks. Query `LoadedAppInfo` for workload type to identify non-app workloads. Use package metadata's `maxSuspendedSystemMemory` for memory floor comparison. Acceptance: Hibernation conditions are checked correctly, non-app workloads are identified.

## 9. ResourceMonitor: Configuration

- [ ] 9.1 Add configuration keys to `ResourceMonitorConfig.h`: `paused_to_suspended_timeout` (default 60s) and `suspended_gpu_grace_period` (default 10s). Acceptance: Configuration keys are defined with correct defaults.
- [ ] 9.2 Load configuration values in `ResourceMonitorConfig.cpp` and use them in deadline calculations. Acceptance: Configuration is loaded and applied correctly.

## 10. ResourceMonitor: Shutdown and cleanup

- [ ] 10.1 Implement shutdown in `~ResourceMonitorImplementation()`. Set `_timerStopping` under `_appTimersMutex`, call `_timerHandler.Revoke()`, call `_timerHandler.Flush()` to drain in-flight callbacks, then release callback-accessible resources. Acceptance: Timer is stopped, in-flight callbacks are drained, no use-after-release.

## 11. Testing

- [ ] 11.1 Add L0 tests in `Tests/L0Tests/ResourceMonitor/ResourceMonitor_EnforcementTests.cpp` for PAUSED and SUSPENDED enforcement logic. Mock LifecycleManager, AppManager, and RuntimeManager APIs. Acceptance: L0 tests pass, enforcement logic is verified.
- [ ] 11.2 Add L0 tests for timer scheduling and cancellation in `Tests/L0Tests/ResourceMonitor/ResourceMonitor_TimerTests.cpp`. Verify earliest-deadline scheduling, batch expiry handling, and cancellation. Acceptance: L0 tests pass, timer behavior is verified.
- [ ] 11.3 Add L0 tests for capability snapshot lifetime in `Tests/L0Tests/ResourceMonitor/ResourceMonitor_LifecycleConcurrencyTests.cpp`. Verify snapshot retention, retry on `UNAVAILABLE`, and request-scoped interface acquisition. Acceptance: L0 tests pass, snapshot behavior is verified.
- [ ] 11.4 Add L1 tests in `Tests/L1Tests/tests/test_ResourceMonitor.cpp` for end-to-end lifecycle enforcement with real plugin initialization. Cover PAUSED→SUSPENDED, SUSPENDED→HIBERNATED, and termination scenarios. Acceptance: L1 tests pass, integration with AppManager, LifecycleManager, and RuntimeManager is verified.
- [ ] 11.5 Register new L0 test targets in `Tests/L0Tests/ResourceMonitor/CMakeLists.txt`. Acceptance: CMake configuration succeeds, L0 tests are included in the build.

## 12. Documentation

- [ ] 12.1 Update `ResourceMonitor/README.md` with lifecycle enforcement configuration and behavior. Document timer scheduling, capability snapshots, and integration with LifecycleManager. Acceptance: README is updated, enforcement is documented.
- [ ] 12.2 Update `ResourceMonitor/PackageLifecycleConfigProvider.h` usage contract to reflect PAUSED/SUSPENDED entry capture and snapshot lifetime. Acceptance: Header documentation is accurate.

## 13. Verification

- [ ] 13.1 Plugin build: `NO_TTY=1 ./docker-run.sh build.sh entservices-appmanagers plugins --clean`. Zero compile or link errors. Acceptance: build succeeds.
- [ ] 13.2 L0: `NO_TTY=1 ./docker-run.sh build.sh entservices-appmanagers l0 run --clean`. Every `*_l0test` runs, including new ResourceMonitor enforcement and timer tests. Record pass and fail counts. Acceptance: all L0 tests pass.
- [ ] 13.3 L1: `NO_TTY=1 ./docker-run.sh bash -c 'GTEST_FILTER="ResourceMonitor*" bash /workspace/local-build/build.sh entservices-appmanagers l1 run'`. Exit code 0 and no failures. Acceptance: L1 tests pass.
- [ ] 13.4 Run `git diff --check` with no whitespace errors, then write the completion report: modules built, tests added, exact commands, counts, warnings and anything not run. Acceptance: report is complete.
