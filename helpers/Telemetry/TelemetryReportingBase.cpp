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
#include <chrono>
#include <array>
#include <map>
#include <mutex>

#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {
namespace Utils {

namespace {
    static const string BOOTSTRAP_AGGREGATE_ID = "AppManagersBootstrapMarker"; //used for aggregating all plugin init times only , not used for publishing anywhere
    static const char* BOOTSTRAP_TIMEOUT_VALUE = "TIMEOUT";
    static const std::chrono::seconds BOOTSTRAP_PUBLISH_TIMEOUT(120);
    static const std::array<const char*, 7> EXPECTED_BOOTSTRAP_FIELDS = {{
        "appManagerBootstrapTime",
        "appStorageManagerBootstrapTime",
        "lifecycleManagerBootstrapTime",
        "runtimeManagerBootstrapTime",
        "packageManagerBootstrapTime",
        "windowManagerBootstrapTime",
        "downloadManagerBootstrapTime"
    }};
    static std::map<std::string, int> gBootstrapDurations;
    static bool gBootstrapAggregationStarted = false;
    static bool gBootstrapPublishCompleted = false;
    static std::chrono::steady_clock::time_point gBootstrapStartTime;
    static std::mutex gPendingBootstrapLock;

    static bool HasAllExpectedBootstrapFields(const std::map<std::string, int>& durations)
    {
        for (const char* fieldName : EXPECTED_BOOTSTRAP_FIELDS) {
            if (durations.find(fieldName) == durations.end()) {
                return false;
            }
        }
        return true;
    }
}

TelemetryReportingBase::ScopedBootstrapTimer::~ScopedBootstrapTimer()
{
    if ((nullptr == mTelemetry) || (false == isTelemetryMetricsEnabled())) {
        return;
    }

    const auto endTime = std::chrono::steady_clock::now();
    const auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - mStartTime).count();
    const Core::hresult status = mTelemetry->recordBootstrapTelemetry(mService,
        static_cast<int>(durationMs),
        mFieldName);
    if (Core::ERROR_NONE != status) {
        LOGWARN("Failed to record aggregated bootstrap telemetry");
    }
}

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
    return static_cast<time_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count());
}

int TelemetryReportingBase::durationSinceMs(uint64_t requestTimeMs) const
{
    return static_cast<int>(currentTimestampMs() - requestTimeMs);
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

Core::hresult TelemetryReportingBase::recordBootstrapTelemetry(PluginHost::IShell* service,
    const int durationMs,
    const std::string& fieldName)
{
    if ((nullptr == service) || fieldName.empty()) {
        return Core::ERROR_GENERAL;
    }

    setService(service);
    std::map<std::string, int> recordedDurations;
    bool shouldPublish = false;
    bool timeoutReached = false;
    {
        std::lock_guard<std::mutex> lock(gPendingBootstrapLock);
        if (true == gBootstrapPublishCompleted) {
            return Core::ERROR_NONE;
        }

        if (false == gBootstrapAggregationStarted) {
            gBootstrapAggregationStarted = true;
            gBootstrapStartTime = std::chrono::steady_clock::now();
        }

        gBootstrapDurations[fieldName] = durationMs;
        recordedDurations = gBootstrapDurations;

        shouldPublish = HasAllExpectedBootstrapFields(recordedDurations);
        timeoutReached = (std::chrono::steady_clock::now() - gBootstrapStartTime) >= BOOTSTRAP_PUBLISH_TIMEOUT;

        if ((false == shouldPublish) && (false == timeoutReached)) {
            LOGWARN("Bootstrap metric field '%s' recorded; waiting for remaining managers", fieldName.c_str());
            return Core::ERROR_NONE;
        }
    }

    if (!ensureTelemetryClient()) {
        LOGWARN("Telemetry unavailable, cached bootstrap metric field '%s'", fieldName.c_str());
        return Core::ERROR_NONE;
    }

    JsonObject jsonParam;
    for (const auto& bootstrapEntry : recordedDurations) {
        jsonParam[bootstrapEntry.first.c_str()] = bootstrapEntry.second;
    }

    if ((false == shouldPublish) && (true == timeoutReached)) {
        for (const char* expectedField : EXPECTED_BOOTSTRAP_FIELDS) {
            if (recordedDurations.find(expectedField) == recordedDurations.end()) {
                jsonParam[expectedField] = BOOTSTRAP_TIMEOUT_VALUE;
            }
        }
        shouldPublish = true;
        LOGWARN("Bootstrap publish timeout reached; missing manager fields marked as TIMEOUT");
    }

    Core::hresult status = recordTelemetry(BOOTSTRAP_AGGREGATE_ID, jsonParam, TELEMETRY_MARKER_BOOTSTRAP_TIME);
    if ((Core::ERROR_NONE == status) && shouldPublish) {
        status = publishTelemetry(BOOTSTRAP_AGGREGATE_ID, TELEMETRY_MARKER_BOOTSTRAP_TIME);
        if (Core::ERROR_NONE == status) {
            std::lock_guard<std::mutex> lock(gPendingBootstrapLock);
            gBootstrapPublishCompleted = true;
        }
    }

    return status;
}

} // namespace Utils
} // namespace Plugin
} // namespace WPEFramework
