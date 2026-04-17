# RDKWindowManager L0 Test Implementation Plan

## Objective

Create a standalone L0 test suite for `RDKWindowManager` in:

- `Tests/L0Tests/RDKWindowManager/`

using the same test style already established by RuntimeManager L0 tests, while isolating external runtime dependencies on:

- `rdkcentral/rdk-window-manager`

This plan focuses on deterministic, CI-friendly tests with high branch coverage for plugin lifecycle, JSON-RPC-facing implementation behavior, callback dispatch, and error handling.

---

## Implemented Updates (Current Branch)

The following runner-level updates are now implemented in `Tests/L0Tests/RDKWindowManager/RDKWindowManagerTest.cpp`:

1. Per-test process isolation using `fork()` + `waitpid()`.
   - Each `Test_*` case runs in a child process to prevent static/global state from leaking across cases.
   - This directly addresses lifecycle-state coupling observed during early bring-up.

2. Bootstrap moved into child process scope.
   - `L0Test::L0BootstrapGuard` is created inside the child path (not parent scope), so worker-pool initialization is local to each test process.
   - This avoids parent/child bootstrap ordering side effects around compositor/listener tests.

3. Child termination uses `std::exit(...)`.
   - The runner intentionally uses `std::exit` (not `_exit`) after each child case.
   - This ensures gcov counters are flushed from child processes and fixes missing/zero coverage accounting for forked tests.

4. Runner case matrix expanded and centralized.
   - The `cases[]` registry now includes the extended implementation tests for display/event variants, intercept/inactivity branches, and error-path matrix coverage.
   - Case-level pass/fail summary remains deterministic and CI-friendly.

5. Signal-aware failure reporting retained.
   - The runner distinguishes non-zero exits vs signal terminations (`WIFEXITED`/`WIFSIGNALED`) and prints explicit failure diagnostics.
   - This improves triage quality for gdb-backed CI runs.

---

## Inputs Reviewed

Primary references used for this plan:

- `Tests/L0Tests/docs/RuntimeManager_L0_Coverage_Analysis.md`
- `Tests/L0Tests/RuntimeManager/RuntimeManagerTest.cpp`
- `Tests/L0Tests/RuntimeManager/RuntimeManager_ImplementationTests.cpp`
- `Tests/L0Tests/RuntimeManager/RuntimeManager_ComponentTests.cpp`
- `Tests/L0Tests/RuntimeManager/ServiceMock.h`
- `Tests/L0Tests/CMakeLists.txt`
- `RDKWindowManager/RDKWindowManager.cpp`
- `RDKWindowManager/RDKWindowManager.h`
- `RDKWindowManager/RDKWindowManagerImplementation.cpp`
- `RDKWindowManager/RDKWindowManagerImplementation.h`
- `RDKWindowManager/CMakeLists.txt`
- `rdkcentral/rdk-window-manager/include/compositorcontroller.h`
- `rdkcentral/rdk-window-manager/include/rdkwindowmanagerevents.h`

---

## Design Principles (Copied from RuntimeManager L0 Success Pattern)

1. Keep tests at L0 scope: logic and branch validation without requiring real Wayland/Westeros/compositor runtime.
2. Reuse the existing micro test framework:
   - `common/L0Bootstrap.hpp`
   - `common/L0Expect.hpp`
   - simple `uint32_t Test_*()` functions + single runner.
3. Build production plugin sources directly into the test binary for accurate coverage accounting.
4. Stub external/plugin dependencies using fakes in test code (not production changes).
5. Prioritize API contract and failure behavior first, then asynchronous and event-dispatch paths.

---

## Dependency Strategy for `rdkcentral/rdk-window-manager`

`RDKWindowManagerImplementation.cpp` depends heavily on static methods from `RdkWindowManager::CompositorController` and global functions in the `RdkWindowManager` namespace.

### Challenge

L0 tests cannot rely on actual compositor/Wayland stack and should not require real `-lrdkwindowmanager` behavior.

