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
 * @file PreinstallManagerTest.cpp
 *
 * Main entry point for the PreinstallManager L0 test suite.
 *
 * All test functions are declared as extern here and invoked sequentially.
 * Each function returns uint32_t: 0 on pass, or the number of inner assertion
 * failures.
 */

#include <cstdint>
#include <iostream>

#include "common/L0Bootstrap.hpp"
#include "common/L0TestTypes.hpp"

// ── Lifecycle tests (PreinstallManager_LifecycleTests.cpp) ───────────────────
extern uint32_t Test_PIM_InitializeFailsWhenRootCreationFails();
extern uint32_t Test_PIM_InitializeSucceedsAndDeinitializeCleansUp();
extern uint32_t Test_PIM_InitializeFailsWhenConfigurationInterfaceMissing();
extern uint32_t Test_PIM_InitializeFailsWhenConfigureFails();
extern uint32_t Test_PIM_InformationReturnsEmptyString();
extern uint32_t Test_PIM_SingletonBehaviorConstructAndDestruct();

// ── Implementation tests (PreinstallManager_ImplementationTests.cpp) ─────────
extern uint32_t Test_Impl_PIM_RegisterNotification();
extern uint32_t Test_Impl_PIM_RegisterDuplicate();
extern uint32_t Test_Impl_PIM_UnregisterNotification();
extern uint32_t Test_Impl_PIM_UnregisterNotRegistered();
extern uint32_t Test_Impl_PIM_ConfigureWithNullService();
extern uint32_t Test_Impl_PIM_ConfigureParsesPreinstallDirectory();
extern uint32_t Test_Impl_PIM_ConfigureWithEmptyDirectoryParsesOk();
extern uint32_t Test_Impl_PIM_GetPreinstallStateInitiallyNotStarted();
extern uint32_t Test_Impl_PIM_GetPreinstallStateAfterConfigureStillNotStarted();
extern uint32_t Test_Impl_PIM_GetInstanceReturnsSelf();
extern uint32_t Test_Impl_PIM_GetInstanceAfterDestructionReturnsNull();
extern uint32_t Test_Impl_PIM_QueryInterfaceIPreinstallManager();
extern uint32_t Test_Impl_PIM_QueryInterfaceIConfiguration();
extern uint32_t Test_Impl_PIM_QueryInterfaceUnknownID();
extern uint32_t Test_Impl_PIM_MultipleNotificationsRegisterUnregister();
extern uint32_t Test_Impl_PIM_StartPreinstallWithoutServiceFails();
extern uint32_t Test_Impl_PIM_StartPreinstallWithNoInstallerFails();

// ── Component tests (PreinstallManager_ComponentTests.cpp) ───────────────────
extern uint32_t Test_Comp_PIM_StartPreinstallInvalidDirectoryFails();
extern uint32_t Test_Comp_PIM_StartPreinstallEmptyDirSendsCompletionEvent();
extern uint32_t Test_Comp_PIM_StartPreinstallAlreadyInProgressReturnsError();
extern uint32_t Test_Comp_PIM_StartPreinstallForceInstallAllPackages();
extern uint32_t Test_Comp_PIM_StartPreinstallFiltersOlderVersionWhenNotForce();
extern uint32_t Test_Comp_PIM_StartPreinstallInstallsNewerVersionWhenNotForce();
extern uint32_t Test_Comp_PIM_StartPreinstallEqualVersionFilteredSendsCompletionEvent();
extern uint32_t Test_Comp_PIM_InstallPackagesWithInvalidFieldsSkipsPackage();
extern uint32_t Test_Comp_PIM_InstallPackagesHandlesInstallFailure();
extern uint32_t Test_Comp_PIM_GetFailReasonAllEnumsWithoutCrash();
extern uint32_t Test_Comp_PIM_DispatchFiresPreinstallCompleteOnAllObservers();
extern uint32_t Test_Comp_PIM_StartPreinstallJoinsPreviousCompletedThread();
extern uint32_t Test_Comp_PIM_InstallPackagesCreateManagerFails();
extern uint32_t Test_Comp_PIM_ReadPreinstallDirectorySkipsDotEntries();
extern uint32_t Test_Comp_PIM_StartPreinstallVersionWithSuffixStripped();
extern uint32_t Test_Comp_PIM_InvalidVersionDoesNotCrash();
extern uint32_t Test_Comp_PIM_SendOnPreinstallationCompleteEventEnqueuesJob();
extern uint32_t Test_Comp_PIM_ReadPreinstallDirectoryLoadsValidPackages();

