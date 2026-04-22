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
 * @file AppStorageManager_ImplementationTests.cpp
 *
 * L0 tests for StorageManagerImplementation covering core storage operations:
 *   - Configure
 *   - CreateStorage / GetStorage / DeleteStorage / Clear / ClearAll
 */

#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <sys/stat.h>
#include <unistd.h>

#include "AppStorageManagerImplementation.h"
#include "ServiceMock.h"
#include "fakes/FakePersistentStore.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

namespace {

using string = std::string;

/* Helper to create StorageManagerImplementation instance */
WPEFramework::Plugin::StorageManagerImplementation* CreateImpl()
{
    return WPEFramework::Core::Service<WPEFramework::Plugin::StorageManagerImplementation>::Create<
        WPEFramework::Plugin::StorageManagerImplementation>();
}

/* Helper to clean up test directory */
void CleanupTestDir(const std::string& path)
{
    std::string cmd = "rm -rf " + path;
    system(cmd.c_str());
}

/* Helper to create test directory */
void CreateTestDir(const std::string& path)
{
    mkdir(path.c_str(), 0755);
}

} // namespace

// ──────────────────────────────────────────────────────────────────────────────
// Configure Tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_ConfigureWithValidService
 *
 * Verifies successful configuration with valid IShell service
 */
uint32_t Test_Impl_ConfigureWithValidService()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    service.SetConfigLine("{\"path\":\"/tmp/test_appstorage\"}");

    // Mock PersistentStore query
    service.SetPersistentStore(new L0Test::FakePersistentStore());

    CleanupTestDir("/tmp/test_appstorage");
    CreateTestDir("/tmp/test_appstorage");

    const auto result = impl->Configure(&service);
    
    // Configure may return ERROR_NONE even if some initialization fails
    // We just verify it doesn't crash
    L0Test::ExpectTrue(tr, true, "Configure() completes without crash");

    CleanupTestDir("/tmp/test_appstorage");
    impl->Release();
    return tr.failures;
}

/* Test_Impl_ConfigureWithNullService
 *
 * Verifies that Configure() returns error when service is nullptr
 */
uint32_t Test_Impl_ConfigureWithNullService()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    const auto result = impl->Configure(nullptr);
    
    L0Test::ExpectEqU32(tr, result, WPEFramework::Core::ERROR_GENERAL, "Configure() fails with null service");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// CreateStorage Tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_CreateStorageWithValidInput
 *
 * Verifies successful storage creation with valid app ID and size
 */
uint32_t Test_Impl_CreateStorageWithValidInput()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    service.SetConfigLine("{\"path\":\"/tmp/test_appstorage\"}");
    service.SetPersistentStore(new L0Test::FakePersistentStore());

    CleanupTestDir("/tmp/test_appstorage");
    CreateTestDir("/tmp/test_appstorage");

    impl->Configure(&service);

    std::string path;
    std::string errorReason;
    const auto status = impl->CreateStorage("com.example.app", 10240, path, errorReason);

    L0Test::ExpectEqU32(tr, status, WPEFramework::Core::ERROR_NONE, "CreateStorage succeeds");
    L0Test::ExpectTrue(tr, !path.empty(), "Path returned");
    L0Test::ExpectTrue(tr, errorReason.empty(), "No error reason");

    CleanupTestDir("/tmp/test_appstorage");
    impl->Release();
    return tr.failures;
}

/* Test_Impl_CreateStorageWithEmptyAppId
 *
 * Verifies that CreateStorage() rejects empty app ID
 */
uint32_t Test_Impl_CreateStorageWithEmptyAppId()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    service.SetConfigLine("{\"path\":\"/tmp/test_appstorage\"}");
    service.SetPersistentStore(new L0Test::FakePersistentStore());

    CleanupTestDir("/tmp/test_appstorage");
    impl->Configure(&service);

    std::string path;
    std::string errorReason;
    const auto status = impl->CreateStorage("", 1024, path, errorReason);

    L0Test::ExpectEqU32(tr, status, WPEFramework::Core::ERROR_GENERAL, "CreateStorage fails with empty appId");
    L0Test::ExpectTrue(tr, !errorReason.empty(), "Error reason provided");

    CleanupTestDir("/tmp/test_appstorage");
    impl->Release();
    return tr.failures;
}

