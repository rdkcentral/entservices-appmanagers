/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
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
 **/

/**
 * @file AppManagerTest.cpp
 *
 * L0 test entry point for the AppManager plugin.
 * Aggregates all test functions, runs them in sequence,
 * and reports a GTest-style summary to stderr/stdout.
 *
 * Build: see CMakeLists.txt in this directory.
 * Run:   ./appmanager_l0test
 */

#include <cstdint>
#include <iostream>

#include "common/L0Bootstrap.hpp"
#include "common/L0TestTypes.hpp"

// ── AppManager_LifecycleTests.cpp ─────────────────────────────────────────────
extern uint32_t Test_AppManager_InitializeFailsWhenRootCreationFails();
extern uint32_t Test_AppManager_InitializeFailsWhenConfigInterfaceMissing();
extern uint32_t Test_AppManager_InitializeFailsWhenConfigureFails();
extern uint32_t Test_AppManager_InitializeSucceedsAndDeinitializeCleansUp();
extern uint32_t Test_AppManager_InformationReturnsServiceName();

// ── AppManager_ImplementationTests.cpp ───────────────────────────────────────
extern uint32_t Test_Impl_RegisterNotificationSucceeds();
extern uint32_t Test_Impl_RegisterDuplicateNotificationIsIdempotent();
extern uint32_t Test_Impl_UnregisterRegisteredNotificationSucceeds();
extern uint32_t Test_Impl_UnregisterUnknownNotificationReturnsError();
extern uint32_t Test_Impl_ConfigureNullServiceReturnsError();
extern uint32_t Test_Impl_ConfigureWithServiceSucceeds();
extern uint32_t Test_Impl_LaunchAppEmptyAppIdReturnsInvalidParam();
extern uint32_t Test_Impl_LaunchAppWithNoPackageManagerReturnsError();
extern uint32_t Test_Impl_CloseAppEmptyAppIdReturnsError();
extern uint32_t Test_Impl_CloseAppNoLifecycleConnectorReturnsError();
extern uint32_t Test_Impl_TerminateAppEmptyAppIdReturnsError();
extern uint32_t Test_Impl_TerminateAppNoLifecycleConnectorReturnsError();
extern uint32_t Test_Impl_KillAppNoConnectorReturnsNone();
extern uint32_t Test_Impl_GetLoadedAppsNoConnectorReturnsError();
extern uint32_t Test_Impl_SendIntentNoConnectorReturnsNone();
extern uint32_t Test_Impl_PreloadAppEmptyAppIdReturnsError();
extern uint32_t Test_Impl_PreloadAppNoPackageManagerReturnsError();
extern uint32_t Test_Impl_GetAppPropertyEmptyAppIdReturnsError();
extern uint32_t Test_Impl_GetAppPropertyEmptyKeyReturnsError();
extern uint32_t Test_Impl_SetAppPropertyEmptyAppIdReturnsError();
extern uint32_t Test_Impl_SetAppPropertyNoPersistentStoreReturnsError();
extern uint32_t Test_Impl_GetInstalledAppsNoPackageManagerReturnsError();
extern uint32_t Test_Impl_IsInstalledNoPackageManagerReturnsError();
extern uint32_t Test_Impl_StartSystemAppReturnsNone();
extern uint32_t Test_Impl_StopSystemAppReturnsNone();
extern uint32_t Test_Impl_ClearAppDataEmptyAppIdReturnsError();
extern uint32_t Test_Impl_ClearAppDataNoStorageManagerReturnsError();
extern uint32_t Test_Impl_ClearAllAppDataNoStorageManagerReturnsError();
extern uint32_t Test_Impl_GetMaxRunningAppsReturnsNone();
extern uint32_t Test_Impl_GetMaxHibernatedAppsReturnsNone();
extern uint32_t Test_Impl_GetMaxHibernatedFlashUsageReturnsNone();
extern uint32_t Test_Impl_GetMaxInactiveRamUsageReturnsNone();
extern uint32_t Test_Impl_HandleOnAppUnloadedEmptyAppIdDoesNotDispatch();
extern uint32_t Test_Impl_HandleOnAppLaunchRequestEmptyAppIdDoesNotDispatch();
extern uint32_t Test_Impl_HandleOnAppLifecycleStateChangedDispatchesNotification();
extern uint32_t Test_Impl_GetInstallAppTypeInteractiveReturnsCorrectString();
extern uint32_t Test_Impl_GetInstallAppTypeSystemReturnsCorrectString();
extern uint32_t Test_Impl_GetInstallAppTypeUnknownReturnsEmptyString();
extern uint32_t Test_Impl_GetInstanceReturnsSelf();
extern uint32_t Test_Impl_GetInstanceBeforeConstructionReturnsNull();

