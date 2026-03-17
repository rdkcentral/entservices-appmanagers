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

#include "RDKWindowManagerTelemetryReporting.h"
#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

RDKWindowManagerTelemetryReporting::RDKWindowManagerTelemetryReporting()
{
}

RDKWindowManagerTelemetryReporting::~RDKWindowManagerTelemetryReporting()
{
}

RDKWindowManagerTelemetryReporting& RDKWindowManagerTelemetryReporting::getInstance()
{
    static RDKWindowManagerTelemetryReporting instance;
    return instance;
}

void RDKWindowManagerTelemetryReporting::initialize(PluginHost::IShell* service)
{
    setService(service);
    if (Core::ERROR_NONE != initializeTelemetryClient()) {
        LOGERR("TelemetryMetrics client initialization failed");
    }
}

void RDKWindowManagerTelemetryReporting::reset()
{
    resetTelemetryClient();
}

int RDKWindowManagerTelemetryReporting::durationSinceMs(uint64_t requestTimeMs)
{
    return Utils::TelemetryReportingBase::durationSinceMs(requestTimeMs);
}

void RDKWindowManagerTelemetryReporting::recordDisplayTelemetry(const std::string& client, int duration, bool isCreate)
{
    if (!Utils::isTelemetryMetricsEnabled()) {
        return;
    }

    if (!ensureTelemetryClient()) {
        LOGWARN("TelemetryMetrics client unavailable; display telemetry not recorded");
        return;
    }

    JsonObject jsonParam;
    std::string markerName;

    if (true == isCreate)
    {
        jsonParam["windowManagerCreateDisplayTime"] = duration;
        markerName = TELEMETRY_MARKER_LAUNCH_TIME;
        LOGINFO("RDKWindowManager: Recording createDisplay telemetry: client=%s time=%dms", client.c_str(), duration);
    }
    else
    {
        jsonParam["windowManagerDestroyTime"] = duration;
        markerName = TELEMETRY_MARKER_CLOSE_TIME;
        LOGINFO("RDKWindowManager: Recording destroyDisplay telemetry: client=%s time=%dms", client.c_str(), duration);
    }

    if (Core::ERROR_NONE != recordTelemetry(client, jsonParam, markerName)) {
        LOGERR("RDKWindowManager: Failed to record display telemetry for client=%s", client.c_str());
    }
}

} // namespace Plugin
} // namespace WPEFramework
