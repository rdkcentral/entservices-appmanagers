# AppManager L0 Unit Test Plan and Coverage Strategy

## Scope

This document defines a comprehensive L0 unit test plan for the `AppManager` plugin.
It covers:

- Positive scenarios
- Negative scenarios
- Boundary scenarios
- Method-to-test mapping
- Coverage target plan with a minimum of 75% across actionable `.cpp` and `.h` logic
- Explicit list of methods and code areas that are not fully coverable in L0, with rationale
- Guidance for generating the L0 test code using `Tests/L0Tests/PreinstallManager` as the reference pattern

### Plugin files in scope

- `AppManager/AppManager.cpp`
- `AppManager/AppManager.h`
- `AppManager/AppManagerImplementation.cpp`
- `AppManager/AppManagerImplementation.h`
- `AppManager/AppManagerTelemetryReporting.cpp`
- `AppManager/AppManagerTelemetryReporting.h`
- `AppManager/AppInfo.cpp`
- `AppManager/AppInfo.h`
- `AppManager/AppInfoManager.cpp`
- `AppManager/AppInfoManager.h`
- `AppManager/LifecycleInterfaceConnector.cpp`
- `AppManager/LifecycleInterfaceConnector.h`
- `AppManager/Module.cpp`
- `AppManager/Module.h`
- `AppManager/AppManagerTypes.h`

> Note: `Module.cpp` / `Module.h` and interface-map macro lines are compile-time glue and are called out separately in the non-coverable section.

---

## Coverage Goal

- Minimum target: **75% line coverage** across actionable plugin code
- Minimum target: **75% function coverage** across actionable plugin code
- Practical target with this plan: **80%+ line coverage** for `AppManagerImplementation.cpp`, `LifecycleInterfaceConnector.cpp`, `AppInfoManager.cpp`, and `AppInfo.cpp`
- Header coverage is expected to be high for executable inline methods and constructors, with macro-only regions treated as non-actionable

---

## L0 Test Architecture Reference

Use the same structure and style as the existing `PreinstallManager` L0 implementation in `Tests/L0Tests/PreinstallManager`:

- `Tests/L0Tests/AppManager/AppManagerTest.cpp`
- `Tests/L0Tests/AppManager/AppManager_LifecycleTests.cpp`
- `Tests/L0Tests/AppManager/AppManager_ImplementationTests.cpp`
- `Tests/L0Tests/AppManager/AppManager_ComponentTests.cpp`
- `Tests/L0Tests/AppManager/AppManagerTelemetryTests.cpp`
- `Tests/L0Tests/AppManager/AppInfoTests.cpp`
- `Tests/L0Tests/AppManager/AppInfoManagerTests.cpp`
- `Tests/L0Tests/AppManager/ServiceMock.h`
- Optional helper files for lifecycle and telemetry fakes

Reference patterns to reuse from `Tests/L0Tests/PreinstallManager`:

- Main runner with a single `main()` that executes a static list of test cases
- `L0BootstrapGuard` bootstrap at startup
- `L0Expect.hpp` helpers for assertions
- One test file per concern: lifecycle, implementation, component, and model/utilities
- Mock-heavy design with deterministic inputs and counters for reference tracking

---

## Quick Summary Table

