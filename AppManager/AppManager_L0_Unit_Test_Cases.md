# AppManager Plugin L0 Unit Test Cases

## 1. Scope and Objective
This document defines comprehensive L0 unit test coverage for the `AppManager` plugin, including:
- Positive scenarios
- Negative scenarios
- Boundary scenarios

It provides:
- Test IDs
- Test descriptions
- Inputs
- Expected results
- File and method mapping for fast review
- Explicit list of uncovered/deferred methods

Target: cover at least 75% of `AppManager/*.cpp` and `AppManager/*.h` files.

## 2. Coverage Summary (File-Level)
Total files in scope (`.cpp` + `.h`) under `AppManager`: **15**

Planned L0 direct coverage: **13 / 15 files = 86.67%**

| File | Planned L0 Coverage | Notes |
|---|---:|---|
| `AppManager/AppManager.cpp` | Yes | Plugin lifecycle, registration, deactivation handling |
| `AppManager/AppManager.h` | Yes | Notification callback behavior via plugin tests |
| `AppManager/AppManagerImplementation.cpp` | Yes | Core API behavior, error paths, queueing logic |
| `AppManager/AppManagerImplementation.h` | Yes | Public API and handler interfaces |
| `AppManager/LifecycleInterfaceConnector.cpp` | Yes | Lifecycle manager remote object and action delegation |
| `AppManager/LifecycleInterfaceConnector.h` | Yes | Notification handler and mapper behavior |
| `AppManager/AppInfo.cpp` | Yes | Default initialization and getter/setter behavior |
| `AppManager/AppInfo.h` | Yes | Data-model API contract |
| `AppManager/AppInfoManager.cpp` | Yes | Thread-safe map operations and update/upsert flows |
| `AppManager/AppInfoManager.h` | Yes | Registry API contract |
| `AppManager/AppManagerTelemetryReporting.cpp` | Partial | Telemetry call-path validation via stubs/mocks |
| `AppManager/AppManagerTelemetryReporting.h` | Partial | Interface-level validation |
| `AppManager/AppManagerTypes.h` | Yes | Enum/struct mapping and serialization input integrity |
| `AppManager/Module.cpp` | No (deferred) | Macro-only declaration file; negligible unit behavior |
| `AppManager/Module.h` | No (deferred) | Macro-only module name declaration |

## 3. Quick Test Matrix (Short Summary)
Type legend: `P` = Positive, `N` = Negative, `B` = Boundary

