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
 * L0 test runner for the PreinstallManager plugin.
 * Registers all test cases from the lifecycle and implementation test files,
 * drives them in sequence, and returns the total failure count as the process
 * exit code (0 = all passed).
 */

#include <cstdint>
#include <iostream>

#include "common/L0Bootstrap.hpp"
#include "common/L0TestTypes.hpp"

// ── PreinstallManager_LifecycleTests.cpp ─────────────────────────────────────
extern uint32_t Test_PM_ServiceName_ConstantValue();
extern uint32_t Test_PM_Constructor_SetsInstancePointer();
extern uint32_t Test_PM_Destructor_ClearsInstancePointer();
extern uint32_t Test_Initialize_WithValidService_Succeeds();
extern uint32_t Test_Initialize_WhenImplCreationFails_ReturnsError();
extern uint32_t Test_Initialize_WhenConfigureInterfaceMissing_ReturnsError();
extern uint32_t Test_Initialize_WhenConfigureFails_ReturnsError();
extern uint32_t Test_Initialize_Twice_Idempotent();
extern uint32_t Test_Deinitialize_ReleasesAllResources();
extern uint32_t Test_Information_ReturnsEmptyString();
extern uint32_t Test_Notification_Activated_DoesNotCrash();
extern uint32_t Test_Notification_Deactivated_TriggersParentDeactivated();

// ── PreinstallManagerImpl_Tests.cpp ──────────────────────────────────────────
extern uint32_t Test_Impl_Configure_WithNullService_ReturnsError();
extern uint32_t Test_Impl_Configure_WithValidService_ReturnsNone();
extern uint32_t Test_Impl_Configure_WithAppPreinstallDirectoryInConfig();
extern uint32_t Test_Impl_Configure_WithEmptyPreinstallDirectory();
extern uint32_t Test_Impl_Register_ValidNotification_ReturnsNone();
extern uint32_t Test_Impl_Register_DuplicateNotification_NotAddedTwice();
extern uint32_t Test_Impl_Unregister_RegisteredNotification_ReturnsNone();
extern uint32_t Test_Impl_Unregister_NotRegistered_ReturnsGeneral();
extern uint32_t Test_Impl_Unregister_AfterDuplicate_CorrectRefCount();
extern uint32_t Test_Impl_GetInstance_ReturnsConstructedInstance();
extern uint32_t Test_Impl_Destructor_ReleasesCurrentService();
extern uint32_t Test_Impl_StartPreinstall_WhenPkgManagerCreationFails_ReturnsError();
extern uint32_t Test_Impl_StartPreinstall_WhenDirectoryReadFails_ReturnsError();
extern uint32_t Test_Impl_StartPreinstall_EmptyDirectory_NoInstallCalls();
extern uint32_t Test_Impl_StartPreinstall_ForceInstall_InstallsAllPackages();
extern uint32_t Test_Impl_StartPreinstall_NoForce_SkipsAlreadyInstalledSameVersion();
extern uint32_t Test_Impl_StartPreinstall_NoForce_InstallsNewerVersion();
extern uint32_t Test_Impl_StartPreinstall_NoForce_SkipsOlderVersion();
extern uint32_t Test_Impl_StartPreinstall_ListPackagesFails_ReturnsError();
extern uint32_t Test_Impl_StartPreinstall_NullPackageList_ReturnsError();
extern uint32_t Test_Impl_StartPreinstall_InstallFails_ReturnsError();
extern uint32_t Test_Impl_StartPreinstall_PackageWithEmptyFields_Skipped();
extern uint32_t Test_Impl_StartPreinstall_ReleasesPackageManagerAfterInstall();
extern uint32_t Test_Impl_StartPreinstall_MultiplePackages_AllInstalled();
extern uint32_t Test_Impl_StartPreinstall_MixedSuccessAndFailure_ReturnsError();
extern uint32_t Test_Impl_IsNewerVersion_NewerMajor_ReturnsTrue();
extern uint32_t Test_Impl_IsNewerVersion_OlderMajor_ReturnsFalse();
extern uint32_t Test_Impl_IsNewerVersion_EqualVersions_ReturnsFalse();
extern uint32_t Test_Impl_IsNewerVersion_NewerMinor_ReturnsTrue();
extern uint32_t Test_Impl_IsNewerVersion_NewerPatch_ReturnsTrue();
extern uint32_t Test_Impl_IsNewerVersion_NewerBuild_ReturnsTrue();
extern uint32_t Test_Impl_IsNewerVersion_WithPreReleaseSeparator();
extern uint32_t Test_Impl_IsNewerVersion_WithBuildMetaSeparator();
extern uint32_t Test_Impl_IsNewerVersion_InvalidFormat_V1_ReturnsFalse();
extern uint32_t Test_Impl_IsNewerVersion_InvalidFormat_V2_ReturnsFalse();
extern uint32_t Test_Impl_IsNewerVersion_TwoPartVersion_ReturnsFalse();
extern uint32_t Test_Impl_GetFailReason_AllEnumValues();
extern uint32_t Test_Impl_GetFailReason_Default_ReturnsNONE();
extern uint32_t Test_Impl_HandleOnAppInstallationStatus_ValidJson_DispatchesFired();
extern uint32_t Test_Impl_HandleOnAppInstallationStatus_EmptyString_LogsError();
extern uint32_t Test_Impl_Dispatch_UnknownEvent_LogsError();
extern uint32_t Test_Impl_Dispatch_MissingJsonresponseKey_LogsError();
extern uint32_t Test_Impl_Dispatch_MultipleNotifications_AllReceiveEvent();
extern uint32_t Test_Impl_CreatePackageManagerObject_WithNullService_ReturnsError();
extern uint32_t Test_Impl_CreatePackageManagerObject_PluginNotRegistered_ReturnsError();
extern uint32_t Test_Impl_CreatePackageManagerObject_ValidPlugin_ReturnsNone();
extern uint32_t Test_Impl_ReleasePackageManagerObject_ClearsPointer();
extern uint32_t Test_Impl_ReadPreinstallDirectory_InvalidDir_ReturnsFalse();
extern uint32_t Test_Impl_ReadPreinstallDirectory_EmptyDir_ReturnsTrue();

