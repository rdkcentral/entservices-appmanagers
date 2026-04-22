# LifecycleManager L0 Coverage Analysis and Improvement Plan

## Scope

- Coverage report analyzed: `artifacts-L0-appmanagers/coverage/LifecycleManager/*` (generated on 2026-04-02)
- Source analyzed: `LifecycleManager/*`
- Existing L0 tests reviewed:
  - `Tests/L0Tests/LifecycleManager/LifecycleManagerTest.cpp` (test runner)
  - `Tests/L0Tests/LifecycleManager/LifecycleManager_ContextTests.cpp`
  - `Tests/L0Tests/LifecycleManager/LifecycleManager_ImplementationTests.cpp`
  - `Tests/L0Tests/LifecycleManager/LifecycleManager_ShellTests.cpp`

## Coverage Baseline (LifecycleManager package)

- **Line coverage:** `920 / 1369` (**67.2%**) — entire `LifecycleManager/` package
- **Function coverage:** `153 / 194` (**78.9%**)

Primary low-coverage files:

1. `RuntimeManagerHandler.cpp` — 23/138 lines (16.7%), 4/17 functions (23.5%)
2. `LifecycleManagerTelemetryReporting.cpp` — 16/85 lines (18.8%), 5/6 functions (83.3%)
3. `WindowManagerHandler.cpp` — 23/66 lines (34.8%), 4/10 functions (40.0%)
4. `State.cpp` — 45/87 lines (51.7%), 8/8 functions (100%)
5. `Module.cpp` — 0/1 lines, 0/2 functions (ABI artifact)

---

## Functions with 0 Hits (Uncovered)

### `RuntimeManagerHandler.cpp`

- `RuntimeManagerHandler::getRuntimeStats(appInstanceId, info)` — requires live `IRuntimeManager::GetInfo()` mock dispatch
- `RuntimeManagerHandler::run(appId, appInstanceId, launchArgs, targetState, runtimeConfigObject, errorReason)` — entire body uncovered; requires a tracked `IRuntimeManager::Run()` call to succeed
- `RuntimeManagerHandler::kill(appInstanceId, errorReason)` — requires `IRuntimeManager::Kill()`
- `RuntimeManagerHandler::terminate(appInstanceId, errorReason)` — requires `IRuntimeManager::Terminate()`
- `RuntimeManagerHandler::suspend(appInstanceId, errorReason)` — requires `IRuntimeManager::Suspend()`
- `RuntimeManagerHandler::resume(appInstanceId, errorReason)` — requires `IRuntimeManager::Resume()`
- `RuntimeManagerHandler::hibernate(appInstanceId, errorReason)` — requires `IRuntimeManager::Hibernate()`
- `RuntimeManagerHandler::wake(appInstanceId, state, errorReason)` — requires `IRuntimeManager::Wake()`
- `RuntimeManagerHandler::RuntimeManagerNotification::OnStarted(appInstanceId)` — requires real COM-RPC notification callback from mock `IRuntimeManager`
- `RuntimeManagerHandler::RuntimeManagerNotification::OnTerminated(appInstanceId)` — same dependency
- `RuntimeManagerHandler::RuntimeManagerNotification::OnFailure(appInstanceId, error)` — same dependency
- `RuntimeManagerHandler::RuntimeManagerNotification::OnStateChanged(appInstanceId, state)` — same dependency
- `RuntimeManagerHandler::onEvent(data)` — dispatches to IEventHandler; only reachable when a `RuntimeManagerNotification` callback fires

### `WindowManagerHandler.cpp`

- `WindowManagerHandler::WindowManagerNotification::OnUserInactivity(minutes)` — requires real WM notification callback
- `WindowManagerHandler::WindowManagerNotification::OnDisconnected(client)` — same dependency
- `WindowManagerHandler::WindowManagerNotification::OnReady(client)` — same dependency
- `WindowManagerHandler::onEvent(data)` — dispatches to IEventHandler; only reachable when WM notification fires
- `WindowManagerHandler::renderReady(appInstanceId, isReady)` — requires `IRDKWindowManager::RenderReady()` to be called
- `WindowManagerHandler::enableDisplayRender(appInstanceId, render)` — requires `IRDKWindowManager::EnableDisplayRender()`

