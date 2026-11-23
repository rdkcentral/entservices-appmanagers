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
#include <interfaces/ITelemetryMetrics.h>
#include <utility>

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
        time_t currentTime = currentTimestampMs();

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
                const bool shouldPublishCloseOnRecord =
                    (AppManagerImplementation::APP_ACTION_CLOSE == currentAction) &&
                    (Exchange::IAppManager::AppLifecycleState::APP_STATE_SUSPENDED != telSnap.getTargetAppState()) &&
                    (Exchange::IAppManager::AppLifecycleState::APP_STATE_HIBERNATED != telSnap.getTargetAppState());

                if (shouldPublishCloseOnRecord)
                {
                    /* For Close flow that keeps app in PAUSED (non-suspendable apps),
                     * there may be no UNLOADED state transition to trigger publish later.
                     * Publish close telemetry here to ensure AppCloseTime is reported to T2.
                     */
                    JsonObject publishParam = std::move(jsonParam);
                    publishParam["appId"] = appId;
                    publishParam["appInstanceId"] = telSnap.getAppInstanceId();
                    publishParam["appVersion"] = telSnap.getPackageInfo().version;
                    publishParam["closeType"] = "CLOSE";

                    publishParam.ToString(telemetryMetrics);
                    if(!telemetryMetrics.empty())
                    {
                        getTelemetryClient().record(appId, telemetryMetrics, markerName);
                        getTelemetryClient().publish(appId, markerName);
                    }
                }
                else
                {
                    jsonParam.ToString(telemetryMetrics);
                    if(!telemetryMetrics.empty())
                    {
                        getTelemetryClient().record(appId, telemetryMetrics, markerName);
                    }
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
        time_t currentTime = currentTimestampMs();

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

        LOGINFO("Received data for appId %s current action %d app errorCode %d",appId.c_str(), currentAction, errorCode);

        if ((AppManagerImplementation::APP_ACTION_CLOSE == currentAction) ||
            (AppManagerImplementation::APP_ACTION_TERMINATE == currentAction) ||
            (AppManagerImplementation::APP_ACTION_KILL == currentAction))
        {
            reportAppCloseErrorTelemetry(appId, currentAction, errorCode);
            return;
        }

        JsonObject jsonParam;
        std::string telemetryMetrics = "";
        std::string markerName = "";

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

    void AppManagerTelemetryReporting::reportAppCrashedTelemetry(const std::string& appId, const std::string& appVersion,
        Exchange::ILifecycleManager::LifecycleState lifecycleState)
    {
        if (!Utils::isTelemetryMetricsEnabled()) {
            return;
        }

        std::string state;
        switch (lifecycleState) {
            case Exchange::ILifecycleManager::ACTIVE:
                state = "ACTIVE";
                break;
            case Exchange::ILifecycleManager::PAUSED:
            case Exchange::ILifecycleManager::SUSPENDED:
            case Exchange::ILifecycleManager::HIBERNATED:
                state = "INACTIVE";
                break;
            default:
                return;
        }
        if (appId.empty() || appVersion.empty()) {
            LOGWARN("AppCrashed telemetry skipped: app identity or package version is missing for appId=%s", appId.c_str());
            return;
        }

        JsonObject jsonParam;
        jsonParam["appId"] = appId;
        jsonParam["appVersion"] = appVersion;
        jsonParam["lifecycleState"] = state;
        publishEventDirect(appId, jsonParam, TELEMETRY_MARKER_APP_CRASHED);
    }

    void AppManagerTelemetryReporting::reportAppCloseErrorTelemetry(const std::string& appId,
        AppManagerImplementation::CurrentAction currentAction, AppManagerImplementation::CurrentActionError errorCode)
    {
        switch (errorCode) {
            case AppManagerImplementation::ERROR_INVALID_PARAMS:
            case AppManagerImplementation::ERROR_INTERNAL:
            case AppManagerImplementation::ERROR_PACKAGE_UNLOCK:
            case AppManagerImplementation::ERROR_UNLOAD_APP:
            case AppManagerImplementation::ERROR_KILL_APP:
            case AppManagerImplementation::ERROR_SET_TARGET_APP_STATE:
                break;
            default:
                LOGWARN("AppCloseError telemetry skipped: unsupported errorCode %d for appId=%s", static_cast<int>(errorCode), appId.c_str());
                return;
        }

        // Prefer the app's outstanding close-type action: cleanup errors are reported with APP_ACTION_CLOSE
        // even when the close was started by terminate or kill.
        AppManagerImplementation::CurrentAction closeAction = currentAction;
        std::string appVersion;
        AppInfo snapshot;
        if (AppInfoManager::getInstance().get(appId, snapshot)) {
            appVersion = snapshot.getPackageInfo().version;
            const AppManagerImplementation::CurrentAction active = snapshot.getCurrentAction();
            if ((AppManagerImplementation::APP_ACTION_CLOSE == active) ||
                (AppManagerImplementation::APP_ACTION_TERMINATE == active) ||
                (AppManagerImplementation::APP_ACTION_KILL == active)) {
                closeAction = active;
            }
        }

        std::string closeType;
        switch (closeAction) {
            case AppManagerImplementation::APP_ACTION_CLOSE:
                closeType = ((Exchange::IAppManager::APP_STATE_SUSPENDED == snapshot.getTargetAppState()) ||
                             (Exchange::IAppManager::APP_STATE_HIBERNATED == snapshot.getTargetAppState())) ? "DEACTIVATE" : "CLOSE";
                break;
            case AppManagerImplementation::APP_ACTION_TERMINATE:
                closeType = "TERMINATE";
                break;
            case AppManagerImplementation::APP_ACTION_KILL:
                closeType = "KILL";
                break;
            default:
                LOGWARN("AppCloseError telemetry skipped: invalid close action %d for appId=%s", static_cast<int>(closeAction), appId.c_str());
                return;
        }
        if (appVersion.empty()) {
            LOGWARN("AppCloseError for appId=%s has no package version metadata", appId.c_str());
        }

        JsonObject jsonParam;
        jsonParam["appId"] = appId;
        jsonParam["appVersion"] = appVersion;
        jsonParam["closeType"] = closeType;
        jsonParam["errorCode"] = static_cast<int>(errorCode);
        publishEventDirect(appId, jsonParam, TELEMETRY_MARKER_CLOSE_ERROR);
    }

    void AppManagerTelemetryReporting::publishEventDirect(const std::string& appId, const JsonObject& jsonParam, const char* marker)
    {
        std::string telemetryMetrics;
        if (!buildTelemetryPayload(jsonParam, telemetryMetrics)) {
            LOGERR("Failed to serialize %s telemetry for appId=%s", marker, appId.c_str());
            return;
        }
        PluginHost::IShell* service = getService();
        if (nullptr == service) {
            LOGWARN("%s telemetry skipped: AppManager service is unavailable", marker);
            return;
        }
        Exchange::ITelemetryMetrics* telemetry =
            service->QueryInterfaceByCallsign<Exchange::ITelemetryMetrics>("org.rdk.TelemetryMetrics");
        if (nullptr == telemetry) {
            LOGWARN("%s telemetry skipped: TelemetryMetrics is unavailable", marker);
            return;
        }
        const Core::hresult status = telemetry->PublishEvent(telemetryMetrics, marker, false);
        telemetry->Release();
        if (Core::ERROR_NONE != status) {
            LOGWARN("%s telemetry publishing failed for appId=%s (status=%d)", marker, appId.c_str(), status);
        }
    }

} /* namespace Plugin */
} /* namespace WPEFramework */


