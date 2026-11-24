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
#include <interfaces/ITelemetryMetrics.h>

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

void PackageManagerTelemetryReporting::publishAppPackageInstallErrorTelemetry(
    const std::string& packageId,
    const std::string& version,
    const std::string& errorTelemetry)
{
    if (errorTelemetry.empty()) {
        LOGERR("AppPackageInstallError telemetry requires an errorTelemetry value");
        return;
    }

    JsonObject jsonParam;
    jsonParam["metricContext"] = packageId + ":" + version;
    jsonParam["errorTelemetry"] = errorTelemetry;

    std::string telemetryMetrics;
    if (!buildTelemetryPayload(jsonParam, telemetryMetrics)) {
        LOGERR("Failed to serialize AppPackageInstallError telemetry for packageId=%s", packageId.c_str());
        return;
    }

    PluginHost::IShell* service = getService();
    if (nullptr == service) {
        LOGWARN("AppPackageInstallError telemetry skipped: PackageManager service is unavailable");
        return;
    }

    Exchange::ITelemetryMetrics* telemetry =
        service->QueryInterfaceByCallsign<Exchange::ITelemetryMetrics>("org.rdk.TelemetryMetrics");
    if (nullptr == telemetry) {
        LOGWARN("AppPackageInstallError telemetry skipped: TelemetryMetrics is unavailable");
        return;
    }

    const Core::hresult status = telemetry->PublishEvent(
        telemetryMetrics, TELEMETRY_MARKER_APP_PACKAGE_INSTALL_ERROR, false);
    telemetry->Release();
    if (Core::ERROR_NONE != status) {
        LOGWARN("AppPackageInstallError telemetry publishing failed for packageId=%s (status=%d)",
            packageId.c_str(), status);
    }
}

void PackageManagerTelemetryReporting::publishAppPackageInstallCompleteTelemetry(
    const std::string& packageId,
    const std::string& version,
    uint64_t packageSize,
    uint64_t installTime)
{
    if (packageId.empty() || version.empty()) {
        LOGWARN("AppPackageInstallComplete telemetry skipped: packageId or version is empty");
        return;
    }

    JsonObject jsonParam;
    jsonParam["metricContext"] = packageId + ":" + version;
    jsonParam["packageSize"] = packageSize;
    jsonParam["appId"] = packageId;
    jsonParam["version"] = version;
    jsonParam["installTime"] = installTime;

    std::string telemetryMetrics;
    if (!buildTelemetryPayload(jsonParam, telemetryMetrics)) {
        LOGERR("Failed to serialize AppPackageInstallComplete telemetry for packageId=%s", packageId.c_str());
        return;
    }

    PluginHost::IShell* service = getService();
    if (nullptr == service) {
        LOGWARN("AppPackageInstallComplete telemetry skipped: PackageManager service is unavailable");
        return;
    }

    Exchange::ITelemetryMetrics* telemetry =
        service->QueryInterfaceByCallsign<Exchange::ITelemetryMetrics>("org.rdk.TelemetryMetrics");
    if (nullptr == telemetry) {
        LOGWARN("AppPackageInstallComplete telemetry skipped: TelemetryMetrics is unavailable");
        return;
    }

    const Core::hresult status = telemetry->PublishEvent(
        telemetryMetrics, TELEMETRY_MARKER_APP_PACKAGE_INSTALL_COMPLETE, true);
    telemetry->Release();
    if (Core::ERROR_NONE != status) {
        LOGWARN("AppPackageInstallComplete telemetry publishing failed for packageId=%s (status=%d)",
            packageId.c_str(), status);
    }
}

void PackageManagerTelemetryReporting::recordAndPublishTelemetryData(const std::string& marker,
    const std::string& appId,
    uint64_t requestTime,
    const std::string& runtimeId,
    const std::string& runtimeVersion,
    int count,
    const std::string& backend)
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
    } else if (marker == TELEMETRY_MARKER_PACKAGE_CACHE_INIT_TIME) {
        jsonParam["packageManagerCacheInitTime"] = duration;
        if (0 <= count) {
            jsonParam["count"] = count;
        }
        if (false == backend.empty()) {
            jsonParam["backend"] = backend;
        }
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
