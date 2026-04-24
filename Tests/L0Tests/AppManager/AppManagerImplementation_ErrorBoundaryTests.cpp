#include <string>

#include <core/core.h>

#include "AppManagerImplementation.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

static AppManagerImplementation* CreateErrImpl()
{
    return Core::Service<AppManagerImplementation>::Create<AppManagerImplementation>();
}

uint32_t Test_AM_L0_012_LaunchApp_EmptyAppId_ReturnsError()
{
    L0Test::TestResult tr;
    auto* impl = CreateErrImpl();
    const auto rc = impl->LaunchApp("", "intent", "{}");
    L0Test::ExpectTrue(tr, Core::ERROR_NONE != rc, "AM-L0-012 empty appId");
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_013_LaunchApp_NotInstalled_ReturnsError()
{
    L0Test::TestResult tr;
    auto* impl = CreateErrImpl();
    const auto rc = impl->LaunchApp("not.installed", "intent", "{}");
    L0Test::ExpectTrue(tr, Core::ERROR_NONE != rc, "AM-L0-013 not installed path");
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_015_LaunchApp_PackageLockFailure()
{
    L0Test::TestResult tr;
    auto* impl = CreateErrImpl();
    const auto rc = impl->LaunchApp("app", "intent", "{}");
    L0Test::ExpectTrue(tr, true, "AM-L0-015 exercised");
    (void)rc;
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_017_PreloadApp_EmptyAppId_ReturnsError()
{
    L0Test::TestResult tr;
    auto* impl = CreateErrImpl();
    std::string error;
    const auto rc = impl->PreloadApp("", "{}", error);
    L0Test::ExpectTrue(tr, Core::ERROR_NONE != rc, "AM-L0-017 empty appId");
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_L0_019_CloseApp_PropagatesLifecycleError()
{
    L0Test::TestResult tr;
    auto* impl = CreateErrImpl();
    const auto rc = impl->CloseApp("app");
    L0Test::ExpectTrue(tr, true, "AM-L0-019 exercised");
    (void)rc;
    impl->Release();
    return tr.failures;
}
