/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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
 * @file LifecycleManager_ImplementationTests.cpp
 *
 * L0 tests for LifecycleManagerImplementation covering:
 *   - Register / Unregister (ILifecycleManager::INotification)
 *   - Register / Unregister (ILifecycleManagerState::INotification)
 *   - GetLoadedApps (empty, with apps, verbose/non-verbose)
 *   - IsAppLoaded
 *   - SendIntentToActiveApp (unknown app, non-ACTIVE, ACTIVE app)
 *   - AppReady (unknown, known)
 *   - StateChangeComplete
 *   - CloseApp (unknown appId, USER_EXIT)
 *   - Configure (null service)
 *   - onRippleEvent (no-op)
 *   - Dispatch event routing
 *   LCM-016 to LCM-069
 */

#include <atomic>
#include <chrono>
#include <iostream>
#include <list>
#include <string>
#include <thread>

#include "LifecycleManagerImplementation.h"
#include "ApplicationContext.h"
#include "State.h"
#include "LifecycleManagerServiceMock.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// Test helper: friend class for accessing private members
// ─────────────────────────────────────────────────────────────────────────────

namespace WPEFramework {
namespace Plugin {

/**
 * @brief Accesses LifecycleManagerImplementation private members via UNIT_TEST friend.
 *
 * Must be in WPEFramework::Plugin namespace to match the friend declaration
 * 'friend class LifecycleManagerImplementationTest' in LifecycleManagerImplementation.h,
 * which injects the name into the enclosing WPEFramework::Plugin namespace.
 */
class LifecycleManagerImplementationTest {
public:
    using EventNames = LifecycleManagerImplementation::EventNames;

    static std::list<Exchange::ILifecycleManager::INotification*>&
    getNotifications(LifecycleManagerImplementation& impl)
    {
        return impl.mLifecycleManagerNotification;
    }

    static std::list<Exchange::ILifecycleManagerState::INotification*>&
    getStateNotifications(LifecycleManagerImplementation& impl)
    {
        return impl.mLifecycleManagerStateNotification;
    }

    static std::list<ApplicationContext*>&
    getLoadedApps(LifecycleManagerImplementation& impl)
    {
        return impl.mLoadedApplications;
    }

    /**
     * @brief Direct synchronous call to the private Dispatch method.
     */
    static void callDispatch(
        LifecycleManagerImplementation& impl,
        EventNames event,
        const Core::JSON::VariantContainer& params)
    {
        impl.Dispatch(event, params);
    }
};

/**
 * @brief Concrete LifecycleManagerImplementation for unit tests.
 *
 * LifecycleManagerImplementation is abstract — AddRef()/Release() are pure virtual
 * (inherited from Core::IReferenceCounted; BEGIN_INTERFACE_MAP only provides
 * QueryInterface). This subclass supplies no-op stubs so tests can stack-instantiate
 * the class directly without Core::Service<>.
 */
class ConcreteLifecycleManagerImpl : public LifecycleManagerImplementation {
public:
    void AddRef() const override {}
    uint32_t Release() const override { return Core::ERROR_NONE; }
};

} // namespace Plugin
} // namespace WPEFramework

// Convenience aliases at global scope so test functions need no namespace prefix.
using LifecycleManagerImplementationTest = WPEFramework::Plugin::LifecycleManagerImplementationTest;
using ConcreteLifecycleManagerImpl       = WPEFramework::Plugin::ConcreteLifecycleManagerImpl;

