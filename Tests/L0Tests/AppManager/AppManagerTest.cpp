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
