# RDKWindowManager L1 Uncovered Analysis

## Scope and Inputs
- Coverage report analyzed:
  - `l1build/coverage/RDKWindowManager/index.html`
  - `l1build/coverage/RDKWindowManager/RDKWindowManager.cpp.gcov.html`
  - `l1build/coverage/RDKWindowManager/RDKWindowManagerImplementation.cpp.gcov.html`
  - function tables (`*.func.html`)
- Source analyzed:
  - `RDKWindowManager/RDKWindowManager.cpp`
  - `RDKWindowManager/RDKWindowManagerImplementation.cpp`
  - `RDKWindowManager/Module.cpp`
- Current L1 tests analyzed:
  - `Tests/L1Tests/tests/test_RDKWindowManager.cpp`

## Coverage Snapshot (Current)
From `l1build/coverage/RDKWindowManager/index.html`:
- Overall plugin folder line coverage: 18.2% (163/896)
- Overall plugin folder function coverage: 23.9% (26/109)
- `RDKWindowManagerImplementation.cpp` line coverage: 14.5% (110/760)
- `RDKWindowManagerImplementation.cpp` function coverage: 22.1% (15/68)

## Why Coverage is Low
Primary reason: most tests in `test_RDKWindowManager.cpp` are wrapper-level JSONRPC tests that mock `IRDKWindowManager` (`WindowManagerMock`) and do not execute implementation internals.

In `RDKWindowManagerTest::SetUp`, plugin `Root<IRDKWindowManager>` is mocked to return `WindowManagerMock`, so calls such as `createDisplay`, `addKeyIntercept`, `generateKey`, etc. stop at the interface boundary and do not execute `RDKWindowManagerImplementation.cpp` logic.

Secondary reason: implementation tests currently cover only a small subset (`GetApps`, `EnableInputEvents` wildcard/invalid JSON, `KeyRepeatConfig` nominal/invalid input, `SetFocus`, `GetVisibility`, `RenderReady`, `GetLastKeyInfo`, VNC start/stop).

Third reason: difficult async/lifecycle paths are not exercised:
- `Initialize`/`Deinitialize` thread and environment setup
- event listener registration/dispatch
- screenshot async pipeline (`GetScreenshot` + `notifyScreenshotComplete`)
- bulk key parsing paths

## Uncovered Functions (0 Hit Count)

### Module and Plugin Wrapper
- `Module.cpp:20` `ModuleBuildRef`
- `RDKWindowManager.cpp:169` `RDKWindowManager::Information() const`
- `RDKWindowManager.cpp:174` `RDKWindowManager::Deactivated(RPC::IRemoteConnection*)`

