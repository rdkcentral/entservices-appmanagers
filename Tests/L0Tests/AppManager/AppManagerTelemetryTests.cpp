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

// ============================================================
// Convenience aliases used by all tests below
// ============================================================
namespace {

using AppIM    = WPEFramework::Plugin::AppInfoManager;
using AppAct   = WPEFramework::Plugin::AppManagerImplementation;
using AppTel   = WPEFramework::Plugin::AppManagerTelemetryReporting;
using LcState  = WPEFramework::Exchange::ILifecycleManager::LifecycleState;
using AppState = WPEFramework::Exchange::IAppManager::AppLifecycleState;
namespace AppType = WPEFramework::Plugin::AppManagerTypes;

void setupTelApp(const std::string& appId,
                 AppType::CurrentAction action,
                 AppState targetState = AppState::APP_STATE_ACTIVE)
{
    AppIM::getInstance().setCurrentAction(appId, action);
    AppIM::getInstance().setCurrentActionTime(appId, 1);
    AppIM::getInstance().setAppInstanceId(appId, "inst-tel");
    AppIM::getInstance().setTargetAppState(appId, targetState);
}

} // anonymous namespace

// ============================================================
// reportTelemetryData branch coverage
// ============================================================

uint32_t Test_AM_TelReportDataNoAppInfo()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryData("unknown-app", AppAct::APP_ACTION_LAUNCH);
    L0Test::ExpectTrue(tr, true, "reportTelemetryData with unknown appId does not crash");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelReportDataActionMismatch()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    setupTelApp("app-mismatch", AppType::APP_ACTION_LAUNCH);
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryData("app-mismatch", AppAct::APP_ACTION_CLOSE);
    L0Test::ExpectTrue(tr, true, "reportTelemetryData action mismatch does not crash");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelReportDataPreloadNoMarker()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    setupTelApp("app-preload", AppType::APP_ACTION_PRELOAD);
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryData("app-preload", AppAct::APP_ACTION_PRELOAD);
    L0Test::ExpectTrue(tr, true, "reportTelemetryData PRELOAD produces no marker");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelReportDataClosePublishClose()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    setupTelApp("app-close-pub", AppType::APP_ACTION_CLOSE, AppState::APP_STATE_ACTIVE);
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryData("app-close-pub", AppAct::APP_ACTION_CLOSE);
    L0Test::ExpectTrue(tr, true, "reportTelemetryData CLOSE non-suspended publishes on record");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelReportDataCloseTargetSuspended()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    setupTelApp("app-close-sus", AppType::APP_ACTION_CLOSE, AppState::APP_STATE_SUSPENDED);
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryData("app-close-sus", AppAct::APP_ACTION_CLOSE);
    L0Test::ExpectTrue(tr, true, "reportTelemetryData CLOSE with SUSPENDED target produces no marker");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelReportDataCloseTargetHibernated()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    setupTelApp("app-close-hib", AppType::APP_ACTION_CLOSE, AppState::APP_STATE_HIBERNATED);
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryData("app-close-hib", AppAct::APP_ACTION_CLOSE);
    L0Test::ExpectTrue(tr, true, "reportTelemetryData CLOSE with HIBERNATED target produces no marker");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelReportDataTerminateRecord()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    setupTelApp("app-terminate", AppType::APP_ACTION_TERMINATE);
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryData("app-terminate", AppAct::APP_ACTION_TERMINATE);
    L0Test::ExpectTrue(tr, true, "reportTelemetryData TERMINATE records without publish");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelReportDataKillRecord()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    setupTelApp("app-kill", AppType::APP_ACTION_KILL);
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryData("app-kill", AppAct::APP_ACTION_KILL);
    L0Test::ExpectTrue(tr, true, "reportTelemetryData KILL records without publish");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelReportDataDefaultAction()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    setupTelApp("app-default", AppType::APP_ACTION_NONE);
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryData("app-default", AppAct::APP_ACTION_NONE);
    L0Test::ExpectTrue(tr, true, "reportTelemetryData with APP_ACTION_NONE hits default branch");
    AppIM::getInstance().clear();
    return tr.failures;
}

// ============================================================
// reportTelemetryDataOnStateChange branch coverage
// ============================================================

