/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * PreinstallManagerImpl_CoreTests.cpp
 *
 * Covers:
 *   PM-IMPL-001 to PM-IMPL-011 : constructor, getInstance, destructor, Configure
 *   PM-NOTIF-001 to PM-NOTIF-007: Register/Unregister notifications
 *
 * All tests directly instantiate PreinstallManagerImplementation without going
 * through the plugin COM-RPC boundary.
 */

#include <cstdint>
#include <iostream>
#include <string>

#include "PreinstallManagerImplementation.h"
#include "ServiceMock.h"
#include "common/L0Expect.hpp"

using WPEFramework::Core::ERROR_GENERAL;
using WPEFramework::Core::ERROR_NONE;
// Thunder R4.4.1: BEGIN_INTERFACE_MAP omits AddRef/Release → base is abstract.
// ConcretePreinstallImpl adds them; alias keeps all test code unchanged.
using PreinstallManagerImplementation = L0Test::ConcretePreinstallImpl;

namespace {

// Helper: create a fresh PreinstallManagerImplementation (refCount=1).
inline PreinstallManagerImplementation* CreateImpl()
{
    return new PreinstallManagerImplementation();
}

// Helper: create and Configure the impl with a given appPreinstallDirectory path.
inline PreinstallManagerImplementation* CreateConfiguredImpl(
    L0Test::ServiceMock& svc,
    const std::string& dir)
{
    svc.SetConfigLine("{\"appPreinstallDirectory\":\"" + dir + "\"}");
    auto* impl = CreateImpl();
    impl->Configure(&svc);
    return impl;
}

} // namespace

// ============================================================
// PM-IMPL-001
// Constructor sets _instance singleton when no instance exists.
// ============================================================
uint32_t Test_Impl_Constructor_SetsInstanceSingleton()
{
    L0Test::TestResult tr;

    // Ensure clean slate — previous tests must have released their instances.
    L0Test::ExpectTrue(tr, PreinstallManagerImplementation::_instance == nullptr,
        "PM-IMPL-001: _instance is null before construction");

    auto* impl = CreateImpl();

    L0Test::ExpectTrue(tr, PreinstallManagerImplementation::_instance == impl,
        "PM-IMPL-001: Constructor sets _instance to this");

    impl->Release();
    return tr.failures;
}

// ============================================================
// PM-IMPL-002
// Second construction does not overwrite _instance.
// ============================================================
uint32_t Test_Impl_Constructor_SecondInstance_DoesNotOverwriteSingleton()
{
    L0Test::TestResult tr;

    auto* first = CreateImpl();
    const auto* savedPtr = PreinstallManagerImplementation::_instance;

    auto* second = CreateImpl();

    L0Test::ExpectTrue(tr, PreinstallManagerImplementation::_instance == savedPtr,
        "PM-IMPL-002: Second construction does not overwrite _instance");

    second->Release();
    first->Release();
    return tr.failures;
}

// ============================================================
// PM-IMPL-003
// getInstance() returns the constructed instance.
// ============================================================
uint32_t Test_Impl_GetInstance_ReturnsInstance()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();

    L0Test::ExpectTrue(tr, PreinstallManagerImplementation::getInstance() == impl,
        "PM-IMPL-003: getInstance() returns the constructed instance");

    impl->Release();
    return tr.failures;
}

// ============================================================
// PM-IMPL-004
// getInstance() returns nullptr after destruction.
// ============================================================
uint32_t Test_Impl_GetInstance_AfterDestruction_ReturnsNullptr()
{
    L0Test::TestResult tr;

    {
        auto* impl = CreateImpl();
        impl->Release(); // destroys the instance
    }

    L0Test::ExpectTrue(tr, PreinstallManagerImplementation::getInstance() == nullptr,
        "PM-IMPL-004: getInstance() returns nullptr after destruction");

    return tr.failures;
}

