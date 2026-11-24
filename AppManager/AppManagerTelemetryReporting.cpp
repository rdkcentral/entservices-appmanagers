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
#include <algorithm>
#include <cctype>
#include <climits>
#include <sstream>
#include <utility>

namespace WPEFramework
{
namespace Plugin
{
    namespace
    {
        std::string initialLaunchStateName(Exchange::IAppManager::AppLifecycleState state)
        {
            switch (state) {
                case Exchange::IAppManager::APP_STATE_UNLOADED:
                    return "COLD";
                case Exchange::IAppManager::APP_STATE_HIBERNATED:
                    return "HIBERNATED";
                case Exchange::IAppManager::APP_STATE_SUSPENDED:
                    return "SUSPENDED";
                case Exchange::IAppManager::APP_STATE_PAUSED:
                    return "PAUSED";
                default:
                    return "";
            }
        }

        bool hasCapability(const std::string& capabilities, const std::string& expectedCapability)
        {
            std::istringstream stream(capabilities);
            std::string token;
            while (std::getline(stream, token, ',')) {
                const size_t parameterStart = token.find('=');
                if (parameterStart != std::string::npos) {
                    token.erase(parameterStart);
                }
                const auto notSpace = [](unsigned char ch) { return !std::isspace(ch); };
                token.erase(token.begin(), std::find_if(token.begin(), token.end(), notSpace));
                token.erase(std::find_if(token.rbegin(), token.rend(), notSpace).base(), token.end());
                if (token == expectedCapability) {
                    return true;
                }
            }
            return false;
        }

        bool isHomeApp(const std::string& configMetadata)
        {
            JsonObject runtimeConfig;
            if (!runtimeConfig.FromString(configMetadata) || !runtimeConfig.HasLabel("capabilities")) {
                return false;
            }
            const JsonValue capabilities = runtimeConfig["capabilities"];
            return (JsonValue::type::STRING == capabilities.Content()) &&
                hasCapability(capabilities.String(), "home-app");
        }

        std::string appTypeName(const AppManagerTypes::PackageInfo& packageInfo)
        {
            if (isHomeApp(packageInfo.configMetadata)) {
                return "HOME_APP";
            }
            if (AppManagerTypes::APPLICATION_TYPE_INTERACTIVE == packageInfo.type) {
                return "INTERACTIVE_APP";
            }
            if (AppManagerTypes::APPLICATION_TYPE_SYSTEM == packageInfo.type) {
                return "SYSTEM_APP";
            }
            return "";
        }

        Exchange::IAppManager::AppErrorReason mapLaunchError(
            AppManagerImplementation::CurrentActionError errorCode)
        {
            switch (errorCode) {
                case AppManagerImplementation::ERROR_INVALID_PARAMS:
                    return Exchange::IAppManager::APP_ERROR_INVALID_PARAM;
                case AppManagerImplementation::ERROR_PACKAGE_NOT_INSTALLED:
                    return Exchange::IAppManager::APP_ERROR_NOT_INSTALLED;
                case AppManagerImplementation::ERROR_PACKAGE_LOCK:
                    return Exchange::IAppManager::APP_ERROR_PACKAGE_LOCK;
                default:
                    return Exchange::IAppManager::APP_ERROR_UNKNOWN;
            }
        }
    }

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
        if (stateFound &&
            (Exchange::ILifecycleManager::LifecycleState::ACTIVE == newState) &&
            ((AppManagerImplementation::APP_ACTION_LAUNCH == stateSnap.getCurrentAction()) ||
             (AppManagerImplementation::APP_ACTION_RESUME == stateSnap.getCurrentAction()))) {
            reportAppLaunchCompleteTelemetry(appId);
        }
        if (stateFound &&
            (AppManagerImplementation::APP_ACTION_PRELOAD == stateSnap.getCurrentAction()) &&
            (Exchange::ILifecycleManager::LifecycleState::PAUSED == newState)) {
            reportAppPreloadCompleteTelemetry(appId);
        }
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
                case AppManagerImplementation::APP_ACTION_RESUME:
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
                            if (AppManagerImplementation::APP_ACTION_CLOSE != stateSnap.getCurrentAction())
                            {
                                reportAppCloseCompleteTelemetry(appId, stateSnap.getCurrentAction());
                            }
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

        if (AppManagerImplementation::APP_ACTION_LAUNCH == currentAction) {
            reportAppLaunchErrorTelemetry(appId, mapLaunchError(errorCode));
            return;
        }