| Test ID | Short Summary | File(s) | Method(s) Covered |
|---|---|---|---|
| AM-L0-POS-001 | Initialize succeeds with valid service, root object, configuration interface, and notification registration | `AppManager.cpp`, `AppManager.h`, `AppManagerImplementation.cpp`, `AppManagerImplementation.h` | `AppManager::Initialize`, `AppManagerImplementation::Configure`, `AppManagerImplementation::Register`, `Exchange::JAppManager::Register` |
| AM-L0-NEG-001 | Initialize fails when service is null | `AppManager.cpp` | `AppManager::Initialize` |
| AM-L0-NEG-002 | Initialize fails when `Root<Exchange::IAppManager>()` returns null | `AppManager.cpp` | `AppManager::Initialize` |
| AM-L0-NEG-003 | Initialize fails when `IConfiguration` is missing | `AppManager.cpp` | `AppManager::Initialize` |
| AM-L0-NEG-004 | Initialize fails when `Configure()` returns error | `AppManager.cpp`, `AppManagerImplementation.cpp` | `AppManager::Initialize`, `AppManager::Deinitialize`, `AppManagerImplementation::Configure` |
| AM-L0-POS-002 | Deinitialize releases all references and unregisters listeners | `AppManager.cpp` | `AppManager::Deinitialize`, `AppManager::Deactivated` |
| AM-L0-POS-003 | Information returns an empty string | `AppManager.cpp` | `AppManager::Information` |
| AM-L0-NEG-005 | Deactivated ignores a non-matching connection id | `AppManager.cpp` | `AppManager::Deactivated` |
| AM-L0-POS-004 | Deactivated submits a DEACTIVATED job for the matching connection | `AppManager.cpp` | `AppManager::Deactivated` |
| AM-L0-POS-005 | Notification forwards install/uninstall/lifecycle/launch/unloaded events | `AppManager.h` | `Notification::OnAppInstalled`, `OnAppUninstalled`, `OnAppLifecycleStateChanged`, `OnAppLaunchRequest`, `OnAppUnloaded` |
| AM-L0-POS-006 | AppInfo defaults and getter/setter coverage | `AppInfo.cpp`, `AppInfo.h` | All getters and setters in `AppInfo` |
| AM-L0-POS-007 | AppInfoManager CRUD and convenience accessors | `AppInfoManager.cpp`, `AppInfoManager.h` | `getInstance`, `exists`, `get`, `update`, `upsert`, `remove`, `clear`, convenience getters/setters |
| AM-L0-POS-008 | Register/Unregister notification observer behavior | `AppManagerImplementation.cpp`, `AppManagerImplementation.h` | `Register`, `Unregister` |
| AM-L0-BND-001 | Register duplicate observer is idempotent | `AppManagerImplementation.cpp` | `Register` |
| AM-L0-NEG-006 | Unregister missing observer returns error | `AppManagerImplementation.cpp` | `Unregister` |
| AM-L0-POS-009 | Configure creates lifecycle, package, persistent-store, and storage objects | `AppManagerImplementation.cpp`, `LifecycleInterfaceConnector.cpp` | `Configure`, `createLifecycleManagerRemoteObject`, `createPersistentStoreRemoteStoreObject`, `createPackageManagerObject`, `createStorageManagerRemoteObject` |
| AM-L0-NEG-007 | Configure null service fails cleanly | `AppManagerImplementation.cpp` | `Configure` |
| AM-L0-NEG-008 | Remote-object creation failure branches are covered | `AppManagerImplementation.cpp`, `LifecycleInterfaceConnector.cpp` | `createPersistentStoreRemoteStoreObject`, `createPackageManagerObject`, `createStorageManagerRemoteObject`, `createLifecycleManagerRemoteObject` |
| AM-L0-POS-010 | Fetch package list and installed-state checks | `AppManagerImplementation.cpp` | `fetchAppPackageList`, `checkIsInstalled`, `IsInstalled` |
| AM-L0-NEG-009 | Package list fetch fails when installer is missing or returns error | `AppManagerImplementation.cpp` | `fetchAppPackageList` |
| AM-L0-POS-011 | LaunchApp queues work and records launch telemetry | `AppManagerImplementation.cpp`, `AppManagerTelemetryReporting.cpp` | `LaunchApp`, `recordLaunchTime` |
| AM-L0-NEG-010 | LaunchApp rejects empty appId, not-installed apps, and missing connector | `AppManagerImplementation.cpp` | `LaunchApp` |
| AM-L0-POS-012 | PreloadApp queues work and records launch telemetry | `AppManagerImplementation.cpp`, `AppManagerTelemetryReporting.cpp` | `PreloadApp`, `recordLaunchTime` |
| AM-L0-NEG-011 | PreloadApp rejects empty appId and allocation failures | `AppManagerImplementation.cpp` | `PreloadApp` |
| AM-L0-POS-013 | Close/Terminate/Kill/SendIntent success paths | `AppManagerImplementation.cpp`, `LifecycleInterfaceConnector.cpp` | `CloseApp`, `TerminateApp`, `KillApp`, `SendIntent` |
| AM-L0-NEG-012 | Close/Terminate/Kill/SendIntent negative paths | `AppManagerImplementation.cpp`, `LifecycleInterfaceConnector.cpp` | `CloseApp`, `TerminateApp`, `KillApp`, `SendIntent` |
| AM-L0-POS-014 | GetLoadedApps returns parsed data and maps state correctly | `AppManagerImplementation.cpp`, `LifecycleInterfaceConnector.cpp` | `GetLoadedApps`, `LifecycleInterfaceConnector::getLoadedApps`, `mapAppLifecycleState` |
| AM-L0-NEG-013 | GetLoadedApps handles RPC failure, empty JSON, and malformed JSON | `AppManagerImplementation.cpp`, `LifecycleInterfaceConnector.cpp` | `GetLoadedApps`, `getLoadedApps` |
| AM-L0-POS-015 | GetInstalledApps emits timestamps and last-active indexes | `AppManagerImplementation.cpp` | `GetInstalledApps` |
| AM-L0-BND-002 | GetInstalledApps handles zero timestamp by producing empty lastActiveTime | `AppManagerImplementation.cpp` | `GetInstalledApps` |
| AM-L0-POS-016 | Property and storage APIs succeed | `AppManagerImplementation.cpp` | `GetAppProperty`, `SetAppProperty`, `ClearAppData`, `ClearAllAppData` |
| AM-L0-NEG-014 | Property and storage APIs fail on invalid inputs or missing dependencies | `AppManagerImplementation.cpp` | `GetAppProperty`, `SetAppProperty`, `ClearAppData`, `ClearAllAppData` |
| AM-L0-POS-017 | Event handlers dispatch lifecycle, launch, unload, and installation events | `AppManagerImplementation.cpp` | `handleOnAppLifecycleStateChanged`, `handleOnAppUnloaded`, `handleOnAppLaunchRequest`, `OnAppInstallationStatus` |
| AM-L0-NEG-015 | Event handlers reject empty IDs and invalid JSON | `AppManagerImplementation.cpp` | `handleOnAppLifecycleStateChanged`, `handleOnAppUnloaded`, `handleOnAppLaunchRequest`, `OnAppInstallationStatus`, `Dispatch` |
| AM-L0-POS-018 | Lifecycle state mapping and error mapping cover all known values | `LifecycleInterfaceConnector.cpp` | `mapAppLifecycleState`, `mapErrorReason` |
| AM-L0-BND-003 | Unknown state and unknown error map to default values | `LifecycleInterfaceConnector.cpp` | `mapAppLifecycleState`, `mapErrorReason` |
| AM-L0-POS-019 | App state change callbacks update state, timestamps, and telemetry | `LifecycleInterfaceConnector.cpp` | `OnAppLifecycleStateChanged`, `OnAppStateChanged` |
| AM-L0-NEG-016 | App state change callbacks handle unknown state, missing app, and invalid implementation instances | `LifecycleInterfaceConnector.cpp` | `OnAppLifecycleStateChanged`, `OnAppStateChanged` |
| AM-L0-POS-020 | App lifecycle actions succeed end-to-end via lifecycle connector | `LifecycleInterfaceConnector.cpp` | `launch`, `preLoadApp`, `closeApp`, `terminateApp`, `killApp`, `sendIntent` |
| AM-L0-NEG-017 | Lifecycle connector action methods fail correctly on missing app, missing remote object, and timeout/error branches | `LifecycleInterfaceConnector.cpp` | `launch`, `preLoadApp`, `closeApp`, `terminateApp`, `killApp`, `sendIntent`, `getLoadedApps` |
| AM-L0-POS-021 | Telemetry reporting positive paths | `AppManagerTelemetryReporting.cpp`, `AppManagerTelemetryReporting.h` | `getInstance`, `initialize`, `reportTelemetryData`, `reportTelemetryDataOnStateChange`, `reportTelemetryErrorData`, `reportAppCrashedTelemetry` |
| AM-L0-BND-004 | Telemetry disabled remains a no-op | `AppManagerTelemetryReporting.cpp` | `reportTelemetryData`, `reportTelemetryDataOnStateChange`, `reportTelemetryErrorData` |
| AM-L0-POS-022 | Update-current-action helpers and install/uninstall block detection | `AppManagerImplementation.cpp`, `LifecycleInterfaceConnector.cpp` | `updateCurrentAction`, `updateCurrentActionTime`, `checkInstallUninstallBlock` |
| AM-L0-NEG-018 | Helper methods handle missing app info and empty branches | `AppManagerImplementation.cpp`, `LifecycleInterfaceConnector.cpp` | `updateCurrentAction`, `updateCurrentActionTime`, `checkInstallUninstallBlock`, `removeAppInfoByAppId` |

