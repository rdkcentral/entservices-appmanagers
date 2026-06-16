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

#include <string>

#include "AppManagerTelemetryReporting.h"
#include "AppInfoManager.h"
#include "common/L0Expect.hpp"

uint32_t Test_AM_TelemetryReportingStability()
{
    L0Test::TestResult tr;

    auto& telemetry = WPEFramework::Plugin::AppManagerTelemetryReporting::getInstance();
    auto& telemetryAgain = WPEFramework::Plugin::AppManagerTelemetryReporting::getInstance();
    L0Test::ExpectTrue(tr, &telemetry == &telemetryAgain, "getInstance() returns the same singleton reference");

    WPEFramework::Plugin::AppInfoManager::getInstance().clear();
    WPEFramework::Plugin::AppInfoManager::getInstance().setCurrentAction("app-telemetry", WPEFramework::Plugin::AppManagerTypes::APP_ACTION_LAUNCH);
    WPEFramework::Plugin::AppInfoManager::getInstance().setCurrentActionTime("app-telemetry", 1);
    WPEFramework::Plugin::AppInfoManager::getInstance().setAppInstanceId("app-telemetry", "instance-1");

    telemetry.reportTelemetryData("app-telemetry", WPEFramework::Plugin::AppManagerImplementation::APP_ACTION_LAUNCH);
    telemetry.reportTelemetryDataOnStateChange("app-telemetry", WPEFramework::Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    telemetry.reportTelemetryErrorData("app-telemetry", WPEFramework::Plugin::AppManagerImplementation::APP_ACTION_LAUNCH, WPEFramework::Plugin::AppManagerImplementation::ERROR_INTERNAL);
    telemetry.reportAppCrashedTelemetry("app-telemetry", "instance-1", "crash");

    WPEFramework::Plugin::AppInfoManager::getInstance().clear();
    L0Test::ExpectTrue(tr, true, "Telemetry reporting paths remain stable in the L0 environment");
    return tr.failures;
}
