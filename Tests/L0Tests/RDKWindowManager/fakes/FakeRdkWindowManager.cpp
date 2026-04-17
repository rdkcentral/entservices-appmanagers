#include "RDKWindowManager/fakes/FakeRdkWindowManager.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <memory>
#include <mutex>

#include "rdkwindowmanager/include/compositorcontroller.h"
#include "rdkwindowmanager/include/rdkwindowmanager.h"
#include "rdkwindowmanager/include/rdkwindowmanagerevents.h"

int gCurrentFramerate = 60;

namespace {

struct State {
    std::mutex lock;

    bool createDisplayResult { true };
    bool getClientsResult { true };
    std::vector<std::string> clients { "testclient" };

    bool setFocusResult { true };
    bool setVisibilityResult { true };
    bool getVisibilityResult { true };
    bool visibleValue { true };

    bool renderReadyResult { true };
    bool enableDisplayRenderResult { true };

    bool getLastKeyResult { true };
    uint32_t keyCode { 13 };
    uint32_t modifiers { 0 };
    uint64_t ts { 100 };

    bool setZOrderResult { true };
    bool getZOrderResult { true };
    int32_t zOrder { 5 };

    bool startVncResult { true };
    bool stopVncResult { true };

    bool enableKeyRepeatResult { true };
    bool keyRepeatsEnabled { true };

    bool enableInputEventsResult { true };

    bool addInterceptResult { true };
    bool removeInterceptResult { true };
    bool addListenerResult { true };
    bool removeListenerResult { true };
    bool injectResult { true };
    bool generateResult { true };

    bool screenshotResult { true };
    std::string screenshotData { "ZmFrZQ==" };

    std::shared_ptr<RdkWindowManager::RdkWindowManagerEventListener> eventListener;
};

State& S()
{
    static State s;
    return s;
}

} // namespace

