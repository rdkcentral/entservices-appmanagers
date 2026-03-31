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

#include "PackageManagerTelemetryReporting.h"
#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

PackageManagerTelemetryReporting::PackageManagerTelemetryReporting()
{
}

PackageManagerTelemetryReporting::~PackageManagerTelemetryReporting()
{
}

PackageManagerTelemetryReporting& PackageManagerTelemetryReporting::getInstance()
{
    static PackageManagerTelemetryReporting instance;
    return instance;
}

void PackageManagerTelemetryReporting::initialize(PluginHost::IShell* service)
{
    setService(service);
    if (Core::ERROR_NONE != initializeTelemetryClient()) {
        LOGERR("TelemetryMetrics client initialization failed");
    }
}

void PackageManagerTelemetryReporting::reset()
{
    resetTelemetryClient();
}

void PackageManagerTelemetryReporting::recordAndPublishTelemetryData(const std::string& marker,
    const std::string& appId,
    uint64_t requestTime,
    int errorCode,
    const std::string& runtimeId,
    const std::string& runtimeVersion)
{
    if (!Utils::isTelemetryMetricsEnabled()) {
        return;
    }

    JsonObject jsonParam;
    int duration = 0;
    bool shouldProcessMarker = true;
    bool publish = true;

    if (marker.empty()) {
        LOGERR("Telemetry marker is empty");
        return;
    }

    if (!ensureTelemetryClient()) {
        LOGWARN("TelemetryMetrics client unavailable; telemetry marker '%s' not recorded", marker.c_str());
        return;
    }

    time_t currentTime = currentTimestampMs();
    duration = durationSinceMs(requestTime);
    LOGINFO("End time for %s: %llu", marker.c_str(), static_cast<unsigned long long>(currentTime));

    if (marker == TELEMETRY_MARKER_LAUNCH_TIME) {
        jsonParam["packageManagerLockTime"] = duration;
        jsonParam["runtimeId"] = runtimeId;
        jsonParam["runtimeVersion"] = runtimeVersion;
        publish = false;
    } else if (marker == TELEMETRY_MARKER_CLOSE_TIME) {
        jsonParam["packageManagerUnlockTime"] = duration;
        publish = false;
    } else if (marker == TELEMETRY_MARKER_INSTALL_TIME) {
        jsonParam["installTime"] = duration;
    } else if (marker == TELEMETRY_MARKER_UNINSTALL_TIME) {
        jsonParam["uninstallTime"] = duration;
    } else if ((marker == TELEMETRY_MARKER_INSTALL_ERROR) || (marker == TELEMETRY_MARKER_UNINSTALL_ERROR)) {
        jsonParam["errorCode"] = errorCode;
    } else {
        LOGERR("Unknown telemetry marker: %s", marker.c_str());
        shouldProcessMarker = false;
    }

    if (true == shouldProcessMarker) {
        jsonParam["appId"] = appId;

        LOGINFO("Record appId %s marker %s duration %d", appId.c_str(), marker.c_str(), duration);
        if (Core::ERROR_NONE != recordAndPublishTelemetry(appId, jsonParam, marker, publish)) {
            LOGERR("Telemetry Record/Publish Failed");
        }
    }
}

} // namespace Plugin
} // namespace WPEFramework
