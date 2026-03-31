# AppManager L1 Coverage Analysis and Improvement Plan

## Scope

- Coverage report analyzed: `artifacts-L1-appmanagers (2)/coverage/AppManager/*` (generated on 2026-03-12)
- Source analyzed: `AppManager/*`
- Existing L1 tests reviewed: `Tests/L1Tests/tests/test_AppManager.cpp`
- Total test cases: **128**

## Coverage Baseline (AppManager package)

- **Line coverage:** `1017 / 1145` (**88.8%**)
- **Function coverage:** `108 / 137` (**78.8%**)

Per-file breakdown:

| File | Lines Hit/Total | Line % | Functions Hit/Total | Function % |
|---|---|---|---|---|
| `AppManager.cpp` | 48 / 63 | 76.2% | 7 / 11 | 63.6% |
| `AppManager.h` | 29 / 40 | 72.5% | 10 / 16 | 62.5% |
| `AppManagerImplementation.cpp` | 565 / 611 | 92.5% | 52 / 55 | 94.5% |
| `AppManagerImplementation.h` | 26 / 28 | 92.9% | 10 / 15 | 66.7% |
| `LifecycleInterfaceConnector.cpp` | 342 / 388 | 88.1% | 20 / 20 | 100.0% |
| `LifecycleInterfaceConnector.h` | 7 / 14 | 50.0% | 9 / 18 | 50.0% |
| `Module.cpp` | 0 / 1 | 0.0% | 0 / 2 | 0.0% |

Primary low-coverage files:

1. `LifecycleInterfaceConnector.h` — 7/14 lines (50.0%), 9/18 functions (50.0%)
2. `AppManager.h` — 29/40 lines (72.5%), 10/16 functions (62.5%)
3. `AppManager.cpp` — 48/63 lines (76.2%), 7/11 functions (63.6%)
4. `LifecycleInterfaceConnector.cpp` — 342/388 lines (88.1%), 20/20 functions (100.0%)
5. `Module.cpp` — 0/1 lines (0.0%), 0/2 functions (0.0%)

---

## Functions with 0 Hits (Uncovered)

### `AppManager.cpp`

- `AppManager::Deactivated(RPC::IRemoteConnection*)` — remote-connection deactivation handler
- `AppManager::AppManager()` — constructor (ABI symbol variant)
- `AppManager::~AppManager()` (D0 variant) — ABI destructor symbol
- `AppManager::~AppManager()` (D1 variant) — ABI destructor symbol

### `AppManager.h`

- `AppManager::Notification::Activated(RPC::IRemoteConnection*)` — in-process RPC activated callback
- `AppManager::Notification::Deactivated(RPC::IRemoteConnection*)` — in-process RPC deactivated callback
- `AppManager::Notification::QueryInterface(uint32_t)` — COM-RPC interface lookup on Notification inner class
- `AppManager::Notification::Notification(AppManager*)` — constructor (ABI symbol variant)
- `AppManager::Notification::~Notification()` (D0 / D1 variants) — ABI destructor symbols

### `AppManagerImplementation.cpp`

- `AppManagerImplementation::AppManagerImplementation()` — constructor (ABI symbol variant)
- `AppManagerImplementation::~AppManagerImplementation()` (D0 variant) — ABI destructor symbol
- `AppManagerImplementation::~AppManagerImplementation()` (D1 variant) — ABI destructor symbol

### `AppManagerImplementation.h`

- `PackageManagerNotification::QueryInterface(uint32_t)` — COM-RPC interface lookup on inner notification class
- `PackageManagerNotification::PackageManagerNotification(AppManagerImplementation&)` — constructor (ABI variant)
- `PackageManagerNotification::~PackageManagerNotification()` (D0 / D1 variants) — ABI destructor symbols
- `Job::~Job()` (D0 variant) — ABI destructor symbol for worker-pool Job

### `LifecycleInterfaceConnector.h`

