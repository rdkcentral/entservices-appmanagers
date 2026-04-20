#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>

#include "RDKWindowManagerImplementation.h"
#include "RDKWindowManager/fakes/FakeRdkWindowManager.h"
#include "RDKWindowManager/ServiceMock.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

namespace {

class FakeNotification final : public WPEFramework::Exchange::IRDKWindowManager::INotification {
public:
    FakeNotification()
        : _refCount(1)
        , onConnectedCount(0)
        , onDisconnectedCount(0)
        , onReadyCount(0)
        , onVisibleCount(0)
        , onHiddenCount(0)
        , onFocusCount(0)
        , onBlurCount(0)
        , onInactiveCount(0)
        , onScreenshotCount(0)
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
        if (id == WPEFramework::Exchange::IRDKWindowManager::INotification::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IRDKWindowManager::INotification*>(this);
        }
        return nullptr;
    }

    void OnUserInactivity(const double minutes) override
    {
        (void)minutes;
        onInactiveCount++;
    }

    void OnDisconnected(const std::string& client) override
    {
        lastClient = client;
        onDisconnectedCount++;
    }

    void OnReady(const std::string& client) override
    {
        lastClient = client;
        onReadyCount++;
    }

    void OnConnected(const std::string& appInstanceId) override
    {
        lastClient = appInstanceId;
        onConnectedCount++;
    }

    void OnVisible(const std::string& appInstanceId) override
    {
        lastClient = appInstanceId;
        onVisibleCount++;
    }

    void OnHidden(const std::string& appInstanceId) override
    {
        lastClient = appInstanceId;
        onHiddenCount++;
    }

    void OnFocus(const std::string& appInstanceId) override
    {
        lastClient = appInstanceId;
        onFocusCount++;
    }

    void OnBlur(const std::string& appInstanceId) override
    {
        lastClient = appInstanceId;
        onBlurCount++;
    }
    void OnScreenshotComplete(const bool success, const std::string& imageData) override
    {
        lastScreenshotSuccess = success;
        lastScreenshotData = imageData;
        onScreenshotCount++;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> onConnectedCount;
    std::atomic<uint32_t> onDisconnectedCount;
    std::atomic<uint32_t> onReadyCount;
    std::atomic<uint32_t> onVisibleCount;
    std::atomic<uint32_t> onHiddenCount;
    std::atomic<uint32_t> onFocusCount;
    std::atomic<uint32_t> onBlurCount;
    std::atomic<uint32_t> onInactiveCount;
    std::atomic<uint32_t> onScreenshotCount;
    std::string lastClient;
    bool lastScreenshotSuccess { false };
    std::string lastScreenshotData;
};

WPEFramework::Plugin::RDKWindowManagerImplementation* CreateImpl()
{
    return WPEFramework::Core::Service<WPEFramework::Plugin::RDKWindowManagerImplementation>::Create<
        WPEFramework::Plugin::RDKWindowManagerImplementation>();
}

void WaitUntil(const std::function<bool()>& pred)
{
    for (int i = 0; i < 500; ++i) {
        if (pred()) {
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

} // namespace

uint32_t Test_RDKWM_Impl_RegisterUnregister()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    auto* notif = new FakeNotification();

    const auto regRc = impl->Register(notif);
    L0Test::ExpectEqU32(tr, regRc, WPEFramework::Core::ERROR_NONE,
        "Register returns ERROR_NONE");

    const auto unregRc = impl->Unregister(notif);
    L0Test::ExpectEqU32(tr, unregRc, WPEFramework::Core::ERROR_NONE,
        "Unregister returns ERROR_NONE for registered notification");

    const auto missingRc = impl->Unregister(notif);
    L0Test::ExpectEqU32(tr, missingRc, WPEFramework::Core::ERROR_GENERAL,
        "Unregister returns ERROR_GENERAL for unknown notification");

    notif->Release();
    impl->Release();
    return tr.failures;
}

uint32_t Test_RDKWM_Impl_InitializeDeinitialize()
{
    L0Test::TestResult tr;

    L0Test::RDKWMShim::Reset();

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    const auto rc = impl->Initialize(&service);
    L0Test::ExpectEqU32(tr, rc, WPEFramework::Core::ERROR_NONE,
        "Initialize returns ERROR_NONE");

    const auto deinitRc = impl->Deinitialize(&service);
    L0Test::ExpectEqU32(tr, deinitRc, WPEFramework::Core::ERROR_NONE,
        "Deinitialize returns ERROR_NONE");

    impl->Release();
    return tr.failures;
}

uint32_t Test_RDKWM_Impl_AddKeyInterceptValidationAndSuccess()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::RDKWMShim::Reset();

    const auto emptyRc = impl->AddKeyIntercept("");
    L0Test::ExpectEqU32(tr, emptyRc, WPEFramework::Core::ERROR_GENERAL,
        "AddKeyIntercept with empty payload returns ERROR_GENERAL");

    const auto missingKeyRc = impl->AddKeyIntercept("{\"client\":\"test\"}");
    L0Test::ExpectEqU32(tr, missingKeyRc, WPEFramework::Core::ERROR_GENERAL,
        "AddKeyIntercept with missing keyCode returns ERROR_GENERAL");

    const auto okRc = impl->AddKeyIntercept("{\"client\":\"test\",\"keyCode\":13,\"modifiers\":[\"ctrl\"]}");
    L0Test::ExpectEqU32(tr, okRc, WPEFramework::Core::ERROR_NONE,
        "AddKeyIntercept with valid payload returns ERROR_NONE");

    impl->Release();
    return tr.failures;
}

uint32_t Test_RDKWM_Impl_GetAppsAndFocusPaths()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::RDKWMShim::Reset();

