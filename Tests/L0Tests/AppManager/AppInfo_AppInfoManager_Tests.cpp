#include <string>

#include "AppInfo.h"
#include "AppInfoManager.h"
#include "AppManagerTypes.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

uint32_t Test_AM_L0_040_AppInfo_DefaultObjectValues()
{
    L0Test::TestResult tr;
    AppInfo info;
    L0Test::ExpectTrue(tr, info.getAppInstanceId().empty(), "AM-L0-040 default app instance id");
    return tr.failures;
}

uint32_t Test_AM_L0_041_AppInfo_GetterSetter_Consistency()
{
    L0Test::TestResult tr;
    AppInfo info;
    info.setAppInstanceId("id");
    L0Test::ExpectEqStr(tr, info.getAppInstanceId(), "id", "AM-L0-041 app instance roundtrip");
    return tr.failures;
}

uint32_t Test_AM_L0_042_AppInfoManager_Upsert_Create_Update()
{
    L0Test::TestResult tr;
    auto& mgr = AppInfoManager::getInstance();
    mgr.clear();
    mgr.upsert("app", [](AppInfo& item) { item.setAppInstanceId("iid1"); });
    mgr.upsert("app", [](AppInfo& item) { item.setAppInstanceId("iid2"); });
    L0Test::ExpectEqStr(tr, mgr.getAppInstanceId("app"), "iid2", "AM-L0-042 upsert update");
    mgr.clear();
    return tr.failures;
}

uint32_t Test_AM_L0_043_AppInfoManager_Update_AbsentKey_Fails()
{
    L0Test::TestResult tr;
    auto& mgr = AppInfoManager::getInstance();
    mgr.clear();
    const bool ok = mgr.update("absent", [](AppInfo&) {});
    L0Test::ExpectTrue(tr, !ok, "AM-L0-043 update absent key");
    return tr.failures;
}

uint32_t Test_AM_L0_044_AppInfoManager_Remove_Clear_Behavior()
{
    L0Test::TestResult tr;
    auto& mgr = AppInfoManager::getInstance();
    mgr.clear();
    mgr.upsert("a", [](AppInfo&) {});
    mgr.upsert("b", [](AppInfo&) {});
    mgr.remove("a");
    L0Test::ExpectTrue(tr, !mgr.exists("a"), "AM-L0-044 remove one");
    mgr.clear();
    L0Test::ExpectTrue(tr, !mgr.exists("b"), "AM-L0-044 clear all");
    return tr.failures;
}

uint32_t Test_AM_L0_045_AppInfoManager_Defaults_UnknownApp()
{
    L0Test::TestResult tr;
    auto& mgr = AppInfoManager::getInstance();
    mgr.clear();
    L0Test::ExpectTrue(tr, mgr.getAppInstanceId("unknown").empty(), "AM-L0-045 unknown defaults");
    return tr.failures;
}

uint32_t Test_AM_L0_055_AppManagerTypes_Boundary_ValueMapping()
{
    L0Test::TestResult tr;
    AppManagerTypes::PackageInfo info;
    info.type = AppManagerTypes::APPLICATION_TYPE_SYSTEM;
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(info.type), static_cast<uint32_t>(AppManagerTypes::APPLICATION_TYPE_SYSTEM), "AM-L0-055 package type");
    return tr.failures;
}