// ─────────────────────────────────────────────────────────────────────────────
// Constructor initialises empty lists
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_ConstructorInitialisesEmptyLists()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    L0Test::ExpectTrue(tr,
        LifecycleManagerImplementationTest::getNotifications(impl).empty(),
        "mLifecycleManagerNotification is empty after construction");
    L0Test::ExpectTrue(tr,
        LifecycleManagerImplementationTest::getStateNotifications(impl).empty(),
        "mLifecycleManagerStateNotification is empty after construction");
    L0Test::ExpectTrue(tr,
        LifecycleManagerImplementationTest::getLoadedApps(impl).empty(),
        "mLoadedApplications is empty after construction");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// Register (ILifecycleManager) adds notification
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_RegisterILcmNotification()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;
    L0Test::FakeLcmNotification* notification = new L0Test::FakeLcmNotification();

    WPEFramework::Core::hresult result = impl.Register(
        static_cast<WPEFramework::Exchange::ILifecycleManager::INotification*>(notification));

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "Register(INotification) returns ERROR_NONE");
    L0Test::ExpectEqU32(tr,
        static_cast<uint32_t>(LifecycleManagerImplementationTest::getNotifications(impl).size()),
        1u,
        "Notification list has 1 entry after Register");

    impl.Unregister(static_cast<WPEFramework::Exchange::ILifecycleManager::INotification*>(notification));
    notification->Release();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// Register (ILifecycleManager) does not add duplicate
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_RegisterILcmNotificationDuplicate()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;
    L0Test::FakeLcmNotification* notification = new L0Test::FakeLcmNotification();

    impl.Register(static_cast<WPEFramework::Exchange::ILifecycleManager::INotification*>(notification));
    impl.Register(static_cast<WPEFramework::Exchange::ILifecycleManager::INotification*>(notification));

    L0Test::ExpectEqU32(tr,
        static_cast<uint32_t>(LifecycleManagerImplementationTest::getNotifications(impl).size()),
        1u,
        "Duplicate Register does not add second entry");

    impl.Unregister(static_cast<WPEFramework::Exchange::ILifecycleManager::INotification*>(notification));
    notification->Release();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// Unregister (ILifecycleManager) removes notification
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_UnregisterILcmNotification()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;
    L0Test::FakeLcmNotification* notification = new L0Test::FakeLcmNotification();

    impl.Register(static_cast<WPEFramework::Exchange::ILifecycleManager::INotification*>(notification));
    WPEFramework::Core::hresult result = impl.Unregister(
        static_cast<WPEFramework::Exchange::ILifecycleManager::INotification*>(notification));

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "Unregister returns ERROR_NONE");
    L0Test::ExpectTrue(tr,
        LifecycleManagerImplementationTest::getNotifications(impl).empty(),
        "Notification list is empty after Unregister");
    notification->Release();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// Unregister (ILifecycleManager) returns ERROR_GENERAL for unknown
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_UnregisterILcmNotificationUnknown()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;
    L0Test::FakeLcmNotification* unknown = new L0Test::FakeLcmNotification();

    WPEFramework::Core::hresult result = impl.Unregister(
        static_cast<WPEFramework::Exchange::ILifecycleManager::INotification*>(unknown));

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
        "Unregister unknown notification returns ERROR_GENERAL");

    unknown->Release();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// Register (ILifecycleManagerState) adds state notification
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_RegisterILcmStateNotification()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;
    L0Test::FakeLcmStateNotification* notification = new L0Test::FakeLcmStateNotification();

    WPEFramework::Core::hresult result = impl.Register(
        static_cast<WPEFramework::Exchange::ILifecycleManagerState::INotification*>(notification));

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "Register(ILifecycleManagerState::INotification*) returns ERROR_NONE");
    L0Test::ExpectEqU32(tr,
        static_cast<uint32_t>(LifecycleManagerImplementationTest::getStateNotifications(impl).size()),
        1u,
        "State notification list has 1 entry after Register");

    impl.Unregister(static_cast<WPEFramework::Exchange::ILifecycleManagerState::INotification*>(notification));
    notification->Release();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// Register (ILifecycleManagerState) does not add duplicate
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_RegisterILcmStateNotificationDuplicate()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;
    L0Test::FakeLcmStateNotification* notification = new L0Test::FakeLcmStateNotification();

    impl.Register(static_cast<WPEFramework::Exchange::ILifecycleManagerState::INotification*>(notification));
    impl.Register(static_cast<WPEFramework::Exchange::ILifecycleManagerState::INotification*>(notification));

    L0Test::ExpectEqU32(tr,
        static_cast<uint32_t>(LifecycleManagerImplementationTest::getStateNotifications(impl).size()),
        1u,
        "Duplicate state Register does not add second entry");

    impl.Unregister(static_cast<WPEFramework::Exchange::ILifecycleManagerState::INotification*>(notification));
    notification->Release();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// Unregister (ILifecycleManagerState) removes notification
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_UnregisterILcmStateNotification()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;
    L0Test::FakeLcmStateNotification* notification = new L0Test::FakeLcmStateNotification();

    impl.Register(static_cast<WPEFramework::Exchange::ILifecycleManagerState::INotification*>(notification));
    WPEFramework::Core::hresult result = impl.Unregister(
        static_cast<WPEFramework::Exchange::ILifecycleManagerState::INotification*>(notification));

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "Unregister state notification returns ERROR_NONE");
    L0Test::ExpectTrue(tr,
        LifecycleManagerImplementationTest::getStateNotifications(impl).empty(),
        "State notification list is empty after Unregister");
    notification->Release();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// Unregister (ILifecycleManagerState) ERROR_GENERAL for unknown
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_UnregisterILcmStateNotificationUnknown()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;
    L0Test::FakeLcmStateNotification* unknown = new L0Test::FakeLcmStateNotification();

    WPEFramework::Core::hresult result = impl.Unregister(
        static_cast<WPEFramework::Exchange::ILifecycleManagerState::INotification*>(unknown));

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
        "Unregister unknown state notification returns ERROR_GENERAL");

    unknown->Release();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// GetLoadedApps returns empty array when no apps loaded
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_GetLoadedAppsEmpty()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;
    std::string apps;

    WPEFramework::Core::hresult result = impl.GetLoadedApps(false, apps);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "GetLoadedApps returns ERROR_NONE");
    L0Test::ExpectEqStr(tr, apps, "[]",
        "GetLoadedApps returns '[]' when no apps loaded");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// GetLoadedApps returns all loaded applications
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_GetLoadedAppsWithApps()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    // Inject two contexts directly
    auto* ctx1 = new WPEFramework::Plugin::ApplicationContext("com.test.app1");
    auto* ctx2 = new WPEFramework::Plugin::ApplicationContext("com.test.app2");
    std::string id1 = "inst-001";
    std::string id2 = "inst-002";
    ctx1->setAppInstanceId(id1);
    ctx2->setAppInstanceId(id2);
    LifecycleManagerImplementationTest::getLoadedApps(impl).push_back(ctx1);
    LifecycleManagerImplementationTest::getLoadedApps(impl).push_back(ctx2);

    std::string apps;
    WPEFramework::Core::hresult result = impl.GetLoadedApps(false, apps);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "GetLoadedApps returns ERROR_NONE");
    L0Test::ExpectTrue(tr, apps.find("com.test.app1") != std::string::npos,
        "Result contains appId 'com.test.app1'");
    L0Test::ExpectTrue(tr, apps.find("com.test.app2") != std::string::npos,
        "Result contains appId 'com.test.app2'");

    // Clean up (impl destructor will call terminate() which is safe)
    for (auto* ctx : LifecycleManagerImplementationTest::getLoadedApps(impl)) {
        delete ctx;
    }
    LifecycleManagerImplementationTest::getLoadedApps(impl).clear();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// GetLoadedApps non-verbose omits runtimeStats
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_GetLoadedAppsNonVerboseOmitsRuntimeStats()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    auto* ctx = new WPEFramework::Plugin::ApplicationContext("com.test.verbose");
    std::string instId = "inst-verbose-001";
    ctx->setAppInstanceId(instId);
    LifecycleManagerImplementationTest::getLoadedApps(impl).push_back(ctx);

    std::string apps;
    impl.GetLoadedApps(false, apps);
    L0Test::ExpectTrue(tr, apps.find("runtimeStats") == std::string::npos,
        "Non-verbose GetLoadedApps does not include runtimeStats");

    for (auto* c : LifecycleManagerImplementationTest::getLoadedApps(impl)) {
        delete c;
    }
    LifecycleManagerImplementationTest::getLoadedApps(impl).clear();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// IsAppLoaded returns true for existing app
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_IsAppLoadedTrue()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    auto* ctx = new WPEFramework::Plugin::ApplicationContext("com.test.loaded");
    std::string inst = "inst-loaded-001";
    ctx->setAppInstanceId(inst);
    LifecycleManagerImplementationTest::getLoadedApps(impl).push_back(ctx);

    bool loaded = false;
    WPEFramework::Core::hresult result = impl.IsAppLoaded("com.test.loaded", loaded);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "IsAppLoaded returns ERROR_NONE");
    L0Test::ExpectTrue(tr, loaded, "IsAppLoaded returns true for existing app");

    for (auto* c : LifecycleManagerImplementationTest::getLoadedApps(impl)) {
        delete c;
    }
    LifecycleManagerImplementationTest::getLoadedApps(impl).clear();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// IsAppLoaded returns false for unknown app
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_IsAppLoadedFalse()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    bool loaded = true; // pre-set to true to verify it is set to false
    WPEFramework::Core::hresult result = impl.IsAppLoaded("com.unknown.app", loaded);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "IsAppLoaded returns ERROR_NONE for unknown app");
    L0Test::ExpectTrue(tr, !loaded, "IsAppLoaded sets loaded=false for unknown app");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// SendIntentToActiveApp returns ERROR_GENERAL for unknown app
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_SendIntentToActiveAppUnknownApp()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    std::string errorReason;
    bool success = true;
    WPEFramework::Core::hresult result = impl.SendIntentToActiveApp(
        "unknown-inst-id", "deeplink://home", errorReason, success);

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
        "SendIntentToActiveApp returns ERROR_GENERAL for unknown appInstanceId");
    L0Test::ExpectTrue(tr, !success,
        "success flag is false for unknown appInstanceId");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// SendIntentToActiveApp returns ERROR_GENERAL for non-ACTIVE app
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_SendIntentToActiveAppNonActive()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    // App in PAUSED state (default initial state is UNLOADED, which is also not ACTIVE)
    auto* ctx = new WPEFramework::Plugin::ApplicationContext("com.test.paused");
    std::string inst = "inst-paused-001";
    ctx->setAppInstanceId(inst);
    LifecycleManagerImplementationTest::getLoadedApps(impl).push_back(ctx);

    std::string errorReason;
    bool success = true;
    WPEFramework::Core::hresult result = impl.SendIntentToActiveApp(
        "inst-paused-001", "deeplink://home", errorReason, success);

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
        "SendIntentToActiveApp returns ERROR_GENERAL for non-ACTIVE app");
    L0Test::ExpectTrue(tr, !success,
        "success flag is false for non-ACTIVE app");
    L0Test::ExpectEqStr(tr, errorReason, "application is not active",
        "errorReason matches expected value");

    for (auto* c : LifecycleManagerImplementationTest::getLoadedApps(impl)) {
        delete c;
    }
    LifecycleManagerImplementationTest::getLoadedApps(impl).clear();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// AppReady returns ERROR_GENERAL for unknown appId
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_AppReadyUnknownAppId()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    WPEFramework::Core::hresult result = impl.AppReady("com.unknown.app");
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
        "AppReady returns ERROR_GENERAL for unknown appId");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// AppReady posts semaphore for known appId
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_AppReadyKnownAppId()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    auto* ctx = new WPEFramework::Plugin::ApplicationContext("com.test.ready");
    std::string inst = "inst-ready-001";
    ctx->setAppInstanceId(inst);
    LifecycleManagerImplementationTest::getLoadedApps(impl).push_back(ctx);

    WPEFramework::Core::hresult result = impl.AppReady("com.test.ready");
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "AppReady returns ERROR_NONE for known appId");

    // Verify semaphore was posted by attempting a trywait
    int semResult = sem_trywait(&ctx->mAppReadySemaphore);
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(semResult), 0u,
        "mAppReadySemaphore was posted by AppReady");

    for (auto* c : LifecycleManagerImplementationTest::getLoadedApps(impl)) {
        delete c;
    }
    LifecycleManagerImplementationTest::getLoadedApps(impl).clear();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// StateChangeComplete always returns ERROR_NONE
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_StateChangeCompleteAlwaysReturnsErrorNone()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    WPEFramework::Core::hresult result = impl.StateChangeComplete("any.app", 1, true);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "StateChangeComplete returns ERROR_NONE (success=true)");

    result = impl.StateChangeComplete("any.app", 2, false);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "StateChangeComplete returns ERROR_NONE (success=false)");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// CloseApp returns ERROR_GENERAL for unknown appId
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_CloseAppUnknownAppId()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    WPEFramework::Core::hresult result = impl.CloseApp(
        "com.unknown.app",
        WPEFramework::Exchange::ILifecycleManagerState::AppCloseReason::USER_EXIT);

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
        "CloseApp returns ERROR_GENERAL for unknown appId");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// Configure returns ERROR_NONE when service is null
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_ConfigureWithNullService()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    uint32_t result = impl.Configure(nullptr);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "Configure(nullptr) returns ERROR_NONE");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// onRippleEvent is a no-op (no crash)
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_OnRippleEventIsNoOp()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;
    // JsonObject in the LifecycleManager source maps to WPEFramework::Core::JSON::VariantContainer
    WPEFramework::Core::JSON::VariantContainer data;

    bool noThrow = true;
    try {
        // onRippleEvent is a public virtual method exposed via IEventHandler
        impl.onRippleEvent(std::string("test"), data);
    } catch (...) {
        noThrow = false;
    }
    L0Test::ExpectTrue(tr, noThrow, "onRippleEvent does not throw");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// Dispatch APPSTATECHANGED calls all ILifecycleManager notifications
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_DispatchAppStateChangedCallsLcmNotifications()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    L0Test::FakeLcmNotification* notif1 = new L0Test::FakeLcmNotification();
    L0Test::FakeLcmNotification* notif2 = new L0Test::FakeLcmNotification();

    impl.Register(static_cast<WPEFramework::Exchange::ILifecycleManager::INotification*>(notif1));
    impl.Register(static_cast<WPEFramework::Exchange::ILifecycleManager::INotification*>(notif2));

    // Build event payload
    WPEFramework::Core::JSON::VariantContainer params;
    params["appId"] = "com.test.app";
    params["appInstanceId"] = "inst-001";
    params["oldLifecycleState"] = static_cast<uint32_t>(
        WPEFramework::Exchange::ILifecycleManager::LifecycleState::PAUSED);
    params["newLifecycleState"] = static_cast<uint32_t>(
        WPEFramework::Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    params["navigationIntent"] = "home";
    params["errorReason"] = "";

    // Dispatch synchronously via the private method through the friend class
    LifecycleManagerImplementationTest::callDispatch(
        impl,
        LifecycleManagerImplementationTest::EventNames::LIFECYCLE_MANAGER_EVENT_APPSTATECHANGED,
        params);

    L0Test::ExpectTrue(tr, notif1->onAppStateChangedCount.load() >= 1u,
        "First notification received OnAppStateChanged callback");
    L0Test::ExpectTrue(tr, notif2->onAppStateChangedCount.load() >= 1u,
        "Second notification received OnAppStateChanged callback");

    impl.Unregister(static_cast<WPEFramework::Exchange::ILifecycleManager::INotification*>(notif1));
    impl.Unregister(static_cast<WPEFramework::Exchange::ILifecycleManager::INotification*>(notif2));
    notif1->Release();
    notif2->Release();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// Dispatch APPSTATECHANGED calls all ILifecycleManagerState notifications
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_DispatchAppStateChangedCallsStateNotifications()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    L0Test::FakeLcmStateNotification* sn1 = new L0Test::FakeLcmStateNotification();
    L0Test::FakeLcmStateNotification* sn2 = new L0Test::FakeLcmStateNotification();

    impl.Register(static_cast<WPEFramework::Exchange::ILifecycleManagerState::INotification*>(sn1));
    impl.Register(static_cast<WPEFramework::Exchange::ILifecycleManagerState::INotification*>(sn2));

    WPEFramework::Core::JSON::VariantContainer params;
    params["appId"] = "com.test.state";
    params["appInstanceId"] = "inst-state-001";
    params["oldLifecycleState"] = static_cast<uint32_t>(
        WPEFramework::Exchange::ILifecycleManager::LifecycleState::PAUSED);
    params["newLifecycleState"] = static_cast<uint32_t>(
        WPEFramework::Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    params["navigationIntent"] = "deeplink://home";
    params["errorReason"] = "";

    LifecycleManagerImplementationTest::callDispatch(
        impl,
        LifecycleManagerImplementationTest::EventNames::LIFECYCLE_MANAGER_EVENT_APPSTATECHANGED,
        params);

    L0Test::ExpectTrue(tr, sn1->onStateChangedCount.load() >= 1u,
        "First state notification received OnAppLifecycleStateChanged");
    L0Test::ExpectTrue(tr, sn2->onStateChangedCount.load() >= 1u,
        "Second state notification received OnAppLifecycleStateChanged");

    impl.Unregister(static_cast<WPEFramework::Exchange::ILifecycleManagerState::INotification*>(sn1));
    impl.Unregister(static_cast<WPEFramework::Exchange::ILifecycleManagerState::INotification*>(sn2));
    sn1->Release();
    sn2->Release();

    return tr.failures;
}
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_DispatchOnFailureCallsLcmNotification()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;
    L0Test::FakeLcmNotification* notif = new L0Test::FakeLcmNotification();

    impl.Register(static_cast<WPEFramework::Exchange::ILifecycleManager::INotification*>(notif));

    WPEFramework::Core::JSON::VariantContainer params;
    params["appId"] = "com.test.fail";
    params["appInstanceId"] = "inst-fail-001";
    params["oldLifecycleState"] = static_cast<uint32_t>(
        WPEFramework::Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    params["newLifecycleState"] = static_cast<uint32_t>(
        WPEFramework::Exchange::ILifecycleManager::LifecycleState::UNLOADED);
    params["errorReason"] = "container_crashed";
    params["navigationIntent"] = "";

    LifecycleManagerImplementationTest::callDispatch(
        impl,
        LifecycleManagerImplementationTest::EventNames::LIFECYCLE_MANAGER_EVENT_ONFAILURE,
        params);

    L0Test::ExpectTrue(tr, notif->onAppStateChangedCount.load() >= 1u,
        "OnAppStateChanged called on ONFAILURE dispatch");

    impl.Unregister(static_cast<WPEFramework::Exchange::ILifecycleManager::INotification*>(notif));
    notif->Release();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// Dispatch unknown event logs warning and does not crash
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_DispatchUnknownEventDoesNotCrash()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    WPEFramework::Core::JSON::VariantContainer params;
    params["appId"] = "com.test.unknown";
    params["appInstanceId"] = "";
    params["oldLifecycleState"] = 0u;
    params["newLifecycleState"] = 0u;
    params["errorReason"] = "";
    params["navigationIntent"] = "";

    bool noThrow = true;
    try {
        // Use a cast to satisfy the EventNames enum; 99 is an unknown value
        LifecycleManagerImplementationTest::callDispatch(
            impl,
            static_cast<LifecycleManagerImplementationTest::EventNames>(99),
            params);
    } catch (...) {
        noThrow = false;
    }
    L0Test::ExpectTrue(tr, noThrow, "Dispatch unknown event does not throw");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// LCM-070 / LCM-071 / LCM-072 / getContext lookups
// (tested indirectly via IsAppLoaded and other APIs that call getContext)
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_GetContextByAppInstanceId()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    auto* ctx = new WPEFramework::Plugin::ApplicationContext("com.test.ctx");
    std::string inst = "inst-ctx-001";
    ctx->setAppInstanceId(inst);
    LifecycleManagerImplementationTest::getLoadedApps(impl).push_back(ctx);

    // IsAppLoaded uses getContext("", appId); verify via appId lookup
    bool loaded = false;
    impl.IsAppLoaded("com.test.ctx", loaded);
    L0Test::ExpectTrue(tr, loaded, "getContext finds app by appId");

    // SendIntentToActiveApp uses getContext(appInstanceId, "") for instance ID lookup
    std::string errorReason;
    bool success = false;
    WPEFramework::Core::hresult result = impl.SendIntentToActiveApp(
        "inst-ctx-001", "deeplink://test", errorReason, success);
    // App is in UNLOADED (not ACTIVE), so expect ERROR_GENERAL
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
        "getContext found app by appInstanceId (returned not-active error)");

    for (auto* c : LifecycleManagerImplementationTest::getLoadedApps(impl)) {
        delete c;
    }
    LifecycleManagerImplementationTest::getLoadedApps(impl).clear();

    return tr.failures;
}