| Test ID | Type | Short Summary | File(s) | Method(s) Covered |
|---|---|---|---|---|
| AM-L0-001 | P | Initialize plugin with valid service | `AppManager/AppManager.cpp` | `Initialize` |
| AM-L0-002 | N | Initialize fails when implementation root creation fails | `AppManager/AppManager.cpp` | `Initialize` |
| AM-L0-003 | N | Initialize fails when `IConfiguration` unavailable | `AppManager/AppManager.cpp` | `Initialize` |
| AM-L0-004 | N | Initialize fails when configure returns error | `AppManager/AppManager.cpp` | `Initialize`, `Deinitialize` |
| AM-L0-005 | P | Deinitialize releases interfaces and unregisters | `AppManager/AppManager.cpp` | `Deinitialize` |
| AM-L0-006 | B | Deactivated callback with matching connection ID submits shell job | `AppManager/AppManager.cpp` | `Deactivated` |
| AM-L0-007 | B | Deactivated callback with non-matching ID does nothing | `AppManager/AppManager.cpp` | `Deactivated` |
| AM-L0-008 | P | Register notification adds new listener | `AppManager/AppManagerImplementation.cpp` | `Register` |
| AM-L0-009 | N | Register with duplicate notification is rejected | `AppManager/AppManagerImplementation.cpp` | `Register` |
| AM-L0-010 | P | Unregister existing notification succeeds | `AppManager/AppManagerImplementation.cpp` | `Unregister` |
| AM-L0-011 | N | Unregister unknown notification fails | `AppManager/AppManagerImplementation.cpp` | `Unregister` |
| AM-L0-012 | N | Launch app fails when appId is empty | `AppManager/AppManagerImplementation.cpp` | `LaunchApp` |
| AM-L0-013 | N | Launch app fails when package not installed | `AppManager/AppManagerImplementation.cpp` | `LaunchApp`, `IsInstalled` |
| AM-L0-014 | P | Launch app success path queues and delegates to lifecycle connector | `AppManager/AppManagerImplementation.cpp` | `LaunchApp` |
| AM-L0-015 | N | Launch app fails when package lock fails | `AppManager/AppManagerImplementation.cpp` | `LaunchApp`, `packageLock` |
| AM-L0-016 | P | Preload app success path with runtime config | `AppManager/AppManagerImplementation.cpp` | `PreloadApp` |
| AM-L0-017 | N | Preload app fails for invalid appId | `AppManager/AppManagerImplementation.cpp` | `PreloadApp` |
| AM-L0-018 | P | Close app delegates and succeeds | `AppManager/AppManagerImplementation.cpp` | `CloseApp` |
| AM-L0-019 | N | Close app propagates lifecycle manager error | `AppManager/AppManagerImplementation.cpp` | `CloseApp` |
| AM-L0-020 | P | Terminate app delegates and succeeds | `AppManager/AppManagerImplementation.cpp` | `TerminateApp` |
| AM-L0-021 | P | Kill app delegates and succeeds | `AppManager/AppManagerImplementation.cpp` | `KillApp` |
| AM-L0-022 | P | Get loaded apps returns iterator from lifecycle manager | `AppManager/AppManagerImplementation.cpp` | `GetLoadedApps` |
| AM-L0-023 | P | Send intent success path | `AppManager/AppManagerImplementation.cpp` | `SendIntent` |
| AM-L0-024 | P | Get installed apps returns JSON list | `AppManager/AppManagerImplementation.cpp` | `GetInstalledApps`, `fetchAppPackageList` |
| AM-L0-025 | B | Get installed apps with empty package list returns empty JSON array | `AppManager/AppManagerImplementation.cpp` | `GetInstalledApps` |
| AM-L0-026 | P | IsInstalled true when package exists | `AppManager/AppManagerImplementation.cpp` | `IsInstalled`, `checkIsInstalled` |
| AM-L0-027 | P | IsInstalled false when package missing | `AppManager/AppManagerImplementation.cpp` | `IsInstalled`, `checkIsInstalled` |
| AM-L0-028 | P | GetAppProperty returns stored property | `AppManager/AppManagerImplementation.cpp` | `GetAppProperty` |
| AM-L0-029 | N | GetAppProperty invalid key handling | `AppManager/AppManagerImplementation.cpp` | `GetAppProperty` |
| AM-L0-030 | P | SetAppProperty stores value and succeeds | `AppManager/AppManagerImplementation.cpp` | `SetAppProperty` |
| AM-L0-031 | P | ClearAppData delegates to storage manager | `AppManager/AppManagerImplementation.cpp` | `ClearAppData` |
| AM-L0-032 | P | ClearAllAppData delegates to storage manager | `AppManager/AppManagerImplementation.cpp` | `ClearAllAppData` |
| AM-L0-033 | P | GetAppMetadata pass-through behavior | `AppManager/AppManagerImplementation.cpp` | `GetAppMetadata` |
| AM-L0-034 | B | Max limits getters return configured defaults | `AppManager/AppManagerImplementation.cpp` | `GetMaxRunningApps`, `GetMaxHibernatedApps`, `GetMaxHibernatedFlashUsage`, `GetMaxInactiveRamUsage` |
| AM-L0-035 | P | App lifecycle change callback updates app state and dispatches event | `AppManager/AppManagerImplementation.cpp` | `handleOnAppLifecycleStateChanged` |
| AM-L0-036 | P | App unload callback removes app info and dispatches event | `AppManager/AppManagerImplementation.cpp` | `handleOnAppUnloaded`, `removeAppInfoByAppId` |
| AM-L0-037 | P | App launch request callback dispatches event | `AppManager/AppManagerImplementation.cpp` | `handleOnAppLaunchRequest` |
| AM-L0-038 | P | Configure creates required remote objects successfully | `AppManager/AppManagerImplementation.cpp` | `Configure`, `createPersistentStoreRemoteStoreObject`, `createPackageManagerObject`, `createStorageManagerRemoteObject` |
| AM-L0-039 | N | Configure fails if lifecycle connector setup fails | `AppManager/AppManagerImplementation.cpp` | `Configure` |
| AM-L0-040 | P | AppInfo default object values are safe | `AppManager/AppInfo.cpp` | `AppInfo::AppInfo` |
| AM-L0-041 | P | AppInfo getter/setter consistency | `AppManager/AppInfo.cpp` | all getters/setters |
| AM-L0-042 | P | AppInfoManager upsert creates and updates record | `AppManager/AppInfoManager.cpp` | `upsert`, `get`, `exists` |
| AM-L0-043 | N | AppInfoManager update fails when app not found | `AppManager/AppInfoManager.cpp` | `update` |
| AM-L0-044 | P | AppInfoManager remove and clear behavior | `AppManager/AppInfoManager.cpp` | `remove`, `clear` |
| AM-L0-045 | B | AppInfoManager field defaults for unknown app | `AppManager/AppInfoManager.cpp` | all convenience getters |
| AM-L0-046 | P | Lifecycle connector create/release remote objects | `AppManager/LifecycleInterfaceConnector.cpp` | `createLifecycleManagerRemoteObject`, `releaseLifecycleManagerRemoteObject` |
| AM-L0-047 | P | Lifecycle connector launch success path | `AppManager/LifecycleInterfaceConnector.cpp` | `launch`, `isAppLoaded` |
| AM-L0-048 | N | Lifecycle connector launch failure when LM unavailable | `AppManager/LifecycleInterfaceConnector.cpp` | `launch` |
| AM-L0-049 | P | Lifecycle connector close/terminate/kill/sendIntent delegation | `AppManager/LifecycleInterfaceConnector.cpp` | `closeApp`, `terminateApp`, `killApp`, `sendIntent` |
| AM-L0-050 | B | map lifecycle and error reason conversion correctness | `AppManager/LifecycleInterfaceConnector.cpp` | `mapAppLifecycleState`, `mapErrorReason` |
| AM-L0-051 | P | Telemetry initialize and singleton retrieval | `AppManager/AppManagerTelemetryReporting.cpp` | `getInstance`, `initialize` |
| AM-L0-052 | P | Telemetry records launch metric when enabled | `AppManager/AppManagerTelemetryReporting.cpp` | `recordLaunchTime` |
| AM-L0-053 | N | Telemetry does nothing when metrics disabled | `AppManager/AppManagerTelemetryReporting.cpp` | all report methods guard path |
| AM-L0-054 | P | Telemetry crash reporting publishes expected payload | `AppManager/AppManagerTelemetryReporting.cpp` | `reportAppCrashedTelemetry` |
| AM-L0-055 | B | Enum and package struct boundary/value mapping checks | `AppManager/AppManagerTypes.h` | `ApplicationType`, `CurrentAction`, `CurrentActionError`, `PackageInfo` |

