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
 * @file AppManager_ImplementationTests.cpp
 *
 * L0 tests for AppManagerImplementation methods.
 *
 * Covers:
 *   - Register / Unregister (AM-I-01 … AM-I-04)
 *   - Configure null/valid (AM-I-05 … AM-I-06)
 *   - LaunchApp / CloseApp / TerminateApp / KillApp (AM-I-07 … AM-I-17)
 *   - GetLoadedApps / SendIntent / PreloadApp (AM-I-18 … AM-I-22)
 *   - GetAppProperty / SetAppProperty (AM-I-23 … AM-I-26)
 *   - GetInstalledApps / IsInstalled (AM-I-27 … AM-I-28)
 *   - StartSystemApp / StopSystemApp (AM-I-29 … AM-I-30)
 *   - ClearAppData / ClearAllAppData (AM-I-31 … AM-I-32)
 *   - GetMax* stubs (AM-I-52 … AM-I-55)
 *   - handleOnAppLifecycleStateChanged dispatch  (AM-I-35)
 *   - handleOnAppUnloaded / handleOnAppLaunchRequest guards (AM-I-45 … AM-I-46)
 *   - getInstallAppType (AM-I-36 … AM-I-38)
 *   - getInstance() singleton (AM-I-56 … AM-I-57)
 */

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "AppManagerImplementation.h"
#include "ServiceMock.h"
#include "common/L0Bootstrap.hpp"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

// ──────────────────────────────────────────────────────────────────────────────
// Local fakes (anonymous namespace)
// ──────────────────────────────────────────────────────────────────────────────

namespace {

/**
 * @brief Minimal INotification implementation with per-event call counters.
 *
 * The fake is intentionally NOT heap-allocated like a real Thunder object —
 * it lives on the stack/in test scope.  Its lifetime is always longer than
 * the AppManagerImplementation under test because we call Unregister before
 * the test stack frame returns.
 */
class FakeNotification final : public WPEFramework::Exchange::IAppManager::INotification {
public:
    FakeNotification() = default;

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t r = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == r) {
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

    // Counters for every notification event
    mutable std::atomic<uint32_t> onLifecycleChangedCount { 0 };
    mutable std::atomic<uint32_t> onInstalledCount { 0 };
    mutable std::atomic<uint32_t> onUninstalledCount { 0 };
    mutable std::atomic<uint32_t> onUnloadedCount { 0 };
    mutable std::atomic<uint32_t> onLaunchRequestCount { 0 };

    void OnAppLifecycleStateChanged(
        const std::string& /*appId*/,
        const std::string& /*appInstanceId*/,
        const WPEFramework::Exchange::IAppManager::AppLifecycleState /*newState*/,
        const WPEFramework::Exchange::IAppManager::AppLifecycleState /*oldState*/,
        const WPEFramework::Exchange::IAppManager::AppErrorReason /*reason*/) override
    {
        onLifecycleChangedCount.fetch_add(1, std::memory_order_relaxed);
    }

    void OnAppInstalled(const std::string& /*appId*/,
                        const std::string& /*version*/) override
    {
        onInstalledCount.fetch_add(1, std::memory_order_relaxed);
    }

    void OnAppUninstalled(const std::string& /*appId*/) override
    {
        onUninstalledCount.fetch_add(1, std::memory_order_relaxed);
    }

    void OnAppUnloaded(const std::string& /*appId*/,
                       const std::string& /*appInstanceId*/) override
    {
        onUnloadedCount.fetch_add(1, std::memory_order_relaxed);
    }

    void OnAppLaunchRequest(const std::string& /*appId*/,
                            const std::string& /*intent*/,
                            const std::string& /*source*/) override
    {
        onLaunchRequestCount.fetch_add(1, std::memory_order_relaxed);
    }

private:
    mutable std::atomic<uint32_t> _refCount { 1 };
};

// Helper: create a fresh AppManagerImplementation instance owned by caller.
inline WPEFramework::Plugin::AppManagerImplementation* CreateImpl()
{
    return WPEFramework::Core::Service<
        WPEFramework::Plugin::AppManagerImplementation>::Create<
        WPEFramework::Plugin::AppManagerImplementation>();
}

} // anonymous namespace

