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

#pragma once

#include "Module.h"
#include "TelemetryReportingBase.h"

namespace WPEFramework {
namespace Plugin {

class DownloadManagerTelemetryReporting : public Utils::TelemetryReportingBase {
public:
    DownloadManagerTelemetryReporting(const DownloadManagerTelemetryReporting&) = delete;
    DownloadManagerTelemetryReporting& operator=(const DownloadManagerTelemetryReporting&) = delete;

    static DownloadManagerTelemetryReporting& getInstance();

    void initialize(PluginHost::IShell* service);
    void reset();
    void recordDownloadTimeTelemetry(const std::string& downloadId, int64_t downloadTimeMs);
    void recordDownloadErrorTelemetry(const std::string& downloadId, int errorCode);
    void publishAppPackageDownloadErrorTelemetry(const std::string& metricContext,
        const std::string& errorTelemetry);
    void publishAppPackageDownloadCompleteTelemetry(const std::string& metricContext,
        uint64_t packageSize,
        bool priority,
        uint64_t rateLimit,
        uint32_t totalRetries,
        uint64_t timeInQueue,
        uint64_t totalDownloadTime);

private:
    void publishEvent(const char* markerName, const std::string& telemetryMetrics,
        bool verboseOnly, const std::string& metricContext);
    DownloadManagerTelemetryReporting();
    ~DownloadManagerTelemetryReporting();
};

} // namespace Plugin
} // namespace WPEFramework
