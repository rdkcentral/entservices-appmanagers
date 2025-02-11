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

#include "WebSocket.h"
#include "IEventHandler.h"
#include <string>

namespace WPEFramework {
namespace Plugin {

    class RippleHandler
    {
        public:
            RippleHandler();
            ~RippleHandler();

            // We do not allow this plugin to be copied !!
            RippleHandler(const RippleHandler&) = delete;
            RippleHandler& operator=(const RippleHandler&) = delete;

        public:
            bool initialize(IEventHandler* eventHandler);
	    void terminate();
            bool sendIntent(std::string& appId, const std::string& intent, std::string& errorReason);

        private:
            WebSocketEndPoint* mWSEndPoint;
            int mRippleConnectionId;
	    std::string mFireboltEndpoint;
            IEventHandler* mEventHandler;
    };
} // namespace Plugin
} // namespace WPEFramework