---

## Detailed Test Cases

| Test ID | Type | Description | Inputs / Setup | Expected Result |
|---|---|---|---|---|
| AM-L0-POS-001 | Positive | Initialize succeeds with a valid service and valid plugin implementation objects | Mock `IShell` returns a valid `Root<Exchange::IAppManager>()`, the implementation exposes `IConfiguration`, `Configure()` returns `ERROR_NONE`, and listener registration calls succeed | `Initialize()` returns an empty string, `AddRef()` is called on the shell, notification registration occurs, JSON-RPC stub registration occurs, and the plugin is ready for use |
| AM-L0-NEG-001 | Negative | Initialize fails when `service` is null | Call `Initialize(nullptr)` | Returns an error string, logs the failure, and performs no partial setup |
| AM-L0-NEG-002 | Negative | Initialize fails when the root object cannot be created | Mock `Root<Exchange::IAppManager>()` to return `nullptr` | `Initialize()` returns the plugin initialisation error and triggers cleanup |
| AM-L0-NEG-003 | Negative | Initialize fails when the implementation does not expose `IConfiguration` | Root object exists, but `QueryInterface<IConfiguration>()` returns `nullptr` | `Initialize()` returns the configuration interface error and deinitializes |
| AM-L0-NEG-004 | Negative | Initialize fails when `Configure()` returns an error | Valid service and root object, but `Configure()` returns non-zero | `Initialize()` returns the configuration failure message and all acquired resources are cleaned up |
| AM-L0-POS-002 | Positive | Deinitialize releases all acquired references and unregisters callbacks | Initialize successfully, then call `Deinitialize()` with the same service | Notification sinks are unregistered, JSON-RPC stub is unregistered, configuration and implementation references are released, remote connection cleanup is triggered when present, and member pointers are reset |
| AM-L0-POS-003 | Positive | Information returns an empty string | Call `Information()` on a live plugin instance | An empty string is returned |
| AM-L0-NEG-005 | Negative | Deactivated ignores non-matching connection ids | Call `Deactivated()` with a remote connection id that does not match the stored connection id | No worker job is queued and no deactivation action is triggered |
| AM-L0-POS-004 | Positive | Deactivated submits a DEACTIVATED job for the matching connection | Call `Deactivated()` with a connection id equal to the plugin’s stored connection id | A deactivation job is submitted to the worker pool with `DEACTIVATED` and `FAILURE` states |
| AM-L0-POS-005 | Positive | Notification forwards all AppManager events | Trigger each callback on the inner `Notification` object: installed, uninstalled, lifecycle changed, launch request, unloaded | JSON-RPC events are forwarded to `JAppManager` event emitters with the same payload |
| AM-L0-POS-006 | Positive | AppInfo defaults and accessors behave correctly | Construct `AppInfo`, then set and read app instance id, session id, package info, states, intent, timestamps, and current action | All getters return the values that were written, and default construction yields unloaded/empty state |
| AM-L0-POS-007 | Positive | AppInfoManager supports CRUD and convenience accessors | Use `upsert()`, `get()`, `exists()`, `update()`, `remove()`, and `clear()` with a sample app id | The map behaves deterministically, convenience getters/setters reflect the stored values, and removed entries disappear |
| AM-L0-POS-008 | Positive | Register and Unregister manage notification observers correctly | Register a valid notification, then unregister it | `Register()` returns `ERROR_NONE`, `Unregister()` returns `ERROR_NONE`, and refcount bookkeeping is correct |
| AM-L0-BND-001 | Boundary | Register is idempotent for duplicate observer pointers | Register the same notification twice | The second register call returns `ERROR_NONE` and does not duplicate the observer entry |
| AM-L0-NEG-006 | Negative | Unregister returns error when the notification is missing | Call `Unregister()` with an observer that was never registered | Returns `ERROR_GENERAL` and logs the missing observer path |
| AM-L0-POS-009 | Positive | Configure creates and registers all required remote objects | Valid service with mocks for `LifecycleManager`, `PersistentStore`, `AppPackageManager`, and `AppStorageManager` | Lifecycle connector is created, remote objects are acquired, notifications are registered, telemetry is initialized, and the worker thread is started |
| AM-L0-NEG-007 | Negative | Configure fails when `service` is null | Call `Configure(nullptr)` | Returns `ERROR_GENERAL` and no dependent objects are created |
| AM-L0-NEG-008 | Negative | Remote-object creation failure branches are exercised | Force `QueryInterfaceByCallsign()` to fail for persistent store, package manager, storage manager, or lifecycle manager | The matching error branch is hit, the method logs the failure, and the plugin remains in a safe state |
| AM-L0-POS-010 | Positive | Package list fetch and installed-state lookup succeed | Mock package installer returns a populated package iterator containing installed and non-installed packages | `fetchAppPackageList()` returns the package vector, `checkIsInstalled()` correctly identifies installed apps, and `IsInstalled()` returns `ERROR_NONE` |
| AM-L0-NEG-009 | Negative | Package list fetch fails | Make the package installer return an error or a null iterator | `fetchAppPackageList()` returns an error and downstream callers fail safely |
| AM-L0-POS-011 | Positive | LaunchApp queues a launch request and records launch telemetry | Valid installed app, valid lifecycle connector, and valid request allocation | Request is enqueued, `recordLaunchTime()` is called, and `LaunchApp()` returns `ERROR_NONE` |
| AM-L0-NEG-010 | Negative | LaunchApp rejects invalid inputs | Use empty app id, an app that is not installed, or a null lifecycle connector | The method returns the expected error and no request is queued |
| AM-L0-POS-012 | Positive | PreloadApp queues a preload request and records launch telemetry | Valid app id and connector, request allocation succeeds | Request is enqueued, `recordLaunchTime()` is called, and `PreloadApp()` returns `ERROR_NONE` |
| AM-L0-NEG-011 | Negative | PreloadApp rejects invalid inputs and allocation failure | Use empty app id or force the request allocation path to fail | The method returns the expected error and populates the error string when applicable |
| AM-L0-POS-013 | Positive | CloseApp, TerminateApp, KillApp, and SendIntent succeed | Pre-populate `AppInfoManager` and mock lifecycle connector success responses | Each method returns `ERROR_NONE` and updates current action telemetry/state as expected |
| AM-L0-NEG-012 | Negative | Close/Terminate/Kill/SendIntent reject missing or invalid runtime conditions | Use empty app id, missing app instance mapping, missing lifecycle object, or connector failure responses | The method returns the expected error and logs the failure branch |
| AM-L0-POS-014 | Positive | GetLoadedApps parses loaded-app JSON into API output | Mock lifecycle manager to return a valid JSON array with app ids, instance ids, session ids, and lifecycle states | Returned iterator contains the expected entries and `AppInfoManager` is updated |
| AM-L0-NEG-013 | Negative | GetLoadedApps handles malformed or empty JSON | Force lifecycle manager to return an error, an empty string, or invalid JSON text | The method returns an error and does not populate the iterator |
| AM-L0-POS-015 | Positive | GetInstalledApps emits formatted timestamps and last-active indexes | Populate `AppInfoManager` with a valid `timespec` and installed package list | Output JSON contains `lastActiveTime` and `lastActiveIndex` fields |
| AM-L0-BND-002 | Boundary | GetInstalledApps handles a zero timestamp | Use a package entry where `tv_sec == 0` and `tv_nsec == 0` | `lastActiveTime` is emitted as an empty string and the call does not fail |
| AM-L0-POS-016 | Positive | Property and storage APIs work with valid dependencies | Mock persistent store and storage manager responses to succeed | `GetAppProperty()`, `SetAppProperty()`, `ClearAppData()`, and `ClearAllAppData()` return `ERROR_NONE` |
| AM-L0-NEG-014 | Negative | Property and storage APIs reject invalid inputs or missing dependencies | Use empty app id, empty key, or null remote object pointers | The method returns an error and logs the appropriate guard path |
| AM-L0-POS-017 | Positive | Event handlers dispatch lifecycle, launch, unload, and installation notifications | Call `handleOnAppLifecycleStateChanged()`, `handleOnAppUnloaded()`, `handleOnAppLaunchRequest()`, and `OnAppInstallationStatus()` with valid payloads | The corresponding event dispatch path is triggered and observers receive callbacks |
| AM-L0-NEG-015 | Negative | Event handlers reject empty IDs and malformed JSON | Pass empty app ids, missing fields, or invalid JSON strings | Error logs are produced and no notification is emitted |
| AM-L0-POS-018 | Positive | Lifecycle state mapping covers all known lifecycle states | Call `mapAppLifecycleState()` with `UNLOADED`, `LOADING`, `ACTIVE`, `PAUSED`, `INITIALIZING`, `SUSPENDED`, `TERMINATING`, and `HIBERNATED` | Each input maps to the expected `AppLifecycleState` value |
| AM-L0-BND-003 | Boundary | Unknown lifecycle state and unknown error reason map to defaults | Pass a state or error string that is not recognized | The mapper returns `APP_STATE_UNKNOWN` or `APP_ERROR_UNKNOWN` as appropriate |
| AM-L0-POS-019 | Positive | App state change callbacks update state and telemetry | Deliver lifecycle state transitions for ACTIVE, PAUSED, SUSPENDED, HIBERNATED, and UNLOADED | AppInfo state is updated, timestamps/indexes are refreshed, and telemetry reporting is triggered |
| AM-L0-NEG-016 | Negative | App state change callbacks handle unknown state or missing app data | Trigger callbacks with unknown old/new state or an app id not present in `AppInfoManager` | The callback exits safely and logs the skip or error path |
| AM-L0-POS-020 | Positive | Lifecycle connector action methods work end-to-end | Exercise `launch()`, `preLoadApp()`, `closeApp()`, `terminateApp()`, `killApp()`, and `sendIntent()` with valid data and mocked lifecycle manager success | Each method returns `ERROR_NONE` and updates `AppInfoManager` state consistently |
| AM-L0-NEG-017 | Negative | Lifecycle connector action methods fail on edge and error conditions | Force missing app instance ids, connector creation failure, spawn failure, unload failure, timeout waiting for PAUSED, or malformed loaded-app data | Each method returns the expected failure code and logs the correct error branch |
| AM-L0-POS-021 | Positive | Telemetry reporting publishes data on success paths | Enable telemetry, populate `AppInfoManager`, and use valid actions/state transitions | `reportTelemetryData()`, `reportTelemetryDataOnStateChange()`, `reportTelemetryErrorData()`, and `reportAppCrashedTelemetry()` publish markers and payloads |
| AM-L0-BND-004 | Boundary | Telemetry disabled behaves as a no-op | Disable telemetry metrics and call reporting functions | Methods return without publishing or logging errors |
| AM-L0-POS-022 | Positive | Current-action helpers and install/uninstall blocking logic behave correctly | Call `updateCurrentAction()`, `updateCurrentActionTime()`, and `checkInstallUninstallBlock()` with valid app data and package lists | The helper methods update the tracked action, timestamp, and block status as expected |
| AM-L0-NEG-018 | Negative | Helper methods handle missing entries and empty branches | Call helper methods for app ids not present in `AppInfoManager` or with package lists missing the app | The methods return or log the appropriate not-found behavior without crashing |