namespace L0Test {
namespace RDKWMShim {

void Reset()
{
    std::lock_guard<std::mutex> guard(S().lock);
    S().createDisplayResult = true;
    S().getClientsResult = true;
    S().clients = { "testclient" };
    S().setFocusResult = true;
    S().setVisibilityResult = true;
    S().getVisibilityResult = true;
    S().visibleValue = true;
    S().renderReadyResult = true;
    S().enableDisplayRenderResult = true;
    S().getLastKeyResult = true;
    S().keyCode = 13;
    S().modifiers = 0;
    S().ts = 100;
    S().setZOrderResult = true;
    S().getZOrderResult = true;
    S().zOrder = 5;
    S().startVncResult = true;
    S().stopVncResult = true;
    S().enableKeyRepeatResult = true;
    S().keyRepeatsEnabled = true;
    S().enableInputEventsResult = true;
    S().addInterceptResult = true;
    S().removeInterceptResult = true;
    S().addListenerResult = true;
    S().removeListenerResult = true;
    S().injectResult = true;
    S().generateResult = true;
    S().screenshotResult = true;
    S().screenshotData = "ZmFrZQ==";
    S().eventListener.reset();
}

void SetCreateDisplayResult(bool value)
{
    std::lock_guard<std::mutex> guard(S().lock);
    S().createDisplayResult = value;
}

void SetGetClientsResult(bool value, const std::vector<std::string>& clients)
{
    std::lock_guard<std::mutex> guard(S().lock);
    S().getClientsResult = value;
    S().clients = clients;
}

void SetSetFocusResult(bool value)
{
    std::lock_guard<std::mutex> guard(S().lock);
    S().setFocusResult = value;
}

void SetVisibilityResult(bool setVisibleResult, bool getVisibleResult, bool visibleValue)
{
    std::lock_guard<std::mutex> guard(S().lock);
    S().setVisibilityResult = setVisibleResult;
    S().getVisibilityResult = getVisibleResult;
    S().visibleValue = visibleValue;
}

void SetRenderReadyResult(bool value)
{
    std::lock_guard<std::mutex> guard(S().lock);
    S().renderReadyResult = value;
}

void SetEnableDisplayRenderResult(bool value)
{
    std::lock_guard<std::mutex> guard(S().lock);
    S().enableDisplayRenderResult = value;
}

void SetLastKeyResult(bool value, uint32_t keyCode, uint32_t modifiers, uint64_t timestamp)
{
    std::lock_guard<std::mutex> guard(S().lock);
    S().getLastKeyResult = value;
    S().keyCode = keyCode;
    S().modifiers = modifiers;
    S().ts = timestamp;
}

void SetZOrderResult(bool setResult, bool getResult, int32_t zOrder)
{
    std::lock_guard<std::mutex> guard(S().lock);
    S().setZOrderResult = setResult;
    S().getZOrderResult = getResult;
    S().zOrder = zOrder;
}

void SetVncResults(bool startResult, bool stopResult)
{
    std::lock_guard<std::mutex> guard(S().lock);
    S().startVncResult = startResult;
    S().stopVncResult = stopResult;
}

void SetKeyRepeatResult(bool enableResult, bool enabledValue)
{
    std::lock_guard<std::mutex> guard(S().lock);
    S().enableKeyRepeatResult = enableResult;
    S().keyRepeatsEnabled = enabledValue;
}

void SetInputEventsResult(bool value)
{
    std::lock_guard<std::mutex> guard(S().lock);
    S().enableInputEventsResult = value;
}

void SetKeyOpsResult(bool addInterceptResult, bool removeInterceptResult, bool addListenerResult, bool removeListenerResult, bool injectResult, bool generateResult)
{
    std::lock_guard<std::mutex> guard(S().lock);
    S().addInterceptResult = addInterceptResult;
    S().removeInterceptResult = removeInterceptResult;
    S().addListenerResult = addListenerResult;
    S().removeListenerResult = removeListenerResult;
    S().injectResult = injectResult;
    S().generateResult = generateResult;
}

void SetScreenshotResult(bool result, const std::string& imageData)
{
    std::lock_guard<std::mutex> guard(S().lock);
    S().screenshotResult = result;
    S().screenshotData = imageData;
}

void EmitOnConnected(const std::string& client)
{
    std::shared_ptr<RdkWindowManager::RdkWindowManagerEventListener> listener;
    {
        std::lock_guard<std::mutex> guard(S().lock);
        listener = S().eventListener;
    }
    if (listener != nullptr) {
        listener->onApplicationConnected(client);
    }
}

void EmitOnDisconnected(const std::string& client)
{
    std::shared_ptr<RdkWindowManager::RdkWindowManagerEventListener> listener;
    {
        std::lock_guard<std::mutex> guard(S().lock);
        listener = S().eventListener;
    }
    if (listener != nullptr) {
        listener->onApplicationDisconnected(client);
    }
}

void EmitOnReady(const std::string& client)
{
    std::shared_ptr<RdkWindowManager::RdkWindowManagerEventListener> listener;
    {
        std::lock_guard<std::mutex> guard(S().lock);
        listener = S().eventListener;
    }
    if (listener != nullptr) {
        listener->onReady(client);
    }
}

void EmitOnVisible(const std::string& client)
{
    std::shared_ptr<RdkWindowManager::RdkWindowManagerEventListener> listener;
    {
        std::lock_guard<std::mutex> guard(S().lock);
        listener = S().eventListener;
    }
    if (listener != nullptr) {
        listener->onApplicationVisible(client);
    }
}

void EmitOnHidden(const std::string& client)
{
    std::shared_ptr<RdkWindowManager::RdkWindowManagerEventListener> listener;
    {
        std::lock_guard<std::mutex> guard(S().lock);
        listener = S().eventListener;
    }
    if (listener != nullptr) {
        listener->onApplicationHidden(client);
    }
}

void EmitOnFocus(const std::string& client)
{
    std::shared_ptr<RdkWindowManager::RdkWindowManagerEventListener> listener;
    {
        std::lock_guard<std::mutex> guard(S().lock);
        listener = S().eventListener;
    }
    if (listener != nullptr) {
        listener->onApplicationFocus(client);
    }
}

void EmitOnBlur(const std::string& client)
{
    std::shared_ptr<RdkWindowManager::RdkWindowManagerEventListener> listener;
    {
        std::lock_guard<std::mutex> guard(S().lock);
        listener = S().eventListener;
    }
    if (listener != nullptr) {
        listener->onApplicationBlur(client);
    }
}

void EmitOnInactive(double minutes)
{
    std::shared_ptr<RdkWindowManager::RdkWindowManagerEventListener> listener;
    {
        std::lock_guard<std::mutex> guard(S().lock);
        listener = S().eventListener;
    }
    if (listener != nullptr) {
        listener->onUserInactive(minutes);
    }
}

} // namespace RDKWMShim
} // namespace L0Test

