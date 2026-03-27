/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
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
 */

/**
 * @file PreinstallManagerTest.cpp
 *
 * Entry point for the PreinstallManager L0 test binary.
 *
 * Registers all test functions from:
 *   - PreinstallManager_LifecycleTests.cpp   (L0-LC-001 to L0-LC-014)
 *   - PreinstallManager_ImplementationTests.cpp (L0-IMPL-001 to L0-IMPL-080)
 *   - PreinstallManager_ComponentTests.cpp   (L0-IMPL-020 to L0-IMPL-034)
 */

#include <cstdint>
#include <iostream>

#include "common/L0Bootstrap.hpp"
#include "common/L0TestTypes.hpp"

// ──────────────────────────────────────────────────────────────────────────────
// Lifecycle test declarations  (PreinstallManager_LifecycleTests.cpp)
// ──────────────────────────────────────────────────────────────────────────────
extern uint32_t Test_PreinstallManager_ConstructorSetsSingletonInstance();
extern uint32_t Test_PreinstallManager_DestructorClearsSingleton();
extern uint32_t Test_PreinstallManager_InitializeSucceedsAndDeinitializeCleansUp();
extern uint32_t Test_PreinstallManager_InitializeFailsWhenRootCreationFails();
extern uint32_t Test_PreinstallManager_InitializeFailsWhenConfigurationInterfaceMissing();
extern uint32_t Test_PreinstallManager_InitializeFailsWhenConfigureFails();
extern uint32_t Test_PreinstallManager_InitializeWithNoInstantiateHandler();
extern uint32_t Test_PreinstallManager_DeinitializeReleasesAllResources();
extern uint32_t Test_PreinstallManager_DeinitializeSafeWhenImplIsNull();
extern uint32_t Test_PreinstallManager_InformationReturnsExpectedValue();
extern uint32_t Test_PreinstallManager_DeactivatedWithMismatchedConnectionIdIgnored();
extern uint32_t Test_PreinstallManager_NotificationActivatedDoesNotCrash();
extern uint32_t Test_PreinstallManager_NotificationDeactivatedDelegatesToParent();
extern uint32_t Test_PreinstallManager_NotificationOnAppInstallationStatusDoesNotCrash();

// ──────────────────────────────────────────────────────────────────────────────
// Implementation test declarations  (PreinstallManager_ImplementationTests.cpp)
// ──────────────────────────────────────────────────────────────────────────────
extern uint32_t Test_Impl_ConstructorSetsSingletonInstance();
extern uint32_t Test_Impl_DestructorClearsSingleton();
extern uint32_t Test_Impl_GetInstanceReturnsSelf();
extern uint32_t Test_Impl_GetInstanceBeforeConstruction();
extern uint32_t Test_Impl_RegisterNotification();
extern uint32_t Test_Impl_RegisterDuplicate();
extern uint32_t Test_Impl_UnregisterNotification();
extern uint32_t Test_Impl_UnregisterNotRegistered();
extern uint32_t Test_Impl_ConfigureWithNullService();
extern uint32_t Test_Impl_ConfigureWithEmptyConfigLine();
extern uint32_t Test_Impl_ConfigureParsesAppPreinstallDirectory();
extern uint32_t Test_Impl_CreatePackageManagerObjectNullService();
extern uint32_t Test_Impl_CreatePackageManagerObjectPluginUnavailable();
extern uint32_t Test_Impl_CreatePackageManagerObjectSuccess();
extern uint32_t Test_Impl_ReleasePackageManagerObjectAfterStartPreinstall();
extern uint32_t Test_Impl_HandleOnAppInstallationStatusDispatchesEvent();
extern uint32_t Test_Impl_HandleOnAppInstallationStatusEmptyStringNoDispatch();
extern uint32_t Test_Impl_DispatchEventFiresCallback();
extern uint32_t Test_Impl_DispatchInstallationStatusNotifiesBothListeners();
extern uint32_t Test_Impl_HandleEmptyStringNoCallback();
extern uint32_t Test_Impl_DispatchWithNoListeners();
extern uint32_t Test_Impl_StartPreinstallNoPackageManagerReturnsError();
extern uint32_t Test_Impl_StartPreinstallBadDirectoryReturnsError();
extern uint32_t Test_Impl_StartPreinstallForceInstallAllSucceed();
extern uint32_t Test_Impl_StartPreinstallListPackagesFailsReturnsError();
extern uint32_t Test_Impl_StartPreinstallSkipsIfSameVersionInstalled();
extern uint32_t Test_Impl_StartPreinstallInstallsNewerVersion();
extern uint32_t Test_Impl_StartPreinstallInstallFailureReturnsError();
extern uint32_t Test_Impl_StartPreinstallSkipsPackageWithEmptyFields();
extern uint32_t Test_Impl_StartPreinstallMixedResultsReturnsError();
extern uint32_t Test_Impl_StartPreinstallNullPackageIteratorReturnsError();
extern uint32_t Test_Impl_MultipleNotificationsRegisterUnregister();