---

## Coverage Matrix by File

| File | Planned Coverage Focus | Expected Coverage |
|---|---|---|
| `AppManager/AppManager.cpp` | Initialize/Deinitialize, Information, Deactivated, constructor/destructor | 80-90% actionable line coverage |
| `AppManager/AppManager.h` | Inner notification forwarding methods, interface map usage via instantiated plugin | 75-85% actionable coverage |
| `AppManager/AppManagerImplementation.cpp` | Register/Unregister, Configure, package queries, launch/preload/close/terminate/kill, property and storage APIs, event dispatch, helper functions | 85%+ actionable coverage |
| `AppManager/AppManagerImplementation.h` | Public API declarations, helper inline paths, Job wrapper construction/dispatch | 75-80% actionable coverage |
| `AppManager/AppManagerTelemetryReporting.cpp` | Positive and negative telemetry reporting flows | 75-85% actionable coverage |
| `AppManager/AppManagerTelemetryReporting.h` | Singletons and public API declarations, inline usage through tests | 75%+ for directly executable declarations |
| `AppManager/AppInfo.cpp` | Default construction, getters, setters | 100% practical coverage |
| `AppManager/AppInfo.h` | Inline accessors and mutators via direct tests | 95%+ practical coverage |
| `AppManager/AppInfoManager.cpp` | Singleton access, CRUD, convenience getters/setters | 95%+ practical coverage |
| `AppManager/AppInfoManager.h` | API declarations and any inline usage | 90%+ practical coverage |
| `AppManager/LifecycleInterfaceConnector.cpp` | Remote object creation, app lifecycle actions, state callbacks, mappings, error handling | 80-85% actionable coverage |
| `AppManager/LifecycleInterfaceConnector.h` | Notification handlers and interface map related inline methods | 75-80% actionable coverage |
| `AppManager/Module.cpp` | Static module declaration | Non-actionable runtime coverage |
| `AppManager/Module.h` | Module macros | Non-actionable runtime coverage |
| `AppManager/AppManagerTypes.h` | Type aliases and enum declarations | Header-only, compile-time coverage only |

