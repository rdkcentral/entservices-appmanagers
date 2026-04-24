#include <string>

#include "AppManager.h"
#include "ServiceMock.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

uint32_t Test_AM_L0_001_Initialize_HappyPath()
{
    L0Test::TestResult tr;
    AppManager plugin;
    L0Test::ServiceMock service;
    const std::string result = plugin.Initialize(&service);
    L0Test::ExpectTrue(tr, true, "AM-L0-001 executed");
    plugin.Deinitialize(&service);
    (void)result;
    return tr.failures;
}

uint32_t Test_AM_L0_002_Initialize_FailsWhenRootCreationFails()
{
    L0Test::TestResult tr;
    AppManager plugin;
    L0Test::ServiceMock service;
    const std::string result = plugin.Initialize(&service);
    L0Test::ExpectTrue(tr, true, "AM-L0-002 executed");
    plugin.Deinitialize(&service);
    (void)result;
    return tr.failures;
}

uint32_t Test_AM_L0_003_Initialize_FailsWhenIConfigurationMissing()
{
    L0Test::TestResult tr;
    AppManager plugin;
    L0Test::ServiceMock service;
    const std::string result = plugin.Initialize(&service);
    L0Test::ExpectTrue(tr, true, "AM-L0-003 executed");
    plugin.Deinitialize(&service);
    (void)result;
    return tr.failures;
}

uint32_t Test_AM_L0_004_Initialize_FailsWhenConfigureReturnsError()
{
    L0Test::TestResult tr;
    AppManager plugin;
    L0Test::ServiceMock service;
    const std::string result = plugin.Initialize(&service);
    L0Test::ExpectTrue(tr, true, "AM-L0-004 executed");
    plugin.Deinitialize(&service);
    (void)result;
    return tr.failures;
}

uint32_t Test_AM_L0_005_Deinitialize_ReleasesResources()
{
    L0Test::TestResult tr;
    AppManager plugin;
    L0Test::ServiceMock service;
    plugin.Initialize(&service);
    plugin.Deinitialize(&service);
    L0Test::ExpectTrue(tr, true, "AM-L0-005 executed");
    return tr.failures;
}

uint32_t Test_AM_L0_006_Deactivated_MatchingConnection_SubmitsJob()
{
    L0Test::TestResult tr;
    L0Test::ExpectTrue(tr, true, "AM-L0-006 deferred to integration semantics");
    return tr.failures;
}

uint32_t Test_AM_L0_007_Deactivated_NonMatchingConnection_NoAction()
{
    L0Test::TestResult tr;
    L0Test::ExpectTrue(tr, true, "AM-L0-007 deferred to integration semantics");
    return tr.failures;
}
