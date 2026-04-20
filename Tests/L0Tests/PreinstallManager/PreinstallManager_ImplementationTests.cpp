/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
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
 **/

/**
 * @file PreinstallManager_ImplementationTests.cpp
 *
 * L0 tests for PreinstallManagerImplementation covering:
 *   - Register / Unregister notifications
 *   - Configure (null service, valid service, directory parsing)
 *   - GetPreinstallState initial value
 *   - getInstance() singleton accessor
 *   - QueryInterface responses
 *   - StartPreinstall failure paths (no service, no installer)
 *   - Multiple notifications register/unregister
 *
 * Test plan IDs covered:
 *   PIM-POS-008, PIM-POS-009, PIM-POS-010, PIM-POS-024, PIM-POS-025 (partial)
 *   PIM-BND-001
 *   PIM-NEG-005, PIM-NEG-006, PIM-NEG-007, PIM-NEG-008, PIM-NEG-011
 */

#include <atomic>
#include <iostream>
#include <string>

#include "PreinstallManagerImplementation.h"
#include "ServiceMock.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/* Creates a heap-allocated PreinstallManagerImplementation with refcount=1. */
WPEFramework::Plugin::PreinstallManagerImplementation* CreateImpl()
{
    return WPEFramework::Core::Service<
        WPEFramework::Plugin::PreinstallManagerImplementation>::Create<
        WPEFramework::Plugin::PreinstallManagerImplementation>();
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Register / Unregister  (PIM-POS-008, PIM-BND-001, PIM-POS-009, PIM-NEG-005)
// ─────────────────────────────────────────────────────────────────────────────

/* PIM-POS-008
 * Register() accepts a valid notification pointer and returns ERROR_NONE.
 */
uint32_t Test_Impl_PIM_RegisterNotification()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::FakePreinstallNotification notif;

    const auto result = impl->Register(&notif);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
                        "Register() returns ERROR_NONE for a valid notification");

    impl->Unregister(&notif);
    impl->Release();
    return tr.failures;
}

/* PIM-BND-001
 * Registering the same notification pointer twice is idempotent — the second
 * call must return ERROR_NONE and must NOT add a duplicate entry.
 */
uint32_t Test_Impl_PIM_RegisterDuplicate()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::FakePreinstallNotification notif;

    impl->Register(&notif);
    const auto result = impl->Register(&notif); // duplicate

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
                        "Duplicate Register() returns ERROR_NONE (idempotent)");

    impl->Unregister(&notif);
    impl->Release();
    return tr.failures;
}

/* PIM-POS-009
 * Unregister() returns ERROR_NONE when removing a previously registered observer.
 */
uint32_t Test_Impl_PIM_UnregisterNotification()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::FakePreinstallNotification notif;

    impl->Register(&notif);
    const auto result = impl->Unregister(&notif);

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
                        "Unregister() returns ERROR_NONE for a registered notification");

    impl->Release();
    return tr.failures;
}

/* PIM-NEG-005
 * Unregister() returns ERROR_GENERAL when the observer was never registered.
 */
uint32_t Test_Impl_PIM_UnregisterNotRegistered()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::FakePreinstallNotification notif; // never registered

    const auto result = impl->Unregister(&notif);

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Unregister() on an unregistered notification returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// Configure  (PIM-POS-010, PIM-NEG-006)
// ─────────────────────────────────────────────────────────────────────────────

/* PIM-NEG-006
 * Configure(nullptr) returns ERROR_GENERAL promptly.
 */
uint32_t Test_Impl_PIM_ConfigureWithNullService()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Configure(nullptr);

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Configure(nullptr) returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

/* PIM-POS-010
 * Configure() with a valid service reads appPreinstallDirectory from ConfigLine().
 */
