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

#include "UtilsTelemetryMetrics.h"

#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
#include <interfaces/ITelemetryMetrics.h>
#include <mutex>
#endif

namespace WPEFramework {
namespace Plugin {
namespace Utils {

#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
namespace {
    Exchange::ITelemetryMetrics* gTelemetryMetricsObject = nullptr;
    std::mutex gTelemetryLock;
}
#endif

TelemetryMetricsClient::~TelemetryMetricsClient()
{
    // Shared telemetry interface is process-wide and intentionally retained.
}

Core::hresult TelemetryMetricsClient::initialize(PluginHost::IShell* service,
    const std::string& callsign)
{
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    std::lock_guard<std::mutex> lock(gTelemetryLock);

    if (gTelemetryMetricsObject != nullptr)
    {
        return Core::ERROR_NONE;
    }

    if (service == nullptr)
    {
        return Core::ERROR_GENERAL;
    }

    gTelemetryMetricsObject = service->QueryInterfaceByCallsign<WPEFramework::Exchange::ITelemetryMetrics>(callsign.c_str());

    return (gTelemetryMetricsObject != nullptr) ? Core::ERROR_NONE : Core::ERROR_GENERAL;
#else
    (void)service;
    (void)callsign;
    return Core::ERROR_NONE;
#endif
}

Core::hresult TelemetryMetricsClient::ensure(PluginHost::IShell* service,
    const std::string& callsign)
{
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    if (gTelemetryMetricsObject != nullptr)
    {
        return Core::ERROR_NONE;
    }
#endif
    return initialize(service, callsign);
}

void TelemetryMetricsClient::reset()
{
    // No-op for shared instance to avoid cross-plugin invalidation.
}

bool TelemetryMetricsClient::isAvailable() const
{
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    return (gTelemetryMetricsObject != nullptr);
#else
    return true;
#endif
}

Core::hresult TelemetryMetricsClient::record(const std::string& appId,
    const std::string& telemetryPayload,
    const std::string& marker)
{
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    return recordTelemetry(gTelemetryMetricsObject, appId, telemetryPayload, marker);
#else
    (void)appId;
    (void)telemetryPayload;
    (void)marker;
    return Core::ERROR_NONE;
#endif
}

Core::hresult TelemetryMetricsClient::publish(const std::string& appId,
    const std::string& marker)
{
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    return publishTelemetry(gTelemetryMetricsObject, appId, marker);
#else
    (void)appId;
    (void)marker;
    return Core::ERROR_NONE;
#endif
}

} // namespace Utils
} // namespace Plugin
} // namespace WPEFramework