### Plan

Introduce a test-only shim translation unit under:

- `Tests/L0Tests/RDKWindowManager/fakes/FakeRdkWindowManager.cpp`

that provides minimal symbol implementations for all externally used methods called by `RDKWindowManagerImplementation.cpp`.

The shim will:

1. Provide deterministic return values and configurable outcomes.
2. Capture call counts and last-arguments for assertions.
3. Expose a small control surface namespace, for example:
   - `L0Test::RDKWMShim::Reset()`
   - `L0Test::RDKWMShim::SetCreateDisplayResult(bool)`
   - `L0Test::RDKWMShim::SetGetClientsResult(bool, vector<string>)`
   - `L0Test::RDKWMShim::SetScreenshotResult(bool, data, size)`

### Build Wiring

For L0 target only:

1. Add `RDKWindowManager` production sources into `Tests/L0Tests/CMakeLists.txt` similarly to RuntimeManager sources.
2. Add the fake shim source before link resolution.
3. Avoid linking real `-lrdkwindowmanager` in L0 build.
4. Keep this isolated to L0 target only to avoid impacting plugin production build.

---

## Proposed L0 Test Folder Layout

Create:

- `Tests/L0Tests/RDKWindowManager/RDKWindowManagerTest.cpp` (runner)
- `Tests/L0Tests/RDKWindowManager/RDKWindowManager_LifecycleTests.cpp`
- `Tests/L0Tests/RDKWindowManager/RDKWindowManager_ImplementationTests.cpp`
- `Tests/L0Tests/RDKWindowManager/RDKWindowManager_EventDispatchTests.cpp`
- `Tests/L0Tests/RDKWindowManager/ServiceMock.h`
- `Tests/L0Tests/RDKWindowManager/fakes/FakeRdkWindowManager.cpp`
- `Tests/L0Tests/RDKWindowManager/fakes/FakeRdkWindowManager.h`

Optional split for maintainability after initial commit:

- `RDKWindowManager_InputTests.cpp`
- `RDKWindowManager_DisplayTests.cpp`
- `RDKWindowManager_ScreenshotVncTests.cpp`

---

## Test Double Plan

## 1) `ServiceMock` for plugin lifecycle tests

Create an `IShell` mock modeled after RuntimeManager `ServiceMock.h` with:

- `Root<Exchange::IRDKWindowManager>()` path support through `COMLink()->Instantiate(...)` handler.
- Configurable return of:
  - valid `IRDKWindowManager*` fake implementation
  - `nullptr` (creation failure path)
- call counters for:
  - `AddRef/Release`
  - `Register/Unregister`
  - `RemoteConnection()`

## 2) `FakeRDKWindowManagerImpl` (interface fake)

A fake implementing `Exchange::IRDKWindowManager` with configurable return codes for:

- `Initialize/Deinitialize`
- `Register/Unregister`
- representative API methods used by wrapper plugin.

This fake enables coverage of `RDKWindowManager.cpp` wrapper behavior independently from implementation internals.

## 3) `FakeNotification`

A local fake implementing `Exchange::IRDKWindowManager::INotification` to count callback invocations and verify event payload forwarding.

## 4) `FakeRdkWindowManager` static shim

Implements required `RdkWindowManager` APIs referenced by implementation, including:

- lifecycle helpers: `initialize`, `draw`, `update`, `milliseconds`, `microseconds`
- compositor APIs: `CompositorController::*` methods used by implementation
- screenshot and vnc paths: `screenShot`, `startVncServer`, `stopVncServer`

---

## Test Coverage Plan by Source File

## A) `RDKWindowManager/RDKWindowManager.cpp`

### Lifecycle and wrapper contract tests

1. `Initialize` fails when `Root<IRDKWindowManager>` returns null.
2. `Initialize` fails when `impl->Initialize(service)` returns error.
3. `Initialize` success path:
   - registers shell notification
   - registers interface notification
   - registers `Exchange::JRDKWindowManager` stubs
