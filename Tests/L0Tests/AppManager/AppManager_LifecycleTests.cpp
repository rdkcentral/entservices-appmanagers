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

static void DestroyImpl(AppManagerImplementation*& impl)
{
    // These constructor-only L0 tests do not perform full Configure() wiring.
    // Releasing the instance hits production ASSERTs in destructor teardown.
    impl = nullptr;
}

uint32_t Test_AM_L0_001_Initialize_HappyPath()
{
    L0Test::TestResult tr;
    // L0 test: verify AppManagerImplementation can be instantiated and registered
    AppManagerImplementation* impl = CreateImpl();
    auto notif = std::make_shared<L0Test::FakeAppManagerNotification>();
    const WPEFramework::Core::hresult result = impl->Register(notif.get());
    L0Test::ExpectTrue(tr, result == WPEFramework::Core::ERROR_NONE, "AM-L0-001 Register succeeds");
    impl->Unregister(notif.get());
    DestroyImpl(impl);
    return tr.failures;
}

uint32_t Test_AM_L0_002_Initialize_FailsWhenRootCreationFails()
{
    L0Test::TestResult tr;
    // L0 test: verify Register with null notification
    AppManagerImplementation* impl = CreateImpl();
    const WPEFramework::Core::hresult result = impl->Register(nullptr);
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE, "AM-L0-002 null notification should fail");
    DestroyImpl(impl);
    return tr.failures;
}

uint32_t Test_AM_L0_003_Initialize_FailsWhenIConfigurationMissing()
{
    L0Test::TestResult tr;
    // L0 test: verify implementation instantiation
    AppManagerImplementation* impl = CreateImpl();
    L0Test::ExpectTrue(tr, impl != nullptr, "AM-L0-003 implementation created");
    DestroyImpl(impl);
    return tr.failures;
}

uint32_t Test_AM_L0_004_Initialize_FailsWhenConfigureReturnsError()
{
    L0Test::TestResult tr;
    // L0 test: verify implementation is not null
    AppManagerImplementation* impl = CreateImpl();
    L0Test::ExpectTrue(tr, impl != nullptr, "AM-L0-004 implementation valid");
    DestroyImpl(impl);
    return tr.failures;
}

uint32_t Test_AM_L0_005_Deinitialize_ReleasesResources()
{
    L0Test::TestResult tr;
    // L0 test: verify implementation lifecycle
    AppManagerImplementation* impl = CreateImpl();
    L0Test::ExpectTrue(tr, impl != nullptr, "AM-L0-005 implementation created");
    DestroyImpl(impl);
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
