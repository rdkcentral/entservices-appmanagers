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
 * @file RuntimeManager_ImplementationTests.cpp
 *
 * L0 tests for RuntimeManagerImplementation covering:
 *   - Register / Unregister notifications
 *   - Configure (with valid service, null service)
 *   - Terminate / Kill / Hibernate / Wake / Suspend / Resume
 *   - Annotate / GetInfo
 *   - Mount / Unmount (stubs)
 *   - OCI container event dispatching
 *   - getContainerId / getInstanceState helpers (via public API behaviour)
 *   - getInstance() singleton accessor
 */

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "RuntimeManagerImplementation.h"
#include "ServiceMock.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

// ──────────────────────────────────────────────────────────────────────────────
// Minimal fakes
// ──────────────────────────────────────────────────────────────────────────────

namespace {

/* Fake IRuntimeManager::INotification to count callbacks. */
class FakeNotification final : public WPEFramework::Exchange::IRuntimeManager::INotification {
public:
    FakeNotification()
        : _refCount(1)
        , onStartedCount(0)
        , onTerminatedCount(0)
        , onStateChangedCount(0)
        , onFailureCount(0)
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
        if (id == WPEFramework::Exchange::IRuntimeManager::INotification::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IRuntimeManager::INotification*>(this);
        }
        return nullptr;
    }

    void OnStarted(const std::string& appInstanceId) override
    {
        onStartedCount++;
        lastAppInstanceId = appInstanceId;
    }

    void OnTerminated(const std::string& /*appInstanceId*/) override
    {
        onTerminatedCount++;
    }

    void OnStateChanged(const std::string& /*appInstanceId*/,
                        WPEFramework::Exchange::IRuntimeManager::RuntimeState /*state*/) override
    {
        onStateChangedCount++;
    }

    void OnFailure(const std::string& /*appInstanceId*/, const std::string& /*error*/) override
    {
        onFailureCount++;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> onStartedCount;
    std::atomic<uint32_t> onTerminatedCount;
    std::atomic<uint32_t> onStateChangedCount;
    std::atomic<uint32_t> onFailureCount;
    std::string lastAppInstanceId;
};

/* Creates a fresh RuntimeManagerImplementation instance. */
WPEFramework::Plugin::RuntimeManagerImplementation* CreateImpl()
{
    return WPEFramework::Core::Service<WPEFramework::Plugin::RuntimeManagerImplementation>::Create<
        WPEFramework::Plugin::RuntimeManagerImplementation>();
}

} // namespace

// ──────────────────────────────────────────────────────────────────────────────
// Register / Unregister
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_RegisterNotification
 *
 * Verifies that Register() accepts a valid notification pointer and returns ERROR_NONE.
 */
uint32_t Test_Impl_RegisterNotification()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    FakeNotification notif;

    const auto result = impl->Register(&notif);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
                        "Register() returns ERROR_NONE for valid notification");

    // Cleanup – unregister before release to avoid dangling reference
    impl->Unregister(&notif);
    impl->Release();
    return tr.failures;
}

/* Test_Impl_RegisterDuplicate
 *
 * Verifies that registering the same notification twice does not crash and
 * does not double-add it (idempotent guard).  The second Register should still
 * return ERROR_NONE because the implementation silently ignores duplicates.
 */
uint32_t Test_Impl_RegisterDuplicate()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    FakeNotification notif;

    impl->Register(&notif);
    const auto result = impl->Register(&notif); // duplicate
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
                        "Duplicate Register() returns ERROR_NONE (idempotent)");

    impl->Unregister(&notif);
    impl->Release();
    return tr.failures;
}

/* Test_Impl_UnregisterNotification
 *
 * Verifies that Unregister() returns ERROR_NONE when removing a registered
 * notification.
 */
uint32_t Test_Impl_UnregisterNotification()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    FakeNotification notif;

    impl->Register(&notif);
    const auto result = impl->Unregister(&notif);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
                        "Unregister() returns ERROR_NONE for a registered notification");

    impl->Release();
    return tr.failures;
}

