/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2025 RDK Management
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

#include "AppManagerTelemetryReporting.h"
#include "AppInfoManager.h"
#include "UtilsLogging.h"
#include "tracing/Logging.h"

namespace WPEFramework
{
namespace Plugin
{
    AppManagerTelemetryReporting::AppManagerTelemetryReporting()
    {
    }

    AppManagerTelemetryReporting::~AppManagerTelemetryReporting()
    {
    }

    AppManagerTelemetryReporting& AppManagerTelemetryReporting::getInstance()
    {
        LOGINFO("Get AppManagerTelemetryReporting Instance");
        static AppManagerTelemetryReporting instance;
        return instance;
    }

    void AppManagerTelemetryReporting::initialize(PluginHost::IShell* service)
    {
        ASSERT(nullptr != service);
        setService(service);
        if(Core::ERROR_NONE != initializeTelemetryClient())
        {
            LOGERR("Failed to create TelemetryMetricsObject\n");
        }
    }

    void AppManagerTelemetryReporting::reportTelemetryData(const std::string& appId, AppManagerImplementation::CurrentAction currentAction)
    {
        if (!Utils::isTelemetryMetricsEnabled()) {
            return;
        }

        JsonObject jsonParam;
        std::string telemetryMetrics = "";
        std::string markerName = "";
        time_t currentTime = getCurrentTimestamp();

        if(!ensureTelemetryClient())
        {
            LOGERR("Failed to create TelemetryMetricsObject\n");
        }

        AppInfo telSnap;
        bool telFound = AppInfoManager::getInstance().get(appId, telSnap);
        if(telFound && (currentAction == telSnap.getCurrentAction()) && isTelemetryClientAvailable())
        {
            LOGINFO("Received data for appId %s current action %d ",appId.c_str(), currentAction);

            switch(currentAction)
            {
                // APP_ACTION_LAUNCH and APP_ACTION_PRELOAD appManagerLaunchTime is now recorded via recordLaunchTime()
                case AppManagerImplementation::APP_ACTION_LAUNCH:
                case AppManagerImplementation::APP_ACTION_PRELOAD:
                break;
                case AppManagerImplementation::APP_ACTION_CLOSE:
                    if ((Exchange::IAppManager::AppLifecycleState::APP_STATE_SUSPENDED != telSnap.getTargetAppState()) &&
                        (Exchange::IAppManager::AppLifecycleState::APP_STATE_HIBERNATED != telSnap.getTargetAppState()))
                    {
                        jsonParam["appManagerCloseTime"] = (int)(currentTime - telSnap.getCurrentActionTime());
                        markerName = TELEMETRY_MARKER_CLOSE_TIME;
                    }
                break;
                case AppManagerImplementation::APP_ACTION_TERMINATE:
                case AppManagerImplementation::APP_ACTION_KILL:
                    jsonParam["appManagerCloseTime"] = (int)(currentTime - telSnap.getCurrentActionTime());
                    markerName = TELEMETRY_MARKER_CLOSE_TIME;
                break;
                default:
                    LOGERR("currentAction is invalid");
                break;
            }

            if(!markerName.empty())
            {
                jsonParam.ToString(telemetryMetrics);
                if(!telemetryMetrics.empty())
                {
                    getTelemetryClient().record(appId, telemetryMetrics, markerName);
                }
            }
        }
        else
        {
            LOGERR("Failed to report telemetry data as appId/currentAction or TelemetryMetrics client is not valid");
        }
    }

    void AppManagerTelemetryReporting::reportTelemetryDataOnStateChange(const string& appId, const Exchange::ILifecycleManager::LifecycleState newState)
    {
        if (!Utils::isTelemetryMetricsEnabled()) {
            return;
        }

        JsonObject jsonParam;
        std::string telemetryMetrics = "";
        std::string markerName = "";
        time_t currentTime = getCurrentTimestamp();

        if(!ensureTelemetryClient())
        {
            LOGERR("Failed to create TelemetryMetricsObject\n");
        }

        AppInfo stateSnap;
        bool stateFound = AppInfoManager::getInstance().get(appId, stateSnap);
        if(stateFound && isTelemetryClientAvailable())
        {
            switch(stateSnap.getCurrentAction())
            {
                case AppManagerImplementation::APP_ACTION_LAUNCH:
                    if(Exchange::ILifecycleManager::LifecycleState::ACTIVE == newState)
                    {
                        jsonParam["totalLaunchTime"] = (int)(currentTime - stateSnap.getCurrentActionTime());
                        jsonParam["launchType"] = ((AppManagerTypes::APPLICATION_TYPE_INTERACTIVE == stateSnap.getPackageInfo().type)?"LAUNCH_INTERACTIVE":"START_SYSTEM");
                        markerName = TELEMETRY_MARKER_LAUNCH_TIME;
                    }
                break;
                case AppManagerImplementation::APP_ACTION_PRELOAD:
                    if(Exchange::ILifecycleManager::LifecycleState::PAUSED == newState)
                    {
                        jsonParam["totalLaunchTime"] = (int)(currentTime - stateSnap.getCurrentActionTime());
                        jsonParam["launchType"] = ((AppManagerTypes::APPLICATION_TYPE_INTERACTIVE == stateSnap.getPackageInfo().type)?"PRELOAD_INTERACTIVE":"START_SYSTEM");
                        markerName = TELEMETRY_MARKER_LAUNCH_TIME;
                    }
                break;
                case AppManagerImplementation::APP_ACTION_CLOSE:
                case AppManagerImplementation::APP_ACTION_TERMINATE:
                case AppManagerImplementation::APP_ACTION_KILL:
                        if(Exchange::ILifecycleManager::LifecycleState::UNLOADED == newState)
                        {
                            jsonParam["totalCloseTime"] = (int)(currentTime - stateSnap.getCurrentActionTime());
                            jsonParam["closeType"] = ((AppManagerImplementation::APP_ACTION_CLOSE == stateSnap.getCurrentAction())?"CLOSE":((AppManagerImplementation::APP_ACTION_TERMINATE == stateSnap.getCurrentAction())?"TERMINATE":"KILL"));
                            markerName = TELEMETRY_MARKER_CLOSE_TIME;
                        }
                break;
                default:
                    LOGERR("currentAction is invalid");
                break;
            }

            if(!markerName.empty())
            {
                jsonParam["appId"] = appId;
                jsonParam["appInstanceId"] = stateSnap.getAppInstanceId();
                jsonParam["appVersion"] = stateSnap.getPackageInfo().version;
                jsonParam.ToString(telemetryMetrics);
                if(!telemetryMetrics.empty())
                {
                    getTelemetryClient().record(appId, telemetryMetrics, markerName);
                    getTelemetryClient().publish(appId, markerName);
                }
            }
        }
        else
        {
            LOGERR("Failed to report telemetry data as appId/TelemetryMetrics client is not valid");
        }
    }

