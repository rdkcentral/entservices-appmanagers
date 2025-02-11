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
#include <interfaces/IRDKWindowManager.h>
#include <map>
#include <plugins/plugins.h>
#include "IEventHandler.h"
#include "UtilsLogging.h"
#include "tracing/Logging.h"

namespace WPEFramework {
namespace Plugin {
    class WindowManagerHandler
    {
        public:
            WindowManagerHandler();
            ~WindowManagerHandler();

            // We do not allow this plugin to be copied !!
            WindowManagerHandler(const WindowManagerHandler&) = delete;
            WindowManagerHandler& operator=(const WindowManagerHandler&) = delete;

        public:
            bool initialize(PluginHost::IShell* service, IEventHandler* eventHandler);
	    void terminate();
            bool createDisplay(const string& appPath, const string& appConfig, const string& runtimeAppId, const string& runtimePath, const string& runtimeConfig, const string& launchArgs, string& errorReason);

        private:
            PluginHost::IShell *mWindowManagerController;
            Exchange::IRDKWindowManager* mWindowManager;
            IEventHandler* mEventHandler;
    };
} // namespace Plugin
} // namespace WPEFramework
