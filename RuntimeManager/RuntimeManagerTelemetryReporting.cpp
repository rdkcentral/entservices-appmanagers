/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2026 RDK Management
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

#include "RuntimeManagerTelemetryReporting.h"
#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

RuntimeManagerTelemetryReporting::RuntimeManagerTelemetryReporting()
{
}

RuntimeManagerTelemetryReporting::~RuntimeManagerTelemetryReporting()
{
}

RuntimeManagerTelemetryReporting& RuntimeManagerTelemetryReporting::getInstance()
{
    static RuntimeManagerTelemetryReporting instance;
    return instance;
}

void RuntimeManagerTelemetryReporting::initialize(PluginHost::IShell* service)
{
    setService(service);
    if (Core::ERROR_NONE != initializeTelemetryClient()) {
        LOGERR("TelemetryMetrics client init failed");
    }
}

void RuntimeManagerTelemetryReporting::reset()
{
    resetTelemetryClient();
}

void RuntimeManagerTelemetryReporting::recordTelemetryData(const std::string& marker, const std::string& appId, uint64_t requestTime)
{
    time_t currentTime = currentTimestampMs();
    LOGINFO("End time for %s: %lu", marker.c_str(), currentTime);

    JsonObject jsonParam;
    int duration = durationSinceMs(requestTime);

    if (marker == TELEMETRY_MARKER_RESUME_TIME) {
        jsonParam["runtimeManagerResumeTime"] = duration;
    } else if (marker == TELEMETRY_MARKER_SUSPEND_TIME) {
        jsonParam["runtimeManagerSuspendTime"] = duration;
    } else if (marker == TELEMETRY_MARKER_HIBERNATE_TIME) {
        jsonParam["runtimeManagerHibernateTime"] = duration;
    } else if (marker == TELEMETRY_MARKER_WAKE_TIME) {
        jsonParam["runtimeManagerWakeTime"] = duration;
    } else if (marker == TELEMETRY_MARKER_LAUNCH_TIME) {
        jsonParam["runtimeManagerRunTime"] = duration;
    } else if (marker == TELEMETRY_MARKER_CLOSE_TIME) {
        jsonParam["runtimeManagerTerminateTime"] = duration;
    } else {
        LOGERR("Unknown telemetry marker: %s", marker.c_str());
        return;
    }

    jsonParam["appId"] = appId;

    LOGINFO("Record appId %s marker %s start time %d", appId.c_str(), marker.c_str(), duration);
    if (Core::ERROR_NONE != recordTelemetry(appId, jsonParam, marker)) {
        LOGERR("Failed to record telemetry for appId %s marker %s", appId.c_str(), marker.c_str());
    }
}

} // namespace Plugin
} // namespace WPEFramework