    void AppManagerTelemetryReporting::reportTelemetryErrorData(const std::string& appId, AppManagerImplementation::CurrentAction currentAction, AppManagerImplementation::CurrentActionError errorCode)
    {
        if (!Utils::isTelemetryMetricsEnabled()) {
            return;
        }

        JsonObject jsonParam;
        std::string telemetryMetrics = "";
        std::string markerName = "";

        LOGINFO("Received data for appId %s current action %d app errorCode %d",appId.c_str(), currentAction, errorCode);

        if(!ensureTelemetryClient())
        {
            LOGERR("Failed to create TelemetryMetricsObject\n");
        }

        switch(currentAction)
        {
            case AppManagerImplementation::APP_ACTION_LAUNCH:
            case AppManagerImplementation::APP_ACTION_PRELOAD:
                markerName = TELEMETRY_MARKER_LAUNCH_ERROR;
            break;
            case AppManagerImplementation::APP_ACTION_CLOSE:
            case AppManagerImplementation::APP_ACTION_TERMINATE:
            case AppManagerImplementation::APP_ACTION_KILL:
                markerName = TELEMETRY_MARKER_CLOSE_ERROR;
            break;
            default:
                LOGERR("currentAction is invalid");
            break;
        }

        if(!markerName.empty() && isTelemetryClientAvailable())
        {
            jsonParam["errorCode"] = (int)errorCode;
            jsonParam.ToString(telemetryMetrics);
            if(!telemetryMetrics.empty())
            {
                getTelemetryClient().record(appId, telemetryMetrics, markerName);
                getTelemetryClient().publish(appId, markerName);
            }
        }
    }

    void AppManagerTelemetryReporting::recordLaunchTime(const std::string& appId, int launchTimeMs)
    {
        if (!Utils::isTelemetryMetricsEnabled()) {
            return;
        }

        if (!ensureTelemetryClient())
        {
            LOGERR("Failed to create TelemetryMetrics client");
            return;
        }

        JsonObject jsonParam;
        jsonParam["appManagerLaunchTime"] = launchTimeMs;

        std::string telemetryMetrics;
        jsonParam.ToString(telemetryMetrics);
        if (!telemetryMetrics.empty())
        {
            getTelemetryClient().record(appId, telemetryMetrics, TELEMETRY_MARKER_LAUNCH_TIME);
        }
    }

    void AppManagerTelemetryReporting::reportAppCrashedTelemetry(const std::string& appId, const std::string& appInstanceId, const std::string& crashReason)
    {
        if (!Utils::isTelemetryMetricsEnabled()) {
            return;
        }

        LOGINFO("Received app crash data for appId %s appInstanceId %s crashReason %s", appId.c_str(), appInstanceId.c_str(), crashReason.c_str());

        if (!ensureTelemetryClient())
        {
            LOGERR("Failed to create TelemetryMetrics client");
            return;
        }

        JsonObject jsonParam;
        jsonParam["appId"] = appId;
        jsonParam["appInstanceId"] = appInstanceId;
        jsonParam["crashReason"] = crashReason;

        std::string telemetryMetrics;
        jsonParam.ToString(telemetryMetrics);
        if (!telemetryMetrics.empty())
        {
            getTelemetryClient().record(appId, telemetryMetrics, TELEMETRY_MARKER_APP_CRASHED);
            getTelemetryClient().publish(appId, TELEMETRY_MARKER_APP_CRASHED);
        }
    }

} /* namespace Plugin */
} /* namespace WPEFramework */

