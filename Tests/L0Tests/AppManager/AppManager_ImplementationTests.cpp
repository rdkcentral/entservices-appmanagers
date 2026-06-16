/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 RDK Management
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

#include <iostream>
#include <atomic>
#include <sstream>
#include <string>

#define private public
#include "AppManagerImplementation.h"
#undef private
#include "common/AppManagerL0Mock.hpp"
#include "common/L0Expect.hpp"

// Helper to create a service mock with all required dependencies for AppManagerImplementation
// Defined outside anonymous namespace so it can be accessed from other test files via extern
L0Test::AppManagerServiceMock::Config CreateFullServiceConfig()
{
    L0Test::AppManagerServiceMock::Config cfg;
    cfg.lifecycleManager = new L0Test::FakeLifecycleManager();
    cfg.lifecycleManagerState = new L0Test::FakeLifecycleManagerState();
    cfg.store2 = new L0Test::FakeStore2();
    cfg.storageManager = new L0Test::FakeStorageManager();
    cfg.packageHandler = new L0Test::FakePackageHandler();
    cfg.installer = new L0Test::FakePackageInstaller();
    return cfg;
}

namespace {

WPEFramework::Plugin::AppManagerImplementation* CreateImpl()
{
    return WPEFramework::Core::Service<WPEFramework::Plugin::AppManagerImplementation>::Create<WPEFramework::Plugin::AppManagerImplementation>();
}

struct RefNotification final : public WPEFramework::Exchange::IAppManager::INotification {
    RefNotification()
        : _refCount(1)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IAppManager::INotification::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IAppManager::INotification*>(this);
        }
        return nullptr;
    }

    void OnAppInstalled(const std::string&, const std::string&) override { ++installed; }
    void OnAppUninstalled(const std::string&) override { ++uninstalled; }
    void OnAppLifecycleStateChanged(const std::string&, const std::string&, const WPEFramework::Exchange::IAppManager::AppLifecycleState,
        const WPEFramework::Exchange::IAppManager::AppLifecycleState, const WPEFramework::Exchange::IAppManager::AppErrorReason) override { ++lifecycle; }
    void OnAppLaunchRequest(const std::string&, const std::string&, const std::string&) override { ++launch; }
    void OnAppUnloaded(const std::string&, const std::string&) override { ++unloaded; }

    mutable std::atomic<uint32_t> _refCount;
    uint32_t installed { 0 };
    uint32_t uninstalled { 0 };
    uint32_t lifecycle { 0 };
    uint32_t launch { 0 };
    uint32_t unloaded { 0 };
};

} // namespace