### `LifecycleManagerImplementation.cpp`

- `LifecycleManagerImplementation::SpawnApp(...)` — full function body (0 hits): context allocation, `setApplicationLaunchParams`, `semaphore wait`, `RequestHandler::getInstance()->launch()` success path
- `LifecycleManagerImplementation::SetTargetAppState(...)` success path — context-found branch: entire `switch(targetLifecycleState)` block, `updateState()` call
- `LifecycleManagerImplementation::UnloadApp(...)` success path — context-found branch: `setRequestType`, `setTargetLifecycleState`, `setApplicationKillParams`, `RequestHandler::terminate()`
- `LifecycleManagerImplementation::KillApp(...)` success path — context-found branch: same pattern as above
- `LifecycleManagerImplementation::CloseApp(...)` success path — `KillApp()` cascade + `KILL_AND_RUN`/`KILL_AND_ACTIVATE` re-spawn branch + `SpawnApp()` call
- `LifecycleManagerImplementation::GetLoadedApps(verbose=true)` inner path — `runtimeManagerHandler != nullptr` block: `getRuntimeStats()` call, `appData["runtimeStats"]` assignment, failure `printf`
- `LifecycleManagerImplementation::Configure(...)` failure path — `initialize()` returns false: `printf("unable to configure lifecyclemanager")` lines
- `LifecycleManagerImplementation::onRuntimeManagerEvent(data)` — 0 hits: dispatches `LIFECYCLE_MANAGER_EVENT_RUNTIME` event; not fired in current mock service path
- `LifecycleManagerImplementation::onWindowManagerEvent(data)` — 0 hits: dispatches `LIFECYCLE_MANAGER_EVENT_WINDOW` event
- `LifecycleManagerImplementation::onStateChangeEvent(data)` — 0 hits: dispatches `LIFECYCLE_MANAGER_EVENT_APPSTATECHANGED` event
- `LifecycleManagerImplementation::handleRuntimeManagerEvent()` failure branch — `!(terminated && stateUpdated)` path: `LOGERR(...)` on unexpected termination handling failure

### `LifecycleManagerTelemetryReporting.cpp`

- `LifecycleManagerTelemetryReporting::reportTelemetryDataOnStateChange(context, data)` — entire body after early-return guard: gated behind `Utils::isTelemetryMetricsEnabled()` which returns `false` in L0 environment
  - `ensureTelemetryClient()` call and failure path
  - Context null check / appId empty check branches
  - All six `switch(requestType)` case bodies: `REQUEST_TYPE_LAUNCH`, `REQUEST_TYPE_TERMINATE`, `REQUEST_TYPE_SUSPEND`, `REQUEST_TYPE_RESUME`, `REQUEST_TYPE_HIBERNATE`, `default`
  - Telemetry `record()` / `publish()` calls
- `LifecycleManagerTelemetryReporting::initialize()` error path — `initializeTelemetryClient()` failure branch: `LOGERR("Failed to create TelemetryMetricsObject")`

### `State.cpp`

- `InitializingState::handle()` — `runtimeManagerHandler != nullptr` success branch: `runtimeManagerHandler->run(...)`, `setPendingEventName("onAppRunning")`, `setPendingStateTransition(true)`
- `PausedState::handle()` — `SUSPENDED` → resume path: `WindowManagerHandler::enableDisplayRender(appInstanceId, true)` call
- `ActiveState::handle()` — `windowManagerHandler != nullptr` branch: `renderReady()`, pending event name assignment (`onFirstFrame`), `pendingStateTransition` flag set; also `return false` error path
- `SuspendedState::handle()` — `runtimeManagerHandler != nullptr` branch (both sub-branches):
  - `HIBERNATED` source: `runtimeManagerHandler->wake(appInstanceId, SUSPENDED, errorReason)`
  - Other source: `windowManagerHandler->enableDisplayRender(appInstanceId, false)` call
