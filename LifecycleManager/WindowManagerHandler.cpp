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

#include "WindowManagerHandler.h"
#include <fstream>
#include <random>

namespace WPEFramework {
namespace Plugin {

WindowManagerHandler::WindowManagerHandler()
: mWindowManager(nullptr), mWindowManagerNotification(*this), mEventHandler(nullptr)
{
    LOGINFO("Create WindowManagerHandler Instance");
}

WindowManagerHandler::~WindowManagerHandler()
{
    LOGINFO("Delete WindowManagerHandler Instance");
}

bool WindowManagerHandler::initialize(PluginHost::IShell* service, IEventHandler* eventHandler)
{
    bool ret = false;
    mEventHandler = eventHandler;
    mWindowManager = service->QueryInterfaceByCallsign<Exchange::IRDKWindowManager>("org.rdk.RDKWindowManager");
    if (mWindowManager != nullptr)
    {
        ret = true;
        Core::hresult registerResult = mWindowManager->Register(&mWindowManagerNotification);
        if (Core::ERROR_NONE != registerResult)
        {
            LOGWARN("Unable to register with WindowManager callsign=org.rdk.RDKWindowManager result=%d", registerResult);
        }
    }
    else
    {
        LOGERR("WindowManager is null - unable to initialize handler");
    }
    return ret;
}

void WindowManagerHandler::terminate()
{
    if (mWindowManager != nullptr)
    {
        Core::hresult unregisterResult = mWindowManager->Unregister(&mWindowManagerNotification);
        if (Core::ERROR_NONE != unregisterResult)
        {
            LOGINFO("Unable to unregister from windowmanager [%d] \n", unregisterResult);
        }
        uint32_t result = mWindowManager->Release();
        LOGINFO("Window manager releases [%d]\n", result);
        mWindowManager = nullptr;
    }
}

void WindowManagerHandler::WindowManagerNotification::OnUserInactivity(const double minutes)
{
    printf(" Received onUserInactivity event for %f minutes \n", minutes);
    fflush(stdout);
    JsonObject eventData;
    eventData["minutes"] = minutes;
    eventData["name"] = "onUserInactivity";
    _parent.onEvent(eventData);
}

void WindowManagerHandler::WindowManagerNotification::OnDisconnected(const std::string& client)
{
    printf(" Received onDisconnect event for client[%s] \n", client.c_str());
    fflush(stdout);
    JsonObject eventData;
    eventData["client"] = client;
    eventData["name"] = "onDisconnect";
    _parent.onEvent(eventData);
}

void WindowManagerHandler::onEvent(JsonObject& data)
{
    if (mEventHandler)
    {
        mEventHandler->onWindowManagerEvent(data);
    }
}

Core::hresult WindowManagerHandler::renderReady(std::string appInstanceId, bool& isReady)
{
    Core::hresult renderReadyResult = mWindowManager->RenderReady(appInstanceId, isReady);
    if (Core::ERROR_NONE != renderReadyResult)
    {
        printf("unable to get render ready from window manager [%d] \n", renderReadyResult);
	fflush(stdout);
    }
    return renderReadyResult;
}

Core::hresult WindowManagerHandler::enableDisplayRender(std::string appInstanceId, bool render)
{
    Core::hresult enableDisplayRenderResult = mWindowManager->EnableDisplayRender(appInstanceId, render);
    if (Core::ERROR_NONE != enableDisplayRenderResult)
    {
        printf("unable to perform enable display renderer from window manager [%d] \n", enableDisplayRenderResult);
	fflush(stdout);
    }
    return enableDisplayRenderResult;
}

void WindowManagerHandler::WindowManagerNotification::OnReady(const std::string &client)
{
    printf("Received onReady event for app[%s] \n", client.c_str());
    fflush(stdout);
    JsonObject eventData;
    eventData["appInstanceId"] = client;
    eventData["name"] = "onReady";
    _parent.onEvent(eventData);
}

void WindowManagerHandler::WindowManagerNotification::OnFocus(const std::string &client)
{
    printf("Received onFocus event for app[%s] \n", client.c_str());
    fflush(stdout);
    JsonObject eventData;
    eventData["appInstanceId"] = client;
    eventData["name"] = "onFocus";
    _parent.onEvent(eventData);
}

void WindowManagerHandler::WindowManagerNotification::OnBlur(const std::string &client)
{
    printf("Received onBlur event for app[%s] \n", client.c_str());
    fflush(stdout);
    JsonObject eventData;
    eventData["appInstanceId"] = client;
    eventData["name"] = "onBlur";
    _parent.onEvent(eventData);
}

Core::hresult WindowManagerHandler::setWindowBounds(const std::string& appInstanceId, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    if (mWindowManager == nullptr)
    {
        LOGERR("setWindowBounds: WindowManager is null for appInstanceId=%s", appInstanceId.c_str());
        return Core::ERROR_GENERAL;
    }
    Core::hresult result = mWindowManager->SetBounds(appInstanceId, x, y, width, height);
    if (Core::ERROR_NONE != result)
    {
        LOGERR("setWindowBounds: Failed for appInstanceId=%s x=%u y=%u width=%u height=%u result=%d",
               appInstanceId.c_str(), x, y, width, height, result);
    }
    else
    {
        LOGINFO("setWindowBounds: appInstanceId=%s x=%u y=%u width=%u height=%u",
                appInstanceId.c_str(), x, y, width, height);
    }
    return result;
}

Core::hresult WindowManagerHandler::getWindowBounds(const std::string& appInstanceId, uint32_t& x, uint32_t& y, uint32_t& width, uint32_t& height)
{
    if (mWindowManager == nullptr)
    {
        LOGERR("getWindowBounds: WindowManager is null for appInstanceId=%s", appInstanceId.c_str());
        return Core::ERROR_GENERAL;
    }
    Core::hresult result = mWindowManager->GetBounds(appInstanceId, x, y, width, height);
    if (Core::ERROR_NONE != result)
    {
        LOGERR("getWindowBounds: Failed for appInstanceId=%s result=%d", appInstanceId.c_str(), result);
    }
    else
    {
        LOGINFO("getWindowBounds: appInstanceId=%s x=%u y=%u width=%u height=%u",
                appInstanceId.c_str(), x, y, width, height);
    }
    return result;
}

} // namespace Plugin
} // namespace WPEFramework
