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

/*
 * PreinstallManagerTest.cpp
 *
 * Main entry point for the PreinstallManager L0 test executable.
 * Registers and runs all PM-PLUGIN, PM-IMPL, PM-NOTIF, PM-EVT,
 * PM-PKG, PM-DIR, PM-VER, and PM-INST test functions.
 */

#include <cstdint>
#include <iostream>

#include "common/L0Bootstrap.hpp"
#include "common/L0TestTypes.hpp"

// ---------------------------------------------------------------------------
// PM-PLUGIN: Outer plugin lifecycle tests (PreinstallManager_PluginTests.cpp)
// ---------------------------------------------------------------------------
extern uint32_t Test_Plugin_Initialize_WithValidService_Succeeds();
extern uint32_t Test_Plugin_Initialize_WhenImplCreationFails_ReturnsError();
extern uint32_t Test_Plugin_Initialize_WhenConfigInterfaceMissing_ReturnsError();
extern uint32_t Test_Plugin_Initialize_WhenConfigure_Fails_ReturnsError();
extern uint32_t Test_Plugin_Initialize_And_Deinitialize_CleansUp();
extern uint32_t Test_Plugin_Deinitialize_WithNullImpl_NoCrash();
extern uint32_t Test_Plugin_Information_ReturnsEmptyString();
extern uint32_t Test_Plugin_Initialize_CallsServiceAddRef();
extern uint32_t Test_Plugin_Initialize_RegistersRpcNotification();
extern uint32_t Test_Plugin_Deactivated_MatchingId_SubmitsJob();
extern uint32_t Test_Plugin_Deactivated_NonMatchingId_DoesNotDeactivate();
extern uint32_t Test_Plugin_Initialize_RegistersJsonRpcDispatcher();

// ---------------------------------------------------------------------------
// PM-IMPL: Implementation constructor / singleton / Configure tests
// PM-NOTIF: Register / Unregister notification tests
// (PreinstallManagerImpl_CoreTests.cpp)
// ---------------------------------------------------------------------------
extern uint32_t Test_Impl_Constructor_SetsInstanceSingleton();
extern uint32_t Test_Impl_Constructor_SecondInstance_DoesNotOverwriteSingleton();
extern uint32_t Test_Impl_GetInstance_ReturnsInstance();
extern uint32_t Test_Impl_GetInstance_AfterDestruction_ReturnsNullptr();
extern uint32_t Test_Impl_Destructor_ResetsInstanceToNullptr();
extern uint32_t Test_Impl_Destructor_ReleasesMCurrentservice();
extern uint32_t Test_Impl_Destructor_ReleasesPackageManager();
extern uint32_t Test_Impl_Configure_WithValidService_Succeeds();
extern uint32_t Test_Impl_Configure_WithEmptyDirectory_ReturnsOk();
extern uint32_t Test_Impl_Configure_WithNullService_ReturnsError();
extern uint32_t Test_Impl_Configure_CallsAddRefOnService();
extern uint32_t Test_Notif_Register_First_ReturnsOk();
extern uint32_t Test_Notif_Register_SameNotif_Twice_IsIdempotent();
extern uint32_t Test_Notif_Register_CallsAddRef();
extern uint32_t Test_Notif_Unregister_Known_ReturnsOk();
extern uint32_t Test_Notif_Unregister_CallsRelease();
extern uint32_t Test_Notif_Unregister_Unknown_ReturnsError();
extern uint32_t Test_Notif_MultipleNotifications_AllReceiveEvent();

// ---------------------------------------------------------------------------
// PM-EVT: Event dispatch tests (PreinstallManagerImpl_EventTests.cpp)
// ---------------------------------------------------------------------------
extern uint32_t Test_Evt_HandleOnAppInstallationStatus_NonEmpty_Dispatches();
extern uint32_t Test_Evt_HandleOnAppInstallationStatus_EmptyString_NoDispatch();
extern uint32_t Test_Evt_MultipleNotifications_AllDispatched();
extern uint32_t Test_Evt_Dispatch_MissingJsonresponse_NoNotification();
extern uint32_t Test_Evt_HandleOnAppInstallationStatus_TwoCalls_TwoEvents();

