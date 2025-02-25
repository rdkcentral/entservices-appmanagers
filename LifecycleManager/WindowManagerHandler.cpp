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

namespace WPEFramework {
namespace Plugin {

WindowManagerHandler::WindowManagerHandler()
: mWindowManager(nullptr), mEventHandler(nullptr)
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
        // PENDING any event handler registrations
        //registerEventHandlers();
    }
    else
    {
        LOGERR("windowmanager is null \n");
    }
    return ret;
}

void WindowManagerHandler::terminate()
{
    uint32_t result = mWindowManager->Release();
    LOGINFO("Window manager releases [%d]\n", result);
    mWindowManager = nullptr;
}

bool WindowManagerHandler::createDisplay(const string& appPath, const string& appConfig, const string& runtimeAppId, const string& runtimePath, const string& runtimeConfig, const string& launchArgs, string& errorReason)
{
    JsonObject displayParams;
    displayParams["client"] = runtimeAppId;
    string displayParamsString;
    displayParams.ToString(displayParamsString);
    Core::hresult createDisplayResult = mWindowManager->CreateDisplay(displayParamsString);
    if (Core::ERROR_NONE != createDisplayResult)
    {
        errorReason = "unable to create display for application";
        return false;
    }
    return true;
}
} // namespace Plugin
} // namespace WPEFramework