    std::string apps;
    L0Test::RDKWMShim::SetGetClientsResult(true, { "alpha", "beta" });
    const auto appsRc = impl->GetApps(apps);
    L0Test::ExpectEqU32(tr, appsRc, WPEFramework::Core::ERROR_NONE,
        "GetApps returns ERROR_NONE when shim getClients succeeds");
    L0Test::ExpectTrue(tr, apps.find("alpha") != std::string::npos,
        "GetApps output contains alpha");

    L0Test::RDKWMShim::SetSetFocusResult(true);
    const auto focusRc = impl->SetFocus("alpha");
    L0Test::ExpectEqU32(tr, focusRc, WPEFramework::Core::ERROR_NONE,
        "SetFocus returns ERROR_NONE when shim returns success");

    const auto emptyFocusRc = impl->SetFocus("");
    L0Test::ExpectEqU32(tr, emptyFocusRc, WPEFramework::Core::ERROR_GENERAL,
        "SetFocus returns ERROR_GENERAL for empty client");

    impl->Release();
    return tr.failures;
}

uint32_t Test_RDKWM_Impl_VncAndZOrder()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::RDKWMShim::Reset();

    L0Test::RDKWMShim::SetVncResults(true, false);

    const auto startRc = impl->StartVncServer();
    L0Test::ExpectEqU32(tr, startRc, WPEFramework::Core::ERROR_NONE,
        "StartVncServer returns ERROR_NONE when shim returns true");

    const auto stopRc = impl->StopVncServer();
    L0Test::ExpectEqU32(tr, stopRc, WPEFramework::Core::ERROR_GENERAL,
        "StopVncServer returns ERROR_GENERAL when shim returns false");

    L0Test::RDKWMShim::SetZOrderResult(true, true, 42);

    const auto setZRc = impl->SetZOrder("alpha", 42);
    L0Test::ExpectEqU32(tr, setZRc, WPEFramework::Core::ERROR_NONE,
        "SetZOrder returns ERROR_NONE when shim returns true");

    int32_t z = 0;
    const auto getZRc = impl->GetZOrder("alpha", z);
    L0Test::ExpectEqU32(tr, getZRc, WPEFramework::Core::ERROR_NONE,
        "GetZOrder returns ERROR_NONE when shim returns true");
    L0Test::ExpectTrue(tr, z == 42,
        "GetZOrder output value matches shim value");

    impl->Release();
    return tr.failures;
}

uint32_t Test_RDKWM_Impl_EventDispatchViaListener()
{
    L0Test::TestResult tr;

    L0Test::RDKWMShim::Reset();

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    const auto initRc = impl->Initialize(&service);
    L0Test::ExpectEqU32(tr, initRc, WPEFramework::Core::ERROR_NONE,
        "Initialize returns ERROR_NONE");

    auto* notif = new FakeNotification();
    impl->Register(notif);

    L0Test::RDKWMShim::EmitOnConnected("org.rdk.test");
    L0Test::RDKWMShim::EmitOnVisible("org.rdk.test");
    L0Test::RDKWMShim::EmitOnInactive(2.5);

    WaitUntil([&]() {
        return notif->onConnectedCount.load() > 0
            && notif->onVisibleCount.load() > 0
            && notif->onInactiveCount.load() > 0;
    });

    L0Test::ExpectTrue(tr, notif->onConnectedCount.load() > 0,
        "OnConnected callback fired");
    L0Test::ExpectTrue(tr, notif->onVisibleCount.load() > 0,
        "OnVisible callback fired");
    L0Test::ExpectTrue(tr, notif->onInactiveCount.load() > 0,
        "OnUserInactivity callback fired");

    impl->Unregister(notif);
    notif->Release();

    const auto deinitRc = impl->Deinitialize(&service);
    L0Test::ExpectEqU32(tr, deinitRc, WPEFramework::Core::ERROR_NONE,
        "Deinitialize returns ERROR_NONE");

    impl->Release();
    return tr.failures;
}