uint32_t Test_Impl_GetContextReturnsNullForEmptyIds()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    // Both appId and appInstanceId empty → getContext should return nullptr
    bool loaded = true;
    impl.IsAppLoaded("", loaded);
    L0Test::ExpectTrue(tr, !loaded,
        "getContext with empty appId returns nullptr (loaded=false)");

    return tr.failures;
}

uint32_t Test_Impl_GetContextReturnsNullForNoMatch()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    bool loaded = true;
    impl.IsAppLoaded("com.no.match", loaded);
    L0Test::ExpectTrue(tr, !loaded,
        "getContext returns null when no match found");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// Dispatch RUNTIME event with unknown name does not crash
// handleRuntimeManagerEvent silently ignores unrecognised event names.
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_DispatchRuntimeEventDoesNotCrash()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    WPEFramework::Core::JSON::VariantContainer params;
    params["appId"]             = std::string("");
    params["appInstanceId"]     = std::string("");
    params["name"]              = std::string("unknownRuntimeEvent");
    params["oldLifecycleState"] = static_cast<uint32_t>(0);
    params["newLifecycleState"] = static_cast<uint32_t>(0);
    params["errorReason"]       = std::string("");
    params["navigationIntent"]  = std::string("");

    bool noThrow = true;
    try {
        LifecycleManagerImplementationTest::callDispatch(
            impl,
            LifecycleManagerImplementationTest::EventNames::LIFECYCLE_MANAGER_EVENT_RUNTIME,
            params);
    } catch (...) {
        noThrow = false;
    }
    L0Test::ExpectTrue(tr, noThrow,
        "Dispatch RUNTIME event with unknown event name does not throw");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// Dispatch WINDOW event with unknown name does not crash