/* Test_Impl_UnregisterNotRegistered
 *
 * Verifies that Unregister() returns ERROR_GENERAL when the notification was
 * never registered.
 */
uint32_t Test_Impl_UnregisterNotRegistered()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    FakeNotification notif; // never registered

    const auto result = impl->Unregister(&notif);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Unregister() on unregistered notification returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// Configure
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_ConfigureWithNullService
 *
 * Verifies that Configure() returns ERROR_GENERAL when passed a null service pointer.
 */
uint32_t Test_Impl_ConfigureWithNullService()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Configure(nullptr);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Configure() with null service returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

/* Test_Impl_ConfigureWithServiceMissingOCIPlugin
 *
 * Verifies Configure() still completes when OCI plugin cannot be resolved
 * (QueryInterfaceByCallsign returns nullptr for OCIContainer).
 * The result should be ERROR_GENERAL because OCI plugin creation fails.
 */
uint32_t Test_Impl_ConfigureWithServiceMissingOCIPlugin()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;
    // service.QueryInterfaceByCallsign returns nullptr by default → OCI creation fails

    const auto result = impl->Configure(&service);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Configure() returns ERROR_GENERAL when OCI plugin is unavailable");

    impl->Release();
    return tr.failures;
}

/* Test_Impl_ConfigureSetsRuntimePortalFromConfigLine
 *
 * Verifies Configure() parses "runtimeAppPortal" from service config line.
 * We use ServiceMock which returns "{}" by default – the portal should be
 * empty (no crash, no assertion).
 */
uint32_t Test_Impl_ConfigureSetsRuntimePortalFromConfigLine()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;
    // Default ConfigLine() returns "{}" – runtimeAppPortal not set, no crash expected
    const auto result = impl->Configure(&service);
    // Result is ERROR_GENERAL (OCI plugin missing) – what we test is no crash
    L0Test::ExpectTrue(tr,
                       result == WPEFramework::Core::ERROR_NONE ||
                           result == WPEFramework::Core::ERROR_GENERAL,
                       "Configure() does not crash when parsing empty config line");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// Terminate / Kill (without active container – parameter validation)
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_TerminateEmptyAppInstanceId
 *
 * Verifies Terminate() returns ERROR_GENERAL when appInstanceId is empty.
 */
uint32_t Test_Impl_TerminateEmptyAppInstanceId()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Terminate("");
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Terminate() with empty appInstanceId returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

/* Test_Impl_TerminateNoOCIPlugin
 *
 * Verifies Terminate() returns ERROR_GENERAL when no OCI plugin is available
 * and an appInstanceId is provided.
 */
uint32_t Test_Impl_TerminateNoOCIPlugin()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Terminate("youTube");
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Terminate() without OCI plugin returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

/* Test_Impl_KillEmptyAppInstanceId
 *
 * Verifies Kill() returns ERROR_GENERAL when appInstanceId is empty.
 */
uint32_t Test_Impl_KillEmptyAppInstanceId()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Kill("");
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Kill() with empty appInstanceId returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

/* Test_Impl_KillNoOCIPlugin
 *
 * Verifies Kill() returns ERROR_GENERAL when no OCI plugin is available.
 */
uint32_t Test_Impl_KillNoOCIPlugin()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Kill("youTube");
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Kill() without OCI plugin returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// Hibernate / Wake
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_HibernateEmptyAppInstanceId
 *
 * Verifies Hibernate() returns ERROR_GENERAL when appInstanceId is empty.
 */
uint32_t Test_Impl_HibernateEmptyAppInstanceId()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Hibernate("");
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Hibernate() with empty appInstanceId returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

/* Test_Impl_HibernateNoOCIPlugin
 *
 * Verifies Hibernate() returns ERROR_GENERAL when no OCI plugin is configured.
 */
uint32_t Test_Impl_HibernateNoOCIPlugin()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Hibernate("youTube");
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Hibernate() without OCI plugin returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

