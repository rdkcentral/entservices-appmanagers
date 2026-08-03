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
#include <list>
#include <sys/stat.h>
#include <unistd.h>

#include "PreinstallManagerImplementation.h"
#include <interfaces/IPreinstallManager.h>
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

    void* raw = impl->QueryInterface(WPEFramework::Exchange::IPreinstallManager::ID);
    auto* iface = static_cast<WPEFramework::Exchange::IPreinstallManager*>(raw);
    L0Test::ExpectTrue(tr, iface != nullptr,
                       "QueryInterface(IPreinstallManager::ID) returns non-null");

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

    void* raw = impl->QueryInterface(WPEFramework::Exchange::IConfiguration::ID);
    auto* iface = static_cast<WPEFramework::Exchange::IConfiguration*>(raw);
    L0Test::ExpectTrue(tr, iface != nullptr,
                       "QueryInterface(IConfiguration::ID) returns non-null");

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

// ─────────────────────────────────────────────────────────────────────────────
// isNewerVersion — version comparison unit tests (PIM-POS-015, PIM-NEG-010, PIM-BND-004)
//
// isNewerVersion is private; driven via StartPreinstall(force=false):
//   - Install called   → v1 was treated as newer than v2
//   - Install not called → v1 was equal/older, or either string was invalid
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/* Runs StartPreinstall(force=false) with one preinstall package (version=preinstallVer)
 * against one installed package (version=installedVer) and returns installCallCount. */
uint32_t RunVersionComparison(const std::string& preinstallVer, const std::string& installedVer)
{
    char tmpPath[256];
    std::snprintf(tmpPath, sizeof(tmpPath), "/tmp/pim_ver_XXXXXX");
    if (!mkdtemp(tmpPath)) return 0;

    std::string subDir = std::string(tmpPath) + "/myapp";
    mkdir(subDir.c_str(), 0755);

    L0Test::FakePackageInstaller installer;

    installer.getConfigHandler = [preinstallVer](const std::string&, std::string& id,
                                                  std::string& version,
                                                  WPEFramework::Exchange::RuntimeConfig&) {
        id      = "myapp";
        version = preinstallVer;
        return WPEFramework::Core::ERROR_NONE;
    };

    installer.listPackagesHandler = [installedVer](WPEFramework::Exchange::IPackageInstaller::IPackageIterator*& packages) {
        std::list<WPEFramework::Exchange::IPackageInstaller::Package> pkgs;
        WPEFramework::Exchange::IPackageInstaller::Package pkg;
        pkg.packageId = "myapp";
        pkg.version   = installedVer;
        pkg.state     = WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLED;
        pkgs.push_back(pkg);
        packages = L0Test::MakePackageIterator(pkgs);
        return WPEFramework::Core::ERROR_NONE;
    };

    L0Test::ServiceMock::Config cfg(&installer);
    cfg.configLine = std::string("{\"appPreinstallDirectory\":\"") + tmpPath + "\"}"; 
    L0Test::ServiceMock service(cfg);

    auto* impl = CreateImpl();
    impl->Configure(&service);
    impl->StartPreinstall(false);

    // Wait for completion (max 2s).
    WPEFramework::Exchange::IPreinstallManager::State state;
    for (int i = 0; i < 200; ++i) {
        impl->GetPreinstallState(state);
        if (state == WPEFramework::Exchange::IPreinstallManager::State::COMPLETED) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    uint32_t calls = installer.installCallCount.load();
    impl->Release();

    rmdir(subDir.c_str());
    rmdir(tmpPath);
    return calls;
}

} // namespace

/* PIM-POS-015 — newer major is installed */
uint32_t Test_Impl_PIM_IsNewerVersion_NewerMajor()
{
    L0Test::TestResult tr;
    L0Test::ExpectEqU32(tr, RunVersionComparison("2.0.0", "1.0.0"), 1U,
                        "2.0.0 > 1.0.0 → Install called");
    return tr.failures;
}

/* PIM-POS-015 — newer minor is installed */
uint32_t Test_Impl_PIM_IsNewerVersion_NewerMinor()
{
    L0Test::TestResult tr;
    L0Test::ExpectEqU32(tr, RunVersionComparison("1.3.0", "1.2.0"), 1U,
                        "1.3.0 > 1.2.0 → Install called");
    return tr.failures;
}

