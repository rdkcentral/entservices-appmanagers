/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
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
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

// -----------------------------------------------------------------------
// Stub types used by RdkWindowManager / CompositorController interfaces
// -----------------------------------------------------------------------

// Keep the same namespace as the real library so the implementation
// compiles without modification.
namespace RdkWindowManager
{
    // ---- rdkwindowmanagerdata types (minimal stub) ----------------
    union RdkWindowManagerDataInfo
    {
        bool        booleanData;
        int32_t     integer32Data;
        double      doubleData;
        std::string* stringData;
        void*       pointerData;
        RdkWindowManagerDataInfo() {}
        ~RdkWindowManagerDataInfo() {}
    };

    class RdkWindowManagerData
    {
    public:
        RdkWindowManagerData() {}
        ~RdkWindowManagerData() {}
        explicit RdkWindowManagerData(bool v)     { info.booleanData  = v; }
        explicit RdkWindowManagerData(int32_t v)  { info.integer32Data = v; }
        explicit RdkWindowManagerData(double v)   { info.doubleData    = v; }

        bool    toBoolean()  const { return info.booleanData;  }
        int32_t toInteger32() const { return info.integer32Data; }
        double  toDouble()   const { return info.doubleData;   }

        RdkWindowManagerData& operator=(bool v)    { info.booleanData   = v; return *this; }
        RdkWindowManagerData& operator=(int32_t v) { info.integer32Data  = v; return *this; }
        RdkWindowManagerData& operator=(double v)  { info.doubleData    = v; return *this; }

    private:
        RdkWindowManagerDataInfo info;
    };

    // ---- Forward declarations ------------------------------------
    class RdkCompositor;
    struct WstCompositor;
    struct FireboltSurfaceInfo {};

    struct ClientInfo
    {
        int32_t  x{};
        int32_t  y{};
        uint32_t width{};
        uint32_t height{};
        double   sx{};
        double   sy{};
        double   opacity{};
        int32_t  zorder{};
        bool     visible{};
        int32_t  cropX{};
        int32_t  cropY{};
        int32_t  cropWidth{};
        int32_t  cropHeight{};
        int32_t  ownerId{};
    };

    // ---- Key-modifier flags --------------------------------------
    constexpr uint32_t RDK_WINDOW_MANAGER_FLAGS_CONTROL = 0x01;
    constexpr uint32_t RDK_WINDOW_MANAGER_FLAGS_SHIFT   = 0x02;
    constexpr uint32_t RDK_WINDOW_MANAGER_FLAGS_ALT     = 0x04;

    // ---- Event listener interface --------------------------------
    class RdkWindowManagerEventListener
    {
    public:
        virtual ~RdkWindowManagerEventListener() = default;
        virtual void onApplicationConnected(const std::string& /*client*/) {}
        virtual void onApplicationDisconnected(const std::string& /*client*/) {}
        virtual void onApplicationTerminated(const std::string& /*client*/) {}
        virtual void onReady(const std::string& /*client*/) {}
        virtual void onUserInactive(double /*minutes*/) {}
        virtual void onKeyEvent(uint32_t /*keyCode*/, uint32_t /*flags*/, bool /*keyDown*/) {}
        virtual void onSizeChangeComplete(const std::string& /*client*/) {}
        virtual void onApplicationVisible(const std::string& /*client*/) {}
        virtual void onApplicationHidden(const std::string& /*client*/) {}
        virtual void onApplicationFocus(const std::string& /*client*/) {}
        virtual void onApplicationBlur(const std::string& /*client*/) {}
    };

    // ---- FireboltExtension listener interface --------------------
    class FireboltExtensionEventListener
    {
    public:
        virtual ~FireboltExtensionEventListener() = default;
        virtual void on_focus(const char* /*clientName*/) {}
        virtual void on_blur(const char* /*clientName*/) {}
        virtual void client_connected(const char* /*clientName*/) {}
        virtual void client_disconnected(const char* /*clientName*/) {}
    };