- `HibernatedState::handle()` — `runtimeManagerHandler != nullptr` branch: `runtimeManagerHandler->hibernate(appInstanceId, errorReason)` call
- `TerminatingState::handle()` — `runtimeManagerHandler != nullptr` branch: `kill()`/`terminate()` routing by `killParams.mForce`, `setPendingEventName("onAppTerminating")`, `setPendingStateTransition(true)`
- `handleStateChangeEvent()` — null context guard within iterator loop: `continue` branch (line 694)

### `StateTransitionHandler.cpp`

- `~StateTransitionHandler()` — 0 hits: singleton, never deleted in tests
- Request processing inner loop body — `gRequests.size() > 0` branch (never reached because `addRequest()` calls are not yet paired with a running loop in test fixtures):
  - Null request guard: `!request` → erase and continue
  - `StateHandler::changeState(*request, errorReason)` call
  - Erase-after-process line
  - `!success` error branch: `LOGERR(...)`, `break`

### `LifecycleManager.cpp`

- `LifecycleManager::Deactivated(RPC::IRemoteConnection*)` — RPC connection deactivation lifecycle, not reachable via public API in L0
- ABI ctor/dtor symbol variants (`C2`/`D0`/`D1` forms generated by the compiler)

### `RequestHandler.cpp`

- `~RequestHandler()` — 0 hits: singleton is never deleted via direct destructor call in tests
- `RequestHandler::initialize()` error paths:
  - `mRuntimeManagerHandler->initialize()` returns false: `printf("unable to initialize with runtimemanager")`, early return
  - `mWindowManagerHandler->initialize()` returns false: `printf("unable to initialize with windowmanager")`, early return

### `Module.cpp`

- `ModuleBuildRef`
- `ModuleServiceMetadata`

---

## Uncovered Lines (by file)

> Full detail is in the LCOV pages. Key ranges are summarized here.

- `RuntimeManagerHandler.cpp`: **115 lines**
  - `getRuntimeStats()`: `GetInfo()` call + error branch (3 lines)
  - `run()`: entire 57-line function body — Firebolt endpoint env var composition, iterator creation, `mRuntimeManager->Run()` call, error branch
  - `kill()` / `terminate()` / `suspend()` / `resume()` / `hibernate()` / `wake()`: 6 small methods × ~5 lines each (~35 lines)
  - `RuntimeManagerNotification::OnStarted()` / `OnTerminated()` / `OnFailure()` / `OnStateChanged()`: 4 callbacks × ~5 lines each (~20 lines)
  - `onEvent()`: `mEventHandler->onRuntimeManagerEvent()` dispatch (3 lines)

- `WindowManagerHandler.cpp`: **43 lines**
  - `OnUserInactivity()` / `OnDisconnected()` / `OnReady()`: 3 notification callbacks × ~6 lines each (~18 lines)
  - `onEvent()`: `mEventHandler->onWindowManagerEvent()` dispatch (3 lines)
  - `renderReady()` body: `RenderReady()` call + error print (6 lines)
  - `enableDisplayRender()` body: `EnableDisplayRender()` call + error print (7 lines)
  - Register/unregister failure branches in `initialize()` and `terminate()` (~3 lines each)

- `LifecycleManagerTelemetryReporting.cpp`: **69 lines**
  - Entire `reportTelemetryDataOnStateChange()` post-guard body (69 lines) — telemetry client check, appId validation, `switch(requestType)` with 6 case bodies, `record()` / `publish()` calls

- `LifecycleManagerImplementation.cpp`: **106 lines**
  - `SpawnApp()`: ~35 lines
  - `SetTargetAppState()` success path: ~22 lines (switch on 5 lifecycle states)
  - `UnloadApp()` / `KillApp()` success paths: ~15 lines each
  - `CloseApp()` success path: ~12 lines
  - `GetLoadedApps(verbose=true)` runtime stats block: ~7 lines
  - `onRuntimeManagerEvent()` / `onWindowManagerEvent()` / `onStateChangeEvent()`: 3 × 3 lines = 9 lines
  - `Configure()` failure path: 2 lines
  - `handleRuntimeManagerEvent()` failure branch: 1 line

