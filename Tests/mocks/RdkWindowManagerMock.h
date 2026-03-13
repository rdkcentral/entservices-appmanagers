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

#include <gmock/gmock.h>
#include "RdkWindowManager.h"

namespace RdkWindowManager
{

// =============================================================
// Mock for CompositorController (all static methods via impl)
// =============================================================
class CompositorControllerImplMock : public CompositorControllerImpl
{
public:
    CompositorControllerImplMock() = default;
    virtual ~CompositorControllerImplMock() = default;

    MOCK_METHOD(void, initialize, (), (override));
    MOCK_METHOD(bool, moveToFront, (const std::string& client), (override));
    MOCK_METHOD(bool, moveToBack, (const std::string& client), (override));
    MOCK_METHOD(bool, moveBehind, (const std::string& client, const std::string& target), (override));
    MOCK_METHOD(bool, setFocus, (const std::string& client), (override));
    MOCK_METHOD(bool, getFocused, (std::string& client), (override));
    MOCK_METHOD(bool, kill, (const std::string& client), (override));
    MOCK_METHOD(bool, addKeyIntercept, (const std::string& client, const uint32_t& keyCode, const uint32_t& flags, const bool& focusOnly, const bool& propagate), (override));
    MOCK_METHOD(bool, removeKeyIntercept, (const std::string& client, const uint32_t& keyCode, const uint32_t& flags), (override));
    MOCK_METHOD(bool, addKeyListener, (const std::string& client, const uint32_t& keyCode, const uint32_t& flags, (std::map<std::string, RdkWindowManagerData>&) listenerProperties), (override));
    MOCK_METHOD(bool, addNativeKeyListener, (const std::string& client, const uint32_t& keyCode, const uint32_t& flags, (std::map<std::string, RdkWindowManagerData>&) listenerProperties), (override));
    MOCK_METHOD(bool, removeKeyListener, (const std::string& client, const uint32_t& keyCode, const uint32_t& flags), (override));
    MOCK_METHOD(bool, removeAllKeyListeners, (), (override));
    MOCK_METHOD(bool, removeAllKeyIntercepts, (), (override));
    MOCK_METHOD(bool, removeNativeKeyListener, (const std::string& client, const uint32_t& keyCode, const uint32_t& flags), (override));
    MOCK_METHOD(bool, injectKey, (const uint32_t& keyCode, const uint32_t& flags), (override));
    MOCK_METHOD(bool, generateKey, (const std::string& client, const uint32_t& keyCode, const uint32_t& flags, std::string virtualKey), (override));
    MOCK_METHOD(bool, generateKey, (const std::string& client, const uint32_t& keyCode, const uint32_t& flags, std::string virtualKey, double duration), (override));
    MOCK_METHOD(bool, getScreenResolution, (uint32_t& width, uint32_t& height), (override));
    MOCK_METHOD(bool, setScreenResolution, (uint32_t width, uint32_t height), (override));
    MOCK_METHOD(bool, getClients, (std::vector<std::string>& clients), (override));
    MOCK_METHOD(bool, getZOrder, (const std::string& client, int32_t& zorder), (override));
    MOCK_METHOD(bool, setZorder, (const std::string& client, int32_t zorder), (override));
    MOCK_METHOD(bool, getBounds, (const std::string& client, uint32_t& x, uint32_t& y, uint32_t& width, uint32_t& height), (override));
    MOCK_METHOD(bool, setBounds, (const std::string& client, uint32_t x, uint32_t y, uint32_t width, uint32_t height), (override));
    MOCK_METHOD(bool, getVisibility, (const std::string& client, bool& visible), (override));
    MOCK_METHOD(bool, setVisibility, (const std::string& client, bool visible), (override));
    MOCK_METHOD(bool, getOpacity, (const std::string& client, unsigned int& opacity), (override));
    MOCK_METHOD(bool, setOpacity, (const std::string& client, unsigned int opacity), (override));
    MOCK_METHOD(bool, getScale, (const std::string& client, double& scaleX, double& scaleY), (override));
    MOCK_METHOD(bool, setScale, (const std::string& client, double scaleX, double scaleY), (override));
    MOCK_METHOD(bool, getHolePunch, (const std::string& client, bool& holePunch), (override));
    MOCK_METHOD(bool, setHolePunch, (const std::string& client, bool holePunch), (override));
    MOCK_METHOD(bool, getCrop, (const std::string& client, int32_t& cropX, int32_t& cropY, int32_t& cropWidth, int32_t& cropHeight), (override));
    MOCK_METHOD(bool, setCrop, (const std::string& client, int32_t cropX, int32_t cropY, int32_t cropWidth, int32_t cropHeight), (override));
    MOCK_METHOD(bool, scaleToFit, (const std::string& client, int32_t x, int32_t y, uint32_t width, uint32_t height), (override));
    MOCK_METHOD(void, onKeyPress, (uint32_t keycode, uint32_t flags, uint64_t metadata, bool physicalKeyPress), (override));
    MOCK_METHOD(void, onKeyRelease, (uint32_t keycode, uint32_t flags, uint64_t metadata, bool physicalKeyPress), (override));
    MOCK_METHOD(void, onPointerMotion, (uint32_t x, uint32_t y), (override));
    MOCK_METHOD(void, onPointerButtonPress, (uint32_t keyCode, uint32_t x, uint32_t y), (override));
    MOCK_METHOD(void, onPointerButtonRelease, (uint32_t keyCode, uint32_t x, uint32_t y), (override));
    MOCK_METHOD(bool, createDisplay, (const std::string& client, const std::string& displayName,
                                      uint32_t displayWidth, uint32_t displayHeight,
                                      bool virtualDisplayEnabled, uint32_t virtualWidth, uint32_t virtualHeight,
                                      bool topmost, bool focus, int32_t ownerId, int32_t groupId), (override));
    MOCK_METHOD(bool, addListener, (const std::string& client, std::shared_ptr<RdkWindowManagerEventListener> listener), (override));
    MOCK_METHOD(bool, removeListener, (const std::string& client, std::shared_ptr<RdkWindowManagerEventListener> listener), (override));
    MOCK_METHOD(bool, onEvent, (RdkCompositor* eventCompositor, const std::string& eventName), (override));
    MOCK_METHOD(void, enableInactivityReporting, (bool enable), (override));
    MOCK_METHOD(void, setInactivityInterval, (double minutes), (override));
    MOCK_METHOD(void, resetInactivityTime, (), (override));
    MOCK_METHOD(double, getInactivityTimeInMinutes, (), (override));
    MOCK_METHOD(void, setEventListener, (std::shared_ptr<RdkWindowManagerEventListener> listener), (override));
    MOCK_METHOD(bool, draw, (), (override));
    MOCK_METHOD(bool, update, (), (override));
    MOCK_METHOD(bool, setLogLevel, (const std::string level), (override));
    MOCK_METHOD(bool, getLogLevel, (std::string& level), (override));
    MOCK_METHOD(bool, setTopmost, (const std::string& client, bool topmost, bool focus), (override));
    MOCK_METHOD(bool, getTopmost, (std::string& client), (override));
    MOCK_METHOD(bool, sendEvent, (const std::string& eventName, (std::vector<std::map<std::string, RdkWindowManagerData>>&) data), (override));
    MOCK_METHOD(bool, enableKeyRepeats, (bool enable), (override));
    MOCK_METHOD(bool, getKeyRepeatsEnabled, (bool& enable), (override));
    MOCK_METHOD(bool, getVirtualResolution, (const std::string& client, uint32_t& virtualWidth, uint32_t& virtualHeight), (override));
    MOCK_METHOD(bool, setVirtualResolution, (const std::string& client, uint32_t virtualWidth, uint32_t virtualHeight), (override));
    MOCK_METHOD(bool, enableVirtualDisplay, (const std::string& client, bool enable), (override));
    MOCK_METHOD(bool, getVirtualDisplayEnabled, (const std::string& client, bool& enabled), (override));
    MOCK_METHOD(bool, getLastKeyPress, (uint32_t& keyCode, uint32_t& modifiers, uint64_t& timestampInSeconds), (override));
    MOCK_METHOD(bool, ignoreKeyInputs, (bool ignore), (override));
    MOCK_METHOD(bool, screenShot, (uint8_t*& data, uint32_t& size), (override));
    MOCK_METHOD(bool, enableInputEvents, (const std::string& client, bool enable), (override));
    MOCK_METHOD(bool, showCursor, (), (override));
    MOCK_METHOD(bool, hideCursor, (), (override));
    MOCK_METHOD(bool, setCursorSize, (uint32_t width, uint32_t height), (override));
    MOCK_METHOD(bool, getCursorSize, (uint32_t& width, uint32_t& height), (override));
    MOCK_METHOD(void, setKeyRepeatConfig, (bool enabled, int32_t initialDelay, int32_t repeatInterval), (override));
    MOCK_METHOD(bool, setAVBlocked, (std::string callsign, bool blockAV), (override));
    MOCK_METHOD(bool, getBlockedAVApplications, (std::vector<std::string>& apps), (override));
    MOCK_METHOD(bool, isErmEnabled, (), (override));
    MOCK_METHOD(bool, getClientInfo, (const std::string& client, ClientInfo& ci), (override));
    MOCK_METHOD(bool, setClientInfo, (const std::string& client, const ClientInfo& ci), (override));
    MOCK_METHOD(bool, enableDisplayRender, (const std::string& client, bool enable), (override));
    MOCK_METHOD(bool, renderReady, (const std::string& client), (override));
    MOCK_METHOD(bool, startVncServer, (), (override));
    MOCK_METHOD(bool, stopVncServer, (), (override));
};

// =============================================================
// Mock for RdkWindowManager namespace free functions
// =============================================================
class RdkWindowManagerImplMock : public RdkWindowManagerImpl
{
public:
    RdkWindowManagerImplMock() = default;
    virtual ~RdkWindowManagerImplMock() = default;

    MOCK_METHOD(void,   initialize,    (), (override));
    MOCK_METHOD(void,   run,           (), (override));
    MOCK_METHOD(void,   update,        (), (override));
    MOCK_METHOD(void,   draw,          (), (override));
    MOCK_METHOD(void,   deinitialize,  (), (override));
    MOCK_METHOD(double, seconds,       (), (override));
    MOCK_METHOD(double, milliseconds,  (), (override));
    MOCK_METHOD(double, microseconds,  (), (override));
};

} // namespace RdkWindowManager