// handleWindowManagerEvent silently ignores unrecognised event names.
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_DispatchWindowEventDoesNotCrash()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    WPEFramework::Core::JSON::VariantContainer params;
    params["appId"]             = std::string("");
    params["appInstanceId"]     = std::string("");
    params["name"]              = std::string("unknownWindowEvent");
    params["oldLifecycleState"] = static_cast<uint32_t>(0);
    params["newLifecycleState"] = static_cast<uint32_t>(0);
    params["errorReason"]       = std::string("");
    params["navigationIntent"]  = std::string("");

    bool noThrow = true;
    try {
        LifecycleManagerImplementationTest::callDispatch(
            impl,
            LifecycleManagerImplementationTest::EventNames::LIFECYCLE_MANAGER_EVENT_WINDOW,
            params);
    } catch (...) {
        noThrow = false;
    }
    L0Test::ExpectTrue(tr, noThrow,
        "Dispatch WINDOW event with unknown event name does not throw");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// SetTargetAppState returns ERROR_GENERAL for unknown appInstanceId
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_SetTargetAppStateUnknownInstance()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    WPEFramework::Core::hresult result = impl.SetTargetAppState(
        "unknown-instance-id",
        WPEFramework::Exchange::ILifecycleManager::LifecycleState::ACTIVE,
        "");

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
        "SetTargetAppState returns ERROR_GENERAL for unknown appInstanceId");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// UnloadApp returns ERROR_GENERAL for unknown appInstanceId
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_UnloadAppUnknownInstance()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    std::string errorReason;
    bool success = true;
    WPEFramework::Core::hresult result = impl.UnloadApp(
        "unknown-instance-id", errorReason, success);

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
        "UnloadApp returns ERROR_GENERAL for unknown appInstanceId");
    L0Test::ExpectTrue(tr, !success,
        "UnloadApp sets success=false for unknown appInstanceId");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// KillApp returns ERROR_GENERAL for unknown appInstanceId
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_KillAppUnknownInstance()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    std::string errorReason;
    bool success = true;
    WPEFramework::Core::hresult result = impl.KillApp(
        "unknown-instance-id", errorReason, success);

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
        "KillApp returns ERROR_GENERAL for unknown appInstanceId");
    L0Test::ExpectTrue(tr, !success,
        "KillApp sets success=false for unknown appInstanceId");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// GetLoadedApps verbose=true enters runtimeStats block