- `State.cpp`: **42 lines**
  - `InitializingState::handle()` success branch: 6 lines
  - `PausedState::handle()` SUSPENDED→resume branch: 5 lines
  - `ActiveState::handle()` WM branch: 10 lines (renderReady, pending event name, return false error path)
  - `SuspendedState::handle()` RTM branch: 11 lines (both HIBERNATED wake and ACTIVE→SUSPENDED display render paths)
  - `HibernatedState::handle()` RTM branch: 2 lines
  - `TerminatingState::handle()` RTM branch: 9 lines

- `StateTransitionHandler.cpp`: **13 lines**
  - `~StateTransitionHandler()`: 1 line (ABI)
  - Inner request processing loop: 12 lines

- `LifecycleManager.cpp`: **5 lines**
  - `Deactivated()` body: 3 lines
  - ABI ctor/dtor variants: 2 lines

- `RequestHandler.cpp`: **8 lines**
  - `~RequestHandler()`: 1 line (ABI)
  - `initialize()` RTM failure path: 3 lines
  - `initialize()` WM failure path: 3 lines
  - ABI variant: 1 line

---

## Why These Are Not Covered (Root-Cause) + Complexity

### 1) IPC handler dispatch path — live notification callbacks required (**Not actionable in L0 without notification injection**)

- `RuntimeManagerHandler::RuntimeManagerNotification::On*()` callbacks (4 functions, ~20 lines): These are COM-RPC notification callbacks that would only run if the `FakeRuntimeManager` mock triggered `Notification::OnStarted()` / `OnTerminated()` / `OnFailure()` / `OnStateChanged()` on a registered listener. The current `FakeRuntimeManager` stub in `LifecycleManagerServiceMock.h` records that `Register()` was called, but it does not retain the notification pointer or invoke callbacks.
- `WindowManagerHandler::WindowManagerNotification::On*()` callbacks (3 functions, ~18 lines): Same situation for `FakeWindowManager`; the current stub tracks registration activity but does not retain the listener pointer or fire notifications.
- Both `onEvent()` dispatchers: unreachable until the fakes are extended to support notification injection (or equivalent callback triggering) so the respective `On*()` callbacks can be exercised.

### 2) IRuntimeManager / IRDKWindowManager operation paths — live method dispatch (**Medium complexity**)

- `RuntimeManagerHandler::run()`, `kill()`, `terminate()`, `suspend()`, `resume()`, `hibernate()`, `wake()` (7 functions, ~100+ lines): The current `FakeRuntimeManager::Run()`/`Terminate()`/`Kill()`/etc. return `Core::ERROR_NONE` but are stub no-ops; the calls are never triggered from `LifecycleManagerImplementation` because `SpawnApp()` is never called (no test covers the success path of SpawnApp).
- `WindowManagerHandler::renderReady()`, `enableDisplayRender()` (2 functions, ~13 lines): Never called because no test follows a lifecycle path that reaches `ActiveState::handle()` or `SuspendedState::handle()` with a live `WindowManagerHandler`.

### 3) IEventHandler interface forwarding through IPC handlers (**Medium complexity**)

- `onRuntimeManagerEvent()`, `onWindowManagerEvent()`, `onStateChangeEvent()` in `LifecycleManagerImplementation` (3 functions, 9 lines): These are IEventHandler interface methods invoked by `RuntimeManagerHandler::onEvent()` and `WindowManagerHandler::onEvent()`. They are unreachable until notification injection (Category 1) is solved.

### 4) Full application lifecycle operation paths (**High complexity**)

- `SpawnApp()` (35 lines): Requires a full launch flow — `RequestHandler::launch()` → `StateTransitionHandler::addRequest()` → `StateHandler::changeState()` → each intermediate state `handle()` — plus a semaphore-wait mechanism (`mReachedLoadingStateSemaphore`). Currently no test inserts an `ApplicationContext` into `mLoadedApplications` and then calls `SpawnApp()`/`RequestHandler::launch()`.
- `SetTargetAppState()` / `UnloadApp()` / `KillApp()` success paths: All require a context found in `mLoadedApplications` (currently tests always pass an unknown appInstanceId so context = nullptr → error return).
- `CloseApp()` with `KILL_AND_RUN`/`KILL_AND_ACTIVATE`: Requires a loaded app context + `KillApp()` success + re-spawn via `SpawnApp()`.

