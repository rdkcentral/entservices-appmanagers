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

#include <limits>

namespace WPEFramework {
namespace Plugin {
namespace Utils {

TelemetryReportingBase::TelemetryReportingBase()
    : mCurrentservice(nullptr)
{
}

TelemetryReportingBase::~TelemetryReportingBase()
{
    resetTelemetryClient();
}

time_t TelemetryReportingBase::getCurrentTimestampMs() const
{
    return currentTimestampMs();
}

void TelemetryReportingBase::setService(PluginHost::IShell* service)
{
    mCurrentservice = service;
}

PluginHost::IShell* TelemetryReportingBase::getService() const
{
    return mCurrentservice;
}

Core::hresult TelemetryReportingBase::initializeTelemetryClient()
{
    if (nullptr == mCurrentservice) {
        return Core::ERROR_GENERAL;
    }
    return mTelemetryMetrics.initialize(mCurrentservice);
}

bool TelemetryReportingBase::ensureTelemetryClient()
{
    if (!mTelemetryMetrics.isAvailable()) {
        initializeTelemetryClient();
    }
    return mTelemetryMetrics.isAvailable();
}

bool TelemetryReportingBase::isTelemetryClientAvailable() const
{
    return mTelemetryMetrics.isAvailable();
}

void TelemetryReportingBase::resetTelemetryClient()
{
    mTelemetryMetrics.reset();
    mCurrentservice = nullptr;
}

time_t TelemetryReportingBase::currentTimestampMs() const
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (((time_t)ts.tv_sec * 1000) + ((time_t)ts.tv_nsec / 1000000));
}

int TelemetryReportingBase::durationSinceMs(uint64_t requestTimeMs) const
{
    int64_t diff = static_cast<int64_t>(currentTimestampMs()) - static_cast<int64_t>(requestTimeMs);
    if (diff > static_cast<int64_t>(std::numeric_limits<int>::max())) {
        return std::numeric_limits<int>::max();
    }
    if (diff < static_cast<int64_t>(std::numeric_limits<int>::min())) {
        return std::numeric_limits<int>::min();
    }
    return static_cast<int>(diff);
}

bool TelemetryReportingBase::buildTelemetryPayload(const JsonObject& jsonParam, std::string& telemetryMetrics) const
{
    return jsonParam.ToString(telemetryMetrics);
}

Core::hresult TelemetryReportingBase::recordTelemetry(const std::string& appId, const std::string& telemetryMetrics, const std::string& marker)
{
    if (!ensureTelemetryClient()) {
        return Core::ERROR_GENERAL;
    }
    return getTelemetryClient().record(appId, telemetryMetrics, marker);
}

Core::hresult TelemetryReportingBase::recordTelemetry(const std::string& appId, const JsonObject& jsonParam, const std::string& marker)
{
    std::string telemetryMetrics;
    if (!buildTelemetryPayload(jsonParam, telemetryMetrics)) {
        return Core::ERROR_GENERAL;
    }
    return recordTelemetry(appId, telemetryMetrics, marker);
}

Core::hresult TelemetryReportingBase::publishTelemetry(const std::string& appId, const std::string& marker)
{
    if (!ensureTelemetryClient()) {
        return Core::ERROR_GENERAL;
    }
    return getTelemetryClient().publish(appId, marker);
}

Core::hresult TelemetryReportingBase::recordAndPublishTelemetry(const std::string& appId, const JsonObject& jsonParam, const std::string& marker, bool publish)
{
    Core::hresult status = recordTelemetry(appId, jsonParam, marker);
    if ((Core::ERROR_NONE == status) && publish) {
        status = publishTelemetry(appId, marker);
    }
    return status;
}

TelemetryMetricsClient& TelemetryReportingBase::getTelemetryClient()
{
    return mTelemetryMetrics;
}

} // namespace Utils
} // namespace Plugin
} // namespace WPEFramework
