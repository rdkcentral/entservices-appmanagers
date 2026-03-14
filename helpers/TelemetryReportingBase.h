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
#include "UtilsTelemetryMetrics.h"

namespace WPEFramework {
namespace Plugin {
namespace Utils {

class TelemetryReportingBase {
public:
    TelemetryReportingBase(const TelemetryReportingBase&) = delete;
    TelemetryReportingBase& operator=(const TelemetryReportingBase&) = delete;
    time_t TimestampMs() const;

protected:
    TelemetryReportingBase();
    virtual ~TelemetryReportingBase();

    void SetService(PluginHost::IShell* service);
    PluginHost::IShell* GetService() const;

    Core::hresult InitializeTelemetryClient();
    bool EnsureTelemetryClient();
    bool IsTelemetryClientAvailable() const;
    void ResetTelemetryClient();

    time_t CurrentTimestampMs() const;
    int DurationSinceMs(uint64_t requestTimeMs) const;
    bool BuildTelemetryPayload(const JsonObject& jsonParam, std::string& telemetryMetrics) const;
    Core::hresult RecordTelemetry(const std::string& appId, const std::string& telemetryMetrics, const std::string& marker);
    Core::hresult RecordTelemetry(const std::string& appId, const JsonObject& jsonParam, const std::string& marker);
    Core::hresult PublishTelemetry(const std::string& appId, const std::string& marker);
    Core::hresult RecordAndPublishTelemetry(const std::string& appId, const JsonObject& jsonParam, const std::string& marker, bool publish);
    TelemetryMetricsClient& TelemetryClient();

private:
    PluginHost::IShell* mCurrentservice;
    TelemetryMetricsClient mTelemetryMetrics;
};

} // namespace Utils
} // namespace Plugin
} // namespace WPEFramework