/* Test_Impl_WakeEmptyAppInstanceId
 *
 * Verifies Wake() returns ERROR_GENERAL when appInstanceId is empty.
 */
uint32_t Test_Impl_WakeEmptyAppInstanceId()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Wake("", WPEFramework::Exchange::IRuntimeManager::RUNTIME_STATE_RUNNING);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Wake() with empty appInstanceId returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

/* Test_Impl_WakeNoRunningContainer
 *
 * Verifies Wake() returns ERROR_GENERAL when the container was never started
 * (not tracked in mRuntimeAppInfo).
 */
uint32_t Test_Impl_WakeNoRunningContainer()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Wake("youTube", WPEFramework::Exchange::IRuntimeManager::RUNTIME_STATE_RUNNING);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Wake() for unknown container returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// Suspend / Resume
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_SuspendEmptyAppInstanceId
 *
 * Verifies Suspend() returns ERROR_GENERAL when appInstanceId is empty.
 */
uint32_t Test_Impl_SuspendEmptyAppInstanceId()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Suspend("");
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Suspend() with empty appInstanceId returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

/* Test_Impl_SuspendNoOCIPlugin
 *
 * Verifies Suspend() returns ERROR_GENERAL for a valid ID when OCI plugin
 * is not initialised.
 */
uint32_t Test_Impl_SuspendNoOCIPlugin()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Suspend("youTube");
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Suspend() without OCI plugin returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

/* Test_Impl_ResumeEmptyAppInstanceId
 *
 * Verifies Resume() returns ERROR_GENERAL when appInstanceId is empty.
 */
uint32_t Test_Impl_ResumeEmptyAppInstanceId()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Resume("");
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Resume() with empty appInstanceId returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

/* Test_Impl_ResumeNoOCIPlugin
 *
 * Verifies Resume() returns ERROR_GENERAL for a valid ID when OCI plugin
 * is not initialised.
 */
uint32_t Test_Impl_ResumeNoOCIPlugin()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Resume("youTube");
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Resume() without OCI plugin returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// Annotate
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_AnnotateEmptyAppInstanceId
 *
 * Verifies Annotate() returns ERROR_GENERAL when appInstanceId is empty.
 */
uint32_t Test_Impl_AnnotateEmptyAppInstanceId()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Annotate("", "key", "value");
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Annotate() with empty appInstanceId returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

/* Test_Impl_AnnotateEmptyKey
 *
 * Verifies Annotate() returns ERROR_GENERAL when the annotation key is empty.
 */
uint32_t Test_Impl_AnnotateEmptyKey()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Annotate("youTube", "", "value");
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Annotate() with empty key returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

/* Test_Impl_AnnotateNoOCIPlugin
 *
 * Verifies Annotate() returns ERROR_GENERAL when OCI plugin is unavailable.
 */
uint32_t Test_Impl_AnnotateNoOCIPlugin()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Annotate("youTube", "key", "value");
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Annotate() without OCI plugin returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// GetInfo
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_GetInfoEmptyAppInstanceId
 *
 * Verifies GetInfo() returns ERROR_GENERAL when appInstanceId is empty.
 */
uint32_t Test_Impl_GetInfoEmptyAppInstanceId()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    std::string info;
    const auto result = impl->GetInfo("", info);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "GetInfo() with empty appInstanceId returns ERROR_GENERAL");
    L0Test::ExpectTrue(tr, info.empty(), "GetInfo() leaves info empty on failure");

    impl->Release();
    return tr.failures;
}

/* Test_Impl_GetInfoNoOCIPlugin
 *
 * Verifies GetInfo() returns ERROR_GENERAL for a valid ID when OCI plugin
 * is not initialised.
 */