## 4. Detailed Test Cases

| Test ID | Description | Inputs | Expected Results |
|---|---|---|---|
| AM-L0-001 | Plugin initialize happy path with valid `IShell` mocks and valid implementation root. | Valid service mock; root returns `IAppManager` and `IConfiguration`; configure returns `Core::ERROR_NONE`. | `Initialize()` returns empty string; `JAppManager::Register` path active; notification registration performed. |
| AM-L0-002 | Initialize failure when root cannot create `AppManagerImplementation`. | Valid service mock; `Root<Exchange::IAppManager>` returns `nullptr`. | Returns non-empty error message; `Deinitialize()` cleanup invoked; no leak. |
| AM-L0-003 | Initialize failure when implementation does not expose `IConfiguration`. | Root returns implementation mock; `QueryInterface<IConfiguration>` returns `nullptr`. | Returns non-empty error; implementation released during cleanup. |
| AM-L0-004 | Initialize failure when `Configure` returns non-success. | Configure returns non-zero error code. | Initialize returns `"AppManager could not be configured"`; deinitialize sequence executed. |
| AM-L0-005 | Deinitialize normal release order and unregistration. | Already initialized plugin instance with registered notifications. | Unregisters shell and app notifications, releases configure/implementation/service, resets connectionId. |
| AM-L0-006 | Deactivated with matching remote connection ID. | Connection mock with ID equal to plugin `mConnectionId`. | Submits `IShell::DEACTIVATED` job via worker pool once. |
| AM-L0-007 | Deactivated with non-matching connection ID. | Connection mock with unmatched ID. | No shell job submission; state unchanged. |
| AM-L0-008 | Register new notification subscriber. | Valid notification mock pointer. | Returns `Core::ERROR_NONE`; subscriber list contains pointer once. |
| AM-L0-009 | Register duplicate notification subscriber. | Same notification pointer registered twice. | Second call returns non-success (reject duplicate). |
| AM-L0-010 | Unregister existing notification. | Registered notification pointer. | Returns success and calls `Release` once on removed notification. |
| AM-L0-011 | Unregister unknown notification. | Non-registered notification pointer. | Returns error (`Core::ERROR_GENERAL` path). |
| AM-L0-012 | LaunchApp invalid input guard for empty appId. | `appId=""`, valid intent/args. | Returns input error; no lifecycle call. |
| AM-L0-013 | LaunchApp rejects not-installed app. | Package list mock without requested appId. | Returns not-installed error; no spawn request. |
| AM-L0-014 | LaunchApp success for installed app. | Installed app package; lock succeeds; lifecycle spawn succeeds. | Returns success; action and app info state updated; request processed. |
| AM-L0-015 | LaunchApp lock failure. | Installed app; `packageLock` returns failure. | Returns lock error and telemetry error code path invoked. |
| AM-L0-016 | PreloadApp success with runtime config defaults/custom values. | Valid appId, launchArgs, runtime config from store and package metadata. | Returns success; target state set to paused/suspended based on policy; app info updated. |
| AM-L0-017 | PreloadApp invalid appId handling. | Empty appId. | Returns error and does not call lifecycle manager. |
| AM-L0-018 | CloseApp success delegation. | Existing appId; lifecycle connector mock returns success. | Returns success; close action update path executed. |
| AM-L0-019 | CloseApp remote failure propagation. | Existing appId; lifecycle connector returns error code. | Same error returned to caller; telemetry error path evaluated. |
| AM-L0-020 | TerminateApp success delegation. | Valid appId and active app info. | Returns success; state transition path tested. |
| AM-L0-021 | KillApp success delegation. | Valid appId and active app info. | Returns success; kill action telemetry path tested. |
| AM-L0-022 | GetLoadedApps pass-through result handling. | Lifecycle connector returns valid iterator. | Returns success and output iterator non-null. |
| AM-L0-023 | SendIntent pass-through success. | Valid appId/intent. | Returns lifecycle connector result; no unintended mutation. |
| AM-L0-024 | GetInstalledApps JSON conversion with multiple packages. | Package list with >=2 entries, mixed types. | Output JSON contains all expected app identifiers and metadata fields. |
| AM-L0-025 | GetInstalledApps empty result set boundary. | Empty package list. | Returns success with empty JSON array string. |
| AM-L0-026 | IsInstalled true path. | Package list includes `appId`. | Returns success, `installed=true`. |
| AM-L0-027 | IsInstalled false path. | Package list excludes `appId`. | Returns success, `installed=false`. |
| AM-L0-028 | GetAppProperty for known property. | Existing app info and known key. | Returns success and correct value. |
| AM-L0-029 | GetAppProperty unknown/invalid key. | Existing app info with unsupported key. | Returns expected non-success or empty value per implementation contract. |
| AM-L0-030 | SetAppProperty success and persistence in manager/store. | Valid key/value and appId. | Returns success and value visible in subsequent get/read path. |
| AM-L0-031 | ClearAppData delegates to storage manager. | Valid appId and storage manager mock. | Returns delegated result; called once with same appId. |
| AM-L0-032 | ClearAllAppData delegates to storage manager. | Storage manager mock. | Returns delegated result; clear-all invoked once. |
| AM-L0-033 | GetAppMetadata retrieval path. | appId + metadata key and mocked backing store. | Returns expected metadata value and success code. |
| AM-L0-034 | Max limits getter boundary/default values. | Configuration with explicit values and default/fallback values. | Each getter returns configured int32 value; no overflow/negative invalid state. |
| AM-L0-035 | Lifecycle state change callback updates app state and notifies observers. | Callback inputs for appId, appInstanceId, old/new states, reason. | AppInfoManager updated correctly; notification dispatch path triggered. |
| AM-L0-036 | App unloaded callback removes cached app info. | Existing app info and unload callback. | App info entry removed and unload event dispatched. |
| AM-L0-037 | App launch request callback dispatch verification. | appId, intent, source callback input. | Launch request event emitted to registered notification listeners. |
| AM-L0-038 | Configure success path creates all dependencies. | Valid service with store/package/storage/lifecycle dependencies. | Returns `Core::ERROR_NONE`; all required remote objects are created/registered. |
| AM-L0-039 | Configure failure path when required dependency creation fails. | One dependency creator returns error. | Configure returns failure and partial resources are safely cleaned. |
| AM-L0-040 | AppInfo constructor default values. | `AppInfo` default instance. | States are default (`UNLOADED`/none), strings empty, numeric/time values zeroed. |
| AM-L0-041 | AppInfo round-trip getters/setters. | Set all fields with non-default values. | All getters return exact values previously set. |
| AM-L0-042 | AppInfoManager upsert create then update. | New appId + lambda update, repeated with second lambda. | Entry created then updated; values reflect latest write. |
| AM-L0-043 | AppInfoManager update absent-key negative path. | `update` on non-existent appId. | Returns `false`; map remains unchanged. |
| AM-L0-044 | AppInfoManager remove/clear operations. | Add multiple entries, then remove one, then clear. | Removed entry not found; clear empties registry. |
| AM-L0-045 | AppInfoManager default getters for unknown appId. | Query unknown appId across all convenience getters. | Returns documented defaults without crash. |
| AM-L0-046 | Lifecycle connector create/release remote object flow. | Valid shell with lifecycle interfaces + notifications. | Create returns success and registers; release unregisters and nulls pointers. |
| AM-L0-047 | Lifecycle connector launch success path with spawn/update behavior. | Valid appId; LM available; spawn returns instanceId and success. | Correct lifecycle API called; app info upsert updated with target state and intent. |
| AM-L0-048 | Lifecycle connector launch when LM unavailable. | Shell returns null lifecycle interfaces. | Returns error; no dereference crash; warning/error path exercised. |
| AM-L0-049 | Lifecycle connector action delegation functions. | Valid appId/intent for close/terminate/kill/sendIntent. | Each function propagates underlying LM return code correctly. |
| AM-L0-050 | Lifecycle and error reason mapping boundaries. | All known lifecycle states and representative error strings. | Mapped enum values are correct; unknown values map to safe default. |
| AM-L0-051 | Telemetry singleton and initialize. | Valid service mock and telemetry client mock. | `getInstance` stable; `initialize` sets service and initializes client. |
| AM-L0-052 | Telemetry launch-time recording. | Telemetry enabled and valid appId + launch time. | `record` called with marker `appManagerLaunchTime` payload. |
| AM-L0-053 | Telemetry disabled guard path. | Force `isTelemetryMetricsEnabled=false`. | No telemetry `record/publish` call for all reporting APIs. |
| AM-L0-054 | App crash telemetry payload. | appId, appInstanceId, crashReason. | Record + publish called with crash marker and correct JSON fields. |
| AM-L0-055 | Type and enum value boundary checks. | Unknown/min/max enum values and default `PackageInfo`. | Serialization/mapping logic remains stable; unknown values handled safely. |

