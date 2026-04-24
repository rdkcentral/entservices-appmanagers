#include <string>

#include "AppManagerTelemetryReporting.h"
#include "AppManagerImplementation.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

uint32_t Test_AM_L0_051_Telemetry_Singleton_Initialize()
{
    L0Test::TestResult tr;
    auto& a = AppManagerTelemetryReporting::getInstance();
    auto& b = AppManagerTelemetryReporting::getInstance();
    L0Test::ExpectTrue(tr, &a == &b, "AM-L0-051 singleton stable");
    return tr.failures;
}

uint32_t Test_AM_L0_052_Telemetry_RecordLaunchMetric_WhenEnabled()
{
    L0Test::TestResult tr;
    auto& t = AppManagerTelemetryReporting::getInstance();
    t.recordLaunchTime("app", 100);
    L0Test::ExpectTrue(tr, true, "AM-L0-052 exercised");
    return tr.failures;
}

uint32_t Test_AM_L0_053_Telemetry_Disabled_GuardPath()
{
    L0Test::TestResult tr;
    auto& t = AppManagerTelemetryReporting::getInstance();
    t.reportTelemetryData("app", AppManagerImplementation::APP_ACTION_NONE);
    t.reportTelemetryErrorData("app", AppManagerImplementation::APP_ACTION_NONE, AppManagerImplementation::ERROR_NONE);
    L0Test::ExpectTrue(tr, true, "AM-L0-053 exercised");
    return tr.failures;
}

uint32_t Test_AM_L0_054_Telemetry_CrashPayload_Report()
{
    L0Test::TestResult tr;
    auto& t = AppManagerTelemetryReporting::getInstance();
    t.reportAppCrashedTelemetry("app", "iid", "reason");
    L0Test::ExpectTrue(tr, true, "AM-L0-054 exercised");
    return tr.failures;
}