- `NotificationHandler::QueryInterface(uint32_t)` — COM-RPC interface lookup on inner notification class
- `NotificationHandler::NotificationHandler(LifecycleInterfaceConnector&)` — constructor (ABI variant)
- `NotificationHandler::~NotificationHandler()` (D0 / D1 variants) — ABI destructor symbols
- `AppStateChangeNotificationHandler::OnAppStateChanged(const string&, ILifecycleManager::LifecycleState, const string&)` — app-state-changed callback
- `AppStateChangeNotificationHandler::QueryInterface(uint32_t)` — COM-RPC interface lookup
- `AppStateChangeNotificationHandler::AppStateChangeNotificationHandler(LifecycleInterfaceConnector&)` — constructor (ABI variant)
- `AppStateChangeNotificationHandler::~AppStateChangeNotificationHandler()` (D0 / D1 variants) — ABI destructor symbols

### `Module.cpp`

- `ModuleBuildRef`
- `ModuleServiceMetadata`

---

## Uncovered Lines (by file)

### `AppManager.cpp` — 15 uncovered lines

| Source line | Content |
|---|---|
| L91–92 | `SYSLOG("AppManager::Initialize: object creation failed")` + error message assignment |
| L96–97 | `SYSLOG("did not provide a configuration interface")` + error message |
| L108–109 | `SYSLOG("could not be configured")` + error message |
| L115–116 | `SYSLOG("service is not valid")` + error message |
| L121 | `Deinitialize(service)` — called when `message.length() != 0` |
| L165–166 | `connection->Terminate()` / `connection->Release()` — out-of-process cleanup in `Deinitialize` |
| L182–189 | Entire `Deactivated(connection)` method body |

### `AppManager.h` — 11 uncovered lines

| Source line | Content |
|---|---|
| L56–58 | `BEGIN_INTERFACE_MAP` / `INTERFACE_ENTRY` macros for `Notification` inner class |
| L61–64 | `Activated(RPC::IRemoteConnection*)` body |
| L66–70 | `Deactivated(RPC::IRemoteConnection*)` body |
| L116 | `INTERFACE_AGGREGATE(Exchange::IAppManager, mAppManagerImpl)` |

### `AppManagerImplementation.cpp` — 46 uncovered lines

| Source lines | Content |
|---|---|
| L168 | `LOGERR("preLoadApp failed reason %s status %d")` |
| L188, L190 | `default:` case + `LOGERR("action type is invalid")` in action dispatch switch |
| L255 | `LOGERR("newState not present")` in Dispatch APP_EVENT_LIFECYCLE |
| L259 | `LOGERR("oldState not present")` in Dispatch APP_EVENT_LIFECYCLE |
| L263 | `LOGERR("errorReason not present")` in Dispatch APP_EVENT_LIFECYCLE |
| L286 | `LOGERR("mLifecycleInterfaceConnector is null, cannot kill app")` |
| L344 | `LOGERR("appId is missing or empty")` — APP_EVENT_LAUNCH_REQUEST path |
| L375 | `LOGERR("appId is missing or empty")` — APP_EVENT_UNLOADED path |
| L397–399 | `default: LOGERR("Unknown event") break` in Dispatch switch |
| L539–542 | `catch(std::system_error)` — worker thread creation failure path |
| L546 | `LOGERR("service is null")` in `Configure()` |
| L558 | `LOGERR("mCurrentservice is null")` in `createPersistentStoreRemoteStoreObject()` |
| L588 | `LOGERR("mCurrentservice is null")` in `createPackageManagerObject()` |
| L596 | `LOGERR("mPackageManagerInstallerObject is null")` |
| L634 | `LOGERR("mCurrentservice is null")` in `createStorageManagerRemoteObject()` |
| L679 | `LOGWARN("unpacked path or version empty")` |
| L799 | `LOGERR("Failed to createOrUpdate the PackageInfo")` |
| L817 | `LOGERR("PackageManager handler is null / version empty")` |
| L922 | `LOGERR("LifecycleInterfaceConnector is null")` |
| L943, L948 | `LOGERR("Failed to perform operation due to no memory")` — x2 paths |
| L1158–1159, L1164–1165 | `LOGERR("no memory")` + `error` string set in `PreloadApp` |
| L1371 | `package["lastActiveTime"] = ""` — zero-timestamp branch |
| L1598–1631 | Entire `/tmp/aipath` file-read block in `getCustomValues()` |