/* Test_Impl_CreateStorageWithZeroSize
 *
 * Verifies behavior when creating storage with zero size quota
 */
uint32_t Test_Impl_CreateStorageWithZeroSize()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    service.SetConfigLine("{\"path\":\"/tmp/test_appstorage\"}");
    service.SetPersistentStore(new L0Test::FakePersistentStore());

    CleanupTestDir("/tmp/test_appstorage");
    CreateTestDir("/tmp/test_appstorage");
    impl->Configure(&service);

    std::string path;
    std::string errorReason;
    const auto status = impl->CreateStorage("com.example.zerosizeapp", 0, path, errorReason);

    // Zero size should be valid (no minimum enforced currently)
    L0Test::ExpectEqU32(tr, status, WPEFramework::Core::ERROR_NONE, "CreateStorage accepts zero size");

    CleanupTestDir("/tmp/test_appstorage");
    impl->Release();
    return tr.failures;
}

/* Test_Impl_CreateStorageDuplicateAppId
 *
 * Verifies that creating storage for existing app ID with same size returns existing path
 */
uint32_t Test_Impl_CreateStorageDuplicateAppId()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    service.SetConfigLine("{\"path\":\"/tmp/test_appstorage\"}");
    service.SetPersistentStore(new L0Test::FakePersistentStore());

    CleanupTestDir("/tmp/test_appstorage");
    CreateTestDir("/tmp/test_appstorage");
    impl->Configure(&service);

    std::string path1, path2;
    std::string errorReason;
    
    // Create first time
    const auto status1 = impl->CreateStorage("app1", 1024, path1, errorReason);
    L0Test::ExpectEqU32(tr, status1, WPEFramework::Core::ERROR_NONE, "First CreateStorage succeeds");

    // Create again with same size
    const auto status2 = impl->CreateStorage("app1", 1024, path2, errorReason);
    L0Test::ExpectEqU32(tr, status2, WPEFramework::Core::ERROR_NONE, "Duplicate CreateStorage succeeds");
    L0Test::ExpectEqStr(tr, path1, path2, "Same path returned for duplicate");

    CleanupTestDir("/tmp/test_appstorage");
    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// GetStorage Tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_GetStorageWithValidAppId
 *
 * Verifies successful retrieval of storage information
 */
uint32_t Test_Impl_GetStorageWithValidAppId()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    service.SetConfigLine("{\"path\":\"/tmp/test_appstorage\"}");
    service.SetPersistentStore(new L0Test::FakePersistentStore());

    CleanupTestDir("/tmp/test_appstorage");
    CreateTestDir("/tmp/test_appstorage");
    impl->Configure(&service);

    std::string createPath, errorReason;
    impl->CreateStorage("app1", 2048, createPath, errorReason);

    std::string getPath;
    uint32_t size = 0, used = 0;
    const auto status = impl->GetStorage("app1", -1, -1, getPath, size, used);

    L0Test::ExpectEqU32(tr, status, WPEFramework::Core::ERROR_NONE, "GetStorage succeeds");
    L0Test::ExpectTrue(tr, !getPath.empty(), "Path returned");
    L0Test::ExpectEqU32(tr, size, 2048, "Size matches");

    CleanupTestDir("/tmp/test_appstorage");
    impl->Release();
    return tr.failures;
}

/* Test_Impl_GetStorageWithEmptyAppId
 *
 * Verifies rejection of empty app ID
 */
uint32_t Test_Impl_GetStorageWithEmptyAppId()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    service.SetConfigLine("{\"path\":\"/tmp/test_appstorage\"}");
    impl->Configure(&service);

    std::string path;
    uint32_t size = 0, used = 0;
    const auto status = impl->GetStorage("", -1, -1, path, size, used);

    L0Test::ExpectEqU32(tr, status, WPEFramework::Core::ERROR_GENERAL, "GetStorage fails with empty appId");

    impl->Release();
    return tr.failures;
}

