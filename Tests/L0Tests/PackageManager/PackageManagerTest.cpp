#include <cstdint>
#include <iostream>

#include "common/L0Bootstrap.hpp"
#include "common/L0TestTypes.hpp"

extern uint32_t Test_PM_Impl_InitializeAndDeinitialize();
extern uint32_t Test_PM_Shell_InitializeFailsWhenRootInstantiationFails();
extern uint32_t Test_PM_Shell_InitializeSuccessAndQueryInterfaces();
extern uint32_t Test_PM_Shell_InformationReturnsEmptyString();
extern uint32_t Test_PM_Shell_DownloadPathThroughPluginAggregate();
extern uint32_t Test_PM_Impl_DownloaderRegisterUnregister();
extern uint32_t Test_PM_Impl_InstallerRegisterUnregister();
extern uint32_t Test_PM_Impl_UnregisterUnknownNotification();
extern uint32_t Test_PM_Impl_ListPackagesAndPackageStateForDummyData();
extern uint32_t Test_PM_Impl_ConfigAndGetConfigForPackageEmptyLocator();
extern uint32_t Test_PM_Impl_PauseResumeCancelProgressRateLimitWithoutActiveDownload();
extern uint32_t Test_PM_Impl_DeleteFilePaths();
extern uint32_t Test_PM_Impl_InstallAndUninstallFlowWithNotifications();
extern uint32_t Test_PM_Impl_LockUnlockAndGetLockedInfo();
extern uint32_t Test_PM_Impl_DownloadAndStorageInfoPaths();
extern uint32_t Test_PM_Impl_InstallInputValidationAndUnknownPaths();
extern uint32_t Test_PM_Impl_GetLockedInfoAndUnlockNegativePaths();
extern uint32_t Test_PM_Impl_DownloadWithoutInternetReturnsUnavailable();
extern uint32_t Test_PM_Impl_GetConfigForPackageSuccessPath();
extern uint32_t Test_PM_Impl_InstallDifferentVersionBlockedWhileLockedThenProcessedOnUnlock();
extern uint32_t Test_PM_Impl_UninstallBlockedWhileLockedThenProcessedOnUnlock();
extern uint32_t Test_PM_Component_HttpClient_InvalidOutputPathReturnsDiskError();
extern uint32_t Test_PM_Component_HttpClient_InvalidUrlReturnsHttpError();
extern uint32_t Test_PM_Component_HttpClient_InlineMethodsCoverage();
extern uint32_t Test_PM_Component_TelemetryReporting_Markers();

int main()
{
    L0Test::L0BootstrapGuard bootstrap;

    struct Case {
        const char* name;
        uint32_t (*fn)();
    };

    const Case cases[] = {
        { "PM_Shell_InitializeFailsWhenRootInstantiationFails", Test_PM_Shell_InitializeFailsWhenRootInstantiationFails },
        { "PM_Shell_InitializeSuccessAndQueryInterfaces", Test_PM_Shell_InitializeSuccessAndQueryInterfaces },
        { "PM_Shell_InformationReturnsEmptyString", Test_PM_Shell_InformationReturnsEmptyString },
        { "PM_Shell_DownloadPathThroughPluginAggregate", Test_PM_Shell_DownloadPathThroughPluginAggregate },
        { "PM_Impl_InitializeAndDeinitialize", Test_PM_Impl_InitializeAndDeinitialize },
        { "PM_Impl_DownloaderRegisterUnregister", Test_PM_Impl_DownloaderRegisterUnregister },
        { "PM_Impl_InstallerRegisterUnregister", Test_PM_Impl_InstallerRegisterUnregister },
        { "PM_Impl_UnregisterUnknownNotification", Test_PM_Impl_UnregisterUnknownNotification },
        { "PM_Impl_ListPackagesAndPackageStateForDummyData", Test_PM_Impl_ListPackagesAndPackageStateForDummyData },
        { "PM_Impl_ConfigAndGetConfigForPackageEmptyLocator", Test_PM_Impl_ConfigAndGetConfigForPackageEmptyLocator },
        { "PM_Impl_PauseResumeCancelProgressRateLimitWithoutActiveDownload", Test_PM_Impl_PauseResumeCancelProgressRateLimitWithoutActiveDownload },
        { "PM_Impl_DeleteFilePaths", Test_PM_Impl_DeleteFilePaths },
        { "PM_Impl_InstallAndUninstallFlowWithNotifications", Test_PM_Impl_InstallAndUninstallFlowWithNotifications },
        { "PM_Impl_LockUnlockAndGetLockedInfo", Test_PM_Impl_LockUnlockAndGetLockedInfo },
        { "PM_Impl_DownloadAndStorageInfoPaths", Test_PM_Impl_DownloadAndStorageInfoPaths },
        { "PM_Impl_InstallInputValidationAndUnknownPaths", Test_PM_Impl_InstallInputValidationAndUnknownPaths },
        { "PM_Impl_GetLockedInfoAndUnlockNegativePaths", Test_PM_Impl_GetLockedInfoAndUnlockNegativePaths },
        { "PM_Impl_DownloadWithoutInternetReturnsUnavailable", Test_PM_Impl_DownloadWithoutInternetReturnsUnavailable },
        { "PM_Impl_GetConfigForPackageSuccessPath", Test_PM_Impl_GetConfigForPackageSuccessPath },
        { "PM_Impl_InstallDifferentVersionBlockedWhileLockedThenProcessedOnUnlock", Test_PM_Impl_InstallDifferentVersionBlockedWhileLockedThenProcessedOnUnlock },
        { "PM_Impl_UninstallBlockedWhileLockedThenProcessedOnUnlock", Test_PM_Impl_UninstallBlockedWhileLockedThenProcessedOnUnlock },
        { "PM_Component_HttpClient_InvalidOutputPathReturnsDiskError", Test_PM_Component_HttpClient_InvalidOutputPathReturnsDiskError },
        { "PM_Component_HttpClient_InvalidUrlReturnsHttpError", Test_PM_Component_HttpClient_InvalidUrlReturnsHttpError },
        { "PM_Component_HttpClient_InlineMethodsCoverage", Test_PM_Component_HttpClient_InlineMethodsCoverage },
        { "PM_Component_TelemetryReporting_Markers", Test_PM_Component_TelemetryReporting_Markers },
    };

    L0Test::TestResult tr;

    for (const auto& c : cases) {
        const uint32_t failures = c.fn();
        tr.failures += failures;
        if (0U != failures) {
            std::cerr << "FAIL: " << c.name << " failures=" << failures << std::endl;
        }
    }

    L0Test::PrintTotals(std::cerr, "PackageManager l0test", tr.failures);
    return L0Test::ResultToExitCode(tr);
}