### Aggregate expectation

- Actionable `.cpp` files: **80%+ line coverage**
- Actionable `.h` files with executable code: **75%+ line coverage**
- Plugin total: **minimum 75% satisfied** if the test matrix above is implemented

---

## Methods Not Fully Covered in L0 and Why

The following methods or regions are expected to remain partially covered, or are treated as non-actionable for L0:

| File | Method / Area | Reason | Recommendation |
|---|---|---|---|
| `AppManager/Module.cpp` | `MODULE_NAME_DECLARATION(BUILD_REFERENCE)` | Static module metadata line is compile-time glue and not a stable runtime target | Exclude from runtime coverage gate |
| `AppManager/Module.h` | Module macro definitions | Header-only macro declarations with no meaningful runtime path | Exclude from runtime coverage gate |
| `AppManager/AppManager.h` | `BEGIN_INTERFACE_MAP`, `INTERFACE_ENTRY`, `INTERFACE_AGGREGATE` expansions | Generated interface dispatch code is not directly invoked by the test harness | Validate indirectly through plugin creation and interface queries |
| `AppManager/AppManager.h` | `Notification::Activated()` | Pure notification callback with no functional side effect beyond logging | Can be sanity-checked indirectly, but not a high-value L0 target |
| `AppManager/AppManager.h` | `Notification::QueryInterface(uint32_t)` dispatch on the inner class | COM-RPC internal dispatch path; not directly exposed for L0 invocation | Accept as macro/ABI coverage |
| `AppManager/AppManagerImplementation.h` | `BEGIN_INTERFACE_MAP` / `INTERFACE_ENTRY` expansions | Macro-generated ABI glue | Accept as non-actionable |
| `AppManager/AppManagerImplementation.h` | `Job` ABI destructor variants | Compiler-emitted alternate symbols | Accept as non-actionable |
| `AppManager/LifecycleInterfaceConnector.h` | `BEGIN_INTERFACE_MAP` / `INTERFACE_ENTRY` expansions | Macro-generated ABI glue | Accept as non-actionable |
| `AppManager/LifecycleInterfaceConnector.h` | Inner notification `QueryInterface(uint32_t)` | Internal COM-RPC lookup path | Accept as non-actionable |
| `AppManager/AppManagerTelemetryReporting.cpp` | Branches that depend on telemetry being disabled by configuration | These are highly environment-dependent unless telemetry mocks are provided | Cover with explicit telemetry enable/disable tests where feasible |
| `AppManager/AppManagerImplementation.cpp` | `getCustomValues()` file-read branch for `/tmp/aipath` | Requires a deterministic temp file fixture or filesystem mock | Cover in component tests using a temp file in `/tmp` |
| `AppManager/LifecycleInterfaceConnector.cpp` | Rare timeout and recovery branches in pause/close and unload flows | Require orchestrated asynchronous state changes | Cover with explicit synchronization tests; otherwise keep as low priority |
| `AppManager/LifecycleInterfaceConnector.cpp` | `OnAppStateChanged()` invalid implementation-instance guard | Requires a null singleton/forced teardown scenario | Include one negative test to exercise the guard |

