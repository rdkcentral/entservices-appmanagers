/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdint>
#include <iostream>

#include "common/L0Bootstrap.hpp"
#include "common/L0TestTypes.hpp"

extern uint32_t Test_AM_InitializeSucceedsAndDeinitializeCleansUp();
extern uint32_t Test_AM_InformationReturnsEmptyString();
extern uint32_t Test_AM_RegisterAndUnregisterNotification();
extern uint32_t Test_AM_ConfigureWithNullServiceFails();
extern uint32_t Test_AM_ConfigureWithValidServiceReturnsSuccess();
extern uint32_t Test_AM_LaunchAppEmptyIdRejected();
extern uint32_t Test_AM_PreloadAppEmptyIdRejected();
extern uint32_t Test_AM_CloseTerminateKillSendIntentEmptyIdRejected();
extern uint32_t Test_AM_GetAppPropertyAndSetAppPropertyInvalidInputs();
extern uint32_t Test_AM_ClearAppDataAndClearAllAppDataWithoutDependencies();
extern uint32_t Test_AM_GetLoadedAppsWithoutConnectorFails();
extern uint32_t Test_AM_GetInstalledAppsWithoutPackagesFailsOrEmpty();
extern uint32_t Test_AM_UpdateCurrentActionHelpers();
extern uint32_t Test_AM_CheckInstallUninstallBlockWithoutPackages();
extern uint32_t Test_AM_IsInstalledAndGetInstalledAppsWithPackages();
extern uint32_t Test_AM_AppPropertyRoundTripWithStore();
extern uint32_t Test_AM_ClearAppDataAndSystemApisWithDependencies();
extern uint32_t Test_AM_CheckInstallUninstallBlockTrueForBlockedPackage();
extern uint32_t Test_AM_MapLifecycleStateAndErrorReason();
extern uint32_t Test_AM_LifecycleConnectorNullServiceConstructor();
extern uint32_t Test_AM_LifecycleConnectorOnAppStateChangedWithoutManager();
extern uint32_t Test_AM_LifecycleConnectorRemoveAppInfoByAppId();
extern uint32_t Test_AM_LifecycleConnectorGetAppInstanceId();
extern uint32_t Test_AM_LifecycleConnectorCreateAndGetLoadedAppsSuccess();
extern uint32_t Test_AM_LifecycleConnectorIsAppLoadedAndErrorPaths();
extern uint32_t Test_AM_LifecycleConnectorSendIntentAndKillEdgeCases();
extern uint32_t Test_AM_LifecycleConnectorStateCallbacksStability();
extern uint32_t Test_AM_AppInfoDefaultsAndSetters();
extern uint32_t Test_AM_AppInfoManagerCrudAndConvenienceAccessors();
extern uint32_t Test_AM_TelemetryReportingStability();
extern uint32_t Test_AM_SingletonPointerSetAndCleared();
extern uint32_t Test_AM_LifecycleConnectorLaunchNewApp();
extern uint32_t Test_AM_LifecycleConnectorLaunchSuspendedApp();
extern uint32_t Test_AM_LifecycleConnectorPreloadApp();
extern uint32_t Test_AM_LifecycleConnectorTerminateApp();
extern uint32_t Test_AM_LifecycleConnectorCloseApp();
extern uint32_t Test_AM_LifecycleConnectorReleaseRemoteObjects();
extern uint32_t Test_AM_LifecycleConnectorMapRemainingStates();
extern uint32_t Test_AM_ImplHandleOnAppUnloadedAndLaunchRequest();
extern uint32_t Test_AM_ImplOnAppInstallationStatus();
extern uint32_t Test_AM_AppInfoManagerUpsertUpdateBranch();
extern uint32_t Test_AM_ConfigurationGettersReturnDefaults();
extern uint32_t Test_AM_GetAppMetadataInvalidParams();
extern uint32_t Test_AM_StartAndStopSystemApp();
extern uint32_t Test_AM_LaunchAppWithPackageHandler();
extern uint32_t Test_AM_PreloadAppWithPackageHandler();
extern uint32_t Test_AM_IsInstalledMultiplePackages();
extern uint32_t Test_AM_GetInstalledAppsMixedTypes();
extern uint32_t Test_AM_WorkerThreadStability();
extern uint32_t Test_AM_ClearAppDataWithStorageManager();
extern uint32_t Test_AM_ClearAllAppDataWithStorageManager();
extern uint32_t Test_AM_GetLoadedAppsWithLifecycleConnector();
extern uint32_t Test_AM_SendIntentWithLifecycleConnector();
extern uint32_t Test_AM_CloseAppWithLifecycleConnector();
extern uint32_t Test_AM_TerminateAppWithLifecycleConnector();
extern uint32_t Test_AM_KillAppWithLifecycleConnector();
extern uint32_t Test_AM_AppInfoManagerIteratorEdgeCases();
extern uint32_t Test_AM_AppInfoManagerRemoveMultipleInstances();
extern uint32_t Test_AM_AppInfoManagerRemoveByInstanceIdEdgeCases();
extern uint32_t Test_AM_AppInfoManagerGetByAppIdMultipleInstances();
extern uint32_t Test_AM_AppInfoManagerUpsertEmptyFields();
extern uint32_t Test_AM_AppInfoManagerClear();
extern uint32_t Test_AM_AppInfoManagerBasicThreadSafety();
extern uint32_t Test_AM_AppInfoManagerUpdateOperationBranches();
extern uint32_t Test_AM_AppInfoManagerGetByCopyEdgeCases();
extern uint32_t Test_AM_AppInfoManagerStateSettersAndGetters();
extern uint32_t Test_AM_AppInfoManagerGetterNotFoundBranches();
extern uint32_t Test_AM_UnregisterNotFoundBranch();
extern uint32_t Test_AM_SetAppPropertyEmptyValue();
extern uint32_t Test_AM_GetInstalledAppsWithActiveAppInfo();
extern uint32_t Test_AM_UpdateCurrentActionTimeMismatchedAction();
extern uint32_t Test_AM_CheckInstallUninstallBlockUninstallBlockedState();
extern uint32_t Test_AM_OnAppInstallationStatusInstalledWithEmptyVersion();
extern uint32_t Test_AM_HandleOnAppLifecycleStateChangedLoadingToLoading();
extern uint32_t Test_AM_CreateOrUpdatePackageInfoEmptyVersion();
extern uint32_t Test_AM_CreateOrUpdatePackageInfoValidData();
extern uint32_t Test_AM_RemoveAppInfoByAppIdNotFound();
extern uint32_t Test_AM_OnAppInstallationStatusEmptyResponse();
extern uint32_t Test_AM_OnAppInstallationStatusInvalidJson();
extern uint32_t Test_AM_DispatchInstallationStatusEmptyPackageId();
extern uint32_t Test_AM_DispatchLaunchRequestEmptyIntentAndSource();
extern uint32_t Test_AM_DispatchUnloadedEmptyAppInstanceId();
extern uint32_t Test_AM_FetchPackageListErrorPath();
extern uint32_t Test_AM_GetAppPropertyGetValueFailed();
extern uint32_t Test_AM_LICLaunchWithEmptyAppId();
extern uint32_t Test_AM_LICPreloadWithEmptyAppId();
extern uint32_t Test_AM_LICOnAppStateChangedUnknownAppId();
extern uint32_t Test_AM_LICOnAppLifecycleStateChangedActiveNewState();
extern uint32_t Test_AM_LICOnAppLifecycleStateChangedUnloadedAbnormalClose();
extern uint32_t Test_AM_DispatchLoadingToLoadingNullConnector();
extern uint32_t Test_AM_DispatchLifecycleStateMissingAppId();
extern uint32_t Test_AM_DispatchLifecycleStateMissingNewState();
extern uint32_t Test_AM_DispatchLifecycleStateMissingOldState();
extern uint32_t Test_AM_DispatchLifecycleStateMissingErrorReason();
extern uint32_t Test_AM_DispatchLaunchRequestMissingAppId();
extern uint32_t Test_AM_DispatchUnloadedMissingAppId();
extern uint32_t Test_AM_DispatchDefaultUnknownEvent();
extern uint32_t Test_AM_DispatchLifecycleAllFieldsNoAppInstanceId();
extern uint32_t Test_AM_CheckInstallUninstallBlockMatchingNotBlocked();
extern uint32_t Test_AM_PackageUnlockFails();
extern uint32_t Test_AM_PackageUnlockAppNotInMap();
extern uint32_t Test_AM_TerminateAppSuccessPath();
extern uint32_t Test_AM_LICTerminateUnloadFails();
extern uint32_t Test_AM_KillAppSuccessPath();
extern uint32_t Test_AM_LICKillAppFails();
extern uint32_t Test_AM_LICSendIntentFails();
extern uint32_t Test_AM_LICSpawnAppFails();
extern uint32_t Test_AM_LICOnAppLifecycleStateChangedNormalClose();
extern uint32_t Test_AM_LICCloseAppPausedConfirmation();
extern uint32_t Test_AM_PackageLockLockFails();
extern uint32_t Test_AM_PackageLockFetchListFails();
extern uint32_t Test_AM_GetInstallAppTypeSystem();
extern uint32_t Test_AM_CloseAppNullConnector();
extern uint32_t Test_AM_TerminateAppNullConnector();
extern uint32_t Test_AM_KillAppNullConnector();
extern uint32_t Test_AM_SendIntentNullConnector();
extern uint32_t Test_AM_GetLoadedAppsNullConnector();
extern uint32_t Test_AM_LaunchAppNotInstalled();
extern uint32_t Test_AM_LaunchAppFetchFails();
extern uint32_t Test_AM_PreloadAppNotInstalled();
extern uint32_t Test_AM_PreloadAppFetchFails();
extern uint32_t Test_AM_LaunchAppNullConnectorInstalledApp();
extern uint32_t Test_AM_PackageLockAlreadyLoaded();
extern uint32_t Test_AM_ClearAppDataNullStorage();
extern uint32_t Test_AM_ClearAppDataStorageError();
extern uint32_t Test_AM_ClearAllAppDataNullStorage();
extern uint32_t Test_AM_ClearAllAppDataStorageError();
extern uint32_t Test_AM_WorkerThreadLaunchSuccess();
extern uint32_t Test_AM_WorkerThreadPackageLockFails();
extern uint32_t Test_AM_WorkerThreadPreloadSuccess();
extern uint32_t Test_AM_PackageLockAlreadyLoadedEmptyVersion();
extern uint32_t Test_AM_PackageLockIsAppLoadedError();
extern uint32_t Test_AM_CloseAppLICReturnsError();
extern uint32_t Test_AM_DispatchInstallationStatusUnknownState();
extern uint32_t Test_AM_PackageLockLocksSuccessfully();
extern uint32_t Test_AM_PackageLockInstalledEmptyVersion();
extern uint32_t Test_AM_PackageUnLockNullPackageHandler();
extern uint32_t Test_AM_CloseAppSuccessPath();
extern uint32_t Test_AM_GetAppPropertyNullStoreRecreates();
extern uint32_t Test_AM_RegisterNotificationAlreadyRegistered();
extern uint32_t Test_AM_PackageLockCreateOrUpdateFails();
extern uint32_t Test_AM_PackageLockNullHandlerNonEmptyVersion();
extern uint32_t Test_AM_PackageLockNullLifecycleConnector();
extern uint32_t Test_AM_LICLaunchLoadedNotSuspended();
extern uint32_t Test_AM_LICOnAppLifecycleStateChangedSuspended();
extern uint32_t Test_AM_LICOnAppLifecycleStateChangedUnknownNewState();
extern uint32_t Test_AM_ConfigureWithNullLifecycleManager();
extern uint32_t Test_AM_ConfigureWithNullLifecycleManagerState();
extern uint32_t Test_AM_ConfigureWithNullPersistentStore();
extern uint32_t Test_AM_ConfigureWithNullPackageHandler();
extern uint32_t Test_AM_ConfigureWithNullPackageInstaller();
extern uint32_t Test_AM_ConfigureWithNullStorageManager();
extern uint32_t Test_AM_IsInstalledNonInstalledState();
extern uint32_t Test_AM_GetInstalledAppsNoAppInfo();
extern uint32_t Test_AM_LICRemoveAppInfoByAppIdNotFound();
extern uint32_t Test_AM_LICOnAppStateChangedMultipleErrorReasons();
extern uint32_t Test_AM_TelReportDataNoAppInfo();
extern uint32_t Test_AM_TelReportDataActionMismatch();
extern uint32_t Test_AM_TelReportDataPreloadNoMarker();
extern uint32_t Test_AM_TelReportDataClosePublishClose();
extern uint32_t Test_AM_TelReportDataCloseTargetSuspended();
extern uint32_t Test_AM_TelReportDataCloseTargetHibernated();
extern uint32_t Test_AM_TelReportDataTerminateRecord();
extern uint32_t Test_AM_TelReportDataKillRecord();
extern uint32_t Test_AM_TelReportDataDefaultAction();
extern uint32_t Test_AM_TelOnStateChangeNoAppInfo();
extern uint32_t Test_AM_TelOnStateChangeLaunchNonActive();
extern uint32_t Test_AM_TelOnStateChangePreloadPaused();
extern uint32_t Test_AM_TelOnStateChangePreloadNonPaused();
extern uint32_t Test_AM_TelOnStateChangeCloseUnloaded();
extern uint32_t Test_AM_TelOnStateChangeTerminateUnloaded();
extern uint32_t Test_AM_TelOnStateChangeKillUnloaded();
extern uint32_t Test_AM_TelOnStateChangeCloseNonUnloaded();
extern uint32_t Test_AM_TelOnStateChangeDefaultAction();
extern uint32_t Test_AM_TelOnStateChangeLaunchInteractive();
extern uint32_t Test_AM_TelOnStateChangePreloadInteractive();
extern uint32_t Test_AM_TelErrorDataPreload();
extern uint32_t Test_AM_TelErrorDataClose();
extern uint32_t Test_AM_TelErrorDataTerminate();
extern uint32_t Test_AM_TelErrorDataKill();
extern uint32_t Test_AM_TelErrorDataDefault();
extern uint32_t Test_AM_TelRecordLaunchTime();