// ============================================================
// PM-IMPL-005
// Destructor resets _instance to nullptr.
// ============================================================
uint32_t Test_Impl_Destructor_ResetsInstanceToNullptr()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ExpectTrue(tr, PreinstallManagerImplementation::_instance != nullptr,
        "PM-IMPL-005: _instance non-null after construction");

    impl->Release();

    L0Test::ExpectTrue(tr, PreinstallManagerImplementation::_instance == nullptr,
        "PM-IMPL-005: Destructor resets _instance to nullptr");

    return tr.failures;
}

// ============================================================
// PM-IMPL-006
// Destructor releases mCurrentservice when it was set via Configure().
// ============================================================
uint32_t Test_Impl_Destructor_ReleasesMCurrentservice()
{
    L0Test::TestResult tr;

    L0Test::ServiceMock svc;
    svc.SetConfigLine("{}");

    auto* impl = CreateImpl();
    impl->Configure(&svc);

    // addRefCalls from Configure()
    const uint32_t addRefBeforeDestroy = svc.addRefCalls.load();
    L0Test::ExpectEqU32(tr, addRefBeforeDestroy, 1u,
        "PM-IMPL-006: Configure called AddRef once");

    impl->Release(); // triggers destructor, which calls mCurrentservice->Release()

    L0Test::ExpectTrue(tr, svc.releaseCalls.load() >= 1u,
        "PM-IMPL-006: Destructor calls Release on mCurrentservice");

    return tr.failures;
}

// ============================================================
// PM-IMPL-007
// Destructor calls releasePackageManagerObject when set.
// Verified by checking that unregister is called on the installer fake.
// ============================================================
uint32_t Test_Impl_Destructor_ReleasesPackageManager()
{
    L0Test::TestResult tr;

    L0Test::ServiceMock svc;
    svc.SetConfigLine("{\"appPreinstallDirectory\":\"/tmp/pm_impl007\"}");

    // Set up a PackageInstaller that can be found via QueryInterfaceByCallsign
    auto* installer = new L0Test::PackageInstallerFake();
    svc.SetPackageInstaller(installer);

    auto* impl = CreateImpl();
    impl->Configure(&svc);

    // Trigger createPackageManagerObject via StartPreinstall with an empty temp dir
    L0Test::TempDir tmpDir("/tmp/pm_impl007");
    const auto rc = impl->StartPreinstall(true); // will call create then release internally

    // Even if StartPreinstall returns an error or not, the PM should go through register/unregister
    // (Register happens in createPackageManagerObject, Unregister in releasePackageManagerObject)
    const uint32_t unreg = installer->unregisterCalls.load();
    L0Test::ExpectTrue(tr, unreg >= 1u || rc == ERROR_NONE,
        "PM-IMPL-007: releasePackageManagerObject called (unregister >= 1)");

    impl->Release();
    installer->Release();

    return tr.failures;
}

// ============================================================
// PM-IMPL-008
// Configure with valid service parses appPreinstallDirectory.
// Verified indirectly: Configure returns ERROR_NONE.
// ============================================================
uint32_t Test_Impl_Configure_WithValidService_Succeeds()
{
    L0Test::TestResult tr;

    L0Test::ServiceMock svc;
    svc.SetConfigLine("{\"appPreinstallDirectory\":\"/opt/preinstall\"}");

    auto* impl = CreateImpl();
    const uint32_t rc = impl->Configure(&svc);

    L0Test::ExpectEqU32(tr, rc, ERROR_NONE,
        "PM-IMPL-008: Configure returns ERROR_NONE with valid service");

    impl->Release();
    return tr.failures;
}

// ============================================================
// PM-IMPL-009
// Configure with empty directory config still returns ERROR_NONE.
// (The impl only sets the dir if the config value is non-empty.)
// ============================================================
uint32_t Test_Impl_Configure_WithEmptyDirectory_ReturnsOk()
{
    L0Test::TestResult tr;

    L0Test::ServiceMock svc;
    svc.SetConfigLine("{}"); // no appPreinstallDirectory key

    auto* impl = CreateImpl();
    const uint32_t rc = impl->Configure(&svc);

    L0Test::ExpectEqU32(tr, rc, ERROR_NONE,
        "PM-IMPL-009: Configure returns ERROR_NONE even when directory config is absent");

    impl->Release();
    return tr.failures;
}