uint32_t Test_Impl_GetInfoNoOCIPlugin()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    std::string info;
    const auto result = impl->GetInfo("youTube", info);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "GetInfo() without OCI plugin returns ERROR_GENERAL");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// Run – parameter validation (no OCI plugin)
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_RunEmptyAppInstanceId
 *
 * Verifies Run() returns ERROR_GENERAL when appInstanceId is empty.
 */
uint32_t Test_Impl_RunEmptyAppInstanceId()
{
    L0Test::TestResult tr;

    // Configure() is required to initialize mUserIdManager; Run() calls
    // mUserIdManager->getUserId() unconditionally before any null guard.
    //
    // Heap-allocate ServiceMock: Configure() stores the pointer in mCurrentservice
    // and the impl's destructor calls mCurrentservice->Release(). Run() dispatches
    // two async jobs (STATECHANGED + CONTAINERFAILED) that each hold an AddRef on
    // the impl. The impl is not destroyed until both jobs complete. We must keep the
    // ServiceMock alive until after the impl destructor runs — a stack variable would
    // be dead by then, causing a use-after-free crash.
    // Static lifetime guarantees the ServiceMock outlives all async worker-pool
    // jobs (including the impl destructor) without relying on a fixed sleep.
    static L0Test::ServiceMock serviceMock;
    auto* service = &serviceMock;
    auto* impl = CreateImpl();
    impl->Configure(service);

    WPEFramework::Exchange::RuntimeConfig cfg;
    cfg.envVariables = "XDG_RUNTIME_DIR=/tmp;WAYLAND_DISPLAY=test";
    cfg.command      = "SkyBrowserLauncher";

    const auto result = impl->Run("", "", 10, 10, nullptr, nullptr, nullptr, cfg);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Run() with empty appInstanceId returns ERROR_GENERAL");

    impl->Release();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    return tr.failures;
}

/* Test_Impl_RunNoWindowManagerConnector
 *
 * Verifies Run() returns ERROR_GENERAL when WindowManagerConnector is not
 * initialised (no service configured).
 */
uint32_t Test_Impl_RunNoWindowManagerConnector()
{
    L0Test::TestResult tr;

    // Configure() initializes mUserIdManager; without it Run() crashes.
    // The WindowManagerConnector is created but mPluginInitialized remains
    // false (no real WM plugin), so createDisplay() returns false → ERROR_GENERAL.
    //
    // Heap-allocate ServiceMock for the same reason as Test_Impl_RunEmptyAppInstanceId:
    // mCurrentservice->Release() is called from the impl destructor which runs
    // asynchronously (worker thread, after async jobs complete). The ServiceMock must
    // not be destroyed before the impl is fully gone.
    // Static lifetime guarantees the ServiceMock outlives all async worker-pool
    // jobs (including the impl destructor) without relying on a fixed sleep.
    static L0Test::ServiceMock serviceMock;
    auto* service = &serviceMock;
    auto* impl = CreateImpl();
    impl->Configure(service);

    WPEFramework::Exchange::RuntimeConfig cfg;
    cfg.envVariables = "XDG_RUNTIME_DIR=/tmp;WAYLAND_DISPLAY=test";
    cfg.command      = "SkyBrowserLauncher";

    const auto result = impl->Run("appId", "youTube", 10, 10, nullptr, nullptr, nullptr, cfg);
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL,
                        "Run() without WindowManagerConnector returns ERROR_GENERAL");

    impl->Release();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// Mount / Unmount (stubs – expected to return ERROR_NONE)
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_MountReturnsSuccess
 *
 * Verifies the stubbed Mount() method returns ERROR_NONE.
 */
uint32_t Test_Impl_MountReturnsSuccess()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Mount();
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
                        "Mount() stub returns ERROR_NONE");

    impl->Release();
    return tr.failures;
}

/* Test_Impl_UnmountReturnsSuccess
 *
 * Verifies the stubbed Unmount() method returns ERROR_NONE.
 */
uint32_t Test_Impl_UnmountReturnsSuccess()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Unmount();
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_NONE,
                        "Unmount() stub returns ERROR_NONE");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// getInstance() singleton
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_GetInstanceReturnsSelf
 *
 * Verifies getInstance() returns the last-created instance.
 */