4. `Deinitialize` success path:
   - unregister sequence is correct
   - impl released
   - remote connection terminate invoked when present
   - service unregistered/released
5. `Information()` returns expected service JSON containing `org.rdk.RDKWindowManager`.
6. `Deactivated()` with matching connection id submits DEACTIVATED job.
7. `Deactivated()` with non-matching connection id does nothing.

### Notification forwarding tests

Validate that inner `Notification` forwards all callbacks to JRDKWindowManager event bridge methods without crash:

- `OnUserInactivity`
- `OnDisconnected`
- `OnReady`
- `OnConnected`
- `OnVisible`
- `OnHidden`
- `OnFocus`
- `OnBlur`
- `OnScreenshotComplete`

Note: If direct static event-bus assertion is hard at L0, verify callback execution path and no error/abort.

## B) `RDKWindowManager/RDKWindowManagerImplementation.cpp`

### 1. Construction / lifecycle

1. construction sets singleton instance.
2. `Initialize(nullptr)` guarded behavior.
3. `Initialize(valid service)` basic success using shim.
4. `Deinitialize` releases service and resets state.
5. `Register`/`Unregister` normal, duplicate, and unknown unregister paths.

### 2. Event dispatch from listener

For each listener callback, verify internal dispatch reaches registered notification:

- inactivity
- application connected/disconnected
- ready
- visible/hidden
- focus/blur
- screenshot complete

### 3. Display and client management

1. `CreateDisplay` invalid inputs (empty client/display) => bad request.
2. `CreateDisplay` compositor false => error path.
3. `CreateDisplay` compositor true => success.
4. `GetApps` when compositor query fails.
5. `GetApps` when compositor returns clients list => JSON output is non-empty and valid.

### 4. Key intercept/listener APIs

Cover parsing and forwarding behavior for:

- `AddKeyIntercept`
- `AddKeyIntercepts`
- `RemoveKeyIntercept`
- `AddKeyListener`
- `RemoveKeyListener`
- `InjectKey`
- `GenerateKey`

Include malformed JSON tests and missing mandatory field tests.

### 5. Inactivity and input-control APIs

- `EnableInactivityReporting`
- `SetInactivityInterval`
- `ResetInactivityTime`
- `EnableKeyRepeats`
- `GetKeyRepeatsEnabled`
- `IgnoreKeyInputs`
- `EnableInputEvents`
- `KeyRepeatConfig`

For each API, add both success and shim-failure paths.

### 6. Visibility, focus, rendering, z-order

- `SetFocus`
- `SetVisible`
- `GetVisibility`
- `RenderReady`
- `EnableDisplayRender`
- `SetZOrder`
- `GetZOrder`

Include empty client validation and compositor false-return branches.

### 7. VNC and screenshot

- `StartVncServer` success/failure
- `StopVncServer` success/failure
- `GetScreenshot` success path
- `GetScreenshot` failure path when no data or screenshot fails

Also verify screenshot completion event dispatch behavior through notifications.

### 8. Thread and queue behavior around `CreateDisplayRequest`

Given implementation uses global queue + semaphore + render loop thread:

1. verify request enqueue/dequeue success path under deterministic shim timing.
2. verify timeout/error path does not deadlock.
3. verify `Deinitialize` safely terminates/joins thread.

Use short deterministic test timeouts to avoid flakes.

---

## Prioritized Delivery Phases

## Phase 0: Build plumbing and fakes (must-have)

1. Extend `Tests/L0Tests/CMakeLists.txt` to include RDKWindowManager sources and tests.
2. Add fake shim for `rdkcentral/rdk-window-manager` symbols.
3. Add local `ServiceMock` and interface fakes.
4. Add basic test runner.

Exit criteria:

- L0 target compiles and runs without real compositor dependency.

## Phase 1: Lifecycle and API validation tests

