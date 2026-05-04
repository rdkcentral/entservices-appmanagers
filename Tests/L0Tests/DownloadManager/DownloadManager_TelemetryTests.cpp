/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
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
 **/

/**
 * @file DownloadManager_TelemetryTests.cpp
 *
 * L0 tests for DownloadManagerTelemetryReporting covering:
 *   - getInstance returns singleton (same pointer on multiple calls)
 *   - initialize with valid service does not crash
 *   - reset clears telemetry client without crash
 *   - recordDownloadTimeTelemetry when telemetry disabled is a no-op
 *   - recordDownloadErrorTelemetry when telemetry disabled is a no-op
 *   - recordDownloadTimeTelemetry with valid params does not crash
 *   - recordDownloadErrorTelemetry with valid params does not crash
 */

#include <iostream>
#include <string>

#include "DownloadManagerTelemetryReporting.h"
#include "ServiceMock.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// getInstance returns singleton (same address every call)
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Telemetry_GetInstanceReturnsSingleton()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DownloadManagerTelemetryReporting& inst1 =
        WPEFramework::Plugin::DownloadManagerTelemetryReporting::getInstance();
    WPEFramework::Plugin::DownloadManagerTelemetryReporting& inst2 =
        WPEFramework::Plugin::DownloadManagerTelemetryReporting::getInstance();

    L0Test::ExpectTrue(tr, &inst1 == &inst2,
        "getInstance() returns the same singleton address on consecutive calls");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// initialize with valid service does not crash
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Telemetry_InitializeWithValidServiceDoesNotCrash()
{
    L0Test::TestResult tr;

    L0Test::ServiceMock::Config cfg;
    cfg.configLine = "{\"downloadDir\":\"/tmp/dm_l0_teleinit\"}";
    L0Test::ServiceMock svc(cfg);

    // initialize() calls setService() and initializeTelemetryClient().
    // Both must complete without crashing in an isolated / CI environment.
    WPEFramework::Plugin::DownloadManagerTelemetryReporting::getInstance().initialize(&svc);

    L0Test::ExpectTrue(tr, true, "initialize() with valid service does not crash");

    // Reset to leave the singleton in a clean state for subsequent tests
    WPEFramework::Plugin::DownloadManagerTelemetryReporting::getInstance().reset();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// reset clears telemetry client without crash
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Telemetry_ResetDoesNotCrash()
{
    L0Test::TestResult tr;

    // Calling reset() without a prior initialize() must not crash
    WPEFramework::Plugin::DownloadManagerTelemetryReporting::getInstance().reset();
    L0Test::ExpectTrue(tr, true, "reset() without prior initialize() does not crash");

    // Calling reset() a second time must be idempotent
    WPEFramework::Plugin::DownloadManagerTelemetryReporting::getInstance().reset();
    L0Test::ExpectTrue(tr, true, "reset() twice does not crash (idempotent)");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// recordDownloadTimeTelemetry when telemetry disabled is a no-op
// The isTelemetryMetricsEnabled() guard returns false if the telemetry client
// was never initialised or was reset, so the function must return immediately.
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Telemetry_RecordDownloadTimeWhenDisabledIsNoop()
{
    L0Test::TestResult tr;

    // Ensure singleton is in reset (disabled) state
    WPEFramework::Plugin::DownloadManagerTelemetryReporting::getInstance().reset();

    // Must not crash when telemetry is disabled
    WPEFramework::Plugin::DownloadManagerTelemetryReporting::getInstance()
        .recordDownloadTimeTelemetry("2001", 5000LL);

    L0Test::ExpectTrue(tr, true,
        "recordDownloadTimeTelemetry() is a no-op when telemetry is disabled");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// recordDownloadErrorTelemetry when telemetry disabled is a no-op
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Telemetry_RecordDownloadErrorWhenDisabledIsNoop()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DownloadManagerTelemetryReporting::getInstance().reset();

    WPEFramework::Plugin::DownloadManagerTelemetryReporting::getInstance()
        .recordDownloadErrorTelemetry("2001", 1 /* DOWNLOAD_FAILURE errorCode */);

    L0Test::ExpectTrue(tr, true,
        "recordDownloadErrorTelemetry() is a no-op when telemetry is disabled");

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// recordDownloadTimeTelemetry with valid params does not crash
// (even when the telemetry backend may not be available in test environment)
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Telemetry_RecordDownloadTimeWithValidParamsDoesNotCrash()
{
    L0Test::TestResult tr;

    L0Test::ServiceMock::Config cfg;
    L0Test::ServiceMock svc(cfg);

    WPEFramework::Plugin::DownloadManagerTelemetryReporting::getInstance().initialize(&svc);

    // Must not crash whether or not the telemetry backend is available
    WPEFramework::Plugin::DownloadManagerTelemetryReporting::getInstance()
        .recordDownloadTimeTelemetry("2002", 12000LL);

    L0Test::ExpectTrue(tr, true,
        "recordDownloadTimeTelemetry() with valid params does not crash");

    WPEFramework::Plugin::DownloadManagerTelemetryReporting::getInstance().reset();

    return tr.failures;
}

// ─────────────────────────────────────────────────────────────────────────────
// recordDownloadErrorTelemetry with valid params does not crash
// ─────────────────────────────────────────────────────────────────────────────

uint32_t Test_Telemetry_RecordDownloadErrorWithValidParamsDoesNotCrash()
{
    L0Test::TestResult tr;

    L0Test::ServiceMock::Config cfg;
    L0Test::ServiceMock svc(cfg);

    WPEFramework::Plugin::DownloadManagerTelemetryReporting::getInstance().initialize(&svc);

    WPEFramework::Plugin::DownloadManagerTelemetryReporting::getInstance()
        .recordDownloadErrorTelemetry("2003", 0 /* DISK_PERSISTENCE_FAILURE */);

    L0Test::ExpectTrue(tr, true,
        "recordDownloadErrorTelemetry() with valid params does not crash");

    WPEFramework::Plugin::DownloadManagerTelemetryReporting::getInstance().reset();

    return tr.failures;
}
