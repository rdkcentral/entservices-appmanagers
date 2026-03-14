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

#include "TelemetryReportingBase.h"

namespace WPEFramework {
namespace Plugin {
namespace Utils {

TelemetryReportingBase::TelemetryReportingBase()
    : mCurrentservice(nullptr)
{
}

TelemetryReportingBase::~TelemetryReportingBase()
{
    ResetTelemetryClient();
}

time_t TelemetryReportingBase::TimestampMs() const
{
    return CurrentTimestampMs();
}

void TelemetryReportingBase::SetService(PluginHost::IShell* service)
{
    mCurrentservice = service;
}

PluginHost::IShell* TelemetryReportingBase::GetService() const
{
    return mCurrentservice;
}

Core::hresult TelemetryReportingBase::InitializeTelemetryClient()
{
    if (nullptr == mCurrentservice) {
        return Core::ERROR_GENERAL;
    }
    return mTelemetryMetrics.Initialize(mCurrentservice);
}

bool TelemetryReportingBase::EnsureTelemetryClient()
{
    if (!mTelemetryMetrics.IsAvailable()) {
        InitializeTelemetryClient();
    }
    return mTelemetryMetrics.IsAvailable();
}

bool TelemetryReportingBase::IsTelemetryClientAvailable() const
{
    return mTelemetryMetrics.IsAvailable();
}

void TelemetryReportingBase::ResetTelemetryClient()
{
    mTelemetryMetrics.Reset();
    mCurrentservice = nullptr;
}

time_t TelemetryReportingBase::CurrentTimestampMs() const
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (((time_t)ts.tv_sec * 1000) + ((time_t)ts.tv_nsec / 1000000));
}

int TelemetryReportingBase::DurationSinceMs(uint64_t requestTimeMs) const
{
    return static_cast<int>(CurrentTimestampMs() - requestTimeMs);
}

bool TelemetryReportingBase::BuildTelemetryPayload(const JsonObject& jsonParam, std::string& telemetryMetrics) const
{
    return jsonParam.ToString(telemetryMetrics);
}

Core::hresult TelemetryReportingBase::RecordTelemetry(const std::string& appId, const std::string& telemetryMetrics, const std::string& marker)
{
    if (!EnsureTelemetryClient()) {
        return Core::ERROR_GENERAL;
    }
    return TelemetryClient().Record(appId, telemetryMetrics, marker);
}

Core::hresult TelemetryReportingBase::RecordTelemetry(const std::string& appId, const JsonObject& jsonParam, const std::string& marker)
{
    std::string telemetryMetrics;
    if (!BuildTelemetryPayload(jsonParam, telemetryMetrics)) {
        return Core::ERROR_GENERAL;
    }
    return RecordTelemetry(appId, telemetryMetrics, marker);
}

Core::hresult TelemetryReportingBase::PublishTelemetry(const std::string& appId, const std::string& marker)
{
    if (!EnsureTelemetryClient()) {
        return Core::ERROR_GENERAL;
    }
    return TelemetryClient().Publish(appId, marker);
}

Core::hresult TelemetryReportingBase::RecordAndPublishTelemetry(const std::string& appId, const JsonObject& jsonParam, const std::string& marker, bool publish)
{
    Core::hresult status = RecordTelemetry(appId, jsonParam, marker);
    if ((Core::ERROR_NONE == status) && publish) {
        status = PublishTelemetry(appId, marker);
    }
    return status;
}

TelemetryMetricsClient& TelemetryReportingBase::TelemetryClient()
{
    return mTelemetryMetrics;
}

} // namespace Utils
} // namespace Plugin
} // namespace WPEFramework