## 5. Uncovered or Deferred Methods (Explicit)
The following methods are not fully covered at strict L0 because they require heavy async/runtime integration or platform services. They should be covered in L1/L2 integration tests.

| File | Method | Why Not Fully L0 | Planned Level |
|---|---|---|---|
| `AppManager/AppManagerImplementation.cpp` | `AppManagerWorkerThread` | Threaded consumer loop, condition variable timing, and worker lifecycle race behavior are difficult to deterministically assert in pure unit tests. | L1 |
| `AppManager/AppManagerImplementation.cpp` | `Dispatch` (worker-pool execution semantics) | Requires Thunder worker pool scheduling/ordering behavior beyond unit scope. | L1 |
| `AppManager/AppManagerImplementation.cpp` | `dispatchEvent` (full async execution path) | L0 can verify submit call; end-to-end asynchronous delivery belongs to integration. | L1 |
| `AppManager/LifecycleInterfaceConnector.cpp` | `OnAppLifecycleStateChanged` end-to-end notification chain | Depends on remote LifecycleManager callback timing and external state transitions. | L1 |
| `AppManager/LifecycleInterfaceConnector.cpp` | `OnAppStateChanged` end-to-end notification chain | Requires remote event source behavior and multi-plugin interaction. | L1 |
| `AppManager/AppManagerTelemetryReporting.cpp` | `reportTelemetryDataOnStateChange` full publish side effects | Depends on telemetry runtime and dynamic app state transitions; L0 should mock and validate invocation/payload only. | L1 |
| `AppManager/Module.cpp` | `MODULE_NAME_DECLARATION` | Macro expansion only; no executable branch logic to unit test. | N/A |
| `AppManager/Module.h` | `MODULE_NAME` macro declaration | Header macro contract only; compile-time artifact, not runtime behavior. | N/A |

