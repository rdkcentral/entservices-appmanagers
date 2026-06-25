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

#include "TelemetryReportingBase.h"

namespace WPEFramework {
namespace Plugin {
namespace Utils {

class AppManagersBootstrapper final : public TelemetryReportingBase {
public:
    AppManagersBootstrapper(const AppManagersBootstrapper&) = delete;
    AppManagersBootstrapper& operator=(const AppManagersBootstrapper&) = delete;
    static AppManagersBootstrapper& getInstance()
    {
        static AppManagersBootstrapper s_instance;
        return s_instance;
    }

private:
    AppManagersBootstrapper() = default;
};

} // namespace Utils
} // namespace Plugin
} // namespace WPEFramework

/*
 * Macro pattern intentionally mirrors AppGateway telemetry helpers:
 * 1. Define per-plugin telemetry client accessors in the translation unit.
 * 2. Initialize telemetry through a single macro.
 * 3. Start RAII bootstrap timing through a single macro.
 */
#define RDKAM_DEFINE_TELEMETRY_CLIENT(telemetryType, bootstrapFieldName) \
    namespace { \
        telemetryType& GetLocalTelemetryClient() { \
            return telemetryType::getInstance(); \
        } \
        __attribute__((unused)) const char* GetLocalBootstrapFieldName() { \
            return bootstrapFieldName; \
        } \
    }

#define RDKAM_TELEMETRY_INIT(service) \
    do { \
        GetLocalTelemetryClient().initialize(service); \
    } while (0)

#define RDKAM_RECORD_BOOTSTRAP_TIME(service) \
    WPEFramework::Plugin::Utils::TelemetryReportingBase::ScopedBootstrapTimer bootstrapTimer( \
        &GetLocalTelemetryClient(), \
        service, \
        GetLocalBootstrapFieldName())