### 5) Telemetry gated by feature flag (**Low to Medium complexity**)

- Entire `reportTelemetryDataOnStateChange()` body (69 lines): Gated by `Utils::isTelemetryMetricsEnabled()` which returns `false` in the L0 environment because `TelemetryMetrics` plugin is not present. All telemetry switch-case branches are unreachable in L0 without mocking the telemetry enablement check.

### 6) State machine inner dispatch loop (**Medium complexity**)

- `StateTransitionHandler` inner request processing loop (12 lines): `addRequest()` does successfully enqueue items and `sem_post()` unblocks the thread, but existing tests do not wait for the request-processing thread to drain the queue before `terminate()` is called (`sem_post()` in `terminate()` unblocks and sets `sRunning = false`, so the loop exits before picking up queued items with 0-ms race window).

### 7) ABI symbol artifacts (**Low priority / non-actionable**)

- `Module.cpp` declarations (`ModuleBuildRef`, `ModuleServiceMetadata`): Generated by `MODULE_NAME_DECLARATION(BUILD_REFERENCE)` — never directly instantiated in unit tests.
- `~RequestHandler()`, `~StateTransitionHandler()`: Singleton destructors never called explicitly during test teardown.
- `LifecycleManager::Deactivated()`: Plugin lifecycle hook dispatched only by the WPEFramework COM-RPC runtime on connection drop — not reachable via L0 public API.

---

## How to Improve Coverage with Additional Tests

### Phase 1 (Quick Wins — high value / low effort)

1. **Inject RuntimeManager notification callbacks via mock**
   - Extend `FakeRuntimeManager` in `LifecycleManagerServiceMock.h` to store registered `IRuntimeManager::INotification*` pointer and expose a helper `fireOnStarted()` / `fireOnTerminated()` / `fireOnFailure()` / `fireOnStateChanged()`.
   - Calling these in a test after `RequestHandler::initialize()` fires the full `RuntimeManagerNotification::On*()` → `onEvent()` → `IEventHandler::onRuntimeManagerEvent()` → `LifecycleManagerImplementation::handleRuntimeManagerEvent()` chain.
   - Unlocks ~135 uncovered lines across `RuntimeManagerHandler.cpp` (notifications + `onEvent`) and all `onRuntimeManagerEvent()`-driven branches in `LifecycleManagerImplementation.cpp`.

2. **Inject WindowManager notification callbacks via mock**
   - Same pattern: extend `FakeWindowManager` with `fireOnUserInactivity()`, `fireOnDisconnected()`, `fireOnReady()`.
   - Unlocks `WindowManagerNotification` callbacks, `WindowManagerHandler::onEvent()`, and `LifecycleManagerImplementation::handleWindowManagerEvent()` branches.

3. **Cover `SpawnApp()` with a loaded application context**
   - Directly pre-populate `mLoadedApplications` via a helper (or expose a `setContext()` test hook), then call `SpawnApp()` with a matching appId to exercise the existing-context branch.
   - Alternatively, add a test that calls `LifecycleManagerImplementation::Configure()` with a mock service that provides both RTM + WM, then calls `SpawnApp()` and waits on `mReachedLoadingStateSemaphore`.
   - Unlocks 35 lines in `SpawnApp()`, plus downstream `SetTargetAppState()`, `UnloadApp()`, `KillApp()` by reusing the inserted context.

4. **Cover `SetTargetAppState()` and `UnloadApp()` / `KillApp()` success paths**
   - After inserting a context into `mLoadedApplications`, call `SetTargetAppState(appInstanceId, PAUSED/SUSPENDED/HIBERNATED/ACTIVE, "")` to exercise the switch branches.
   - Call `UnloadApp(appInstanceId, ...)` and `KillApp(appInstanceId, ...)` with a found context.
   - Unlocks ~50 additional lines across these three functions.