// ──────────────────────────────────────────────────────────────────────────────
// Component (white-box) test declarations  (PreinstallManager_ComponentTests.cpp)
// ──────────────────────────────────────────────────────────────────────────────
extern uint32_t Test_Comp_IsNewerVersion_MajorIncrement();
extern uint32_t Test_Comp_IsNewerVersion_OlderMajor();
extern uint32_t Test_Comp_IsNewerVersion_EqualVersions();
extern uint32_t Test_Comp_IsNewerVersion_MinorIncrement();
extern uint32_t Test_Comp_IsNewerVersion_PatchIncrement();
extern uint32_t Test_Comp_IsNewerVersion_SuffixStrippedEqual();
extern uint32_t Test_Comp_IsNewerVersion_FourPartBuildIncrement();
extern uint32_t Test_Comp_IsNewerVersion_InvalidFormatReturnsFalse();
extern uint32_t Test_Comp_GetFailReason_SignatureVerification();
extern uint32_t Test_Comp_GetFailReason_PackageMismatch();
extern uint32_t Test_Comp_GetFailReason_InvalidMetadata();
extern uint32_t Test_Comp_GetFailReason_PersistenceFailure();
extern uint32_t Test_Comp_GetFailReason_None();

// ──────────────────────────────────────────────────────────────────────────────
// main
// ──────────────────────────────────────────────────────────────────────────────
int main()
{
    L0Test::L0BootstrapGuard bootstrap;

    struct Case {
        const char* name;
        uint32_t (*fn)();
    };

    const Case cases[] = {
        // ── Lifecycle: IPlugin layer (L0-LC-001 to L0-LC-014) ────────────────
        { "PreinstallManager_ConstructorSetsSingletonInstance",
              Test_PreinstallManager_ConstructorSetsSingletonInstance },
        { "PreinstallManager_DestructorClearsSingleton",
              Test_PreinstallManager_DestructorClearsSingleton },
        { "PreinstallManager_InitializeSucceedsAndDeinitializeCleansUp",
              Test_PreinstallManager_InitializeSucceedsAndDeinitializeCleansUp },
        { "PreinstallManager_InitializeFailsWhenRootCreationFails",
              Test_PreinstallManager_InitializeFailsWhenRootCreationFails },
        { "PreinstallManager_InitializeFailsWhenConfigurationInterfaceMissing",
              Test_PreinstallManager_InitializeFailsWhenConfigurationInterfaceMissing },
        { "PreinstallManager_InitializeFailsWhenConfigureFails",
              Test_PreinstallManager_InitializeFailsWhenConfigureFails },
        { "PreinstallManager_InitializeWithNoInstantiateHandler",
              Test_PreinstallManager_InitializeWithNoInstantiateHandler },
        { "PreinstallManager_DeinitializeReleasesAllResources",
              Test_PreinstallManager_DeinitializeReleasesAllResources },
        { "PreinstallManager_DeinitializeSafeWhenImplIsNull",
              Test_PreinstallManager_DeinitializeSafeWhenImplIsNull },
        { "PreinstallManager_InformationReturnsExpectedValue",
              Test_PreinstallManager_InformationReturnsExpectedValue },
        { "PreinstallManager_DeactivatedWithMismatchedConnectionIdIgnored",
              Test_PreinstallManager_DeactivatedWithMismatchedConnectionIdIgnored },
        { "PreinstallManager_NotificationActivatedDoesNotCrash",
              Test_PreinstallManager_NotificationActivatedDoesNotCrash },
        { "PreinstallManager_NotificationDeactivatedDelegatesToParent",
              Test_PreinstallManager_NotificationDeactivatedDelegatesToParent },
        { "PreinstallManager_NotificationOnAppInstallationStatusDoesNotCrash",
              Test_PreinstallManager_NotificationOnAppInstallationStatusDoesNotCrash },

        // ── Implementation: construction & singleton (L0-IMPL-001 to -004) ──
        { "Impl_ConstructorSetsSingletonInstance",
              Test_Impl_ConstructorSetsSingletonInstance },
        { "Impl_DestructorClearsSingleton",
              Test_Impl_DestructorClearsSingleton },
        { "Impl_GetInstanceReturnsSelf",
              Test_Impl_GetInstanceReturnsSelf },
        { "Impl_GetInstanceBeforeConstruction",
              Test_Impl_GetInstanceBeforeConstruction },

        // ── Implementation: Register / Unregister (L0-IMPL-005 to -008) ─────
        { "Impl_RegisterNotification",
              Test_Impl_RegisterNotification },
        { "Impl_RegisterDuplicate",
              Test_Impl_RegisterDuplicate },
        { "Impl_UnregisterNotification",
              Test_Impl_UnregisterNotification },
        { "Impl_UnregisterNotRegistered",
              Test_Impl_UnregisterNotRegistered },

        // ── Implementation: Configure() (L0-IMPL-009 to -011) ───────────────
        { "Impl_ConfigureWithNullService",
              Test_Impl_ConfigureWithNullService },
        { "Impl_ConfigureWithEmptyConfigLine",
              Test_Impl_ConfigureWithEmptyConfigLine },
        { "Impl_ConfigureParsesAppPreinstallDirectory",
              Test_Impl_ConfigureParsesAppPreinstallDirectory },

        // ── Implementation: PM object lifecycle (L0-IMPL-040 to -043) ────────
        { "Impl_CreatePackageManagerObjectNullService",
              Test_Impl_CreatePackageManagerObjectNullService },
        { "Impl_CreatePackageManagerObjectPluginUnavailable",
              Test_Impl_CreatePackageManagerObjectPluginUnavailable },
        { "Impl_CreatePackageManagerObjectSuccess",
              Test_Impl_CreatePackageManagerObjectSuccess },
        { "Impl_ReleasePackageManagerObjectAfterStartPreinstall",
              Test_Impl_ReleasePackageManagerObjectAfterStartPreinstall },

        // ── Implementation: event dispatch (L0-IMPL-050 to -055) ─────────────
        { "Impl_HandleOnAppInstallationStatusDispatchesEvent",
              Test_Impl_HandleOnAppInstallationStatusDispatchesEvent },
        { "Impl_HandleOnAppInstallationStatusEmptyStringNoDispatch",
              Test_Impl_HandleOnAppInstallationStatusEmptyStringNoDispatch },
        { "Impl_DispatchEventFiresCallback",
              Test_Impl_DispatchEventFiresCallback },
        { "Impl_DispatchInstallationStatusNotifiesBothListeners",
              Test_Impl_DispatchInstallationStatusNotifiesBothListeners },
        { "Impl_HandleEmptyStringNoCallback",
              Test_Impl_HandleEmptyStringNoCallback },
        { "Impl_DispatchWithNoListeners",
              Test_Impl_DispatchWithNoListeners },

        // ── Implementation: StartPreinstall() (L0-IMPL-070 to -080) ──────────
        { "Impl_StartPreinstallNoPackageManagerReturnsError",
              Test_Impl_StartPreinstallNoPackageManagerReturnsError },
        { "Impl_StartPreinstallBadDirectoryReturnsError",
              Test_Impl_StartPreinstallBadDirectoryReturnsError },
        { "Impl_StartPreinstallForceInstallAllSucceed",
              Test_Impl_StartPreinstallForceInstallAllSucceed },
        { "Impl_StartPreinstallListPackagesFailsReturnsError",
              Test_Impl_StartPreinstallListPackagesFailsReturnsError },
        { "Impl_StartPreinstallSkipsIfSameVersionInstalled",
              Test_Impl_StartPreinstallSkipsIfSameVersionInstalled },
        { "Impl_StartPreinstallInstallsNewerVersion",
              Test_Impl_StartPreinstallInstallsNewerVersion },
        { "Impl_StartPreinstallInstallFailureReturnsError",
              Test_Impl_StartPreinstallInstallFailureReturnsError },
        { "Impl_StartPreinstallSkipsPackageWithEmptyFields",
              Test_Impl_StartPreinstallSkipsPackageWithEmptyFields },
        { "Impl_StartPreinstallMixedResultsReturnsError",
              Test_Impl_StartPreinstallMixedResultsReturnsError },
        { "Impl_StartPreinstallNullPackageIteratorReturnsError",
              Test_Impl_StartPreinstallNullPackageIteratorReturnsError },
        { "Impl_MultipleNotificationsRegisterUnregister",
              Test_Impl_MultipleNotificationsRegisterUnregister },

        // ── Component: isNewerVersion() (L0-IMPL-020 to -027) ───────────────
        { "Comp_IsNewerVersion_MajorIncrement",
              Test_Comp_IsNewerVersion_MajorIncrement },
        { "Comp_IsNewerVersion_OlderMajor",
              Test_Comp_IsNewerVersion_OlderMajor },
        { "Comp_IsNewerVersion_EqualVersions",
              Test_Comp_IsNewerVersion_EqualVersions },
        { "Comp_IsNewerVersion_MinorIncrement",
              Test_Comp_IsNewerVersion_MinorIncrement },
        { "Comp_IsNewerVersion_PatchIncrement",
              Test_Comp_IsNewerVersion_PatchIncrement },
        { "Comp_IsNewerVersion_SuffixStrippedEqual",
              Test_Comp_IsNewerVersion_SuffixStrippedEqual },
        { "Comp_IsNewerVersion_FourPartBuildIncrement",
              Test_Comp_IsNewerVersion_FourPartBuildIncrement },
        { "Comp_IsNewerVersion_InvalidFormatReturnsFalse",
              Test_Comp_IsNewerVersion_InvalidFormatReturnsFalse },

        // ── Component: getFailReason() (L0-IMPL-030 to -034) ────────────────
        { "Comp_GetFailReason_SignatureVerification",
              Test_Comp_GetFailReason_SignatureVerification },
        { "Comp_GetFailReason_PackageMismatch",
              Test_Comp_GetFailReason_PackageMismatch },
        { "Comp_GetFailReason_InvalidMetadata",
              Test_Comp_GetFailReason_InvalidMetadata },
        { "Comp_GetFailReason_PersistenceFailure",
              Test_Comp_GetFailReason_PersistenceFailure },
        { "Comp_GetFailReason_None",
              Test_Comp_GetFailReason_None },
    };

    uint32_t failures = 0;
    const size_t totalCases = sizeof(cases) / sizeof(cases[0]);

    for (size_t i = 0; i < totalCases; ++i) {
        const uint32_t f = cases[i].fn();
        if (f > 0) {
            std::cerr << "FAILED [" << (i + 1) << "/" << totalCases << "]: "
                      << cases[i].name
                      << " (" << f << " failure(s))\n";
        } else {
            std::cout << "  PASSED [" << (i + 1) << "/" << totalCases << "]: "
                      << cases[i].name << "\n";
        }
        failures += f;
    }

    L0Test::PrintTotals(std::cout, "PreinstallManager_L0Test", failures);
    return L0Test::ResultToExitCode(failures);
}
