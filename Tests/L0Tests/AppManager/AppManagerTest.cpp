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