uint32_t Test_RDKWM_Impl_VisibilityAndRenderApis()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::RDKWMShim::Reset();

    L0Test::RDKWMShim::SetVisibilityResult(true, true, true);

    const auto setVisibleRc = impl->SetVisible("app.alpha", true);
    L0Test::ExpectEqU32(tr, setVisibleRc, WPEFramework::Core::ERROR_NONE,
        "SetVisible returns ERROR_NONE when shim returns success");

    bool visible = false;
    const auto getVisibleRc = impl->GetVisibility("app.alpha", visible);
    L0Test::ExpectEqU32(tr, getVisibleRc, WPEFramework::Core::ERROR_NONE,
        "GetVisibility returns ERROR_NONE when shim returns success");
    L0Test::ExpectTrue(tr, visible == true,
        "GetVisibility returns expected value from shim");

    L0Test::RDKWMShim::SetRenderReadyResult(true);
    bool renderReady = false;
    const auto renderReadyRc = impl->RenderReady("app.alpha", renderReady);
    L0Test::ExpectEqU32(tr, renderReadyRc, WPEFramework::Core::ERROR_NONE,
        "RenderReady returns ERROR_NONE for valid client");
    L0Test::ExpectTrue(tr, renderReady == true,
        "RenderReady status reflects shim value");

    L0Test::RDKWMShim::SetEnableDisplayRenderResult(false);
    const auto enableDisplayRc = impl->EnableDisplayRender("app.alpha", true);
    L0Test::ExpectEqU32(tr, enableDisplayRc, WPEFramework::Core::ERROR_GENERAL,
        "EnableDisplayRender returns ERROR_GENERAL when shim returns false");

    impl->Release();
    return tr.failures;
}

uint32_t Test_RDKWM_Impl_KeyListenerAndInjectionPaths()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::RDKWMShim::Reset();

    const auto addKeyListenerEmptyRc = impl->AddKeyListener("");
    L0Test::ExpectEqU32(tr, addKeyListenerEmptyRc, WPEFramework::Core::ERROR_GENERAL,
        "AddKeyListener returns ERROR_GENERAL for empty payload");

    const auto addKeyListenerValidRc = impl->AddKeyListener("{\"client\":\"alpha\",\"keys\":[{\"keyCode\":\"13\",\"modifiers\":[\"ctrl\"]}]}");
    L0Test::ExpectEqU32(tr, addKeyListenerValidRc, WPEFramework::Core::ERROR_NONE,
        "AddKeyListener returns ERROR_NONE for valid payload");

    const auto removeKeyListenerValidRc = impl->RemoveKeyListener("{\"client\":\"alpha\",\"keys\":[{\"keyCode\":\"13\",\"modifiers\":[\"ctrl\"]}]}");
    L0Test::ExpectEqU32(tr, removeKeyListenerValidRc, WPEFramework::Core::ERROR_NONE,
        "RemoveKeyListener returns ERROR_NONE for valid payload");

    const auto injectRc = impl->InjectKey(13, "{\"modifiers\":[\"ctrl\"]}");
    L0Test::ExpectEqU32(tr, injectRc, WPEFramework::Core::ERROR_NONE,
        "InjectKey returns ERROR_NONE when shim inject succeeds");

    L0Test::RDKWMShim::SetGetClientsResult(true, { "alpha" });
    const auto generateRc = impl->GenerateKey("{\"keys\":[{\"keyCode\":13,\"client\":\"alpha\",\"delay\":0}]}", "");
    L0Test::ExpectEqU32(tr, generateRc, WPEFramework::Core::ERROR_NONE,
        "GenerateKey returns ERROR_NONE for valid payload and known client");

    impl->Release();
    return tr.failures;
}