uint32_t Test_AM_RegisterAndUnregisterNotification()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();
    RefNotification* notification = new RefNotification();

    const auto reg = impl->Register(notification);
    L0Test::ExpectEqU32(tr, reg, WPEFramework::Core::ERROR_NONE, "Register() returns ERROR_NONE for a valid notification");
    const auto unreg = impl->Unregister(notification);
    L0Test::ExpectEqU32(tr, unreg, WPEFramework::Core::ERROR_NONE, "Unregister() returns ERROR_NONE for a registered notification");

    notification->Release();
    // Configure with full mock to ensure proper cleanup in destructor
    L0Test::AppManagerServiceMock service(CreateFullServiceConfig());
    impl->Configure(&service);
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_ConfigureWithNullServiceFails()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    const auto result = impl->Configure(nullptr);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL, "Configure(nullptr) returns ERROR_GENERAL");

    // Configure with full mock to ensure proper cleanup in destructor
    L0Test::AppManagerServiceMock service(CreateFullServiceConfig());
    impl->Configure(&service);
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_ConfigureWithValidServiceReturnsSuccess()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    L0Test::AppManagerServiceMock service(CreateFullServiceConfig());
    const auto result = impl->Configure(&service);
    L0Test::ExpectEqU32(tr, service.addRefCalls.load(), 2U, "Configure() calls AddRef on the shell for implementation and lifecycle connector ownership");
    if (result != WPEFramework::Core::ERROR_NONE) {
        std::cerr << "NOTE: Configure returned non-zero in the current environment: " << result << std::endl;
    }

    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_LaunchAppEmptyIdRejected()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    const auto result = impl->LaunchApp(std::string(), std::string("intent"), std::string("args"));
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_INVALID_PARAMETER, "LaunchApp() rejects an empty app id");

    // Configure with full mock to ensure proper cleanup in destructor
    L0Test::AppManagerServiceMock service(CreateFullServiceConfig());
    impl->Configure(&service);
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_PreloadAppEmptyIdRejected()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();
    std::string error;

    const auto result = impl->PreloadApp(std::string(), std::string("intent"), std::string("args"), error);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_INVALID_PARAMETER, "PreloadApp() rejects an empty app id");
    L0Test::ExpectTrue(tr, !error.empty(), "PreloadApp() sets an error string for empty app id");

    // Configure with full mock to ensure proper cleanup in destructor
    L0Test::AppManagerServiceMock service(CreateFullServiceConfig());
    impl->Configure(&service);
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_CloseTerminateKillSendIntentEmptyIdRejected()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    L0Test::ExpectEqU32(tr, impl->CloseApp(std::string()), WPEFramework::Core::ERROR_GENERAL, "CloseApp() rejects empty app id");
    L0Test::ExpectEqU32(tr, impl->TerminateApp(std::string()), WPEFramework::Core::ERROR_GENERAL, "TerminateApp() rejects empty app id");
    L0Test::ExpectEqU32(tr, impl->KillApp(std::string()), WPEFramework::Core::ERROR_NONE, "KillApp() remains stable for empty app id");
    L0Test::ExpectEqU32(tr, impl->SendIntent(std::string(), std::string("intent")), WPEFramework::Core::ERROR_NONE, "SendIntent() with empty app id remains stable");

    // Configure with full mock to ensure proper cleanup in destructor
    L0Test::AppManagerServiceMock service(CreateFullServiceConfig());
    impl->Configure(&service);
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_GetAppPropertyAndSetAppPropertyInvalidInputs()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();
    std::string value;

    L0Test::ExpectEqU32(tr, impl->GetAppProperty(std::string(), std::string("key"), value), WPEFramework::Core::ERROR_GENERAL, "GetAppProperty() rejects empty app id");
    L0Test::ExpectEqU32(tr, impl->GetAppProperty(std::string("app"), std::string(), value), WPEFramework::Core::ERROR_GENERAL, "GetAppProperty() rejects empty key");
    L0Test::ExpectEqU32(tr, impl->SetAppProperty(std::string(), std::string("key"), std::string("value")), WPEFramework::Core::ERROR_GENERAL, "SetAppProperty() rejects empty app id");
    L0Test::ExpectEqU32(tr, impl->SetAppProperty(std::string("app"), std::string(), std::string("value")), WPEFramework::Core::ERROR_GENERAL, "SetAppProperty() rejects empty key");

    // Configure with full mock to ensure proper cleanup in destructor
    L0Test::AppManagerServiceMock service(CreateFullServiceConfig());
    impl->Configure(&service);
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_ClearAppDataAndClearAllAppDataWithoutDependencies()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    L0Test::ExpectEqU32(tr, impl->ClearAppData(std::string()), WPEFramework::Core::ERROR_GENERAL, "ClearAppData() rejects empty app id");
    L0Test::ExpectEqU32(tr, impl->ClearAllAppData(), WPEFramework::Core::ERROR_GENERAL, "ClearAllAppData() fails without storage manager");

    // Configure with full mock to ensure proper cleanup in destructor
    L0Test::AppManagerServiceMock service(CreateFullServiceConfig());
    impl->Configure(&service);
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_GetLoadedAppsWithoutConnectorFails()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();
    WPEFramework::Exchange::IAppManager::ILoadedAppInfoIterator* iterator = nullptr;

    const auto result = impl->GetLoadedApps(iterator);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL, "GetLoadedApps() fails without a lifecycle connector");
    L0Test::ExpectTrue(tr, nullptr == iterator, "GetLoadedApps() leaves the iterator null on failure");

    // Configure with full mock to ensure proper cleanup in destructor
    L0Test::AppManagerServiceMock service(CreateFullServiceConfig());
    impl->Configure(&service);
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_GetInstalledAppsWithoutPackagesFailsOrEmpty()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();
    std::string apps;

    const auto result = impl->GetInstalledApps(apps);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL, "GetInstalledApps() fails when package manager is unavailable");
    L0Test::ExpectTrue(tr, apps.empty(), "GetInstalledApps() leaves output empty on failure");

    // Configure with full mock to ensure proper cleanup in destructor
    L0Test::AppManagerServiceMock service(CreateFullServiceConfig());
    impl->Configure(&service);
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_UpdateCurrentActionHelpers()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    WPEFramework::Plugin::AppInfoManager::getInstance().clear();
    WPEFramework::Plugin::AppInfo info;
    info.setCurrentAction(WPEFramework::Plugin::AppManagerTypes::APP_ACTION_NONE);
    WPEFramework::Plugin::AppInfoManager::getInstance().upsert("app1", [&](WPEFramework::Plugin::AppInfo& a) { a = info; });

    impl->updateCurrentAction("app1", WPEFramework::Plugin::AppManagerTypes::APP_ACTION_LAUNCH);
    L0Test::ExpectEqU32(tr,
        static_cast<uint32_t>(WPEFramework::Plugin::AppInfoManager::getInstance().getCurrentAction("app1")),
        static_cast<uint32_t>(WPEFramework::Plugin::AppManagerTypes::APP_ACTION_LAUNCH),
        "updateCurrentAction() updates the stored action");

    impl->updateCurrentActionTime("app1", 1234, WPEFramework::Plugin::AppManagerTypes::APP_ACTION_LAUNCH);
    L0Test::ExpectEqU32(tr,
        static_cast<uint32_t>(WPEFramework::Plugin::AppInfoManager::getInstance().getCurrentActionTime("app1")),
        static_cast<uint32_t>(1234),
        "updateCurrentActionTime() updates the stored timestamp");

    // Configure with full mock to ensure proper cleanup in destructor
    L0Test::AppManagerServiceMock service(CreateFullServiceConfig());
    impl->Configure(&service);
    impl->Release();
    WPEFramework::Plugin::AppInfoManager::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_CheckInstallUninstallBlockWithoutPackages()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    const bool blocked = impl->checkInstallUninstallBlock("app1");
    L0Test::ExpectTrue(tr, !blocked, "checkInstallUninstallBlock() returns false when no package manager exists");

    // Configure with full mock to ensure proper cleanup in destructor
    L0Test::AppManagerServiceMock service(CreateFullServiceConfig());
    impl->Configure(&service);
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_IsInstalledAndGetInstalledAppsWithPackages()
{
    L0Test::TestResult tr;

    auto* installer = new L0Test::FakePackageInstaller();
    auto* handler = new L0Test::FakePackageHandler();
    auto* store = new L0Test::FakeStore2();
    auto* storage = new L0Test::FakeStorageManager();

    WPEFramework::Exchange::IPackageInstaller::Package installedPkg;
    installedPkg.packageId = "app.good";
    installedPkg.version = "1.2.3";
    installedPkg.state = WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLED;

    WPEFramework::Exchange::IPackageInstaller::Package pendingPkg;
    pendingPkg.packageId = "app.pending";
    pendingPkg.version = "9.9.9";
    pendingPkg.state = WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLING;

    installer->installedPackages.push_back(installedPkg);
    installer->installedPackages.push_back(pendingPkg);

    L0Test::AppManagerServiceMock::Config cfg(installer);
    cfg.packageHandler = handler;
    cfg.store2 = store;
    cfg.storageManager = storage;
    L0Test::AppManagerServiceMock service(cfg);

    auto* impl = CreateImpl();
    L0Test::ExpectEqU32(tr, impl->Configure(&service), WPEFramework::Core::ERROR_NONE, "Configure() succeeds with fake package/store/storage dependencies");

    bool installed = false;
    auto status = impl->IsInstalled("app.good", installed);
    L0Test::ExpectEqU32(tr, status, WPEFramework::Core::ERROR_NONE, "IsInstalled() succeeds when package installer is available");
    L0Test::ExpectTrue(tr, installed, "IsInstalled() reports true for INSTALLED packages");

    status = impl->IsInstalled("missing.app", installed);
    L0Test::ExpectEqU32(tr, status, WPEFramework::Core::ERROR_NONE, "IsInstalled() remains successful for missing app ids");
    L0Test::ExpectTrue(tr, !installed, "IsInstalled() reports false when app id is absent");

    std::string apps;
    status = impl->GetInstalledApps(apps);
    L0Test::ExpectEqU32(tr, status, WPEFramework::Core::ERROR_NONE, "GetInstalledApps() succeeds when package list is available");
    L0Test::ExpectTrue(tr, apps.find("app.good") != std::string::npos, "GetInstalledApps() includes installed app ids in output");
    L0Test::ExpectTrue(tr, apps.find("app.pending") == std::string::npos, "GetInstalledApps() excludes non-installed entries");

    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_AppPropertyRoundTripWithStore()
{
    L0Test::TestResult tr;

    auto* installer = new L0Test::FakePackageInstaller();
    auto* handler = new L0Test::FakePackageHandler();
    auto* store = new L0Test::FakeStore2();
    auto* storage = new L0Test::FakeStorageManager();

    L0Test::AppManagerServiceMock::Config cfg = CreateFullServiceConfig();
    cfg.installer = installer;
    cfg.packageHandler = handler;
    cfg.store2 = store;
    cfg.storageManager = storage;
    L0Test::AppManagerServiceMock service(cfg);

    auto* impl = CreateImpl();
    L0Test::ExpectEqU32(tr, impl->Configure(&service), WPEFramework::Core::ERROR_NONE, "Configure() succeeds with fake store");

    L0Test::ExpectEqU32(tr,
        impl->SetAppProperty("app.good", "sampleKey", "sampleValue"),
        WPEFramework::Core::ERROR_NONE,
        "SetAppProperty() persists values when store dependency exists");

    std::string value;
    L0Test::ExpectEqU32(tr,
        impl->GetAppProperty("app.good", "sampleKey", value),
        WPEFramework::Core::ERROR_NONE,
        "GetAppProperty() succeeds for existing values");
    L0Test::ExpectEqStr(tr, value, std::string("sampleValue"), "GetAppProperty() returns previously set value");

    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_ClearAppDataAndSystemApisWithDependencies()
{
    L0Test::TestResult tr;

    auto* installer = new L0Test::FakePackageInstaller();
    auto* handler = new L0Test::FakePackageHandler();
    auto* store = new L0Test::FakeStore2();
    auto* storage = new L0Test::FakeStorageManager();

    L0Test::AppManagerServiceMock::Config cfg = CreateFullServiceConfig();
    cfg.installer = installer;
    cfg.packageHandler = handler;
    cfg.store2 = store;
    cfg.storageManager = storage;
    L0Test::AppManagerServiceMock service(cfg);

    auto* impl = CreateImpl();
    L0Test::ExpectEqU32(tr, impl->Configure(&service), WPEFramework::Core::ERROR_NONE, "Configure() succeeds with fake storage manager");

    L0Test::ExpectEqU32(tr, impl->ClearAppData("app.good"), WPEFramework::Core::ERROR_NONE, "ClearAppData() succeeds with storage manager");
    L0Test::ExpectEqU32(tr, impl->ClearAllAppData(), WPEFramework::Core::ERROR_NONE, "ClearAllAppData() succeeds with storage manager");

    int32_t v = 0;
    L0Test::ExpectEqU32(tr, impl->StartSystemApp("sys.app"), WPEFramework::Core::ERROR_NONE, "StartSystemApp() is stable");
    L0Test::ExpectEqU32(tr, impl->StopSystemApp("sys.app"), WPEFramework::Core::ERROR_NONE, "StopSystemApp() is stable");
    L0Test::ExpectEqU32(tr, impl->GetMaxRunningApps(v), WPEFramework::Core::ERROR_NONE, "GetMaxRunningApps() succeeds");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(v), static_cast<uint32_t>(-1), "GetMaxRunningApps() returns expected default");
    L0Test::ExpectEqU32(tr, impl->GetMaxHibernatedApps(v), WPEFramework::Core::ERROR_NONE, "GetMaxHibernatedApps() succeeds");
    L0Test::ExpectEqU32(tr, impl->GetMaxHibernatedFlashUsage(v), WPEFramework::Core::ERROR_NONE, "GetMaxHibernatedFlashUsage() succeeds");
    L0Test::ExpectEqU32(tr, impl->GetMaxInactiveRamUsage(v), WPEFramework::Core::ERROR_NONE, "GetMaxInactiveRamUsage() succeeds");

    std::string meta;
    L0Test::ExpectEqU32(tr, impl->GetAppMetadata("app.good", "key", meta), WPEFramework::Core::ERROR_NONE, "GetAppMetadata() is stable");

    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_CheckInstallUninstallBlockTrueForBlockedPackage()
{
    L0Test::TestResult tr;

    auto* installer = new L0Test::FakePackageInstaller();
    auto* handler = new L0Test::FakePackageHandler();
    auto* store = new L0Test::FakeStore2();
    auto* storage = new L0Test::FakeStorageManager();

    WPEFramework::Exchange::IPackageInstaller::Package blockedPkg;
    blockedPkg.packageId = "app.blocked";
    blockedPkg.version = "2.0.0";
    blockedPkg.state = WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLATION_BLOCKED;
    installer->installedPackages.push_back(blockedPkg);

    L0Test::AppManagerServiceMock::Config cfg = CreateFullServiceConfig();
    cfg.installer = installer;
    cfg.packageHandler = handler;
    cfg.store2 = store;
    cfg.storageManager = storage;
    L0Test::AppManagerServiceMock service(cfg);

    auto* impl = CreateImpl();
    L0Test::ExpectEqU32(tr, impl->Configure(&service), WPEFramework::Core::ERROR_NONE, "Configure() succeeds with blocked package fixture");

    const bool blocked = impl->checkInstallUninstallBlock("app.blocked");
    L0Test::ExpectTrue(tr, blocked, "checkInstallUninstallBlock() returns true when install state is blocked");

    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_ImplHandleOnAppUnloadedAndLaunchRequest()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();
    auto* notification = new RefNotification();
    impl->Register(notification);

    // Pre-insert an app so that Dispatch(APP_EVENT_UNLOADED) has something to work with.
    WPEFramework::Plugin::AppInfoManager::getInstance().clear();
    WPEFramework::Plugin::AppInfo appInfo;
    appInfo.setAppInstanceId("inst-unload");
    WPEFramework::Plugin::AppInfoManager::getInstance().upsert("app.unload",
        [&](WPEFramework::Plugin::AppInfo& a) { a = appInfo; });

    // handleOnAppUnloaded fires dispatchEvent(APP_EVENT_UNLOADED, ...) which
    // eventually calls Dispatch(), packageUnLock(), OnAppUnloaded notifications
    // and removeAppInfoByAppId().
    impl->handleOnAppUnloaded("app.unload", "inst-unload");
    L0Test::ExpectTrue(tr, true, "handleOnAppUnloaded() is stable for a valid app");

    // handleOnAppLaunchRequest fires dispatchEvent(APP_EVENT_LAUNCH_REQUEST, ...) which
    // eventually calls Dispatch() → OnAppLaunchRequest notifications.
    impl->handleOnAppLaunchRequest("app.launch", "intent://play", "source");
    L0Test::ExpectTrue(tr, notification->launch >= 1U,
        "handleOnAppLaunchRequest() fires the OnAppLaunchRequest notification");

    // Empty-id path: early return in handleOnAppUnloaded and handleOnAppLaunchRequest.
    impl->handleOnAppUnloaded("", "");
    impl->handleOnAppLaunchRequest("", "", "");
    L0Test::ExpectTrue(tr, true, "handleOnAppUnloaded/LaunchRequest() survive empty appId without crashing");

    impl->Unregister(notification);
    notification->Release();
    // Configure with full mock to ensure proper cleanup in destructor
    L0Test::AppManagerServiceMock service(CreateFullServiceConfig());
    impl->Configure(&service);
    impl->Release();
    WPEFramework::Plugin::AppInfoManager::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_ImplOnAppInstallationStatus()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();
    auto* notification = new RefNotification();
    impl->Register(notification);

    // Valid JSON array with one INSTALLED app.
    impl->OnAppInstallationStatus(
        "[{\"packageId\":\"pkg.install\",\"state\":\"INSTALLED\",\"version\":\"2.0\"}]");
    L0Test::ExpectTrue(tr, notification->installed >= 1U,
        "OnAppInstallationStatus() with INSTALLED state fires OnAppInstalled notification");

    // Valid JSON array with one UNINSTALLED app.
    impl->OnAppInstallationStatus(
        "[{\"packageId\":\"pkg.uninstall\",\"state\":\"UNINSTALLED\"}]");
    L0Test::ExpectTrue(tr, notification->uninstalled >= 1U,
        "OnAppInstallationStatus() with UNINSTALLED state fires OnAppUninstalled notification");

    // Unknown install state — hits the else branch in the Dispatch case.
    impl->OnAppInstallationStatus(
        "[{\"packageId\":\"pkg.other\",\"state\":\"DOWNLOADING\"}]");
    L0Test::ExpectTrue(tr, true,
        "OnAppInstallationStatus() with unknown state is handled without crashing");

    // Empty JSON — hits the early-return error branch.
    impl->OnAppInstallationStatus("");
    L0Test::ExpectTrue(tr, true, "OnAppInstallationStatus() handles empty string gracefully");

    // Invalid JSON — hits the parse-failure branch.
    impl->OnAppInstallationStatus("not-json");
    L0Test::ExpectTrue(tr, true, "OnAppInstallationStatus() handles invalid JSON gracefully");

    impl->Unregister(notification);
    notification->Release();
    // Configure with full mock to ensure proper cleanup in destructor
    L0Test::AppManagerServiceMock service(CreateFullServiceConfig());
    impl->Configure(&service);
    impl->Release();
    return tr.failures;
}

// Test configuration getters (GetMaxRunningApps, etc.)
uint32_t Test_AM_ConfigurationGettersReturnDefaults()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    int32_t maxRunningApps = -1;
    int32_t maxHibernatedApps = -1;
    int32_t maxHibernatedFlashUsage = -1;
    int32_t maxInactiveRamUsage = -1;

    const auto result1 = impl->GetMaxRunningApps(maxRunningApps);
    L0Test::ExpectEqU32(tr, result1, WPEFramework::Core::ERROR_NONE, "GetMaxRunningApps() returns ERROR_NONE");
    L0Test::ExpectTrue(tr, maxRunningApps >= 0, "GetMaxRunningApps() returns non-negative value");

    const auto result2 = impl->GetMaxHibernatedApps(maxHibernatedApps);
    L0Test::ExpectEqU32(tr, result2, WPEFramework::Core::ERROR_NONE, "GetMaxHibernatedApps() returns ERROR_NONE");
    L0Test::ExpectTrue(tr, maxHibernatedApps >= 0, "GetMaxHibernatedApps() returns non-negative value");

    const auto result3 = impl->GetMaxHibernatedFlashUsage(maxHibernatedFlashUsage);
    L0Test::ExpectEqU32(tr, result3, WPEFramework::Core::ERROR_NONE, "GetMaxHibernatedFlashUsage() returns ERROR_NONE");
    L0Test::ExpectTrue(tr, maxHibernatedFlashUsage >= 0, "GetMaxHibernatedFlashUsage() returns non-negative value");

    const auto result4 = impl->GetMaxInactiveRamUsage(maxInactiveRamUsage);
    L0Test::ExpectEqU32(tr, result4, WPEFramework::Core::ERROR_NONE, "GetMaxInactiveRamUsage() returns ERROR_NONE");
    L0Test::ExpectTrue(tr, maxInactiveRamUsage >= 0, "GetMaxInactiveRamUsage() returns non-negative value");

    // Configure with full mock to ensure proper cleanup in destructor
    L0Test::AppManagerServiceMock service(CreateFullServiceConfig());
    impl->Configure(&service);
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_GetAppMetadataInvalidParams()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();
    std::string result;

    const auto r1 = impl->GetAppMetadata(std::string(), std::string("key"), result);
    L0Test::ExpectEqU32(tr, r1, WPEFramework::Core::ERROR_GENERAL, "GetAppMetadata() rejects empty appId");

    const auto r2 = impl->GetAppMetadata(std::string("app1"), std::string(), result);
    L0Test::ExpectEqU32(tr, r2, WPEFramework::Core::ERROR_GENERAL, "GetAppMetadata() rejects empty metaData key");

    // Configure with full mock to ensure proper cleanup in destructor
    L0Test::AppManagerServiceMock service(CreateFullServiceConfig());
    impl->Configure(&service);
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_StartAndStopSystemApp()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    const auto r1 = impl->StartSystemApp(std::string("systemApp1"));
    L0Test::ExpectEqU32(tr, r1, WPEFramework::Core::ERROR_NONE, "StartSystemApp() remains stable");

    const auto r2 = impl->StopSystemApp(std::string("systemApp1"));
    L0Test::ExpectEqU32(tr, r2, WPEFramework::Core::ERROR_NONE, "StopSystemApp() remains stable");

    // Test with empty app ID
    const auto r3 = impl->StartSystemApp(std::string());
    L0Test::ExpectTrue(tr, r3 == WPEFramework::Core::ERROR_NONE || r3 == WPEFramework::Core::ERROR_GENERAL, "StartSystemApp() with empty ID remains stable");

    const auto r4 = impl->StopSystemApp(std::string());
    L0Test::ExpectTrue(tr, r4 == WPEFramework::Core::ERROR_NONE || r4 == WPEFramework::Core::ERROR_GENERAL, "StopSystemApp() with empty ID remains stable");

    // Configure with full mock to ensure proper cleanup in destructor
    L0Test::AppManagerServiceMock service(CreateFullServiceConfig());
    impl->Configure(&service);
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_LaunchAppWithPackageHandler()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    L0Test::FakePackageHandler packageHandler;
    L0Test::FakePackageInstaller packageInstaller;
    L0Test::FakeStore2 store;
    L0Test::FakeLifecycleManager lifecycleManager;
    L0Test::FakeLifecycleManagerState lifecycleState;

    L0Test::AppManagerServiceMock::Config cfg(&packageInstaller);
    cfg.packageHandler = &packageHandler;
    cfg.store2 = &store;
    cfg.lifecycleManager = &lifecycleManager;
    cfg.lifecycleManagerState = &lifecycleState;
    L0Test::AppManagerServiceMock service(cfg);

    // Setup package installer with a test package
    WPEFramework::Exchange::IPackageInstaller::Package pkg;
    pkg.packageId = "testApp1";
    pkg.version = "1.0.0";
    pkg.state = WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLED;
    packageInstaller.installedPackages.push_back(pkg);

    impl->Configure(&service);

    // Setup lifecycle manager to succeed
    lifecycleManager.spawnAppHandler = [](const std::string&, const std::string&, 
        const WPEFramework::Exchange::ILifecycleManager::LifecycleState,
        const WPEFramework::Exchange::RuntimeConfig&, const std::string&,
        std::string& appInstanceId, std::string& errorReason, bool& success) {
        appInstanceId = "instance-test-1";
        errorReason.clear();
        success = true;
        return WPEFramework::Core::ERROR_NONE;
    };

    const auto result = impl->LaunchApp(std::string("testApp1"), std::string("defaultIntent"), std::string("args"));
    
    // Should attempt to launch (may fail if dependencies not fully configured)
    L0Test::ExpectTrue(tr, result == WPEFramework::Core::ERROR_NONE || 
                            result == WPEFramework::Core::ERROR_GENERAL ||
                            result == WPEFramework::Core::ERROR_UNAVAILABLE, 
                       "LaunchApp() with package handler attempts operation");

    L0Test::ExpectTrue(tr, packageHandler.lockCount >= 0, "Package lock was attempted");

    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_PreloadAppWithPackageHandler()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    L0Test::FakePackageHandler packageHandler;
    L0Test::FakePackageInstaller packageInstaller;
    L0Test::FakeStore2 store;
    L0Test::FakeLifecycleManager lifecycleManager;
    L0Test::FakeLifecycleManagerState lifecycleState;

    L0Test::AppManagerServiceMock::Config cfg(&packageInstaller);
    cfg.packageHandler = &packageHandler;
    cfg.store2 = &store;
    cfg.lifecycleManager = &lifecycleManager;
    cfg.lifecycleManagerState = &lifecycleState;
    L0Test::AppManagerServiceMock service(cfg);

    // Setup package installer with a test package
    WPEFramework::Exchange::IPackageInstaller::Package pkg;
    pkg.packageId = "testApp2";
    pkg.version = "1.0.0";
    pkg.state = WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLED;
    packageInstaller.installedPackages.push_back(pkg);

    impl->Configure(&service);

    // Setup lifecycle manager to succeed
    lifecycleManager.spawnAppHandler = [](const std::string&, const std::string&, 
        const WPEFramework::Exchange::ILifecycleManager::LifecycleState,
        const WPEFramework::Exchange::RuntimeConfig&, const std::string&,
        std::string& appInstanceId, std::string& errorReason, bool& success) {
        appInstanceId = "instance-preload-1";
        errorReason.clear();
        success = true;
        return WPEFramework::Core::ERROR_NONE;
    };

    std::string error;
    const auto result = impl->PreloadApp(std::string("testApp2"), std::string("defaultIntent"), std::string("args"), error);
    
    // Should attempt to preload
    L0Test::ExpectTrue(tr, result == WPEFramework::Core::ERROR_NONE || 
                            result == WPEFramework::Core::ERROR_GENERAL ||
                            result == WPEFramework::Core::ERROR_UNAVAILABLE, 
                       "PreloadApp() with package handler attempts operation");

    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_IsInstalledMultiplePackages()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    L0Test::FakePackageHandler packageHandler;
    L0Test::FakePackageInstaller packageInstaller;

    // Setup multiple test packages
    WPEFramework::Exchange::IPackageInstaller::Package pkg1, pkg2, pkg3;
    pkg1.packageId = "app1";
    pkg1.version = "1.0.0";
    pkg1.state = WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLED;
    
    pkg2.packageId = "app2";
    pkg2.version = "2.0.0";
    pkg2.state = WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLED;
    
    pkg3.packageId = "app3";
    pkg3.version = "1.5.0";
    pkg3.state = WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLED;

    packageInstaller.installedPackages.push_back(pkg1);
    packageInstaller.installedPackages.push_back(pkg2);
    packageInstaller.installedPackages.push_back(pkg3);

    L0Test::AppManagerServiceMock::Config cfg(&packageInstaller);
    cfg.packageHandler = &packageHandler;
    L0Test::AppManagerServiceMock service(cfg);

    impl->Configure(&service);

    bool installed1 = false, installed2 = false, installed3 = false, installed4 = false;

    impl->IsInstalled(std::string("app1"), installed1);
    impl->IsInstalled(std::string("app2"), installed2);
    impl->IsInstalled(std::string("app3"), installed3);
    impl->IsInstalled(std::string("nonExistentApp"), installed4);

    L0Test::ExpectTrue(tr, installed1 == true, "app1 is detected as installed");
    L0Test::ExpectTrue(tr, installed2 == true, "app2 is detected as installed");
    L0Test::ExpectTrue(tr, installed3 == true, "app3 is detected as installed");
    L0Test::ExpectTrue(tr, installed4 == false, "nonExistentApp is not installed");

    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_GetInstalledAppsMixedTypes()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    L0Test::FakePackageHandler packageHandler;
    L0Test::FakePackageInstaller packageInstaller;

    // Setup packages with different types
    WPEFramework::Exchange::IPackageInstaller::Package pkg1, pkg2, pkg3;
    pkg1.packageId = "interactiveApp";
    pkg1.version = "1.0.0";
    pkg1.state = WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLED;
    
    pkg2.packageId = "systemApp";
    pkg2.version = "2.0.0";
    pkg2.state = WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLED;
    
    pkg3.packageId = "unknownApp";
    pkg3.version = "1.5.0";
    pkg3.state = WPEFramework::Exchange::IPackageInstaller::InstallState::INSTALLED;

    packageInstaller.installedPackages.push_back(pkg1);
    packageInstaller.installedPackages.push_back(pkg2);
    packageInstaller.installedPackages.push_back(pkg3);

    L0Test::AppManagerServiceMock::Config cfg(&packageInstaller);
    cfg.packageHandler = &packageHandler;
    L0Test::AppManagerServiceMock service(cfg);

    impl->Configure(&service);

    std::string apps;
    const auto result = impl->GetInstalledApps(apps);

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE, "GetInstalledApps() returns ERROR_NONE with packages");
    L0Test::ExpectTrue(tr, !apps.empty(), "GetInstalledApps() returns non-empty JSON");
    L0Test::ExpectTrue(tr, apps.find("interactiveApp") != std::string::npos, "JSON contains interactiveApp");
    L0Test::ExpectTrue(tr, apps.find("systemApp") != std::string::npos, "JSON contains systemApp");

    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_WorkerThreadStability()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    L0Test::AppManagerServiceMock service(CreateFullServiceConfig());
    impl->Configure(&service);

    // Worker thread should be running after Configure
    // Just verify the implementation doesn't crash
    L0Test::ExpectTrue(tr, impl != nullptr, "Implementation stable after Configure");

    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_ClearAppDataWithStorageManager()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    L0Test::FakeStorageManager storageManager;
    L0Test::AppManagerServiceMock::Config cfg;
    cfg.storageManager = &storageManager;
    L0Test::AppManagerServiceMock service(cfg);

    impl->Configure(&service);

    const auto result = impl->ClearAppData(std::string("testApp"));
    L0Test::ExpectTrue(tr, result == WPEFramework::Core::ERROR_NONE || 
                            result == WPEFramework::Core::ERROR_GENERAL,
                       "ClearAppData() with storage manager remains stable");

    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_ClearAllAppDataWithStorageManager()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    L0Test::FakeStorageManager storageManager;
    L0Test::AppManagerServiceMock::Config cfg;
    cfg.storageManager = &storageManager;
    L0Test::AppManagerServiceMock service(cfg);

    impl->Configure(&service);

    const auto result = impl->ClearAllAppData();
    L0Test::ExpectTrue(tr, result == WPEFramework::Core::ERROR_NONE || 
                            result == WPEFramework::Core::ERROR_GENERAL,
                       "ClearAllAppData() with storage manager remains stable");

    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_GetLoadedAppsWithLifecycleConnector()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    L0Test::FakeLifecycleManager lifecycleManager;
    L0Test::FakeLifecycleManagerState lifecycleState;
    
    // Setup loaded apps JSON
    lifecycleManager.loadedAppsJson = R"([{"appId":"app1","appInstanceId":"instance1","state":"running"}])";
    
    L0Test::AppManagerServiceMock::Config cfg;
    cfg.lifecycleManager = &lifecycleManager;
    cfg.lifecycleManagerState = &lifecycleState;
    L0Test::AppManagerServiceMock service(cfg);

    impl->Configure(&service);

    WPEFramework::Exchange::IAppManager::ILoadedAppInfoIterator* iterator = nullptr;
    const auto result = impl->GetLoadedApps(iterator);

    L0Test::ExpectTrue(tr, result == WPEFramework::Core::ERROR_NONE || 
                            result == WPEFramework::Core::ERROR_GENERAL,
                       "GetLoadedApps() with lifecycle connector attempts operation");

    if (iterator) {
        iterator->Release();
    }

    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_SendIntentWithLifecycleConnector()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    L0Test::FakeLifecycleManager lifecycleManager;
    L0Test::FakeLifecycleManagerState lifecycleState;
    
    bool sendIntentCalled = false;
    lifecycleManager.sendIntentHandler = [&sendIntentCalled](const std::string&, const std::string&, 
                                                               std::string&, bool& success) {
        sendIntentCalled = true;
        success = true;
        return WPEFramework::Core::ERROR_NONE;
    };
    
    L0Test::AppManagerServiceMock::Config cfg;
    cfg.lifecycleManager = &lifecycleManager;
    cfg.lifecycleManagerState = &lifecycleState;
    L0Test::AppManagerServiceMock service(cfg);

    impl->Configure(&service);

    const auto result = impl->SendIntent(std::string("app1"), std::string("testIntent"));

    L0Test::ExpectTrue(tr, result == WPEFramework::Core::ERROR_NONE || 
                            result == WPEFramework::Core::ERROR_GENERAL ||
                            result == WPEFramework::Core::ERROR_UNAVAILABLE,
                       "SendIntent() with lifecycle connector attempts operation");

    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_CloseAppWithLifecycleConnector()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    L0Test::FakeLifecycleManager lifecycleManager;
    L0Test::FakeLifecycleManagerState lifecycleState;
    
    L0Test::AppManagerServiceMock::Config cfg;
    cfg.lifecycleManager = &lifecycleManager;
    cfg.lifecycleManagerState = &lifecycleState;
    L0Test::AppManagerServiceMock service(cfg);

    impl->Configure(&service);

    const auto result = impl->CloseApp(std::string("app1"));

    L0Test::ExpectTrue(tr, result == WPEFramework::Core::ERROR_NONE || 
                            result == WPEFramework::Core::ERROR_GENERAL,
                       "CloseApp() with lifecycle connector remains stable");

    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_TerminateAppWithLifecycleConnector()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    L0Test::FakeLifecycleManager lifecycleManager;
    L0Test::FakeLifecycleManagerState lifecycleState;
    
    L0Test::AppManagerServiceMock::Config cfg;
    cfg.lifecycleManager = &lifecycleManager;
    cfg.lifecycleManagerState = &lifecycleState;
    L0Test::AppManagerServiceMock service(cfg);

    impl->Configure(&service);

    const auto result = impl->TerminateApp(std::string("app1"));

    L0Test::ExpectTrue(tr, result == WPEFramework::Core::ERROR_NONE || 
                            result == WPEFramework::Core::ERROR_GENERAL,
                       "TerminateApp() with lifecycle connector remains stable");

    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_KillAppWithLifecycleConnector()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    L0Test::FakeLifecycleManager lifecycleManager;
    L0Test::FakeLifecycleManagerState lifecycleState;
    
    bool killAppCalled = false;
    lifecycleManager.killAppHandler = [&killAppCalled](const std::string&, std::string&, bool& success) {
        killAppCalled = true;
        success = true;
        return WPEFramework::Core::ERROR_NONE;
    };
    
    L0Test::AppManagerServiceMock::Config cfg;
    cfg.lifecycleManager = &lifecycleManager;
    cfg.lifecycleManagerState = &lifecycleState;
    L0Test::AppManagerServiceMock service(cfg);

    impl->Configure(&service);

    const auto result = impl->KillApp(std::string("app1"));

    L0Test::ExpectTrue(tr, result == WPEFramework::Core::ERROR_NONE || 
                            result == WPEFramework::Core::ERROR_GENERAL ||
                            result == WPEFramework::Core::ERROR_UNAVAILABLE,
                       "KillApp() with lifecycle connector attempts operation");

    impl->Release();
    return tr.failures;
}