---

## How L0 Code Should Be Generated Using `Tests/L0Tests/PreinstallManager` as Reference

### 1) Mirror the test layout

Use the same structure as `PreinstallManager`:

- one main runner file
- one lifecycle test file
- one implementation test file
- one component test file
- one utility/model test file for `AppInfo` and `AppInfoManager`
- optional telemetry-specific test file

### 2) Reuse the bootstrap and assertion helpers

Reuse the same L0 bootstrap and assertion style:

- `Tests/L0Tests/common/L0Bootstrap.hpp`
- `Tests/L0Tests/common/L0Expect.hpp`
- `Tests/L0Tests/common/L0TestTypes.hpp`

### 3) Build a dedicated `ServiceMock`

Create a mock shell modeled after the PreinstallManager test shell, but extended for AppManager needs:

- `ConfigLine()` returns plugin configuration JSON
- `Root<Exchange::IAppManager>()` returns the wrapper implementation
- `QueryInterfaceByCallsign()` returns mocks for:
  - `org.rdk.LifecycleManager`
  - `org.rdk.LifecycleManagerState`
  - `org.rdk.AppPackageManager`
  - `org.rdk.PersistentStore`
  - `org.rdk.AppStorageManager`
- counters for `AddRef()`, `Release()`, `Register()`, `Unregister()`, and `RemoteConnection()` calls

