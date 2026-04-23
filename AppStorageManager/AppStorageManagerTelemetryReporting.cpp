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
// Spec: app-storage-manager

#include "AppStorageManagerTelemetryReporting.h"
#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

AppStorageManagerTelemetryReporting::AppStorageManagerTelemetryReporting()
{
}

AppStorageManagerTelemetryReporting::~AppStorageManagerTelemetryReporting()
{
}

AppStorageManagerTelemetryReporting& AppStorageManagerTelemetryReporting::getInstance()
{
    static AppStorageManagerTelemetryReporting instance;
    return instance;
}

void AppStorageManagerTelemetryReporting::initialize(PluginHost::IShell* service)
{
    setService(service);
    if (Core::ERROR_NONE != initializeTelemetryClient()) {
        LOGERR("TelemetryMetrics client initialization failed");
    }
}

void AppStorageManagerTelemetryReporting::reset()
{
    resetTelemetryClient();
}

void AppStorageManagerTelemetryReporting::recordGetStorageTelemetry(const std::string& appId, uint64_t requestTime)
{
    if (!Utils::isTelemetryMetricsEnabled()) {
        return;
    }

    JsonObject jsonParam;
    int duration = durationSinceMs(requestTime);

    jsonParam["storageManagerLaunchTime"] = duration;
    jsonParam["appId"] = appId;

    LOGINFO("Record appId %s storageManagerLaunchTime %d", appId.c_str(), duration);
    if (Core::ERROR_NONE != recordTelemetry(appId, jsonParam, TELEMETRY_MARKER_LAUNCH_TIME)) {
        LOGERR("Failed to record telemetry for appId %s", appId.c_str());
    }
}

} // namespace Plugin
} // namespace WPEFramework
