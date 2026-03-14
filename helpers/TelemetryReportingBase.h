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
#include "TelemetryMarkers.h"
#include "UtilsTelemetryMetrics.h"

namespace WPEFramework {
namespace Plugin {
namespace Utils {

class TelemetryReportingBase {
public:
    TelemetryReportingBase(const TelemetryReportingBase&) = delete;
    TelemetryReportingBase& operator=(const TelemetryReportingBase&) = delete;
    time_t getCurrentTimestampMs() const;

protected:
    TelemetryReportingBase();
    virtual ~TelemetryReportingBase();

    void setService(PluginHost::IShell* service);
    PluginHost::IShell* getService() const;

    Core::hresult initializeTelemetryClient();
    bool ensureTelemetryClient();
    bool isTelemetryClientAvailable() const;
    void resetTelemetryClient();

    time_t currentTimestampMs() const;
    int durationSinceMs(uint64_t requestTimeMs) const;
    bool buildTelemetryPayload(const JsonObject& jsonParam, std::string& telemetryMetrics) const;
    Core::hresult recordTelemetry(const std::string& appId, const std::string& telemetryMetrics, const std::string& marker);
    Core::hresult recordTelemetry(const std::string& appId, const JsonObject& jsonParam, const std::string& marker);
    Core::hresult publishTelemetry(const std::string& appId, const std::string& marker);
    Core::hresult recordAndPublishTelemetry(const std::string& appId, const JsonObject& jsonParam, const std::string& marker, bool publish);
    TelemetryMetricsClient& getTelemetryClient();

private:
    PluginHost::IShell* mCurrentservice;
    TelemetryMetricsClient mTelemetryMetrics;
};

} // namespace Utils
} // namespace Plugin
} // namespace WPEFramework
