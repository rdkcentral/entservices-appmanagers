# RuntimeManager L1 Coverage Analysis and Improvement Plan

## Scope

- Coverage report analyzed: `ref/coverage/RuntimeManager/*` (generated on 2026-03-09)
- Source analyzed: `RuntimeManager/*`
- Existing L1 tests reviewed: `Tests/L1Tests/tests/test_RunTimeManager.cpp`
- Reference mock catalog reviewed: `ref/entservices-testframework/Tests/mocks/*`

## Coverage Baseline (RuntimeManager package)

- **Line coverage:** `1028 / 1300` (**79.1%**)
- **Function coverage:** `114 / 146` (**78.1%**)

Primary low-coverage files:

1. `DobbySpecGenerator.cpp` — 399/505 lines (79.0%), 27/30 functions (90.0%)
2. `RuntimeManagerImplementation.cpp` — 321/395 lines (81.3%), 32/38 functions (84.2%)
3. `DobbyEventListener.cpp` — 20/55 lines (36.4%), 4/8 functions (50.0%)
4. `WindowManagerConnector.cpp` — 51/74 lines (68.9%), 6/8 functions (75.0%)
5. `Module.cpp` — 0/1 lines, 0/2 functions

---

## Functions with 0 Hits (Uncovered)

### `AIConfiguration.cpp`

- `AIConfiguration::getNonHomeAppMemoryLimit()`
- `AIConfiguration::getNonHomeAppGpuLimit()`
- `AIConfiguration::getGstreamerRegistryEnabled()`
- `AIConfiguration::getSvpEnabled()`
- `AIConfiguration::getEnableUsbMassStorage()`

### `DobbyEventListener.cpp`

- `OCIContainerNotification::OnContainerStarted(...)`
- `OCIContainerNotification::OnContainerStopped(...)`
- `OCIContainerNotification::OnContainerFailed(...)`
- `OCIContainerNotification::OnContainerStateChanged(...)`

### `DobbyEventListener.h`

- `OCIContainerNotification::QueryInterface(...)`
- ABI ctor/dtor symbol variants for `OCIContainerNotification`

### `DobbySpecGenerator.cpp`

- `~DobbySpecGenerator()` (one ABI variant symbol)
- `createMulticastSocketPlugin(...)`
- `addHolePunchPortToSpec(...)`

### `Module.cpp`

- `ModuleBuildRef`
- `ModuleServiceMetadata`

### `RuntimeManagerImplementation.cpp`

- `RuntimeManagerImplementation::getInstance()`
- `RuntimeManagerImplementation::Register(...)`
- `RuntimeManagerImplementation::Unregister(...)`
- ABI ctor/dtor symbol variants

### `RuntimeManagerImplementation.h`

- `Configuration::~Configuration()` (ABI variant)
- `Job::~Job()` (ABI variant)

### `WindowManagerConnector.cpp`

- `WindowManagerConnector::isPluginInitialized()`
- `WindowManagerNotification::OnUserInactivity(...)`

### `WindowManagerConnector.h`

- `WindowManagerNotification::QueryInterface(...)`
- ABI ctor/dtor symbol variants for `WindowManagerNotification`

---

## Uncovered Lines (by file)

> Full detail is in the LCOV pages. Key ranges are summarized here.

- `AIConfiguration.cpp`: **26 lines**
  - Getter lines and a few parse/default branches.
- `DobbyEventListener.cpp`: **35 lines**
  - Callback handlers (`OnContainer*`) and registration failure paths.
- `DobbyEventListener.h`: **2 lines**
  - Interface map macro lines.
- `DobbySpecGenerator.cpp`: **106 lines**
  - Optional plugin paths, filesystem/permission branches, hole-punch logic.
- `Module.cpp`: **1 line**
  - `MODULE_NAME_DECLARATION(BUILD_REFERENCE)`
- `RuntimeManagerImplementation.cpp`: **74 lines**
  - Notification register/unregister path, callback dispatch edges, retry/error branches.
- `UserIdManager.cpp`: **3 lines**
  - Existing-user return and clear pool return path.
- `WindowManagerConnector.cpp`: **23 lines**
  - initialization failure paths, fallback branches, inactivity callback.
- `WindowManagerConnector.h`: **2 lines**
  - Interface map macro lines.

---

## Why These Are Not Covered (Root-Cause) + Complexity

## 1) Not invoked by current L1 scenario set (**Low complexity to fix**)

- Trivial getters in `AIConfiguration`
- `RuntimeManagerImplementation::getInstance()`
- `WindowManagerConnector::isPluginInitialized()`

Reason: existing tests validate API command flows (`Run`, `Terminate`, `Wake`, `Suspend`, etc.) but do not explicitly call these helper accessors.

## 2) Callback-driven paths not simulated (**Medium complexity**)

- `DobbyEventListener::OCIContainerNotification::OnContainer*`
- `WindowManagerNotification::OnUserInactivity`

Reason: tests mock `Register(...)` success but do not capture and invoke the notification sink object passed by RuntimeManager.

## 3) Error/retry/fallback branches under-tested (**Medium to High complexity**)

- Multiple `RuntimeManagerImplementation` retry and failover lines
- Window manager init/deinit failure branches
- Storage manager recreation path

Reason: current tests mostly use straightforward success/failure of primary calls, not multi-attempt or staged failure/recovery sequences.

## 4) Optional / TODO / environment-dependent branches (**High complexity**)

