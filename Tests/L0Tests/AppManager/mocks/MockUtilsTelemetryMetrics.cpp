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

/**
 * @file MockUtilsTelemetryMetrics.cpp
 *
 * Test-only mock replacing helpers/Telemetry/UtilsTelemetryMetrics.cpp used
 * exclusively by the AppManager L0 test binary (appmanager_l0test).
 *
 * Key difference from the production file:
 *   isTelemetryMetricsEnabled() returns TRUE so that all telemetry reporting
 *   code paths in AppManagerTelemetryReporting.cpp are exercised under coverage.
 *
 * All TelemetryMetricsClient methods remain no-ops (record/publish are
 * discarded) so existing tests are unaffected and no real telemetry
 * infrastructure is required.
 */

#include "UtilsTelemetryMetrics.h"

namespace WPEFramework {
namespace Plugin {
namespace Utils {

// Override: return true so telemetry function bodies are entered during tests.
bool isTelemetryMetricsEnabled()
{
    return true;
}

TelemetryMetricsClient::~TelemetryMetricsClient()
{
    // No shared state to clean up in test stub.
}

Core::hresult TelemetryMetricsClient::initialize(PluginHost::IShell* service,
    const std::string& callsign)
{
    (void)service;
    (void)callsign;
    return Core::ERROR_NONE;
}

Core::hresult TelemetryMetricsClient::ensure(PluginHost::IShell* service,
    const std::string& callsign)
{
    return initialize(service, callsign);
}

void TelemetryMetricsClient::reset()
{
    // No-op: no shared global state in test stub.
}

bool TelemetryMetricsClient::isAvailable() const
{
    // Always available in test stub so ensureTelemetryClient() returns true.
    return true;
}

Core::hresult TelemetryMetricsClient::record(const std::string& appId,
    const std::string& telemetryPayload,
    const std::string& marker)
{
    (void)appId;
    (void)telemetryPayload;
    (void)marker;
    return Core::ERROR_NONE;
}

Core::hresult TelemetryMetricsClient::publish(const std::string& appId,
    const std::string& marker)
{
    (void)appId;
    (void)marker;
    return Core::ERROR_NONE;
}

} // namespace Utils
} // namespace Plugin
} // namespace WPEFramework