uint32_t Test_RDKWM_Impl_KeyRepeatAndInputEvents()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::RDKWMShim::Reset();

    L0Test::RDKWMShim::SetKeyRepeatResult(true, true);
    const auto enableKeyRepeatRc = impl->EnableKeyRepeats(true);
    L0Test::ExpectEqU32(tr, enableKeyRepeatRc, WPEFramework::Core::ERROR_NONE,
        "EnableKeyRepeats returns ERROR_NONE when shim returns true");

    bool keyRepeatEnabled = false;
    const auto getKeyRepeatRc = impl->GetKeyRepeatsEnabled(keyRepeatEnabled);
    L0Test::ExpectEqU32(tr, getKeyRepeatRc, WPEFramework::Core::ERROR_NONE,
        "GetKeyRepeatsEnabled returns ERROR_NONE on query success");
    L0Test::ExpectTrue(tr, keyRepeatEnabled == true,
        "GetKeyRepeatsEnabled reflects shim-enabled value");

    L0Test::RDKWMShim::SetGetKeyRepeatsEnabledResult(false);
    const auto getKeyRepeatFailRc = impl->GetKeyRepeatsEnabled(keyRepeatEnabled);
    L0Test::ExpectEqU32(tr, getKeyRepeatFailRc, WPEFramework::Core::ERROR_GENERAL,
        "GetKeyRepeatsEnabled returns ERROR_GENERAL when shim query fails");
    L0Test::RDKWMShim::SetGetKeyRepeatsEnabledResult(true);

    const auto keyRepeatConfigRc = impl->KeyRepeatConfig("keyboard", "{\"enabled\":true,\"initialDelay\":100,\"repeatInterval\":20}");
    L0Test::ExpectEqU32(tr, keyRepeatConfigRc, WPEFramework::Core::ERROR_NONE,
        "KeyRepeatConfig returns ERROR_NONE for valid config");

    const auto inputEventsBadJsonRc = impl->EnableInputEvents("not-json", true);
    L0Test::ExpectEqU32(tr, inputEventsBadJsonRc, WPEFramework::Core::ERROR_GENERAL,
        "EnableInputEvents returns ERROR_GENERAL for invalid JSON");

    L0Test::RDKWMShim::SetGetClientsResult(true, { "alpha", "beta" });
    L0Test::RDKWMShim::SetInputEventsResult(true);
    const auto inputEventsWildcardRc = impl->EnableInputEvents("{\"clients\":[\"*\"]}", true);
    L0Test::ExpectEqU32(tr, inputEventsWildcardRc, WPEFramework::Core::ERROR_NONE,
        "EnableInputEvents returns ERROR_NONE for wildcard clients and successful shim path");

    impl->Release();
    return tr.failures;
}

uint32_t Test_RDKWM_Impl_LastKeyInfoAndScreenshotEvent()
{
    L0Test::TestResult tr;

    L0Test::RDKWMShim::Reset();
    L0Test::RDKWMShim::SetLastKeyResult(true, 77, 4, 123456);
    L0Test::RDKWMShim::SetScreenshotResult(true, "ZmFrZQ==");

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    const auto initRc = impl->Initialize(&service);
    L0Test::ExpectEqU32(tr, initRc, WPEFramework::Core::ERROR_NONE,
        "Initialize returns ERROR_NONE");

    uint32_t keyCode = 0;
    uint32_t modifiers = 0;
    uint64_t ts = 0;
    const auto lastKeyRc = impl->GetLastKeyInfo(keyCode, modifiers, ts);
    L0Test::ExpectEqU32(tr, lastKeyRc, WPEFramework::Core::ERROR_NONE,
        "GetLastKeyInfo returns ERROR_NONE when shim returns key info");
    L0Test::ExpectTrue(tr, keyCode == 77 && modifiers == 4 && ts == 123456,
        "GetLastKeyInfo returns values from shim");

    auto* notif = new FakeNotification();
    impl->Register(notif);

    const auto screenshotRc = impl->GetScreenshot();
    L0Test::ExpectEqU32(tr, screenshotRc, WPEFramework::Core::ERROR_NONE,
        "GetScreenshot returns ERROR_NONE while queuing async capture");

    WaitUntil([&]() {
        return notif->onScreenshotCount.load() > 0;
    });

    L0Test::ExpectTrue(tr, notif->onScreenshotCount.load() > 0,
        "Screenshot completion callback fired");

    impl->Unregister(notif);
    notif->Release();

    const auto deinitRc = impl->Deinitialize(&service);
    L0Test::ExpectEqU32(tr, deinitRc, WPEFramework::Core::ERROR_NONE,
        "Deinitialize returns ERROR_NONE");

    impl->Release();
    return tr.failures;
}