- `DobbySpecGenerator` blocks around FKPS, hole-punching, multicast plugin, mount/permission handling, `/tmp` and `/opt` dependent logic.

Reason: these require crafted runtime JSON plus filesystem state and errno-level branch simulation.

## 5) ABI symbol artifacts in coverage reports (**Low priority / often non-actionable**)

- Duplicate constructor/destructor symbols (`C1/C2`, `D0/D1` forms)
- Interface-map macro lines in headers

Reason: compiler-emitted variants can show uncovered even when source-level constructor/destructor execution is already covered.

---

## How to Improve Coverage with Additional Tests

## Phase 1 (Quick Wins, high value / low effort)

1. **Add AI getter coverage test**
   - In `test_RunTimeManager.cpp`, add a focused test that triggers config load and validates:
     - `getNonHomeAppMemoryLimit()`
     - `getNonHomeAppGpuLimit()`
     - `getGstreamerRegistryEnabled()`
     - `getSvpEnabled()`
     - `getEnableUsbMassStorage()`

2. **Add Register/Unregister API tests for RuntimeManager notifications**
   - Explicitly call `interface->Register(...)` and `interface->Unregister(...)`
   - Cover both:
     - success path
     - unregister-not-found path

3. **Add `getInstance()` micro-test**
   - Validate returned pointer non-null after implementation creation.

4. **Add `isPluginInitialized()` direct test**
   - Assert false before init, true after successful init path.

## Phase 2 (Event callback path coverage)

5. **Drive OCI notification callbacks explicitly**
   - Capture the argument passed to `OCIContainerMock::Register(INotification*)`.
   - Manually invoke:
     - `OnContainerStarted`
     - `OnContainerStopped`
     - `OnContainerFailed`
     - `OnContainerStateChanged`
   - Verify RuntimeManager notification dispatch behavior.

6. **Drive window inactivity callback explicitly**
   - Capture argument passed to `WindowManagerMock::Register(INotification*)`.
   - Invoke `OnUserInactivity(...)`.
   - Assert no crash / expected side effect.

## Phase 3 (Error-path and retry coverage)

7. **Retry-loop tests in `RuntimeManagerImplementation`**
   - `QueryInterfaceByCallsign` fail first N-1 times, succeed on Nth attempt.
   - Fully fail all attempts to hit “max retries exhausted” branch.

8. **Storage manager recovery tests**
   - Force initial storage object creation failure and subsequent recovery.
   - Cover branch where retry attempt also fails.

9. **Dispatch invalid-state/event-data tests**
   - Inject malformed / incomplete event JSON into dispatch path.
   - Cover default event and parse-failure branches.

## Phase 4 (DobbySpecGenerator hard branches)

10. **Filesystem-conditioned tests**
    - Create temp files/dirs for FKPS and hole-punch scenarios.
    - Use malformed permissions to trigger `chmod/chown` error paths.

11. **Spec branch tests**
    - Add targeted tests for:
      - `addHolePunchPortToSpec(...)` with `rdkPlugins` format and legacy `plugins` format
      - `createMulticastSocketPlugin(...)` current null-return behavior

---

## Mocks from `ref/entservices-testframework` to Reuse / Sync

The following RuntimeManager-relevant mocks are available in `ref/entservices-testframework/Tests/mocks` and should be treated as canonical reusable assets:

- `OCIContainerMock.h` (includes `IOCIContainerNotificationMock`)
- `WindowManagerMock.h` (includes `WindowManagerNotificationMock`)
- `StorageManagerMock.h`
- `RuntimeManagerMock.h`
- `thunder/ServiceMock.h`
- `thunder/COMLinkMock.h`
- `WorkerPoolImplementation.h`

### Important observation

For the RuntimeManager subset, your local mocks are currently **effectively in sync** with the ref versions for:

- `OCIContainerMock.h`
- `WindowManagerMock.h`
- `ServiceMock.h`
- `RuntimeManagerMock.h`

So, no mandatory copy-in is needed immediately for these files.

### Recommended mock additions (not currently present as concrete behavior mocks)

To improve RuntimeManager callback coverage, add a concrete `IRuntimeManager::INotification` gmock class with methods:

- `OnStarted(const string&)`
- `OnTerminated(const string&)`
- `OnFailure(const string&, const string&)`
- `OnStateChanged(const string&, RuntimeState)`

This enables assertions on dispatch calls from `Dispatch(...)` and callback relay paths.

---

## Suggested Prioritized Test Backlog (Top 10)

1. Register/Unregister success + duplicate/unregister-missing
2. OCI callback relay (started/stopped/failed/state-changed)
3. Window inactivity callback path
4. AI getter invocation test (5 uncovered getters)
5. `getInstance()` test
6. `isPluginInitialized()` test
7. Retry success-on-last-attempt for OCI object creation
8. Retry total-failure for OCI object creation
9. Storage manager recovery fail branch
10. `addHolePunchPortToSpec` two-format tests

---

## Expected Coverage Impact

- **Fast uplift target:** +3% to +6% line coverage by implementing Phases 1–2.
- **Medium uplift target:** additional +2% to +4% with focused retry/error tests (Phase 3).
- **Hard uplift target:** +2% to +5% from DobbySpecGenerator environment-dependent branches (Phase 4).

Total realistic improvement with disciplined test additions: **~7% to 12%** line coverage uplift for RuntimeManager package.