### RDKWindowManagerImplementation.cpp (0 hit)
- `59` `getKeyFlag(std::string)`
- `88` `CreateDisplayRequest::CreateDisplayRequest(...)`
- `109` `CreateDisplayRequest::~CreateDisplayRequest()`
- `136` `isClientExists(std::string)`
- `179` `toLower(const std::string&)`
- `186` `RDKWindowManagerImplementation::Initialize(PluginHost::IShell*)`
- `221` lambda inside `Initialize` (`shellThread` worker)
- `308` `RDKWindowManagerImplementation::Deinitialize(PluginHost::IShell*)`
- `379` `RDKWindowManagerImplementation::Register(INotification*)`
- `401` `RDKWindowManagerImplementation::Unregister(INotification*)`
- `428` `RdkWindowManagerListener::onUserInactive(double)`
- `442` `RdkWindowManagerListener::onApplicationDisconnected(const std::string&)`
- `456` `RdkWindowManagerListener::onReady(const std::string&)`
- `470` `RdkWindowManagerListener::onApplicationConnected(const std::string&)`
- `484` `RdkWindowManagerListener::onApplicationVisible(const std::string&)`
- `498` `RdkWindowManagerListener::onApplicationHidden(const std::string&)`
- `512` `RdkWindowManagerListener::onApplicationFocus(const std::string&)`
- `526` `RdkWindowManagerListener::onApplicationBlur(const std::string&)`
- `540` `RDKWindowManagerImplementation::dispatchEvent(Event, const JsonValue&)`
- `545` `RDKWindowManagerImplementation::Dispatch(Event, JsonValue)`
- `821` `RDKWindowManagerImplementation::AddKeyIntercept(const string&)`
- `887` `RDKWindowManagerImplementation::AddKeyIntercepts(const string&, const string&)`
- `931` `RDKWindowManagerImplementation::RemoveKeyIntercept(const string&, uint32_t, const string&)`
- `972` `RDKWindowManagerImplementation::AddKeyListener(const string&)`
- `1025` `RDKWindowManagerImplementation::RemoveKeyListener(const string&)`
- `1078` `RDKWindowManagerImplementation::InjectKey(uint32_t, const string&)`
- `1113` `RDKWindowManagerImplementation::GenerateKey(const string&, const string&)`
- `1153` `RDKWindowManagerImplementation::EnableInactivityReporting(bool)`
- `1175` `RDKWindowManagerImplementation::SetInactivityInterval(uint32_t)`
- `1196` `RDKWindowManagerImplementation::ResetInactivityTime()`
- `1220` `RDKWindowManagerImplementation::EnableKeyRepeats(bool)`
- `1249` `RDKWindowManagerImplementation::GetKeyRepeatsEnabled(bool&) const`
- `1278` `RDKWindowManagerImplementation::IgnoreKeyInputs(bool)`
- `1484` `RDKWindowManagerImplementation::SetVisible(const string&, bool)`
- `1569` `RDKWindowManagerImplementation::createDisplay(...)`
- `1610` `RDKWindowManagerImplementation::enableInactivityReporting(bool)`
- `1623` `RDKWindowManagerImplementation::addKeyIntercept(...)`
- `1637` `RDKWindowManagerImplementation::addKeyIntercepts(...)`
- `1685` `RDKWindowManagerImplementation::removeKeyIntercept(...)`
- `1699` `RDKWindowManagerImplementation::addKeyListeners(...)`
- `1783` `RDKWindowManagerImplementation::removeKeyListeners(...)`
- `1855` `RDKWindowManagerImplementation::injectKey(...)`
- `1869` `RDKWindowManagerImplementation::generateKey(...)`
- `1929` `RDKWindowManagerImplementation::setInactivityInterval(uint32_t)`
- `1950` `RDKWindowManagerImplementation::resetInactivityTime()`
- `2012` `RDKWindowManagerImplementation::EnableDisplayRender(const string&, bool)`
- `2080` `RDKWindowManagerImplementation::SetZOrder(const string&, int32_t)`
- `2119` `RDKWindowManagerImplementation::GetZOrder(const string&, int32_t&)`
- `2220` `RDKWindowManagerImplementation::GetScreenshot()`
- `2241` `RDKWindowManagerImplementation::notifyScreenshotComplete(bool)`

## Uncovered Line Regions

### RDKWindowManager.cpp uncovered lines
- 88-89, 101-102, 107-108, 113
- 154-155
- 171, 173, 176, 178, 180-181, 183

### Module.cpp uncovered lines
- 22

### RDKWindowManagerImplementation.cpp uncovered line clusters (major)
The implementation has many sparse misses; key contiguous clusters are:
- 61-117 (key flag + create display request ctor/dtor)
- 129-185 (lock fallback + `isClientExists` + `toLower`)
- 188-307 (`Initialize` + thread path)
- 310-422 (`Deinitialize`, `Register`, `Unregister`)
- 427-547 (listener callbacks + `dispatchEvent`/`Dispatch`)
- 805-914 (key intercept APIs)
- 922-1147 (remove/listener/inject/generate wrappers)
- 1155-1298 (inactivity/key-repeat/input-control wrappers)
- 1318-1544 (partial misses in `EnableInputEvents`, `KeyRepeatConfig`, `SetVisible`, `GetVisibility`)
- 1571-1970 (core internal helpers: create/add/remove/generate/set/reset)
- 2014-2147 (`EnableDisplayRender`, `SetZOrder`, `GetZOrder`)
- 2222-2251 (screenshot completion handling)

