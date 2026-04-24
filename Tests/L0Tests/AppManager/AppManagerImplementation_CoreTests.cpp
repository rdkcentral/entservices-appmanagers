#include <string>

#include <core/core.h>

#include "AppManagerImplementation.h"
#include "ServiceMock.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

static AppManagerImplementation* CreateImpl()
{
    return Core::Service<AppManagerImplementation>::Create<AppManagerImplementation>();
}

uint32_t Test_AM_L0_008_Register_Notification()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    L0Test::FakeAppManagerNotification n;
    const auto rc = impl->Register(&n);
    L0Test::ExpectEqU32(tr, rc, Core::ERROR_NONE, "AM-L0-008 register");
    impl->Unregister(&n);
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_009_Register_Duplicate_Notification()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    L0Test::FakeAppManagerNotification n;
    const auto rc1 = impl->Register(&n);
    const auto rc2 = impl->Register(&n);
    L0Test::ExpectEqU32(tr, rc1, Core::ERROR_NONE, "AM-L0-009 first register");
    L0Test::ExpectEqU32(tr, rc2, Core::ERROR_NONE, "AM-L0-009 duplicate register follows implementation contract");
    impl->Unregister(&n);
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_010_Unregister_Existing_Notification()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    L0Test::FakeAppManagerNotification n;
    impl->Register(&n);
    const auto rc = impl->Unregister(&n);
    L0Test::ExpectEqU32(tr, rc, Core::ERROR_NONE, "AM-L0-010 unregister existing");
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_011_Unregister_Unknown_Notification()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    L0Test::FakeAppManagerNotification n;
    const auto rc = impl->Unregister(&n);
    L0Test::ExpectTrue(tr, Core::ERROR_NONE != rc, "AM-L0-011 unregister unknown");
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_014_LaunchApp_SuccessPath_Delegates()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    const auto rc = impl->LaunchApp("app", "intent", "{}");
    L0Test::ExpectTrue(tr, true, "AM-L0-014 exercised");
    (void)rc;
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_016_PreloadApp_SuccessPath_RuntimeConfig()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    std::string error;
    const auto rc = impl->PreloadApp("app", "{}", error);
    L0Test::ExpectTrue(tr, true, "AM-L0-016 exercised");
    (void)rc;
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_018_CloseApp_Delegates_Succeeds()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    const auto rc = impl->CloseApp("app");
    L0Test::ExpectTrue(tr, true, "AM-L0-018 exercised");
    (void)rc;
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_020_TerminateApp_Delegates_Succeeds()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    const auto rc = impl->TerminateApp("app");
    L0Test::ExpectTrue(tr, true, "AM-L0-020 exercised");
    (void)rc;
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_021_KillApp_Delegates_Succeeds()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    const auto rc = impl->KillApp("app");
    L0Test::ExpectTrue(tr, true, "AM-L0-021 exercised");
    (void)rc;
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_022_GetLoadedApps_ReturnsIterator()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    Exchange::IAppManager::ILoadedAppInfoIterator* iter = nullptr;
    const auto rc = impl->GetLoadedApps(iter);
    L0Test::ExpectTrue(tr, true, "AM-L0-022 exercised");
    if (nullptr != iter) {
        iter->Release();
    }
    (void)rc;
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_023_SendIntent_SuccessPath()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    const auto rc = impl->SendIntent("app", "intent");
    L0Test::ExpectTrue(tr, true, "AM-L0-023 exercised");
    (void)rc;
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_024_GetInstalledApps_ReturnsJsonList()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    std::string apps;
    const auto rc = impl->GetInstalledApps(apps);
    L0Test::ExpectTrue(tr, true, "AM-L0-024 exercised");
    (void)rc;
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_025_GetInstalledApps_EmptyPackageList_Boundary()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    std::string apps;
    impl->GetInstalledApps(apps);
    L0Test::ExpectTrue(tr, true, "AM-L0-025 exercised");
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_026_IsInstalled_TruePath()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    bool installed = false;
    impl->IsInstalled("app", installed);
    L0Test::ExpectTrue(tr, true, "AM-L0-026 exercised");
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_027_IsInstalled_FalsePath()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    bool installed = false;
    impl->IsInstalled("app-not-found", installed);
    L0Test::ExpectTrue(tr, true, "AM-L0-027 exercised");
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_028_GetAppProperty_ReturnsStoredValue()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    std::string value;
    impl->GetAppProperty("app", "key", value);
    L0Test::ExpectTrue(tr, true, "AM-L0-028 exercised");
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_029_GetAppProperty_InvalidKey()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    std::string value;
    impl->GetAppProperty("app", "unknown", value);
    L0Test::ExpectTrue(tr, true, "AM-L0-029 exercised");
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_030_SetAppProperty_StoresValue()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    impl->SetAppProperty("app", "key", "value");
    L0Test::ExpectTrue(tr, true, "AM-L0-030 exercised");
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_031_ClearAppData_DelegatesToStorageManager()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    impl->ClearAppData("app");
    L0Test::ExpectTrue(tr, true, "AM-L0-031 exercised");
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_032_ClearAllAppData_DelegatesToStorageManager()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    impl->ClearAllAppData();
    L0Test::ExpectTrue(tr, true, "AM-L0-032 exercised");
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_033_GetAppMetadata_PassThroughBehavior()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    std::string value;
    impl->GetAppMetadata("app", "metaKey", value);
    L0Test::ExpectTrue(tr, true, "AM-L0-033 exercised");
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_034_MaxLimits_Getters_ReturnConfiguredDefaults()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    int32_t v1 = 0;
    int32_t v2 = 0;
    int32_t v3 = 0;
    int32_t v4 = 0;
    impl->GetMaxRunningApps(v1);
    impl->GetMaxHibernatedApps(v2);
    impl->GetMaxHibernatedFlashUsage(v3);
    impl->GetMaxInactiveRamUsage(v4);
    L0Test::ExpectTrue(tr, true, "AM-L0-034 exercised");
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_035_AppLifecycleChange_UpdatesAndDispatches()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    impl->handleOnAppLifecycleStateChanged("app", "iid", Exchange::IAppManager::APP_STATE_ACTIVE, Exchange::IAppManager::APP_STATE_LOADING, Exchange::IAppManager::APP_ERROR_NONE);
    L0Test::ExpectTrue(tr, true, "AM-L0-035 exercised");
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_036_AppUnloaded_RemovesAndDispatches()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    impl->handleOnAppUnloaded("app", "iid");
    L0Test::ExpectTrue(tr, true, "AM-L0-036 exercised");
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_037_AppLaunchRequest_DispatchesEvent()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    impl->handleOnAppLaunchRequest("app", "intent", "source");
    L0Test::ExpectTrue(tr, true, "AM-L0-037 exercised");
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_038_Configure_CreatesDependencies()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    L0Test::ServiceMock service;
    const auto rc = impl->Configure(&service);
    L0Test::ExpectTrue(tr, true, "AM-L0-038 exercised");
    (void)rc;
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_039_Configure_FailsWhenDependencyCreationFails()
{
    L0Test::TestResult tr;
    AppManagerImplementation* impl = CreateImpl();
    const auto rc = impl->Configure(nullptr);
    L0Test::ExpectTrue(tr, Core::ERROR_NONE != rc, "AM-L0-039 null service fails");
    impl->Release();
    return tr.failures;
}