### `AppManagerImplementation.h` — 2 uncovered lines

| Source lines | Content |
|---|---|
| L158–159 | `BEGIN_INTERFACE_MAP(PackageManagerNotification)` / `INTERFACE_ENTRY` macros |

### `LifecycleInterfaceConnector.cpp` — 46 uncovered lines

| Source lines | Content |
|---|---|
| L90 | `LOGWARN("mCurrentservice is null")` in `createLifecycleManagerRemoteObject()` |
| L98 | `LOGWARN("Failed to create LifecycleManagerState Remote object")` |
| L179 | `LOGERR("appId is empty")` — LaunchApp path null-appId guard |
| L187–190 | Lazy-create `LifecycleManagerRemoteObject` inside `launchApp` + error log |
| L206–219 | `APP_ACTION_RESUME` path in `launchApp` — `SetTargetAppState` + update + error |
| L292 | `LOGERR("appId is empty")` — SuspendApp path |
| L300–303 | Lazy-create `LifecycleManagerRemoteObject` inside `suspendApp` + error log |
| L311 | `updateCurrentAction(APP_ACTION_SUSPEND)` |
| L336 | `LOGERR("PreLoad failed")` |
| L411–414 | Blocked-state termination path (`TerminateApp`) |
| L432 | `LOGERR("Failed to apply hibernate policy")` |
| L435–437 | Suspend-policy failure error |
| L447 | `LOGERR("AppId not found after PAUSED wait")` |
| L456 | `LOGERR("Timed out waiting for appId to reach PAUSED state")` |
| L492 | `LOGERR("AppManagerImplementation instance is null")` |
| L533 | `LOGERR("UnloadApp failed with error reason")` |
| L552 | `LOGERR("appManagerImplInstance is null")` |
| L685–688 | Lazy-create `LifecycleManagerRemoteObject` inside `killApp` + error log |
| L703–704 | `LOGERR("GetLoadedApps call Failed / empty list")` + `goto End` |
| L748 | `LOGERR("mLifecycleManagerRemoteObject is nullptr")` in `sendIntent` |
| L839 | `LOGERR("Unable to update the active state change time")` |
| L852–854 | `std::lock_guard` + `mStateChangedCV.notify_all()` — timeout-wait wake-up branch |
| L875–876 | `LOGINFO("Terminate event from plugin")` + `handleOnAppLifecycleStateChanged` |
| L913 | `LOGERR("Invalid AppManager Implementation Instance")` |
| L1016 | `LOGINFO("File exists but is not a regular file")` |
| L1026 | `LOGINFO("Filename pointer is null")` |

### `LifecycleInterfaceConnector.h` — 7 uncovered lines

| Source lines | Content |
|---|---|
| L60–61 | `BEGIN_INTERFACE_MAP` / `INTERFACE_ENTRY` for `NotificationHandler` |
| L73–76 | `AppStateChangeNotificationHandler::OnAppStateChanged()` body |
| L78–79 | `BEGIN_INTERFACE_MAP` / `INTERFACE_ENTRY` for `AppStateChangeNotificationHandler` |

### `Module.cpp` — 1 uncovered line

| Source line | Content |
|---|---|
| L22 | `MODULE_NAME_DECLARATION(BUILD_REFERENCE)` |

---

## Why These Are Not Covered (Root-Cause) + Complexity

## 1) Initialize / Deinitialize error-branch paths — **Not actionable in L1**

`AppManager.cpp` lines 91–121 and 165–166 are inside `Initialize()` and `Deinitialize()`. The constraint prohibits direct calls to `Initialize`/`Deinitialize`, so these paths cannot be covered from L1 tests.

Affected lines: **15 (AppManager.cpp)**

## 2) Deactivated / RPC Notification inner-class paths — **Not actionable in L1**

`AppManager::Deactivated()` (L182–189) and `AppManager::Notification::Activated/Deactivated` are driven by the out-of-process COM-RPC connection lifecycle — not reachable via public plugin API. `QueryInterface` variants for inner notification classes are ABI-dispatch paths, not callable directly.