## Root Cause and Complexity by Area

### 1) Lifecycle and threading paths
- Functions: `Initialize`, lambda worker, `Deinitialize`
- Reason uncovered:
  - Tests do not call implementation lifecycle directly in isolation.
  - Thread loop depends on global `sRunning`, framerate, semaphores, and compositor behavior.
- Complexity: High
  - Async behavior, global mutable state, locking, environment variable handling (`WAYLAND_DISPLAY`), and teardown ordering.

### 2) Notification registration and event fanout
- Functions: `Register`, `Unregister`, listener callbacks, `dispatchEvent`, `Dispatch`, `notifyScreenshotComplete`
- Reason uncovered:
  - Current tests avoid event callback testing.
  - No notification mock assertions wired against implementation event dispatch.
- Complexity: Medium-High
  - Requires deterministic worker-pool dispatch and callback ordering checks.

### 3) Input/key handling pipeline
- Functions: `AddKeyIntercept*`, `RemoveKeyIntercept*`, `AddKeyListener*`, `RemoveKeyListener*`, `InjectKey`, `GenerateKey`, helper methods (`getKeyFlag`, `add*/remove*/injectKey/generateKey`)
- Reason uncovered:
  - JSONRPC tests only validate wrapper forwarding to mocked interface.
  - Implementation parsing/validation branches are not directly exercised.
- Complexity: Medium
  - Many validation combinations (missing fields, malformed arrays, wildcard `*`, `keyCode` vs `nativeKeyCode`, modifiers fold).

### 4) Display and visibility controls
- Functions: `CreateDisplay`, `createDisplay`, `SetVisible`, `EnableDisplayRender`, `SetZOrder`, `GetZOrder`, plus helper `isClientExists`
- Reason uncovered:
  - Existing implementation tests do not assert these operations.
  - `createDisplay` path includes semaphore and queue handling through worker thread.
- Complexity: Medium-High
  - Requires controlled orchestration of request queue and compositor interactions.

### 5) Inactivity and key repeat controls
- Functions: `EnableInactivityReporting`, `SetInactivityInterval`, `ResetInactivityTime`, `EnableKeyRepeats`, `GetKeyRepeatsEnabled`, `IgnoreKeyInputs`, and internal `enableInactivityReporting/setInactivityInterval/resetInactivityTime`
- Reason uncovered:
  - Only small subset currently tested.
  - No pre-operation expectations for these compositor calls in most APIs.
- Complexity: Low-Medium
  - Mostly deterministic if mocks cover each compositor API call.

### 6) Screenshot path
- Functions: `GetScreenshot`, `notifyScreenshotComplete`, and worker-side screenshot branch
- Reason uncovered:
  - Async event path not tested.
  - Requires synchronization and callback expectation for `OnScreenshotComplete`.
- Complexity: High
  - Thread + shared buffer + event callback delivery.

## Coverage Improvement Plan (Pre-Operation Expectation Validation)

The target is to improve coverage by validating expected collaborator behavior before invoking each API (not just calling methods).

### A. Strengthen implementation-focused L1 tests
Create/add tests in `Tests/L1Tests/tests/test_RDKWindowManager.cpp` under `RDKWindowManagerImplementationTest` with strict expectations:

1. Lifecycle tests
- `Impl_Initialize_SetsEventListener_And_AddRef`:
  - `EXPECT_CALL(serviceMock, AddRef()).Times(1)`
  - `EXPECT_CALL(compositorMock, setEventListener(_)).Times(1)` if exposed via wrapper
  - validate success/failure branch for `WAYLAND_DISPLAY` unset path if hooks available
- `Impl_Deinitialize_ReleasesResources`:
  - Expect `serviceMock.Release`, `CompositorController::setEventListener(nullptr)`, and listener removal flow.