    // =============================================================
    // Abstract interface for CompositorController static methods
    // =============================================================
    class CompositorControllerImpl
    {
    public:
        virtual ~CompositorControllerImpl() = default;

        virtual void initialize() = 0;
        virtual bool moveToFront(const std::string& client) = 0;
        virtual bool moveToBack(const std::string& client) = 0;
        virtual bool moveBehind(const std::string& client, const std::string& target) = 0;
        virtual bool setFocus(const std::string& client) = 0;
        virtual bool getFocused(std::string& client) = 0;
        virtual bool kill(const std::string& client) = 0;
        virtual bool addKeyIntercept(const std::string& client, const uint32_t& keyCode, const uint32_t& flags, const bool& focusOnly, const bool& propagate) = 0;
        virtual bool removeKeyIntercept(const std::string& client, const uint32_t& keyCode, const uint32_t& flags) = 0;
        virtual bool addKeyListener(const std::string& client, const uint32_t& keyCode, const uint32_t& flags, std::map<std::string, RdkWindowManagerData>& listenerProperties) = 0;
        virtual bool addNativeKeyListener(const std::string& client, const uint32_t& keyCode, const uint32_t& flags, std::map<std::string, RdkWindowManagerData>& listenerProperties) = 0;
        virtual bool removeKeyListener(const std::string& client, const uint32_t& keyCode, const uint32_t& flags) = 0;
        virtual bool removeAllKeyListeners() = 0;
        virtual bool removeAllKeyIntercepts() = 0;
        virtual bool removeNativeKeyListener(const std::string& client, const uint32_t& keyCode, const uint32_t& flags) = 0;
        virtual bool injectKey(const uint32_t& keyCode, const uint32_t& flags) = 0;
        virtual bool generateKey(const std::string& client, const uint32_t& keyCode, const uint32_t& flags, std::string virtualKey) = 0;
        virtual bool generateKey(const std::string& client, const uint32_t& keyCode, const uint32_t& flags, std::string virtualKey, double duration) = 0;
        virtual bool getScreenResolution(uint32_t& width, uint32_t& height) = 0;
        virtual bool setScreenResolution(uint32_t width, uint32_t height) = 0;
        virtual bool getClients(std::vector<std::string>& clients) = 0;
        virtual bool getZOrder(const std::string& client, int32_t& zorder) = 0;
        virtual bool setZorder(const std::string& client, int32_t zorder) = 0;
        virtual bool getBounds(const std::string& client, uint32_t& x, uint32_t& y, uint32_t& width, uint32_t& height) = 0;
        virtual bool setBounds(const std::string& client, uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
        virtual bool getVisibility(const std::string& client, bool& visible) = 0;
        virtual bool setVisibility(const std::string& client, bool visible) = 0;
        virtual bool getOpacity(const std::string& client, unsigned int& opacity) = 0;
        virtual bool setOpacity(const std::string& client, unsigned int opacity) = 0;
        virtual bool getScale(const std::string& client, double& scaleX, double& scaleY) = 0;
        virtual bool setScale(const std::string& client, double scaleX, double scaleY) = 0;
        virtual bool getHolePunch(const std::string& client, bool& holePunch) = 0;
        virtual bool setHolePunch(const std::string& client, bool holePunch) = 0;
        virtual bool getCrop(const std::string& client, int32_t& cropX, int32_t& cropY, int32_t& cropWidth, int32_t& cropHeight) = 0;
        virtual bool setCrop(const std::string& client, int32_t cropX, int32_t cropY, int32_t cropWidth, int32_t cropHeight) = 0;
        virtual bool scaleToFit(const std::string& client, int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;
        virtual void onKeyPress(uint32_t keycode, uint32_t flags, uint64_t metadata, bool physicalKeyPress) = 0;
        virtual void onKeyRelease(uint32_t keycode, uint32_t flags, uint64_t metadata, bool physicalKeyPress) = 0;
        virtual void onPointerMotion(uint32_t x, uint32_t y) = 0;
        virtual void onPointerButtonPress(uint32_t keyCode, uint32_t x, uint32_t y) = 0;
        virtual void onPointerButtonRelease(uint32_t keyCode, uint32_t x, uint32_t y) = 0;
        virtual bool createDisplay(const std::string& client, const std::string& displayName,
                                   uint32_t displayWidth, uint32_t displayHeight,
                                   bool virtualDisplayEnabled, uint32_t virtualWidth, uint32_t virtualHeight,
                                   bool topmost, bool focus, int32_t ownerId, int32_t groupId) = 0;
        virtual bool addListener(const std::string& client, std::shared_ptr<RdkWindowManagerEventListener> listener) = 0;
        virtual bool removeListener(const std::string& client, std::shared_ptr<RdkWindowManagerEventListener> listener) = 0;
        virtual bool onEvent(RdkCompositor* eventCompositor, const std::string& eventName) = 0;
        virtual void enableInactivityReporting(bool enable) = 0;
        virtual void setInactivityInterval(double minutes) = 0;
        virtual void resetInactivityTime() = 0;
        virtual double getInactivityTimeInMinutes() = 0;
        virtual void setEventListener(std::shared_ptr<RdkWindowManagerEventListener> listener) = 0;
        virtual bool draw() = 0;
        virtual bool update() = 0;
        virtual bool setLogLevel(const std::string level) = 0;
        virtual bool getLogLevel(std::string& level) = 0;
        virtual bool setTopmost(const std::string& client, bool topmost, bool focus) = 0;
        virtual bool getTopmost(std::string& client) = 0;
        virtual bool sendEvent(const std::string& eventName, std::vector<std::map<std::string, RdkWindowManagerData>>& data) = 0;
        virtual bool enableKeyRepeats(bool enable) = 0;
        virtual bool getKeyRepeatsEnabled(bool& enable) = 0;
        virtual bool getVirtualResolution(const std::string& client, uint32_t& virtualWidth, uint32_t& virtualHeight) = 0;
        virtual bool setVirtualResolution(const std::string& client, uint32_t virtualWidth, uint32_t virtualHeight) = 0;
        virtual bool enableVirtualDisplay(const std::string& client, bool enable) = 0;
        virtual bool getVirtualDisplayEnabled(const std::string& client, bool& enabled) = 0;
        virtual bool getLastKeyPress(uint32_t& keyCode, uint32_t& modifiers, uint64_t& timestampInSeconds) = 0;
        virtual bool ignoreKeyInputs(bool ignore) = 0;
        virtual bool screenShot(uint8_t*& data, uint32_t& size) = 0;
        virtual bool enableInputEvents(const std::string& client, bool enable) = 0;
        virtual bool showCursor() = 0;
        virtual bool hideCursor() = 0;
        virtual bool setCursorSize(uint32_t width, uint32_t height) = 0;
        virtual bool getCursorSize(uint32_t& width, uint32_t& height) = 0;
        virtual void setKeyRepeatConfig(bool enabled, int32_t initialDelay, int32_t repeatInterval) = 0;
        virtual bool setAVBlocked(std::string callsign, bool blockAV) = 0;
        virtual bool getBlockedAVApplications(std::vector<std::string>& apps) = 0;
        virtual bool isErmEnabled() = 0;
        virtual bool getClientInfo(const std::string& client, ClientInfo& ci) = 0;
        virtual bool setClientInfo(const std::string& client, const ClientInfo& ci) = 0;
        virtual bool enableDisplayRender(const std::string& client, bool enable) = 0;
        virtual bool renderReady(const std::string& client) = 0;
        virtual bool startVncServer() = 0;
        virtual bool stopVncServer() = 0;
    };

    // =============================================================
    // CompositorController wrapper – all static calls delegate to impl
    // =============================================================
    class CompositorController
    {
    public:
        static CompositorControllerImpl*& impl()
        {
            static CompositorControllerImpl* instance = nullptr;
            return instance;
        }

        static void setImpl(CompositorControllerImpl* newImpl)
        {
            impl() = newImpl;
        }

        static void initialize()
        {
            if (impl()) impl()->initialize();
        }

        static bool moveToFront(const std::string& client)
        {
            return impl() ? impl()->moveToFront(client) : false;
        }

        static bool moveToBack(const std::string& client)
        {
            return impl() ? impl()->moveToBack(client) : false;
        }

        static bool moveBehind(const std::string& client, const std::string& target)
        {
            return impl() ? impl()->moveBehind(client, target) : false;
        }

        static bool setFocus(const std::string& client)
        {
            return impl() ? impl()->setFocus(client) : false;
        }

        static bool getFocused(std::string& client)
        {
            return impl() ? impl()->getFocused(client) : false;
        }

        static bool kill(const std::string& client)
        {
            return impl() ? impl()->kill(client) : false;
        }

        static bool addKeyIntercept(const std::string& client, const uint32_t& keyCode, const uint32_t& flags, const bool& focusOnly, const bool& propagate)
        {
            return impl() ? impl()->addKeyIntercept(client, keyCode, flags, focusOnly, propagate) : false;
        }

        static bool removeKeyIntercept(const std::string& client, const uint32_t& keyCode, const uint32_t& flags)
        {
            return impl() ? impl()->removeKeyIntercept(client, keyCode, flags) : false;
        }

        static bool addKeyListener(const std::string& client, const uint32_t& keyCode, const uint32_t& flags, std::map<std::string, RdkWindowManagerData>& listenerProperties)
        {
            return impl() ? impl()->addKeyListener(client, keyCode, flags, listenerProperties) : false;
        }

        static bool addNativeKeyListener(const std::string& client, const uint32_t& keyCode, const uint32_t& flags, std::map<std::string, RdkWindowManagerData>& listenerProperties)
        {
            return impl() ? impl()->addNativeKeyListener(client, keyCode, flags, listenerProperties) : false;
        }

        static bool removeKeyListener(const std::string& client, const uint32_t& keyCode, const uint32_t& flags)
        {
            return impl() ? impl()->removeKeyListener(client, keyCode, flags) : false;
        }

        static bool removeAllKeyListeners()
        {
            return impl() ? impl()->removeAllKeyListeners() : false;
        }

        static bool removeAllKeyIntercepts()
        {
            return impl() ? impl()->removeAllKeyIntercepts() : false;
        }

        static bool removeNativeKeyListener(const std::string& client, const uint32_t& keyCode, const uint32_t& flags)
        {
            return impl() ? impl()->removeNativeKeyListener(client, keyCode, flags) : false;
        }

        static bool injectKey(const uint32_t& keyCode, const uint32_t& flags)
        {
            return impl() ? impl()->injectKey(keyCode, flags) : false;
        }

        static bool generateKey(const std::string& client, const uint32_t& keyCode, const uint32_t& flags, std::string virtualKey = "")
        {
            return impl() ? impl()->generateKey(client, keyCode, flags, std::move(virtualKey)) : false;
        }

        static bool generateKey(const std::string& client, const uint32_t& keyCode, const uint32_t& flags, std::string virtualKey, double duration)
        {
            return impl() ? impl()->generateKey(client, keyCode, flags, std::move(virtualKey), duration) : false;
        }

        static bool getScreenResolution(uint32_t& width, uint32_t& height)
        {
            return impl() ? impl()->getScreenResolution(width, height) : false;
        }

        static bool setScreenResolution(uint32_t width, uint32_t height)
        {
            return impl() ? impl()->setScreenResolution(width, height) : false;
        }

        static bool getClients(std::vector<std::string>& clients)
        {
            return impl() ? impl()->getClients(clients) : false;
        }

        static bool getZOrder(const std::string& client, int32_t& zorder)
        {
            return impl() ? impl()->getZOrder(client, zorder) : false;
        }

        static bool setZorder(const std::string& client, int32_t zorder)
        {
            return impl() ? impl()->setZorder(client, zorder) : false;
        }

        static bool getBounds(const std::string& client, uint32_t& x, uint32_t& y, uint32_t& width, uint32_t& height)
        {
            return impl() ? impl()->getBounds(client, x, y, width, height) : false;
        }

        static bool setBounds(const std::string& client, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
        {
            return impl() ? impl()->setBounds(client, x, y, width, height) : false;
        }

        static bool getVisibility(const std::string& client, bool& visible)
        {
            return impl() ? impl()->getVisibility(client, visible) : false;
        }

        static bool setVisibility(const std::string& client, bool visible)
        {
            return impl() ? impl()->setVisibility(client, visible) : false;
        }

        static bool getOpacity(const std::string& client, unsigned int& opacity)
        {
            return impl() ? impl()->getOpacity(client, opacity) : false;
        }

        static bool setOpacity(const std::string& client, unsigned int opacity)
        {
            return impl() ? impl()->setOpacity(client, opacity) : false;
        }

        static bool getScale(const std::string& client, double& scaleX, double& scaleY)
        {
            return impl() ? impl()->getScale(client, scaleX, scaleY) : false;
        }

        static bool setScale(const std::string& client, double scaleX, double scaleY)
        {
            return impl() ? impl()->setScale(client, scaleX, scaleY) : false;
        }

        static bool getHolePunch(const std::string& client, bool& holePunch)
        {
            return impl() ? impl()->getHolePunch(client, holePunch) : false;
        }

        static bool setHolePunch(const std::string& client, bool holePunch)
        {
            return impl() ? impl()->setHolePunch(client, holePunch) : false;
        }

        static bool getCrop(const std::string& client, int32_t& cropX, int32_t& cropY, int32_t& cropWidth, int32_t& cropHeight)
        {
            return impl() ? impl()->getCrop(client, cropX, cropY, cropWidth, cropHeight) : false;
        }

        static bool setCrop(const std::string& client, int32_t cropX, int32_t cropY, int32_t cropWidth, int32_t cropHeight)
        {
            return impl() ? impl()->setCrop(client, cropX, cropY, cropWidth, cropHeight) : false;
        }

        static bool scaleToFit(const std::string& client, int32_t x, int32_t y, uint32_t width, uint32_t height)
        {
            return impl() ? impl()->scaleToFit(client, x, y, width, height) : false;
        }

        static void onKeyPress(uint32_t keycode, uint32_t flags, uint64_t metadata, bool physicalKeyPress = true)
        {
            if (impl()) impl()->onKeyPress(keycode, flags, metadata, physicalKeyPress);
        }

        static void onKeyRelease(uint32_t keycode, uint32_t flags, uint64_t metadata, bool physicalKeyPress = true)
        {
            if (impl()) impl()->onKeyRelease(keycode, flags, metadata, physicalKeyPress);
        }

        static void onPointerMotion(uint32_t x, uint32_t y)
        {
            if (impl()) impl()->onPointerMotion(x, y);
        }

        static void onPointerButtonPress(uint32_t keyCode, uint32_t x, uint32_t y)
        {
            if (impl()) impl()->onPointerButtonPress(keyCode, x, y);
        }

        static void onPointerButtonRelease(uint32_t keyCode, uint32_t x, uint32_t y)
        {
            if (impl()) impl()->onPointerButtonRelease(keyCode, x, y);
        }

        static bool createDisplay(const std::string& client, const std::string& displayName,
                                  uint32_t displayWidth = 0, uint32_t displayHeight = 0,
                                  bool virtualDisplayEnabled = false, uint32_t virtualWidth = 0,
                                  uint32_t virtualHeight = 0, bool topmost = false, bool focus = false,
                                  int32_t ownerId = 0, int32_t groupId = 0)
        {
            return impl() ? impl()->createDisplay(client, displayName, displayWidth, displayHeight,
                                                  virtualDisplayEnabled, virtualWidth, virtualHeight,
                                                  topmost, focus, ownerId, groupId) : false;
        }

        static bool addListener(const std::string& client, std::shared_ptr<RdkWindowManagerEventListener> listener)
        {
            return impl() ? impl()->addListener(client, listener) : false;
        }

        static bool removeListener(const std::string& client, std::shared_ptr<RdkWindowManagerEventListener> listener)
        {
            return impl() ? impl()->removeListener(client, listener) : false;
        }

        static bool onEvent(RdkCompositor* eventCompositor, const std::string& eventName)
        {
            return impl() ? impl()->onEvent(eventCompositor, eventName) : false;
        }

        static void enableInactivityReporting(bool enable)
        {
            if (impl()) impl()->enableInactivityReporting(enable);
        }

        static void setInactivityInterval(double minutes)
        {
            if (impl()) impl()->setInactivityInterval(minutes);
        }

        static void resetInactivityTime()
        {
            if (impl()) impl()->resetInactivityTime();
        }

        static double getInactivityTimeInMinutes()
        {
            return impl() ? impl()->getInactivityTimeInMinutes() : 0.0;
        }

        static void setEventListener(std::shared_ptr<RdkWindowManagerEventListener> listener)
        {
            if (impl()) impl()->setEventListener(listener);
        }

        static bool draw()
        {
            return impl() ? impl()->draw() : false;
        }

        static bool update()
        {
            return impl() ? impl()->update() : false;
        }

        static bool setLogLevel(const std::string level)
        {
            return impl() ? impl()->setLogLevel(level) : false;
        }

        static bool getLogLevel(std::string& level)
        {
            return impl() ? impl()->getLogLevel(level) : false;
        }

        static bool setTopmost(const std::string& client, bool topmost, bool focus = false)
        {
            return impl() ? impl()->setTopmost(client, topmost, focus) : false;
        }

        static bool getTopmost(std::string& client)
        {
            return impl() ? impl()->getTopmost(client) : false;
        }

        static bool sendEvent(const std::string& eventName, std::vector<std::map<std::string, RdkWindowManagerData>>& data)
        {
            return impl() ? impl()->sendEvent(eventName, data) : false;
        }

        static bool enableKeyRepeats(bool enable)
        {
            return impl() ? impl()->enableKeyRepeats(enable) : false;
        }

        static bool getKeyRepeatsEnabled(bool& enable)
        {
            return impl() ? impl()->getKeyRepeatsEnabled(enable) : false;
        }

        static bool getVirtualResolution(const std::string& client, uint32_t& virtualWidth, uint32_t& virtualHeight)
        {
            return impl() ? impl()->getVirtualResolution(client, virtualWidth, virtualHeight) : false;
        }

        static bool setVirtualResolution(const std::string& client, uint32_t virtualWidth, uint32_t virtualHeight)
        {
            return impl() ? impl()->setVirtualResolution(client, virtualWidth, virtualHeight) : false;
        }

        static bool enableVirtualDisplay(const std::string& client, bool enable)
        {
            return impl() ? impl()->enableVirtualDisplay(client, enable) : false;
        }

        static bool getVirtualDisplayEnabled(const std::string& client, bool& enabled)
        {
            return impl() ? impl()->getVirtualDisplayEnabled(client, enabled) : false;
        }

        static bool getLastKeyPress(uint32_t& keyCode, uint32_t& modifiers, uint64_t& timestampInSeconds)
        {
            return impl() ? impl()->getLastKeyPress(keyCode, modifiers, timestampInSeconds) : false;
        }

        static bool ignoreKeyInputs(bool ignore)
        {
            return impl() ? impl()->ignoreKeyInputs(ignore) : false;
        }

        static bool screenShot(uint8_t*& data, uint32_t& size)
        {
            return impl() ? impl()->screenShot(data, size) : false;
        }

        static bool enableInputEvents(const std::string& client, bool enable)
        {
            return impl() ? impl()->enableInputEvents(client, enable) : false;
        }

        static bool showCursor()
        {
            return impl() ? impl()->showCursor() : false;
        }

        static bool hideCursor()
        {
            return impl() ? impl()->hideCursor() : false;
        }

        static bool setCursorSize(uint32_t width, uint32_t height)
        {
            return impl() ? impl()->setCursorSize(width, height) : false;
        }

        static bool getCursorSize(uint32_t& width, uint32_t& height)
        {
            return impl() ? impl()->getCursorSize(width, height) : false;
        }

        static void setKeyRepeatConfig(bool enabled, int32_t initialDelay, int32_t repeatInterval)
        {
            if (impl()) impl()->setKeyRepeatConfig(enabled, initialDelay, repeatInterval);
        }

        static bool setAVBlocked(std::string callsign, bool blockAV)
        {
            return impl() ? impl()->setAVBlocked(callsign, blockAV) : false;
        }

        static bool getBlockedAVApplications(std::vector<std::string>& apps)
        {
            return impl() ? impl()->getBlockedAVApplications(apps) : false;
        }

        static bool isErmEnabled()
        {
            return impl() ? impl()->isErmEnabled() : false;
        }

        static bool getClientInfo(const std::string& client, ClientInfo& ci)
        {
            return impl() ? impl()->getClientInfo(client, ci) : false;
        }

        static bool setClientInfo(const std::string& client, const ClientInfo& ci)
        {
            return impl() ? impl()->setClientInfo(client, ci) : false;
        }

        static bool enableDisplayRender(const std::string& client, bool enable)
        {
            return impl() ? impl()->enableDisplayRender(client, enable) : false;
        }

        static bool renderReady(const std::string& client)
        {
            return impl() ? impl()->renderReady(client) : false;
        }

        static bool startVncServer()
        {
            return impl() ? impl()->startVncServer() : false;
        }

        static bool stopVncServer()
        {
            return impl() ? impl()->stopVncServer() : false;
        }
    };

    // =============================================================
    // Abstract interface for RdkWindowManager namespace free functions
    // =============================================================
    class RdkWindowManagerImpl
    {
    public:
        virtual ~RdkWindowManagerImpl() = default;

        virtual void initialize() = 0;
        virtual void run() = 0;
        virtual void update() = 0;
        virtual void draw() = 0;
        virtual void deinitialize() = 0;
        virtual double seconds() = 0;
        virtual double milliseconds() = 0;
        virtual double microseconds() = 0;
    };

    // =============================================================
    // Wrapper for RdkWindowManager namespace free functions
    // =============================================================
    class RdkWindowManagerWrapper
    {
    public:
        static RdkWindowManagerImpl*& impl()
        {
            static RdkWindowManagerImpl* instance = nullptr;
            return instance;
        }

        static void setImpl(RdkWindowManagerImpl* newImpl)
        {
            impl() = newImpl;
        }

        static void initialize()
        {
            if (impl()) impl()->initialize();
        }

        static void run()
        {
            if (impl()) impl()->run();
        }

        static void update()
        {
            if (impl()) impl()->update();
        }

        static void draw()
        {
            if (impl()) impl()->draw();
        }

        static void deinitialize()
        {
            if (impl()) impl()->deinitialize();
        }

        static double seconds()
        {
            return impl() ? impl()->seconds() : 0.0;
        }

        static double milliseconds()
        {
            return impl() ? impl()->milliseconds() : 0.0;
        }

        static double microseconds()
        {
            return impl() ? impl()->microseconds() : 0.0;
        }
    };

    // Free functions forward to RdkWindowManagerWrapper
    inline void initialize()   { RdkWindowManagerWrapper::initialize(); }
    inline void run()           { RdkWindowManagerWrapper::run(); }
    inline void update()        { RdkWindowManagerWrapper::update(); }
    inline void draw()          { RdkWindowManagerWrapper::draw(); }
    inline void deinitialize()  { RdkWindowManagerWrapper::deinitialize(); }
    inline double seconds()     { return RdkWindowManagerWrapper::seconds(); }
    inline double milliseconds(){ return RdkWindowManagerWrapper::milliseconds(); }
    inline double microseconds(){ return RdkWindowManagerWrapper::microseconds(); }

} // namespace RdkWindowManager

// Global variable required by RDKWindowManagerImplementation.cpp.
// In test builds this is defined in Tests/mocks/Wraps.cpp.
extern int gCurrentFramerate;