// ── AppManager_ComponentTests.cpp ────────────────────────────────────────────
extern uint32_t Test_Comp_HandleOnAppUnloadedValidAppIdDispatchesCallback();
extern uint32_t Test_Comp_HandleOnAppLaunchRequestValidAppIdDispatchesCallback();
extern uint32_t Test_Comp_OnAppInstallationStatusInstalledDispatchesCallback();
extern uint32_t Test_Comp_MAppInfoDirectInsertionAndLookup();
extern uint32_t Test_Comp_HandleOnAppUnloadedRemovesAppInfoEntry();
extern uint32_t Test_Comp_UpdateCurrentActionFoundUpdatesEntry();
extern uint32_t Test_Comp_UpdateCurrentActionNotFoundDoesNotCrash();
extern uint32_t Test_Comp_CheckInstallUninstallBlockNoPackageManagerReturnsFalse();
extern uint32_t Test_Comp_GetAppMetadataReturnsNone();
extern uint32_t Test_Comp_GetAppPropertyNoStoreReturnsError();
extern uint32_t Test_Comp_MultipleNotificationsReceiveDispatchedEvent();

// ─────────────────────────────────────────────────────────────────────────────

int main()
{
    L0Test::L0BootstrapGuard bootstrap;

    struct Case {
        const char* name;
        uint32_t (*fn)();
    };

    const Case cases[] = {
        // ── Lifecycle tests ──────────────────────────────────────────────────
        { "AppManager_InitializeFailsWhenRootCreationFails",         Test_AppManager_InitializeFailsWhenRootCreationFails         },
        { "AppManager_InitializeFailsWhenConfigInterfaceMissing",    Test_AppManager_InitializeFailsWhenConfigInterfaceMissing    },
        { "AppManager_InitializeFailsWhenConfigureFails",            Test_AppManager_InitializeFailsWhenConfigureFails            },
        { "AppManager_InitializeSucceedsAndDeinitializeCleansUp",    Test_AppManager_InitializeSucceedsAndDeinitializeCleansUp    },
        { "AppManager_InformationReturnsServiceName",                Test_AppManager_InformationReturnsServiceName                },

        // ── Implementation tests ─────────────────────────────────────────────
        { "Impl_RegisterNotificationSucceeds",                       Test_Impl_RegisterNotificationSucceeds                       },
        { "Impl_RegisterDuplicateNotificationIsIdempotent",          Test_Impl_RegisterDuplicateNotificationIsIdempotent          },
        { "Impl_UnregisterRegisteredNotificationSucceeds",           Test_Impl_UnregisterRegisteredNotificationSucceeds           },
        { "Impl_UnregisterUnknownNotificationReturnsError",          Test_Impl_UnregisterUnknownNotificationReturnsError          },
        { "Impl_ConfigureNullServiceReturnsError",                   Test_Impl_ConfigureNullServiceReturnsError                   },
        { "Impl_ConfigureWithServiceSucceeds",                       Test_Impl_ConfigureWithServiceSucceeds                       },
        { "Impl_LaunchAppEmptyAppIdReturnsInvalidParam",             Test_Impl_LaunchAppEmptyAppIdReturnsInvalidParam             },
        { "Impl_LaunchAppWithNoPackageManagerReturnsError",          Test_Impl_LaunchAppWithNoPackageManagerReturnsError          },
        { "Impl_CloseAppEmptyAppIdReturnsError",                     Test_Impl_CloseAppEmptyAppIdReturnsError                     },
        { "Impl_CloseAppNoLifecycleConnectorReturnsError",           Test_Impl_CloseAppNoLifecycleConnectorReturnsError           },
        { "Impl_TerminateAppEmptyAppIdReturnsError",                 Test_Impl_TerminateAppEmptyAppIdReturnsError                 },
        { "Impl_TerminateAppNoLifecycleConnectorReturnsError",       Test_Impl_TerminateAppNoLifecycleConnectorReturnsError       },
        { "Impl_KillAppNoConnectorReturnsNone",                      Test_Impl_KillAppNoConnectorReturnsNone                      },
        { "Impl_GetLoadedAppsNoConnectorReturnsError",               Test_Impl_GetLoadedAppsNoConnectorReturnsError               },
        { "Impl_SendIntentNoConnectorReturnsNone",                   Test_Impl_SendIntentNoConnectorReturnsNone                   },
        { "Impl_PreloadAppEmptyAppIdReturnsError",                   Test_Impl_PreloadAppEmptyAppIdReturnsError                   },
        { "Impl_PreloadAppNoPackageManagerReturnsError",             Test_Impl_PreloadAppNoPackageManagerReturnsError             },
        { "Impl_GetAppPropertyEmptyAppIdReturnsError",               Test_Impl_GetAppPropertyEmptyAppIdReturnsError               },
        { "Impl_GetAppPropertyEmptyKeyReturnsError",                 Test_Impl_GetAppPropertyEmptyKeyReturnsError                 },
        { "Impl_SetAppPropertyEmptyAppIdReturnsError",               Test_Impl_SetAppPropertyEmptyAppIdReturnsError               },
        { "Impl_SetAppPropertyNoPersistentStoreReturnsError",        Test_Impl_SetAppPropertyNoPersistentStoreReturnsError        },
        { "Impl_GetInstalledAppsNoPackageManagerReturnsError",       Test_Impl_GetInstalledAppsNoPackageManagerReturnsError       },
        { "Impl_IsInstalledNoPackageManagerReturnsError",            Test_Impl_IsInstalledNoPackageManagerReturnsError            },
        { "Impl_StartSystemAppReturnsNone",                          Test_Impl_StartSystemAppReturnsNone                          },
        { "Impl_StopSystemAppReturnsNone",                           Test_Impl_StopSystemAppReturnsNone                           },
        { "Impl_ClearAppDataEmptyAppIdReturnsError",                 Test_Impl_ClearAppDataEmptyAppIdReturnsError                 },
        { "Impl_ClearAppDataNoStorageManagerReturnsError",           Test_Impl_ClearAppDataNoStorageManagerReturnsError           },
        { "Impl_ClearAllAppDataNoStorageManagerReturnsError",        Test_Impl_ClearAllAppDataNoStorageManagerReturnsError        },
        { "Impl_GetMaxRunningAppsReturnsNone",                       Test_Impl_GetMaxRunningAppsReturnsNone                       },
        { "Impl_GetMaxHibernatedAppsReturnsNone",                    Test_Impl_GetMaxHibernatedAppsReturnsNone                    },
        { "Impl_GetMaxHibernatedFlashUsageReturnsNone",              Test_Impl_GetMaxHibernatedFlashUsageReturnsNone              },
        { "Impl_GetMaxInactiveRamUsageReturnsNone",                  Test_Impl_GetMaxInactiveRamUsageReturnsNone                  },
        { "Impl_HandleOnAppUnloadedEmptyAppIdDoesNotDispatch",       Test_Impl_HandleOnAppUnloadedEmptyAppIdDoesNotDispatch       },
        { "Impl_HandleOnAppLaunchRequestEmptyAppIdDoesNotDispatch",  Test_Impl_HandleOnAppLaunchRequestEmptyAppIdDoesNotDispatch  },
        { "Impl_HandleOnAppLifecycleStateChangedDispatchesNotif",    Test_Impl_HandleOnAppLifecycleStateChangedDispatchesNotification },
        { "Impl_GetInstallAppTypeInteractiveReturnsCorrectString",   Test_Impl_GetInstallAppTypeInteractiveReturnsCorrectString   },
        { "Impl_GetInstallAppTypeSystemReturnsCorrectString",        Test_Impl_GetInstallAppTypeSystemReturnsCorrectString        },
        { "Impl_GetInstallAppTypeUnknownReturnsEmptyString",         Test_Impl_GetInstallAppTypeUnknownReturnsEmptyString         },
        { "Impl_GetInstanceReturnsSelf",                             Test_Impl_GetInstanceReturnsSelf                             },
        { "Impl_GetInstanceBeforeConstructionReturnsNull",           Test_Impl_GetInstanceBeforeConstructionReturnsNull           },

        // ── Component tests ──────────────────────────────────────────────────
        { "Comp_HandleOnAppUnloadedValidDispatchesCallback",         Test_Comp_HandleOnAppUnloadedValidAppIdDispatchesCallback    },
        { "Comp_HandleOnAppLaunchRequestValidDispatchesCallback",    Test_Comp_HandleOnAppLaunchRequestValidAppIdDispatchesCallback },
        { "Comp_OnAppInstallationStatusInstalledDispatch",           Test_Comp_OnAppInstallationStatusInstalledDispatchesCallback },
        { "Comp_MAppInfoDirectInsertionAndLookup",                   Test_Comp_MAppInfoDirectInsertionAndLookup                   },
        { "Comp_HandleOnAppUnloadedRemovesAppInfoEntry",             Test_Comp_HandleOnAppUnloadedRemovesAppInfoEntry             },
        { "Comp_UpdateCurrentActionFoundUpdatesEntry",               Test_Comp_UpdateCurrentActionFoundUpdatesEntry               },
        { "Comp_UpdateCurrentActionNotFoundDoesNotCrash",            Test_Comp_UpdateCurrentActionNotFoundDoesNotCrash            },
        { "Comp_CheckInstallUninstallBlockNoPackageMgrReturnsFalse", Test_Comp_CheckInstallUninstallBlockNoPackageManagerReturnsFalse },
        { "Comp_GetAppMetadataReturnsNone",                          Test_Comp_GetAppMetadataReturnsNone                          },
        { "Comp_GetAppPropertyNoStoreReturnsError",                  Test_Comp_GetAppPropertyNoStoreReturnsError                  },
        { "Comp_MultipleNotificationsReceiveDispatchedEvent",        Test_Comp_MultipleNotificationsReceiveDispatchedEvent        },
    };

    uint32_t failures = 0;

    for (const auto& c : cases) {
        std::cerr << "[ RUN      ] " << c.name << std::endl;
        const uint32_t f = c.fn();
        if (f == 0) {
            std::cerr << "[       OK ] " << c.name << std::endl;
        } else {
            std::cerr << "[  FAILED  ] " << c.name << " failures=" << f << std::endl;
        }
        failures += f;
    }

    L0Test::PrintTotals(std::cout, "AppManager l0test", failures);
    return L0Test::ResultToExitCode(failures);
}
