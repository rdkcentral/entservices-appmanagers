/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2020 RDK Management
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

#pragma once

#include "Module.h"
#include "UtilsLogging.h"
#include <map>
#include <string>
#include <mutex>
#include <condition_variable>
#include "rialto/ServerManagerServiceFactory.h"

#include "AIConfiguration.h"
namespace WPEFramework
{
    using namespace rialto::servermanager::service;
    // Shadowing to keep Rialto contained in this
    typedef firebolt::rialto::common::SessionServerState RialtoServerStates;

    class RialtoConnector : public IStateObserver, public std::enable_shared_from_this<RialtoConnector>
    {
    public:
        RialtoConnector() : mInitialized(false) {}
        virtual ~RialtoConnector() = default;
        void initialize();
        bool waitForStateChange(const std::string &appid, const RialtoServerStates &state, int timeoutMillis);
        bool createAppSession(const std::string &callsign, const std::string &displayName, const std::string &appId);
        bool resumeSession(const std::string &callsign);
        bool suspendSession(const std::string &callsign);
 
        bool deactivateSession(const std::string &callsign);
        void stateChanged(const std::string &appId, const RialtoServerStates &state) override;
	std::string getSocketPath(const std::string &appId) const;

        RialtoConnector(const RialtoConnector &) = delete;            // No copying
        RialtoConnector &operator=(const RialtoConnector &) = delete; // No assignment

    private:
        class RialtoLogHandler : public rialto::servermanager::service::ILogHandler
        {
        public:
            void log(Level level, const std::string & /*file*/, int line,
                     const std::string &function, const std::string &message) const override
            {
                switch (level)
                {
                    case Level::Fatal:
                    case Level::Error:
                        LOGERR("[rialto][%s:%d] %s", function.c_str(), line, message.c_str());
                        break;
                    case Level::Warning:
                    case Level::Milestone:
                        LOGWARN("[rialto][%s:%d] %s", function.c_str(), line, message.c_str());
                        break;
                    case Level::Info:
                    case Level::Debug:
                    case Level::External:
                    default:
                        LOGINFO("[rialto][%s:%d] %s", function.c_str(), line, message.c_str());
                        break;
                }
            }
        };

        bool mInitialized;
        std::mutex m_stateMutex;
        std::condition_variable m_stateCond;
        std::unique_ptr<IServerManagerService> mServerManagerService;
        std::shared_ptr<rialto::servermanager::service::ILogHandler> mLogHandler;
        std::map<std::string, RialtoServerStates> appStateMap;
        const RialtoServerStates getCurrentAppState(const std::string &callsign);
      
    };
} // namespace WPEFramework
