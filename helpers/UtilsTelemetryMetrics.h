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

#include <string>
#include <core/core.h>
#include <plugins/plugins.h>

namespace WPEFramework {
namespace Plugin {
namespace Utils {

template<typename TelemetryMetricsObjectType>
inline Core::hresult RecordTelemetry(TelemetryMetricsObjectType* telemetryMetricsObject,
    const std::string& appId,
    const std::string& telemetryPayload,
    const std::string& marker)
{
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    if (telemetryMetricsObject != nullptr)
    {
        return telemetryMetricsObject->Record(appId, telemetryPayload, marker);
    }
#else
    (void)telemetryMetricsObject;
    (void)appId;
    (void)telemetryPayload;
    (void)marker;
#endif
    return Core::ERROR_NONE;
}

template<typename TelemetryMetricsObjectType>
inline Core::hresult PublishTelemetry(TelemetryMetricsObjectType* telemetryMetricsObject,
    const std::string& appId,
    const std::string& marker)
{
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    if (telemetryMetricsObject != nullptr)
    {
        return telemetryMetricsObject->Publish(appId, marker);
    }
#else
    (void)telemetryMetricsObject;
    (void)appId;
    (void)marker;
#endif
    return Core::ERROR_NONE;
}

class TelemetryMetricsClient {
public:
    TelemetryMetricsClient() = default;
    ~TelemetryMetricsClient();

    Core::hresult Initialize(PluginHost::IShell* service,
        const std::string& callsign = "org.rdk.TelemetryMetrics");

    Core::hresult Ensure(PluginHost::IShell* service,
        const std::string& callsign = "org.rdk.TelemetryMetrics");

    void Reset();

    bool IsAvailable() const;

    Core::hresult Record(const std::string& appId,
        const std::string& telemetryPayload,
        const std::string& marker);

    Core::hresult Publish(const std::string& appId,
        const std::string& marker);
};

} // namespace Utils
} // namespace Plugin
} // namespace WPEFramework