Affected lines/functions: **11 lines (AppManager.h)**, **4 functions (AppManager.cpp/h)**

## 3) ABI ctor/dtor symbol variants — **Not actionable (compiler-emitted)**

`C1/D0/D1` ABI symbols for `AppManagerImplementation`, `PackageManagerNotification`, `Job`, `NotificationHandler`, `AppStateChangeNotificationHandler` appear uncovered even when source-level construction/destruction is exercised. These are compiler-emitted alternate linkage symbols.

Affected functions: **14 ABI symbols across AppManagerImplementation.h, AppManager.h, LifecycleInterfaceConnector.h**

## 4) Error branches requiring null/invalid internal state — **Medium complexity**

Several `LOGERR` branches in `AppManagerImplementation.cpp` and `LifecycleInterfaceConnector.cpp` guard against null internal objects (`mCurrentservice`, `mPackageManagerInstallerObject`, `mLifecycleInterfaceConnector`, `mLifecycleManagerRemoteObject`, `appManagerImplInstance`). These paths require triggering the relevant public API method via `createAppManagerImpl()` (partial init without full mocks) so that the internal pointer is legitimately null at call time.

Affected lines: **~18 lines in AppManagerImplementation.cpp, ~8 lines in LifecycleInterfaceConnector.cpp**

## 5) Dispatch switch default / missing-field branches — **Low–Medium complexity**

Several `LOGERR` paths in `Dispatch()` (`APP_EVENT_LIFECYCLE` missing fields, `APP_EVENT_LAUNCH_REQUEST` / `APP_EVENT_UNLOADED` empty appId, unknown event default case) require calling the internal event dispatch path with crafted JSON that omits or corrupts required keys. These can be triggered via `handleOnApp*` callbacks with malformed arguments.

Affected lines: **~9 lines in AppManagerImplementation.cpp (L255, L259, L263, L344, L375, L397–399)**

## 6) Worker-thread creation failure path — **High complexity**

`AppManagerImplementation.cpp` L539–542 (`catch(std::system_error)`) is the path triggered when `std::thread` constructor throws. Requires simulating an OS-level thread creation failure — not directly achievable in the existing mock set.

Affected lines: **4 lines**

## 7) `getCustomValues()` — `/tmp/aipath` file-read block — **Low complexity**

`AppManagerImplementation.cpp` L1598–1631 reads `/tmp/aipath` only when the file exists. Coverage requires `wraps` mock extended to `fopen`/`fclose`/`getline`, or creating a real temp file at that path and calling `GetAppProperty(appId, "runtimeConfig", value)`.

Affected lines: **~20 lines**

## 8) LifecycleInterfaceConnector lazy-reconnect and policy error branches — **Medium–High complexity**

Multiple paths in `LifecycleInterfaceConnector.cpp` require `mLifecycleManagerRemoteObject` to be null at entry time (lazy-create paths at L187, L300, L685), or for `SetTargetAppState` to succeed/fail, or for the state-machine pause/hibernate/terminate blocked-state policies to fire. These require precise `mAppInfo` state and mock responses across `LaunchApp`/`SuspendApp`/`KillApp`.

Affected lines: **~25 lines**

## 9) `AppStateChangeNotificationHandler::OnAppStateChanged` — **Medium complexity**

`LifecycleInterfaceConnector.h` L73–76: this callback is registered as `ILifecycleManager::INotification`. No existing test captures and invokes the `ILifecycleManager::INotification*` sink passed to `mLifecycleManagerMock`. Requires capturing the argument to `mLifecycleManagerMock->Register(INotification*)` and then calling `OnAppStateChanged` on it.

Affected lines: **4 lines (L73–76)**

## 10) Package-info error and lastActiveTime empty-timestamp branches — **Low complexity**

`AppManagerImplementation.cpp` L679, L799, L817, L1371: coverable by arranging `mAppInfo` to contain empty `unpackedPath`/`version`, or arranging `mPackageManagerHandlerObject` to be null during a `LaunchApp` or `GetInstalledApps` call, or pre-populating an entry with `tv_sec = 0` before calling `GetInstalledApps`.