1. Wrapper plugin lifecycle tests (`RDKWindowManager.cpp`).
2. Implementation basic lifecycle and register/unregister tests.
3. Input validation + malformed JSON tests for key/public APIs.

Exit criteria:

- stable pass in CI.
- major failure branches covered.

## Phase 2: Functional forwarding and event tests

1. Notification callback dispatch coverage.
2. compositor success/failure forwarding across APIs.
3. screenshot and vnc branches.

Exit criteria:

- end-to-end callback paths covered with fake notifications.

## Phase 3: Concurrency and robustness

1. thread/queue edge cases around display requests.
2. re-initialize/deinitialize sequence tests.
3. lock contention and non-flaky timeout checks.

Exit criteria:

- no intermittent failures under repeated local runs.

---

## Candidate Initial Backlog (Top 25)

1. Init_FailsWhenRootCreationFails
2. Init_FailsWhenImplementationInitializeFails
3. Init_SucceedsRegistersNotifications
4. Deinit_UnregistersAndReleasesEverything
5. Deactivated_MatchingConnectionSubmitsFailureJob
6. Impl_RegisterNotification_Succeeds
7. Impl_RegisterDuplicate_IsIdempotent
8. Impl_UnregisterUnknown_ReturnsError
9. Impl_CreateDisplay_EmptyClient_BadRequest
10. Impl_CreateDisplay_CompositorFailure_Error
11. Impl_CreateDisplay_Success
12. Impl_GetApps_CompositorFailure_Error
13. Impl_GetApps_Success_ReturnsJson
14. Impl_AddKeyIntercept_InvalidJson_BadRequest
15. Impl_AddKeyIntercept_Success
16. Impl_AddKeyIntercepts_Success
17. Impl_RemoveKeyIntercept_Success
18. Impl_EnableInactivityReporting_SuccessAndFailure
19. Impl_GetVisibility_SuccessAndFailure
20. Impl_RenderReady_SuccessAndFailure
21. Impl_SetZOrder_SuccessAndFailure
22. Impl_StartVncServer_SuccessAndFailure
23. Impl_StopVncServer_SuccessAndFailure
24. Impl_GetScreenshot_SuccessDispatchesEvent
25. Impl_ListenerEvents_ForwardToNotifications

---

## CMake / Build Integration Tasks

1. Add RDKWindowManager production sources to L0 test target.
2. Add new test source files and fake shim sources.
3. Ensure include paths cover:
   - `RDKWindowManager/`
   - `helpers/`
   - test folder
4. Keep RuntimeManager L0 target untouched where possible; append sources conservatively.
5. Ensure no accidental production CMake changes in `RDKWindowManager/CMakeLists.txt`.

---

## Risks and Mitigations

1. Risk: static/global state in implementation (`sRunning`, global mutex/request queue) causes test leakage.
   - Mitigation: add test fixture reset hooks via shim and strict `Initialize/Deinitialize` pairing per test.

2. Risk: asynchronous render thread causes flaky tests.
   - Mitigation: deterministic shim timing, bounded waits, and explicit join verification.

3. Risk: too many API tests in one file reduce maintainability.
   - Mitigation: split tests by domain after initial baseline lands.

4. Risk: symbol mismatch between implementation usage and fake shim.
   - Mitigation: add compile-time checks and keep shim signatures identical to ref headers.

---

## Success Metrics

1. New L0 suite compiles and runs in CI without compositor runtime.
2. Significant line/function coverage increase for:
   - `RDKWindowManager.cpp`
   - `RDKWindowManagerImplementation.cpp`
3. No flaky failures in repeated local runs.
4. Clear backlog for incremental branch-coverage expansion.

---

## Implementation Order Recommendation

1. Build + shim scaffolding.
2. `RDKWindowManager.cpp` lifecycle tests.
3. `RDKWindowManagerImplementation` validation and forwarding tests.
4. Event dispatch tests.
5. Concurrency edge cases.

This order mirrors the RuntimeManager L0 adoption pattern and minimizes bring-up risk.