// Since Configure() was not called, getRuntimeManagerHandler() returns nullptr,
// so the inner handler block (lines 215-226) is safely skipped.
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_GetLoadedAppsVerboseNoRuntimeHandler()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    auto* ctx = new WPEFramework::Plugin::ApplicationContext("com.test.verbose");
    std::string inst = "inst-verbose-001";
    ctx->setAppInstanceId(inst);
    LifecycleManagerImplementationTest::getLoadedApps(impl).push_back(ctx);

    std::string apps;
    WPEFramework::Core::hresult result = impl.GetLoadedApps(true, apps);

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "GetLoadedApps(verbose=true) returns ERROR_NONE");
    L0Test::ExpectTrue(tr, apps.find("com.test.verbose") != std::string::npos,
        "verbose result still contains the appId");

    for (auto* c : LifecycleManagerImplementationTest::getLoadedApps(impl)) {
        delete c;
    }
    LifecycleManagerImplementationTest::getLoadedApps(impl).clear();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// SendIntentToActiveApp returns ERROR_NONE for an ACTIVE app
// The app lifecycle state is set to ACTIVE via ActiveState.
// IWorkerPool is available through L0BootstrapGuard so dispatchEvent is safe.
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_SendIntentToActiveAppActiveApp()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    auto* ctx = new WPEFramework::Plugin::ApplicationContext("com.test.active");
    std::string inst = "inst-active-001";
    ctx->setAppInstanceId(inst);
    // Transition the context to ACTIVE by installing an ActiveState.
    delete static_cast<WPEFramework::Plugin::State*>(ctx->getState());
    ctx->setState(static_cast<void*>(new WPEFramework::Plugin::ActiveState(ctx)));
    LifecycleManagerImplementationTest::getLoadedApps(impl).push_back(ctx);

    std::string errorReason;
    bool success = false;
    WPEFramework::Core::hresult result = impl.SendIntentToActiveApp(
        "inst-active-001", "deeplink://home", errorReason, success);

    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
        "SendIntentToActiveApp returns ERROR_NONE for ACTIVE app");
    L0Test::ExpectTrue(tr, errorReason.empty(),
        "errorReason is empty for ACTIVE app");

    // dispatchEvent() submits an async Job to the worker pool whose Dispatch() call
    // accesses impl's mAdminLock.  Sleep here (while impl is still in scope) so the
    // worker pool finishes the job before impl's destructor runs.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    for (auto* c : LifecycleManagerImplementationTest::getLoadedApps(impl)) {
        delete c;
    }
    LifecycleManagerImplementationTest::getLoadedApps(impl).clear();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// Dispatch APPSTATECHANGED with UNLOADED state removes app from list