// ---------------------------------------------------------------------------
// PM-PKG: createPackageManagerObject / releasePackageManagerObject tests
// PM-DIR: readPreinstallDirectory tests
// PM-VER: isNewerVersion tests (via StartPreinstall forceInstall=false)
// PM-INST: StartPreinstall orchestration tests
// (PreinstallManagerImpl_InstallAndVersionTests.cpp)
// ---------------------------------------------------------------------------
extern uint32_t Test_Pkg_CreatePackageManager_NullService_ReturnsError();
extern uint32_t Test_Pkg_CreatePackageManager_QiByCallsignReturnsNull_ReturnsError();
extern uint32_t Test_Pkg_CreatePackageManager_Success_RegistersCalled();
extern uint32_t Test_Pkg_ReleasePackageManager_UnregistersAndReleases();

extern uint32_t Test_Dir_Empty_Directory_StartPreinstall_Succeeds();
extern uint32_t Test_Dir_NonExistent_Directory_ReturnsError();
extern uint32_t Test_Dir_SkipsDotAndDotDot_Entries();
extern uint32_t Test_Dir_ValidPackage_Install_Called();
extern uint32_t Test_Dir_GetConfigFails_InstallSkipped();
extern uint32_t Test_Dir_MixedEntries_OnlyValidInstalled();

extern uint32_t Test_Ver_V1_Major_Greater_Than_V2();
extern uint32_t Test_Ver_V1_Major_Less_Than_V2();
extern uint32_t Test_Ver_Equal_Versions_NotNewer();
extern uint32_t Test_Ver_Minor_Greater();
extern uint32_t Test_Ver_Patch_Greater();
extern uint32_t Test_Ver_Build_Greater();
extern uint32_t Test_Ver_PreRelease_Suffix_Stripped();
extern uint32_t Test_Ver_BuildMetadata_Stripped();
extern uint32_t Test_Ver_Malformed_V1_NotNewer();
extern uint32_t Test_Ver_Malformed_V2_NotNewer();

extern uint32_t Test_Inst_StartPreinstall_CreatesPackageManagerObjectFirst();
extern uint32_t Test_Inst_StartPreinstall_CreateFails_ReturnsError();
extern uint32_t Test_Inst_StartPreinstall_ReadDirectoryFails_ReturnsError();
extern uint32_t Test_Inst_StartPreinstall_ForceInstall_SkipsListPackages();
extern uint32_t Test_Inst_StartPreinstall_ForceInstallFalse_ListFails_ReturnsError();
extern uint32_t Test_Inst_StartPreinstall_SameVersionAlreadyInstalled_Skipped();
extern uint32_t Test_Inst_StartPreinstall_NewerVersionAvailable_InstallCalled();
extern uint32_t Test_Inst_StartPreinstall_EmptyPackageId_Skipped();
extern uint32_t Test_Inst_StartPreinstall_EmptyVersion_Skipped();
extern uint32_t Test_Inst_StartPreinstall_EmptyFileLocator_Skipped();
extern uint32_t Test_Inst_StartPreinstall_InstallFails_ReturnsError();
extern uint32_t Test_Inst_StartPreinstall_AllSucceed_ReturnsOk();
extern uint32_t Test_Inst_StartPreinstall_ReleasesPackageManagerAtEnd();
extern uint32_t Test_Inst_GetFailReason_AllVariants_LeadToInstallError();