int main()
{
    L0Test::L0BootstrapGuard bootstrap;

    struct Case {
        const char* name;
        uint32_t (*fn)();
    };

    const Case cases[] = {
        // ── Plugin wrapper lifecycle ─────────────────────────────────────────
        { "PM_ServiceName_ConstantValue",                               Test_PM_ServiceName_ConstantValue },
        { "PM_Constructor_SetsInstancePointer",                         Test_PM_Constructor_SetsInstancePointer },
        { "PM_Destructor_ClearsInstancePointer",                        Test_PM_Destructor_ClearsInstancePointer },
        { "Initialize_WithValidService_Succeeds",                       Test_Initialize_WithValidService_Succeeds },
        { "Initialize_WhenImplCreationFails_ReturnsError",              Test_Initialize_WhenImplCreationFails_ReturnsError },
        { "Initialize_WhenConfigureInterfaceMissing_ReturnsError",      Test_Initialize_WhenConfigureInterfaceMissing_ReturnsError },
        { "Initialize_WhenConfigureFails_ReturnsError",                 Test_Initialize_WhenConfigureFails_ReturnsError },
        { "Initialize_Twice_Idempotent",                                Test_Initialize_Twice_Idempotent },
        { "Deinitialize_ReleasesAllResources",                          Test_Deinitialize_ReleasesAllResources },
        { "Information_ReturnsEmptyString",                             Test_Information_ReturnsEmptyString },
        { "Notification_Activated_DoesNotCrash",                        Test_Notification_Activated_DoesNotCrash },
        { "Notification_Deactivated_TriggersParentDeactivated",         Test_Notification_Deactivated_TriggersParentDeactivated },

        // ── Implementation: Configure ─────────────────────────────────────────
        { "Impl_Configure_WithNullService_ReturnsError",                Test_Impl_Configure_WithNullService_ReturnsError },
        { "Impl_Configure_WithValidService_ReturnsNone",                Test_Impl_Configure_WithValidService_ReturnsNone },
        { "Impl_Configure_WithAppPreinstallDirectoryInConfig",          Test_Impl_Configure_WithAppPreinstallDirectoryInConfig },
        { "Impl_Configure_WithEmptyPreinstallDirectory",                Test_Impl_Configure_WithEmptyPreinstallDirectory },

        // ── Implementation: Register / Unregister ─────────────────────────────
        { "Impl_Register_ValidNotification_ReturnsNone",                Test_Impl_Register_ValidNotification_ReturnsNone },
        { "Impl_Register_DuplicateNotification_NotAddedTwice",          Test_Impl_Register_DuplicateNotification_NotAddedTwice },
        { "Impl_Unregister_RegisteredNotification_ReturnsNone",         Test_Impl_Unregister_RegisteredNotification_ReturnsNone },
        { "Impl_Unregister_NotRegistered_ReturnsGeneral",               Test_Impl_Unregister_NotRegistered_ReturnsGeneral },
        { "Impl_Unregister_AfterDuplicate_CorrectRefCount",             Test_Impl_Unregister_AfterDuplicate_CorrectRefCount },

        // ── Implementation: Singleton / dtor ──────────────────────────────────
        { "Impl_GetInstance_ReturnsConstructedInstance",                Test_Impl_GetInstance_ReturnsConstructedInstance },
        { "Impl_Destructor_ReleasesCurrentService",                     Test_Impl_Destructor_ReleasesCurrentService },

        // ── Implementation: StartPreinstall ───────────────────────────────────
        { "Impl_StartPreinstall_WhenPkgManagerCreationFails_ReturnsError",  Test_Impl_StartPreinstall_WhenPkgManagerCreationFails_ReturnsError },
        { "Impl_StartPreinstall_WhenDirectoryReadFails_ReturnsError",        Test_Impl_StartPreinstall_WhenDirectoryReadFails_ReturnsError },
        { "Impl_StartPreinstall_EmptyDirectory_NoInstallCalls",              Test_Impl_StartPreinstall_EmptyDirectory_NoInstallCalls },
        { "Impl_StartPreinstall_ForceInstall_InstallsAllPackages",           Test_Impl_StartPreinstall_ForceInstall_InstallsAllPackages },
        { "Impl_StartPreinstall_NoForce_SkipsAlreadyInstalledSameVersion",   Test_Impl_StartPreinstall_NoForce_SkipsAlreadyInstalledSameVersion },
        { "Impl_StartPreinstall_NoForce_InstallsNewerVersion",               Test_Impl_StartPreinstall_NoForce_InstallsNewerVersion },
        { "Impl_StartPreinstall_NoForce_SkipsOlderVersion",                  Test_Impl_StartPreinstall_NoForce_SkipsOlderVersion },
        { "Impl_StartPreinstall_ListPackagesFails_ReturnsError",             Test_Impl_StartPreinstall_ListPackagesFails_ReturnsError },
        { "Impl_StartPreinstall_NullPackageList_ReturnsError",               Test_Impl_StartPreinstall_NullPackageList_ReturnsError },
        { "Impl_StartPreinstall_InstallFails_ReturnsError",                  Test_Impl_StartPreinstall_InstallFails_ReturnsError },
        { "Impl_StartPreinstall_PackageWithEmptyFields_Skipped",             Test_Impl_StartPreinstall_PackageWithEmptyFields_Skipped },
        { "Impl_StartPreinstall_ReleasesPackageManagerAfterInstall",         Test_Impl_StartPreinstall_ReleasesPackageManagerAfterInstall },
        { "Impl_StartPreinstall_MultiplePackages_AllInstalled",              Test_Impl_StartPreinstall_MultiplePackages_AllInstalled },
        { "Impl_StartPreinstall_MixedSuccessAndFailure_ReturnsError",        Test_Impl_StartPreinstall_MixedSuccessAndFailure_ReturnsError },

        // ── Implementation: isNewerVersion (via StartPreinstall) ──────────────
        { "Impl_IsNewerVersion_NewerMajor_ReturnsTrue",                 Test_Impl_IsNewerVersion_NewerMajor_ReturnsTrue },
        { "Impl_IsNewerVersion_OlderMajor_ReturnsFalse",                Test_Impl_IsNewerVersion_OlderMajor_ReturnsFalse },
        { "Impl_IsNewerVersion_EqualVersions_ReturnsFalse",             Test_Impl_IsNewerVersion_EqualVersions_ReturnsFalse },
        { "Impl_IsNewerVersion_NewerMinor_ReturnsTrue",                 Test_Impl_IsNewerVersion_NewerMinor_ReturnsTrue },
        { "Impl_IsNewerVersion_NewerPatch_ReturnsTrue",                 Test_Impl_IsNewerVersion_NewerPatch_ReturnsTrue },
        { "Impl_IsNewerVersion_NewerBuild_ReturnsTrue",                 Test_Impl_IsNewerVersion_NewerBuild_ReturnsTrue },
        { "Impl_IsNewerVersion_WithPreReleaseSeparator",                Test_Impl_IsNewerVersion_WithPreReleaseSeparator },
        { "Impl_IsNewerVersion_WithBuildMetaSeparator",                 Test_Impl_IsNewerVersion_WithBuildMetaSeparator },
        { "Impl_IsNewerVersion_InvalidFormat_V1_ReturnsFalse",          Test_Impl_IsNewerVersion_InvalidFormat_V1_ReturnsFalse },
        { "Impl_IsNewerVersion_InvalidFormat_V2_ReturnsFalse",          Test_Impl_IsNewerVersion_InvalidFormat_V2_ReturnsFalse },
        { "Impl_IsNewerVersion_TwoPartVersion_ReturnsFalse",            Test_Impl_IsNewerVersion_TwoPartVersion_ReturnsFalse },

        // ── Implementation: getFailReason ─────────────────────────────────────
        { "Impl_GetFailReason_AllEnumValues",                           Test_Impl_GetFailReason_AllEnumValues },
        { "Impl_GetFailReason_Default_ReturnsNONE",                     Test_Impl_GetFailReason_Default_ReturnsNONE },

        // ── Implementation: event dispatch ────────────────────────────────────
        { "Impl_HandleOnAppInstallationStatus_ValidJson_DispatchesFired", Test_Impl_HandleOnAppInstallationStatus_ValidJson_DispatchesFired },
        { "Impl_HandleOnAppInstallationStatus_EmptyString_LogsError",     Test_Impl_HandleOnAppInstallationStatus_EmptyString_LogsError },
        { "Impl_Dispatch_UnknownEvent_LogsError",                          Test_Impl_Dispatch_UnknownEvent_LogsError },
        { "Impl_Dispatch_MissingJsonresponseKey_LogsError",                Test_Impl_Dispatch_MissingJsonresponseKey_LogsError },
        { "Impl_Dispatch_MultipleNotifications_AllReceiveEvent",           Test_Impl_Dispatch_MultipleNotifications_AllReceiveEvent },

        // ── Implementation: pkg manager object lifecycle ───────────────────────
        { "Impl_CreatePackageManagerObject_WithNullService_ReturnsError",  Test_Impl_CreatePackageManagerObject_WithNullService_ReturnsError },
        { "Impl_CreatePackageManagerObject_PluginNotRegistered_ReturnsError", Test_Impl_CreatePackageManagerObject_PluginNotRegistered_ReturnsError },
        { "Impl_CreatePackageManagerObject_ValidPlugin_ReturnsNone",       Test_Impl_CreatePackageManagerObject_ValidPlugin_ReturnsNone },
        { "Impl_ReleasePackageManagerObject_ClearsPointer",                Test_Impl_ReleasePackageManagerObject_ClearsPointer },

        // ── Implementation: readPreinstallDirectory ───────────────────────────
        { "Impl_ReadPreinstallDirectory_InvalidDir_ReturnsFalse",          Test_Impl_ReadPreinstallDirectory_InvalidDir_ReturnsFalse },
        { "Impl_ReadPreinstallDirectory_EmptyDir_ReturnsTrue",             Test_Impl_ReadPreinstallDirectory_EmptyDir_ReturnsTrue },
    };

    uint32_t totalFailures = 0;
    uint32_t totalTests    = static_cast<uint32_t>(sizeof(cases) / sizeof(cases[0]));
    uint32_t passed        = 0;

    for (const auto& tc : cases) {
        const uint32_t f = tc.fn();
        if (0U == f) {
            std::cout << "[ PASSED ] " << tc.name << std::endl;
            passed++;
        } else {
            std::cout << "[ FAILED ] " << tc.name << " (" << f << " assertion(s) failed)" << std::endl;
            totalFailures += f;
        }
    }

    std::cout << "\n============================" << std::endl;
    std::cout << "PreinstallManager L0 Tests: " << passed << "/" << totalTests << " passed";
    if (totalFailures > 0) {
        std::cout << "  (" << totalFailures << " failure(s))";
    }
    std::cout << std::endl;

    return L0Test::ResultToExitCode(totalFailures);
}