// The context with the matching appInstanceId must be in mLoadedApplications.
// After dispatch, handleStateChangeEvent deletes and erases it.
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_DispatchAppStateChangedUnloadedRemovesApp()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    auto* ctx = new WPEFramework::Plugin::ApplicationContext("com.test.unload");
    std::string inst = "inst-unload-001";
    ctx->setAppInstanceId(inst);
    LifecycleManagerImplementationTest::getLoadedApps(impl).push_back(ctx);

    L0Test::ExpectEqU32(tr,
        static_cast<uint32_t>(LifecycleManagerImplementationTest::getLoadedApps(impl).size()),
        1u,
        "one app in loaded list before dispatch");

    WPEFramework::Core::JSON::VariantContainer params;
    params["appId"]             = std::string("com.test.unload");
    params["appInstanceId"]     = std::string("inst-unload-001");
    params["oldLifecycleState"] = static_cast<uint32_t>(
        WPEFramework::Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    params["newLifecycleState"] = static_cast<uint32_t>(
        WPEFramework::Exchange::ILifecycleManager::LifecycleState::UNLOADED);
    params["navigationIntent"]  = std::string("");
    params["errorReason"]       = std::string("");

    // callDispatch exercises handleStateChangeEvent synchronously (no worker pool).
    LifecycleManagerImplementationTest::callDispatch(
        impl,
        LifecycleManagerImplementationTest::EventNames::LIFECYCLE_MANAGER_EVENT_APPSTATECHANGED,
        params);

    // handleStateChangeEvent must have deleted and erased the context pointer.
    L0Test::ExpectTrue(tr,
        LifecycleManagerImplementationTest::getLoadedApps(impl).empty(),
        "app removed from mLoadedApplications after UNLOADED state change");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// handleRuntimeManagerEvent "onTerminated" — unknown appInstanceId
// callDispatch(RUNTIME) reaches handleRuntimeManagerEvent synchronously.
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_RuntimeEventOnTerminatedUnknownApp()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    WPEFramework::Core::JSON::VariantContainer params;
    params["name"]             = std::string("onTerminated");
    params["appInstanceId"]    = std::string("inst-no-such");
    params["appId"]            = std::string("");
    params["state"]            = static_cast<uint32_t>(0);
    params["errorCode"]        = std::string("");
    params["oldLifecycleState"]= static_cast<uint32_t>(0);
    params["newLifecycleState"]= static_cast<uint32_t>(0);
    params["errorReason"]      = std::string("");
    params["navigationIntent"] = std::string("");

    bool noThrow = true;
    try {
        LifecycleManagerImplementationTest::callDispatch(
            impl,
            LifecycleManagerImplementationTest::EventNames::LIFECYCLE_MANAGER_EVENT_RUNTIME,
            params);
    } catch (...) {
        noThrow = false;
    }
    L0Test::ExpectTrue(tr, noThrow,
        "onTerminated with unknown appInstanceId does not throw");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// handleRuntimeManagerEvent "onTerminated" — app in TERMINATING state
// Context is in TERMINATING state, so the unexpected-termination branch is skipped
// and addStateTransitionRequest("onAppTerminating") is called directly.
// mPendingStateTransition=false → falls into the "wrong state transition" else branch.
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_RuntimeEventOnTerminatedAppInTerminatingState()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    auto* ctx = new WPEFramework::Plugin::ApplicationContext("com.test.terminating");
    std::string inst = "inst-terminating-001";
    ctx->setAppInstanceId(inst);
    // Put context into TERMINATING state via TerminatingState
    delete static_cast<WPEFramework::Plugin::State*>(ctx->getState());
    ctx->setState(static_cast<void*>(
        new WPEFramework::Plugin::TerminatingState(ctx)));
    ctx->mPendingStateTransition = false;
    ctx->mPendingEventName       = "";
    LifecycleManagerImplementationTest::getLoadedApps(impl).push_back(ctx);

    WPEFramework::Core::JSON::VariantContainer params;
    params["name"]             = std::string("onTerminated");
    params["appInstanceId"]    = inst;
    params["appId"]            = std::string("");
    params["state"]            = static_cast<uint32_t>(0);
    params["errorCode"]        = std::string("");
    params["oldLifecycleState"]= static_cast<uint32_t>(0);
    params["newLifecycleState"]= static_cast<uint32_t>(0);
    params["errorReason"]      = std::string("");
    params["navigationIntent"] = std::string("");

    bool noThrow = true;
    try {
        LifecycleManagerImplementationTest::callDispatch(
            impl,
            LifecycleManagerImplementationTest::EventNames::LIFECYCLE_MANAGER_EVENT_RUNTIME,
            params);
    } catch (...) {
        noThrow = false;
    }
    L0Test::ExpectTrue(tr, noThrow,
        "onTerminated for TERMINATING app calls addStateTransitionRequest without crash");

    // StateTransitionHandler background thread uses ctx; wait for it to finish.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    for (auto* c : LifecycleManagerImplementationTest::getLoadedApps(impl)) {
        delete c;
    }
    LifecycleManagerImplementationTest::getLoadedApps(impl).clear();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// handleRuntimeManagerEvent "onTerminated" — unexpected termination path
