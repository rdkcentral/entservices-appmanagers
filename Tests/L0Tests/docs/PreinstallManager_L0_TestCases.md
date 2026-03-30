# PreinstallManager — L0 Unit Test Cases

> **Branch:** `topic/L0_preinstall`  
> **Plugin sources:** `PreinstallManager/`  
> **Reference L0 pattern:** `ref/entservices-appgateway/Tests/L0Tests/`  
> **Target coverage:** ≥ 75 % line/function across all `.cpp` and `.h` files

---

## Table of Contents

1. [How L0 Test Code Is Generated](#how-l0-test-code-is-generated)
2. [Files Under Test](#files-under-test)
3. [Quick-Reference Table](#quick-reference-table)
4. [Test Cases — PreinstallManager (Plugin Wrapper)](#test-cases--preinstallmanager-plugin-wrapper)
5. [Test Cases — PreinstallManagerImplementation (Core Logic)](#test-cases--preinstallmanagerimplementation-core-logic)
6. [Test Cases — Module](#test-cases--module)
7. [Coverage Estimate](#coverage-estimate)
8. [Methods Not Covered at L0 and Rationale](#methods-not-covered-at-l0-and-rationale)

---

## How L0 Test Code Is Generated

### What is an L0 Test?

An **L0 test** (Layer-0) is a fully *in-process*, *unit-level* test. No live Thunder daemon, no sockets, no OS-level services. Every external dependency (e.g. `PluginHost::IShell`, `IPackageInstaller`) is replaced by a hand-written *fake/mock* in the test translation unit. The tests link directly against the plugin source files, not against the installed `.so`, and run as a standalone executable.

### Pattern adopted from `entservices-appgateway`

The reference project (`ref/entservices-appgateway/Tests/L0Tests/`) provides a mature, repeatable pattern. The same pattern is re-used here:

```
Tests/L0Tests/
├── CMakeLists.txt              # standalone build; links plugin sources directly
├── common/
│   ├── L0Bootstrap.hpp/cpp     # creates/destroys Core::WorkerPool for the process
│   ├── L0Expect.hpp            # ExpectTrue / ExpectEqU32 / ExpectEqStr helpers
│   ├── L0ServiceMock.hpp       # callsign → interface registry base class
│   └── L0TestTypes.hpp         # TestResult { uint32_t failures; }
└── PreinstallManager/
    ├── ServiceMock.h           # IShell + IPackageInstaller fakes
    ├── PreinstallManagerTest.cpp               # main() + test registry
    ├── PreinstallManager_LifecycleTests.cpp    # Initialize / Deinitialize
    └── PreinstallManagerImpl_Tests.cpp         # Implementation business logic
```

#### Step-by-step code generation guide

| Step | Action | Reference file |
|------|--------|---------------|
| 1 | Copy `common/` verbatim from the RuntimeManager L0 tests (already exists in `Tests/L0Tests/common/`). | `common/L0Bootstrap.cpp` |
| 2 | Create `PreinstallManager/ServiceMock.h` that implements `PluginHost::IShell` (for `Initialize`) and a fake `IPackageInstaller` (for `StartPreinstall`). Model on `RuntimeManager/ServiceMock.h` and `appgateway/Tests/L0Tests/AppGateway/ServiceMock.h`. | `RuntimeManager/ServiceMock.h` |
| 3 | Create `PreinstallManagerTest.cpp` with `main()`: call `L0Test::L0Init()`, run each `Test_*` function, accumulate failures, call `L0Test::L0Shutdown()`, return the total count. | `AppGateway/AppGatewayTest.cpp` |
| 4 | Add `PreinstallManager_LifecycleTests.cpp` covering `Initialize` / `Deinitialize`. Each test function signature is `uint32_t Test_<Name>()` and returns `tr.failures`. | `AppGateway/AppGateway_Init_DeinitTests.cpp` |
| 5 | Add `PreinstallManagerImpl_Tests.cpp` covering `Configure`, `Register`, `Unregister`, `StartPreinstall`, helper methods. | `AppGateway/AppGatewayImplementation_BranchTests.cpp` |
| 6 | Update `Tests/L0Tests/CMakeLists.txt`: add `PreinstallManager/*.cpp` and both plugin source files to `APPMANAGERS_L0_SOURCES`. Add required compile definitions (`PREINSTALL_MANAGER_API_VERSION_NUMBER_*`). | `Tests/L0Tests/CMakeLists.txt` |
| 7 | Build and run with coverage flags (`-O0 -g --coverage`), then generate HTML report with `lcov`/`genhtml`. Iterate until ≥ 75 % line coverage is reached. | `Tests/L0Tests/CMakeLists.txt` (`APPMGR_ENABLE_COVERAGE`) |

#### Fake / Mock construction rules

- **No gmock.** All fakes are plain C++ classes with `std::atomic<uint32_t> _refCount`.
- **`IShell` fake** must implement at minimum: `ConfigLine()` (returns a JSON string), `Register/Unregister(IRemoteConnection::INotification*)`, `QueryInterfaceByCallsign<T>(callsign)`, `RemoteConnection(id)`, `IShell::ICOMLink::Instantiate(...)`.
- **`IPackageInstaller` fake** must implement: `ListPackages(IPackageIterator*&)`, `GetConfigForPackage(...)`, `Install(...)`, `Register/Unregister(INotification*)`.
- The fake `Instantiate` handler (in `IShell::ICOMLink`) is used in `Initialize` tests to return a `PreinstallManagerImplementation` in-proc instance (same trick used in RuntimeManager lifecycle tests).

#### `ExpectTrue` / `ExpectEqU32` usage example

```cpp
// uint32_t Test_Impl_RegisterNotification()
uint32_t Test_Impl_RegisterNotification()
{
    TestResult tr;
    auto* impl = WPEFramework::Core::Service<PreinstallManagerImplementation>
                     ::Create<Exchange::IPreinstallManager>();
    NotificationFake fake;
    ExpectEqU32(tr, impl->Register(&fake), Core::ERROR_NONE, "Register returns ERROR_NONE");
    impl->Unregister(&fake);
    impl->Release();
    return tr.failures;
}
```

---

## Files Under Test

| File | Lines (approx.) | Key responsibilities |
|------|----------------|---------------------|
| `PreinstallManager.h` | ~120 | Plugin class, `Notification` inner class, public API declarations |
| `PreinstallManager.cpp` | ~190 | `Initialize`, `Deinitialize`, `Information`, `Deactivated` |
| `PreinstallManagerImplementation.h` | ~200 | `PackageInfo` struct, `PackageManagerNotification`, `Job`, public method declarations |
| `PreinstallManagerImplementation.cpp` | ~490 | `Configure`, `Register`, `Unregister`, `StartPreinstall`, helpers (`isNewerVersion`, `readPreinstallDirectory`, `getFailReason`, `createPackageManagerObject`, `releasePackageManagerObject`, `dispatchEvent`, `Dispatch`, `handleOnAppInstallationStatus`) |
| `Module.h` | ~30 | `MODULE_NAME` macro |
| `Module.cpp` | ~22 | `MODULE_NAME_DECLARATION` |

---

## Quick-Reference Table

| ID | Test Name | File | Methods Covered | Scenario Type | Expected Result |
|----|-----------|------|-----------------|---------------|-----------------|
| TC-PM-001 | `Test_Initialize_WithValidService_Succeeds` | `PreinstallManager_LifecycleTests.cpp` | `Initialize`, `Deinitialize` | Positive | `Initialize` returns empty string |
| TC-PM-002 | `Test_Initialize_WithNullService_ReturnsError` | `PreinstallManager_LifecycleTests.cpp` | `Initialize` | Negative | Returns non-empty error string |
| TC-PM-003 | `Test_Initialize_WhenImplCreationFails_ReturnsError` | `PreinstallManager_LifecycleTests.cpp` | `Initialize` | Negative | Returns error; `Deinitialize` called internally |
| TC-PM-004 | `Test_Initialize_WhenConfigureInterfaceMissing_ReturnsError` | `PreinstallManager_LifecycleTests.cpp` | `Initialize` | Negative | Returns error re. configure interface |
| TC-PM-005 | `Test_Initialize_WhenConfigureFails_ReturnsError` | `PreinstallManager_LifecycleTests.cpp` | `Initialize` | Negative | Returns "could not be configured" error |
| TC-PM-006 | `Test_Initialize_Twice_Idempotent` | `PreinstallManager_LifecycleTests.cpp` | `Initialize`, `Deinitialize` | Boundary | No crash, second call on fresh instance |
| TC-PM-007 | `Test_Deinitialize_ReleasesAllResources` | `PreinstallManager_LifecycleTests.cpp` | `Deinitialize` | Positive | Connection terminated, refs released |
| TC-PM-008 | `Test_Information_ReturnsEmptyString` | `PreinstallManager_LifecycleTests.cpp` | `Information` | Positive | Returns empty string |
| TC-PM-009 | `Test_Deactivated_SubmitsDeactivateJob` | `PreinstallManager_LifecycleTests.cpp` | `Deactivated` | Positive | Worker pool job submitted |
| TC-PM-010 | `Test_Notification_OnAppInstallationStatus_ForwardsEvent` | `PreinstallManager_LifecycleTests.cpp` | `Notification::OnAppInstallationStatus` | Positive | `JPreinstallManager::Event::OnAppInstallationStatus` fired |
| TC-PM-011 | `Test_Notification_Activated_DoesNotCrash` | `PreinstallManager_LifecycleTests.cpp` | `Notification::Activated` | Positive | No crash |
| TC-PM-012 | `Test_Notification_Deactivated_TriggersParentDeactivated` | `PreinstallManager_LifecycleTests.cpp` | `Notification::Deactivated` | Positive | Parent `Deactivated` called |
| TC-PI-001 | `Test_Impl_Configure_WithNullService_ReturnsError` | `PreinstallManagerImpl_Tests.cpp` | `Configure` | Negative | Returns `Core::ERROR_GENERAL` |
| TC-PI-002 | `Test_Impl_Configure_WithValidService_ReturnsNone` | `PreinstallManagerImpl_Tests.cpp` | `Configure` | Positive | Returns `Core::ERROR_NONE`, `mCurrentservice` set |
| TC-PI-003 | `Test_Impl_Configure_WithAppPreinstallDirectoryInConfig` | `PreinstallManagerImpl_Tests.cpp` | `Configure` | Positive | `mAppPreinstallDirectory` set from JSON config |
| TC-PI-004 | `Test_Impl_Configure_WithEmptyPreinstallDirectory` | `PreinstallManagerImpl_Tests.cpp` | `Configure` | Boundary | Directory stays empty string; no error |
| TC-PI-005 | `Test_Impl_Register_ValidNotification_ReturnsNone` | `PreinstallManagerImpl_Tests.cpp` | `Register` | Positive | Returns `Core::ERROR_NONE` |
| TC-PI-006 | `Test_Impl_Register_DuplicateNotification_NotAddedTwice` | `PreinstallManagerImpl_Tests.cpp` | `Register` | Boundary | List size stays 1; returns `Core::ERROR_NONE` |
| TC-PI-007 | `Test_Impl_Register_NullNotification_Asserts` | `PreinstallManagerImpl_Tests.cpp` | `Register` | Negative | `ASSERT` triggers (NDEBUG: undefined behaviour) |
| TC-PI-008 | `Test_Impl_Unregister_RegisteredNotification_ReturnsNone` | `PreinstallManagerImpl_Tests.cpp` | `Unregister` | Positive | Returns `Core::ERROR_NONE`; list empty |
| TC-PI-009 | `Test_Impl_Unregister_NotRegistered_ReturnsGeneral` | `PreinstallManagerImpl_Tests.cpp` | `Unregister` | Negative | Returns `Core::ERROR_GENERAL` |
| TC-PI-010 | `Test_Impl_Unregister_AfterDuplicate_CorrectRefCount` | `PreinstallManagerImpl_Tests.cpp` | `Unregister`, `Register` | Boundary | `Release` called once; no double-free |
| TC-PI-011 | `Test_Impl_GetInstance_ReturnsConstructedInstance` | `PreinstallManagerImpl_Tests.cpp` | `getInstance` | Positive | Returns non-null pointer matching `_instance` |
| TC-PI-012 | `Test_Impl_Destructor_ReleasesCurrentService` | `PreinstallManagerImpl_Tests.cpp` | `~PreinstallManagerImplementation` | Positive | `service->Release()` called in dtor |
| TC-PI-013 | `Test_Impl_Destructor_ReleasesPackageManager` | `PreinstallManagerImpl_Tests.cpp` | `~PreinstallManagerImplementation` | Positive | `releasePackageManagerObject` called when object non-null |
| TC-PI-014 | `Test_Impl_StartPreinstall_WhenPkgManagerCreationFails_ReturnsError` | `PreinstallManagerImpl_Tests.cpp` | `StartPreinstall`, `createPackageManagerObject` | Negative | Returns `Core::ERROR_GENERAL` |
| TC-PI-015 | `Test_Impl_StartPreinstall_WhenDirectoryReadFails_ReturnsError` | `PreinstallManagerImpl_Tests.cpp` | `StartPreinstall`, `readPreinstallDirectory` | Negative | Returns `Core::ERROR_GENERAL` (dir invalid) |
| TC-PI-016 | `Test_Impl_StartPreinstall_EmptyDirectory_NoInstallCalls` | `PreinstallManagerImpl_Tests.cpp` | `StartPreinstall` | Boundary | Returns `Core::ERROR_NONE`; zero `Install` calls |
| TC-PI-017 | `Test_Impl_StartPreinstall_ForceInstall_InstallsAllPackages` | `PreinstallManagerImpl_Tests.cpp` | `StartPreinstall` | Positive | `forceInstall=true`; `Install` called for each package |
| TC-PI-018 | `Test_Impl_StartPreinstall_NoForceInstall_SkipsAlreadyInstalledSameVersion` | `PreinstallManagerImpl_Tests.cpp` | `StartPreinstall`, `isNewerVersion` | Positive | Package with same version filtered out |
| TC-PI-019 | `Test_Impl_StartPreinstall_NoForceInstall_InstallsNewerVersion` | `PreinstallManagerImpl_Tests.cpp` | `StartPreinstall`, `isNewerVersion` | Positive | Newer package in dir replaces old installed version |
| TC-PI-020 | `Test_Impl_StartPreinstall_NoForceInstall_SkipsOlderVersion` | `PreinstallManagerImpl_Tests.cpp` | `StartPreinstall`, `isNewerVersion` | Positive | Older package in dir skipped |
| TC-PI-021 | `Test_Impl_StartPreinstall_ListPackagesFails_ReturnsError` | `PreinstallManagerImpl_Tests.cpp` | `StartPreinstall` | Negative | `ListPackages` returns error; `StartPreinstall` returns `Core::ERROR_GENERAL` |
| TC-PI-022 | `Test_Impl_StartPreinstall_NullPackageList_ReturnsError` | `PreinstallManagerImpl_Tests.cpp` | `StartPreinstall` | Negative | `packageList` is null after `ListPackages`; returns `Core::ERROR_GENERAL` |
| TC-PI-023 | `Test_Impl_StartPreinstall_InstallFails_ReturnsError` | `PreinstallManagerImpl_Tests.cpp` | `StartPreinstall`, `getFailReason` | Negative | `Install` returns non-NONE; `pkg.installStatus` contains "FAILED" |
| TC-PI-024 | `Test_Impl_StartPreinstall_PackageWithEmptyFields_Skipped` | `PreinstallManagerImpl_Tests.cpp` | `StartPreinstall` | Boundary | Package with empty `packageId`/`version` is skipped; counted as failed |
| TC-PI-025 | `Test_Impl_StartPreinstall_ReleasesPackageManagerAfterInstall` | `PreinstallManagerImpl_Tests.cpp` | `StartPreinstall`, `releasePackageManagerObject` | Positive | `mPackageManagerInstallerObject` is null after completion |
| TC-PI-026 | `Test_Impl_StartPreinstall_MultiplePackages_AllInstalled` | `PreinstallManagerImpl_Tests.cpp` | `StartPreinstall` | Positive | All valid packages installed; returns `Core::ERROR_NONE` |
| TC-PI-027 | `Test_Impl_StartPreinstall_MixedSuccessAndFailure_ReturnsError` | `PreinstallManagerImpl_Tests.cpp` | `StartPreinstall` | Boundary | Some pass, some fail; overall returns `Core::ERROR_GENERAL` |
| TC-PI-028 | `Test_Impl_IsNewerVersion_NewerMajor_ReturnsTrue` | `PreinstallManagerImpl_Tests.cpp` | `isNewerVersion` | Positive | `"2.0.0"` > `"1.9.9"` → `true` |
| TC-PI-029 | `Test_Impl_IsNewerVersion_OlderMajor_ReturnsFalse` | `PreinstallManagerImpl_Tests.cpp` | `isNewerVersion` | Negative | `"1.0.0"` vs `"2.0.0"` → `false` |
| TC-PI-030 | `Test_Impl_IsNewerVersion_EqualVersions_ReturnsFalse` | `PreinstallManagerImpl_Tests.cpp` | `isNewerVersion` | Boundary | `"1.2.3"` vs `"1.2.3"` → `false` |
| TC-PI-031 | `Test_Impl_IsNewerVersion_NewerMinor_ReturnsTrue` | `PreinstallManagerImpl_Tests.cpp` | `isNewerVersion` | Positive | `"1.3.0"` > `"1.2.9"` → `true` |
| TC-PI-032 | `Test_Impl_IsNewerVersion_NewerPatch_ReturnsTrue` | `PreinstallManagerImpl_Tests.cpp` | `isNewerVersion` | Positive | `"1.2.4"` > `"1.2.3"` → `true` |
| TC-PI-033 | `Test_Impl_IsNewerVersion_NewerBuild_ReturnsTrue` | `PreinstallManagerImpl_Tests.cpp` | `isNewerVersion` | Positive | `"1.2.3.5"` > `"1.2.3.4"` → `true` |
| TC-PI-034 | `Test_Impl_IsNewerVersion_WithPreReleaseSeparator_IgnoredInComparison` | `PreinstallManagerImpl_Tests.cpp` | `isNewerVersion` | Boundary | `"1.2.3-alpha"` treated as `"1.2.3"` |
| TC-PI-035 | `Test_Impl_IsNewerVersion_WithBuildMetaSeparator_IgnoredInComparison` | `PreinstallManagerImpl_Tests.cpp` | `isNewerVersion` | Boundary | `"1.2.3+build42"` treated as `"1.2.3"` |
| TC-PI-036 | `Test_Impl_IsNewerVersion_InvalidFormat_V1_ReturnsFalse` | `PreinstallManagerImpl_Tests.cpp` | `isNewerVersion` | Negative | `"notaversion"` vs `"1.0.0"` → `false` |
| TC-PI-037 | `Test_Impl_IsNewerVersion_InvalidFormat_V2_ReturnsFalse` | `PreinstallManagerImpl_Tests.cpp` | `isNewerVersion` | Negative | `"1.0.0"` vs `"bad"` → `false` |
| TC-PI-038 | `Test_Impl_IsNewerVersion_TwoPartVersion_ReturnsFalse` | `PreinstallManagerImpl_Tests.cpp` | `isNewerVersion` | Negative | `"1.2"` (only 2 parts) → `false` for both V1 and V2 |
| TC-PI-039 | `Test_Impl_GetFailReason_AllEnumValues` | `PreinstallManagerImpl_Tests.cpp` | `getFailReason` | Positive | Each `FailReason` enum maps to correct string |
| TC-PI-040 | `Test_Impl_GetFailReason_Default_ReturnsNONE` | `PreinstallManagerImpl_Tests.cpp` | `getFailReason` | Boundary | Unrecognised enum value returns `"NONE"` |
| TC-PI-041 | `Test_Impl_HandleOnAppInstallationStatus_ValidJson_DispatchesFired` | `PreinstallManagerImpl_Tests.cpp` | `handleOnAppInstallationStatus`, `dispatchEvent`, `Dispatch` | Positive | Notification called with correct JSON |
| TC-PI-042 | `Test_Impl_HandleOnAppInstallationStatus_EmptyString_LogsError` | `PreinstallManagerImpl_Tests.cpp` | `handleOnAppInstallationStatus` | Negative | Empty input; no crash; no dispatch fired |
| TC-PI-043 | `Test_Impl_Dispatch_UnknownEvent_LogsError` | `PreinstallManagerImpl_Tests.cpp` | `Dispatch` | Negative | Unknown event ID; no crash |
| TC-PI-044 | `Test_Impl_Dispatch_MissingJsonresponseKey_LogsError` | `PreinstallManagerImpl_Tests.cpp` | `Dispatch` | Negative | Params lack `"jsonresponse"` key; no notification called |
| TC-PI-045 | `Test_Impl_Dispatch_MultipleNotifications_AllReceiveEvent` | `PreinstallManagerImpl_Tests.cpp` | `Dispatch` | Positive | Two registered notifications both receive the event |
| TC-PI-046 | `Test_Impl_CreatePackageManagerObject_WithNullService_ReturnsError` | `PreinstallManagerImpl_Tests.cpp` | `createPackageManagerObject` | Negative | Returns `Core::ERROR_GENERAL` |
| TC-PI-047 | `Test_Impl_CreatePackageManagerObject_PluginNotRegistered_ReturnsError` | `PreinstallManagerImpl_Tests.cpp` | `createPackageManagerObject` | Negative | `QueryInterfaceByCallsign` returns null → `Core::ERROR_GENERAL` |
| TC-PI-048 | `Test_Impl_CreatePackageManagerObject_ValidPlugin_ReturnsNone` | `PreinstallManagerImpl_Tests.cpp` | `createPackageManagerObject` | Positive | Object created, `Register` called on notification |
| TC-PI-049 | `Test_Impl_ReleasePackageManagerObject_ClearsPointer` | `PreinstallManagerImpl_Tests.cpp` | `releasePackageManagerObject` | Positive | Pointer set to null after release |
| TC-PI-050 | `Test_Impl_ReadPreinstallDirectory_InvalidDir_ReturnsFalse` | `PreinstallManagerImpl_Tests.cpp` | `readPreinstallDirectory` | Negative | Non-existent directory → `false` |
| TC-PI-051 | `Test_Impl_ReadPreinstallDirectory_EmptyDir_ReturnsTrue` | `PreinstallManagerImpl_Tests.cpp` | `readPreinstallDirectory` | Boundary | Valid but empty dir → `true`, empty list |
| TC-PI-052 | `Test_Impl_ReadPreinstallDirectory_WithPackages_PopulatesList` | `PreinstallManagerImpl_Tests.cpp` | `readPreinstallDirectory` | Positive | Packages found → list populated with `fileLocator` set |
| TC-PI-053 | `Test_Impl_ReadPreinstallDirectory_GetConfigFails_PackageSkipped` | `PreinstallManagerImpl_Tests.cpp` | `readPreinstallDirectory` | Negative | `GetConfigForPackage` fails → package added with `installStatus = SKIPPED` |
| TC-PM-013 | `Test_PM_ServiceName_ConstantValue` | `PreinstallManager_LifecycleTests.cpp` | `SERVICE_NAME` | Positive | Equals `"org.rdk.PreinstallManager"` |
| TC-PM-014 | `Test_PM_Constructor_SetsInstancePointer` | `PreinstallManager_LifecycleTests.cpp` | `PreinstallManager()` ctor | Positive | `PreinstallManager::_instance` set to `this` |
| TC-PM-015 | `Test_PM_Destructor_ClearsInstancePointer` | `PreinstallManager_LifecycleTests.cpp` | `~PreinstallManager()` dtor | Positive | `PreinstallManager::_instance` becomes null |

---

## Test Cases — PreinstallManager (Plugin Wrapper)

### TC-PM-001 — Initialize With Valid Service Succeeds

- **File:** `PreinstallManager_LifecycleTests.cpp`
- **Methods:** `PreinstallManager::Initialize`, `PreinstallManager::Deinitialize`
- **Scenario:** Positive
- **Setup:** `ServiceMock` configured with a valid `IShell::ICOMLink::Instantiate` handler that returns an in-proc `PreinstallManagerImplementation`; configure interface is also provided.
- **Input:** `service` is a non-null `ServiceMock*`
- **Steps:**
  1. Construct `PreinstallManager` via `Core::Service<PreinstallManager>::Create<IPlugin>()`.
  2. Call `plugin->Initialize(service)`.
  3. Call `plugin->Deinitialize(service)`.
- **Expected:** `Initialize` returns `""`. No crashes.

---

### TC-PM-002 — Initialize With Null Service Returns Error

- **File:** `PreinstallManager_LifecycleTests.cpp`
- **Methods:** `PreinstallManager::Initialize`
- **Scenario:** Negative
- **Setup:** None.
- **Input:** `service = nullptr`
- **Expected:** Return value is non-empty string `"PreinstallManager plugin could not be initialised"`.

> **Note:** The plugin ASSERTs on null service in production (`ASSERT(nullptr != service)`). A robust L0 test should compile with `NDEBUG` or guard the call appropriately.

---

### TC-PM-003 — Initialize When Impl Creation Fails Returns Error

- **File:** `PreinstallManager_LifecycleTests.cpp`
- **Methods:** `PreinstallManager::Initialize`, `PreinstallManager::Deinitialize`
- **Scenario:** Negative
- **Setup:** `ServiceMock::ICOMLink::Instantiate` returns `nullptr`.
- **Input:** Valid `service`
- **Expected:** `Initialize` returns `"PreinstallManager plugin could not be initialised"`. Internally calls `Deinitialize` — no crash.

---

### TC-PM-004 — Initialize When Configure Interface Missing Returns Error

- **File:** `PreinstallManager_LifecycleTests.cpp`
- **Methods:** `PreinstallManager::Initialize`
- **Scenario:** Negative
- **Setup:** `Instantiate` returns an object that does **not** implement `Exchange::IConfiguration`.
- **Expected:** `Initialize` returns `"PreinstallManager implementation did not provide a configuration interface"`.

---

### TC-PM-005 — Initialize When Configure Fails Returns Error

- **File:** `PreinstallManager_LifecycleTests.cpp`
- **Methods:** `PreinstallManager::Initialize`
- **Scenario:** Negative
- **Setup:** Fake `IConfiguration::Configure` returns `Core::ERROR_GENERAL`.
- **Expected:** `Initialize` returns `"PreinstallManager could not be configured"`.

---

### TC-PM-006 — Initialize Twice Idempotent (Fresh Instance Each Time)

- **File:** `PreinstallManager_LifecycleTests.cpp`
- **Methods:** `PreinstallManager::Initialize`, `PreinstallManager::Deinitialize`
- **Scenario:** Boundary
- **Setup:** Two separate plugin instances, each initialized and deinitialized sequentially.
- **Expected:** No crash, no assertion failure.

---

### TC-PM-007 — Deinitialize Releases All Resources

- **File:** `PreinstallManager_LifecycleTests.cpp`
- **Methods:** `PreinstallManager::Deinitialize`
- **Scenario:** Positive
- **Setup:** Plugin fully initialized.
- **Steps:** Call `Deinitialize`.
- **Expected:** `mPreinstallManagerImpl` set to null; `mConnectionId` set to 0; `mCurrentService->Release()` called.

---

### TC-PM-008 — Information Returns Empty String

- **File:** `PreinstallManager_LifecycleTests.cpp`
- **Methods:** `PreinstallManager::Information`
- **Scenario:** Positive
- **Input:** None
- **Expected:** Returns `""`.

---

### TC-PM-009 — Deactivated Submits Deactivate Job

- **File:** `PreinstallManager_LifecycleTests.cpp`
- **Methods:** `PreinstallManager::Deactivated`
- **Scenario:** Positive
- **Setup:** Plugin initialized with connection ID 42. Fake `RPC::IRemoteConnection` whose `Id()` returns 42.
- **Expected:** Worker pool receives a deactivation job targeting `mCurrentService`.

---

### TC-PM-010 — Notification OnAppInstallationStatus Forwards Event

- **File:** `PreinstallManager_LifecycleTests.cpp`
- **Methods:** `PreinstallManager::Notification::OnAppInstallationStatus`
- **Scenario:** Positive
- **Setup:** Plugin registered as JSONRPC dispatcher.
- **Input:** `jsonresponse = "{\"appId\":\"com.example.app\",\"state\":\"INSTALLED\"}"`
- **Expected:** Event fired on the JSONRPC channel (observable via `JPreinstallManager::Event::OnAppInstallationStatus`).

---

### TC-PM-011 — Notification Activated Does Not Crash

- **File:** `PreinstallManager_LifecycleTests.cpp`
- **Methods:** `PreinstallManager::Notification::Activated`
- **Scenario:** Positive
- **Input:** Any non-null `RPC::IRemoteConnection*`
- **Expected:** No crash; only a log is emitted.

---

### TC-PM-012 — Notification Deactivated Triggers Parent Deactivated

- **File:** `PreinstallManager_LifecycleTests.cpp`
- **Methods:** `PreinstallManager::Notification::Deactivated`, `PreinstallManager::Deactivated`
- **Scenario:** Positive
- **Setup:** Plugin initialized with `mConnectionId = 1`. Fake connection with `Id() = 1`.
- **Expected:** `Deactivated` on the notification delegates to `_parent.Deactivated(connection)`.

---

### TC-PM-013 — SERVICE_NAME Constant Value

- **File:** `PreinstallManager_LifecycleTests.cpp`
- **Methods:** `PreinstallManager::SERVICE_NAME` (static constant)
- **Scenario:** Positive
- **Expected:** Equals `"org.rdk.PreinstallManager"`.

---

### TC-PM-014 — Constructor Sets Instance Pointer

- **File:** `PreinstallManager_LifecycleTests.cpp`
- **Methods:** `PreinstallManager::PreinstallManager()`
- **Scenario:** Positive
- **Expected:** `PreinstallManager::_instance != nullptr` after construction.

---

### TC-PM-015 — Destructor Clears Instance Pointer

- **File:** `PreinstallManager_LifecycleTests.cpp`
- **Methods:** `PreinstallManager::~PreinstallManager()`
- **Scenario:** Positive
- **Expected:** `PreinstallManager::_instance == nullptr` after destruction.

---

## Test Cases — PreinstallManagerImplementation (Core Logic)

### TC-PI-001 — Configure With Null Service Returns Error

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::Configure`
- **Scenario:** Negative
- **Input:** `service = nullptr`
- **Expected:** Returns `Core::ERROR_GENERAL`.

---

### TC-PI-002 — Configure With Valid Service Returns None

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::Configure`
- **Scenario:** Positive
- **Setup:** `ServiceMock` with `ConfigLine() = "{}"`.
- **Expected:** Returns `Core::ERROR_NONE`; `mCurrentservice` set.

---

### TC-PI-003 — Configure With AppPreinstallDirectory In Config

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::Configure`, `Configuration::FromString`
- **Scenario:** Positive
- **Setup:** `ConfigLine() = "{\"appPreinstallDirectory\":\"/opt/preinstall\"}"`.
- **Expected:** Returns `Core::ERROR_NONE`; internal `mAppPreinstallDirectory` equals `"/opt/preinstall"`.

---

### TC-PI-004 — Configure With Empty PreinstallDirectory

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::Configure`
- **Scenario:** Boundary
- **Setup:** Config JSON with `appPreinstallDirectory = ""`.
- **Expected:** Returns `Core::ERROR_NONE`; `mAppPreinstallDirectory` remains `""`.

---

### TC-PI-005 — Register Valid Notification Returns None

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::Register`
- **Scenario:** Positive
- **Input:** Valid `IPreinstallManager::INotification*` fake.
- **Expected:** Returns `Core::ERROR_NONE`; list size = 1; `AddRef` called on notification.

---

### TC-PI-006 — Register Duplicate Notification Not Added Twice

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::Register`
- **Scenario:** Boundary
- **Input:** Same notification pointer registered twice.
- **Expected:** Returns `Core::ERROR_NONE` both times; list size = 1 (duplicate rejected).

---

### TC-PI-007 — Register Null Notification Asserts

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::Register`
- **Scenario:** Negative
- **Note:** `ASSERT(nullptr != notification)` fires in debug builds. In builds with `NDEBUG`, behaviour is undefined. Tested by verifying the assertion path is instrumented and not silently skipped.

---

### TC-PI-008 — Unregister Registered Notification Returns None

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::Unregister`
- **Scenario:** Positive
- **Setup:** Notification registered first.
- **Expected:** Returns `Core::ERROR_NONE`; list is empty; `Release` called once.

---

### TC-PI-009 — Unregister Not Registered Returns General

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::Unregister`
- **Scenario:** Negative
- **Input:** Notification that was never registered.
- **Expected:** Returns `Core::ERROR_GENERAL`.

---

### TC-PI-010 — Unregister After Duplicate Register Correct RefCount

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::Register`, `PreinstallManagerImplementation::Unregister`
- **Scenario:** Boundary
- **Input:** Register twice (duplicate rejected), then unregister once.
- **Expected:** `Release` called exactly once; no double-free.

---

### TC-PI-011 — GetInstance Returns Constructed Instance

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::getInstance`
- **Scenario:** Positive
- **Expected:** `getInstance()` returns the same pointer as the constructed object.

---

### TC-PI-012 — Destructor Releases CurrentService

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::~PreinstallManagerImplementation`
- **Scenario:** Positive
- **Setup:** `Configure` called to set `mCurrentservice`.
- **Expected:** After destruction, `service.releaseCalls == 1`.

---

### TC-PI-013 — Destructor Releases PackageManager

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::~PreinstallManagerImplementation`, `releasePackageManagerObject`
- **Scenario:** Positive
- **Setup:** `mPackageManagerInstallerObject` set to a fake via `createPackageManagerObject` path.
- **Expected:** `Unregister` + `Release` on installer called during destruction.

---

### TC-PI-014 — StartPreinstall When PkgManager Creation Fails Returns Error

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::StartPreinstall`, `createPackageManagerObject`
- **Scenario:** Negative
- **Setup:** `mCurrentservice` null (so `createPackageManagerObject` returns `Core::ERROR_GENERAL`).
- **Input:** `forceInstall = true`
- **Expected:** Returns `Core::ERROR_GENERAL`.

---

### TC-PI-015 — StartPreinstall When Directory Read Fails Returns Error

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::StartPreinstall`, `readPreinstallDirectory`
- **Scenario:** Negative
- **Setup:** `mAppPreinstallDirectory` set to a non-existent path (e.g. `"/nonexistent_l0test_dir"`); package manager fake present.
- **Input:** `forceInstall = true`
- **Expected:** Returns `Core::ERROR_GENERAL`.

---

### TC-PI-016 — StartPreinstall Empty Directory No Install Calls

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::StartPreinstall`
- **Scenario:** Boundary
- **Setup:** Temp directory is created (empty) and set as `mAppPreinstallDirectory`.
- **Input:** `forceInstall = true`
- **Expected:** Returns `Core::ERROR_NONE`; fake installer `Install` call count = 0.

---

### TC-PI-017 — StartPreinstall Force Install Installs All Packages

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::StartPreinstall`
- **Scenario:** Positive
- **Setup:** Temp directory with 2 package folders; fake `GetConfigForPackage` succeeds for both; fake `Install` succeeds.
- **Input:** `forceInstall = true`
- **Expected:** `Install` called twice; returns `Core::ERROR_NONE`.

---

### TC-PI-018 — StartPreinstall No Force Install Skips Already Installed Same Version

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::StartPreinstall`, `isNewerVersion`
- **Scenario:** Positive
- **Setup:** One package in dir (`v1.2.3`); `ListPackages` returns same package at same version.
- **Input:** `forceInstall = false`
- **Expected:** `Install` not called; returns `Core::ERROR_NONE`.

---

### TC-PI-019 — StartPreinstall No Force Install Installs Newer Version

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::StartPreinstall`, `isNewerVersion`
- **Scenario:** Positive
- **Setup:** Package in dir is `v2.0.0`; installed version is `v1.5.0`.
- **Input:** `forceInstall = false`
- **Expected:** `Install` called once; returns `Core::ERROR_NONE`.

---

### TC-PI-020 — StartPreinstall No Force Install Skips Older Version

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::StartPreinstall`, `isNewerVersion`
- **Scenario:** Positive
- **Setup:** Package in dir is `v1.0.0`; installed version is `v1.5.0` (newer already installed).
- **Input:** `forceInstall = false`
- **Expected:** `Install` not called; returns `Core::ERROR_NONE`.

---

### TC-PI-021 — StartPreinstall ListPackages Fails Returns Error

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::StartPreinstall`
- **Scenario:** Negative
- **Setup:** Fake `ListPackages` returns `Core::ERROR_GENERAL`.
- **Input:** `forceInstall = false`
- **Expected:** Returns `Core::ERROR_GENERAL`.

---

### TC-PI-022 — StartPreinstall Null PackageList Returns Error

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::StartPreinstall`
- **Scenario:** Negative
- **Setup:** Fake `ListPackages` returns `Core::ERROR_NONE` but leaves `packageList = nullptr`.
- **Input:** `forceInstall = false`
- **Expected:** Returns `Core::ERROR_GENERAL`.

---

### TC-PI-023 — StartPreinstall Install Fails Returns Error

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::StartPreinstall`, `getFailReason`
- **Scenario:** Negative
- **Setup:** One valid package in dir; fake `Install` returns `Core::ERROR_GENERAL` with `FailReason::SIGNATURE_VERIFICATION_FAILURE`.
- **Input:** `forceInstall = true`
- **Expected:** Returns `Core::ERROR_GENERAL`; package `installStatus` contains `"FAILED: reason SIGNATURE_VERIFICATION_FAILURE"`.

---

### TC-PI-024 — StartPreinstall Package With Empty Fields Skipped

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::StartPreinstall`
- **Scenario:** Boundary
- **Setup:** Package in directory where `GetConfigForPackage` returns `Core::ERROR_NONE` but leaves `packageId` and `version` empty.
- **Input:** `forceInstall = true`
- **Expected:** `Install` not called for that package; `failedApps` incremented; returns `Core::ERROR_NONE` (no install was attempted therefore `installError` remains `false`).

---

### TC-PI-025 — StartPreinstall Releases PackageManager After Install

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::StartPreinstall`, `releasePackageManagerObject`
- **Scenario:** Positive
- **Setup:** Successful install of one package.
- **Expected:** `mPackageManagerInstallerObject` is null after `StartPreinstall` returns.

---

### TC-PI-026 — StartPreinstall Multiple Packages All Installed

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::StartPreinstall`
- **Scenario:** Positive
- **Setup:** 3 packages in temp dir; all `GetConfigForPackage` and `Install` calls succeed.
- **Input:** `forceInstall = true`
- **Expected:** `Install` called 3 times; returns `Core::ERROR_NONE`.

---

### TC-PI-027 — StartPreinstall Mixed Success And Failure Returns Error

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::StartPreinstall`
- **Scenario:** Boundary
- **Setup:** 2 packages; first installs OK, second `Install` returns error.
- **Expected:** `installError = true`; returns `Core::ERROR_GENERAL`.

---

### TC-PI-028 through TC-PI-038 — isNewerVersion Tests

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::isNewerVersion` (private; tested indirectly via public `StartPreinstall`, or by befriending/exposing via a test subclass)

| ID | v1 | v2 | Expected | Scenario |
|----|----|----|----------|---------|
| TC-PI-028 | `"2.0.0"` | `"1.9.9"` | `true` | Positive |
| TC-PI-029 | `"1.0.0"` | `"2.0.0"` | `false` | Negative |
| TC-PI-030 | `"1.2.3"` | `"1.2.3"` | `false` | Boundary |
| TC-PI-031 | `"1.3.0"` | `"1.2.9"` | `true` | Positive |
| TC-PI-032 | `"1.2.4"` | `"1.2.3"` | `true` | Positive |
| TC-PI-033 | `"1.2.3.5"` | `"1.2.3.4"` | `true` | Positive (build part) |
| TC-PI-034 | `"1.2.3-alpha"` | `"1.2.3"` | `false` | Boundary (pre-release stripped) |
| TC-PI-035 | `"1.2.3+build42"` | `"1.2.3"` | `false` | Boundary (build metadata stripped) |
| TC-PI-036 | `"notaversion"` | `"1.0.0"` | `false` | Negative (invalid v1) |
| TC-PI-037 | `"1.0.0"` | `"bad"` | `false` | Negative (invalid v2) |
| TC-PI-038 | `"1.2"` | `"1.2"` | `false` | Negative (only 2 parts, `sscanf` < 3) |

> **Testing strategy for private method:** `isNewerVersion` is exercised indirectly via `StartPreinstall` (TC-PI-018 through TC-PI-020). For more targeted coverage, the test class can extend `PreinstallManagerImplementation` using a `friend class` declaration added under `#ifdef L0_TEST`, or use a thin public wrapper method added under the same guard.

---

### TC-PI-039 — getFailReason All Enum Values

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::getFailReason`
- **Scenario:** Positive
- **Inputs:** Each `FailReason` enum value: `SIGNATURE_VERIFICATION_FAILURE`, `PACKAGE_MISMATCH_FAILURE`, `INVALID_METADATA_FAILURE`, `PERSISTENCE_FAILURE`
- **Expected:**

| Input | Expected String |
|-------|----------------|
| `SIGNATURE_VERIFICATION_FAILURE` | `"SIGNATURE_VERIFICATION_FAILURE"` |
| `PACKAGE_MISMATCH_FAILURE` | `"PACKAGE_MISMATCH_FAILURE"` |
| `INVALID_METADATA_FAILURE` | `"INVALID_METADATA_FAILURE"` |
| `PERSISTENCE_FAILURE` | `"PERSISTENCE_FAILURE"` |

---

### TC-PI-040 — getFailReason Default Returns NONE

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::getFailReason`
- **Scenario:** Boundary
- **Input:** Integer cast to `FailReason` that doesn't match any case (e.g. `static_cast<FailReason>(999)`)
- **Expected:** Returns `"NONE"`.

---

### TC-PI-041 — handleOnAppInstallationStatus Valid Json Dispatches Event

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `handleOnAppInstallationStatus`, `dispatchEvent`, `Dispatch`
- **Scenario:** Positive
- **Setup:** One notification registered; worker pool running (via `L0BootstrapGuard`).
- **Input:** `jsonresponse = "{\"appId\":\"com.test\",\"state\":\"INSTALLED\"}"`
- **Steps:**
  1. Call `handleOnAppInstallationStatus(jsonresponse)`.
  2. Allow worker pool to process the dispatched job (short `std::this_thread::sleep_for` or synchronous dispatch).
- **Expected:** Notification's `OnAppInstallationStatus` called with the same JSON string.

---

### TC-PI-042 — handleOnAppInstallationStatus Empty String Logs Error

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `handleOnAppInstallationStatus`
- **Scenario:** Negative
- **Input:** `jsonresponse = ""`
- **Expected:** No crash; `dispatchEvent` not called; no notification fired.

---

### TC-PI-043 — Dispatch Unknown Event Logs Error

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::Dispatch`
- **Scenario:** Negative
- **Setup:** Call `Dispatch` directly with `PREINSTALL_MANAGER_UNKNOWN` (0) event.
- **Expected:** No crash; no notification called; falls into `default:` branch.

---

### TC-PI-044 — Dispatch Missing JsonresponseKey Logs Error

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::Dispatch`
- **Scenario:** Negative
- **Setup:** Params JSON without the `"jsonresponse"` key.
- **Input:** `event = PREINSTALL_MANAGER_APP_INSTALLATION_STATUS`, `params = {}`
- **Expected:** No crash; notification not called (early `break`).

---

### TC-PI-045 — Dispatch Multiple Notifications All Receive Event

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `PreinstallManagerImplementation::Dispatch`
- **Scenario:** Positive
- **Setup:** Two notification fakes registered.
- **Input:** Valid params with `"jsonresponse"` key.
- **Expected:** Both notification fakes each have their `OnAppInstallationStatus` called once with the correct payload.

---

### TC-PI-046 — createPackageManagerObject With Null Service Returns Error

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `createPackageManagerObject`
- **Scenario:** Negative
- **Setup:** `mCurrentservice = nullptr` (Configure not called).
- **Expected:** Returns `Core::ERROR_GENERAL`.

---

### TC-PI-047 — createPackageManagerObject Plugin Not Registered Returns Error

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `createPackageManagerObject`
- **Scenario:** Negative
- **Setup:** `ServiceMock` that returns `nullptr` for callsign `"org.rdk.PackageManagerRDKEMS"`.
- **Expected:** Returns `Core::ERROR_GENERAL`.

---

### TC-PI-048 — createPackageManagerObject Valid Plugin Returns None

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `createPackageManagerObject`
- **Scenario:** Positive
- **Setup:** `ServiceMock` that returns a valid `IPackageInstaller` fake for `"org.rdk.PackageManagerRDKEMS"`.
- **Expected:** Returns `Core::ERROR_NONE`; `Register` called on the notification.

---

### TC-PI-049 — releasePackageManagerObject Clears Pointer

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `releasePackageManagerObject`
- **Scenario:** Positive
- **Setup:** Fake installer in `mPackageManagerInstallerObject`.
- **Expected:** After call, `mPackageManagerInstallerObject == nullptr`; `Unregister` and `Release` called.

---

### TC-PI-050 — readPreinstallDirectory Invalid Dir Returns False

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `readPreinstallDirectory`
- **Scenario:** Negative
- **Setup:** `mAppPreinstallDirectory = "/nonexistent_l0test_path_12345"`.
- **Expected:** Returns `false`.

---

### TC-PI-051 — readPreinstallDirectory Empty Dir Returns True

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `readPreinstallDirectory`
- **Scenario:** Boundary
- **Setup:** Create a temp directory; set `mAppPreinstallDirectory` to it.
- **Expected:** Returns `true`; output list is empty (only `.` and `..` skipped).

---

### TC-PI-052 — readPreinstallDirectory With Packages Populates List

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `readPreinstallDirectory`
- **Scenario:** Positive
- **Setup:** Temp directory with 2 subdirectory entries; fake `GetConfigForPackage` returns `Core::ERROR_NONE` with valid `packageId`, `version`.
- **Expected:** Returns `true`; list has 2 elements with correct `fileLocator` paths.

---

### TC-PI-053 — readPreinstallDirectory GetConfigFails Package Skipped With Status

- **File:** `PreinstallManagerImpl_Tests.cpp`
- **Methods:** `readPreinstallDirectory`
- **Scenario:** Negative
- **Setup:** 1 entry in temp dir; fake `GetConfigForPackage` returns `Core::ERROR_GENERAL`.
- **Expected:** Returns `true`; package is added to list with `installStatus` starting with `"SKIPPED: getConfig failed"`.

---

## Test Cases — Module

### TC-MOD-001 — Module Declaration Does Not Crash

- **File:** `PreinstallManagerImpl_Tests.cpp` (compilation smoke test)
- **Methods:** `MODULE_NAME_DECLARATION` expansion in `Module.cpp`
- **Scenario:** Positive
- **Expected:** Linking plugin sources including `Module.cpp` succeeds; executable runs without abort.

> Module.cpp contains only the `MODULE_NAME_DECLARATION` macro. There is no callable function to test directly at L0. Coverage is achieved by including `Module.cpp` in the L0 build's source list (as done in the CMakeLists template above).

---

## Coverage Estimate

The table below shows the estimated L0 coverage achievable with the 53 test cases above.

| File | Estimated Line Coverage | Estimated Function Coverage | Notes |
|------|------------------------|-----------------------------|-------|
| `PreinstallManager.cpp` | ~90 % | 100 % | All 4 public methods + `Deactivated` covered |
| `PreinstallManager.h` | ~85 % | ~90 % | `Notification` inner class fully covered |
| `PreinstallManagerImplementation.cpp` | ~80 % | ~88 % | All public methods + most private helpers |
| `PreinstallManagerImplementation.h` | ~80 % | ~85 % | `PackageInfo`, `Job`, `PackageManagerNotification` covered via integration |
| `Module.cpp` | ~50 % | ~50 % | Only compilation coverage; macro expands to two symbols |
| `Module.h` | 100 % | 100 % | Macro definition; included in every TU |
| **Overall** | **~84 %** | **~88 %** | Exceeds 75 % target |

---

## Methods Not Covered at L0 and Rationale

| Method / Symbol | File | Reason Not Covered |
|-----------------|------|--------------------|
| `PreinstallManager::Deactivated` — worker-pool side effect observation | `PreinstallManager.cpp` | The actual job execution (`PluginHost::IShell::Job::Create`) involves the production Thunder `IShell::Job` which is not easily interceptable without a full COM framework. The method *is* called (TC-PM-009) but its internal job dispatch is not verified at line level. |
| `PreinstallManagerImplementation::Job::Dispatch` (ABI `D0`/`D1` destructor variants) | `PreinstallManagerImplementation.h` | Compiler-generated ABI symbol variants; not reachable via C++ source-level calls. |
| `PreinstallManagerImplementation::Configuration::~Configuration` (ABI `D0` variant) | `PreinstallManagerImplementation.h` | Same reason as above. |
| `readPreinstallDirectory` — platform-specific `dirent` iteration on Windows | `PreinstallManagerImplementation.cpp` | The `opendir`/`readdir`/`closedir` POSIX API is not available on Windows. Tests TC-PI-050 through TC-PI-053 are Linux-only and would be skipped/stubbed in cross-platform builds. |
| `isValidSemVer` | `PreinstallManagerImplementation.h` | Declared in the header as a private method but **not implemented** in `PreinstallManagerImplementation.cpp` (no definition found). Likely reserved for future use. Cannot be tested until implemented. |
| `StartPreinstall` — `package.state != INSTALLED` path in `while` loop | `PreinstallManagerImplementation.cpp` | The `while (packageList->Next(package) && package.state == InstallState::INSTALLED)` condition exits when a non-installed package is encountered. The post-loop `existingApps` remains correct but the partially-iterated-list scenario is an edge case that requires a multi-state fake iterator. Partial coverage is acceptable at L0. |
| `Module.cpp` — `ModuleBuildRef` / `ModuleServiceMetadata` ABI symbols | `Module.cpp` | Compiler-generated from `MODULE_NAME_DECLARATION`; not directly invocable. Covered at link-time by inclusion in the build. |

---

## Appendix — Suggested File Structure for L0 Tests

```
Tests/L0Tests/PreinstallManager/
├── ServiceMock.h
│   ├── class ServiceMock : public PluginHost::IShell
│   │   ├── ConfigLine() → returns configurable JSON string
│   │   ├── Register/Unregister(IRemoteConnection::INotification*)
│   │   ├── QueryInterfaceByCallsign<T>(callsign) → uses L0ServiceMock registry
│   │   └── class COMLinkMock : public IShell::ICOMLink
│   │       └── Instantiate(...) → returns PreinstallManagerImplementation in-proc
│   └── class PackageInstallerFake : public Exchange::IPackageInstaller
│       ├── ListPackages(IPackageIterator*&) → controllable
│       ├── GetConfigForPackage(...) → controllable
│       ├── Install(...) → controllable return code + FailReason
│       └── Register/Unregister(INotification*)
├── PreinstallManagerTest.cpp
│   └── main() → L0Init, run all Test_*, L0Shutdown, return failures
├── PreinstallManager_LifecycleTests.cpp
│   └── TC-PM-001 … TC-PM-015 (12 plugin wrapper tests)
└── PreinstallManagerImpl_Tests.cpp
    └── TC-PI-001 … TC-PI-053 (53 implementation tests)
```

### CMakeLists.txt additions

```cmake
# Add to APPMANAGERS_L0_SOURCES in Tests/L0Tests/CMakeLists.txt:
${CMAKE_SOURCE_DIR}/../../PreinstallManager/Module.cpp
${CMAKE_SOURCE_DIR}/../../PreinstallManager/PreinstallManager.cpp
${CMAKE_SOURCE_DIR}/../../PreinstallManager/PreinstallManagerImplementation.cpp
PreinstallManager/PreinstallManagerTest.cpp
PreinstallManager/PreinstallManager_LifecycleTests.cpp
PreinstallManager/PreinstallManagerImpl_Tests.cpp

# Add to target_compile_definitions:
PREINSTALL_MANAGER_API_VERSION_NUMBER_MAJOR=1
PREINSTALL_MANAGER_API_VERSION_NUMBER_MINOR=0
PREINSTALL_MANAGER_API_VERSION_NUMBER_PATCH=0
```

---

*Document generated for branch `topic/L0_preinstall` — PreinstallManager plugin. Last updated: 2026-03-29.*