uint32_t Test_RDKWM_Impl_CreateDisplayAndEventVariants()
{
    L0Test::TestResult tr;

    L0Test::RDKWMShim::Reset();
    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    const auto initRc = impl->Initialize(&service);
    L0Test::ExpectEqU32(tr, initRc, WPEFramework::Core::ERROR_NONE,
        "Initialize returns ERROR_NONE");

    auto* notif = new FakeNotification();
    impl->Register(notif);

    const auto emptyClientRc = impl->CreateDisplay("", "disp", 1280, 720, false, 0, 0, 0, 0, true, true);
    L0Test::ExpectEqU32(tr, emptyClientRc, WPEFramework::Core::ERROR_GENERAL,
        "CreateDisplay returns ERROR_GENERAL when client is empty");

    L0Test::RDKWMShim::SetGetClientsResult(true, { "alpha" });
    L0Test::RDKWMShim::SetCreateDisplayResult(true);
    const auto successRc = impl->CreateDisplay("org.rdk.test", "disp-main", 1280, 720, true, 1920, 1080, 100, 101, true, true);
    L0Test::ExpectEqU32(tr, successRc, WPEFramework::Core::ERROR_NONE,
        "CreateDisplay returns ERROR_NONE for valid request");

    L0Test::RDKWMShim::SetGetClientsResult(false, {});
    L0Test::RDKWMShim::SetCreateDisplayResult(false);
    const auto failureRc = impl->CreateDisplay("org.rdk.beta", "disp-fail", 640, 360, false, 0, 0, 0, 0, false, false);
    L0Test::ExpectEqU32(tr, failureRc, WPEFramework::Core::ERROR_GENERAL,
        "CreateDisplay returns ERROR_GENERAL when backend create fails");

    L0Test::RDKWMShim::EmitOnDisconnected("org.rdk.test");
    L0Test::RDKWMShim::EmitOnReady("org.rdk.test");
    L0Test::RDKWMShim::EmitOnHidden("org.rdk.test");
    L0Test::RDKWMShim::EmitOnFocus("org.rdk.test");
    L0Test::RDKWMShim::EmitOnBlur("org.rdk.test");

    WaitUntil([&]() {
        return notif->onDisconnectedCount.load() > 0
            && notif->onReadyCount.load() > 0
            && notif->onHiddenCount.load() > 0
            && notif->onFocusCount.load() > 0
            && notif->onBlurCount.load() > 0;
    });

    L0Test::ExpectTrue(tr, notif->onDisconnectedCount.load() > 0,
        "OnDisconnected callback fired");
    L0Test::ExpectTrue(tr, notif->onReadyCount.load() > 0,
        "OnReady callback fired");
    L0Test::ExpectTrue(tr, notif->onHiddenCount.load() > 0,
        "OnHidden callback fired");
    L0Test::ExpectTrue(tr, notif->onFocusCount.load() > 0,
        "OnFocus callback fired");
    L0Test::ExpectTrue(tr, notif->onBlurCount.load() > 0,
        "OnBlur callback fired");

    impl->Unregister(notif);
    notif->Release();

    const auto deinitRc = impl->Deinitialize(&service);
    L0Test::ExpectEqU32(tr, deinitRc, WPEFramework::Core::ERROR_NONE,
        "Deinitialize returns ERROR_NONE");

    impl->Release();
    return tr.failures;
}