### Phase 2 (State Machine Coverage)

5. **Force `StateTransitionHandler` to process queued requests**
   - After `addRequest()`, insert a bounded `usleep()` + polling loop, or replace the `sem_wait` in the test thread with a `sem_timedwait`, to give the processing thread a deterministic window to drain `gRequests`.
   - Alternatively, expose a `drainForTest()` method guarded by `#ifdef UNIT_TEST` that synchronously processes all pending requests from the calling thread.
   - Unlocks the inner loop body (~12 lines) including `StateHandler::changeState()` dispatch, null-request guard, and error branch.

6. **Cover `State.cpp` runtime-dependent branches via RTM/WM mock dispatch**
   - Once notification injection (Phase 1, items 1–2) is in place, the state machine will naturally traverse `InitializingState::handle()` (calls `runtimeManagerHandler->run()`), `TerminatingState::handle()` (calls `kill()`/`terminate()`), etc.
   - For `ActiveState::handle()` with `windowManagerHandler != nullptr`: make `FakeWindowManager::RenderReady()` return `isReady = false`, then ensure the `onFirstFrame` pending path is covered.
   - For `SuspendedState::handle()` and `PausedState::handle()`: exercise resume-from-suspend / enable-display paths by starting in ACTIVE → SUSPENDED → PAUSED state sequence.
   - Unlocks ~42 lines in `State.cpp`.

7. **Cover `GetLoadedApps(verbose=true)` with a loaded context + live RTM handler**
   - After Phase 1 (RTM accessible), call `GetLoadedApps(true, apps)` with at least one entry in `mLoadedApplications`.
   - Make `FakeRuntimeManager::GetInfo()` return a JSON blob so the success path (`runtimeStats` appended to appData) is exercised.
   - For the failure path (getRuntimeStats returns false), make `GetInfo()` return an error code.
   - Unlocks 7 lines.

### Phase 3 (Telemetry and Error Paths)

8. **Cover `LifecycleManagerTelemetryReporting::reportTelemetryDataOnStateChange()`**
   - Mock `Utils::isTelemetryMetricsEnabled()` to return `true` (or stub the function in a test-only translation unit).
   - Ensure `TelemetryMetrics` plugin mock is available via `QueryInterfaceByCallsign("TelemetryMetrics")`.
   - Exercise each `switch(requestType)` case by setting the appropriate `context->requestType` before triggering `handleStateChangeEvent()`.
   - Unlocks 69 lines.

9. **Cover `LifecycleManagerImplementation::Configure()` failure path**
   - Pass a service mock for which `RequestHandler::initialize()` returns `false` (e.g., make `QueryInterfaceByCallsign("org.rdk.RuntimeManager")` return `nullptr`).
   - Unlocks 2 lines.

10. **Cover `RequestHandler::initialize()` error paths**
    - Test with a mock service where `QueryInterfaceByCallsign("org.rdk.RuntimeManager")` returns `nullptr` → RTM initialization failure path.
    - Test with a mock service where RTM succeeds but WM returns `nullptr` → WM initialization failure path.
    - Unlocks 6 lines.

11. **Cover `handleRuntimeManagerEvent()` unexpected-termination failure branch**
    - Set context to a non-TERMINATING state, inject `onTerminated` event via mock notification callback.
    - Make `RequestHandler::terminate()` or `RequestHandler::updateState()` return `false` (e.g., by configuring mock state handler to fail).
    - Unlocks the `LOGERR(...)` branch for `!(terminated && stateUpdated)`.

---

## Fakes / Stubs Available in L0 Tests

The following L0-specific test infrastructure exists in `Tests/L0Tests/LifecycleManager/`:

- `Tests/L0Tests/LifecycleManager/LifecycleManagerServiceMock.h` — service/manager stubs for LifecycleManager L0 tests:
  - `LcmServiceMock` — `PluginHost::IShell` stub that provides `FakeRuntimeManager` and `FakeWindowManager` via `QueryInterfaceByCallsign()`
  - `FakeRuntimeManager` — `IRuntimeManager` stub with in-memory notification registration; currently stores registered `INotification*` but never calls it back
  - `FakeWindowManager` — `IRDKWindowManager` stub; same pattern
