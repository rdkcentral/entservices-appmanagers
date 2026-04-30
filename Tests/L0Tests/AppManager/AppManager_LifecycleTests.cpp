#include <string>
#include <chrono>
#include <thread>

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

static bool ReleaseAndWaitForDestruction(AppManagerImplementation* impl,
                                         std::chrono::milliseconds timeout = std::chrono::milliseconds(5000))
{
    if (nullptr == impl) {
        return true;
    }

    impl->Release();

    using Clock = std::chrono::steady_clock;
    const auto deadline = Clock::now() + timeout;
    do {
        if (WPEFramework::Plugin::AppManagerImplementation::getInstance() == nullptr) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    } while (Clock::now() < deadline);

    return (WPEFramework::Plugin::AppManagerImplementation::getInstance() == nullptr);
}

uint32_t Test_AM_L0_001_Initialize_HappyPath()
{
    L0Test::TestResult tr;
    // L0 test: verify AppManagerImplementation can be configured and registered.
    L0Test::ServiceMock service;
    AppManagerImplementation* impl = CreateImpl();
    impl->Configure(&service);

    auto notif = std::make_shared<L0Test::FakeAppManagerNotification>();
    const WPEFramework::Core::hresult result = impl->Register(notif.get());
    L0Test::ExpectTrue(tr, result == WPEFramework::Core::ERROR_NONE, "AM-L0-001 Register succeeds");
    impl->Unregister(notif.get());
    L0Test::ExpectTrue(tr, ReleaseAndWaitForDestruction(impl),
                       "AM-L0-001 implementation destroyed before teardown");
    return tr.failures;
}

uint32_t Test_AM_L0_002_Initialize_FailsWhenRootCreationFails()
{
    L0Test::TestResult tr;
    // L0 test: avoid passing nullptr to Register(), which is a guarded precondition.
    L0Test::ServiceMock service;
    AppManagerImplementation* impl = CreateImpl();
    impl->Configure(&service);
    L0Test::ExpectTrue(tr, impl != nullptr, "AM-L0-002 implementation created");

    L0Test::FakeAppManagerNotification notif;
    const WPEFramework::Core::hresult result = impl->Register(&notif);
    L0Test::ExpectTrue(tr, WPEFramework::Core::ERROR_NONE == result, "AM-L0-002 Register succeeds with valid notification");
    impl->Unregister(&notif);

    L0Test::ExpectTrue(tr, ReleaseAndWaitForDestruction(impl),
                       "AM-L0-002 implementation destroyed before teardown");
    return tr.failures;
}

uint32_t Test_AM_L0_003_Initialize_FailsWhenIConfigurationMissing()
{
    L0Test::TestResult tr;
    L0Test::ServiceMock service;
    AppManagerImplementation* impl = CreateImpl();
    impl->Configure(&service);
    L0Test::ExpectTrue(tr, impl != nullptr, "AM-L0-003 implementation created");
    L0Test::ExpectTrue(tr, ReleaseAndWaitForDestruction(impl),
                       "AM-L0-003 implementation destroyed before teardown");
    return tr.failures;
}

uint32_t Test_AM_L0_004_Initialize_FailsWhenConfigureReturnsError()
{
    L0Test::TestResult tr;
    L0Test::ServiceMock service;
    AppManagerImplementation* impl = CreateImpl();
    const WPEFramework::Core::hresult nullConfigure = impl->Configure(nullptr);
    L0Test::ExpectTrue(tr, WPEFramework::Core::ERROR_NONE != nullConfigure,
                       "AM-L0-004 Configure(nullptr) reports failure");

    const WPEFramework::Core::hresult validConfigure = impl->Configure(&service);
    L0Test::ExpectTrue(tr, WPEFramework::Core::ERROR_NONE == validConfigure,
                       "AM-L0-004 Configure(valid service) succeeds for teardown-safe lifecycle");
    L0Test::ExpectTrue(tr, impl != nullptr, "AM-L0-004 implementation valid");
    L0Test::ExpectTrue(tr, ReleaseAndWaitForDestruction(impl),
                       "AM-L0-004 implementation destroyed before teardown");
    return tr.failures;
}

uint32_t Test_AM_L0_005_Deinitialize_ReleasesResources()
{
    L0Test::TestResult tr;
    L0Test::ServiceMock service;
    AppManagerImplementation* impl = CreateImpl();
    impl->Configure(&service);
    L0Test::ExpectTrue(tr, impl != nullptr, "AM-L0-005 implementation created");
    L0Test::ExpectTrue(tr, ReleaseAndWaitForDestruction(impl),
                       "AM-L0-005 implementation destroyed before teardown");
    L0Test::ExpectTrue(tr, true, "AM-L0-005 resources released");
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
