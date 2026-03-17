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

#include "DownloadManagerTelemetryReporting.h"
#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

DownloadManagerTelemetryReporting::DownloadManagerTelemetryReporting()
{
}

DownloadManagerTelemetryReporting::~DownloadManagerTelemetryReporting()
{
}

DownloadManagerTelemetryReporting& DownloadManagerTelemetryReporting::getInstance()
{
    static DownloadManagerTelemetryReporting instance;
    return instance;
}

void DownloadManagerTelemetryReporting::initialize(PluginHost::IShell* service)
{
    setService(service);
    if (Core::ERROR_NONE != initializeTelemetryClient()) {
        LOGERR("TelemetryMetrics client initialization failed");
    }
}

void DownloadManagerTelemetryReporting::reset()
{
    resetTelemetryClient();
}

void DownloadManagerTelemetryReporting::recordDownloadTelemetry(const std::string& downloadId, int64_t downloadTime, bool success, int errorCode)
{
    if (!Utils::isTelemetryMetricsEnabled()) {
        return;
    }

    if (!ensureTelemetryClient()) {
        LOGWARN("TelemetryMetrics client unavailable; download telemetry not recorded");
        return;
    }

    JsonObject jsonParam;
    std::string markerName;

    if (true == success) {
        jsonParam["downloadTime"] = static_cast<int>(downloadTime);
        markerName = TELEMETRY_MARKER_DOWNLOAD_TIME;
        LOGINFO("DM: Recording download success telemetry: time=%lldms", downloadTime);
    } else {
        jsonParam["errorCode"] = errorCode;
        markerName = TELEMETRY_MARKER_DOWNLOAD_ERROR;
        LOGINFO("DM: Recording download error telemetry: errorCode=%d", errorCode);
    }

    if (Core::ERROR_NONE != recordAndPublishTelemetry(downloadId, jsonParam, markerName, true)) {
        LOGERR("DM: Failed to record/publish download telemetry for id=%s", downloadId.c_str());
    }
}

} // namespace Plugin
} // namespace WPEFramework
