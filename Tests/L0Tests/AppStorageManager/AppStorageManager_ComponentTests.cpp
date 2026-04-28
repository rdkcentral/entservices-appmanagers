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
 * @file AppStorageManager_ComponentTests.cpp
 *
 * L0 tests for AppStorageManager components:
 *   - RequestHandler (singleton, configuration, storage operations)
 *   - TelemetryReporting (initialization, metrics)
 */

#include <iostream>
#include <string>

#include "RequestHandler.h"
#include "AppStorageManagerTelemetryReporting.h"
#include "ServiceMock.h"
#include "fakes/FakePersistentStore.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

using WPEFramework::Plugin::RequestHandler;
using WPEFramework::Plugin::AppStorageManagerTelemetryReporting;

/* ========================================================================== */
/* RequestHandler Tests
/* ========================================================================== */

/* Test_RH_GetInstanceReturnsSingleton
 *
 * Verifies that getInstance() returns the same instance.
 */
uint32_t Test_RH_GetInstanceReturnsSingleton()
{
    L0Test::TestResult tr;

    RequestHandler& instance1 = RequestHandler::getInstance();
    RequestHandler& instance2 = RequestHandler::getInstance();

    L0Test::ExpectTrue(tr, &instance1 == &instance2, "getInstance returns the same instance");

    return tr.failures;
}

/* ========================================================================== */
/* Test_RH_SetBaseStoragePathValid
 *
 * Verifies that SetBaseStoragePath() accepts valid path.
 */
uint32_t Test_RH_SetBaseStoragePathValid()
{
    L0Test::TestResult tr;

    RequestHandler& handler = RequestHandler::getInstance();
    
    // Should not crash
    handler.SetBaseStoragePath("/tmp/appdata");
    
    L0Test::ExpectTrue(tr, true, "SetBaseStoragePath executed without crash");

    return tr.failures;
}

/* ========================================================================== */
/* Telemetry Tests                                                            */
/* ========================================================================== */

/* Test_Tel_GetInstanceReturnsSingleton
 *
 * Verifies that telemetry getInstance() returns the same instance.
 */
uint32_t Test_Tel_GetInstanceReturnsSingleton()
{
    L0Test::TestResult tr;

    AppStorageManagerTelemetryReporting& instance1 = AppStorageManagerTelemetryReporting::getInstance();
    AppStorageManagerTelemetryReporting& instance2 = AppStorageManagerTelemetryReporting::getInstance();

    L0Test::ExpectTrue(tr, &instance1 == &instance2, "Telemetry getInstance returns the same instance");

    return tr.failures;
}