uint32_t Test_RDKWM_Impl_InterceptsAndInactivityBranches()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::RDKWMShim::Reset();

    L0Test::RDKWMShim::SetKeyOpsResult(false, true, true, true, true, true);
    const auto addInterceptFailRc = impl->AddKeyIntercept("{\"callsign\":\"alpha\",\"keyCode\":13,\"modifiers\":[\"shift\"],\"focusOnly\":true,\"propagate\":true}");
    L0Test::ExpectEqU32(tr, addInterceptFailRc, WPEFramework::Core::ERROR_GENERAL,
        "AddKeyIntercept returns ERROR_GENERAL when backend rejects intercept");

    L0Test::RDKWMShim::SetKeyOpsResult(true, true, true, true, true, true);
    const auto addInterceptOkRc = impl->AddKeyIntercept("{\"callsign\":\"alpha\",\"keyCode\":13,\"modifiers\":[\"alt\"]}");
    L0Test::ExpectEqU32(tr, addInterceptOkRc, WPEFramework::Core::ERROR_NONE,
        "AddKeyIntercept returns ERROR_NONE with callsign and alt modifier");

    const auto addInterceptsEmptyClientRc = impl->AddKeyIntercepts("", "[]");
    L0Test::ExpectEqU32(tr, addInterceptsEmptyClientRc, WPEFramework::Core::ERROR_GENERAL,
        "AddKeyIntercepts returns ERROR_GENERAL for empty clientId");

    const auto addInterceptsBadJsonRc = impl->AddKeyIntercepts("alpha", "not-json-array");
    L0Test::ExpectEqU32(tr, addInterceptsBadJsonRc, WPEFramework::Core::ERROR_GENERAL,
        "AddKeyIntercepts returns ERROR_GENERAL for invalid JSON");

    const auto addInterceptsMixedRc = impl->AddKeyIntercepts("alpha", "[{\"missing\":1},{\"keyCode\":15,\"modifiers\":[\"shift\"]}]");
    L0Test::ExpectEqU32(tr, addInterceptsMixedRc, WPEFramework::Core::ERROR_NONE,
        "AddKeyIntercepts returns ERROR_NONE when at least one valid intercept is processed");

    L0Test::RDKWMShim::SetKeyOpsResult(true, false, true, true, true, true);
    const auto removeInterceptFailRc = impl->RemoveKeyIntercept("alpha", 13, "[\"alt\"]");
    L0Test::ExpectEqU32(tr, removeInterceptFailRc, WPEFramework::Core::ERROR_GENERAL,
        "RemoveKeyIntercept returns ERROR_GENERAL when backend remove fails");

    const auto removeInterceptBadJsonRc = impl->RemoveKeyIntercept("alpha", 13, "bad-json");
    L0Test::ExpectEqU32(tr, removeInterceptBadJsonRc, WPEFramework::Core::ERROR_GENERAL,
        "RemoveKeyIntercept returns ERROR_GENERAL for invalid modifier JSON");

    L0Test::RDKWMShim::SetKeyOpsResult(true, true, true, true, true, true);
    const auto removeInterceptOkRc = impl->RemoveKeyIntercept("alpha", 13, "[\"shift\",\"alt\"]");
    L0Test::ExpectEqU32(tr, removeInterceptOkRc, WPEFramework::Core::ERROR_NONE,
        "RemoveKeyIntercept returns ERROR_NONE for valid remove request");

    const auto inactivityEnableRc = impl->EnableInactivityReporting(true);
    const auto inactivityIntervalRc = impl->SetInactivityInterval(3);
    const auto inactivityResetRc = impl->ResetInactivityTime();
    L0Test::ExpectEqU32(tr, inactivityEnableRc, WPEFramework::Core::ERROR_NONE,
        "EnableInactivityReporting returns ERROR_NONE");
    L0Test::ExpectEqU32(tr, inactivityIntervalRc, WPEFramework::Core::ERROR_NONE,
        "SetInactivityInterval returns ERROR_NONE");
    L0Test::ExpectEqU32(tr, inactivityResetRc, WPEFramework::Core::ERROR_NONE,
        "ResetInactivityTime returns ERROR_NONE");

    const auto ignoreKeyInputsRc = impl->IgnoreKeyInputs(true);
    L0Test::ExpectEqU32(tr, ignoreKeyInputsRc, WPEFramework::Core::ERROR_NONE,
        "IgnoreKeyInputs returns ERROR_NONE");

    impl->Release();
    return tr.failures;
}