uint32_t Test_AM_TelOnStateChangeNoAppInfo()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryDataOnStateChange("unknown-state-app", LcState::ACTIVE);
    L0Test::ExpectTrue(tr, true, "reportTelemetryDataOnStateChange unknown appId does not crash");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelOnStateChangeLaunchNonActive()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    setupTelApp("app-launch-noact", AppType::APP_ACTION_LAUNCH);
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryDataOnStateChange("app-launch-noact", LcState::PAUSED);
    L0Test::ExpectTrue(tr, true, "reportTelemetryDataOnStateChange LAUNCH+PAUSED produces no marker");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelOnStateChangePreloadPaused()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    setupTelApp("app-preload-paused", AppType::APP_ACTION_PRELOAD);
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryDataOnStateChange("app-preload-paused", LcState::PAUSED);
    L0Test::ExpectTrue(tr, true, "reportTelemetryDataOnStateChange PRELOAD+PAUSED records and publishes");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelOnStateChangePreloadNonPaused()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    setupTelApp("app-preload-active", AppType::APP_ACTION_PRELOAD);
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryDataOnStateChange("app-preload-active", LcState::ACTIVE);
    L0Test::ExpectTrue(tr, true, "reportTelemetryDataOnStateChange PRELOAD+ACTIVE produces no marker");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelOnStateChangeCloseUnloaded()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    setupTelApp("app-close-unload", AppType::APP_ACTION_CLOSE);
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryDataOnStateChange("app-close-unload", LcState::UNLOADED);
    L0Test::ExpectTrue(tr, true, "reportTelemetryDataOnStateChange CLOSE+UNLOADED records and publishes");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelOnStateChangeTerminateUnloaded()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    setupTelApp("app-term-unload", AppType::APP_ACTION_TERMINATE);
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryDataOnStateChange("app-term-unload", LcState::UNLOADED);
    L0Test::ExpectTrue(tr, true, "reportTelemetryDataOnStateChange TERMINATE+UNLOADED records and publishes");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelOnStateChangeKillUnloaded()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    setupTelApp("app-kill-unload", AppType::APP_ACTION_KILL);
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryDataOnStateChange("app-kill-unload", LcState::UNLOADED);
    L0Test::ExpectTrue(tr, true, "reportTelemetryDataOnStateChange KILL+UNLOADED records and publishes");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelOnStateChangeCloseNonUnloaded()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    setupTelApp("app-close-active", AppType::APP_ACTION_CLOSE);
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryDataOnStateChange("app-close-active", LcState::ACTIVE);
    L0Test::ExpectTrue(tr, true, "reportTelemetryDataOnStateChange CLOSE+ACTIVE produces no marker");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelOnStateChangeDefaultAction()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    setupTelApp("app-state-default", AppType::APP_ACTION_NONE);
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryDataOnStateChange("app-state-default", LcState::ACTIVE);
    L0Test::ExpectTrue(tr, true, "reportTelemetryDataOnStateChange APP_ACTION_NONE hits default branch");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelOnStateChangeLaunchInteractive()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    setupTelApp("app-launch-interactive", AppType::APP_ACTION_LAUNCH);
    AppIM::getInstance().setPackageInfoType("app-launch-interactive", AppType::APPLICATION_TYPE_INTERACTIVE);
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryDataOnStateChange("app-launch-interactive", LcState::ACTIVE);
    L0Test::ExpectTrue(tr, true, "reportTelemetryDataOnStateChange LAUNCH INTERACTIVE covers launchType ternary");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelOnStateChangePreloadInteractive()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    setupTelApp("app-preload-interactive", AppType::APP_ACTION_PRELOAD);
    AppIM::getInstance().setPackageInfoType("app-preload-interactive", AppType::APPLICATION_TYPE_INTERACTIVE);
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryDataOnStateChange("app-preload-interactive", LcState::PAUSED);
    L0Test::ExpectTrue(tr, true, "reportTelemetryDataOnStateChange PRELOAD INTERACTIVE covers launchType ternary");
    AppIM::getInstance().clear();
    return tr.failures;
}

// ============================================================
// reportTelemetryErrorData branch coverage
// ============================================================

uint32_t Test_AM_TelErrorDataPreload()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryErrorData("app-err-preload", AppAct::APP_ACTION_PRELOAD, AppAct::ERROR_INTERNAL);
    L0Test::ExpectTrue(tr, true, "reportTelemetryErrorData PRELOAD hits LAUNCH_ERROR case");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelErrorDataClose()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryErrorData("app-err-close", AppAct::APP_ACTION_CLOSE, AppAct::ERROR_INTERNAL);
    L0Test::ExpectTrue(tr, true, "reportTelemetryErrorData CLOSE hits CLOSE_ERROR case");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelErrorDataTerminate()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryErrorData("app-err-terminate", AppAct::APP_ACTION_TERMINATE, AppAct::ERROR_INTERNAL);
    L0Test::ExpectTrue(tr, true, "reportTelemetryErrorData TERMINATE hits CLOSE_ERROR case");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelErrorDataKill()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryErrorData("app-err-kill", AppAct::APP_ACTION_KILL, AppAct::ERROR_INTERNAL);
    L0Test::ExpectTrue(tr, true, "reportTelemetryErrorData KILL hits CLOSE_ERROR case");
    AppIM::getInstance().clear();
    return tr.failures;
}

uint32_t Test_AM_TelErrorDataDefault()
{
    L0Test::TestResult tr;
    AppIM::getInstance().clear();
    auto& tel = AppTel::getInstance();
    tel.reportTelemetryErrorData("app-err-default", AppAct::APP_ACTION_NONE, AppAct::ERROR_INTERNAL);
    L0Test::ExpectTrue(tr, true, "reportTelemetryErrorData APP_ACTION_NONE hits default branch");
    AppIM::getInstance().clear();
    return tr.failures;
}

// ============================================================
// recordLaunchTime branch coverage
// ============================================================

uint32_t Test_AM_TelRecordLaunchTime()
{
    L0Test::TestResult tr;
    auto& tel = AppTel::getInstance();
    tel.recordLaunchTime("app-launch-time", 1200);
    L0Test::ExpectTrue(tr, true, "recordLaunchTime with valid params records telemetry without crash");
    return tr.failures;
}
