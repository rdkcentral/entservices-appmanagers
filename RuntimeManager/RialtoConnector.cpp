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

#include <iostream>
#include <cstdlib>
#include "RialtoConnector.h"


namespace WPEFramework
{
    namespace
    {
        constexpr int RIALTO_TIMEOUT_MILLIS = 5000;
    }

    bool RialtoConnector::initialize()
    {
     if (!mInitialized)
     {
        WPEFramework::Plugin::AIConfiguration* mAIConfiguration;
        LOGWARN("Initializing rialto connector.... Rialto Bridge version 1.1");
        firebolt::rialto::common::ServerManagerConfig config;
        mAIConfiguration = new WPEFramework::Plugin::AIConfiguration();
        mAIConfiguration->initialize();
        config.sessionServerEnvVars = mAIConfiguration->getEnvs();
        mServerManagerService  = create(shared_from_this(), config);
	if (!mServerManagerService)
        {
            LOGERR("Failed to create Rialto ServerManagerService");
            delete mAIConfiguration;
            return false;
        }
	mLogHandler = std::make_shared<RialtoLogHandler>();
        if (!mServerManagerService->registerLogHandler(mLogHandler))
        {
            LOGWARN("Registration of custom logger for Rialto server manager failed");
        }

        mInitialized = true;
        delete mAIConfiguration;
     }
     return true;
    }

    std::string RialtoConnector::getSocketPath(const std::string &appId) const
    {
        if (!mServerManagerService)
        {
            LOGERR("getSocketPath: ServerManagerService is null for appId='%s'", appId.c_str());
            return "";
        }
        return mServerManagerService->getAppConnectionInfo(appId);
    }

    bool RialtoConnector::createAppSession(const std::string &callsign, const std::string &displayName, const std::string &appId)
    {
        LOGINFO("Creating app session with callsign : '%s', display name : '%s', appid : '%s'", callsign.c_str(), displayName.c_str(), appId.c_str());
	if (!mServerManagerService)
        {
            LOGERR("createAppSession: ServerManagerService is null for appId='%s'", appId.c_str());
            return false;
        }
        if (!callsign.empty() && !displayName.empty() && ! appId.empty())
        {
           firebolt::rialto::common::AppConfig config = {appId, displayName};
           return mServerManagerService ->initiateApplication(callsign,
                                                           RialtoServerStates::ACTIVE,
                                                           config);
        }
        else
        {
           LOGERR("Create app session failed..");
           return false;
        }
    }
    bool RialtoConnector::resumeSession(const std::string &callsign)
    {
        if (!mServerManagerService)
        {
            LOGERR("resumeSession: ServerManagerService is null for callsign='%s'",
                callsign.c_str());
            return false;
        }

        if (RialtoServerStates::INACTIVE == getCurrentAppState(callsign))
        {
        LOGINFO("db982 resumeSession: changing session state to ACTIVE for callsign='%s'",
                callsign.c_str());

        if (mServerManagerService->changeSessionServerState(
                callsign, RialtoServerStates::ACTIVE))
        {
            if (!waitForStateChange(
                    callsign, RialtoServerStates::ACTIVE, RIALTO_TIMEOUT_MILLIS))
            {
                LOGERR("db982 resumeSession: Timeout waiting for Rialto server to become ACTIVE for callsign='%s'",
                        callsign.c_str());
                return false;
            }

            LOGINFO("db982 resumeSession: Rialto server is ACTIVE for callsign='%s'",
                    callsign.c_str());

            return true;
        }
        else
        {
            LOGERR("db982 resumeSession: Failed to change session state to ACTIVE for callsign='%s'",
                    callsign.c_str());
            return false;
        }
        }
        else
        {
        LOGINFO("db982 resumeSession: Rialto server is not in INACTIVE state for callsign='%s'",
                callsign.c_str());
        }

        return false;

    }

    bool RialtoConnector::suspendSession(const std::string &callsign)
    {
    if (!mServerManagerService)
    {
        LOGERR("suspendSession: ServerManagerService is null for callsign='%s'",
            callsign.c_str());
        return false;
    }

    if (RialtoServerStates::ACTIVE == getCurrentAppState(callsign))
    {
        LOGINFO("db982 suspendSession: changing session state to INACTIVE for callsign='%s'",
                callsign.c_str());

        if (mServerManagerService->changeSessionServerState(
                callsign, RialtoServerStates::INACTIVE))
        {
            if (!waitForStateChange(
                callsign, RialtoServerStates::INACTIVE, RIALTO_TIMEOUT_MILLIS))
            {
                LOGERR("suspendSession: Timeout waiting for Rialto server to become INACTIVE for callsign='%s'",
                    callsign.c_str());
                return false;
            }

            LOGINFO("db982 suspendSession: Rialto server is INACTIVE for callsign='%s'",
                    callsign.c_str());

            return true;
        }
        else
        {
            LOGERR("suspendSession: Failed to change session state to INACTIVE for callsign='%s'",
                callsign.c_str());
            return false;
        }
    }

    LOGINFO("db982 suspendSession: Rialto server is not in ACTIVE state for callsign='%s'",
            callsign.c_str());

    return false;

    }


    const RialtoServerStates RialtoConnector::getCurrentAppState(const std::string &callsign)
    {
        auto state = appStateMap.find(callsign);
        if (state != appStateMap.end())
        {
            // If the state is not inactive, we have a problem.
            return state->second;
        }
        return RialtoServerStates::ERROR;
    }
    bool RialtoConnector::deactivateSession(const std::string &callsign)
    {
        LOGINFO("Deactiving app %s", callsign.c_str());
	if (!mServerManagerService)
        {
            LOGERR("deactivateSession: ServerManagerService is null for callsign='%s'", callsign.c_str());
            return false;
        }
        RialtoServerStates state = getCurrentAppState(callsign);
        if (RialtoServerStates::ACTIVE == state ||
            RialtoServerStates::INACTIVE == state)
            return mServerManagerService ->changeSessionServerState(callsign,
                                                                    RialtoServerStates::NOT_RUNNING);
        LOGINFO("Rialto server is not in active or running state. ");
        return false;
    }
    void RialtoConnector::stateChanged(const std::string &appId,
                                       const RialtoServerStates &state)
    {
        {
            std::lock_guard<std::mutex> lockguard(m_stateMutex);
            appStateMap[appId] = state;
        }
        LOGINFO("[RialtoConnector::stateChanged] State change announced for %s to %d, isActive ? %d ", appId.c_str(),
                static_cast<int>(state), (state == RialtoServerStates::ACTIVE));
        m_stateCond.notify_one();
    }

    // wait until socket is in given state
    // return true when state set, false on timeout
    bool RialtoConnector::waitForStateChange(const std::string& appId, const RialtoServerStates& state, int timeoutMillis)
    {
        bool status = false;
        std::unique_lock<std::mutex> lock(m_stateMutex);
        auto startTime = std::chrono::steady_clock::now();
        auto endTime = startTime + std::chrono::milliseconds(timeoutMillis);

        while (std::chrono::steady_clock::now() < endTime)
        {
            if (appStateMap[appId] == state)
            {
                status = true;
                break;
            }

            auto remainingTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - std::chrono::steady_clock::now());
            if (remainingTime.count() <= 0)
                break;

            m_stateCond.wait_for(lock, remainingTime);
        }

        return status;
    }
} // namespace WPEFramework