namespace RdkWindowManager {

void initialize()
{
}

void run()
{
}

void update()
{
}

void draw()
{
}

void deinitialize()
{
}

double seconds()
{
    return 1.0;
}

double milliseconds()
{
    return 1000.0;
}

double microseconds()
{
    return 1000000.0;
}

bool CompositorController::addKeyIntercept(const std::string&, const uint32_t&, const uint32_t&, const bool&, const bool&)
{
    std::lock_guard<std::mutex> guard(S().lock);
    return S().addInterceptResult;
}

bool CompositorController::removeKeyIntercept(const std::string&, const uint32_t&, const uint32_t&)
{
    std::lock_guard<std::mutex> guard(S().lock);
    return S().removeInterceptResult;
}

bool CompositorController::addKeyListener(const std::string&, const uint32_t&, const uint32_t&, std::map<std::string, RdkWindowManagerData>&)
{
    std::lock_guard<std::mutex> guard(S().lock);
    return S().addListenerResult;
}

bool CompositorController::addNativeKeyListener(const std::string&, const uint32_t&, const uint32_t&, std::map<std::string, RdkWindowManagerData>&)
{
    std::lock_guard<std::mutex> guard(S().lock);
    return S().addListenerResult;
}

bool CompositorController::removeKeyListener(const std::string&, const uint32_t&, const uint32_t&)
{
    std::lock_guard<std::mutex> guard(S().lock);
    return S().removeListenerResult;
}

bool CompositorController::removeNativeKeyListener(const std::string&, const uint32_t&, const uint32_t&)
{
    std::lock_guard<std::mutex> guard(S().lock);
    return S().removeListenerResult;
}

bool CompositorController::injectKey(const uint32_t&, const uint32_t&)
{
    std::lock_guard<std::mutex> guard(S().lock);
    return S().injectResult;
}

bool CompositorController::generateKey(const std::string&, const uint32_t&, const uint32_t&, std::string)
{
    std::lock_guard<std::mutex> guard(S().lock);
    return S().generateResult;
}

bool CompositorController::getClients(std::vector<std::string>& clients)
{
    std::lock_guard<std::mutex> guard(S().lock);
    clients = S().clients;
    return S().getClientsResult;
}

bool CompositorController::createDisplay(const std::string&, const std::string&, uint32_t, uint32_t, bool, uint32_t, uint32_t, bool, bool, int32_t, int32_t)
{
    std::lock_guard<std::mutex> guard(S().lock);
    return S().createDisplayResult;
}

bool CompositorController::addListener(const std::string&, std::shared_ptr<RdkWindowManagerEventListener>)
{
    return true;
}

bool CompositorController::removeListener(const std::string&, std::shared_ptr<RdkWindowManagerEventListener>)
{
    return true;
}

void CompositorController::setEventListener(std::shared_ptr<RdkWindowManagerEventListener> listener)
{
    std::lock_guard<std::mutex> guard(S().lock);
    S().eventListener = listener;
}

void CompositorController::enableInactivityReporting(const bool)
{
}

void CompositorController::setInactivityInterval(const double)
{
}

void CompositorController::resetInactivityTime()
{
}

bool CompositorController::enableKeyRepeats(bool)
{
    std::lock_guard<std::mutex> guard(S().lock);
    return S().enableKeyRepeatResult;
}

bool CompositorController::getKeyRepeatsEnabled(bool& enable)
{
    std::lock_guard<std::mutex> guard(S().lock);
    enable = S().keyRepeatsEnabled;
    return true;
}

bool CompositorController::ignoreKeyInputs(bool)
{
    return true;
}

bool CompositorController::enableInputEvents(const std::string&, bool)
{
    std::lock_guard<std::mutex> guard(S().lock);
    return S().enableInputEventsResult;
}

void CompositorController::setKeyRepeatConfig(bool, int32_t, int32_t)
{
}

bool CompositorController::setFocus(const std::string&)
{
    std::lock_guard<std::mutex> guard(S().lock);
    return S().setFocusResult;
}

bool CompositorController::setVisibility(const std::string&, const bool)
{
    std::lock_guard<std::mutex> guard(S().lock);
    return S().setVisibilityResult;
}

bool CompositorController::getVisibility(const std::string&, bool& visible)
{
    std::lock_guard<std::mutex> guard(S().lock);
    visible = S().visibleValue;
    return S().getVisibilityResult;
}

bool CompositorController::renderReady(const std::string&)
{
    std::lock_guard<std::mutex> guard(S().lock);
    return S().renderReadyResult;
}

bool CompositorController::enableDisplayRender(const std::string&, bool)
{
    std::lock_guard<std::mutex> guard(S().lock);
    return S().enableDisplayRenderResult;
}

bool CompositorController::getLastKeyPress(uint32_t& keyCode, uint32_t& modifiers, uint64_t& timestampInSeconds)
{
    std::lock_guard<std::mutex> guard(S().lock);
    keyCode = S().keyCode;
    modifiers = S().modifiers;
    timestampInSeconds = S().ts;
    return S().getLastKeyResult;
}

bool CompositorController::setZorder(const std::string&, int32_t)
{
    std::lock_guard<std::mutex> guard(S().lock);
    return S().setZOrderResult;
}

bool CompositorController::getZOrder(const std::string&, int32_t& zOrder)
{
    std::lock_guard<std::mutex> guard(S().lock);
    zOrder = S().zOrder;
    return S().getZOrderResult;
}

bool CompositorController::startVncServer()
{
    std::lock_guard<std::mutex> guard(S().lock);
    return S().startVncResult;
}

bool CompositorController::stopVncServer()
{
    std::lock_guard<std::mutex> guard(S().lock);
    return S().stopVncResult;
}

bool CompositorController::screenShot(uint8_t*& data, uint32_t& size)
{
    std::lock_guard<std::mutex> guard(S().lock);
    if (S().screenshotResult == false) {
        data = nullptr;
        size = 0;
        return false;
    }

    size = static_cast<uint32_t>(S().screenshotData.size());
    data = reinterpret_cast<uint8_t*>(::malloc(size));
    if (data == nullptr) {
        size = 0;
        return false;
    }

    if (size > 0U) {
        std::memcpy(data, S().screenshotData.data(), size);
    }
    return true;
}

} // namespace RdkWindowManager