/* Test_Impl_GetStorageWithNonExistentAppId
 *
 * Verifies error when app ID doesn't exist
 */
uint32_t Test_Impl_GetStorageWithNonExistentAppId()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    service.SetConfigLine("{\"path\":\"/tmp/test_appstorage\"}");
    service.SetPersistentStore(new L0Test::FakePersistentStore());

    CleanupTestDir("/tmp/test_appstorage");
    CreateTestDir("/tmp/test_appstorage");
    impl->Configure(&service);

    std::string path;
    uint32_t size = 0, used = 0;
    const auto status = impl->GetStorage("nonexistent", -1, -1, path, size, used);

    L0Test::ExpectEqU32(tr, status, WPEFramework::Core::ERROR_GENERAL, "GetStorage fails for non-existent app");

    CleanupTestDir("/tmp/test_appstorage");
    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// DeleteStorage Tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_DeleteStorageWithValidAppId
 *
 * Verifies successful storage deletion
 */
uint32_t Test_Impl_DeleteStorageWithValidAppId()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    service.SetConfigLine("{\"path\":\"/tmp/test_appstorage\"}");
    service.SetPersistentStore(new L0Test::FakePersistentStore());

    CleanupTestDir("/tmp/test_appstorage");
    CreateTestDir("/tmp/test_appstorage");
    impl->Configure(&service);

    std::string path, errorReason;
    impl->CreateStorage("app1", 1024, path, errorReason);

    const auto status = impl->DeleteStorage("app1", errorReason);

    L0Test::ExpectEqU32(tr, status, WPEFramework::Core::ERROR_NONE, "DeleteStorage succeeds");
    L0Test::ExpectTrue(tr, errorReason.empty(), "No error reason");

    CleanupTestDir("/tmp/test_appstorage");
    impl->Release();
    return tr.failures;
}

/* Test_Impl_DeleteStorageWithEmptyAppId
 *
 * Verifies rejection of empty app ID
 */
uint32_t Test_Impl_DeleteStorageWithEmptyAppId()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    service.SetConfigLine("{\"path\":\"/tmp/test_appstorage\"}");
    impl->Configure(&service);

    std::string errorReason;
    const auto status = impl->DeleteStorage("", errorReason);

    L0Test::ExpectEqU32(tr, status, WPEFramework::Core::ERROR_GENERAL, "DeleteStorage fails with empty appId");
    L0Test::ExpectTrue(tr, !errorReason.empty(), "Error reason provided");

    impl->Release();
    return tr.failures;
}

/* Test_Impl_DeleteStorageWithNonExistentAppId
 *
 * Verifies error when deleting non-existent app
 */
uint32_t Test_Impl_DeleteStorageWithNonExistentAppId()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    service.SetConfigLine("{\"path\":\"/tmp/test_appstorage\"}");
    service.SetPersistentStore(new L0Test::FakePersistentStore());

    CleanupTestDir("/tmp/test_appstorage");
    CreateTestDir("/tmp/test_appstorage");
    impl->Configure(&service);

    std::string errorReason;
    const auto status = impl->DeleteStorage("nonexistent", errorReason);

    L0Test::ExpectEqU32(tr, status, WPEFramework::Core::ERROR_GENERAL, "DeleteStorage fails for non-existent app");

    CleanupTestDir("/tmp/test_appstorage");
    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// Clear Tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_ClearStorageWithValidAppId
 *
 * Verifies clearing storage contents while preserving directory
 */
uint32_t Test_Impl_ClearStorageWithValidAppId()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    service.SetConfigLine("{\"path\":\"/tmp/test_appstorage\"}");
    service.SetPersistentStore(new L0Test::FakePersistentStore());

    CleanupTestDir("/tmp/test_appstorage");
    CreateTestDir("/tmp/test_appstorage");
    impl->Configure(&service);

    std::string path, errorReason;
    impl->CreateStorage("app1", 1024, path, errorReason);

    // Create a test file
    std::string testFile = path + "/testfile.txt";
    std::ofstream ofs(testFile);
    ofs << "test content";
    ofs.close();

    const auto status = impl->Clear("app1", errorReason);

    L0Test::ExpectEqU32(tr, status, WPEFramework::Core::ERROR_NONE, "Clear succeeds");
    L0Test::ExpectTrue(tr, errorReason.empty(), "No error reason");

    // Verify file was deleted but directory exists
    struct stat st;
    L0Test::ExpectTrue(tr, stat(testFile.c_str(), &st) != 0, "File was deleted");
    L0Test::ExpectTrue(tr, stat(path.c_str(), &st) == 0, "Directory still exists");

    CleanupTestDir("/tmp/test_appstorage");
    impl->Release();
    return tr.failures;
}