/* PIM-POS-015 — newer patch is installed */
uint32_t Test_Impl_PIM_IsNewerVersion_NewerPatch()
{
    L0Test::TestResult tr;
    L0Test::ExpectEqU32(tr, RunVersionComparison("1.2.4", "1.2.3"), 1U,
                        "1.2.4 > 1.2.3 → Install called");
    return tr.failures;
}

/* PIM-POS-015 — 4-component version: newer build */
uint32_t Test_Impl_PIM_IsNewerVersion_NewerBuild()
{
    L0Test::TestResult tr;
    L0Test::ExpectEqU32(tr, RunVersionComparison("1.2.3.2", "1.2.3.1"), 1U,
                        "1.2.3.2 > 1.2.3.1 → Install called");
    return tr.failures;
}

/* PIM-POS-015 — equal versions are not installed */
uint32_t Test_Impl_PIM_IsNewerVersion_EqualVersionsNotInstalled()
{
    L0Test::TestResult tr;
    L0Test::ExpectEqU32(tr, RunVersionComparison("1.2.3", "1.2.3"), 0U,
                        "1.2.3 == 1.2.3 → Install not called");
    return tr.failures;
}

/* PIM-POS-015 — older version is not installed */
uint32_t Test_Impl_PIM_IsNewerVersion_OlderVersionNotInstalled()
{
    L0Test::TestResult tr;
    L0Test::ExpectEqU32(tr, RunVersionComparison("1.0.0", "2.0.0"), 0U,
                        "1.0.0 < 2.0.0 → Install not called");
    return tr.failures;
}

/* PIM-BND-004 — prerelease suffix is stripped before comparison */
uint32_t Test_Impl_PIM_IsNewerVersion_PrereleaseSuffixStripped()
{
    L0Test::TestResult tr;
    L0Test::ExpectEqU32(tr, RunVersionComparison("2.0.0-beta", "1.9.9"), 1U,
                        "2.0.0-beta strips to 2.0.0 > 1.9.9 → Install called");
    return tr.failures;
}

/* PIM-BND-004 — build metadata suffix is stripped */
uint32_t Test_Impl_PIM_IsNewerVersion_BuildMetadataSuffixStripped()
{
    L0Test::TestResult tr;
    L0Test::ExpectEqU32(tr, RunVersionComparison("1.2.3+build99", "1.2.2"), 1U,
                        "1.2.3+build99 strips to 1.2.3 > 1.2.2 → Install called");
    return tr.failures;
}

/* PIM-NEG-010 — only 2 components (1.2) is invalid; treated as not-newer → no install */
uint32_t Test_Impl_PIM_IsNewerVersion_TwoComponentsInvalid()
{
    L0Test::TestResult tr;
    L0Test::ExpectEqU32(tr, RunVersionComparison("1.2", "1.0.0"), 0U,
                        "1.2 has only 2 components → invalid → Install not called");
    return tr.failures;
}

/* PIM-NEG-010 — trailing dot (1.2.3.) is invalid */
uint32_t Test_Impl_PIM_IsNewerVersion_TrailingDotInvalid()
{
    L0Test::TestResult tr;
    L0Test::ExpectEqU32(tr, RunVersionComparison("1.2.3.", "1.0.0"), 0U,
                        "1.2.3. has empty trailing token → invalid → Install not called");
    return tr.failures;
}

/* PIM-NEG-010 — double dot (1..2.3) is invalid */
uint32_t Test_Impl_PIM_IsNewerVersion_DoubleDotInvalid()
{
    L0Test::TestResult tr;
    L0Test::ExpectEqU32(tr, RunVersionComparison("1..2.3", "1.0.0"), 0U,
                        "1..2.3 has empty token → invalid → Install not called");
    return tr.failures;
}

/* PIM-NEG-010 — alpha characters in component (1.2.3a) is invalid */
uint32_t Test_Impl_PIM_IsNewerVersion_AlphaComponentInvalid()
{
    L0Test::TestResult tr;
    L0Test::ExpectEqU32(tr, RunVersionComparison("1.2.3a", "1.0.0"), 0U,
                        "1.2.3a has trailing char in token → invalid → Install not called");
    return tr.failures;
}

/* PIM-NEG-010 — 5 components (1.2.3.4.5) is invalid */
uint32_t Test_Impl_PIM_IsNewerVersion_FiveComponentsInvalid()
{
    L0Test::TestResult tr;
    L0Test::ExpectEqU32(tr, RunVersionComparison("1.2.3.4.5", "1.0.0"), 0U,
                        "1.2.3.4.5 has 5 components → invalid → Install not called");
    return tr.failures;
}