## 6. How L0 Test Code Will Be Generated (Using `ref/entservices-appgateway`)
L0 test implementation for `AppManager` should mirror the proven pattern in `ref/entservices-appgateway`.

### 6.1 Reference Assets
Use these files as templates:
- `ref/entservices-appgateway/Tests/L0Tests/common/L0Bootstrap.hpp`
- `ref/entservices-appgateway/Tests/L0Tests/common/L0Expect.hpp`
- `ref/entservices-appgateway/Tests/L0Tests/common/L0ServiceMock.hpp`
- `ref/entservices-appgateway/Tests/L0Tests/common/L0TestTypes.hpp`
- `ref/entservices-appgateway/Tests/L0Tests/common/README.md`
- `ref/entservices-appgateway/Tests/L0Tests/AppGateway/AppGatewayTest.cpp`
- `ref/entservices-appgateway/Tests/L0Tests/AppGateway/ServiceMock.h`
- `ref/entservices-appgateway/Tests/L0Tests/AppGateway/AppGateway_Init_DeinitTests.cpp`

### 6.2 Proposed AppManager L0 Layout
Create:
- `Tests/L0Tests/AppManager/AppManagerTest.cpp` (test registry and main)
- `Tests/L0Tests/AppManager/ServiceMock.h` (plugin-specific IShell and dependency mocks)
- `Tests/L0Tests/AppManager/AppManager_Init_DeinitTests.cpp`
- `Tests/L0Tests/AppManager/AppManagerImplementation_CoreTests.cpp`
- `Tests/L0Tests/AppManager/AppManagerImplementation_ErrorBoundaryTests.cpp`
- `Tests/L0Tests/AppManager/AppInfo_AppInfoManager_Tests.cpp`
- `Tests/L0Tests/AppManager/LifecycleInterfaceConnector_Tests.cpp`
- `Tests/L0Tests/AppManager/AppManagerTelemetryReporting_Tests.cpp`