// ──────────────────────────────────────────────────────────────────────────────
// Register / Unregister (AM-I-01 … AM-I-04)
// ──────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_RegisterNotificationSucceeds()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    FakeNotification n;

    const auto result = impl->Register(&n);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "Register() returns ERROR_NONE on first registration");

    impl->Unregister(&n);
    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_RegisterDuplicateNotificationIsIdempotent()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    FakeNotification n;

    impl->Register(&n);
    const auto result = impl->Register(&n); // duplicate
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "Register() with duplicate notification is idempotent (returns ERROR_NONE)");

    impl->Unregister(&n);
    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_UnregisterRegisteredNotificationSucceeds()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    FakeNotification n;

    impl->Register(&n);
    const auto result = impl->Unregister(&n);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "Unregister() returns ERROR_NONE when notification was registered");

    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_UnregisterUnknownNotificationReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    FakeNotification n;

    // n was never registered
    const auto result = impl->Unregister(&n);
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "Unregister() returns error when notification was never registered");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// Configure (AM-I-05 … AM-I-06)
// ──────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_ConfigureNullServiceReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Configure(nullptr);
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "Configure(nullptr) returns non-zero error code");

    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_ConfigureWithServiceSucceeds()
{
    L0Test::TestResult tr;

    // NOTE: Configure() internally attempts to acquire remote plugins
    // (LifecycleManager, PersistentStore, PackageManager, StorageManager).
    // The ServiceMock returns nullptr for all QueryInterfaceByCallsign calls, so each
    // remote plugin acquisition fails. StorageManager retries 2 × 200 ms ≈ 400 ms.
    L0Test::AppManagerServiceMock service;
    auto* impl = CreateImpl();
    const auto result = impl->Configure(&service);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "Configure() with valid service returns ERROR_NONE even when remote plugins unavailable");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// LaunchApp (AM-I-07 … AM-I-09)
// ──────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_LaunchAppEmptyAppIdReturnsInvalidParam()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->LaunchApp("", "", "");
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "LaunchApp('') returns non-zero error for empty appId");

    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_LaunchAppWithNoPackageManagerReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    // Package manager not available — IsInstalled check will fail → ERROR_GENERAL
    const auto result = impl->LaunchApp("com.test.app", "{}", "");
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "LaunchApp() returns error when PackageManager is unavailable");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// CloseApp (AM-I-10 … AM-I-11)
// ──────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_CloseAppEmptyAppIdReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->CloseApp("");
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "CloseApp('') returns error for empty appId");

    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_CloseAppNoLifecycleConnectorReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    // No lifecycle connector (Configure not called) → ERROR_GENERAL
    const auto result = impl->CloseApp("com.test.app");
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "CloseApp() returns error when LifecycleInterfaceConnector is null");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// TerminateApp (AM-I-12 … AM-I-13)
// ──────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_TerminateAppEmptyAppIdReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->TerminateApp("");
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "TerminateApp('') returns error for empty appId");

    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_TerminateAppNoLifecycleConnectorReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->TerminateApp("com.test.app");
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "TerminateApp() returns error when LifecycleInterfaceConnector is null");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// KillApp (AM-I-14)
// KillApp defaults to ERROR_NONE even without a connector (no null-guard).
// ──────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_KillAppNoConnectorReturnsNone()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->KillApp("com.test.app");
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "KillApp() returns ERROR_NONE when LifecycleInterfaceConnector is null");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// GetLoadedApps (AM-I-15)
// ──────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_GetLoadedAppsNoConnectorReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    WPEFramework::Exchange::IAppManager::ILoadedAppInfoIterator* iter = nullptr;
    const auto result = impl->GetLoadedApps(iter);
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "GetLoadedApps() returns error when LifecycleInterfaceConnector is null");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// SendIntent (AM-I-16)
// SendIntent defaults to ERROR_NONE even without a connector.
// ──────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_SendIntentNoConnectorReturnsNone()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->SendIntent("com.test.app", "{}");
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "SendIntent() returns ERROR_NONE when LifecycleInterfaceConnector is null");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// PreloadApp (AM-I-17 … AM-I-18)
// ──────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_PreloadAppEmptyAppIdReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    std::string error;
    const auto result = impl->PreloadApp("", "", error);
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "PreloadApp('') returns error for empty appId");

    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_PreloadAppNoPackageManagerReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    std::string error;
    // PackageManager not available → IsInstalled fails → error
    const auto result = impl->PreloadApp("com.test.app", "", error);
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "PreloadApp() returns error when PackageManager is unavailable");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// GetAppProperty / SetAppProperty (AM-I-23 … AM-I-26)
// ──────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_GetAppPropertyEmptyAppIdReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    std::string value;
    const auto result = impl->GetAppProperty("", "someKey", value);
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "GetAppProperty('', key) returns error for empty appId");

    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_GetAppPropertyEmptyKeyReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    std::string value;
    const auto result = impl->GetAppProperty("com.test.app", "", value);
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "GetAppProperty(id, '') returns error for empty key");

    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_SetAppPropertyEmptyAppIdReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->SetAppProperty("", "someKey", "val");
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "SetAppProperty('', key, val) returns error for empty appId");

    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_SetAppPropertyNoPersistentStoreReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    // PersistentStore not available → ERROR_GENERAL
    const auto result = impl->SetAppProperty("com.test.app", "someKey", "val");
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "SetAppProperty() returns error when PersistentStore is unavailable");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// GetInstalledApps / IsInstalled (AM-I-27 … AM-I-28)
// ──────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_GetInstalledAppsNoPackageManagerReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    std::string apps;
    const auto result = impl->GetInstalledApps(apps);
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "GetInstalledApps() returns error when PackageManager is unavailable");

    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_IsInstalledNoPackageManagerReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    bool installed = false;
    const auto result = impl->IsInstalled("com.test.app", installed);
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "IsInstalled() returns error when PackageManager is unavailable");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// StartSystemApp / StopSystemApp (AM-I-29 … AM-I-30)
// Both are stub implementations returning ERROR_NONE unconditionally.
// ──────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_StartSystemAppReturnsNone()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->StartSystemApp("com.system.app");
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "StartSystemApp() returns ERROR_NONE (stub implementation)");

    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_StopSystemAppReturnsNone()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->StopSystemApp("com.system.app");
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "StopSystemApp() returns ERROR_NONE (stub implementation)");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// ClearAppData / ClearAllAppData (AM-I-31 … AM-I-32)
// ──────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_ClearAppDataEmptyAppIdReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->ClearAppData("");
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "ClearAppData('') returns error for empty appId");

    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_ClearAppDataNoStorageManagerReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    // StorageManager not available → ERROR_GENERAL
    const auto result = impl->ClearAppData("com.test.app");
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "ClearAppData() returns error when StorageManager is unavailable");

    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_ClearAllAppDataNoStorageManagerReturnsError()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    // StorageManager not available → ERROR_GENERAL
    const auto result = impl->ClearAllAppData();
    L0Test::ExpectTrue(tr, result != WPEFramework::Core::ERROR_NONE,
        "ClearAllAppData() returns error when StorageManager is unavailable");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// GetMax* stubs (AM-I-52 … AM-I-55)