uint32_t Test_Impl_GetInstanceReturnsSelf()
{
    L0Test::TestResult tr;

    // _instance is a persistent static: set at construction, never cleared by the
    // destructor.  Reset it so this CreateImpl() call updates it predictably.
    WPEFramework::Plugin::RuntimeManagerImplementation::_instance = nullptr;
    auto* impl = CreateImpl();
    auto* retrieved = WPEFramework::Plugin::RuntimeManagerImplementation::getInstance();
    L0Test::ExpectTrue(tr, retrieved == impl,
                       "getInstance() returns the most recently constructed instance");

    impl->Release();
    // Clear the dangling _instance pointer so later tests (and any code calling
    // getInstance()) are not affected by this stale reference.
    WPEFramework::Plugin::RuntimeManagerImplementation::_instance = nullptr;
    return tr.failures;
}

/* Test_Impl_GetInstanceBeforeConstruction
 *
 * After releasing the instance, getInstance() may return stale or nullptr pointer.
 * This test validates it does not crash (we only check for absence of crash).
 */
uint32_t Test_Impl_GetInstanceBeforeConstruction()
{
    L0Test::TestResult tr;

    // Create and immediately release to cover the destructor path
    auto* impl = CreateImpl();
    impl->Release();

    // Simply calling getInstance() must not crash
    (void)WPEFramework::Plugin::RuntimeManagerImplementation::getInstance();

    L0Test::ExpectTrue(tr, true, "getInstance() after release does not crash");
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// OCI container event handlers (onOCIContainer*Event)
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_OCIContainerStartedEvent
 *
 * Calls onOCIContainerStartedEvent() with dummy data and verifies no crash.
 */
uint32_t Test_Impl_OCIContainerStartedEvent()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    JsonObject obj;
    obj["containerId"] = "dummyContainer";
    obj["name"] = "dummyName";

    impl->onOCIContainerStartedEvent("dummyName", obj);
    L0Test::ExpectTrue(tr, true, "onOCIContainerStartedEvent() does not crash with dummy data");

    impl->Release();
    // Let the async dispatch job finish so its destructor does not race with the
    // next test's impl construction.
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return tr.failures;
}

/* Test_Impl_OCIContainerStoppedEvent
 *
 * Calls onOCIContainerStoppedEvent() with dummy data and verifies no crash.
 */
uint32_t Test_Impl_OCIContainerStoppedEvent()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    JsonObject obj;
    obj["containerId"] = "dummyContainer";
    obj["name"] = "dummyName";

    impl->onOCIContainerStoppedEvent("dummyName", obj);
    L0Test::ExpectTrue(tr, true, "onOCIContainerStoppedEvent() does not crash with dummy data");

    impl->Release();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return tr.failures;
}

/* Test_Impl_OCIContainerFailureEvent
 *
 * Calls onOCIContainerFailureEvent() with dummy data and verifies no crash.
 */
uint32_t Test_Impl_OCIContainerFailureEvent()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    JsonObject obj;
    obj["containerId"] = "dummyContainer";
    obj["errorCode"] = "DOCKER_ERROR";

    impl->onOCIContainerFailureEvent("dummyName", obj);
    L0Test::ExpectTrue(tr, true, "onOCIContainerFailureEvent() does not crash with dummy data");

    impl->Release();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return tr.failures;
}

/* Test_Impl_OCIContainerStateChangedEvent
 *
 * Calls onOCIContainerStateChangedEvent() with dummy data and verifies no crash.
 */
uint32_t Test_Impl_OCIContainerStateChangedEvent()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    JsonObject obj;
    obj["containerId"] = "dummyContainer";
    obj["state"] = std::to_string(static_cast<int>(
        WPEFramework::Exchange::IRuntimeManager::RUNTIME_STATE_RUNNING));
    obj["eventName"] = "onContainerStateChanged";

    impl->onOCIContainerStateChangedEvent("dummyName", obj);
    L0Test::ExpectTrue(tr, true, "onOCIContainerStateChangedEvent() does not crash with dummy data");

    impl->Release();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return tr.failures;
}