Affected lines: **~5 lines**

---

## How to Improve Coverage with Additional Tests

## Phase 1 (Quick Wins, high value / low effort)

**1. Missing-field Dispatch branches for APP_EVENT_LIFECYCLE (`AppManagerImplementation.cpp` L255, L259, L263)**

Invoke `handleOnAppLifecycleStateChanged` via the `mLifecycleManagerStateNotification_cb` sink with a params object that omits `newState`, `oldState`, or `errorReason`. Each omission targets a distinct `LOGERR` branch.

**2. Unknown-event default case in Dispatch (`AppManagerImplementation.cpp` L397–399)**

Dispatch an event type that does not match any known `EventNames` value (e.g., a fabricated integer outside the enum range) if accessible via an internal helper, or extend an existing `OnAppInstallationStatus` test to pass an unrecognized status integer so the default Dispatch branch is reached.

**3. Empty-appId in LAUNCH_REQUEST / UNLOADED paths (L344, L375)**

The tests `HandleOnAppLaunchRequestEmptyAppId` and `HandleOnAppUnloadedEmptyAppId` must confirm the dispatched Job drains fully before `releaseResources()` is called. If these tests currently time out before the Job runs, add a `WaitForRequestStatus` guard after calling the public handler so the `LOGERR` line at L344/L375 is actually hit.

**4. `GetInstalledApps` zero-timestamp path (L1371)**

Add `GetInstalledAppsWithZeroLastActiveTime` test: populate `mPackageInfo` with a valid entry, set `lastActiveStateChangeTime.tv_sec = 0` and `tv_nsec = 0`, call `GetInstalledApps`, and verify the returned JSON contains `"lastActiveTime": ""`.

**5. Null handler / empty-version LaunchApp branch (L679, L817)**

Add a test that pre-populates `mAppInfo` with a package entry where `unpackedPath` or `version` is empty string, then calls `LaunchApp`. Verify the call returns an error and the `LOGWARN`/`LOGERR` lines are reached.

## Phase 2 (Event callback path coverage)

**6. Drive `AppStateChangeNotificationHandler::OnAppStateChanged` (LifecycleInterfaceConnector.h L73–76)**

Capture the `ILifecycleManager::INotification*` argument from `mLifecycleManagerMock->Register(...)` and call `OnAppStateChanged(appId, state, errorReason)` directly:

```cpp
Exchange::ILifecycleManager::INotification* appStateCb = nullptr;
ON_CALL(*mLifecycleManagerMock, Register(::testing::_))
    .WillByDefault(::testing::DoAll(
        ::testing::SaveArg<0>(&appStateCb),
        ::testing::Return(Core::ERROR_NONE)));
// after createResources():
ASSERT_NE(appStateCb, nullptr);
appStateCb->OnAppStateChanged(APPMANAGER_APP_ID,
    Exchange::ILifecycleManager::LifecycleState::ACTIVE, "");
```

**7. LifecycleInterfaceConnector lazy-reconnect failure paths (L187–190, L300–303, L685–688)**

Add tests that invoke `LaunchApp`, `SuspendApp`, or `KillApp` after ensuring `mLifecycleManagerRemoteObject` is null (e.g., arrange `mLifecycleManagerStateMock->Register` to fail, preventing assignment). Verify the `LOGERR("Failed to create ... Remote object")` at the respective lines is hit.

**8. `SetTargetAppState` success and failure paths in resume-app (L206–219)**

Add a `LaunchApp` test where `mAppInfo` contains an entry with `currentAction = APP_ACTION_RESUME`, and `mLifecycleManagerMock->SetTargetAppState(...)` is mocked to return `Core::ERROR_NONE` (success branch, L213–215) and separately to return an error code (failure branch, L219).

**9. `APP_ACTION_SUSPEND` update path (L311)**

Add a `SuspendApp` test that has `mLifecycleManagerMock->SetTargetAppState` return success, so that `updateCurrentAction(APP_ACTION_SUSPEND)` at L311 is reached.