uint32_t Test_RDKWM_Impl_ErrorPathMatrix()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::RDKWMShim::Reset();

    L0Test::RDKWMShim::SetGetClientsResult(false, {});
    std::string apps;
    const auto getAppsFailRc = impl->GetApps(apps);
    L0Test::ExpectEqU32(tr, getAppsFailRc, WPEFramework::Core::ERROR_GENERAL,
        "GetApps returns ERROR_GENERAL when backend getClients fails");

    const auto setVisibleEmptyRc = impl->SetVisible("", true);
    L0Test::ExpectEqU32(tr, setVisibleEmptyRc, WPEFramework::Core::ERROR_GENERAL,
        "SetVisible returns ERROR_GENERAL for empty client");

    L0Test::RDKWMShim::SetVisibilityResult(false, false, false);
    const auto setVisibleFailRc = impl->SetVisible("alpha", true);
    L0Test::ExpectEqU32(tr, setVisibleFailRc, WPEFramework::Core::ERROR_GENERAL,
        "SetVisible returns ERROR_GENERAL when backend setVisibility fails");

    bool visible = true;
    const auto getVisibleEmptyRc = impl->GetVisibility("", visible);
    L0Test::ExpectEqU32(tr, getVisibleEmptyRc, WPEFramework::Core::ERROR_GENERAL,
        "GetVisibility returns ERROR_GENERAL for empty client");

    const auto getVisibleFailRc = impl->GetVisibility("alpha", visible);
    L0Test::ExpectEqU32(tr, getVisibleFailRc, WPEFramework::Core::ERROR_GENERAL,
        "GetVisibility returns ERROR_GENERAL when backend getVisibility fails");

    L0Test::RDKWMShim::SetSetFocusResult(false);
    const auto setFocusFailRc = impl->SetFocus("alpha");
    L0Test::ExpectEqU32(tr, setFocusFailRc, WPEFramework::Core::ERROR_GENERAL,
        "SetFocus returns ERROR_GENERAL when backend setFocus fails");

    bool renderReady = true;
    const auto renderEmptyRc = impl->RenderReady("", renderReady);
    L0Test::ExpectEqU32(tr, renderEmptyRc, WPEFramework::Core::ERROR_GENERAL,
        "RenderReady returns ERROR_GENERAL for empty client");

    L0Test::RDKWMShim::SetRenderReadyResult(false);
    const auto renderFailRc = impl->RenderReady("alpha", renderReady);
    L0Test::ExpectEqU32(tr, renderFailRc, WPEFramework::Core::ERROR_NONE,
        "RenderReady returns ERROR_NONE when client is valid");
    L0Test::ExpectTrue(tr, false == renderReady,
        "RenderReady sets status=false when backend renderReady is false");

    const auto enableDisplayEmptyRc = impl->EnableDisplayRender("", true);
    L0Test::ExpectEqU32(tr, enableDisplayEmptyRc, WPEFramework::Core::ERROR_GENERAL,
        "EnableDisplayRender returns ERROR_GENERAL for empty client");

    L0Test::RDKWMShim::SetEnableDisplayRenderResult(true);
    const auto enableDisplayOkRc = impl->EnableDisplayRender("alpha", false);
    L0Test::ExpectEqU32(tr, enableDisplayOkRc, WPEFramework::Core::ERROR_NONE,
        "EnableDisplayRender returns ERROR_NONE when backend succeeds");

    L0Test::RDKWMShim::SetLastKeyResult(false, 0, 0, 0);
    uint32_t keyCode = 0;
    uint32_t modifiers = 0;
    uint64_t ts = 0;
    const auto lastKeyFailRc = impl->GetLastKeyInfo(keyCode, modifiers, ts);
    L0Test::ExpectEqU32(tr, lastKeyFailRc, WPEFramework::Core::ERROR_GENERAL,
        "GetLastKeyInfo returns ERROR_GENERAL when backend returns false");

    int32_t emptyZOrder = 0;
    const auto setZEmptyRc = impl->SetZOrder("", 1);
    const auto getZEmptyRc = impl->GetZOrder("", emptyZOrder);
    L0Test::ExpectEqU32(tr, setZEmptyRc, WPEFramework::Core::ERROR_GENERAL,
        "SetZOrder returns ERROR_GENERAL for empty appInstanceId");
    L0Test::ExpectEqU32(tr, getZEmptyRc, WPEFramework::Core::ERROR_GENERAL,
        "GetZOrder returns ERROR_GENERAL for empty appInstanceId");

    L0Test::RDKWMShim::SetZOrderResult(false, false, 0);
    int32_t zOrder = 0;
    const auto setZFailRc = impl->SetZOrder("alpha", 7);
    const auto getZFailRc = impl->GetZOrder("alpha", zOrder);
    L0Test::ExpectEqU32(tr, setZFailRc, WPEFramework::Core::ERROR_GENERAL,
        "SetZOrder returns ERROR_GENERAL when backend setZorder fails");
    L0Test::ExpectEqU32(tr, getZFailRc, WPEFramework::Core::ERROR_GENERAL,
        "GetZOrder returns ERROR_GENERAL when backend getZOrder fails");

    L0Test::RDKWMShim::SetVncResults(false, true);
    const auto startVncFailRc = impl->StartVncServer();
    const auto stopVncOkRc = impl->StopVncServer();
    L0Test::ExpectEqU32(tr, startVncFailRc, WPEFramework::Core::ERROR_GENERAL,
        "StartVncServer returns ERROR_GENERAL when backend start fails");
    L0Test::ExpectEqU32(tr, stopVncOkRc, WPEFramework::Core::ERROR_NONE,
        "StopVncServer returns ERROR_NONE when backend stop succeeds");

    L0Test::RDKWMShim::SetKeyRepeatResult(false, false);
    const auto enableKeyRepeatFailRc = impl->EnableKeyRepeats(false);
    L0Test::ExpectEqU32(tr, enableKeyRepeatFailRc, WPEFramework::Core::ERROR_GENERAL,
        "EnableKeyRepeats returns ERROR_GENERAL when backend fails");

    const auto keyRepeatMissingEnabledRc = impl->KeyRepeatConfig("keyboard", "{\"initialDelay\":100,\"repeatInterval\":20}");
    const auto keyRepeatUnsupportedInputRc = impl->KeyRepeatConfig("touch", "{\"enabled\":true,\"initialDelay\":100,\"repeatInterval\":20}");
    L0Test::ExpectEqU32(tr, keyRepeatMissingEnabledRc, WPEFramework::Core::ERROR_GENERAL,
        "KeyRepeatConfig returns ERROR_GENERAL when enabled is missing");
    L0Test::ExpectEqU32(tr, keyRepeatUnsupportedInputRc, WPEFramework::Core::ERROR_GENERAL,
        "KeyRepeatConfig returns ERROR_GENERAL for unsupported input");

    const auto inputEventsEmptyClientsRc = impl->EnableInputEvents("", true);
    const auto inputEventsMissingClientArrayRc = impl->EnableInputEvents("{}", true);
    const auto inputEventsInvalidClientEntryRc = impl->EnableInputEvents("{\"clients\":[\"alpha\",123]}", true);
    L0Test::ExpectEqU32(tr, inputEventsEmptyClientsRc, WPEFramework::Core::ERROR_GENERAL,
        "EnableInputEvents returns ERROR_GENERAL when clients payload is empty");
    L0Test::ExpectEqU32(tr, inputEventsMissingClientArrayRc, WPEFramework::Core::ERROR_GENERAL,
        "EnableInputEvents returns ERROR_GENERAL when clientArray is missing");
    L0Test::ExpectEqU32(tr, inputEventsInvalidClientEntryRc, WPEFramework::Core::ERROR_GENERAL,
        "EnableInputEvents returns ERROR_GENERAL when clients array contains invalid entry");

    const auto addKeyListenerNativeRc = impl->AddKeyListener("{\"callsign\":\"alpha\",\"keys\":[{\"nativeKeyCode\":\"*\",\"modifiers\":[\"alt\"],\"activate\":true,\"propagate\":true}]}");
    const auto removeKeyListenerNativeRc = impl->RemoveKeyListener("{\"callsign\":\"alpha\",\"keys\":[{\"nativeKeyCode\":\"*\",\"modifiers\":[\"alt\"]}]}");
    L0Test::ExpectEqU32(tr, addKeyListenerNativeRc, WPEFramework::Core::ERROR_NONE,
        "AddKeyListener returns ERROR_NONE for nativeKeyCode branch");
    L0Test::ExpectEqU32(tr, removeKeyListenerNativeRc, WPEFramework::Core::ERROR_NONE,
        "RemoveKeyListener returns ERROR_NONE for nativeKeyCode branch");

    const auto addKeyListenerDualRc = impl->AddKeyListener("{\"client\":\"alpha\",\"keys\":[{\"keyCode\":\"13\",\"nativeKeyCode\":\"14\"}]}");
    L0Test::ExpectEqU32(tr, addKeyListenerDualRc, WPEFramework::Core::ERROR_GENERAL,
        "AddKeyListener returns ERROR_GENERAL when both keyCode and nativeKeyCode are set");

    L0Test::RDKWMShim::SetKeyOpsResult(true, true, true, true, false, false);
    const auto injectFailRc = impl->InjectKey(13, "{}");
    const auto generateMissingKeysRc = impl->GenerateKey("{\"invalid\":[]}", "alpha");
    const auto generateFailRc = impl->GenerateKey("{\"keys\":[{\"keyCode\":13,\"callsign\":\"alpha\",\"delay\":0}]}", "");
    L0Test::ExpectEqU32(tr, injectFailRc, WPEFramework::Core::ERROR_GENERAL,
        "InjectKey returns ERROR_GENERAL when backend inject fails");
    L0Test::ExpectEqU32(tr, generateMissingKeysRc, WPEFramework::Core::ERROR_GENERAL,
        "GenerateKey returns ERROR_GENERAL when keys array label is missing");
    L0Test::ExpectEqU32(tr, generateFailRc, WPEFramework::Core::ERROR_GENERAL,
        "GenerateKey returns ERROR_GENERAL when backend generate fails");

    impl->Release();
    return tr.failures;
}
