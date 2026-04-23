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
// Spec: download-manager

#include "DownloadManagerTelemetryReporting.h"
#include "TelemetryMarkers.h"
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

void DownloadManagerTelemetryReporting::recordDownloadTimeTelemetry(const std::string& downloadId, int64_t downloadTimeMs)
{
    if (!Utils::isTelemetryMetricsEnabled()) {
        return;
    }

    JsonObject jsonParam;
    jsonParam["downloadTime"] = static_cast<int>(downloadTimeMs);

    LOGINFO("Record downloadId %s downloadTime %lld ms", downloadId.c_str(), static_cast<long long>(downloadTimeMs));
    if (Core::ERROR_NONE != recordAndPublishTelemetry(downloadId, jsonParam, TELEMETRY_MARKER_DOWNLOAD_TIME, true)) {
        LOGERR("Failed to record download time telemetry for downloadId %s", downloadId.c_str());
    }
}

void DownloadManagerTelemetryReporting::recordDownloadErrorTelemetry(const std::string& downloadId, int errorCode)
{
    if (!Utils::isTelemetryMetricsEnabled()) {
        return;
    }

    JsonObject jsonParam;
    jsonParam["errorCode"] = errorCode;

    LOGINFO("Record downloadId %s download error errorCode %d", downloadId.c_str(), errorCode);
    if (Core::ERROR_NONE != recordAndPublishTelemetry(downloadId, jsonParam, TELEMETRY_MARKER_DOWNLOAD_ERROR, true)) {
        LOGERR("Failed to record download error telemetry for downloadId %s", downloadId.c_str());
    }
}

} // namespace Plugin
} // namespace WPEFramework