int main()
{
    L0Test::L0BootstrapGuard bootstrap;

    struct Case {
        const char*   name;
        uint32_t    (*fn)();
    };

    const Case cases[] = {
        // ── Lifecycle tests ──────────────────────────────────────────────────
        { "PIM_InitializeFailsWhenRootCreationFails",           Test_PIM_InitializeFailsWhenRootCreationFails           },
        { "PIM_InitializeSucceedsAndDeinitializeCleansUp",      Test_PIM_InitializeSucceedsAndDeinitializeCleansUp      },
        { "PIM_InitializeFailsWhenConfigurationInterfaceMissing", Test_PIM_InitializeFailsWhenConfigurationInterfaceMissing },
        { "PIM_InitializeFailsWhenConfigureFails",              Test_PIM_InitializeFailsWhenConfigureFails              },
        { "PIM_InformationReturnsEmptyString",                  Test_PIM_InformationReturnsEmptyString                  },
        { "PIM_SingletonBehaviorConstructAndDestruct",          Test_PIM_SingletonBehaviorConstructAndDestruct          },

        // ── Implementation tests ─────────────────────────────────────────────
        { "Impl_PIM_RegisterNotification",                      Test_Impl_PIM_RegisterNotification                      },
        { "Impl_PIM_RegisterDuplicate",                         Test_Impl_PIM_RegisterDuplicate                         },
        { "Impl_PIM_UnregisterNotification",                    Test_Impl_PIM_UnregisterNotification                    },
        { "Impl_PIM_UnregisterNotRegistered",                   Test_Impl_PIM_UnregisterNotRegistered                   },
        { "Impl_PIM_ConfigureWithNullService",                  Test_Impl_PIM_ConfigureWithNullService                  },
        { "Impl_PIM_ConfigureParsesPreinstallDirectory",        Test_Impl_PIM_ConfigureParsesPreinstallDirectory        },
        { "Impl_PIM_ConfigureWithEmptyDirectoryParsesOk",       Test_Impl_PIM_ConfigureWithEmptyDirectoryParsesOk       },
        { "Impl_PIM_GetPreinstallStateInitiallyNotStarted",     Test_Impl_PIM_GetPreinstallStateInitiallyNotStarted     },
        { "Impl_PIM_GetPreinstallStateAfterConfigureStillNotStarted", Test_Impl_PIM_GetPreinstallStateAfterConfigureStillNotStarted },
        { "Impl_PIM_GetInstanceReturnsSelf",                    Test_Impl_PIM_GetInstanceReturnsSelf                    },
        { "Impl_PIM_GetInstanceAfterDestructionReturnsNull",    Test_Impl_PIM_GetInstanceAfterDestructionReturnsNull    },
        { "Impl_PIM_QueryInterfaceIPreinstallManager",          Test_Impl_PIM_QueryInterfaceIPreinstallManager          },
        { "Impl_PIM_QueryInterfaceIConfiguration",              Test_Impl_PIM_QueryInterfaceIConfiguration              },
        { "Impl_PIM_QueryInterfaceUnknownID",                   Test_Impl_PIM_QueryInterfaceUnknownID                   },
        { "Impl_PIM_MultipleNotificationsRegisterUnregister",   Test_Impl_PIM_MultipleNotificationsRegisterUnregister   },
        { "Impl_PIM_StartPreinstallWithoutServiceFails",        Test_Impl_PIM_StartPreinstallWithoutServiceFails        },
        { "Impl_PIM_StartPreinstallWithNoInstallerFails",       Test_Impl_PIM_StartPreinstallWithNoInstallerFails       },

        // ── Component tests ──────────────────────────────────────────────────
        { "Comp_PIM_StartPreinstallInvalidDirectoryFails",                  Test_Comp_PIM_StartPreinstallInvalidDirectoryFails                  },
        { "Comp_PIM_StartPreinstallEmptyDirSendsCompletionEvent",           Test_Comp_PIM_StartPreinstallEmptyDirSendsCompletionEvent           },
        { "Comp_PIM_StartPreinstallAlreadyInProgressReturnsError",          Test_Comp_PIM_StartPreinstallAlreadyInProgressReturnsError          },
        { "Comp_PIM_StartPreinstallForceInstallAllPackages",                Test_Comp_PIM_StartPreinstallForceInstallAllPackages                },
        { "Comp_PIM_StartPreinstallFiltersOlderVersionWhenNotForce",        Test_Comp_PIM_StartPreinstallFiltersOlderVersionWhenNotForce        },
        { "Comp_PIM_StartPreinstallInstallsNewerVersionWhenNotForce",       Test_Comp_PIM_StartPreinstallInstallsNewerVersionWhenNotForce       },
        { "Comp_PIM_StartPreinstallEqualVersionFilteredSendsCompletionEvent", Test_Comp_PIM_StartPreinstallEqualVersionFilteredSendsCompletionEvent },
        { "Comp_PIM_InstallPackagesWithInvalidFieldsSkipsPackage",          Test_Comp_PIM_InstallPackagesWithInvalidFieldsSkipsPackage          },
        { "Comp_PIM_InstallPackagesHandlesInstallFailure",                  Test_Comp_PIM_InstallPackagesHandlesInstallFailure                  },
        { "Comp_PIM_GetFailReasonAllEnumsWithoutCrash",                     Test_Comp_PIM_GetFailReasonAllEnumsWithoutCrash                     },
        { "Comp_PIM_DispatchFiresPreinstallCompleteOnAllObservers",         Test_Comp_PIM_DispatchFiresPreinstallCompleteOnAllObservers         },
        { "Comp_PIM_StartPreinstallJoinsPreviousCompletedThread",           Test_Comp_PIM_StartPreinstallJoinsPreviousCompletedThread           },
        { "Comp_PIM_InstallPackagesCreateManagerFails",                     Test_Comp_PIM_InstallPackagesCreateManagerFails                     },
        { "Comp_PIM_ReadPreinstallDirectorySkipsDotEntries",                Test_Comp_PIM_ReadPreinstallDirectorySkipsDotEntries                },
        { "Comp_PIM_StartPreinstallVersionWithSuffixStripped",              Test_Comp_PIM_StartPreinstallVersionWithSuffixStripped              },
        { "Comp_PIM_InvalidVersionDoesNotCrash",                            Test_Comp_PIM_InvalidVersionDoesNotCrash                            },
        { "Comp_PIM_SendOnPreinstallationCompleteEventEnqueuesJob",         Test_Comp_PIM_SendOnPreinstallationCompleteEventEnqueuesJob         },
        { "Comp_PIM_ReadPreinstallDirectoryLoadsValidPackages",             Test_Comp_PIM_ReadPreinstallDirectoryLoadsValidPackages             },
    };

    uint32_t totalFailures = 0;
    uint32_t totalTests    = 0;

    for (const auto& c : cases) {
        const uint32_t failures = c.fn();
        totalTests++;
        if (failures > 0) {
            std::cerr << "[FAIL] " << c.name << " (" << failures << " assertion(s) failed)" << std::endl;
            totalFailures += failures;
        } else {
            std::cout << "[PASS] " << c.name << std::endl;
        }
    }

    L0Test::PrintTotals(std::cout, "PreinstallManager_L0", totalFailures);
    return L0Test::ResultToExitCode(totalFailures);
}