int main()
{
    L0Test::L0BootstrapGuard bootstrap;

    struct Case {
        const char* name;
        uint32_t (*fn)();
    };

    const Case cases[] = {
        { "AM_InitializeSucceedsAndDeinitializeCleansUp", Test_AM_InitializeSucceedsAndDeinitializeCleansUp },
        { "AM_InformationReturnsEmptyString", Test_AM_InformationReturnsEmptyString },
        { "AM_RegisterAndUnregisterNotification", Test_AM_RegisterAndUnregisterNotification },
        { "AM_ConfigureWithNullServiceFails", Test_AM_ConfigureWithNullServiceFails },
        { "AM_ConfigureWithValidServiceReturnsSuccess", Test_AM_ConfigureWithValidServiceReturnsSuccess },
        { "AM_LaunchAppEmptyIdRejected", Test_AM_LaunchAppEmptyIdRejected },
        { "AM_PreloadAppEmptyIdRejected", Test_AM_PreloadAppEmptyIdRejected },
        { "AM_CloseTerminateKillSendIntentEmptyIdRejected", Test_AM_CloseTerminateKillSendIntentEmptyIdRejected },
        { "AM_GetAppPropertyAndSetAppPropertyInvalidInputs", Test_AM_GetAppPropertyAndSetAppPropertyInvalidInputs },
        { "AM_ClearAppDataAndClearAllAppDataWithoutDependencies", Test_AM_ClearAppDataAndClearAllAppDataWithoutDependencies },
        { "AM_GetLoadedAppsWithoutConnectorFails", Test_AM_GetLoadedAppsWithoutConnectorFails },
        { "AM_GetInstalledAppsWithoutPackagesFailsOrEmpty", Test_AM_GetInstalledAppsWithoutPackagesFailsOrEmpty },
        { "AM_UpdateCurrentActionHelpers", Test_AM_UpdateCurrentActionHelpers },
        { "AM_CheckInstallUninstallBlockWithoutPackages", Test_AM_CheckInstallUninstallBlockWithoutPackages },
        { "AM_IsInstalledAndGetInstalledAppsWithPackages", Test_AM_IsInstalledAndGetInstalledAppsWithPackages },
        { "AM_AppPropertyRoundTripWithStore", Test_AM_AppPropertyRoundTripWithStore },
        { "AM_ClearAppDataAndSystemApisWithDependencies", Test_AM_ClearAppDataAndSystemApisWithDependencies },
        { "AM_CheckInstallUninstallBlockTrueForBlockedPackage", Test_AM_CheckInstallUninstallBlockTrueForBlockedPackage },
        { "AM_MapLifecycleStateAndErrorReason", Test_AM_MapLifecycleStateAndErrorReason },
        { "AM_LifecycleConnectorNullServiceConstructor", Test_AM_LifecycleConnectorNullServiceConstructor },
        { "AM_LifecycleConnectorOnAppStateChangedWithoutManager", Test_AM_LifecycleConnectorOnAppStateChangedWithoutManager },
        { "AM_LifecycleConnectorRemoveAppInfoByAppId", Test_AM_LifecycleConnectorRemoveAppInfoByAppId },
        { "AM_LifecycleConnectorGetAppInstanceId", Test_AM_LifecycleConnectorGetAppInstanceId },
        { "AM_LifecycleConnectorCreateAndGetLoadedAppsSuccess", Test_AM_LifecycleConnectorCreateAndGetLoadedAppsSuccess },
        { "AM_LifecycleConnectorIsAppLoadedAndErrorPaths", Test_AM_LifecycleConnectorIsAppLoadedAndErrorPaths },
        { "AM_LifecycleConnectorSendIntentAndKillEdgeCases", Test_AM_LifecycleConnectorSendIntentAndKillEdgeCases },
        { "AM_LifecycleConnectorStateCallbacksStability", Test_AM_LifecycleConnectorStateCallbacksStability },
        { "AM_AppInfoDefaultsAndSetters", Test_AM_AppInfoDefaultsAndSetters },
        { "AM_AppInfoManagerCrudAndConvenienceAccessors", Test_AM_AppInfoManagerCrudAndConvenienceAccessors },
        { "AM_TelemetryReportingStability", Test_AM_TelemetryReportingStability },
        { "AM_SingletonPointerSetAndCleared", Test_AM_SingletonPointerSetAndCleared },
        { "AM_LifecycleConnectorLaunchNewApp", Test_AM_LifecycleConnectorLaunchNewApp },
        { "AM_LifecycleConnectorLaunchSuspendedApp", Test_AM_LifecycleConnectorLaunchSuspendedApp },
        { "AM_LifecycleConnectorPreloadApp", Test_AM_LifecycleConnectorPreloadApp },
        { "AM_LifecycleConnectorTerminateApp", Test_AM_LifecycleConnectorTerminateApp },
        { "AM_LifecycleConnectorCloseApp", Test_AM_LifecycleConnectorCloseApp },
        { "AM_LifecycleConnectorReleaseRemoteObjects", Test_AM_LifecycleConnectorReleaseRemoteObjects },
        { "AM_LifecycleConnectorMapRemainingStates", Test_AM_LifecycleConnectorMapRemainingStates },
        { "AM_ImplHandleOnAppUnloadedAndLaunchRequest", Test_AM_ImplHandleOnAppUnloadedAndLaunchRequest },
        { "AM_ImplOnAppInstallationStatus", Test_AM_ImplOnAppInstallationStatus },
        { "AM_AppInfoManagerUpsertUpdateBranch", Test_AM_AppInfoManagerUpsertUpdateBranch },
        { "AM_ConfigurationGettersReturnDefaults", Test_AM_ConfigurationGettersReturnDefaults },
        { "AM_GetAppMetadataInvalidParams", Test_AM_GetAppMetadataInvalidParams },
        { "AM_StartAndStopSystemApp", Test_AM_StartAndStopSystemApp },
        { "AM_LaunchAppWithPackageHandler", Test_AM_LaunchAppWithPackageHandler },
        { "AM_PreloadAppWithPackageHandler", Test_AM_PreloadAppWithPackageHandler },
        { "AM_IsInstalledMultiplePackages", Test_AM_IsInstalledMultiplePackages },
        { "AM_GetInstalledAppsMixedTypes", Test_AM_GetInstalledAppsMixedTypes },
        { "AM_WorkerThreadStability", Test_AM_WorkerThreadStability },
        { "AM_ClearAppDataWithStorageManager", Test_AM_ClearAppDataWithStorageManager },
        { "AM_ClearAllAppDataWithStorageManager", Test_AM_ClearAllAppDataWithStorageManager },
        { "AM_GetLoadedAppsWithLifecycleConnector", Test_AM_GetLoadedAppsWithLifecycleConnector },
        { "AM_SendIntentWithLifecycleConnector", Test_AM_SendIntentWithLifecycleConnector },
        { "AM_CloseAppWithLifecycleConnector", Test_AM_CloseAppWithLifecycleConnector },
        { "AM_TerminateAppWithLifecycleConnector", Test_AM_TerminateAppWithLifecycleConnector },
        { "AM_KillAppWithLifecycleConnector", Test_AM_KillAppWithLifecycleConnector },
        { "AM_AppInfoManagerIteratorEdgeCases", Test_AM_AppInfoManagerIteratorEdgeCases },
        { "AM_AppInfoManagerRemoveMultipleInstances", Test_AM_AppInfoManagerRemoveMultipleInstances },
        { "AM_AppInfoManagerRemoveByInstanceIdEdgeCases", Test_AM_AppInfoManagerRemoveByInstanceIdEdgeCases },
        { "AM_AppInfoManagerGetByAppIdMultipleInstances", Test_AM_AppInfoManagerGetByAppIdMultipleInstances },
        { "AM_AppInfoManagerUpsertEmptyFields", Test_AM_AppInfoManagerUpsertEmptyFields },
        { "AM_AppInfoManagerClear", Test_AM_AppInfoManagerClear },
        { "AM_AppInfoManagerBasicThreadSafety", Test_AM_AppInfoManagerBasicThreadSafety },
        { "AM_AppInfoManagerUpdateOperationBranches", Test_AM_AppInfoManagerUpdateOperationBranches },
        { "AM_AppInfoManagerGetByCopyEdgeCases", Test_AM_AppInfoManagerGetByCopyEdgeCases },
        { "AM_AppInfoManagerStateSettersAndGetters", Test_AM_AppInfoManagerStateSettersAndGetters },
        { "AM_AppInfoManagerGetterNotFoundBranches", Test_AM_AppInfoManagerGetterNotFoundBranches },
        { "AM_UnregisterNotFoundBranch", Test_AM_UnregisterNotFoundBranch },
        { "AM_SetAppPropertyEmptyValue", Test_AM_SetAppPropertyEmptyValue },
        { "AM_GetInstalledAppsWithActiveAppInfo", Test_AM_GetInstalledAppsWithActiveAppInfo },
        { "AM_UpdateCurrentActionTimeMismatchedAction", Test_AM_UpdateCurrentActionTimeMismatchedAction },
        { "AM_CheckInstallUninstallBlockUninstallBlockedState", Test_AM_CheckInstallUninstallBlockUninstallBlockedState },
        { "AM_OnAppInstallationStatusInstalledWithEmptyVersion", Test_AM_OnAppInstallationStatusInstalledWithEmptyVersion },
        { "AM_HandleOnAppLifecycleStateChangedLoadingToLoading", Test_AM_HandleOnAppLifecycleStateChangedLoadingToLoading },
        { "AM_CreateOrUpdatePackageInfoEmptyVersion", Test_AM_CreateOrUpdatePackageInfoEmptyVersion },
        { "AM_CreateOrUpdatePackageInfoValidData", Test_AM_CreateOrUpdatePackageInfoValidData },
        { "AM_RemoveAppInfoByAppIdNotFound", Test_AM_RemoveAppInfoByAppIdNotFound },
        { "AM_OnAppInstallationStatusEmptyResponse", Test_AM_OnAppInstallationStatusEmptyResponse },
        { "AM_OnAppInstallationStatusInvalidJson", Test_AM_OnAppInstallationStatusInvalidJson },
        { "AM_DispatchInstallationStatusEmptyPackageId", Test_AM_DispatchInstallationStatusEmptyPackageId },
        { "AM_DispatchLaunchRequestEmptyIntentAndSource", Test_AM_DispatchLaunchRequestEmptyIntentAndSource },
        { "AM_DispatchUnloadedEmptyAppInstanceId", Test_AM_DispatchUnloadedEmptyAppInstanceId },
        { "AM_FetchPackageListErrorPath", Test_AM_FetchPackageListErrorPath },
        { "AM_GetAppPropertyGetValueFailed", Test_AM_GetAppPropertyGetValueFailed },
        { "AM_LICLaunchWithEmptyAppId", Test_AM_LICLaunchWithEmptyAppId },
        { "AM_LICPreloadWithEmptyAppId", Test_AM_LICPreloadWithEmptyAppId },
        { "AM_LICOnAppStateChangedUnknownAppId", Test_AM_LICOnAppStateChangedUnknownAppId },
        { "AM_LICOnAppLifecycleStateChangedActiveNewState", Test_AM_LICOnAppLifecycleStateChangedActiveNewState },
        { "AM_LICOnAppLifecycleStateChangedUnloadedAbnormalClose", Test_AM_LICOnAppLifecycleStateChangedUnloadedAbnormalClose },
        { "AM_DispatchLoadingToLoadingNullConnector", Test_AM_DispatchLoadingToLoadingNullConnector },
        { "AM_DispatchLifecycleStateMissingAppId", Test_AM_DispatchLifecycleStateMissingAppId },
        { "AM_DispatchLifecycleStateMissingNewState", Test_AM_DispatchLifecycleStateMissingNewState },
        { "AM_DispatchLifecycleStateMissingOldState", Test_AM_DispatchLifecycleStateMissingOldState },
        { "AM_DispatchLifecycleStateMissingErrorReason", Test_AM_DispatchLifecycleStateMissingErrorReason },
        { "AM_DispatchLaunchRequestMissingAppId", Test_AM_DispatchLaunchRequestMissingAppId },
        { "AM_DispatchUnloadedMissingAppId", Test_AM_DispatchUnloadedMissingAppId },
        { "AM_DispatchDefaultUnknownEvent", Test_AM_DispatchDefaultUnknownEvent },
        { "AM_DispatchLifecycleAllFieldsNoAppInstanceId", Test_AM_DispatchLifecycleAllFieldsNoAppInstanceId },
        { "AM_CheckInstallUninstallBlockMatchingNotBlocked", Test_AM_CheckInstallUninstallBlockMatchingNotBlocked },
        { "AM_PackageUnlockFails", Test_AM_PackageUnlockFails },
        { "AM_PackageUnlockAppNotInMap", Test_AM_PackageUnlockAppNotInMap },
        { "AM_TerminateAppSuccessPath", Test_AM_TerminateAppSuccessPath },
        { "AM_LICTerminateUnloadFails", Test_AM_LICTerminateUnloadFails },
        { "AM_KillAppSuccessPath", Test_AM_KillAppSuccessPath },
        { "AM_LICKillAppFails", Test_AM_LICKillAppFails },
        { "AM_LICSendIntentFails", Test_AM_LICSendIntentFails },
        { "AM_LICSpawnAppFails", Test_AM_LICSpawnAppFails },
        { "AM_LICOnAppLifecycleStateChangedNormalClose", Test_AM_LICOnAppLifecycleStateChangedNormalClose },
        { "AM_LICCloseAppPausedConfirmation", Test_AM_LICCloseAppPausedConfirmation },
        { "AM_PackageLockLockFails", Test_AM_PackageLockLockFails },
        { "AM_PackageLockFetchListFails", Test_AM_PackageLockFetchListFails },
        { "AM_GetInstallAppTypeSystem", Test_AM_GetInstallAppTypeSystem },
        { "AM_CloseAppNullConnector", Test_AM_CloseAppNullConnector },
        { "AM_TerminateAppNullConnector", Test_AM_TerminateAppNullConnector },
        { "AM_KillAppNullConnector", Test_AM_KillAppNullConnector },
        { "AM_SendIntentNullConnector", Test_AM_SendIntentNullConnector },
        { "AM_GetLoadedAppsNullConnector", Test_AM_GetLoadedAppsNullConnector },
        { "AM_LaunchAppNotInstalled", Test_AM_LaunchAppNotInstalled },
        { "AM_LaunchAppFetchFails", Test_AM_LaunchAppFetchFails },
        { "AM_PreloadAppNotInstalled", Test_AM_PreloadAppNotInstalled },
        { "AM_PreloadAppFetchFails", Test_AM_PreloadAppFetchFails },
        { "AM_LaunchAppNullConnectorInstalledApp", Test_AM_LaunchAppNullConnectorInstalledApp },
        { "AM_PackageLockAlreadyLoaded", Test_AM_PackageLockAlreadyLoaded },
        { "AM_ClearAppDataNullStorage", Test_AM_ClearAppDataNullStorage },
        { "AM_ClearAppDataStorageError", Test_AM_ClearAppDataStorageError },
        { "AM_ClearAllAppDataNullStorage", Test_AM_ClearAllAppDataNullStorage },
        { "AM_ClearAllAppDataStorageError", Test_AM_ClearAllAppDataStorageError },
        { "AM_WorkerThreadLaunchSuccess", Test_AM_WorkerThreadLaunchSuccess },
        { "AM_WorkerThreadPackageLockFails", Test_AM_WorkerThreadPackageLockFails },
        { "AM_WorkerThreadPreloadSuccess", Test_AM_WorkerThreadPreloadSuccess },
        { "AM_PackageLockAlreadyLoadedEmptyVersion", Test_AM_PackageLockAlreadyLoadedEmptyVersion },
        { "AM_PackageLockIsAppLoadedError", Test_AM_PackageLockIsAppLoadedError },
        { "AM_CloseAppLICReturnsError", Test_AM_CloseAppLICReturnsError },
        { "AM_DispatchInstallationStatusUnknownState", Test_AM_DispatchInstallationStatusUnknownState },
        { "AM_PackageLockLocksSuccessfully", Test_AM_PackageLockLocksSuccessfully },
        { "AM_PackageLockInstalledEmptyVersion", Test_AM_PackageLockInstalledEmptyVersion },
        { "AM_PackageUnLockNullPackageHandler", Test_AM_PackageUnLockNullPackageHandler },
        { "AM_CloseAppSuccessPath", Test_AM_CloseAppSuccessPath },
        { "AM_GetAppPropertyNullStoreRecreates", Test_AM_GetAppPropertyNullStoreRecreates },
        { "AM_RegisterNotificationAlreadyRegistered", Test_AM_RegisterNotificationAlreadyRegistered },
        { "AM_PackageLockCreateOrUpdateFails", Test_AM_PackageLockCreateOrUpdateFails },
        { "AM_PackageLockNullHandlerNonEmptyVersion", Test_AM_PackageLockNullHandlerNonEmptyVersion },
        { "AM_PackageLockNullLifecycleConnector", Test_AM_PackageLockNullLifecycleConnector },
        { "AM_LICLaunchLoadedNotSuspended", Test_AM_LICLaunchLoadedNotSuspended },
        { "AM_LICOnAppLifecycleStateChangedSuspended", Test_AM_LICOnAppLifecycleStateChangedSuspended },
        { "AM_LICOnAppLifecycleStateChangedUnknownNewState", Test_AM_LICOnAppLifecycleStateChangedUnknownNewState },
        { "AM_ConfigureWithNullLifecycleManager", Test_AM_ConfigureWithNullLifecycleManager },
        { "AM_ConfigureWithNullLifecycleManagerState", Test_AM_ConfigureWithNullLifecycleManagerState },
        { "AM_ConfigureWithNullPersistentStore", Test_AM_ConfigureWithNullPersistentStore },
        { "AM_ConfigureWithNullPackageHandler", Test_AM_ConfigureWithNullPackageHandler },
        { "AM_ConfigureWithNullPackageInstaller", Test_AM_ConfigureWithNullPackageInstaller },
        { "AM_ConfigureWithNullStorageManager", Test_AM_ConfigureWithNullStorageManager },
        { "AM_IsInstalledNonInstalledState", Test_AM_IsInstalledNonInstalledState },
        { "AM_GetInstalledAppsNoAppInfo", Test_AM_GetInstalledAppsNoAppInfo },
        { "AM_LICRemoveAppInfoByAppIdNotFound", Test_AM_LICRemoveAppInfoByAppIdNotFound },
        { "AM_LICOnAppStateChangedMultipleErrorReasons", Test_AM_LICOnAppStateChangedMultipleErrorReasons },
        { "AM_TelReportDataNoAppInfo", Test_AM_TelReportDataNoAppInfo },
        { "AM_TelReportDataActionMismatch", Test_AM_TelReportDataActionMismatch },
        { "AM_TelReportDataPreloadNoMarker", Test_AM_TelReportDataPreloadNoMarker },
        { "AM_TelReportDataClosePublishClose", Test_AM_TelReportDataClosePublishClose },
        { "AM_TelReportDataCloseTargetSuspended", Test_AM_TelReportDataCloseTargetSuspended },
        { "AM_TelReportDataCloseTargetHibernated", Test_AM_TelReportDataCloseTargetHibernated },
        { "AM_TelReportDataTerminateRecord", Test_AM_TelReportDataTerminateRecord },
        { "AM_TelReportDataKillRecord", Test_AM_TelReportDataKillRecord },
        { "AM_TelReportDataDefaultAction", Test_AM_TelReportDataDefaultAction },
        { "AM_TelOnStateChangeNoAppInfo", Test_AM_TelOnStateChangeNoAppInfo },
        { "AM_TelOnStateChangeLaunchNonActive", Test_AM_TelOnStateChangeLaunchNonActive },
        { "AM_TelOnStateChangePreloadPaused", Test_AM_TelOnStateChangePreloadPaused },
        { "AM_TelOnStateChangePreloadNonPaused", Test_AM_TelOnStateChangePreloadNonPaused },
        { "AM_TelOnStateChangeCloseUnloaded", Test_AM_TelOnStateChangeCloseUnloaded },
        { "AM_TelOnStateChangeTerminateUnloaded", Test_AM_TelOnStateChangeTerminateUnloaded },
        { "AM_TelOnStateChangeKillUnloaded", Test_AM_TelOnStateChangeKillUnloaded },
        { "AM_TelOnStateChangeCloseNonUnloaded", Test_AM_TelOnStateChangeCloseNonUnloaded },
        { "AM_TelOnStateChangeDefaultAction", Test_AM_TelOnStateChangeDefaultAction },
        { "AM_TelOnStateChangeLaunchInteractive", Test_AM_TelOnStateChangeLaunchInteractive },
        { "AM_TelOnStateChangePreloadInteractive", Test_AM_TelOnStateChangePreloadInteractive },
        { "AM_TelErrorDataPreload", Test_AM_TelErrorDataPreload },
        { "AM_TelErrorDataClose", Test_AM_TelErrorDataClose },
        { "AM_TelErrorDataTerminate", Test_AM_TelErrorDataTerminate },
        { "AM_TelErrorDataKill", Test_AM_TelErrorDataKill },
        { "AM_TelErrorDataDefault", Test_AM_TelErrorDataDefault },
        { "AM_TelRecordLaunchTime", Test_AM_TelRecordLaunchTime },
    };

    uint32_t totalFailures = 0;
    for (const auto& c : cases) {
        const uint32_t failures = c.fn();
        if (failures > 0U) {
            std::cerr << "[FAIL] " << c.name << " (" << failures << " assertion(s) failed)" << std::endl;
            totalFailures += failures;
        } else {
            std::cout << "[PASS] " << c.name << std::endl;
        }
    }

    L0Test::PrintTotals(std::cout, "AppManager_L0", totalFailures);
    return L0Test::ResultToExitCode(totalFailures);
}