- `Tests/L0Tests/common/L0Expect.hpp` — lightweight assertion helpers (`ExpectTrue`, `ExpectEqU32`)
- `Tests/L0Tests/common/L0Bootstrap.hpp` — WPEFramework worker-pool / messaging initialisation guard

### Recommended stub additions (not currently present)

To improve `RuntimeManagerHandler` and `WindowManagerHandler` notification coverage, add the following to `FakeRuntimeManager` and `FakeWindowManager`:

**`FakeRuntimeManager` additions:**
```cpp
// Simulate a notification callback being fired by the runtime
void fireOnStarted(const std::string& appInstanceId)    { if (mNotification) mNotification->OnStarted(appInstanceId); }
void fireOnTerminated(const std::string& appInstanceId) { if (mNotification) mNotification->OnTerminated(appInstanceId); }
void fireOnFailure(const std::string& appInstanceId, const std::string& error) { if (mNotification) mNotification->OnFailure(appInstanceId, error); }
void fireOnStateChanged(const std::string& appInstanceId, Exchange::IRuntimeManager::RuntimeState state) { if (mNotification) mNotification->OnStateChanged(appInstanceId, state); }
```

**`FakeWindowManager` additions:**
```cpp
void fireOnUserInactivity(double minutes)               { if (mNotification) mNotification->OnUserInactivity(minutes); }
void fireOnDisconnected(const std::string& client)      { if (mNotification) mNotification->OnDisconnected(client); }
void fireOnReady(const std::string& appInstanceId)      { if (mNotification) mNotification->OnReady(appInstanceId); }
```

---

## Suggested Prioritized Test Backlog (Top 10)

1. RTM notification injection — `fireOnStarted` / `fireOnTerminated` / `fireOnStateChanged` callbacks dispatched through `RuntimeManagerHandler::RuntimeManagerNotification`
2. WM notification injection — `fireOnUserInactivity` / `fireOnDisconnected` / `fireOnReady` callbacks dispatched through `WindowManagerHandler::WindowManagerNotification`
3. `onRuntimeManagerEvent()` / `onWindowManagerEvent()` / `onStateChangeEvent()` in `LifecycleManagerImplementation` via mock event handler binding
4. `SpawnApp()` success path — pre-insert context or full launch flow; exercise semaphore wait / `mReachedLoadingStateSemaphore`
5. `SetTargetAppState()` / `UnloadApp()` / `KillApp()` success paths with pre-inserted context
6. `StateTransitionHandler` request processing loop — deterministic drain after `addRequest()`
7. `State.cpp` RTM/WM branches — `InitializingState`, `TerminatingState`, `SuspendedState`, `HibernatedState`, `ActiveState`, `PausedState` via mock dispatch
8. `GetLoadedApps(verbose=true)` — with context in list + RTM mock returning stats
9. `LifecycleManagerTelemetryReporting::reportTelemetryDataOnStateChange()` — with `isTelemetryMetricsEnabled()` stubbed to `true`
10. `RequestHandler::initialize()` error paths — RTM null and WM null failure branches

---

## Expected Coverage Impact

- **Fast uplift target (Phase 1):** +8% to +12% line coverage by implementing RTM + WM notification injection, `SpawnApp()` coverage, and success paths for `SetTargetAppState()`/`UnloadApp()`/`KillApp()`.
- **Medium uplift target (Phase 2):** additional +5% to +8% with `StateTransitionHandler` drain loop tests and `State.cpp` RTM/WM state branches.
- **Hard uplift target (Phase 3):** additional +4% to +6% from telemetry mock, `Configure()` failure, and `RequestHandler::initialize()` error paths.

Total realistic improvement with disciplined test additions: **~17% to 26%** line coverage uplift for the LifecycleManager package, moving the overall line coverage from the current **67.2%** toward **84%–93%**, with the remaining gap attributable to ABI symbol artifacts (`Module.cpp`, destructor variants) and singleton lifetime paths that are structurally non-actionable in L0.