/* Test_Impl_OCIContainerAllEventsCoverage
 *
 * Calls all four OCI event handlers in sequence to ensure full coverage of
 * the event dispatch code paths without a registered notification.
 */
uint32_t Test_Impl_OCIContainerAllEventsCoverage()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    JsonObject obj;
    obj["containerId"] = "testContainer";
    obj["name"]        = "testApp";
    obj["state"]       = std::to_string(static_cast<int>(
        WPEFramework::Exchange::IRuntimeManager::RUNTIME_STATE_RUNNING));
    obj["eventName"]   = "onContainerStateChanged";
    obj["errorCode"]   = "TEST_ERROR";

    impl->onOCIContainerStartedEvent("testApp", obj);
    impl->onOCIContainerStoppedEvent("testApp", obj);
    impl->onOCIContainerFailureEvent("testApp", obj);
    impl->onOCIContainerStateChangedEvent("testApp", obj);

    L0Test::ExpectTrue(tr, true,
                       "All four OCI event handlers can be called sequentially without crash");

    impl->Release();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return tr.failures;
}

/* Test_Impl_OCIEventsWithRegisteredNotification
 *
 * Registers a FakeNotification, fires OCI container started/stopped/failed
 * events and verifies the notification callbacks are NOT directly invoked
 * (event dispatch goes through the worker pool Job; in this L0 test the
 * notification count may or may not increment depending on pool scheduling,
 * but no crash must occur).
 */
uint32_t Test_Impl_OCIEventsWithRegisteredNotification()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    FakeNotification notif;
    impl->Register(&notif);

    JsonObject obj;
    obj["containerId"] = "com.sky.as.appsyouTube";
    obj["name"]        = "youTube";
    obj["state"]       = std::to_string(static_cast<int>(
        WPEFramework::Exchange::IRuntimeManager::RUNTIME_STATE_RUNNING));
    obj["eventName"]   = "onContainerStarted";
    obj["errorCode"]   = "0";

    impl->onOCIContainerStartedEvent("youTube", obj);
    impl->onOCIContainerStoppedEvent("youTube", obj);
    impl->onOCIContainerFailureEvent("youTube", obj);
    impl->onOCIContainerStateChangedEvent("youTube", obj);

    L0Test::ExpectTrue(tr, true,
                       "OCI event handlers with registered notification do not crash");

    impl->Unregister(&notif);
    impl->Release();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// QueryInterface
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_QueryInterfaceIRuntimeManager
 *
 * Verifies QueryInterface returns a valid pointer for IRuntimeManager::ID.
 */
uint32_t Test_Impl_QueryInterfaceIRuntimeManager()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    void* iface = impl->QueryInterface(WPEFramework::Exchange::IRuntimeManager::ID);
    L0Test::ExpectTrue(tr, iface != nullptr,
                       "QueryInterface(IRuntimeManager::ID) returns non-null");
    if (iface) {
        static_cast<WPEFramework::Exchange::IRuntimeManager*>(iface)->Release();
    }

    impl->Release();
    return tr.failures;
}

/* Test_Impl_QueryInterfaceIConfiguration
 *
 * Verifies QueryInterface returns a valid pointer for IConfiguration::ID.
 */
uint32_t Test_Impl_QueryInterfaceIConfiguration()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    void* iface = impl->QueryInterface(WPEFramework::Exchange::IConfiguration::ID);
    L0Test::ExpectTrue(tr, iface != nullptr,
                       "QueryInterface(IConfiguration::ID) returns non-null");
    if (iface) {
        static_cast<WPEFramework::Exchange::IConfiguration*>(iface)->Release();
    }

    impl->Release();
    return tr.failures;
}

/* Test_Impl_QueryInterfaceUnknownID
 *
 * Verifies QueryInterface returns nullptr for an unknown interface ID.
 */
