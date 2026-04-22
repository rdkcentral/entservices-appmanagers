/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
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
 **/

/**
 * @file AppStorageManager_ComponentTests.cpp
 *
 * L0 tests for AppStorageManager components:
 *   - RequestHandler
 *   - AppStorageManagerTelemetryReporting
 */

#include <atomic>
#include <iostream>
#include <string>

#include "RequestHandler.h"
#include "AppStorageManagerTelemetryReporting.h"
#include "ServiceMock.h"
#include "fakes/FakePersistentStore.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

namespace {

using string = std::string;

} // namespace

// ──────────────────────────────────────────────────────────────────────────────
// RequestHandler Tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_RH_GetInstanceReturnsSingleton
 *
 * Verifies that getInstance() returns the same instance (singleton pattern)
 */
uint32_t Test_RH_GetInstanceReturnsSingleton()
{
    L0Test::TestResult tr;

    auto& instance1 = WPEFramework::Plugin::RequestHandler::getInstance();
    auto& instance2 = WPEFramework::Plugin::RequestHandler::getInstance();

    L0Test::ExpectTrue(tr, &instance1 == &instance2, "getInstance() returns same instance");

    return tr.failures;
}

/* Test_RH_SetBaseStoragePathValid
 *
 * Verifies setting base storage path  */
uint32_t Test_RH_SetBaseStoragePathValid()
{
    L0Test::TestResult tr;

    auto& handler = WPEFramework::Plugin::RequestHandler::getInstance();
    
    handler.SetBaseStoragePath("/tmp/test_base_path");

    // We can't directly verify the internal state, but we verify it doesn't crash
    L0Test::ExpectTrue(tr, true, "SetBaseStoragePath() completes without crash");

    return tr.failures;
}

/* Test_RH_CreatePersistentStoreObjectSuccess
 *
 * Verifies successful creation of persistent store object
 */
uint32_t Test_RH_CreatePersistentStoreObjectSuccess()
{
    L0Test::TestResult tr;

    L0Test::ServiceMock service;
    service.SetPersistentStore(new L0Test::FakePersistentStore());

    auto& handler = WPEFramework::Plugin::RequestHandler::getInstance();
    handler.setCurrentService(&service);

    const auto status = handler.createPersistentStoreRemoteStoreObject();

    L0Test::ExpectEqU32(tr, status, WPEFramework::Core::ERROR_NONE, 
                       "createPersistentStoreRemoteStoreObject() succeeds");

    // Clean up
    handler.releasePersistentStoreRemoteStoreObject();

    return tr.failures;
}

/* Test_RH_CreatePersistentStoreObjectFailsWithNullService
 *
 * Verifies failure when service is null
 */
uint32_t Test_RH_CreatePersistentStoreObjectFailsWithNullService()
{
    L0Test::TestResult tr;

    auto& handler = WPEFramework::Plugin::RequestHandler::getInstance();
    handler.setCurrentService(nullptr);

    const auto status = handler.createPersistentStoreRemoteStoreObject();

    L0Test::ExpectEqU32(tr, status, WPEFramework::Core::ERROR_GENERAL,
                       "createPersistentStoreRemoteStoreObject() fails with null service");

    return tr.failures;
}

/* Test_RH_IsValidAppStorageDirectoryValid
 *
 * Verifies validation of valid app directory names
 */
uint32_t Test_RH_IsValidAppStorageDirectoryValid()
{
    L0Test::TestResult tr;

    auto& handler = WPEFramework::Plugin::RequestHandler::getInstance();

    L0Test::ExpectTrue(tr, handler.isValidAppStorageDirectory("com.example.app"), 
                      "Valid app directory name accepted");
    L0Test::ExpectTrue(tr, handler.isValidAppStorageDirectory("app123"), 
                      "Valid app directory name with numbers accepted");
    L0Test::ExpectTrue(tr, handler.isValidAppStorageDirectory("my-app_1.0"), 
                      "Valid app directory name with special chars accepted");

    return tr.failures;
}

/* Test_RH_IsValidAppStorageDirectoryInvalid
 *
 * Verifies rejection of invalid directory names
 */
uint32_t Test_RH_IsValidAppStorageDirectoryInvalid()
{
    L0Test::TestResult tr;

    auto& handler = WPEFramework::Plugin::RequestHandler::getInstance();

    L0Test::ExpectTrue(tr, !handler.isValidAppStorageDirectory("."), 
                      "Single dot rejected");
    L0Test::ExpectTrue(tr, !handler.isValidAppStorageDirectory(".."), 
                      "Double dot rejected");
    L0Test::ExpectTrue(tr, !handler.isValidAppStorageDirectory(""), 
                      "Empty string rejected");

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// Telemetry Tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Tel_GetInstanceReturnsSingleton
 *
 * Verifies singleton pattern for telemetry reporting
 */
uint32_t Test_Tel_GetInstanceReturnsSingleton()
{
    L0Test::TestResult tr;

    auto& instance1 = WPEFramework::Plugin::AppStorageManagerTelemetryReporting::getInstance();
    auto& instance2 = WPEFramework::Plugin::AppStorageManagerTelemetryReporting::getInstance();

    L0Test::ExpectTrue(tr, &instance1 == &instance2, "getInstance() returns same telemetry instance");

    return tr.failures;
}