        if ((AppManagerImplementation::APP_ACTION_CLOSE == currentAction) ||
            (AppManagerImplementation::APP_ACTION_TERMINATE == currentAction) ||
            (AppManagerImplementation::APP_ACTION_KILL == currentAction))
        {
            reportAppCloseErrorTelemetry(appId, currentAction, errorCode);
            return;
        }

        if (AppManagerImplementation::APP_ACTION_PRELOAD == currentAction) {
            LOGINFO("AppLaunchError telemetry skipped for preload action appId=%s", appId.c_str());
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

    void AppManagerTelemetryReporting::reportAppLaunchErrorTelemetry(
        const std::string& appId, Exchange::IAppManager::AppErrorReason errorCode)
    {
        if (!Utils::isTelemetryMetricsEnabled()) {
            return;
        }
        if (appId.empty()) {
            LOGWARN("AppLaunchError telemetry skipped: appId is empty");
            return;
        }

        AppInfo snapshot;
        const bool found = AppInfoManager::getInstance().get(appId, snapshot);
        const AppManagerTypes::PackageInfo packageInfo = found
            ? snapshot.getPackageInfo() : AppManagerTypes::PackageInfo{};
        const std::string appVersion = found && !snapshot.getLaunchTelemetryVersion().empty()
            ? snapshot.getLaunchTelemetryVersion()
            : (!packageInfo.version.empty() ? packageInfo.version
                : (found ? snapshot.getRequestedLaunchVersion() : std::string{}));
        const std::string initialState = found
            ? initialLaunchStateName(snapshot.getLaunchInitialState()) : "COLD";
        const std::string appType = appTypeName(packageInfo);

        if (appVersion.empty() || initialState.empty() || appType.empty()) {
            LOGWARN("AppLaunchError telemetry has incomplete launch context for appId=%s (version=%d initialState=%d appType=%d)",
                appId.c_str(), !appVersion.empty(), !initialState.empty(), !appType.empty());
        }

        JsonObject jsonParam;
        jsonParam["appId"] = appId;
        jsonParam["appVersion"] = appVersion;
        jsonParam["initialState"] = initialState;
        jsonParam["appType"] = appType;
        jsonParam["errorCode"] = static_cast<uint32_t>(errorCode);
        jsonParam["fireboltIntent"] = found ? snapshot.getLaunchTelemetryIntent() : std::string{};
        publishEventDirect(appId, jsonParam, TELEMETRY_MARKER_LAUNCH_ERROR, false);
    }

    void AppManagerTelemetryReporting::reportAppLaunchCompleteTelemetry(const std::string& appId)
    {
        if (!Utils::isTelemetryMetricsEnabled()) {
            return;
        }
        if (appId.empty()) {
            LOGWARN("AppLaunchComplete telemetry skipped: appId is empty");
            return;
        }

        AppInfo snapshot;
        if (!AppInfoManager::getInstance().get(appId, snapshot) ||
            ((AppManagerImplementation::APP_ACTION_LAUNCH != snapshot.getCurrentAction()) &&
             (AppManagerImplementation::APP_ACTION_RESUME != snapshot.getCurrentAction())) ||
            (Exchange::IAppManager::APP_STATE_ACTIVE != snapshot.getAppNewState())) {
            return;
        }

        const std::string initialState = initialLaunchStateName(snapshot.getLaunchInitialState());
        if ((AppManagerImplementation::APP_ACTION_RESUME == snapshot.getCurrentAction()) &&
            (Exchange::IAppManager::APP_STATE_UNLOADED == snapshot.getLaunchInitialState())) {
            return;
        }
        const std::string appVersion = snapshot.getLaunchTelemetryVersion().empty()
            ? snapshot.getPackageInfo().version : snapshot.getLaunchTelemetryVersion();
        const std::string appType = appTypeName(snapshot.getPackageInfo());
        const long long totalLaunchTime = static_cast<long long>(currentTimestampMs()) -
            static_cast<long long>(snapshot.getCurrentActionTime());
        if (initialState.empty() || appVersion.empty() || appType.empty() ||
            (totalLaunchTime < 0) || (totalLaunchTime > INT_MAX)) {
            LOGWARN("AppLaunchComplete telemetry skipped for appId=%s: initialState empty=%d version empty=%d appType empty=%d totalLaunchTime=%lld",
                appId.c_str(), initialState.empty(), appVersion.empty(), appType.empty(), totalLaunchTime);
            return;
        }

        JsonObject jsonParam;
        jsonParam["appId"] = appId;
        jsonParam["appVersion"] = appVersion;
        jsonParam["initialState"] = initialState;
        jsonParam["appType"] = appType;
        jsonParam["totalLaunchTime"] = static_cast<int>(totalLaunchTime);
        publishEventDirect(appId, jsonParam, TELEMETRY_MARKER_LAUNCH_COMPLETE, false);
    }

    void AppManagerTelemetryReporting::reportAppPreloadCompleteTelemetry(const std::string& appId)
    {
        if (!Utils::isTelemetryMetricsEnabled()) {
            return;
        }
        if (appId.empty()) {
            LOGWARN("AppPreloadComplete telemetry skipped: appId is empty");
            return;
        }

        AppInfo snapshot;
        if (!AppInfoManager::getInstance().get(appId, snapshot) ||
            (AppManagerImplementation::APP_ACTION_PRELOAD != snapshot.getCurrentAction()) ||
            (Exchange::IAppManager::APP_STATE_UNLOADED != snapshot.getLaunchInitialState()) ||
            (Exchange::IAppManager::APP_STATE_PAUSED != snapshot.getAppNewState())) {
            return;
        }

        const std::string& appVersion = snapshot.getPackageInfo().version;
        const std::string appType = appTypeName(snapshot.getPackageInfo());
        const long long totalPreloadTime = static_cast<long long>(currentTimestampMs()) -
            static_cast<long long>(snapshot.getCurrentActionTime());
        if (appVersion.empty() || appType.empty() || (totalPreloadTime < 0) || (totalPreloadTime > INT_MAX)) {
            LOGWARN("AppPreloadComplete telemetry skipped for appId=%s: version empty=%d appType empty=%d totalPreloadTime=%lld",
                appId.c_str(), appVersion.empty(), appType.empty(), totalPreloadTime);
            return;
        }

        JsonObject jsonParam;
        jsonParam["appId"] = appId;
        jsonParam["appVersion"] = appVersion;
        jsonParam["appType"] = appType;
        jsonParam["totalPreloadTime"] = static_cast<int>(totalPreloadTime);
        publishEventDirect(appId, jsonParam, TELEMETRY_MARKER_PRELOAD_COMPLETE, false);
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
        publishEventDirect(appId, jsonParam, TELEMETRY_MARKER_APP_CRASHED, false);
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
        publishEventDirect(appId, jsonParam, TELEMETRY_MARKER_CLOSE_ERROR, false);
    }

    void AppManagerTelemetryReporting::reportAppCloseCompleteTelemetry(const std::string& appId,
        AppManagerImplementation::CurrentAction currentAction)
    {
        if (!Utils::isTelemetryMetricsEnabled()) {
            return;
        }

        // CloseApp only moves the app to the inactive state, so it is reported as DEACTIVATE.
        std::string closeType;
        switch (currentAction) {
            case AppManagerImplementation::APP_ACTION_CLOSE:
                closeType = "DEACTIVATE";
                break;
            case AppManagerImplementation::APP_ACTION_TERMINATE:
                closeType = "TERMINATE";
                break;
            case AppManagerImplementation::APP_ACTION_KILL:
                closeType = "KILL";
                break;
            default:
                return;
        }

        AppInfo snapshot;
        if (!AppInfoManager::getInstance().get(appId, snapshot) || (currentAction != snapshot.getCurrentAction())) {
            // The app is gone or another close action took over, so this close did not complete.
            return;
        }
        const std::string appVersion = snapshot.getPackageInfo().version;
        const long long totalCloseTime = static_cast<long long>(currentTimestampMs()) - static_cast<long long>(snapshot.getCurrentActionTime());
        if (appVersion.empty() || (totalCloseTime < 0) || (totalCloseTime > INT_MAX)) {
            LOGWARN("AppCloseComplete telemetry skipped for appId=%s: version empty=%d totalCloseTime=%lld",
                appId.c_str(), appVersion.empty(), totalCloseTime);
            return;
        }

        JsonObject jsonParam;
        jsonParam["appId"] = appId;
        jsonParam["appVersion"] = appVersion;
        jsonParam["totalCloseTime"] = static_cast<int>(totalCloseTime);
        jsonParam["closeType"] = closeType;
        publishEventDirect(appId, jsonParam, TELEMETRY_MARKER_CLOSE_COMPLETE, true);
    }

    void AppManagerTelemetryReporting::publishEventDirect(const std::string& appId, const JsonObject& jsonParam, const char* marker, bool verboseOnly)
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
        const Core::hresult status = telemetry->PublishEvent(telemetryMetrics, marker, verboseOnly);
        telemetry->Release();
        if (Core::ERROR_NONE != status) {
            LOGWARN("%s telemetry publishing failed for appId=%s (status=%d)", marker, appId.c_str(), status);
        }
    }

} /* namespace Plugin */
} /* namespace WPEFramework */


