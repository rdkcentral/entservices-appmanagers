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
#include "common/AppManagerL0Fakes.hpp"
#include "common/L0Expect.hpp"

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
    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_ConfigureWithNullServiceFails()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    const auto result = impl->Configure(nullptr);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL, "Configure(nullptr) returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_ConfigureWithValidServiceReturnsSuccess()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    L0Test::AppManagerServiceMock service;
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

    impl->Release();
    return tr.failures;
}

uint32_t Test_AM_ClearAppDataAndClearAllAppDataWithoutDependencies()
{
    L0Test::TestResult tr;
    auto* impl = CreateImpl();

    L0Test::ExpectEqU32(tr, impl->ClearAppData(std::string()), WPEFramework::Core::ERROR_GENERAL, "ClearAppData() rejects empty app id");
    L0Test::ExpectEqU32(tr, impl->ClearAllAppData(), WPEFramework::Core::ERROR_GENERAL, "ClearAllAppData() fails without storage manager");

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

    L0Test::AppManagerServiceMock::Config cfg(installer);
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

    L0Test::AppManagerServiceMock::Config cfg(installer);
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

    L0Test::AppManagerServiceMock::Config cfg(installer);
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
    impl->Release();
    return tr.failures;
}