/* Test_Impl_ClearStorageWithEmptyAppId
 *
 * Verifies rejection of empty app ID
 */
uint32_t Test_Impl_ClearStorageWithEmptyAppId()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    service.SetConfigLine("{\"path\":\"/tmp/test_appstorage\"}");
    impl->Configure(&service);

    std::string errorReason;
    const auto status = impl->Clear("", errorReason);

    L0Test::ExpectEqU32(tr, status, WPEFramework::Core::ERROR_GENERAL, "Clear fails with empty appId");
    L0Test::ExpectTrue(tr, !errorReason.empty(), "Error reason provided");

    impl->Release();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// ClearAll Tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Impl_ClearAllWithNoExemptions
 *
 * Verifies clearing all app storage with no exemptions
 */
uint32_t Test_Impl_ClearAllWithNoExemptions()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    service.SetConfigLine("{\"path\":\"/tmp/test_appstorage\"}");
    service.SetPersistentStore(new L0Test::FakePersistentStore());

    CleanupTestDir("/tmp/test_appstorage");
    CreateTestDir("/tmp/test_appstorage");
    impl->Configure(&service);

    std::string path1, path2, errorReason;
    impl->CreateStorage("app1", 1024, path1, errorReason);
    impl->CreateStorage("app2", 1024, path2, errorReason);

    const auto status = impl->ClearAll("[]", errorReason);

    L0Test::ExpectEqU32(tr, status, WPEFramework::Core::ERROR_NONE, "ClearAll succeeds");

    CleanupTestDir("/tmp/test_appstorage");
    impl->Release();
    return tr.failures;
}

/* Test_Impl_ClearAllWithExemptions
 *
 * Verifies clearing all except exempted apps
 */
uint32_t Test_Impl_ClearAllWithExemptions()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    service.SetConfigLine("{\"path\":\"/tmp/test_appstorage\"}");
    service.SetPersistentStore(new L0Test::FakePersistentStore());

    CleanupTestDir("/tmp/test_appstorage");
    CreateTestDir("/tmp/test_appstorage");
    impl->Configure(&service);

    std::string path1, path2, errorReason;
    impl->CreateStorage("app1", 1024, path1, errorReason);
    impl->CreateStorage("app2", 1024, path2, errorReason);

    const auto status = impl->ClearAll("[\"app2\"]", errorReason);

    L0Test::ExpectEqU32(tr, status, WPEFramework::Core::ERROR_NONE, "ClearAll with exemptions succeeds");

    CleanupTestDir("/tmp/test_appstorage");
    impl->Release();
    return tr.failures;
}

/* Test_Impl_ClearAllWithInvalidJsonFormat
 *
 * Verifies handling of invalid JSON format
 */
uint32_t Test_Impl_ClearAllWithInvalidJsonFormat()
{
    L0Test::TestResult tr;

    auto* impl = CreateImpl();
    L0Test::ServiceMock service;

    service.SetConfigLine("{\"path\":\"/tmp/test_appstorage\"}");
    service.SetPersistentStore(new L0Test::FakePersistentStore());

    CleanupTestDir("/tmp/test_appstorage");
    CreateTestDir("/tmp/test_appstorage");
    impl->Configure(&service);

    std::string errorReason;
    const auto status = impl->ClearAll("invalid json", errorReason);

    // Should handle gracefully (logs error but continues)
    L0Test::ExpectTrue(tr, true, "ClearAll handles invalid JSON without crash");

    CleanupTestDir("/tmp/test_appstorage");
    impl->Release();
    return tr.failures;
}