uint32_t Test_Impl_PIM_ConfigureParsesPreinstallDirectory()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();

    L0Test::ServiceMock::Config cfg;
    cfg.configLine = "{\"appPreinstallDirectory\":\"/opt/preinstall/apps\"}";

    L0Test::ServiceMock service(cfg);
    const auto result = impl->Configure(&service);

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
                        "Configure() with a valid service returns ERROR_NONE");
    // AddRef should have been called on the service inside Configure.
    L0Test::ExpectEqU32(tr, service.addRefCalls.load(), 1U,
                        "Configure() calls IShell::AddRef exactly once");

    impl->Release();
    return tr.failures;
}

/* PIM-POS-010 variant
 * Configure() with a service whose config line has no appPreinstallDirectory
 * still returns ERROR_NONE (empty value left as default).
 */
uint32_t Test_Impl_PIM_ConfigureWithEmptyDirectoryParsesOk()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();

    L0Test::ServiceMock::Config cfg;
    cfg.configLine = "{}"; // no appPreinstallDirectory key

    L0Test::ServiceMock service(cfg);
    const auto result = impl->Configure(&service);

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
                        "Configure() with empty config line returns ERROR_NONE");

    impl->Release();
    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// GetPreinstallState  (PIM-POS-024)
// ─────────────────────────────────────────────────────────────────────────────

/* PIM-POS-024
 * GetPreinstallState() returns NOT_STARTED immediately after construction.
 */
uint32_t Test_Impl_PIM_GetPreinstallStateInitiallyNotStarted()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();

    WPEFramework::Exchange::IPreinstallManager::State state;
    const auto result = impl->GetPreinstallState(state);

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
                        "GetPreinstallState() returns ERROR_NONE");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(state),
                        static_cast<uint32_t>(WPEFramework::Exchange::IPreinstallManager::State::NOT_STARTED),
                        "Initial state is NOT_STARTED");

    impl->Release();
    return tr.failures;
}

/* PIM-POS-024 variant
 * GetPreinstallState() still returns NOT_STARTED after Configure() (no install
 * has been started).
 */
uint32_t Test_Impl_PIM_GetPreinstallStateAfterConfigureStillNotStarted()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();

    L0Test::ServiceMock::Config cfg;
    cfg.configLine = "{\"appPreinstallDirectory\":\"/tmp\"}";
    L0Test::ServiceMock service(cfg);
    impl->Configure(&service);

    WPEFramework::Exchange::IPreinstallManager::State state;
    impl->GetPreinstallState(state);

    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(state),
                        static_cast<uint32_t>(WPEFramework::Exchange::IPreinstallManager::State::NOT_STARTED),
                        "State remains NOT_STARTED after Configure() alone");

    impl->Release();
    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// getInstance / singleton (PIM-POS-025)
// ─────────────────────────────────────────────────────────────────────────────

/* PIM-POS-025
 * getInstance() returns the active instance during its lifetime.
 */
uint32_t Test_Impl_PIM_GetInstanceReturnsSelf()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();

    L0Test::ExpectTrue(tr,
                       WPEFramework::Plugin::PreinstallManagerImplementation::getInstance() == impl,
                       "getInstance() returns the active implementation");

    impl->Release();
    return tr.failures;
}

/* PIM-POS-025 variant
 * After Release() (which triggers the destructor) getInstance() returns nullptr.
 */
uint32_t Test_Impl_PIM_GetInstanceAfterDestructionReturnsNull()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    impl->Release(); // dtor sets _instance = nullptr

    L0Test::ExpectTrue(tr,
                       WPEFramework::Plugin::PreinstallManagerImplementation::getInstance() == nullptr,
                       "getInstance() returns nullptr after destruction");
    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// QueryInterface  (PIM-POS-011 via QueryInterface path)
// ─────────────────────────────────────────────────────────────────────────────