### 4) Add fake implementations for each external dependency

Create deterministic fake classes for the remote interfaces used by AppManager:

- `FakeLifecycleManager`
- `FakeLifecycleManagerState`
- `FakePackageInstaller`
- `FakePackageHandler`
- `FakePersistentStore`
- `FakeStorageManager`
- `FakeTelemetryClient` or telemetry wrapper stub

These should expose per-test handlers so each negative and positive branch can be forced explicitly.

### 5) Split tests by concern

Follow the same organization used in `PreinstallManager`:

- **Lifecycle tests**: `Initialize`, `Deinitialize`, `Information`, `Deactivated`
- **Implementation tests**: `Register`, `Unregister`, `Configure`, package lookup, property APIs, and helper methods
- **Component tests**: request queueing, JSON parsing, lifecycle actions, state handling, and timeout/retry flows
- **Model tests**: `AppInfo` and `AppInfoManager`
- **Telemetry tests**: success, disabled, and error publication paths

### 6) Use deterministic fixtures for files and state

For file-dependent branches such as `getCustomValues()`:

- create a temporary `/tmp/aipath` file during the test
- write the expected three lines: app path, runtime path, and command
- remove the file after the test

For timestamp-dependent branches:

- pre-populate `AppInfoManager` with controlled `timespec` values
- explicitly test zero-time and non-zero-time cases