**10. `UnloadApp` failure path (L533)**

Add a test where `mLifecycleManagerMock->UnloadApp` returns a non-zero error code. Verify `LOGERR("UnloadApp failed with error reason")` is reached.

## Phase 3 (Error-path and retry coverage)

**11. Blocked-state termination in `OnAppLifecycleStateChanged` (LifecycleInterfaceConnector.cpp L411–414)**

Pre-populate `mAppInfo` with an app entry in install-blocked state, then deliver an `APP_LIFECYCLE_STATE_LOADING` event via `mLifecycleManagerStateNotification_cb`. Verify `TerminateApp` is called and L413 is covered.

**12. Hibernate/suspend policy failure and PAUSED wait timeout (L432–456)**

Add a test that triggers the suspend flow but never delivers a PAUSED state change, so the `mStateChangedCV` wait times out. Verify `LOGERR("Timed out waiting for appId to reach PAUSED state")` at L456 is reached.

**13. `getCustomValues()` aipath file-read (L1598–1631)**

Create a real file at `/tmp/aipath` with three lines (apppath, runtimepath, command) before the test, call `GetAppProperty(appId, "runtimeConfig", value)`, verify the `RuntimeConfig` JSON fields are populated, then delete the file after the test. Alternatively, extend `p_wrapsImplMock` to intercept `fopen`/`fclose`/`getline` if those wraps are available.

**14. `PreloadApp` no-memory error paths (L1158–1165)**

These require `std::bad_alloc` or equivalent to be thrown during vector/string allocation. Low priority — not easily achievable without a custom allocator mock.

## Phase 4 (State-machine edge cases and filesystem-dependent branches)

**15. `GetLoadedApps` failure path (LifecycleInterfaceConnector.cpp L703–704)**

Arrange `mLifecycleManagerMock->GetLoadedApps(...)` to return a failure code or an empty/malformed list inside a `KillApp` or `GetRunningApps` call. Verify `LOGERR("GetLoadedApps call Failed")` and the `goto End` path at L703–704 are reached.

**16. State-changed CV `notify_all` branch (LifecycleInterfaceConnector.cpp L852–854)**

Arrange a `SuspendApp` test that blocks on `mStateChangedCV.wait_for(...)`, then deliver a lifecycle-state event to trigger `notify_all`. Verify the wake-up path at L852–854 is reached before the test times out. Requires coordinated delivery of an `OnAppLifecycleStateChanged` event from a background thread or via the `mLifecycleManagerStateNotification_cb` sink while `SuspendApp` is in flight.

**17. TERMINATING event → `handleOnAppLifecycleStateChanged` path (LifecycleInterfaceConnector.cpp L875–876)**

Deliver an `APP_LIFECYCLE_STATE_TERMINATING` event via `mLifecycleManagerStateNotification_cb` to an app that is currently tracked in `mAppInfo`. Verify `LOGINFO("Terminate event from plugin")` at L875 and the `handleOnAppLifecycleStateChanged` call at L876 are reached.

**18. File-path helper non-regular-file and null-filename branches (LifecycleInterfaceConnector.cpp L1016, L1026)**

- L1016: Call a public API that exercises the internal file-check helper with a path pointing to an existing directory (not a regular file), e.g., `"/tmp"`. Verify `LOGINFO("File exists but is not a regular file")` is reached.
- L1026: Pass a null filename value through the public API boundary exercising the helper. Verify the null-pointer guard at L1026 is reached.

---

## Non-Actionable Items (No Test Required)