// All return -1 and ERROR_NONE (current stub implementation).
// ──────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_GetMaxRunningAppsReturnsNone()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    int32_t value = 0;
    const auto result = impl->GetMaxRunningApps(value);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "GetMaxRunningApps() returns ERROR_NONE");
    L0Test::ExpectTrue(tr, value == -1,
        "GetMaxRunningApps() sets output to -1 (current stub)");

    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_GetMaxHibernatedAppsReturnsNone()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    int32_t value = 0;
    const auto result = impl->GetMaxHibernatedApps(value);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "GetMaxHibernatedApps() returns ERROR_NONE");
    L0Test::ExpectTrue(tr, value == -1,
        "GetMaxHibernatedApps() sets output to -1 (current stub)");

    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_GetMaxHibernatedFlashUsageReturnsNone()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    int32_t value = 0;
    const auto result = impl->GetMaxHibernatedFlashUsage(value);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "GetMaxHibernatedFlashUsage() returns ERROR_NONE");
    L0Test::ExpectTrue(tr, value == -1,
        "GetMaxHibernatedFlashUsage() sets output to -1 (current stub)");

    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_GetMaxInactiveRamUsageReturnsNone()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    int32_t value = 0;
    const auto result = impl->GetMaxInactiveRamUsage(value);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "GetMaxInactiveRamUsage() returns ERROR_NONE");
    L0Test::ExpectTrue(tr, value == -1,
        "GetMaxInactiveRamUsage() sets output to -1 (current stub)");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// handleOnAppUnloaded / handleOnAppLaunchRequest — empty appId guards (synchronous)