2. Notification tests
- Register/unregister:
  - `EXPECT_CALL(notificationMock, AddRef()).Times(1)` on register
  - `EXPECT_CALL(notificationMock, Release()).Times(1)` on unregister
- Dispatch fanout:
  - register 2 notifications, call `Dispatch(event, payload)`, validate both callbacks invoked with exact payload.

3. Key APIs with branch matrix
- `AddKeyIntercept*`:
  - missing keyCode/client branches -> expect `CompositorController::addKeyIntercept` not called
  - valid modifiers array -> expect exact `flags` value
- `AddKeyListener`/`RemoveKeyListener`:
  - test `keyCode` only, `nativeKeyCode` only, wildcard `*`, invalid object entries
  - assert `addKeyListener` vs `addNativeKeyListener` selection
- `InjectKey`/`GenerateKey`:
  - expect generated flags and exact calls to `CompositorController::injectKey/generateKey`

4. Display/visibility/zOrder APIs
- `SetVisible`, `EnableDisplayRender`, `SetZOrder`, `GetZOrder`:
  - validate empty-id negative path (`Times(0)` on compositor)
  - validate success path with exact args and return mapping
- `CreateDisplay/createDisplay`:
  - mock `getClients` and `createDisplay` to exercise:
    - existing client failure
    - new client success with listener add

5. Inactivity/key repeat APIs
- For each API, add success and failure cases with exact compositor expectations:
  - `enableInactivityReporting`
  - `setInactivityInterval`
  - `resetInactivityTime`
  - `enableKeyRepeats`
  - `getKeyRepeatsEnabled`
  - `ignoreKeyInputs`

6. Screenshot async tests
- `Impl_GetScreenshot_QueuesRequest`:
  - assert status success and queued behavior
- `Impl_NotifyScreenshotComplete_FiresNotification`:
  - register notification mock, invoke notify path, validate `OnScreenshotComplete(success, imageData)`
- If needed, add test hook/synchronization helper to avoid flaky waits.

### B. Improve wrapper/plugin tests (`RDKWindowManager.cpp`)
Add focused tests for currently uncovered wrapper functions:
- `Information` returns expected JSON service payload.
- `Deactivated` submits worker-pool job when connection id matches.
- `Deinitialize` out-of-process cleanup branch where `RemoteConnection != nullptr` and `Terminate` is called.

### C. Recommended mock usage from ref framework
From `ref/entservices-testframework/Tests/mocks`:
- Reuse `WindowManagerMock.h` for plugin wrapper boundary tests.
- Reuse `WorkerPoolImplementation.h` for deterministic worker dispatch.

Additional mocks needed for maximum implementation coverage:
- `RdkWindowManager` static wrapper mocks (already available locally in repo as `Tests/mocks/RdkWindowManagerMock.h` with `CompositorControllerImplMock` and `RdkWindowManagerImplMock`).
- If not already mirrored in ref framework, add equivalent mock(s) under `ref/entservices-testframework/Tests/mocks` to standardize reuse across repositories.

## Prioritized Test Additions (High ROI)
1. `Register`/`Unregister` + `Dispatch` callback fanout tests.
2. `EnableKeyRepeats` / `GetKeyRepeatsEnabled` / `IgnoreKeyInputs` full success-failure matrix.
3. `SetVisible` / `EnableDisplayRender` / `SetZOrder` / `GetZOrder` full matrix.
4. `AddKeyListener`/`RemoveKeyListener` wildcard + nativeKeyCode branches.
5. `InjectKey`/`GenerateKey` modifiers and target resolution branches.
6. `Information` + `Deactivated` wrapper tests.
7. Lifecycle and screenshot async path tests with deterministic synchronization hooks.

## Expected Outcome
With the above additions, `RDKWindowManagerImplementation.cpp` should improve substantially from the current 14.5% line / 22.1% function coverage. The exact final number depends on how much of thread-driven `Initialize`/`Deinitialize` and screenshot workflow is made deterministic in tests, but the largest no-hit groups (event dispatch, key pipelines, control APIs, z-order/display render) are all directly reachable via mock-driven expectation tests.
