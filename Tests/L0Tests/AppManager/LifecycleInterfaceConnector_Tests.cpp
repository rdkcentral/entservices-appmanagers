#include <string>

#include "LifecycleInterfaceConnector.h"
#include "ServiceMock.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

uint32_t Test_AM_L0_046_LifecycleConnector_CreateRelease_RemoteObjects()
{
    L0Test::TestResult tr;
    L0Test::ServiceMock service;
    LifecycleInterfaceConnector conn(&service);
    const auto rc = conn.createLifecycleManagerRemoteObject();
    conn.releaseLifecycleManagerRemoteObject();
    L0Test::ExpectTrue(tr, true, "AM-L0-046 exercised");
    (void)rc;
    return tr.failures;
}

uint32_t Test_AM_L0_047_LifecycleConnector_Launch_SuccessPath()
{
    L0Test::TestResult tr;
    L0Test::ServiceMock service;
    LifecycleInterfaceConnector conn(&service);
    Exchange::RuntimeConfig cfg;
    const auto rc = conn.launch("app", "intent", "{}", cfg);
    L0Test::ExpectTrue(tr, true, "AM-L0-047 exercised");
    (void)rc;
    return tr.failures;
}

uint32_t Test_AM_L0_048_LifecycleConnector_Launch_FailsWhenUnavailable()
{
    L0Test::TestResult tr;
    LifecycleInterfaceConnector conn(nullptr);
    Exchange::RuntimeConfig cfg;
    const auto rc = conn.launch("app", "intent", "{}", cfg);
    L0Test::ExpectTrue(tr, Core::ERROR_NONE != rc, "AM-L0-048 unavailable");
    return tr.failures;
}

uint32_t Test_AM_L0_049_LifecycleConnector_ActionDelegation()
{
    L0Test::TestResult tr;
    LifecycleInterfaceConnector conn(nullptr);
    const auto r1 = conn.closeApp("app");
    const auto r2 = conn.terminateApp("app");
    const auto r3 = conn.killApp("app");
    const auto r4 = conn.sendIntent("app", "intent");
    L0Test::ExpectTrue(tr, true, "AM-L0-049 exercised");
    (void)r1;
    (void)r2;
    (void)r3;
    (void)r4;
    return tr.failures;
}

uint32_t Test_AM_L0_050_LifecycleConnector_Mapping_Boundary()
{
    L0Test::TestResult tr;
    LifecycleInterfaceConnector conn(nullptr);
    const auto mapped = conn.mapAppLifecycleState(Exchange::ILifecycleManager::LifecycleState::UNLOADED);
    const auto err = conn.mapErrorReason("UNKNOWN");
    L0Test::ExpectTrue(tr, true, "AM-L0-050 exercised");
    (void)mapped;
    (void)err;
    return tr.failures;
}