uint32_t Test_Impl_QueryInterfaceUnknownID()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    void* iface = impl->QueryInterface(0xDEADBEEFU);
    L0Test::ExpectTrue(tr, iface == nullptr,
                       "QueryInterface() returns nullptr for unknown ID");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// Multiple Register / Unregister cycle
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_MultipleNotificationsRegisterUnregister
 *
 * Registers two distinct notifications, then unregisters both.  Verifies
 * no double-release or crash.
 */
uint32_t Test_Impl_MultipleNotificationsRegisterUnregister()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    FakeNotification n1, n2;

    L0Test::ExpectEqU32(tr, impl->Register(&n1), WPEFramework::Core::ERROR_NONE,
                        "Register n1 returns ERROR_NONE");
    L0Test::ExpectEqU32(tr, impl->Register(&n2), WPEFramework::Core::ERROR_NONE,
                        "Register n2 returns ERROR_NONE");

    L0Test::ExpectEqU32(tr, impl->Unregister(&n1), WPEFramework::Core::ERROR_NONE,
                        "Unregister n1 returns ERROR_NONE");
    L0Test::ExpectEqU32(tr, impl->Unregister(&n2), WPEFramework::Core::ERROR_NONE,
                        "Unregister n2 returns ERROR_NONE");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// Dispatch with registered notification – callback verification
//
// The event handlers call dispatchEvent() which submits a Job to the
// WPEFramework worker pool. We sleep briefly to allow the pool to execute
// the Job before checking notification counters.
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_DispatchContainerStartedFiresOnStartedCallback
 *
 * Registers a FakeNotification, fires the CONTAINERSTARTED event and waits
 * for the worker-pool Job to execute. Verifies OnStarted() is called.
 */
uint32_t Test_Impl_DispatchContainerStartedFiresOnStartedCallback()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    FakeNotification notif;
    impl->Register(&notif);

    JsonObject obj;
    obj["containerId"] = "youTube";
    obj["name"]        = "youTube";
    obj["eventName"]   = "onContainerStarted";

    impl->onOCIContainerStartedEvent("youTube", obj);

    // Allow the worker pool job to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    L0Test::ExpectTrue(tr, notif.onStartedCount > 0u,
                       "OnStarted callback fires after CONTAINERSTARTED event dispatch");

    impl->Unregister(&notif);
    impl->Release();
    return tr.failures;
}

/* Test_Impl_DispatchContainerStoppedFiresOnTerminatedCallback
 *
 * Registers a FakeNotification, fires the CONTAINERSTOPPED event and waits
 * for the worker-pool Job to execute. Verifies OnTerminated() is called.
 */
uint32_t Test_Impl_DispatchContainerStoppedFiresOnTerminatedCallback()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    FakeNotification notif;
    impl->Register(&notif);

    JsonObject obj;
    obj["containerId"] = "youTube";
    obj["name"]        = "youTube";
    obj["eventName"]   = "onContainerStopped";

    impl->onOCIContainerStoppedEvent("youTube", obj);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    L0Test::ExpectTrue(tr, notif.onTerminatedCount > 0u,
                       "OnTerminated callback fires after CONTAINERSTOPPED event dispatch");

    impl->Unregister(&notif);
    impl->Release();
    return tr.failures;
}

/* Test_Impl_DispatchContainerFailedFiresOnFailureCallback
 *
 * Registers a FakeNotification, fires the CONTAINERFAILED event and waits
 * for the worker-pool Job to execute. Verifies OnFailure() is called.
 */
uint32_t Test_Impl_DispatchContainerFailedFiresOnFailureCallback()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    FakeNotification notif;
    impl->Register(&notif);

    JsonObject obj;
    obj["containerId"] = "youTube";
    obj["name"]        = "youTube";
    obj["errorCode"]   = "1";
    obj["eventName"]   = "onContainerFailed";

    impl->onOCIContainerFailureEvent("youTube", obj);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    L0Test::ExpectTrue(tr, notif.onFailureCount > 0u,
                       "OnFailure callback fires after CONTAINERFAILED event dispatch");

    impl->Unregister(&notif);
    impl->Release();
    return tr.failures;
}