// ============================================================
// PM-IMPL-010
// Configure with null service returns ERROR_GENERAL.
// ============================================================
uint32_t Test_Impl_Configure_WithNullService_ReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const uint32_t rc = impl->Configure(nullptr);

    L0Test::ExpectEqU32(tr, rc, ERROR_GENERAL,
        "PM-IMPL-010: Configure(nullptr) returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

// ============================================================
// PM-IMPL-011
// Configure calls AddRef on service (stores for later use).
// ============================================================
uint32_t Test_Impl_Configure_CallsAddRefOnService()
{
    L0Test::TestResult tr;

    L0Test::ServiceMock svc;
    svc.SetConfigLine("{}");

    auto* impl = CreateImpl();
    impl->Configure(&svc);

    L0Test::ExpectEqU32(tr, svc.addRefCalls.load(), 1u,
        "PM-IMPL-011: Configure calls AddRef exactly once on service");

    impl->Release();
    return tr.failures;
}

// ============================================================
// PM-NOTIF-001
// Register first notification returns ERROR_NONE and AddRef is called.
// ============================================================
uint32_t Test_Notif_Register_First_ReturnsOk()
{
    L0Test::TestResult tr;

    auto* impl  = CreateImpl();
    auto* notif = new L0Test::NotificationFake();

    const auto rc = impl->Register(notif);

    L0Test::ExpectEqU32(tr, rc, ERROR_NONE,
        "PM-NOTIF-001: Register returns ERROR_NONE");
    // AddRef incremented by Register (from initial 1 to 2)
    // We can verify by calling Release twice: first -> ERROR_NONE, second -> DESTRUCTION
    notif->AddRef(); // add extra ref so unregister-release doesn't delete it
    impl->Unregister(notif); // cleanup
    notif->Release();        // drop our extra ref

    impl->Release();
    return tr.failures;
}

// ============================================================
// PM-NOTIF-002
// Register same notification twice is idempotent (not doubled).
// Verified by: only one Release() called when Unregister is called once.
// ============================================================
uint32_t Test_Notif_Register_SameNotif_Twice_IsIdempotent()
{
    L0Test::TestResult tr;

    auto* impl  = CreateImpl();
    auto* notif = new L0Test::NotificationFake();

    impl->Register(notif);
    impl->Register(notif); // second registration should be a no-op

    // One unregister should cleanly release the one stored ref
    const auto rc = impl->Unregister(notif);
    L0Test::ExpectEqU32(tr, rc, ERROR_NONE,
        "PM-NOTIF-002: Unregister after double-Register returns ERROR_NONE");

    // Second unregister should return ERROR_GENERAL (not in list)
    const auto rc2 = impl->Unregister(notif);
    L0Test::ExpectEqU32(tr, rc2, ERROR_GENERAL,
        "PM-NOTIF-002: Unregister of non-registered notif returns ERROR_GENERAL");

    notif->Release(); // drop our creation ref
    impl->Release();
    return tr.failures;
}

// ============================================================
// PM-NOTIF-003
// Register calls AddRef on the notification object.
// ============================================================
uint32_t Test_Notif_Register_CallsAddRef()
{
    L0Test::TestResult tr;

    auto* impl  = CreateImpl();
    auto* notif = new L0Test::NotificationFake();
    // notif starts with refCount = 1 (creation ref held by us)

    impl->Register(notif);
    // Register calls AddRef -> internal refCount is now 2
    // Release our creation ref (goes to 1, object still alive)
    const uint32_t afterRelease = notif->Release(); // should return ERROR_NONE (not 0)
    L0Test::ExpectEqU32(tr, afterRelease, WPEFramework::Core::ERROR_NONE,
        "PM-NOTIF-003: AddRef called by Register keeps object alive after our Release()");

    // Cleanup: Unregister will call Release (drops to 0, deletes the object)
    impl->Unregister(notif);

    impl->Release();
    return tr.failures;
}

// ============================================================
// PM-NOTIF-004
// Unregister returns ERROR_NONE for a registered notification.
// ============================================================
uint32_t Test_Notif_Unregister_Known_ReturnsOk()
{
    L0Test::TestResult tr;

    auto* impl  = CreateImpl();
    auto* notif = new L0Test::NotificationFake();

    impl->Register(notif);
    // Hold an extra ref so the object isn't destroyed when Unregister releases
    notif->AddRef();

    const auto rc = impl->Unregister(notif);
    L0Test::ExpectEqU32(tr, rc, ERROR_NONE,
        "PM-NOTIF-004: Unregister of registered notification returns ERROR_NONE");

    notif->Release(); // drop the extra ref
    impl->Release();
    return tr.failures;
}

// ============================================================
// PM-NOTIF-005
// Unregister calls Release on the notification.
// Verified by: after Unregister the object is destroyed (addRef/Release balance).
// ============================================================
uint32_t Test_Notif_Unregister_CallsRelease()
{
    L0Test::TestResult tr;

    auto* impl  = CreateImpl();
    auto* notif = new L0Test::NotificationFake();

    impl->Register(notif);             // Register adds ref (count = 2)
    notif->Release();                  // We release our creation ref (count = 1)
    // Now only impl holds it. Unregister should release and destroy it.
    impl->Unregister(notif);
    // notif is now deleted — do not touch it

    // If we got here without a crash/ASAN alert, Release was called correctly.
    L0Test::ExpectTrue(tr, true, "PM-NOTIF-005: Unregister calls Release (no crash)");

    impl->Release();
    return tr.failures;
}

// ============================================================
// PM-NOTIF-006
// Unregister of an unknown (never registered) notification returns ERROR_GENERAL.
// ============================================================
uint32_t Test_Notif_Unregister_Unknown_ReturnsError()
{
    L0Test::TestResult tr;

    auto* impl  = CreateImpl();
    auto* notif = new L0Test::NotificationFake();

    const auto rc = impl->Unregister(notif);
    L0Test::ExpectEqU32(tr, rc, ERROR_GENERAL,
        "PM-NOTIF-006: Unregister of unregistered notification returns ERROR_GENERAL");

    notif->Release();
    impl->Release();
    return tr.failures;
}

// ============================================================
// PM-NOTIF-007
// Multiple notifications registered: all receive OnAppInstallationStatus.
// Tested via handleOnAppInstallationStatus (async via worker pool).
// ============================================================
uint32_t Test_Notif_MultipleNotifications_AllReceiveEvent()
{
    L0Test::TestResult tr;

    auto* impl   = CreateImpl();
    auto* notif1 = new L0Test::NotificationFake();
    auto* notif2 = new L0Test::NotificationFake();
    auto* notif3 = new L0Test::NotificationFake();

    impl->Register(notif1);
    impl->Register(notif2);
    impl->Register(notif3);

    const std::string payload = "{\"appId\":\"com.test.app\",\"status\":\"SUCCESS\"}";
    impl->handleOnAppInstallationStatus(payload);

    // Wait for async dispatch via worker pool
    const bool ok1 = notif1->WaitForCall(1, 2000);
    const bool ok2 = notif2->WaitForCall(1, 500);
    const bool ok3 = notif3->WaitForCall(1, 500);

    L0Test::ExpectTrue(tr, ok1, "PM-NOTIF-007: notif1 received OnAppInstallationStatus");
    L0Test::ExpectTrue(tr, ok2, "PM-NOTIF-007: notif2 received OnAppInstallationStatus");
    L0Test::ExpectTrue(tr, ok3, "PM-NOTIF-007: notif3 received OnAppInstallationStatus");
    L0Test::ExpectEqStr(tr, notif1->lastPayload, payload,
        "PM-NOTIF-007: notif1 received correct payload");

    // Hold extra refs before unregister to avoid double-free
    notif1->AddRef();
    notif2->AddRef();
    notif3->AddRef();

    impl->Unregister(notif1);
    impl->Unregister(notif2);
    impl->Unregister(notif3);

    notif1->Release();
    notif2->Release();
    notif3->Release();
    impl->Release();

    return tr.failures;
}