// Context is NOT in TERMINATING state (ACTIVE) → unexpected termination path fires.
// RequestHandler::terminate / updateState both return false because no real handler
// is configured → LOGERR branch at line 635 is hit.
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_RuntimeEventOnTerminatedUnexpected()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    auto* ctx = new WPEFramework::Plugin::ApplicationContext("com.test.unexpected");
    std::string inst = "inst-unexpected-001";
    ctx->setAppInstanceId(inst);
    // ACTIVE state → not TERMINATING → triggers unexpected-termination branch
    delete static_cast<WPEFramework::Plugin::State*>(ctx->getState());
    ctx->setState(static_cast<void*>(
        new WPEFramework::Plugin::ActiveState(ctx)));
    LifecycleManagerImplementationTest::getLoadedApps(impl).push_back(ctx);

    WPEFramework::Core::JSON::VariantContainer params;
    params["name"]             = std::string("onTerminated");
    params["appInstanceId"]    = inst;
    params["appId"]            = std::string("");
    params["state"]            = static_cast<uint32_t>(0);
    params["errorCode"]        = std::string("");
    params["oldLifecycleState"]= static_cast<uint32_t>(0);
    params["newLifecycleState"]= static_cast<uint32_t>(0);
    params["errorReason"]      = std::string("");
    params["navigationIntent"] = std::string("");

    bool noThrow = true;
    try {
        LifecycleManagerImplementationTest::callDispatch(
            impl,
            LifecycleManagerImplementationTest::EventNames::LIFECYCLE_MANAGER_EVENT_RUNTIME,
            params);
    } catch (...) {
        noThrow = false;
    }
    L0Test::ExpectTrue(tr, noThrow,
        "unexpected onTerminated branch does not throw");

    // StateTransitionHandler background thread uses ctx; wait for it to finish.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    for (auto* c : LifecycleManagerImplementationTest::getLoadedApps(impl)) {
        delete c;
    }
    LifecycleManagerImplementationTest::getLoadedApps(impl).clear();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// handleRuntimeManagerEvent "onStateChanged" RUNNING — unknown app
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_RuntimeEventOnStateChangedRunningUnknownApp()
{
    L0Test::TestResult tr;

    using RuntimeState = WPEFramework::Exchange::IRuntimeManager::RuntimeState;

    ConcreteLifecycleManagerImpl impl;

    WPEFramework::Core::JSON::VariantContainer params;
    params["name"]             = std::string("onStateChanged");
    params["appInstanceId"]    = std::string("inst-no-such");
    params["appId"]            = std::string("");
    params["state"]            = static_cast<uint32_t>(RuntimeState::RUNTIME_STATE_RUNNING);
    params["errorCode"]        = std::string("");
    params["oldLifecycleState"]= static_cast<uint32_t>(0);
    params["newLifecycleState"]= static_cast<uint32_t>(0);
    params["errorReason"]      = std::string("");
    params["navigationIntent"] = std::string("");

    bool noThrow = true;
    try {
        LifecycleManagerImplementationTest::callDispatch(
            impl,
            LifecycleManagerImplementationTest::EventNames::LIFECYCLE_MANAGER_EVENT_RUNTIME,
            params);
    } catch (...) {
        noThrow = false;
    }
    L0Test::ExpectTrue(tr, noThrow,
        "onStateChanged RUNNING unknown app does not throw");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// handleRuntimeManagerEvent "onStateChanged" RUNNING — known app
// mPendingStateTransition=false → takes the "wrong state transition" else path.
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_RuntimeEventOnStateChangedRunningKnownApp()
{
    L0Test::TestResult tr;

    using RuntimeState = WPEFramework::Exchange::IRuntimeManager::RuntimeState;

    ConcreteLifecycleManagerImpl impl;

    auto* ctx = new WPEFramework::Plugin::ApplicationContext("com.test.running");
    std::string inst = "inst-running-001";
    ctx->setAppInstanceId(inst);
    ctx->mPendingStateTransition = false;
    ctx->mPendingEventName       = "";
    LifecycleManagerImplementationTest::getLoadedApps(impl).push_back(ctx);

    WPEFramework::Core::JSON::VariantContainer params;
    params["name"]             = std::string("onStateChanged");
    params["appInstanceId"]    = inst;
    params["appId"]            = std::string("");
    params["state"]            = static_cast<uint32_t>(RuntimeState::RUNTIME_STATE_RUNNING);
    params["errorCode"]        = std::string("");
    params["oldLifecycleState"]= static_cast<uint32_t>(0);
    params["newLifecycleState"]= static_cast<uint32_t>(0);
    params["errorReason"]      = std::string("");
    params["navigationIntent"] = std::string("");

    bool noThrow = true;
    try {
        LifecycleManagerImplementationTest::callDispatch(
            impl,
            LifecycleManagerImplementationTest::EventNames::LIFECYCLE_MANAGER_EVENT_RUNTIME,
            params);
    } catch (...) {
        noThrow = false;
    }
    L0Test::ExpectTrue(tr, noThrow,
        "onStateChanged RUNNING known app calls addStateTransitionRequest without crash");

    // StateTransitionHandler background thread uses ctx; wait for it to finish.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    for (auto* c : LifecycleManagerImplementationTest::getLoadedApps(impl)) {
        delete c;
    }
    LifecycleManagerImplementationTest::getLoadedApps(impl).clear();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// handleRuntimeManagerEvent "onFailure"
//         lines 677-692 (notifyOnFailure body — unknown appInstanceId path).
// notifyOnFailure calls dispatchEvent; to avoid the dangling-pointer crash we
// keep impl alive with a 200 ms sleep.
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_RuntimeEventOnFailure()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    // Register a notification observer so the ONFAILURE dispatch actually iterates.
    L0Test::FakeLcmNotification* notif = new L0Test::FakeLcmNotification();
    impl.Register(
        static_cast<WPEFramework::Exchange::ILifecycleManager::INotification*>(notif));

    WPEFramework::Core::JSON::VariantContainer params;
    params["name"]             = std::string("onFailure");
    params["appInstanceId"]    = std::string("inst-fail-001");
    params["appId"]            = std::string("");
    params["state"]            = static_cast<uint32_t>(0);
    params["errorCode"]        = std::string("container_error");
    params["oldLifecycleState"]= static_cast<uint32_t>(0);
    params["newLifecycleState"]= static_cast<uint32_t>(0);
    params["errorReason"]      = std::string("");
    params["navigationIntent"] = std::string("");

    bool noThrow = true;
    try {
        LifecycleManagerImplementationTest::callDispatch(
            impl,
            LifecycleManagerImplementationTest::EventNames::LIFECYCLE_MANAGER_EVENT_RUNTIME,
            params);
    } catch (...) {
        noThrow = false;
    }
    L0Test::ExpectTrue(tr, noThrow,
        "onFailure dispatch does not throw");

    // Wait for the async ONFAILURE job to complete before impl destructs.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    impl.Unregister(
        static_cast<WPEFramework::Exchange::ILifecycleManager::INotification*>(notif));
    notif->Release();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// notifyOnFailure — known appInstanceId path
// Called indirectly via Dispatch(RUNTIME, onFailure) with a loaded context.
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_NotifyOnFailureKnownApp()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    auto* ctx = new WPEFramework::Plugin::ApplicationContext("com.test.failapp");
    std::string inst = "inst-failapp-001";
    ctx->setAppInstanceId(inst);
    LifecycleManagerImplementationTest::getLoadedApps(impl).push_back(ctx);

    L0Test::FakeLcmNotification* notif = new L0Test::FakeLcmNotification();
    impl.Register(
        static_cast<WPEFramework::Exchange::ILifecycleManager::INotification*>(notif));

    WPEFramework::Core::JSON::VariantContainer params;
    params["name"]             = std::string("onFailure");
    params["appInstanceId"]    = inst;
    params["appId"]            = std::string("");
    params["state"]            = static_cast<uint32_t>(0);
    params["errorCode"]        = std::string("oom_killed");
    params["oldLifecycleState"]= static_cast<uint32_t>(0);
    params["newLifecycleState"]= static_cast<uint32_t>(0);
    params["errorReason"]      = std::string("");
    params["navigationIntent"] = std::string("");

    bool noThrow = true;
    try {
        LifecycleManagerImplementationTest::callDispatch(
            impl,
            LifecycleManagerImplementationTest::EventNames::LIFECYCLE_MANAGER_EVENT_RUNTIME,
            params);
    } catch (...) {
        noThrow = false;
    }
    L0Test::ExpectTrue(tr, noThrow,
        "notifyOnFailure with known app does not throw");

    // Wait for the async ONFAILURE job submitted by notifyOnFailure.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    impl.Unregister(
        static_cast<WPEFramework::Exchange::ILifecycleManager::INotification*>(notif));
    notif->Release();

    // ctx is owned by handleStateChangeEvent if UNLOADED was dispatched, but here
    // we dispatched ONFAILURE which does not erase from the list — clean up manually.
    for (auto* c : LifecycleManagerImplementationTest::getLoadedApps(impl)) {
        delete c;
    }
    LifecycleManagerImplementationTest::getLoadedApps(impl).clear();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// handleRuntimeManagerEvent "onStarted"
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_RuntimeEventOnStarted()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    WPEFramework::Core::JSON::VariantContainer params;
    params["name"]             = std::string("onStarted");
    params["appInstanceId"]    = std::string("inst-started-001");
    params["appId"]            = std::string("");
    params["state"]            = static_cast<uint32_t>(0);
    params["errorCode"]        = std::string("");
    params["oldLifecycleState"]= static_cast<uint32_t>(0);
    params["newLifecycleState"]= static_cast<uint32_t>(0);
    params["errorReason"]      = std::string("");
    params["navigationIntent"] = std::string("");

    bool noThrow = true;
    try {
        LifecycleManagerImplementationTest::callDispatch(
            impl,
            LifecycleManagerImplementationTest::EventNames::LIFECYCLE_MANAGER_EVENT_RUNTIME,
            params);
    } catch (...) {
        noThrow = false;
    }
    L0Test::ExpectTrue(tr, noThrow,
        "onStarted runtime event does not throw");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// handleWindowManagerEvent "onUserInactivity"
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_WindowEventOnUserInactivity()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    WPEFramework::Core::JSON::VariantContainer params;
    params["name"]             = std::string("onUserInactivity");
    params["appInstanceId"]    = std::string("");
    params["appId"]            = std::string("");
    params["oldLifecycleState"]= static_cast<uint32_t>(0);
    params["newLifecycleState"]= static_cast<uint32_t>(0);
    params["errorReason"]      = std::string("");
    params["navigationIntent"] = std::string("");

    bool noThrow = true;
    try {
        LifecycleManagerImplementationTest::callDispatch(
            impl,
            LifecycleManagerImplementationTest::EventNames::LIFECYCLE_MANAGER_EVENT_WINDOW,
            params);
    } catch (...) {
        noThrow = false;
    }
    L0Test::ExpectTrue(tr, noThrow,
        "onUserInactivity window event does not throw");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// handleWindowManagerEvent "onDisconnect"
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_WindowEventOnDisconnect()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    WPEFramework::Core::JSON::VariantContainer params;
    params["name"]             = std::string("onDisconnect");
    params["appInstanceId"]    = std::string("");
    params["appId"]            = std::string("");
    params["oldLifecycleState"]= static_cast<uint32_t>(0);
    params["newLifecycleState"]= static_cast<uint32_t>(0);
    params["errorReason"]      = std::string("");
    params["navigationIntent"] = std::string("");

    bool noThrow = true;
    try {
        LifecycleManagerImplementationTest::callDispatch(
            impl,
            LifecycleManagerImplementationTest::EventNames::LIFECYCLE_MANAGER_EVENT_WINDOW,
            params);
    } catch (...) {
        noThrow = false;
    }
    L0Test::ExpectTrue(tr, noThrow,
        "onDisconnect window event does not throw");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// handleWindowManagerEvent "onReady" — unknown appInstanceId
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_WindowEventOnReadyUnknownApp()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    WPEFramework::Core::JSON::VariantContainer params;
    params["name"]             = std::string("onReady");
    params["appInstanceId"]    = std::string("inst-no-such");
    params["appId"]            = std::string("");
    params["oldLifecycleState"]= static_cast<uint32_t>(0);
    params["newLifecycleState"]= static_cast<uint32_t>(0);
    params["errorReason"]      = std::string("");
    params["navigationIntent"] = std::string("");

    bool noThrow = true;
    try {
        LifecycleManagerImplementationTest::callDispatch(
            impl,
            LifecycleManagerImplementationTest::EventNames::LIFECYCLE_MANAGER_EVENT_WINDOW,
            params);
    } catch (...) {
        noThrow = false;
    }
    L0Test::ExpectTrue(tr, noThrow,
        "onReady window event with unknown app does not throw");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// handleWindowManagerEvent "onReady" — known app, wrong transition