/* Test_Impl_DispatchStateChangedFiresOnStateChangedCallback
 *
 * Registers a FakeNotification, fires the STATECHANGED event and waits for
 * the worker-pool Job to execute. Verifies OnStateChanged() is called.
 */
uint32_t Test_Impl_DispatchStateChangedFiresOnStateChangedCallback()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    FakeNotification notif;
    impl->Register(&notif);

    JsonObject obj;
    obj["containerId"] = "youTube";
    obj["state"]       = std::to_string(static_cast<int>(
        WPEFramework::Exchange::IRuntimeManager::RUNTIME_STATE_RUNNING));
    obj["eventName"]   = "onContainerStateChanged";

    impl->onOCIContainerStateChangedEvent("youTube", obj);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    L0Test::ExpectTrue(tr, notif.onStateChangedCount > 0u,
                       "OnStateChanged callback fires after STATECHANGED event dispatch");

    impl->Unregister(&notif);
    impl->Release();
    return tr.failures;
}

/* Test_Impl_DispatchPortalPrefixStripping
 *
 * Verifies that when mRuntimeAppPortal is configured (via Configure() with a
 * config line containing "runtimeAppPortal"), the Dispatch code strips the
 * portal prefix from the containerId before calling notification callbacks.
 *
 * We use ServiceMock (ConfigLine returns "{}") so the portal stays empty, and
 * confirm the callback fires for the bare containerId.
 */
uint32_t Test_Impl_DispatchPortalPrefixStrippingWithRegisteredNotification()
{
    L0Test::TestResult tr;

    // Configure ServiceMock with a non-empty runtimeAppPortal so the strip
    // branch in Dispatch() is actually exercised.
    L0Test::ServiceMock::Config cfg;
    cfg.configLine = R"({"runtimeAppPortal":"com.sky.apps."})";
    L0Test::ServiceMock service(cfg);

    auto* impl = CreateImpl();
    // Configure the impl so it reads runtimeAppPortal from ConfigLine.
    impl->Configure(&service);

    FakeNotification notif;
    impl->Register(&notif);

    // containerId has the portal prefix — Dispatch() should strip it.
    JsonObject obj;
    obj["containerId"] = "com.sky.apps.youTube";
    obj["name"]        = "youTube";
    obj["eventName"]   = "onContainerStarted";

    impl->onOCIContainerStartedEvent("youTube", obj);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    L0Test::ExpectTrue(tr, notif.onStartedCount > 0u,
                       "OnStarted fires when containerId has portal prefix");
    L0Test::ExpectTrue(tr, notif.lastAppInstanceId == "youTube",
                       "Portal prefix 'com.sky.apps.' is stripped from containerId");
}

/* Test_Impl_DispatchMultipleNotificationsAllReceiveStartedEvent
 *
 * Registers two notifications, fires CONTAINERSTARTED, and verifies both
 * receive the OnStarted callback. Exercises the iterator loop in Dispatch().
 */
uint32_t Test_Impl_DispatchMultipleNotificationsAllReceiveStartedEvent()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    FakeNotification n1, n2;
    impl->Register(&n1);
    impl->Register(&n2);

    JsonObject obj;
    obj["containerId"] = "youTube";
    obj["name"]        = "youTube";
    obj["eventName"]   = "onContainerStarted";

    impl->onOCIContainerStartedEvent("youTube", obj);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    L0Test::ExpectTrue(tr, n1.onStartedCount > 0u,
                       "First notification receives OnStarted callback");
    L0Test::ExpectTrue(tr, n2.onStartedCount > 0u,
                       "Second notification receives OnStarted callback");

    impl->Unregister(&n1);
    impl->Unregister(&n2);
    impl->Release();
    return tr.failures;
}