### 6.3 Generation Workflow
1. Copy common L0 harness headers from `ref/entservices-appgateway/Tests/L0Tests/common` (bootstrap, expect, test types, base service mock).
2. Build `AppManager`-specific `ServiceMock.h` by adapting:
   - `QueryInterfaceByCallsign` behavior
   - `Root<T>` behavior for `Exchange::IAppManager`
   - Dependency mocks for lifecycle, package manager, store2, storage manager, telemetry client
3. Create `AppManagerTest.cpp` using AppGateway test registry style:
   - `extern uint32_t Test_*();`
   - test table execution loop
   - `L0BootstrapGuard` lifetime guard
4. Implement test functions per this document test IDs (`AM-L0-001` ... `AM-L0-055`).
5. Add `Tests/L0Tests/AppManager/CMakeLists.txt` target similar to AppGateway L0 target.
6. Register executable with `ctest` label `L0`.

### 6.4 Test Naming Convention
Use deterministic names tied to IDs, for example:
- `Test_AM_L0_001_Initialize_HappyPath`
- `Test_AM_L0_015_LaunchApp_PackageLockFailure`
- `Test_AM_L0_050_MapLifecycleState_Boundary`

### 6.5 Mock Strategy
- Use strict mocks/stubs for all external interfaces.
- Validate return-code propagation from remote/dependency interfaces.
- For async/worker paths, validate submission and state mutation in L0, and reserve full ordering/timing checks for L1.

## 7. Entry/Exit Criteria for L0
Entry:
- Mocks for IShell, lifecycle manager, package manager, store2, app storage manager, telemetry are available.
- Worker pool bootstrap utilities integrated.

Exit:
- All implemented L0 tests pass.
- File-level planned coverage remains >= 75% (target in this plan: 86.67%).
- Deferred methods are tracked for L1/L2 in test backlog.

## 8. Traceability Note
Each implemented test source should include the corresponding test ID in function name or comments to keep this design and executable tests synchronized.
