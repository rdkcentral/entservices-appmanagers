#include <cstdint>
#include <iostream>

#include "common/L0Bootstrap.hpp"
#include "common/L0TestTypes.hpp"

extern uint32_t Test_AM_L0_001_Initialize_HappyPath();
extern uint32_t Test_AM_L0_002_Initialize_FailsWhenRootCreationFails();
extern uint32_t Test_AM_L0_003_Initialize_FailsWhenIConfigurationMissing();
extern uint32_t Test_AM_L0_004_Initialize_FailsWhenConfigureReturnsError();
extern uint32_t Test_AM_L0_005_Deinitialize_ReleasesResources();
extern uint32_t Test_AM_L0_006_Deactivated_MatchingConnection_SubmitsJob();
extern uint32_t Test_AM_L0_007_Deactivated_NonMatchingConnection_NoAction();
extern uint32_t Test_AM_L0_008_Register_Notification();
extern uint32_t Test_AM_L0_009_Register_Duplicate_Notification();
extern uint32_t Test_AM_L0_010_Unregister_Existing_Notification();
extern uint32_t Test_AM_L0_011_Unregister_Unknown_Notification();
extern uint32_t Test_AM_L0_012_LaunchApp_EmptyAppId_ReturnsError();
extern uint32_t Test_AM_L0_013_LaunchApp_NotInstalled_ReturnsError();
extern uint32_t Test_AM_L0_014_LaunchApp_SuccessPath_Delegates();
extern uint32_t Test_AM_L0_015_LaunchApp_PackageLockFailure();
extern uint32_t Test_AM_L0_016_PreloadApp_SuccessPath_RuntimeConfig();
extern uint32_t Test_AM_L0_017_PreloadApp_EmptyAppId_ReturnsError();
extern uint32_t Test_AM_L0_018_CloseApp_Delegates_Succeeds();
extern uint32_t Test_AM_L0_019_CloseApp_PropagatesLifecycleError();
extern uint32_t Test_AM_L0_020_TerminateApp_Delegates_Succeeds();
extern uint32_t Test_AM_L0_021_KillApp_Delegates_Succeeds();
extern uint32_t Test_AM_L0_022_GetLoadedApps_ReturnsIterator();
extern uint32_t Test_AM_L0_023_SendIntent_SuccessPath();
extern uint32_t Test_AM_L0_024_GetInstalledApps_ReturnsJsonList();
extern uint32_t Test_AM_L0_025_GetInstalledApps_EmptyPackageList_Boundary();
extern uint32_t Test_AM_L0_026_IsInstalled_TruePath();
extern uint32_t Test_AM_L0_027_IsInstalled_FalsePath();
extern uint32_t Test_AM_L0_028_GetAppProperty_ReturnsStoredValue();
extern uint32_t Test_AM_L0_029_GetAppProperty_InvalidKey();
extern uint32_t Test_AM_L0_030_SetAppProperty_StoresValue();
extern uint32_t Test_AM_L0_031_ClearAppData_DelegatesToStorageManager();
extern uint32_t Test_AM_L0_032_ClearAllAppData_DelegatesToStorageManager();
extern uint32_t Test_AM_L0_033_GetAppMetadata_PassThroughBehavior();
extern uint32_t Test_AM_L0_034_MaxLimits_Getters_ReturnConfiguredDefaults();
extern uint32_t Test_AM_L0_035_AppLifecycleChange_UpdatesAndDispatches();
extern uint32_t Test_AM_L0_036_AppUnloaded_RemovesAndDispatches();
extern uint32_t Test_AM_L0_037_AppLaunchRequest_DispatchesEvent();
extern uint32_t Test_AM_L0_038_Configure_CreatesDependencies();
extern uint32_t Test_AM_L0_039_Configure_FailsWhenDependencyCreationFails();
extern uint32_t Test_AM_L0_040_AppInfo_DefaultObjectValues();
extern uint32_t Test_AM_L0_041_AppInfo_GetterSetter_Consistency();
extern uint32_t Test_AM_L0_042_AppInfoManager_Upsert_Create_Update();
extern uint32_t Test_AM_L0_043_AppInfoManager_Update_AbsentKey_Fails();
extern uint32_t Test_AM_L0_044_AppInfoManager_Remove_Clear_Behavior();
extern uint32_t Test_AM_L0_045_AppInfoManager_Defaults_UnknownApp();
extern uint32_t Test_AM_L0_046_LifecycleConnector_CreateRelease_RemoteObjects();
extern uint32_t Test_AM_L0_047_LifecycleConnector_Launch_SuccessPath();
extern uint32_t Test_AM_L0_048_LifecycleConnector_Launch_FailsWhenUnavailable();
extern uint32_t Test_AM_L0_049_LifecycleConnector_ActionDelegation();
extern uint32_t Test_AM_L0_050_LifecycleConnector_Mapping_Boundary();
extern uint32_t Test_AM_L0_051_Telemetry_Singleton_Initialize();
extern uint32_t Test_AM_L0_052_Telemetry_RecordLaunchMetric_WhenEnabled();
extern uint32_t Test_AM_L0_053_Telemetry_Disabled_GuardPath();
extern uint32_t Test_AM_L0_054_Telemetry_CrashPayload_Report();
extern uint32_t Test_AM_L0_055_AppManagerTypes_Boundary_ValueMapping();