// When appId is empty, LOGERR is called but no dispatchEvent() occurs.
// ──────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_HandleOnAppUnloadedEmptyAppIdDoesNotDispatch()
{
    L0Test::TestResult tr;

    L0Test::L0BootstrapGuard guard;
    auto* impl = CreateImpl();
    FakeNotification n;
    impl->Register(&n);

    // Empty appId — LOGERR in production code, no dispatch
    impl->handleOnAppUnloaded("", "inst-1");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    L0Test::ExpectEqU32(tr, n.onUnloadedCount.load(), 0U,
        "handleOnAppUnloaded('') does NOT dispatch notification callback");

    impl->Unregister(&n);
    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_HandleOnAppLaunchRequestEmptyAppIdDoesNotDispatch()
{
    L0Test::TestResult tr;

    L0Test::L0BootstrapGuard guard;
    auto* impl = CreateImpl();
    FakeNotification n;
    impl->Register(&n);

    // Empty appId — LOGERR in production code, no dispatch
    impl->handleOnAppLaunchRequest("", "{}", "voice");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    L0Test::ExpectEqU32(tr, n.onLaunchRequestCount.load(), 0U,
        "handleOnAppLaunchRequest('') does NOT dispatch notification callback");

    impl->Unregister(&n);
    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// handleOnAppLifecycleStateChanged — async dispatch through Thunder worker pool
// ──────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_HandleOnAppLifecycleStateChangedDispatchesNotification()
{
    L0Test::TestResult tr;

    L0Test::L0BootstrapGuard guard; // required for Core::IWorkerPool

    auto* impl = CreateImpl();
    FakeNotification n;
    impl->Register(&n);

    impl->handleOnAppLifecycleStateChanged(
        "com.test.app",
        "inst-123",
        WPEFramework::Exchange::IAppManager::AppLifecycleState::APP_STATE_RUNNING,
        WPEFramework::Exchange::IAppManager::AppLifecycleState::APP_STATE_LOADING,
        WPEFramework::Exchange::IAppManager::AppErrorReason::APP_ERROR_NONE);

    // Allow worker-pool thread to execute the dispatched Job
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    L0Test::ExpectEqU32(tr, n.onLifecycleChangedCount.load(), 1U,
        "handleOnAppLifecycleStateChanged() dispatches OnAppLifecycleStateChanged callback");

    impl->Unregister(&n);
    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// getInstallAppType (AM-I-36 … AM-I-38)
// ──────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_GetInstallAppTypeInteractiveReturnsCorrectString()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const std::string result = impl->getInstallAppType(
        WPEFramework::Plugin::AppManagerImplementation::APPLICATION_TYPE_INTERACTIVE);
    L0Test::ExpectEqStr(tr, result, "INTERACTIVE_APP",
        "getInstallAppType(APPLICATION_TYPE_INTERACTIVE) returns 'INTERACTIVE_APP'");

    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_GetInstallAppTypeSystemReturnsCorrectString()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const std::string result = impl->getInstallAppType(
        WPEFramework::Plugin::AppManagerImplementation::APPLICATION_TYPE_SYSTEM);
    L0Test::ExpectEqStr(tr, result, "SYSTEM_APP",
        "getInstallAppType(APPLICATION_TYPE_SYSTEM) returns 'SYSTEM_APP'");

    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_GetInstallAppTypeUnknownReturnsEmptyString()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const std::string result = impl->getInstallAppType(
        WPEFramework::Plugin::AppManagerImplementation::APPLICATION_TYPE_UNKNOWN);
    L0Test::ExpectTrue(tr, result.empty(),
        "getInstallAppType(APPLICATION_TYPE_UNKNOWN) returns empty string");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// getInstance() singleton (AM-I-56 … AM-I-57)
// ──────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_GetInstanceReturnsSelf()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto* instance = WPEFramework::Plugin::AppManagerImplementation::getInstance();
    L0Test::ExpectTrue(tr, instance == impl,
        "getInstance() returns the same instance that was just constructed");

    impl->Release();
    return tr.failures;
}

uint32_t Test_Impl_GetInstanceBeforeConstructionReturnsNull()
{
    L0Test::TestResult tr;

    // Verify singleton is null when no implementation exists in this scope.
    // _instance is reset to nullptr in the constructor / destructor.
    const auto* instance = WPEFramework::Plugin::AppManagerImplementation::getInstance();
    L0Test::ExpectTrue(tr, instance == nullptr,
        "getInstance() returns nullptr when no AppManagerImplementation exists");

    return tr.failures;
}