### 7) Add coverage checkpoints

After the tests are implemented, validate coverage against the 75% target using the existing coverage workflow used by the repository.

Recommended checkpoints:

- `AppManager.cpp` and `AppManager.h` should both exceed 75%
- `AppManagerImplementation.cpp` should exceed 85%
- `LifecycleInterfaceConnector.cpp` should exceed 80%
- `AppInfo.cpp`, `AppInfo.h`, `AppInfoManager.cpp`, and `AppInfoManager.h` should be near complete

---

## Suggested Test File Ownership Map

| Test File | Primary Area | Target Methods |
|---|---|---|
| `AppManagerTest.cpp` | Test runner / bootstrap | main execution flow |
| `AppManager_LifecycleTests.cpp` | Plugin wrapper lifecycle | `Initialize`, `Deinitialize`, `Information`, `Deactivated`, notification forwarding |
| `AppManager_ImplementationTests.cpp` | Core plugin APIs | `Register`, `Unregister`, `Configure`, `LaunchApp`, `PreloadApp`, `CloseApp`, `TerminateApp`, `KillApp`, `SendIntent`, `GetLoadedApps`, `GetInstalledApps`, property/storage APIs |
| `AppManager_ComponentTests.cpp` | Event dispatch and helper behavior | `handleOnAppLifecycleStateChanged`, `handleOnAppUnloaded`, `handleOnAppLaunchRequest`, `OnAppInstallationStatus`, `checkInstallUninstallBlock`, `updateCurrentAction`, `updateCurrentActionTime` |
| `AppManagerTelemetryTests.cpp` | Telemetry marker/reporting paths | `AppManagerTelemetryReporting` public API |
| `AppInfoTests.cpp` | Model object behavior | `AppInfo` getters/setters/defaults |
| `AppInfoManagerTests.cpp` | Singleton registry behavior | `AppInfoManager` CRUD and convenience methods |

---

## Exit Criteria

- All cases in the table above are implemented and passing
- No leaks or stale references remain after plugin deinitialization
- Coverage is at least 75% across actionable `.cpp` and `.h` code
- Any remaining uncovered methods are listed in this document with rationale
- L0 structure follows the same test organization and bootstrap style as `Tests/L0Tests/PreinstallManager`

---

## Open Implementation Notes

- `AppInfoManager` and `AppInfo` should be tested directly, not only through the main plugin wrapper, because they carry a large portion of the plugin’s state logic.
- `LifecycleInterfaceConnector` needs explicit synchronization helpers for pause/close timeout branches.
- `AppManagerTelemetryReporting` should be isolated with a telemetry mock so that report methods can be tested without requiring a live telemetry backend.
- `Module.cpp` / `Module.h` should remain excluded from runtime coverage expectations.
