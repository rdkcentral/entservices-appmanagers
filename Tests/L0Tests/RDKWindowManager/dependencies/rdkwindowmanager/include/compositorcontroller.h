#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "rdkwindowmanagerdata.h"
#include "rdkwindowmanagerevents.h"

namespace RdkWindowManager {

class CompositorController {
public:
    static bool addKeyIntercept(const std::string& client, const uint32_t& keyCode, const uint32_t& flags, const bool& focusOnly, const bool& propagate);
    static bool removeKeyIntercept(const std::string& client, const uint32_t& keyCode, const uint32_t& flags);
    static bool addKeyListener(const std::string& client, const uint32_t& keyCode, const uint32_t& flags, std::map<std::string, RdkWindowManagerData>& listenerProperties);
    static bool addNativeKeyListener(const std::string& client, const uint32_t& keyCode, const uint32_t& flags, std::map<std::string, RdkWindowManagerData>& listenerProperties);
    static bool removeKeyListener(const std::string& client, const uint32_t& keyCode, const uint32_t& flags);
    static bool removeNativeKeyListener(const std::string& client, const uint32_t& keyCode, const uint32_t& flags);
    static bool injectKey(const uint32_t& keyCode, const uint32_t& flags);
    static bool generateKey(const std::string& client, const uint32_t& keyCode, const uint32_t& flags, std::string virtualKey = "");
    static bool getClients(std::vector<std::string>& clients);
    static bool createDisplay(const std::string& client, const std::string& displayName, uint32_t displayWidth = 0, uint32_t displayHeight = 0,
                              bool virtualDisplayEnabled = false, uint32_t virtualWidth = 0, uint32_t virtualHeight = 0,
                              bool topmost = false, bool focus = false, int32_t ownerId = 0, int32_t groupId = 0);
    static bool addListener(const std::string& client, std::shared_ptr<RdkWindowManagerEventListener> listener);
    static bool removeListener(const std::string& client, std::shared_ptr<RdkWindowManagerEventListener> listener);
    static void setEventListener(std::shared_ptr<RdkWindowManagerEventListener> listener);
    static void enableInactivityReporting(const bool enable);
    static void setInactivityInterval(const double minutes);
    static void resetInactivityTime();
    static bool enableKeyRepeats(bool enable);
    static bool getKeyRepeatsEnabled(bool& enable);
    static bool ignoreKeyInputs(bool ignore);
    static bool enableInputEvents(const std::string& client, bool enable);
    static void setKeyRepeatConfig(bool enabled, int32_t initialDelay, int32_t repeatInterval);
    static bool setFocus(const std::string& client);
    static bool setVisibility(const std::string& client, const bool visible);
    static bool getVisibility(const std::string& client, bool& visible);
    static bool renderReady(const std::string& client);
    static bool enableDisplayRender(const std::string& client, bool enable);
    static bool getLastKeyPress(uint32_t& keyCode, uint32_t& modifiers, uint64_t& timestampInSeconds);
    static bool setZorder(const std::string& client, int32_t zOrder);
    static bool getZOrder(const std::string& client, int32_t& zOrder);
    static bool startVncServer();
    static bool stopVncServer();
    static bool screenShot(uint8_t*& data, uint32_t& size);
};

} // namespace RdkWindowManager