int main()
{
    L0Test::L0BootstrapGuard bootstrap;

    uint32_t failures = 0;

    // --- PM-PLUGIN ---
    failures += Test_Plugin_Initialize_WithValidService_Succeeds();
    failures += Test_Plugin_Initialize_WhenImplCreationFails_ReturnsError();
    failures += Test_Plugin_Initialize_WhenConfigInterfaceMissing_ReturnsError();
    failures += Test_Plugin_Initialize_WhenConfigure_Fails_ReturnsError();
    failures += Test_Plugin_Initialize_And_Deinitialize_CleansUp();
    failures += Test_Plugin_Deinitialize_WithNullImpl_NoCrash();
    failures += Test_Plugin_Information_ReturnsEmptyString();
    failures += Test_Plugin_Initialize_CallsServiceAddRef();
    failures += Test_Plugin_Initialize_RegistersRpcNotification();
    failures += Test_Plugin_Deactivated_MatchingId_SubmitsJob();
    failures += Test_Plugin_Deactivated_NonMatchingId_DoesNotDeactivate();
    failures += Test_Plugin_Initialize_RegistersJsonRpcDispatcher();

    // --- PM-IMPL / PM-NOTIF ---
    failures += Test_Impl_Constructor_SetsInstanceSingleton();
    failures += Test_Impl_Constructor_SecondInstance_DoesNotOverwriteSingleton();
    failures += Test_Impl_GetInstance_ReturnsInstance();
    failures += Test_Impl_GetInstance_AfterDestruction_ReturnsNullptr();
    failures += Test_Impl_Destructor_ResetsInstanceToNullptr();
    failures += Test_Impl_Destructor_ReleasesMCurrentservice();
    failures += Test_Impl_Destructor_ReleasesPackageManager();
    failures += Test_Impl_Configure_WithValidService_Succeeds();
    failures += Test_Impl_Configure_WithEmptyDirectory_ReturnsOk();
    failures += Test_Impl_Configure_WithNullService_ReturnsError();
    failures += Test_Impl_Configure_CallsAddRefOnService();
    failures += Test_Notif_Register_First_ReturnsOk();
    failures += Test_Notif_Register_SameNotif_Twice_IsIdempotent();
    failures += Test_Notif_Register_CallsAddRef();
    failures += Test_Notif_Unregister_Known_ReturnsOk();
    failures += Test_Notif_Unregister_CallsRelease();
    failures += Test_Notif_Unregister_Unknown_ReturnsError();
    failures += Test_Notif_MultipleNotifications_AllReceiveEvent();

    // --- PM-EVT ---
    failures += Test_Evt_HandleOnAppInstallationStatus_NonEmpty_Dispatches();
    failures += Test_Evt_HandleOnAppInstallationStatus_EmptyString_NoDispatch();
    failures += Test_Evt_MultipleNotifications_AllDispatched();
    failures += Test_Evt_Dispatch_MissingJsonresponse_NoNotification();
    failures += Test_Evt_HandleOnAppInstallationStatus_TwoCalls_TwoEvents();

    // --- PM-PKG ---
    failures += Test_Pkg_CreatePackageManager_NullService_ReturnsError();
    failures += Test_Pkg_CreatePackageManager_QiByCallsignReturnsNull_ReturnsError();
    failures += Test_Pkg_CreatePackageManager_Success_RegistersCalled();
    failures += Test_Pkg_ReleasePackageManager_UnregistersAndReleases();

    // --- PM-DIR ---
    failures += Test_Dir_Empty_Directory_StartPreinstall_Succeeds();
    failures += Test_Dir_NonExistent_Directory_ReturnsError();
    failures += Test_Dir_SkipsDotAndDotDot_Entries();
    failures += Test_Dir_ValidPackage_Install_Called();
    failures += Test_Dir_GetConfigFails_InstallSkipped();
    failures += Test_Dir_MixedEntries_OnlyValidInstalled();

    // --- PM-VER ---
    failures += Test_Ver_V1_Major_Greater_Than_V2();
    failures += Test_Ver_V1_Major_Less_Than_V2();
    failures += Test_Ver_Equal_Versions_NotNewer();
    failures += Test_Ver_Minor_Greater();
    failures += Test_Ver_Patch_Greater();
    failures += Test_Ver_Build_Greater();
    failures += Test_Ver_PreRelease_Suffix_Stripped();
    failures += Test_Ver_BuildMetadata_Stripped();
    failures += Test_Ver_Malformed_V1_NotNewer();
    failures += Test_Ver_Malformed_V2_NotNewer();

    // --- PM-INST ---
    failures += Test_Inst_StartPreinstall_CreatesPackageManagerObjectFirst();
    failures += Test_Inst_StartPreinstall_CreateFails_ReturnsError();
    failures += Test_Inst_StartPreinstall_ReadDirectoryFails_ReturnsError();
    failures += Test_Inst_StartPreinstall_ForceInstall_SkipsListPackages();
    failures += Test_Inst_StartPreinstall_ForceInstallFalse_ListFails_ReturnsError();
    failures += Test_Inst_StartPreinstall_SameVersionAlreadyInstalled_Skipped();
    failures += Test_Inst_StartPreinstall_NewerVersionAvailable_InstallCalled();
    failures += Test_Inst_StartPreinstall_EmptyPackageId_Skipped();
    failures += Test_Inst_StartPreinstall_EmptyVersion_Skipped();
    failures += Test_Inst_StartPreinstall_EmptyFileLocator_Skipped();
    failures += Test_Inst_StartPreinstall_InstallFails_ReturnsError();
    failures += Test_Inst_StartPreinstall_AllSucceed_ReturnsOk();
    failures += Test_Inst_StartPreinstall_ReleasesPackageManagerAtEnd();
    failures += Test_Inst_GetFailReason_AllVariants_LeadToInstallError();

    L0Test::PrintTotals(std::cout, "PreinstallManager l0test", failures);
    return L0Test::ResultToExitCode(failures);
}
