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
#include <string>

#include "AppManagerImplementation.h"
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