| Item | Reason |
|---|---|
| `AppManager.cpp` L91–121 (Initialize error branches) | Requires calling `Initialize()` — excluded by L1 constraint |
| `AppManager.cpp` L165–166 (`connection->Terminate/Release`) | Requires live out-of-process RPC connection object |
| `AppManager.cpp` L182–189 (`Deactivated` method) | Driven by COM-RPC connection deactivated signal — not reachable in L1 |
| `AppManager.h` L56–70 (`Notification::Activated/Deactivated`) | Same COM-RPC lifecycle — not reachable in L1 |
| All `D0/D1/C1` ABI ctor/dtor symbol variants | Compiler-emitted alternate linkage; exercised indirectly but tracked separately by LCOV |
| `Module.cpp` L22 (`MODULE_NAME_DECLARATION`) | Static init symbol not invoked in test harness |
| `QueryInterface` on all inner COM-RPC notification classes | COM-RPC dispatch infrastructure, not directly callable from L1 test code |
| `AppManagerImplementation.cpp` L539–542 (`catch std::system_error`) | Requires OS-level thread creation failure — not simulatable with current mocks |
| `AppManagerImplementation.h` L158–159, `LifecycleInterfaceConnector.h` L60–61, L78–79 | `BEGIN_INTERFACE_MAP` / `INTERFACE_ENTRY` macro expansions — not directly invocable |

---

## Mocks from `entservices-testframework` to Reuse / Sync

The following AppManager-relevant mocks are available in `Tests/mocks` and should be treated as canonical reusable assets:

- `PackageManagerMock.h` (includes `IPackageManagerNotificationMock`)
- `LifecycleManagerMock.h` (includes `ILifecycleManagerNotificationMock` and `ILifecycleManagerStateMock`)
- `StorageManagerMock.h`
- `PersistentStoreMock.h`
- `AppManagerMock.h`
- `thunder/ServiceMock.h`
- `thunder/COMLinkMock.h`
- `WorkerPoolImplementation.h`
- `WrapsImplMock.h`

### Important observation

For the AppManager subset, the local mocks in `Tests/L1Tests/mocks` are currently effectively in sync with the testframework reference versions for:

- `LifecycleManagerMock.h`
- `PackageManagerMock.h`
- `ServiceMock.h`
- `AppManagerMock.h`

So no mandatory copy-in is needed immediately for these files.

### Recommended mock additions (not currently present as concrete behavior mocks)

To improve AppManager callback and dispatch coverage, add a concrete `IAppManager::INotification` gmock class with methods:

- `OnApplicationStateChanged(const string& appId, const string& appInstanceId, const string& appState, const string& appPrevState)`
- `OnApplicationInstallationStatus(const string& appName, const string& appVersion, const string& appStatus, const string& appCustomData)`
- `OnApplicationLaunchRequest(const string& appId, const string& intent, const string& source)`

This enables assertions on notification dispatch calls from `Dispatch(...)` and relay paths in `AppManagerImplementation`.

---

## Suggested Prioritized Test Backlog (Top 10)

1. Missing-field Dispatch branches for APP_EVENT_LIFECYCLE (`newState`, `oldState`, `errorReason` omitted)
2. Empty-appId LAUNCH_REQUEST and UNLOADED Dispatch paths (with Job drain confirmation)
3. `GetInstalledApps` zero-timestamp path (`lastActiveTime = ""`)
4. Drive `AppStateChangeNotificationHandler::OnAppStateChanged` callback via captured notification sink
5. Null handler / empty-version `LaunchApp` branches (L679, L817)
6. Lazy-reconnect failure paths in `LaunchApp` / `SuspendApp` / `KillApp` (L187, L300, L685)
7. `SetTargetAppState` success and failure paths in resume-app flow (L206–219)
8. `APP_ACTION_SUSPEND` update path in `SuspendApp` (L311)
9. `UnloadApp` failure path (L533)
10. `GetLoadedApps` failure path in `KillApp` context (L703–704)

---

## Expected Coverage Impact

- **Fast uplift target (Phase 1):** +0.8% to +1.5% line coverage by implementing the 5 quick-win tests.
- **Medium uplift target (Phase 2):** additional +1.0% to +2.0% from callback-driven notification sink tests.
- **Error-path uplift (Phase 3):** additional +0.5% to +1.5% from staged-failure and state-machine tests.
- **Hard-branch uplift (Phase 4):** additional +0.3% to +0.8% from state-machine edge cases and filesystem-conditioned tests.

Total realistic improvement with disciplined test additions: **~2.6% to 5.8%** line coverage uplift for AppManager package (from current 88.8% towards 91–94%).