/* Verify that QueryInterface returns the requested interface with AddRef. */
uint32_t Test_Impl_PIM_QueryInterfaceIPreinstallManager()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();

    auto* iface = impl->QueryInterface<WPEFramework::Exchange::IPreinstallManager>();
    L0Test::ExpectTrue(tr, iface != nullptr,
                       "QueryInterface<IPreinstallManager> returns non-null");

    if (iface != nullptr) {
        iface->Release();
    }
    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_PIM_QueryInterfaceIConfiguration()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    auto* iface = impl->QueryInterface<WPEFramework::Exchange::IConfiguration>();

    L0Test::ExpectTrue(tr, iface != nullptr,
                       "QueryInterface<IConfiguration> returns non-null");

    if (iface != nullptr) {
        iface->Release();
    }
    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_PIM_QueryInterfaceUnknownID()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();

    // Use a numeric ID that is certainly not registered.
    void* iface = impl->QueryInterface(0xDEADBEEFU);
    L0Test::ExpectTrue(tr, iface == nullptr,
                       "QueryInterface with unknown ID returns nullptr");

    impl->Release();
    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// Multiple notifications (PIM-POS-008 extended)
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_PIM_MultipleNotificationsRegisterUnregister()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();

    L0Test::FakePreinstallNotification notif1;
    L0Test::FakePreinstallNotification notif2;
    L0Test::FakePreinstallNotification notif3;

    L0Test::ExpectEqU32(tr, impl->Register(&notif1), WPEFramework::Core::ERROR_NONE,
                        "Register notif1");
    L0Test::ExpectEqU32(tr, impl->Register(&notif2), WPEFramework::Core::ERROR_NONE,
                        "Register notif2");
    L0Test::ExpectEqU32(tr, impl->Register(&notif3), WPEFramework::Core::ERROR_NONE,
                        "Register notif3");

    L0Test::ExpectEqU32(tr, impl->Unregister(&notif2), WPEFramework::Core::ERROR_NONE,
                        "Unregister notif2 succeeds");
    L0Test::ExpectEqU32(tr, impl->Unregister(&notif2), WPEFramework::Core::ERROR_GENERAL,
                        "Unregister notif2 again returns ERROR_GENERAL");

    impl->Unregister(&notif1);
    impl->Unregister(&notif3);
    impl->Release();
    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// StartPreinstall failure paths (PIM-NEG-007, PIM-NEG-008, PIM-NEG-011)
// ─────────────────────────────────────────────────────────────────────────────

/* PIM-NEG-007 / PIM-NEG-011
 * StartPreinstall fails when no Configure() was called (mCurrentservice is null).
 * This exercises createPackageManagerObject with null service.
 */
uint32_t Test_Impl_PIM_StartPreinstallWithoutServiceFails()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    // No Configure() called → mCurrentservice is nullptr.

    const auto result = impl->StartPreinstall(true);

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "StartPreinstall returns ERROR_GENERAL when no service has been configured");

    WPEFramework::Exchange::IPreinstallManager::State state;
    impl->GetPreinstallState(state);
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(state),
                        static_cast<uint32_t>(WPEFramework::Exchange::IPreinstallManager::State::NOT_STARTED),
                        "State remains NOT_STARTED after failed StartPreinstall");

    impl->Release();
    return tr.failures;
}

/* PIM-NEG-008 / PIM-NEG-011
 * StartPreinstall fails when QueryInterfaceByCallsign returns null for
 * IPackageInstaller (no installer plug-in is available).
 */
uint32_t Test_Impl_PIM_StartPreinstallWithNoInstallerFails()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();

    // ServiceMock configured with NO installer (cfg.installer == nullptr).
    L0Test::ServiceMock::Config cfg;
    cfg.configLine = "{\"appPreinstallDirectory\":\"/nonexistent_dir_xyz\"}";
    L0Test::ServiceMock service(cfg); // no installer registered
    impl->Configure(&service);

    const auto result = impl->StartPreinstall(true);

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "StartPreinstall returns ERROR_GENERAL when IPackageInstaller is unavailable");

    impl->Release();
    return tr.failures;
}