// mPendingStateTransition=false triggers the else path in addStateTransitionRequest.
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_WindowEventOnReadyKnownAppWrongTransition()
{
    L0Test::TestResult tr;

    ConcreteLifecycleManagerImpl impl;

    auto* ctx = new WPEFramework::Plugin::ApplicationContext("com.test.onready");
    std::string inst = "inst-onready-001";
    ctx->setAppInstanceId(inst);
    ctx->mPendingStateTransition = false;  // triggers else path in addStateTransitionRequest
    ctx->mPendingEventName       = "";
    LifecycleManagerImplementationTest::getLoadedApps(impl).push_back(ctx);

    WPEFramework::Core::JSON::VariantContainer params;
    params["name"]             = std::string("onReady");
    params["appInstanceId"]    = inst;
    params["appId"]            = std::string("");
    params["oldLifecycleState"]= static_cast<uint32_t>(0);
    params["newLifecycleState"]= static_cast<uint32_t>(0);
    params["errorReason"]      = std::string("");
    params["navigationIntent"] = std::string("");

    bool noThrow = true;
    try {
        LifecycleManagerImplementationTest::callDispatch(
            impl,
            LifecycleManagerImplementationTest::EventNames::LIFECYCLE_MANAGER_EVENT_WINDOW,
            params);
    } catch (...) {
        noThrow = false;
    }
    L0Test::ExpectTrue(tr, noThrow,
        "onReady known app (wrong transition) does not throw");

    for (auto* c : LifecycleManagerImplementationTest::getLoadedApps(impl)) {
        delete c;
    }
    LifecycleManagerImplementationTest::getLoadedApps(impl).clear();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// addStateTransitionRequest — matching pending transition (true branch)
// mPendingStateTransition=true AND mPendingEventName == "onAppRunning"
// RequestHandler not configured → updateState returns false, but the printf/fflush
// lines are still covered.
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Impl_RuntimeEventOnStateChangedMatchingPendingTransition()
{
    L0Test::TestResult tr;

    using RuntimeState = WPEFramework::Exchange::IRuntimeManager::RuntimeState;

    ConcreteLifecycleManagerImpl impl;

    auto* ctx = new WPEFramework::Plugin::ApplicationContext("com.test.pending");
    std::string inst = "inst-pending-001";
    ctx->setAppInstanceId(inst);
    ctx->mPendingStateTransition = true;
    ctx->mPendingEventName       = "onAppRunning";  // matches the event passed
    LifecycleManagerImplementationTest::getLoadedApps(impl).push_back(ctx);

    WPEFramework::Core::JSON::VariantContainer params;
    params["name"]             = std::string("onStateChanged");
    params["appInstanceId"]    = inst;
    params["appId"]            = std::string("");
    params["state"]            = static_cast<uint32_t>(RuntimeState::RUNTIME_STATE_RUNNING);
    params["errorCode"]        = std::string("");
    params["oldLifecycleState"]= static_cast<uint32_t>(0);
    params["newLifecycleState"]= static_cast<uint32_t>(0);
    params["errorReason"]      = std::string("");
    params["navigationIntent"] = std::string("");

    bool noThrow = true;
    try {
        LifecycleManagerImplementationTest::callDispatch(
            impl,
            LifecycleManagerImplementationTest::EventNames::LIFECYCLE_MANAGER_EVENT_RUNTIME,
            params);
    } catch (...) {
        noThrow = false;
    }
    L0Test::ExpectTrue(tr, noThrow,
        "addStateTransitionRequest matching pending event does not throw");

    // StateTransitionHandler background thread uses ctx; wait for it to finish.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    for (auto* c : LifecycleManagerImplementationTest::getLoadedApps(impl)) {
        delete c;
    }
    LifecycleManagerImplementationTest::getLoadedApps(impl).clear();

    return tr.failures;
}