int main()
{
    L0Test::L0BootstrapGuard bootstrap;

    struct Case {
        const char* name;
        uint32_t (*fn)();
    };

    const Case cases[] = {
        { "AM_L0_001", Test_AM_L0_001_Initialize_HappyPath },
        { "AM_L0_002", Test_AM_L0_002_Initialize_FailsWhenRootCreationFails },
        { "AM_L0_003", Test_AM_L0_003_Initialize_FailsWhenIConfigurationMissing },
        { "AM_L0_004", Test_AM_L0_004_Initialize_FailsWhenConfigureReturnsError },
        { "AM_L0_005", Test_AM_L0_005_Deinitialize_ReleasesResources },
        { "AM_L0_006", Test_AM_L0_006_Deactivated_MatchingConnection_SubmitsJob },
        { "AM_L0_007", Test_AM_L0_007_Deactivated_NonMatchingConnection_NoAction },
        { "AM_L0_008", Test_AM_L0_008_Register_Notification },
        { "AM_L0_009", Test_AM_L0_009_Register_Duplicate_Notification },
        { "AM_L0_010", Test_AM_L0_010_Unregister_Existing_Notification },
        { "AM_L0_011", Test_AM_L0_011_Unregister_Unknown_Notification },
        { "AM_L0_012", Test_AM_L0_012_LaunchApp_EmptyAppId_ReturnsError },
        { "AM_L0_013", Test_AM_L0_013_LaunchApp_NotInstalled_ReturnsError },
        { "AM_L0_014", Test_AM_L0_014_LaunchApp_SuccessPath_Delegates },
        { "AM_L0_015", Test_AM_L0_015_LaunchApp_PackageLockFailure },
        { "AM_L0_016", Test_AM_L0_016_PreloadApp_SuccessPath_RuntimeConfig },
        { "AM_L0_017", Test_AM_L0_017_PreloadApp_EmptyAppId_ReturnsError },
        { "AM_L0_018", Test_AM_L0_018_CloseApp_Delegates_Succeeds },
        { "AM_L0_019", Test_AM_L0_019_CloseApp_PropagatesLifecycleError },
        { "AM_L0_020", Test_AM_L0_020_TerminateApp_Delegates_Succeeds },
        { "AM_L0_021", Test_AM_L0_021_KillApp_Delegates_Succeeds },
        { "AM_L0_022", Test_AM_L0_022_GetLoadedApps_ReturnsIterator },
        { "AM_L0_023", Test_AM_L0_023_SendIntent_SuccessPath },
        { "AM_L0_024", Test_AM_L0_024_GetInstalledApps_ReturnsJsonList },
        { "AM_L0_025", Test_AM_L0_025_GetInstalledApps_EmptyPackageList_Boundary },
        { "AM_L0_026", Test_AM_L0_026_IsInstalled_TruePath },
        { "AM_L0_027", Test_AM_L0_027_IsInstalled_FalsePath },
        { "AM_L0_028", Test_AM_L0_028_GetAppProperty_ReturnsStoredValue },
        { "AM_L0_029", Test_AM_L0_029_GetAppProperty_InvalidKey },
        { "AM_L0_030", Test_AM_L0_030_SetAppProperty_StoresValue },
        { "AM_L0_031", Test_AM_L0_031_ClearAppData_DelegatesToStorageManager },
        { "AM_L0_032", Test_AM_L0_032_ClearAllAppData_DelegatesToStorageManager },
        { "AM_L0_033", Test_AM_L0_033_GetAppMetadata_PassThroughBehavior },
        { "AM_L0_034", Test_AM_L0_034_MaxLimits_Getters_ReturnConfiguredDefaults },
        { "AM_L0_035", Test_AM_L0_035_AppLifecycleChange_UpdatesAndDispatches },
        { "AM_L0_036", Test_AM_L0_036_AppUnloaded_RemovesAndDispatches },
        { "AM_L0_037", Test_AM_L0_037_AppLaunchRequest_DispatchesEvent },
        { "AM_L0_038", Test_AM_L0_038_Configure_CreatesDependencies },
        { "AM_L0_039", Test_AM_L0_039_Configure_FailsWhenDependencyCreationFails },
        { "AM_L0_040", Test_AM_L0_040_AppInfo_DefaultObjectValues },
        { "AM_L0_041", Test_AM_L0_041_AppInfo_GetterSetter_Consistency },
        { "AM_L0_042", Test_AM_L0_042_AppInfoManager_Upsert_Create_Update },
        { "AM_L0_043", Test_AM_L0_043_AppInfoManager_Update_AbsentKey_Fails },
        { "AM_L0_044", Test_AM_L0_044_AppInfoManager_Remove_Clear_Behavior },
        { "AM_L0_045", Test_AM_L0_045_AppInfoManager_Defaults_UnknownApp },
        { "AM_L0_046", Test_AM_L0_046_LifecycleConnector_CreateRelease_RemoteObjects },
        { "AM_L0_047", Test_AM_L0_047_LifecycleConnector_Launch_SuccessPath },
        { "AM_L0_048", Test_AM_L0_048_LifecycleConnector_Launch_FailsWhenUnavailable },
        { "AM_L0_049", Test_AM_L0_049_LifecycleConnector_ActionDelegation },
        { "AM_L0_050", Test_AM_L0_050_LifecycleConnector_Mapping_Boundary },
        { "AM_L0_051", Test_AM_L0_051_Telemetry_Singleton_Initialize },
        { "AM_L0_052", Test_AM_L0_052_Telemetry_RecordLaunchMetric_WhenEnabled },
        { "AM_L0_053", Test_AM_L0_053_Telemetry_Disabled_GuardPath },
        { "AM_L0_054", Test_AM_L0_054_Telemetry_CrashPayload_Report },
        { "AM_L0_055", Test_AM_L0_055_AppManagerTypes_Boundary_ValueMapping },
    };

    uint32_t failures = 0;
    for (const auto& c : cases) {
        std::cerr << "[ RUN      ] " << c.name << std::endl;
        const uint32_t f = c.fn();
        if (0 == f) {
            std::cerr << "[       OK ] " << c.name << std::endl;
        } else {
            std::cerr << "[  FAILED  ] " << c.name << " failures=" << f << std::endl;
        }
        failures += f;
    }

    L0Test::PrintTotals(std::cout, "AppManager l0test", failures);
    return L0Test::ResultToExitCode(failures);
}
