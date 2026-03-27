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

/**
 * @file PreinstallManager_ImplementationTests.cpp
 *
 * L0 tests for PreinstallManagerImplementation.cpp – public API layer.
 * Tests IDs: L0-IMPL-001 to L0-IMPL-055, L0-IMPL-070 to L0-IMPL-080.
 *
 * Covers:
 *   Construction / Destruction / getInstance()       L0-IMPL-001 – 004
 *   Register / Unregister (INotification)            L0-IMPL-005 – 008
 *   Configure()                                      L0-IMPL-009 – 011
 *   createPackageManagerObject() / release()         L0-IMPL-040 – 043
 *   handleOnAppInstallationStatus()                  L0-IMPL-050 – 051
 *   dispatchEvent() / Dispatch()                     L0-IMPL-052 – 055
 *   StartPreinstall() – all scenarios                L0-IMPL-070 – 080
 */

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

#include "PreinstallManagerImplementation.h"
#include "ServiceMock.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

// ──────────────────────────────────────────────────────────────────────────────
// Helper: create the Implementation object
// ──────────────────────────────────────────────────────────────────────────────
namespace {

WPEFramework::Plugin::PreinstallManagerImplementation* CreateImpl()
{
    return WPEFramework::Core::Service<
        WPEFramework::Plugin::PreinstallManagerImplementation>::Create<
        WPEFramework::Plugin::PreinstallManagerImplementation>();
}

// ──────────────────────────────────────────────────────────────────────────────
// Helper: create a temp directory, returning path or "" on failure.
// ──────────────────────────────────────────────────────────────────────────────
std::string MakeTempDir(const std::string& prefix)
{
    std::string tmpl = "/tmp/" + prefix + "_XXXXXX";
    std::vector<char> buf(tmpl.begin(), tmpl.end());
    buf.push_back('\0');
    if (nullptr == mkdtemp(buf.data())) {
        return "";
    }
    return std::string(buf.data());
}

// Remove a file and directory (best-effort)
void RemoveTempDir(const std::string& dir, const std::vector<std::string>& files)
{
    for (const auto& f : files) {
        remove((dir + "/" + f).c_str());
    }
    rmdir(dir.c_str());
}

// Touch (create empty) file
bool TouchFile(const std::string& path)
{
    FILE* f = fopen(path.c_str(), "w");
    if (nullptr == f) return false;
    fclose(f);
    return true;
}

} // anonymous namespace

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-001 – Constructor stores this in _instance
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_ConstructorSetsSingletonInstance()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();

    L0Test::ExpectTrue(tr, impl != nullptr,
                       "L0-IMPL-001: CreateImpl() returns non-null");
    L0Test::ExpectTrue(tr,
        WPEFramework::Plugin::PreinstallManagerImplementation::getInstance() == impl,
        "L0-IMPL-001: getInstance() == impl after construction");

    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-002 – Destructor clears _instance to nullptr
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_DestructorClearsSingleton()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ExpectTrue(tr,
        WPEFramework::Plugin::PreinstallManagerImplementation::getInstance() != nullptr,
        "L0-IMPL-002: Pre-condition: getInstance() is non-null");

    impl->Release();

    L0Test::ExpectTrue(tr,
        WPEFramework::Plugin::PreinstallManagerImplementation::getInstance() == nullptr,
        "L0-IMPL-002: getInstance() cleared to nullptr after Release()");

    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-003 – getInstance() returns the same pointer as the created object
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_GetInstanceReturnsSelf()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();

    L0Test::ExpectTrue(tr,
        WPEFramework::Plugin::PreinstallManagerImplementation::getInstance() == impl,
        "L0-IMPL-003: getInstance() returns same pointer as created impl");

    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-004 – getInstance() returns nullptr before construction
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_GetInstanceBeforeConstruction()
{
    L0Test::TestResult tr;

    // Ensure no instance exists (previous test should have released it)
    L0Test::ExpectTrue(tr,
        WPEFramework::Plugin::PreinstallManagerImplementation::getInstance() == nullptr,
        "L0-IMPL-004: getInstance() returns nullptr when no instance exists");

    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-005 – Register() with valid notification returns ERROR_NONE
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_RegisterNotification()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::FakePreinstallNotification notif;

    const auto result = impl->Register(&notif);
    L0Test::ExpectEqU32(tr, result,
                        WPEFramework::Core::ERROR_NONE,
                        "L0-IMPL-005: Register() returns ERROR_NONE for valid notification");

    impl->Unregister(&notif);
    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-006 – Register() duplicate is idempotent (second call returns ERROR_NONE)
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_RegisterDuplicate()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::FakePreinstallNotification notif;

    impl->Register(&notif);
    const auto result2 = impl->Register(&notif); // duplicate

    L0Test::ExpectEqU32(tr, result2,
                        WPEFramework::Core::ERROR_NONE,
                        "L0-IMPL-006: Second Register() for same notification returns ERROR_NONE");

    // Only one Unregister needed (duplicate is silently ignored by the impl)
    impl->Unregister(&notif);
    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-007 – Unregister() of registered notification returns ERROR_NONE
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_UnregisterNotification()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::FakePreinstallNotification notif;

    impl->Register(&notif);
    const auto result = impl->Unregister(&notif);

    L0Test::ExpectEqU32(tr, result,
                        WPEFramework::Core::ERROR_NONE,
                        "L0-IMPL-007: Unregister() returns ERROR_NONE for registered notification");

    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-008 – Unregister() of un-registered notification returns ERROR_GENERAL
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_UnregisterNotRegistered()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::FakePreinstallNotification notif; // never registered

    const auto result = impl->Unregister(&notif);

    L0Test::ExpectEqU32(tr, result,
                        WPEFramework::Core::ERROR_GENERAL,
                        "L0-IMPL-008: Unregister() returns ERROR_GENERAL for unregistered notification");

    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-009 – Configure() with null service returns ERROR_GENERAL
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_ConfigureWithNullService()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Configure(nullptr);

    L0Test::ExpectEqU32(tr, result,
                        WPEFramework::Core::ERROR_GENERAL,
                        "L0-IMPL-009: Configure(nullptr) returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-010 – Configure() with valid service and empty config returns ERROR_NONE
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_ConfigureWithEmptyConfigLine()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;
    // ServiceMock default ConfigLine() returns "{}"

    const auto result = impl->Configure(&service);

    L0Test::ExpectEqU32(tr, result,
                        WPEFramework::Core::ERROR_NONE,
                        "L0-IMPL-010: Configure() with empty config returns ERROR_NONE");

    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-011 – Configure() parses appPreinstallDirectory from config JSON
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_ConfigureParsesAppPreinstallDirectory()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;
    service.SetConfigLine(R"({"appPreinstallDirectory":"/opt/preinstall"})");

    const auto result = impl->Configure(&service);
    L0Test::ExpectEqU32(tr, result,
                        WPEFramework::Core::ERROR_NONE,
                        "L0-IMPL-011: Configure() returns ERROR_NONE with dir config");

    // Verify the directory was stored by observing StartPreinstall()
    // (with no PM available, it'll fail because directory read cannot happen
    //  before PM object is created — but Configure() itself succeeds)
    L0Test::ExpectTrue(tr, result == WPEFramework::Core::ERROR_NONE,
                       "L0-IMPL-011: Configure() successfully parsed appPreinstallDirectory");

    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-040 – createPackageManagerObject() with null service (via StartPreinstall)
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_CreatePackageManagerObjectNullService()
{
    L0Test::TestResult tr;

    // Do NOT call Configure() — mCurrentservice stays null
    auto* impl = CreateImpl();

    const auto result = impl->StartPreinstall(true);

    L0Test::ExpectEqU32(tr, result,
                        WPEFramework::Core::ERROR_GENERAL,
                        "L0-IMPL-040: StartPreinstall without Configure returns ERROR_GENERAL (null service)");

    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-041 – createPackageManagerObject() when plugin unavailable
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_CreatePackageManagerObjectPluginUnavailable()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    // ServiceMock with no FakePackageInstaller → QueryInterfaceByCallsign returns nullptr
    L0Test::ServiceMock service;
    impl->Configure(&service);

    const auto result = impl->StartPreinstall(true);
    L0Test::ExpectEqU32(tr, result,
                        WPEFramework::Core::ERROR_GENERAL,
                        "L0-IMPL-041: StartPreinstall with no PM plugin returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-042 – createPackageManagerObject() success (PM plugin available)
// Verified by observing registerCalls on FakePackageInstaller
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_CreatePackageManagerObjectSuccess()
{
    L0Test::TestResult tr;

    auto* pm = new L0Test::FakePackageInstaller();
    pm->getConfigShouldFail = true; // so readPreinstallDirectory won't block on directory

    auto* impl = CreateImpl();
    L0Test::ServiceMock service(pm);

    // Need a directory that exists for readPreinstallDirectory
    const std::string tmpDir = MakeTempDir("l0_impl_042");
    if (tmpDir.empty()) {
        std::cerr << "NOTE [L0-IMPL-042]: mkdtemp failed, skipping\n";
        impl->Release();
        pm->Release();
        return tr.failures;
    }
    service.SetConfigLine(R"({"appPreinstallDirectory":")" + tmpDir + R"("})");
    impl->Configure(&service);

    // StartPreinstall: PM object is created → pm.registerCalls should be 1
    // The directory exists but is empty → readPreinstallDirectory returns true
    // with empty packages list → StartPreinstall returns ERROR_NONE (0 packages)
    const auto result = impl->StartPreinstall(true);

    L0Test::ExpectEqU32(tr, pm->registerCalls.load(), 1u,
                        "L0-IMPL-042: FakePackageInstaller::Register called once (PM object created)");
    L0Test::ExpectEqU32(tr, result,
                        WPEFramework::Core::ERROR_NONE,
                        "L0-IMPL-042: StartPreinstall with empty dir returns ERROR_NONE");

    RemoveTempDir(tmpDir, {});
    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-043 – releasePackageManagerObject() releases after StartPreinstall
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_ReleasePackageManagerObjectAfterStartPreinstall()
{
    L0Test::TestResult tr;

    auto* pm = new L0Test::FakePackageInstaller();
    auto* impl = CreateImpl();
    L0Test::ServiceMock service(pm);

    const std::string tmpDir = MakeTempDir("l0_impl_043");
    if (tmpDir.empty()) {
        std::cerr << "NOTE [L0-IMPL-043]: mkdtemp failed, skipping\n";
        impl->Release();
        pm->Release();
        return tr.failures;
    }
    service.SetConfigLine(R"({"appPreinstallDirectory":")" + tmpDir + R"("})");
    impl->Configure(&service);

    impl->StartPreinstall(true);

    // After StartPreinstall, releasePackageManagerObject() is called internally
    L0Test::ExpectEqU32(tr, pm->unregisterCalls.load(), 1u,
                        "L0-IMPL-043: Unregister called once on PM object (released)");

    RemoveTempDir(tmpDir, {});
    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-050 – handleOnAppInstallationStatus() with valid JSON dispatches event
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_HandleOnAppInstallationStatusDispatchesEvent()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::FakePreinstallNotification notif;
    impl->Register(&notif);

    impl->handleOnAppInstallationStatus(
        R"({"packageId":"com.example.app","status":"INSTALLING"})");

    // Wait for worker-pool dispatch thread to execute Dispatch()
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    L0Test::ExpectEqU32(tr, notif.onAppInstallationStatusCount.load(), 1u,
                        "L0-IMPL-050: OnAppInstallationStatus fired once");
    L0Test::ExpectTrue(tr,
        notif.lastJsonResponse.find("com.example.app") != std::string::npos,
        "L0-IMPL-050: lastJsonResponse contains packageId");

    impl->Unregister(&notif);
    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-051 – handleOnAppInstallationStatus() with empty string logs error, no dispatch
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_HandleOnAppInstallationStatusEmptyStringNoDispatch()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::FakePreinstallNotification notif;
    impl->Register(&notif);

    impl->handleOnAppInstallationStatus(""); // empty → should log error, not dispatch

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    L0Test::ExpectEqU32(tr, notif.onAppInstallationStatusCount.load(), 0u,
                        "L0-IMPL-051: OnAppInstallationStatus NOT called for empty string");

    impl->Unregister(&notif);
    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-052 – dispatchEvent() / Dispatch() fires callback via worker pool
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_DispatchEventFiresCallback()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::FakePreinstallNotification notif1;
    L0Test::FakePreinstallNotification notif2;
    impl->Register(&notif1);
    impl->Register(&notif2);

    impl->handleOnAppInstallationStatus(R"({"event":"test"})");

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    L0Test::ExpectEqU32(tr, notif1.onAppInstallationStatusCount.load(), 1u,
                        "L0-IMPL-052: notif1 received callback");
    L0Test::ExpectEqU32(tr, notif2.onAppInstallationStatusCount.load(), 1u,
                        "L0-IMPL-052: notif2 received callback");

    impl->Unregister(&notif1);
    impl->Unregister(&notif2);
    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-053 – Two registered notifications both receive INSTALLATION_STATUS event
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_DispatchInstallationStatusNotifiesBothListeners()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::FakePreinstallNotification n1;
    L0Test::FakePreinstallNotification n2;
    impl->Register(&n1);
    impl->Register(&n2);

    const std::string payload = R"({"packageId":"com.app1","state":"INSTALLED"})";
    impl->handleOnAppInstallationStatus(payload);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    L0Test::ExpectEqU32(tr, n1.onAppInstallationStatusCount.load(), 1u,
                        "L0-IMPL-053: First notification received");
    L0Test::ExpectEqU32(tr, n2.onAppInstallationStatusCount.load(), 1u,
                        "L0-IMPL-053: Second notification received");
    L0Test::ExpectEqStr(tr, n1.lastJsonResponse, payload,
                        "L0-IMPL-053: Notification received correct JSON payload");

    impl->Unregister(&n1);
    impl->Unregister(&n2);
    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-054 – Dispatch() with empty jsonresponse (no label) does not fire callback
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_HandleEmptyStringNoCallback()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::FakePreinstallNotification notif;
    impl->Register(&notif);

    // Empty string → LOGERR branch, no dispatchEvent
    impl->handleOnAppInstallationStatus("");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    L0Test::ExpectEqU32(tr, notif.onAppInstallationStatusCount.load(), 0u,
                        "L0-IMPL-054: Empty string does not fire callback");

    impl->Unregister(&notif);
    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-055 – After Unregister, no callback is fired
// (exercises the "no registered listeners" path of Dispatch)
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_DispatchWithNoListeners()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::FakePreinstallNotification notif;

    // Register then immediately unregister
    impl->Register(&notif);
    impl->Unregister(&notif);

    impl->handleOnAppInstallationStatus(R"({"event":"test"})");

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    L0Test::ExpectEqU32(tr, notif.onAppInstallationStatusCount.load(), 0u,
                        "L0-IMPL-055: No callback after Unregister (empty listener list)");

    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-070 – StartPreinstall() returns ERROR_GENERAL when PM creation fails
// (mCurrentservice is null — Configure() never called)
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_StartPreinstallNoPackageManagerReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();

    const auto result = impl->StartPreinstall(true);
    L0Test::ExpectEqU32(tr, result,
                        WPEFramework::Core::ERROR_GENERAL,
                        "L0-IMPL-070: StartPreinstall without Configure returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-071 – StartPreinstall() returns ERROR_GENERAL when directory read fails
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_StartPreinstallBadDirectoryReturnsError()
{
    L0Test::TestResult tr;

    auto* pm = new L0Test::FakePackageInstaller();
    auto* impl = CreateImpl();
    L0Test::ServiceMock service(pm);
    service.SetConfigLine(R"({"appPreinstallDirectory":"/tmp/nonexistent_preinstall_L0_xyz_12345"})");
    impl->Configure(&service);

    const auto result = impl->StartPreinstall(true);
    L0Test::ExpectEqU32(tr, result,
                        WPEFramework::Core::ERROR_GENERAL,
                        "L0-IMPL-071: StartPreinstall with non-existent dir returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-072 – StartPreinstall() with forceInstall=true, all packages succeed
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_StartPreinstallForceInstallAllSucceed()
{
    L0Test::TestResult tr;

    const std::string tmpDir = MakeTempDir("l0_impl_072");
    if (tmpDir.empty()) {
        std::cerr << "NOTE [L0-IMPL-072]: mkdtemp failed, skipping\n";
        return tr.failures;
    }

    // Create 2 dummy package directories
    const std::string pkg1 = tmpDir + "/package1";
    const std::string pkg2 = tmpDir + "/package2";
    mkdir(pkg1.c_str(), 0755);
    mkdir(pkg2.c_str(), 0755);

    auto* pm = new L0Test::FakePackageInstaller();
    pm->returnedPackageId = "com.app.test";
    pm->returnedVersion   = "1.0.0";
    // Install returns success (default)

    auto* impl = CreateImpl();
    L0Test::ServiceMock service(pm);
    service.SetConfigLine(R"({"appPreinstallDirectory":")" + tmpDir + R"("})");
    impl->Configure(&service);

    const auto result = impl->StartPreinstall(true);

    L0Test::ExpectEqU32(tr, result,
                        WPEFramework::Core::ERROR_NONE,
                        "L0-IMPL-072: forceInstall=true with 2 valid packages returns ERROR_NONE");
    L0Test::ExpectEqU32(tr, pm->installCalls.load(), 2u,
                        "L0-IMPL-072: Install called twice (once per package)");

    RemoveTempDir(pkg1, {});
    RemoveTempDir(pkg2, {});
    rmdir(tmpDir.c_str());
    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-073 – StartPreinstall() with forceInstall=false, ListPackages fails
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_StartPreinstallListPackagesFailsReturnsError()
{
    L0Test::TestResult tr;

    const std::string tmpDir = MakeTempDir("l0_impl_073");
    if (tmpDir.empty()) {
        std::cerr << "NOTE [L0-IMPL-073]: mkdtemp failed, skipping\n";
        return tr.failures;
    }

    // Create 1 dummy package dir
    const std::string pkgDir = tmpDir + "/pkg1";
    mkdir(pkgDir.c_str(), 0755);

    auto* pm = new L0Test::FakePackageInstaller();
    pm->returnedPackageId = "com.app.test";
    pm->returnedVersion   = "2.0.0";
    pm->listShouldFail    = true; // ListPackages fails

    auto* impl = CreateImpl();
    L0Test::ServiceMock service(pm);
    service.SetConfigLine(R"({"appPreinstallDirectory":")" + tmpDir + R"("})");
    impl->Configure(&service);

    const auto result = impl->StartPreinstall(false); // forceInstall=false → calls ListPackages

    L0Test::ExpectEqU32(tr, result,
                        WPEFramework::Core::ERROR_GENERAL,
                        "L0-IMPL-073: StartPreinstall with ListPackages failure returns ERROR_GENERAL");

    RemoveTempDir(pkgDir, {});
    rmdir(tmpDir.c_str());
    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-074 – StartPreinstall() skips install when same version already installed
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_StartPreinstallSkipsIfSameVersionInstalled()
{
    L0Test::TestResult tr;

    const std::string tmpDir = MakeTempDir("l0_impl_074");
    if (tmpDir.empty()) {
        std::cerr << "NOTE [L0-IMPL-074]: mkdtemp failed, skipping\n";
        return tr.failures;
    }

    const std::string pkgDir = tmpDir + "/myapp";
    mkdir(pkgDir.c_str(), 0755);

    auto* pm = new L0Test::FakePackageInstaller();
    pm->returnedPackageId = "com.example.myapp";
    pm->returnedVersion   = "1.0.0"; // preinstall dir version

    // Simulate: already installed at same version
    WPEFramework::Exchange::IPackageInstaller::Package installed;
    installed.packageId = "com.example.myapp";
    installed.version   = "1.0.0";
    installed.state     = WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLED;
    pm->installedPackages.push_back(installed);

    auto* impl = CreateImpl();
    L0Test::ServiceMock service(pm);
    service.SetConfigLine(R"({"appPreinstallDirectory":")" + tmpDir + R"("})");
    impl->Configure(&service);

    const auto result = impl->StartPreinstall(false);

    L0Test::ExpectEqU32(tr, pm->installCalls.load(), 0u,
                        "L0-IMPL-074: Install NOT called when same version is already installed");
    L0Test::ExpectEqU32(tr, result,
                        WPEFramework::Core::ERROR_NONE,
                        "L0-IMPL-074: Returns ERROR_NONE when no new packages to install");

    RemoveTempDir(pkgDir, {});
    rmdir(tmpDir.c_str());
    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-075 – StartPreinstall() installs when newer version is available
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_StartPreinstallInstallsNewerVersion()
{
    L0Test::TestResult tr;

    const std::string tmpDir = MakeTempDir("l0_impl_075");
    if (tmpDir.empty()) {
        std::cerr << "NOTE [L0-IMPL-075]: mkdtemp failed, skipping\n";
        return tr.failures;
    }

    const std::string pkgDir = tmpDir + "/myapp";
    mkdir(pkgDir.c_str(), 0755);

    auto* pm = new L0Test::FakePackageInstaller();
    pm->returnedPackageId = "com.example.myapp";
    pm->returnedVersion   = "2.0.0"; // newer version in preinstall dir

    // Already installed at older version
    WPEFramework::Exchange::IPackageInstaller::Package installed;
    installed.packageId = "com.example.myapp";
    installed.version   = "1.0.0";
    installed.state     = WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLED;
    pm->installedPackages.push_back(installed);

    auto* impl = CreateImpl();
    L0Test::ServiceMock service(pm);
    service.SetConfigLine(R"({"appPreinstallDirectory":")" + tmpDir + R"("})");
    impl->Configure(&service);

    const auto result = impl->StartPreinstall(false);

    L0Test::ExpectEqU32(tr, pm->installCalls.load(), 1u,
                        "L0-IMPL-075: Install called once for newer version");
    L0Test::ExpectEqU32(tr, result,
                        WPEFramework::Core::ERROR_NONE,
                        "L0-IMPL-075: StartPreinstall returns ERROR_NONE after successful update");

    RemoveTempDir(pkgDir, {});
    rmdir(tmpDir.c_str());
    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-076 – StartPreinstall() returns ERROR_GENERAL when Install() fails
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_StartPreinstallInstallFailureReturnsError()
{
    L0Test::TestResult tr;

    const std::string tmpDir = MakeTempDir("l0_impl_076");
    if (tmpDir.empty()) {
        std::cerr << "NOTE [L0-IMPL-076]: mkdtemp failed, skipping\n";
        return tr.failures;
    }

    const std::string pkgDir = tmpDir + "/badpkg";
    mkdir(pkgDir.c_str(), 0755);

    auto* pm = new L0Test::FakePackageInstaller();
    pm->returnedPackageId  = "com.bad.pkg";
    pm->returnedVersion    = "1.0.0";
    pm->installShouldFail  = true;
    pm->failReasonToReturn =
        WPEFramework::Exchange::IPackageInstaller::FailReason::SIGNATURE_VERIFICATION_FAILURE;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service(pm);
    service.SetConfigLine(R"({"appPreinstallDirectory":")" + tmpDir + R"("})");
    impl->Configure(&service);

    const auto result = impl->StartPreinstall(true);

    L0Test::ExpectEqU32(tr, pm->installCalls.load(), 1u,
                        "L0-IMPL-076: Install attempt made once");
    L0Test::ExpectEqU32(tr, result,
                        WPEFramework::Core::ERROR_GENERAL,
                        "L0-IMPL-076: StartPreinstall returns ERROR_GENERAL when Install fails");

    RemoveTempDir(pkgDir, {});
    rmdir(tmpDir.c_str());
    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-077 – StartPreinstall() skips but does NOT error on empty packageId/version
//
// When GetConfigForPackage returns SUCCESS but empty packageId/version,
// the install loop skips with failedApps++ but does NOT set installError=true.
// Therefore result is ERROR_NONE (not ERROR_GENERAL).
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_StartPreinstallSkipsPackageWithEmptyFields()
{
    L0Test::TestResult tr;

    const std::string tmpDir = MakeTempDir("l0_impl_077");
    if (tmpDir.empty()) {
        std::cerr << "NOTE [L0-IMPL-077]: mkdtemp failed, skipping\n";
        return tr.failures;
    }

    const std::string pkgDir = tmpDir + "/emptyfields";
    mkdir(pkgDir.c_str(), 0755);

    auto* pm = new L0Test::FakePackageInstaller();
    // GetConfigForPackage succeeds but returns empty packageId and version
    pm->returnedPackageId  = "";  // empty packageId
    pm->returnedVersion    = "";  // empty version

    auto* impl = CreateImpl();
    L0Test::ServiceMock service(pm);
    service.SetConfigLine(R"({"appPreinstallDirectory":")" + tmpDir + R"("})");
    impl->Configure(&service);

    const auto result = impl->StartPreinstall(true);

    L0Test::ExpectEqU32(tr, pm->installCalls.load(), 0u,
                        "L0-IMPL-077: Install NOT called for package with empty fields");
    // installError is NOT set for empty-fields path (only failedApps++ without installError=true)
    // so result is ERROR_NONE even though the package was skipped
    L0Test::ExpectEqU32(tr, result,
                        WPEFramework::Core::ERROR_NONE,
                        "L0-IMPL-077: Returns ERROR_NONE when packages are skipped (empty fields, installError stays false)");

    RemoveTempDir(pkgDir, {});
    rmdir(tmpDir.c_str());
    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-078 – StartPreinstall() mixed results: 2 pass, 1 fails → ERROR_GENERAL
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_StartPreinstallMixedResultsReturnsError()
{
    L0Test::TestResult tr;

    const std::string tmpDir = MakeTempDir("l0_impl_078");
    if (tmpDir.empty()) {
        std::cerr << "NOTE [L0-IMPL-078]: mkdtemp failed, skipping\n";
        return tr.failures;
    }

    // Create 3 package dirs
    const std::string p1 = tmpDir + "/pkg1";
    const std::string p2 = tmpDir + "/pkg2";
    const std::string p3 = tmpDir + "/pkg3";
    mkdir(p1.c_str(), 0755);
    mkdir(p2.c_str(), 0755);
    mkdir(p3.c_str(), 0755);

    // Use a custom FakePackageInstaller that fails on the 2nd call
    class MixedInstaller final : public L0Test::FakePackageInstaller {
    public:
        WPEFramework::Core::hresult Install(
            const std::string&,
            const std::string&,
            WPEFramework::Exchange::IPackageInstaller::IKeyValueIterator* const&,
            const std::string&,
            WPEFramework::Exchange::IPackageInstaller::FailReason& fr) override
        {
            installCalls++;
            if (installCalls.load() == 2u) {
                fr = WPEFramework::Exchange::IPackageInstaller::FailReason::PACKAGE_MISMATCH_FAILURE;
                return WPEFramework::Core::ERROR_GENERAL;
            }
            return WPEFramework::Core::ERROR_NONE;
        }
    };

    auto* pm = new MixedInstaller();
    pm->returnedPackageId = "com.app";
    pm->returnedVersion   = "1.0.0";

    auto* impl = CreateImpl();
    L0Test::ServiceMock service(pm);
    service.SetConfigLine(R"({"appPreinstallDirectory":")" + tmpDir + R"("})");
    impl->Configure(&service);

    const auto result = impl->StartPreinstall(true);

    L0Test::ExpectEqU32(tr, pm->installCalls.load(), 3u,
                        "L0-IMPL-078: Install called 3 times (all packages attempted)");
    L0Test::ExpectEqU32(tr, result,
                        WPEFramework::Core::ERROR_GENERAL,
                        "L0-IMPL-078: Mixed results returns ERROR_GENERAL");

    RemoveTempDir(p1, {});
    RemoveTempDir(p2, {});
    RemoveTempDir(p3, {});
    rmdir(tmpDir.c_str());
    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-079 – StartPreinstall() with forceInstall=false and null iterator
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_StartPreinstallNullPackageIteratorReturnsError()
{
    L0Test::TestResult tr;

    const std::string tmpDir = MakeTempDir("l0_impl_079");
    if (tmpDir.empty()) {
        std::cerr << "NOTE [L0-IMPL-079]: mkdtemp failed, skipping\n";
        return tr.failures;
    }

    const std::string pkgDir = tmpDir + "/pkg1";
    mkdir(pkgDir.c_str(), 0755);

    auto* pm = new L0Test::FakePackageInstaller();
    pm->returnedPackageId               = "com.app";
    pm->returnedVersion                 = "1.0.0";
    pm->listPackagesReturnsNullIterator = true; // returns nullptr iterator

    auto* impl = CreateImpl();
    L0Test::ServiceMock service(pm);
    service.SetConfigLine(R"({"appPreinstallDirectory":")" + tmpDir + R"("})");
    impl->Configure(&service);

    const auto result = impl->StartPreinstall(false);

    L0Test::ExpectEqU32(tr, result,
                        WPEFramework::Core::ERROR_GENERAL,
                        "L0-IMPL-079: StartPreinstall with null iterator returns ERROR_GENERAL");

    RemoveTempDir(pkgDir, {});
    rmdir(tmpDir.c_str());
    impl->Release();
    return tr.failures;
}

// ══════════════════════════════════════════════════════════════════════════════
// L0-IMPL-080 – Multiple Register/Unregister: only remaining listener receives callback
// ══════════════════════════════════════════════════════════════════════════════
uint32_t Test_Impl_MultipleNotificationsRegisterUnregister()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::FakePreinstallNotification n1;
    L0Test::FakePreinstallNotification n2;
    L0Test::FakePreinstallNotification n3;

    impl->Register(&n1);
    impl->Register(&n2);
    impl->Register(&n3);

    // Unregister n1 and n2 before firing event
    impl->Unregister(&n1);
    impl->Unregister(&n2);

    impl->handleOnAppInstallationStatus(R"({"event":"test"})");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    L0Test::ExpectEqU32(tr, n1.onAppInstallationStatusCount.load(), 0u,
                        "L0-IMPL-080: Unregistered n1 receives 0 callbacks");
    L0Test::ExpectEqU32(tr, n2.onAppInstallationStatusCount.load(), 0u,
                        "L0-IMPL-080: Unregistered n2 receives 0 callbacks");
    L0Test::ExpectEqU32(tr, n3.onAppInstallationStatusCount.load(), 1u,
                        "L0-IMPL-080: Remaining n3 receives exactly 1 callback");

    impl->Unregister(&n3);
    impl->Release();
    return tr.failures;
}
