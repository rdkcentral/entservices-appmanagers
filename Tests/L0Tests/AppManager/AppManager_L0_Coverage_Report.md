# AppManager Plugin – L0 Unit Test Coverage Report & Documentation

**Plugin:** `AppManager`  
**Source Directory:** `AppManager/`  
**Target Coverage:** ≥ 75% line coverage for every `.cpp` and `.h` file  
**Report Date:** 2026-03-19  
**Reference:** RuntimeManager L0 Tests at `Tests/L0Tests/RuntimeManager/`

---

## Table of Contents

1. [Quick-Reference Test Table](#1-quick-reference-test-table)
2. [File Coverage Baseline vs. Target](#2-file-coverage-baseline-vs-target)
3. [Test Cases: AppManager (Plugin Lifecycle)](#3-test-cases-appmanger-plugin-lifecycle)
4. [Test Cases: AppManagerImplementation](#4-test-cases-appmanagerimplementation)
5. [Test Cases: LifecycleInterfaceConnector](#5-test-cases-lifecycleinterfaceconnector)
6. [Test Cases: AppManagerTelemetryReporting](#6-test-cases-appmanagertelemetryreporting)
7. [Uncoverable Methods and Reasons](#7-uncoverable-methods-and-reasons)
8. [How L0 Test Code Is Generated](#8-how-l0-test-code-is-generated)
9. [Suggested File Structure](#9-suggested-file-structure)

---

## 1. Quick-Reference Test Table

| Test ID | Test Function Name | File Tested | Method(s) Covered | Scenario Type | Summary |
|---|---|---|---|---|---|
| AM-L-01 | `Test_AppManager_InitializeFailsWhenRootCreationFails` | AppManager.cpp | `Initialize` | Negative | Root<IAppManager>() returns nullptr → error message returned |
| AM-L-02 | `Test_AppManager_InitializeFailsWhenConfigInterfaceMissing` | AppManager.cpp | `Initialize` | Negative | IConfiguration not available → error message returned |
| AM-L-03 | `Test_AppManager_InitializeFailsWhenConfigureFails` | AppManager.cpp | `Initialize` | Negative | Configure() returns non-zero → error message returned |
| AM-L-04 | `Test_AppManager_InitializeSucceedsAndDeinitializeCleansUp` | AppManager.cpp | `Initialize`, `Deinitialize` | Positive | Full success path with cleanup |
| AM-L-05 | `Test_AppManager_InformationReturnsServiceName` | AppManager.cpp | `Information` | Positive | Returns string containing "org.rdk.AppManager" |
| AM-L-06 | `Test_AppManager_DeactivatedSubmitsDeactivationJob` | AppManager.cpp | `Deactivated` | Positive | Matching connectionId triggers deactivation job |
| AM-L-07 | `Test_AppManager_DeactivatedIgnoresMismatchedConnectionId` | AppManager.cpp | `Deactivated` | Negative | Non-matching connectionId does not submit job |
| AM-L-08 | `Test_AppManager_NotificationOnAppInstalledFiresEvent` | AppManager.cpp | `Notification::OnAppInstalled` | Positive | JAppManager event forwarded correctly |
| AM-L-09 | `Test_AppManager_NotificationOnAppUninstalledFiresEvent` | AppManager.cpp | `Notification::OnAppUninstalled` | Positive | JAppManager event forwarded correctly |
| AM-L-10 | `Test_AppManager_NotificationOnAppLifecycleStateChangedFiresEvent` | AppManager.cpp | `Notification::OnAppLifecycleStateChanged` | Positive | State change event forwarded correctly |
| AM-L-11 | `Test_AppManager_NotificationOnAppLaunchRequestFiresEvent` | AppManager.cpp | `Notification::OnAppLaunchRequest` | Positive | Launch request event forwarded correctly |
| AM-L-12 | `Test_AppManager_NotificationOnAppUnloadedFiresEvent` | AppManager.cpp | `Notification::OnAppUnloaded` | Positive | Unloaded event forwarded correctly |
| AM-I-01 | `Test_Impl_RegisterNotification` | AppManagerImplementation.cpp | `Register` | Positive | Valid notification registers, returns ERROR_NONE |
| AM-I-02 | `Test_Impl_RegisterDuplicateIsIdempotent` | AppManagerImplementation.cpp | `Register` | Boundary | Duplicate registration does not double-add |
| AM-I-03 | `Test_Impl_UnregisterNotification` | AppManagerImplementation.cpp | `Unregister` | Positive | Registered notification removed, returns ERROR_NONE |
| AM-I-04 | `Test_Impl_UnregisterNotRegisteredReturnsError` | AppManagerImplementation.cpp | `Unregister` | Negative | Unregistered notification returns ERROR_GENERAL |
| AM-I-05 | `Test_Impl_GetInstanceReturnsSelf` | AppManagerImplementation.cpp | `getInstance` | Positive | Singleton returns last-created instance |
| AM-I-06 | `Test_Impl_GetInstanceBeforeConstruction` | AppManagerImplementation.cpp | `getInstance` | Boundary | No crash after instance is released |
| AM-I-07 | `Test_Impl_ConfigureWithNullServiceReturnsError` | AppManagerImplementation.cpp | `Configure` | Negative | Null service returns ERROR_GENERAL |
| AM-I-08 | `Test_Impl_ConfigureWithServiceMissingRemotePlugins` | AppManagerImplementation.cpp | `Configure` | Negative | All QueryInterfaceByCallsign calls return null (mocks) |
| AM-I-09 | `Test_Impl_LaunchAppEmptyAppIdReturnsError` | AppManagerImplementation.cpp | `LaunchApp` | Negative | Empty appId → ERROR_INVALID_PARAMETER |
| AM-I-10 | `Test_Impl_LaunchAppNoLifecycleConnectorReturnsError` | AppManagerImplementation.cpp | `LaunchApp` | Negative | No lifecycle connector → ERROR_GENERAL |
| AM-I-11 | `Test_Impl_CloseAppEmptyAppIdReturnsError` | AppManagerImplementation.cpp | `CloseApp` | Negative | Empty appId → ERROR_GENERAL |
| AM-I-12 | `Test_Impl_CloseAppNoLifecycleConnectorReturnsError` | AppManagerImplementation.cpp | `CloseApp` | Negative | No lifecycle connector → ERROR_GENERAL |
| AM-I-13 | `Test_Impl_TerminateAppEmptyAppIdReturnsError` | AppManagerImplementation.cpp | `TerminateApp` | Negative | Empty appId → ERROR_GENERAL |
| AM-I-14 | `Test_Impl_TerminateAppNoLifecycleConnectorReturnsError` | AppManagerImplementation.cpp | `TerminateApp` | Negative | No lifecycle connector → ERROR_GENERAL |
| AM-I-15 | `Test_Impl_KillAppNoLifecycleConnectorReturnsNone` | AppManagerImplementation.cpp | `KillApp` | Boundary | No lifecycle connector → still returns ERROR_NONE |
| AM-I-16 | `Test_Impl_GetLoadedAppsNoLifecycleConnectorReturnsError` | AppManagerImplementation.cpp | `GetLoadedApps` | Negative | No lifecycle connector → ERROR_GENERAL |
| AM-I-17 | `Test_Impl_SendIntentNoLifecycleConnectorReturnsNone` | AppManagerImplementation.cpp | `SendIntent` | Boundary | No connector → still returns ERROR_NONE |
| AM-I-18 | `Test_Impl_PreloadAppEmptyAppIdReturnsError` | AppManagerImplementation.cpp | `PreloadApp` | Negative | Empty appId → ERROR_INVALID_PARAMETER |
| AM-I-19 | `Test_Impl_PreloadAppNoLifecycleConnectorReturnsError` | AppManagerImplementation.cpp | `PreloadApp` | Negative | No lifecycle connector (null) → ERROR_GENERAL |
| AM-I-20 | `Test_Impl_GetAppPropertyEmptyAppIdReturnsError` | AppManagerImplementation.cpp | `GetAppProperty` | Negative | Empty appId → ERROR_GENERAL |
| AM-I-21 | `Test_Impl_GetAppPropertyNoPersistentStorageReturnsError` | AppManagerImplementation.cpp | `GetAppProperty` | Negative | No persistent store object → ERROR_GENERAL |
| AM-I-22 | `Test_Impl_SetAppPropertyEmptyAppIdReturnsError` | AppManagerImplementation.cpp | `SetAppProperty` | Negative | Empty appId → ERROR_GENERAL |
| AM-I-23 | `Test_Impl_SetAppPropertyNoPersistentStorageReturnsError` | AppManagerImplementation.cpp | `SetAppProperty` | Negative | No persistent store object → ERROR_GENERAL |
| AM-I-24 | `Test_Impl_GetInstalledAppsNoPackageManagerReturnsError` | AppManagerImplementation.cpp | `GetInstalledApps` | Negative | No package manager → ERROR_GENERAL |
| AM-I-25 | `Test_Impl_IsInstalledNoPackageManagerReturnsError` | AppManagerImplementation.cpp | `IsInstalled` | Negative | No package manager → ERROR_GENERAL, installed=false |
| AM-I-26 | `Test_Impl_IsInstalledEmptyAppIdReturnsError` | AppManagerImplementation.cpp | `IsInstalled` | Negative | Empty appId short-circuit |
| AM-I-27 | `Test_Impl_CreateOrUpdatePackageInfoEmptyVersionReturnsFalse` | AppManagerImplementation.cpp | `createOrUpdatePackageInfoByAppId` | Negative | Empty version → returns false |
| AM-I-28 | `Test_Impl_CreateOrUpdatePackageInfoNewEntryReturnsTrue` | AppManagerImplementation.cpp | `createOrUpdatePackageInfoByAppId` | Positive | New appId entry created in mAppInfo |
| AM-I-29 | `Test_Impl_CreateOrUpdatePackageInfoUpdateExistingEntry` | AppManagerImplementation.cpp | `createOrUpdatePackageInfoByAppId` | Positive | Existing entry updated with new values |
| AM-I-30 | `Test_Impl_RemoveAppInfoByAppIdFoundReturnsTrue` | AppManagerImplementation.cpp | `removeAppInfoByAppId` | Positive | Known appId removed → returns true |
| AM-I-31 | `Test_Impl_RemoveAppInfoByAppIdNotFoundReturnsFalse` | AppManagerImplementation.cpp | `removeAppInfoByAppId` | Negative | Unknown appId → returns false |
| AM-I-32 | `Test_Impl_HandleOnAppLifecycleStateChangedEmptyAppIdNoDispatch` | AppManagerImplementation.cpp | `handleOnAppLifecycleStateChanged` | Boundary | Empty appId still dispatches event (params have empty appId) |
| AM-I-33 | `Test_Impl_HandleOnAppUnloadedEmptyAppIdNoDispatch` | AppManagerImplementation.cpp | `handleOnAppUnloaded` | Negative | Empty appId → LOGERR, no dispatch |
| AM-I-34 | `Test_Impl_HandleOnAppLaunchRequestEmptyAppIdNoDispatch` | AppManagerImplementation.cpp | `handleOnAppLaunchRequest` | Negative | Empty appId → LOGERR, no dispatch |
| AM-I-35 | `Test_Impl_DispatchEventLifecycleStateMissingAppId` | AppManagerImplementation.cpp | `Dispatch` | Negative | Params without appId → LOGERR, no notification |
| AM-I-36 | `Test_Impl_DispatchEventLifecycleStateMissingNewState` | AppManagerImplementation.cpp | `Dispatch` | Negative | Params missing newState field → LOGERR |
| AM-I-37 | `Test_Impl_DispatchEventLifecycleStateMissingOldState` | AppManagerImplementation.cpp | `Dispatch` | Negative | Params missing oldState field → LOGERR |
| AM-I-38 | `Test_Impl_DispatchEventLifecycleStateMissingErrorReason` | AppManagerImplementation.cpp | `Dispatch` | Negative | Params missing errorReason field → LOGERR |
| AM-I-39 | `Test_Impl_DispatchEventLifecycleStateCallsRegisteredNotification` | AppManagerImplementation.cpp | `Dispatch` | Positive | All params present → notification.OnAppLifecycleStateChanged called |
| AM-I-40 | `Test_Impl_DispatchEventInstallationStatusInstalledCallsOnAppInstalled` | AppManagerImplementation.cpp | `Dispatch` | Positive | state="INSTALLED" → notification.OnAppInstalled called |
| AM-I-41 | `Test_Impl_DispatchEventInstallationStatusUninstalledCallsOnAppUninstalled` | AppManagerImplementation.cpp | `Dispatch` | Positive | state="UNINSTALLED" → notification.OnAppUninstalled called |
| AM-I-42 | `Test_Impl_DispatchEventInstallationStatusUnknownLogsWarning` | AppManagerImplementation.cpp | `Dispatch` | Boundary | state="PENDING" → LOGWARN, no event fired |
| AM-I-43 | `Test_Impl_DispatchEventInstallationStatusMissingPackageIdLogsError` | AppManagerImplementation.cpp | `Dispatch` | Negative | Missing packageId → LOGERR |
| AM-I-44 | `Test_Impl_DispatchEventLaunchRequestCallsNotification` | AppManagerImplementation.cpp | `Dispatch` | Positive | Full params → notification.OnAppLaunchRequest called |
| AM-I-45 | `Test_Impl_DispatchEventLaunchRequestEmptyAppId` | AppManagerImplementation.cpp | `Dispatch` | Negative | Missing/empty appId → LOGERR |
| AM-I-46 | `Test_Impl_DispatchEventUnloadedCallsNotification` | AppManagerImplementation.cpp | `Dispatch` | Positive | Full params → notification.OnAppUnloaded called |
| AM-I-47 | `Test_Impl_DispatchEventUnloadedEmptyAppId` | AppManagerImplementation.cpp | `Dispatch` | Negative | Empty appId → LOGERR |
| AM-I-48 | `Test_Impl_DispatchEventUnknownEventLogsError` | AppManagerImplementation.cpp | `Dispatch` | Boundary | Unknown event enum value → LOGERR |
| AM-I-49 | `Test_Impl_GetInstallAppTypeInteractive` | AppManagerImplementation.cpp | `getInstallAppType` | Positive | APPLICATION_TYPE_INTERACTIVE → "INTERACTIVE" |
| AM-I-50 | `Test_Impl_GetInstallAppTypeSystem` | AppManagerImplementation.cpp | `getInstallAppType` | Positive | APPLICATION_TYPE_SYSTEM → "SYSTEM" |
| AM-I-51 | `Test_Impl_GetInstallAppTypeUnknown` | AppManagerImplementation.cpp | `getInstallAppType` | Boundary | APPLICATION_TYPE_UNKNOWN → "UNKNOWN" |
| AM-I-52 | `Test_Impl_GetMaxRunningAppsReturnsPositiveValue` | AppManagerImplementation.cpp | `GetMaxRunningApps` | Positive | Returns positive int, ERROR_NONE |
| AM-I-53 | `Test_Impl_GetMaxHibernatedAppsReturnsPositiveValue` | AppManagerImplementation.cpp | `GetMaxHibernatedApps` | Positive | Returns positive int, ERROR_NONE |
| AM-I-54 | `Test_Impl_GetMaxHibernatedFlashUsageReturnsPositiveValue` | AppManagerImplementation.cpp | `GetMaxHibernatedFlashUsage` | Positive | Returns positive value, ERROR_NONE |
| AM-I-55 | `Test_Impl_GetMaxInactiveRamUsageReturnsPositiveValue` | AppManagerImplementation.cpp | `GetMaxInactiveRamUsage` | Positive | Returns positive value, ERROR_NONE |
| AM-I-56 | `Test_Impl_MultipleNotificationsAllReceiveEvent` | AppManagerImplementation.cpp | `Register`, `Dispatch` | Positive | Three registered notifications all receive the same event |
| AM-I-57 | `Test_Impl_PackageUnlockAppIdNotInMapReturnsError` | AppManagerImplementation.cpp | `packageUnLock` | Negative | appId not in mAppInfo → ERROR_GENERAL |
| AM-I-58 | `Test_Impl_GetAppMetadataAppIdNotInMapReturnsError` | AppManagerImplementation.cpp | `GetAppMetadata` | Negative | appId not tracked in mAppInfo → ERROR_GENERAL |
| AM-I-59 | `Test_Impl_StartSystemAppNoLifecycleConnectorReturnsError` | AppManagerImplementation.cpp | `StartSystemApp` | Negative | No lifecycle connector → ERROR_GENERAL |
| AM-I-60 | `Test_Impl_ClearAppDataNoStorageManagerReturnsError` | AppManagerImplementation.cpp | `ClearAppData` | Negative | No storage manager → ERROR_GENERAL |
| AM-I-61 | `Test_Impl_ClearAllAppDataNoStorageManagerReturnsError` | AppManagerImplementation.cpp | `ClearAllAppData` | Negative | No storage manager → ERROR_GENERAL |
| AM-LC-01 | `Test_LC_ConstructionAndDestruction` | LifecycleInterfaceConnector.cpp | constructor/destructor | Positive | Object creation and destruction does not crash |
| AM-LC-02 | `Test_LC_ReleaseWithoutCreateDoesNotCrash` | LifecycleInterfaceConnector.cpp | `releaseLifecycleManagerRemoteObject` | Negative | Release without prior create → no crash |
| AM-LC-03 | `Test_LC_MapAppLifecycleStateActive` | LifecycleInterfaceConnector.cpp | `mapAppLifecycleState` | Positive | ACTIVE maps to APP_STATE_ACTIVE |
| AM-LC-04 | `Test_LC_MapAppLifecycleStateHibernated` | LifecycleInterfaceConnector.cpp | `mapAppLifecycleState` | Positive | HIBERNATED maps to APP_STATE_HIBERNATED |
| AM-LC-05 | `Test_LC_MapAppLifecycleStateAllStates` | LifecycleInterfaceConnector.cpp | `mapAppLifecycleState` | Positive | All lifecycle states mapped correctly |
| AM-LC-06 | `Test_LC_MapErrorReasonKnownValues` | LifecycleInterfaceConnector.cpp | `mapErrorReason` | Positive | Known error reasons map correctly |
| AM-LC-07 | `Test_LC_MapErrorReasonUnknown` | LifecycleInterfaceConnector.cpp | `mapErrorReason` | Boundary | Unknown error string → APP_ERROR_NONE |
| AM-LC-08 | `Test_LC_GetAppInstanceIdUnknownAppReturnsEmpty` | LifecycleInterfaceConnector.cpp | `GetAppInstanceId` | Negative | Untracked appId → empty string |
| AM-LC-09 | `Test_LC_RemoveAppInfoByAppIdDoesNotCrash` | LifecycleInterfaceConnector.cpp | `removeAppInfoByAppId` | Boundary | Unregistered appId → no crash |
| AM-LC-10 | `Test_LC_LaunchNoLifecycleManagerReturnsError` | LifecycleInterfaceConnector.cpp | `launch` | Negative | No LifecycleManager object → ERROR_GENERAL |
| AM-LC-11 | `Test_LC_CloseAppNoLifecycleManagerReturnsError` | LifecycleInterfaceConnector.cpp | `closeApp` | Negative | No LifecycleManager object → ERROR_GENERAL |
| AM-LC-12 | `Test_LC_TerminateAppNoLifecycleManagerReturnsError` | LifecycleInterfaceConnector.cpp | `terminateApp` | Negative | No LifecycleManager object → ERROR_GENERAL |
| AM-LC-13 | `Test_LC_KillAppNoLifecycleManagerReturnsError` | LifecycleInterfaceConnector.cpp | `killApp` | Negative | No LifecycleManager object → ERROR_GENERAL |
| AM-LC-14 | `Test_LC_IsAppLoadedNoLifecycleManagerReturnsError` | LifecycleInterfaceConnector.cpp | `isAppLoaded` | Negative | No LifecycleManager object → ERROR_GENERAL |
| AM-LC-15 | `Test_LC_FileExistsNonExistentReturnsFalse` | LifecycleInterfaceConnector.cpp | `fileExists` | Negative | Non-existent path → false |
| AM-LC-16 | `Test_LC_FileExistsExistingFileReturnsTrue` | LifecycleInterfaceConnector.cpp | `fileExists` | Positive | Existing file → true |

---

## 2. File Coverage Baseline vs. Target

| File | Estimated Lines | Target % | Expected with L0 Tests | Status | Notes |
|---|---|---|---|---|---|
| AppManager.cpp | ~180 | 75% | ~82% ✅ | Achievable | Core lifecycle paths well-covered; notification bridge covered via fakes |
| AppManager.h | ~150 | 75% | ~78% ✅ | Achievable | Header coverage follows .cpp coverage |
| AppManagerImplementation.cpp | ~1600+ | 75% | ~62% ⚠️ | Partial | Methods requiring PackageManager/LifecycleManager remote objects impossible in L0 |
| AppManagerImplementation.h | ~300 | 75% | ~90% ✅ | Achievable | Type/enum definitions always counted |
| AppManagerTelemetryReporting.cpp | ~100 | 75% | ~20% ❌ | Hard to reach | All methods gated on live ITelemetryMetrics COM-RPC plugin |
| AppManagerTelemetryReporting.h | ~60 | 75% | ~40% ❌ | Hard to reach | Tied to implementation coverage |
| LifecycleInterfaceConnector.cpp | ~400+ | 75% | ~55% ⚠️ | Partial | launch/preload/suspend success paths require ILifecycleManager fake |
| LifecycleInterfaceConnector.h | ~150 | 75% | ~78% ✅ | Achievable | Header coverage follows .cpp |
| Module.cpp | 1 | N/A | 0% | Exempt | MODULE_NAME_DECLARATION only; not unit-testable |
| Module.h | ~10 | N/A | 0% | Exempt | Macro definitions only |

---

## 3. Test Cases: AppManager (Plugin Lifecycle)

These tests exercise `AppManager.cpp` and `AppManager.h` — the Thunder plugin wrapper class.

---

### AM-L-01 — `Test_AppManager_InitializeFailsWhenRootCreationFails`

| Field | Value |
|---|---|
| **Test ID** | AM-L-01 |
| **File Covered** | AppManager.cpp |
| **Method(s)** | `Initialize(PluginHost::IShell*)` |
| **Type** | Negative |
| **Input** | ServiceMock whose `Instantiate()` returns `nullptr` (connectionId = 0) |
| **Expected Result** | `Initialize()` returns non-empty error string containing "AppManager plugin could not be initialised"; `IShell::AddRef` called twice; `IShell::Release` called twice |

**Description:**  
When `mCurrentService->Root<Exchange::IAppManager>()` fails to instantiate the implementation object, `Initialize` must return a failure message and call `Deinitialize` to clean up already-acquired references.

---

### AM-L-02 — `Test_AppManager_InitializeFailsWhenConfigInterfaceMissing`

| Field | Value |
|---|---|
| **Test ID** | AM-L-02 |
| **File Covered** | AppManager.cpp |
| **Method(s)** | `Initialize(PluginHost::IShell*)` |
| **Type** | Negative |
| **Input** | ServiceMock whose `Instantiate()` returns a `FakeAppManagerNoConfig` that does NOT implement `IConfiguration` |
| **Expected Result** | Returns error string "AppManager implementation did not provide a configuration interface" |

**Description:**  
After the Root<> call succeeds, `QueryInterface<IConfiguration>` on the returned object returns `nullptr`. The Initialize method should set the error and invoke Deinitialize.

---

### AM-L-03 — `Test_AppManager_InitializeFailsWhenConfigureFails`

| Field | Value |
|---|---|
| **Test ID** | AM-L-03 |
| **File Covered** | AppManager.cpp |
| **Method(s)** | `Initialize(PluginHost::IShell*)` |
| **Type** | Negative |
| **Input** | `FakeAppManagerWithConfig` where `Configure()` returns `Core::ERROR_GENERAL` |
| **Expected Result** | Returns error string "AppManager could not be configured" |

**Description:**  
Tests the third error branch: when the implementation object and IConfiguration are obtained but the `Configure()` call fails.

---

### AM-L-04 — `Test_AppManager_InitializeSucceedsAndDeinitializeCleansUp`

| Field | Value |
|---|---|
| **Test ID** | AM-L-04 |
| **File Covered** | AppManager.cpp |
| **Method(s)** | `Initialize`, `Deinitialize` |
| **Type** | Positive |
| **Input** | `FakeAppManagerWithConfig` where `Configure()` returns `Core::ERROR_NONE` |
| **Expected Result** | `Initialize` returns empty string; `Deinitialize` completes without crashing; IShell AddRef/Release calls are balanced |

**Description:**  
Full happy-path test. After successful Initialize, Deinitialize must unregister notifications, release IConfiguration, release IAppManager, and terminate the remote connection if present.

---

### AM-L-05 — `Test_AppManager_InformationReturnsServiceName`

| Field | Value |
|---|---|
| **Test ID** | AM-L-05 |
| **File Covered** | AppManager.cpp |
| **Method(s)** | `Information()` |
| **Type** | Positive |
| **Input** | Fresh `AppManager` instance (no Initialize needed) |
| **Expected Result** | Returned string contains "org.rdk.AppManager" |

---

### AM-L-06 — `Test_AppManager_DeactivatedSubmitsDeactivationJob`

| Field | Value |
|---|---|
| **Test ID** | AM-L-06 |
| **File Covered** | AppManager.cpp |
| **Method(s)** | `Deactivated(RPC::IRemoteConnection*)` |
| **Type** | Positive |
| **Input** | Fake `IRemoteConnection` whose `Id()` returns the same value as `mConnectionId` set during Initialize |
| **Expected Result** | A deactivation job is submitted to the worker pool; no crash |

**Description:**  
Simulates the out-of-process implementation crashing. The `Deactivated` callback must detect the matching connectionId and gracefully deactivate the plugin.

---

### AM-L-07 — `Test_AppManager_DeactivatedIgnoresMismatchedConnectionId`

| Field | Value |
|---|---|
| **Test ID** | AM-L-07 |
| **File Covered** | AppManager.cpp |
| **Method(s)** | `Deactivated(RPC::IRemoteConnection*)` |
| **Type** | Negative |
| **Input** | Fake `IRemoteConnection` whose `Id()` returns a different connectionId from plugin's `mConnectionId` |
| **Expected Result** | No deactivation job submitted; no crash |

---

### AM-L-08 — `Test_AppManager_NotificationOnAppInstalledFiresEvent`

| Field | Value |
|---|---|
| **Test ID** | AM-L-08 |
| **File Covered** | AppManager.h / AppManager.cpp |
| **Method(s)** | `Notification::OnAppInstalled` |
| **Type** | Positive |
| **Input** | appId="com.test.app", version="1.0.0" |
| **Expected Result** | `JAppManager::Event::OnAppInstalled` is invoked; no crash |

---

### AM-L-09 through AM-L-12 — Notification Event Forwarding

These tests follow the same pattern as AM-L-08 for the remaining notification callbacks:

| Test ID | Method | Input | Expected |
|---|---|---|---|
| AM-L-09 | `Notification::OnAppUninstalled` | appId="com.test.app" | `JAppManager::Event::OnAppUninstalled` called |
| AM-L-10 | `Notification::OnAppLifecycleStateChanged` | appId, instanceId, newState, oldState, errorReason | `JAppManager::Event::OnAppLifecycleStateChanged` called |
| AM-L-11 | `Notification::OnAppLaunchRequest` | appId, intent, source | `JAppManager::Event::OnAppLaunchRequest` called |
| AM-L-12 | `Notification::OnAppUnloaded` | appId, instanceId | `JAppManager::Event::OnAppUnloaded` called |

---

## 4. Test Cases: AppManagerImplementation  

### AM-I-01 — `Test_Impl_RegisterNotification`

| Field | Value |
|---|---|
| **Test ID** | AM-I-01 |
| **File Covered** | AppManagerImplementation.cpp |
| **Method(s)** | `Register(INotification*)` |
| **Type** | Positive |
| **Input** | Valid `FakeNotification` implementing `IAppManager::INotification` |
| **Expected Result** | Returns `Core::ERROR_NONE`; notification is in internal list |

---

### AM-I-02 — `Test_Impl_RegisterDuplicateIsIdempotent`

| Field | Value |
|---|---|
| **Test ID** | AM-I-02 |
| **File Covered** | AppManagerImplementation.cpp |
| **Method(s)** | `Register(INotification*)` |
| **Type** | Boundary |
| **Input** | Same `FakeNotification` pointer registered twice |
| **Expected Result** | Both calls return `Core::ERROR_NONE`; notification appears only once in the list |

---

### AM-I-03 — `Test_Impl_UnregisterNotification`

| Field | Value |
|---|---|
| **Test ID** | AM-I-03 |
| **File Covered** | AppManagerImplementation.cpp |
| **Method(s)** | `Unregister(INotification*)` |
| **Type** | Positive |
| **Input** | Previously registered `FakeNotification` |
| **Expected Result** | Returns `Core::ERROR_NONE` |

---

### AM-I-04 — `Test_Impl_UnregisterNotRegisteredReturnsError`

| Field | Value |
|---|---|
| **Test ID** | AM-I-04 |
| **File Covered** | AppManagerImplementation.cpp |
| **Method(s)** | `Unregister(INotification*)` |
| **Type** | Negative |
| **Input** | `FakeNotification` never registered |
| **Expected Result** | Returns `Core::ERROR_GENERAL` |

---

### AM-I-05 — `Test_Impl_GetInstanceReturnsSelf`

| Field | Value |
|---|---|
| **Test ID** | AM-I-05 |
| **File Covered** | AppManagerImplementation.cpp |
| **Method(s)** | `getInstance()` |
| **Type** | Positive |
| **Input** | Freshly created `AppManagerImplementation` |
| **Expected Result** | `getInstance()` returns the same pointer |

---

### AM-I-06 — `Test_Impl_GetInstanceBeforeConstruction`

| Field | Value |
|---|---|
| **Test ID** | AM-I-06 |
| **File Covered** | AppManagerImplementation.cpp |
| **Method(s)** | `getInstance()` |
| **Type** | Boundary |
| **Input** | Called after releasing the only instance |
| **Expected Result** | Does not crash (pointer may be null or stale) |

---

### AM-I-07 — `Test_Impl_ConfigureWithNullServiceReturnsError`

| Field | Value |
|---|---|
| **Test ID** | AM-I-07 |
| **File Covered** | AppManagerImplementation.cpp |
| **Method(s)** | `Configure(PluginHost::IShell*)` |
| **Type** | Negative |
| **Input** | `nullptr` service pointer |
| **Expected Result** | Returns `Core::ERROR_GENERAL` |

---

### AM-I-08 — `Test_Impl_ConfigureWithServiceMissingRemotePlugins`

| Field | Value |
|---|---|
| **Test ID** | AM-I-08 |
| **File Covered** | AppManagerImplementation.cpp |
| **Method(s)** | `Configure(PluginHost::IShell*)` |
| **Type** | Negative |
| **Input** | `ServiceMock` that returns `nullptr` for all `QueryInterfaceByCallsign` calls |
| **Expected Result** | Returns `Core::ERROR_NONE` eventually (worker thread starts); all internal plugin pointers are nullptr |

**Note:** `Configure()` returns `Core::ERROR_NONE` when the worker thread starts successfully, regardless of whether remote plugin objects could be acquired. The remote plugin acquisition failures are logged but non-blocking.

---

### AM-I-09 — `Test_Impl_LaunchAppEmptyAppIdReturnsError`

| Field | Value |
|---|---|
| **Test ID** | AM-I-09 |
| **File Covered** | AppManagerImplementation.cpp |
| **Method(s)** | `LaunchApp(appId, intent, launchArgs)` |
| **Type** | Negative |
| **Input** | `appId=""`, intent="test", launchArgs="" |
| **Expected Result** | Returns `Core::ERROR_INVALID_PARAMETER` |

---

### AM-I-10 — `Test_Impl_LaunchAppNoLifecycleConnectorReturnsError`

| Field | Value |
|---|---|
| **Test ID** | AM-I-10 |
| **File Covered** | AppManagerImplementation.cpp |
| **Method(s)** | `LaunchApp` |
| **Type** | Negative |
| **Input** | `appId="com.test.app"`, no `Configure()` called (LifecycleInterfaceConnector is null) |
| **Expected Result** | Returns `Core::ERROR_GENERAL` |

---

### AM-I-11 through AM-I-19 — App Action Methods

| Test ID | Method | Input | Expected Result | Scenario |
|---|---|---|---|---|
| AM-I-11 | `CloseApp` | empty appId | `ERROR_GENERAL` | Negative |
| AM-I-12 | `CloseApp` | valid appId, no connector | `ERROR_GENERAL` | Negative |
| AM-I-13 | `TerminateApp` | empty appId | `ERROR_GENERAL` | Negative |
| AM-I-14 | `TerminateApp` | valid appId, no connector | `ERROR_GENERAL` | Negative |
| AM-I-15 | `KillApp` | valid appId, no connector | `ERROR_NONE` (default) | Boundary |
| AM-I-16 | `GetLoadedApps` | no connector | `ERROR_GENERAL` | Negative |
| AM-I-17 | `SendIntent` | valid params, no connector | `ERROR_NONE` (default) | Boundary |
| AM-I-18 | `PreloadApp` | empty appId | `ERROR_INVALID_PARAMETER` | Negative |
| AM-I-19 | `PreloadApp` | valid appId, connector=null | `ERROR_GENERAL` | Negative |

---

### AM-I-20 through AM-I-23 — AppProperty Methods

| Test ID | Method | Input | Expected Result | Scenario |
|---|---|---|---|---|
| AM-I-20 | `GetAppProperty` | empty appId, key="theme" | `ERROR_GENERAL` | Negative |
| AM-I-21 | `GetAppProperty` | valid appId, no PersistentStore | `ERROR_GENERAL` | Negative |
| AM-I-22 | `SetAppProperty` | empty appId, key="k", val="v" | `ERROR_GENERAL` | Negative |
| AM-I-23 | `SetAppProperty` | valid appId, no PersistentStore | `ERROR_GENERAL` | Negative |

---

### AM-I-24 through AM-I-26 — Package/Installation Query Methods

| Test ID | Method | Input | Expected Result | Scenario |
|---|---|---|---|---|
| AM-I-24 | `GetInstalledApps` | no package manager set | `ERROR_GENERAL` | Negative |
| AM-I-25 | `IsInstalled` | valid appId, no package manager | `ERROR_GENERAL`, installed=false | Negative |
| AM-I-26 | `IsInstalled` | empty appId | early `ERROR_GENERAL` | Negative |

---

### AM-I-27 through AM-I-31 — Package Info Map Methods

| Test ID | Method | Input | Expected Result | Scenario |
|---|---|---|---|---|
| AM-I-27 | `createOrUpdatePackageInfoByAppId` | empty version | returns false | Negative |
| AM-I-28 | `createOrUpdatePackageInfoByAppId` | new appId, valid version+path | returns true; entry in mAppInfo | Positive |
| AM-I-29 | `createOrUpdatePackageInfoByAppId` | existing appId, new version | returns true; entry updated | Positive |
| AM-I-30 | `removeAppInfoByAppId` | appId in mAppInfo | returns true | Positive |
| AM-I-31 | `removeAppInfoByAppId` | unknown appId | returns false | Negative |

---

### AM-I-32 through AM-I-34 — Event Handler Helpers

| Test ID | Method | Input | Behavior | Scenario |
|---|---|---|---|---|
| AM-I-32 | `handleOnAppLifecycleStateChanged` | valid params (even empty appId) | dispatchEvent() called | Boundary |
| AM-I-33 | `handleOnAppUnloaded` | empty appId | LOGERR; dispatchEvent NOT called | Negative |
| AM-I-34 | `handleOnAppLaunchRequest` | empty appId | LOGERR; dispatchEvent NOT called | Negative |

---

### AM-I-35 through AM-I-48 — Dispatch() Event Cases

Each Dispatch test creates an `AppManagerImplementation`, registers a `FakeNotification`, builds a `JsonObject` with controlled parameters, and calls `Dispatch()` directly to verify the dispatch logic without threading.

| Test ID | Event | Parameter Variation | Expected |
|---|---|---|---|
| AM-I-35 | `APP_EVENT_LIFECYCLE_STATE_CHANGED` | missing appId label | LOGERR; notification not called |
| AM-I-36 | `APP_EVENT_LIFECYCLE_STATE_CHANGED` | appId present, missing newState | LOGERR; notification not called |
| AM-I-37 | `APP_EVENT_LIFECYCLE_STATE_CHANGED` | missing oldState | LOGERR; notification not called |
| AM-I-38 | `APP_EVENT_LIFECYCLE_STATE_CHANGED` | missing errorReason | LOGERR; notification not called |
| AM-I-39 | `APP_EVENT_LIFECYCLE_STATE_CHANGED` | all fields present | `OnAppLifecycleStateChanged()` called on each registered notification |
| AM-I-40 | `APP_EVENT_INSTALLATION_STATUS` | state="INSTALLED", version present | `OnAppInstalled()` called |
| AM-I-41 | `APP_EVENT_INSTALLATION_STATUS` | state="UNINSTALLED" | `OnAppUninstalled()` called |
| AM-I-42 | `APP_EVENT_INSTALLATION_STATUS` | state="PENDING" (unknown) | LOGWARN; neither fired |
| AM-I-43 | `APP_EVENT_INSTALLATION_STATUS` | missing packageId | LOGERR; nothing called |
| AM-I-44 | `APP_EVENT_LAUNCH_REQUEST` | all fields present | `OnAppLaunchRequest()` called |
| AM-I-45 | `APP_EVENT_LAUNCH_REQUEST` | empty/missing appId | LOGERR; nothing called |
| AM-I-46 | `APP_EVENT_UNLOADED` | all fields present | `OnAppUnloaded()` called |
| AM-I-47 | `APP_EVENT_UNLOADED` | empty appId | LOGERR; nothing called |
| AM-I-48 | Unknown event (9999) | — | LOGERR; no crash |

---

### AM-I-49 through AM-I-51 — `getInstallAppType`

| Test ID | Input | Expected |
|---|---|---|
| AM-I-49 | `APPLICATION_TYPE_INTERACTIVE` | Returns `"INTERACTIVE"` |
| AM-I-50 | `APPLICATION_TYPE_SYSTEM` | Returns `"SYSTEM"` |
| AM-I-51 | `APPLICATION_TYPE_UNKNOWN` | Returns `"UNKNOWN"` (or empty, depending on impl) |

---

### AM-I-52 through AM-I-55 — Capacity Query Methods (Stubs)

| Test ID | Method | Expected Result |
|---|---|---|
| AM-I-52 | `GetMaxRunningApps` | Returns ≥ 0 and `Core::ERROR_NONE` |
| AM-I-53 | `GetMaxHibernatedApps` | Returns ≥ 0 and `Core::ERROR_NONE` |
| AM-I-54 | `GetMaxHibernatedFlashUsage` | Returns ≥ 0 and `Core::ERROR_NONE` |
| AM-I-55 | `GetMaxInactiveRamUsage` | Returns ≥ 0 and `Core::ERROR_NONE` |

---

### AM-I-56 — `Test_Impl_MultipleNotificationsAllReceiveEvent`

| Field | Value |
|---|---|
| **Test ID** | AM-I-56 |
| **File Covered** | AppManagerImplementation.cpp |
| **Method(s)** | `Register`, `Dispatch` |
| **Type** | Positive |
| **Input** | Three `FakeNotification` instances registered; `Dispatch` called with valid `APP_EVENT_LIFECYCLE_STATE_CHANGED` params |
| **Expected Result** | All three notifications invoke `OnAppLifecycleStateChanged` exactly once each |

---

### AM-I-57 through AM-I-61 — Remaining Public Methods

| Test ID | Method | Input | Expected |
|---|---|---|---|
| AM-I-57 | `packageUnLock` (via Dispatch APP_EVENT_UNLOADED) | appId not in mAppInfo | `ERROR_GENERAL` from packageUnLock (logged) |
| AM-I-58 | `GetAppMetadata` | appId not found in mAppInfo | `ERROR_GENERAL` |
| AM-I-59 | `StartSystemApp` | no lifecycle connector | `ERROR_GENERAL` |
| AM-I-60 | `ClearAppData` | no storage manager | `ERROR_GENERAL` |
| AM-I-61 | `ClearAllAppData` | no storage manager | `ERROR_GENERAL` |

---

## 5. Test Cases: LifecycleInterfaceConnector

### AM-LC-01 — `Test_LC_ConstructionAndDestruction`

| Field | Value |
|---|---|
| **Test ID** | AM-LC-01 |
| **File Covered** | LifecycleInterfaceConnector.cpp |
| **Method(s)** | Constructor, Destructor |
| **Type** | Positive |
| **Input** | `ServiceMock` that returns null for all queries |
| **Expected Result** | Object construction and destruction complete without crash |

---

### AM-LC-02 — `Test_LC_ReleaseWithoutCreateDoesNotCrash`

| Field | Value |
|---|---|
| **Test ID** | AM-LC-02 |
| **File Covered** | LifecycleInterfaceConnector.cpp |
| **Method(s)** | `releaseLifecycleManagerRemoteObject` |
| **Type** | Negative |
| **Input** | `releaseLifecycleManagerRemoteObject()` called without prior `createLifecycleManagerRemoteObject()` |
| **Expected Result** | No crash; null guard works correctly |

---

### AM-LC-03 through AM-LC-05 — `mapAppLifecycleState` Mapping Tests

| Test ID | Input `LifecycleState` | Expected `AppLifecycleState` |
|---|---|---|
| AM-LC-03 | `RUNNING` / `ACTIVE` | `APP_STATE_ACTIVE` or `APP_STATE_RUNNING` (per mapping) |
| AM-LC-04 | `HIBERNATED` | `APP_STATE_HIBERNATED` |
| AM-LC-05 | All remaining states (LOADING, SUSPENDED, PAUSED, UNLOADED, etc.) | Correct `AppLifecycleState` per table |

---

### AM-LC-06 through AM-LC-07 — `mapErrorReason` Mapping Tests

| Test ID | Input String | Expected `AppErrorReason` |
|---|---|---|
| AM-LC-06 | Known failure strings (e.g., "package_lock") | Corresponding `APP_ERROR_*` value |
| AM-LC-07 | Unknown string or empty | `APP_ERROR_NONE` |

---

### AM-LC-08 — `Test_LC_GetAppInstanceIdUnknownAppReturnsEmpty`

| Field | Value |
|---|---|
| **Test ID** | AM-LC-08 |
| **File Covered** | LifecycleInterfaceConnector.cpp |
| **Method(s)** | `GetAppInstanceId(appId)` |
| **Type** | Negative |
| **Input** | appId that was never registered |
| **Expected Result** | Returns empty string |

---

### AM-LC-09 — `Test_LC_RemoveAppInfoByAppIdDoesNotCrash`

| Field | Value |
|---|---|
| **Test ID** | AM-LC-09 |
| **File Covered** | LifecycleInterfaceConnector.cpp |
| **Method(s)** | `removeAppInfoByAppId(appId)` |
| **Type** | Boundary |
| **Input** | appId not tracked in internal map |
| **Expected Result** | No crash; function is a silent no-op |

---

### AM-LC-10 through AM-LC-15 — Remote Methods Without Manager Object

All lifecycle delegation methods (`launch`, `closeApp`, `terminateApp`, `killApp`, `preLoadApp`, `sendIntent`, `getLoadedApps`, `isAppLoaded`) follow the same pattern: when the internal `mLifecycleManagerRemoteObject` is null (because `createLifecycleManagerRemoteObject()` was not called or failed), each method must return `Core::ERROR_GENERAL` without crashing.

| Test ID | Method | Expected |
|---|---|---|
| AM-LC-10 | `launch` | `ERROR_GENERAL` |
| AM-LC-11 | `closeApp` | `ERROR_GENERAL` |
| AM-LC-12 | `terminateApp` | `ERROR_GENERAL` |
| AM-LC-13 | `killApp` | `ERROR_GENERAL` |
| AM-LC-14 | `isAppLoaded` | `ERROR_GENERAL` |
| AM-LC-15 | `fileExists` on non-existent path | `false` |
| AM-LC-16 | `fileExists` on existing `/tmp` directory | `true` |

---

## 6. Test Cases: AppManagerTelemetryReporting

> **Note:** `AppManagerTelemetryReporting` is a conditionally-compiled unit (`ENABLE_AIMANAGERS_TELEMETRY_METRICS`). All its public methods depend on a live `Exchange::ITelemetryMetrics` COM-RPC object. Since `ServiceMock::QueryInterfaceByCallsign` returns `nullptr` in L0 environments, full positive path tests are **structurally unreachable** in L0. The following tests are smoke tests only.

| Test ID | Test Function Name | Method(s) | Type | Expected |
|---|---|---|---|---|
| AM-T-01 | `Test_Telemetry_ConstructionAndDestruction` | Constructor/Destructor | Positive | No crash |
| AM-T-02 | `Test_Telemetry_GetCurrentTimestampReturnsPositive` | `getCurrentTimestamp` | Positive | Returns positive `time_t` value |
| AM-T-03 | `Test_Telemetry_ReportTelemetryDataNoPluginDoesNotCrash` | `reportTelemetryData` | Negative | No crash when plugin object is null |
| AM-T-04 | `Test_Telemetry_ReportErrorDataNoPluginDoesNotCrash` | `reportTelemetryErrorData` | Negative | No crash when plugin object is null |

---

## 7. Uncoverable Methods and Reasons

The following methods cannot be covered to ≥ 75% or at all in L0 tests, and are excluded from the coverage calculation:

### 7.1 `AppManagerImplementation.cpp` — Methods Requiring Live Remote Plugins

| Method / Region | Reason Uncoverable in L0 |
|---|---|
| `LaunchApp` — success path (post-LifecycleConnector dispatch) | Requires `mLifecycleInterfaceConnector` (obtained only after `Configure()` successfully connects to `LifecycleManager`). `ServiceMock::QueryInterfaceByCallsign` always returns `nullptr`. |
| `packageLock()` — success path | Requires live `IPackageHandler::Lock` and `IPackageInstaller::GetPackages`. Neither is injectable via `ServiceMock` without a full fake implementation and per-callsign injection. |
| `packageUnLock()` — PackageManager present path | Same dependency on `mPackageManagerHandlerObject` which is always null in L0. |
| `GetAppProperty` / `SetAppProperty` — success path | Requires `mPersistentStoreRemoteStoreObject` (`IStore2`). Service mock returns null. |
| `GetInstalledApps` — success path | Requires `mPackageManagerInstallerObject->GetPackages()` to return a non-empty list. |
| `AppManagerWorkerThread` — launch/preload processing paths | Thread requires `mPackageManagerHandlerObject` and `mLifecycleInterfaceConnector` to be non-null. Both are null without full plugin injection. |
| `createPersistentStoreRemoteStoreObject` — success branch | `QueryInterfaceByCallsign<IStore2>("org.rdk.PersistentStore")` always returns null. |
| `createPackageManagerObject` — success branch | `QueryInterfaceByCallsign<IPackageHandler>(...)` always returns null. |
| `createStorageManagerRemoteObject` — success branch | `QueryInterfaceByCallsign<IAppStorageManager>(...)` always returns null. |
| `ClearAppData` / `ClearAllAppData` — success path | Require `mStorageManagerRemoteObject` to be non-null. |

**Root Cause:** `AppManagerImplementation` tightly couples itself to five out-of-process plugins:
- `org.rdk.LifecycleManager` → `ILifecycleManager` + `ILifecycleManagerState`
- `org.rdk.PersistentStore` → `IStore2`
- `org.rdk.PackageManagerRDKEMS` → `IPackageHandler` + `IPackageInstaller`
- `org.rdk.AppStorageManager` → `IAppStorageManager`

Since `ServiceMock::QueryInterfaceByCallsign` unconditionally returns `nullptr`, all success paths requiring these objects are structurally unreachable at L0 layer.

**Recommended Path to Higher Coverage:** Extend `L0Test::L0ServiceMock` to support per-callsign fake injection (similar to the `L0ServiceMock::RegisterInterface` pattern in `L0ServiceMock.hpp`), or refactor `AppManagerImplementation` to accept interfaces via constructor injection for testability.

### 7.2 `AppManagerTelemetryReporting.cpp` — Entire Positive Path

| Method | Reason |
|---|---|
| `initialize` success path | Requires live `Exchange::ITelemetryMetrics` plugin |
| `reportTelemetryData` dispatch | Requires non-null `mTelemetryMetricsObject` |
| `reportTelemetryErrorData` dispatch | Same dependency |
| `reportTelemetryDataOnStateChange` | Same dependency |

The `ENABLE_AIMANAGERS_TELEMETRY_METRICS` flag must also be set at compile time. In CI/L0 environments this is typically disabled, making the entire file exempt from measurement.

### 7.3 `LifecycleInterfaceConnector.cpp` — Success Paths of Delegation Methods

| Method | Reason |
|---|---|
| `launch` — success path | Requires `mLifecycleManagerRemoteObject->Launch()` to return OK |
| `preLoadApp` — success path | Same dependency |
| `closeApp`, `terminateApp`, `killApp` — success paths | Same dependency |
| `OnAppLifecycleStateChanged` callback logic (full state transition) | Requires `mAppInfo` to be pre-populated via launch, then state change callback |
| Suspend/Hibernate policy file checks (`SUSPEND_POLICY_FILE`) | Requires `/tmp/AI2.0Suspendable` to exist |

### 7.4 `Module.cpp` — Not Testable

`Module.cpp` contains only the `MODULE_NAME_DECLARATION` macro invocation. No instantiatable logic exists. This file is **excluded** from the 75% coverage target.

---

## 8. How L0 Test Code Is Generated

This section documents the conventions and framework used in the AppManager (and RuntimeManager reference) L0 test suite so future tests can be written consistently.

### 8.1 Framework Overview

The L0 tests use a **custom hand-rolled test framework** — no GoogleTest or GMock dependency. Key files located in `Tests/L0Tests/common/`:

| File | Role |
|---|---|
| `L0TestTypes.hpp` | `L0Test::TestResult` struct with a `failures` counter |
| `L0Expect.hpp` | Assertion helpers: `ExpectTrue`, `ExpectEqU32`, `ExpectEqStr`, `ExpectJsonEq` |
| `L0Bootstrap.hpp` | `L0Test::L0BootstrapGuard` — initializes the Thunder Core worker pool |
| `L0ServiceMock.hpp` | `L0Test::L0ServiceMock` — lightweight callsign+interfaceId registry for fakes |

The per-plugin `ServiceMock.h` in `Tests/L0Tests/RuntimeManager/` extends `L0Test::L0ServiceMock` (or implements `PluginHost::IShell` directly) to provide the minimum IShell contract needed by the plugin under test.

### 8.2 Test Function Signature

Every test is a free function returning `uint32_t` (the failure count):

```cpp
uint32_t Test_AppManager_InitializeFailsWhenRootCreationFails()
{
    L0Test::TestResult tr;

    // Setup
    PluginAndService ps;
    ps.service.SetInstantiateHandler([](const WPEFramework::RPC::Object&,
                                        const uint32_t,
                                        uint32_t& connectionId) -> void* {
        connectionId = 0;
        return nullptr;  // Simulate Root<> failure
    });

    // Exercise
    const std::string result = ps.plugin->Initialize(&ps.service);

    // Verify
    L0Test::ExpectTrue(tr, !result.empty(), "Initialize returns error on Root<> failure");

    return tr.failures;  // 0 == pass
}
```

### 8.3 Fake Object Pattern (No GMock)

Fake implementations are declared in anonymous namespaces within the test `.cpp` file. They implement the minimum Thunder COM interface:

```cpp
namespace {

class FakeAppManagerNoConfig final : public WPEFramework::Exchange::IAppManager {
public:
    FakeAppManagerNoConfig() : _refCount(1) {}

    void AddRef() const override {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }
    uint32_t Release() const override {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) { delete this; return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED; }
        return WPEFramework::Core::ERROR_NONE;
    }
    void* QueryInterface(const uint32_t id) override {
        if (id == WPEFramework::Exchange::IAppManager::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IAppManager*>(this);
        }
        // NOTE: Does NOT return IConfiguration — this is the "NoConfig" variant
        return nullptr;
    }

    // Stub all pure-virtual IAppManager methods...
    WPEFramework::Core::hresult Register(WPEFramework::Exchange::IAppManager::INotification*) override
        { return WPEFramework::Core::ERROR_NONE; }
    // ... (all other methods)

private:
    mutable std::atomic<uint32_t> _refCount;
};

class FakeAppManagerWithConfig final : public WPEFramework::Exchange::IAppManager,
                                       public WPEFramework::Exchange::IConfiguration {
public:
    void* QueryInterface(const uint32_t id) override {
        if (id == WPEFramework::Exchange::IAppManager::ID) { AddRef(); return static_cast<WPEFramework::Exchange::IAppManager*>(this); }
        if (id == WPEFramework::Exchange::IConfiguration::ID) { AddRef(); return static_cast<WPEFramework::Exchange::IConfiguration*>(this); }
        return nullptr;
    }
    uint32_t Configure(WPEFramework::PluginHost::IShell*) override {
        return WPEFramework::Core::ERROR_NONE;  // or ERROR_GENERAL for the "ConfigureFails" variant
    }
    // ... stubs
};

} // anonymous namespace
```

### 8.4 Instantiating Plugin Objects

Thunder plugin objects must be created through `Core::Service<T>::Create<T>()`:

```cpp
// Plugin (AppManager) — use IPlugin interface
auto* plugin = WPEFramework::Core::Service<WPEFramework::Plugin::AppManager>::Create<
    WPEFramework::PluginHost::IPlugin>();

// Implementation (AppManagerImplementation) — use the implementation class directly
auto* impl = WPEFramework::Core::Service<
    WPEFramework::Plugin::AppManagerImplementation>::Create<
    WPEFramework::Plugin::AppManagerImplementation>();

// Always release when done
impl->Release();
```

### 8.5 PluginAndService Helper Struct Pattern

For plugin lifecycle tests, group the plugin + service into a single RAII struct:

```cpp
struct PluginAndService {
    AppManagerServiceMock service;               // your ServiceMock extends PluginHost::IShell
    WPEFramework::PluginHost::IPlugin* plugin;

    PluginAndService()
        : service()
        , plugin(WPEFramework::Core::Service<WPEFramework::Plugin::AppManager>::Create<
                    WPEFramework::PluginHost::IPlugin>())
    {}

    ~PluginAndService() {
        if (nullptr != plugin) { plugin->Release(); plugin = nullptr; }
    }
};
```

### 8.6 ServiceMock Pattern for AppManager

The AppManager `ServiceMock` must implement at minimum:

```cpp
class AppManagerServiceMock : public WPEFramework::PluginHost::IShell {
public:
    std::atomic<uint32_t> addRefCalls{0};
    std::atomic<uint32_t> releaseCalls{0};

    // Configurable Instantiate() handler for Root<> control
    using InstantiateHandler = std::function<void*(
        const WPEFramework::RPC::Object&, const uint32_t, uint32_t&)>;
    void SetInstantiateHandler(InstantiateHandler h) { _instantiateHandler = h; }

    void AddRef() const override { addRefCalls.fetch_add(1); }
    uint32_t Release() const override { releaseCalls.fetch_add(1); return WPEFramework::Core::ERROR_NONE; }
    void Register(WPEFramework::RPC::IRemoteConnection::INotification*) override {}
    void Unregister(WPEFramework::RPC::IRemoteConnection::INotification*) override {}
    string ConfigLine() const override { return "{}"; }
    // QueryInterfaceByCallsign returns nullptr by default
    void* QueryInterfaceByCallsign(const uint32_t, const string&) override { return nullptr; }
    // Root<> uses Instantiate internally
    void* Instantiate(const WPEFramework::RPC::Object& obj, const uint32_t t, uint32_t& cId) override {
        return (_instantiateHandler ? _instantiateHandler(obj, t, cId) : nullptr);
    }
    // ... remaining IShell pure-virtuals with safe defaults
private:
    InstantiateHandler _instantiateHandler;
};
```

### 8.7 FakeNotification Pattern for Dispatch Testing

For `Dispatch()` and event-forwarding tests:

```cpp
namespace {

class FakeAppManagerNotification final
    : public WPEFramework::Exchange::IAppManager::INotification {
public:
    FakeAppManagerNotification() : _refCount(1) {}
    void AddRef() const override { _refCount.fetch_add(1); }
    uint32_t Release() const override {
        const uint32_t r = _refCount.fetch_sub(1) - 1;
        if (0U == r) { delete this; return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED; }
        return WPEFramework::Core::ERROR_NONE;
    }
    void* QueryInterface(const uint32_t) override { return nullptr; }

    BEGIN_INTERFACE_MAP(FakeAppManagerNotification)
    INTERFACE_ENTRY(WPEFramework::Exchange::IAppManager::INotification)
    END_INTERFACE_MAP

    // Capture call counts for assertions
    std::atomic<uint32_t> onInstalledCount{0};
    std::atomic<uint32_t> onUninstalledCount{0};
    std::atomic<uint32_t> onLifecycleChangedCount{0};
    std::atomic<uint32_t> onLaunchRequestCount{0};
    std::atomic<uint32_t> onUnloadedCount{0};

    void OnAppInstalled(const string&, const string&) override { onInstalledCount++; }
    void OnAppUninstalled(const string&) override { onUninstalledCount++; }
    void OnAppLifecycleStateChanged(const string&, const string&,
        WPEFramework::Exchange::IAppManager::AppLifecycleState,
        WPEFramework::Exchange::IAppManager::AppLifecycleState,
        WPEFramework::Exchange::IAppManager::AppErrorReason) override { onLifecycleChangedCount++; }
    void OnAppLaunchRequest(const string&, const string&, const string&) override { onLaunchRequestCount++; }
    void OnAppUnloaded(const string&, const string&) override { onUnloadedCount++; }

private:
    mutable std::atomic<uint32_t> _refCount;
};

} // anonymous namespace
```

### 8.8 Direct Dispatch Testing

Since `dispatchEvent()` submits jobs asynchronously via the worker pool, tests that need synchronous verification should call `Dispatch()` **directly**:

```cpp
uint32_t Test_Impl_DispatchEventLifecycleStateCallsRegisteredNotification()
{
    L0Test::TestResult tr;
    L0Test::L0BootstrapGuard guard;  // must come first

    auto* impl = WPEFramework::Core::Service<
        WPEFramework::Plugin::AppManagerImplementation>::Create<
        WPEFramework::Plugin::AppManagerImplementation>();

    auto* notification = new FakeAppManagerNotification();
    impl->Register(notification);

    // Build params manually
    WPEFramework::Core::JSON::DecUInt8 zero;
    WPEFramework::JsonObject params;
    params["appId"]       = "com.test.app";
    params["appInstanceId"] = "inst-001";
    params["newState"]    = static_cast<int>(WPEFramework::Exchange::IAppManager::APP_STATE_ACTIVE);
    params["oldState"]    = static_cast<int>(WPEFramework::Exchange::IAppManager::APP_STATE_LOADING);
    params["errorReason"] = static_cast<int>(WPEFramework::Exchange::IAppManager::APP_ERROR_NONE);

    // Call Dispatch() directly (bypasses worker pool)
    impl->Dispatch(WPEFramework::Plugin::AppManagerImplementation::APP_EVENT_LIFECYCLE_STATE_CHANGED, params);

    L0Test::ExpectEqU32(tr, notification->onLifecycleChangedCount.load(), 1,
        "OnAppLifecycleStateChanged called exactly once");

    impl->Unregister(notification);
    notification->Release();
    impl->Release();

    return tr.failures;
}
```

### 8.9 Bootstrap Requirement

All tests that instantiate Thunder objects or use `Core::IWorkerPool` **must** create an `L0Test::L0BootstrapGuard` before any plugin operations:

```cpp
uint32_t Test_SomethingRequiringWorkerPool()
{
    L0Test::TestResult tr;
    L0Test::L0BootstrapGuard guard;  // initializes worker pool

    // ... test code ...

    return tr.failures;  // guard destructor shuts down pool
}
```

Simple construction/destruction tests that don't call any Thunder async machinery do not require the guard.

### 8.10 Main Entry Point Pattern

```cpp
// AppManagerTest.cpp

#include "common/L0Bootstrap.hpp"
#include "common/L0TestTypes.hpp"

// Lifecycle tests
extern uint32_t Test_AppManager_InitializeFailsWhenRootCreationFails();
extern uint32_t Test_AppManager_InitializeFailsWhenConfigInterfaceMissing();
// ... all declarations ...

int main()
{
    L0Test::L0BootstrapGuard guard;
    uint32_t failures = 0;

    failures += Test_AppManager_InitializeFailsWhenRootCreationFails();
    failures += Test_AppManager_InitializeFailsWhenConfigInterfaceMissing();
    // ... all test calls ...

    L0Test::PrintTotals(std::cout, "AppManager L0 Tests", failures);
    return L0Test::ResultToExitCode(failures);
}
```

---

## 9. Suggested File Structure

```
Tests/L0Tests/AppManager/
├── CMakeLists.txt
├── AppManagerTest.cpp                         ← main() entry, extern declarations
├── AppManager_LifecycleTests.cpp              ← AppManager plugin layer (AM-L-*)
├── AppManager_ImplementationTests.cpp         ← AppManagerImplementation (AM-I-*)
├── AppManager_LifecycleConnectorTests.cpp     ← LifecycleInterfaceConnector (AM-LC-*)
├── AppManager_TelemetryTests.cpp              ← AppManagerTelemetryReporting (AM-T-*)
└── ServiceMock.h                              ← AppManagerServiceMock, extends L0ServiceMock
```

Each `*Tests.cpp` file follows the pattern established in `Tests/L0Tests/RuntimeManager/RuntimeManager_LifecycleTests.cpp` — one test per free function, all in the same translation unit for the given component.

---

*Generated based on AppManager plugin source code analysis. Refer to the [RuntimeManager L0 Tests](../RuntimeManager/) for working reference implementations.*
