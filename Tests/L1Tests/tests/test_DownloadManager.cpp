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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <atomic>

#include "DownloadManager.h"
#include "DownloadManagerImplementation.h"
#include <interfaces/IDownloadManager.h>
#include <interfaces/json/JDownloadManager.h>
using namespace WPEFramework;

#include "ISubSystemMock.h"
#include "ServiceMock.h"
#include "COMLinkMock.h"
#include "ThunderPortability.h"
#include "WorkerPoolImplementation.h"
#include "FactoriesImplementation.h"

#define TEST_LOG(x, ...) printf("[TEST_LOG] " x "\n", ##__VA_ARGS__)

using ::testing::NiceMock;
using namespace std;

typedef enum : uint32_t {
    DownloadManager_invalidStatus = 0,
    DownloadManager_AppDownloadStatus
} DownloadManagerTest_status_t;

struct StatusParams {
    string downloadId;
    string fileLocator;
    Exchange::IDownloadManager::FailReason reason;
};

class DownloadManagerTest : public ::testing::Test {
protected:
    // Declare the protected members
    ServiceMock* mServiceMock = nullptr;
    SubSystemMock* mSubSystemMock = nullptr;

    Core::ProxyType<WorkerPoolImplementation> workerPool; 
    Core::ProxyType<Plugin::DownloadManager> plugin;
    FactoriesImplementation factoriesImplementation;

    // Constructor
    DownloadManagerTest()
     : workerPool(Core::ProxyType<WorkerPoolImplementation>::Create(
         2, Core::Thread::DefaultStackSize(), 16)),
        plugin(Core::ProxyType<Plugin::DownloadManager>::Create())
    {
        if (workerPool.IsValid()) {
            Core::IWorkerPool::Assign(&(*workerPool));
            workerPool->Run();
        }

        if (!plugin.IsValid()) {
            TEST_LOG("WARNING: Plugin creation failed - tests may be limited");
        }
    }

    // Destructor
    virtual ~DownloadManagerTest() override
    {

        Core::IWorkerPool::Assign(nullptr);
        workerPool.Release();
    }

    Core::hresult createResources()
    {        
        Core::hresult status = Core::ERROR_GENERAL;

        try {
            // Set up mocks and expect calls
            mServiceMock = new NiceMock<ServiceMock>;
            mSubSystemMock = new NiceMock<SubSystemMock>;

            EXPECT_CALL(*mServiceMock, ConfigLine())
              .Times(::testing::AnyNumber())
              .WillRepeatedly(::testing::Return("{\"downloadDir\": \"/opt/downloads/\"}"));

            EXPECT_CALL(*mServiceMock, PersistentPath())
              .Times(::testing::AnyNumber())
              .WillRepeatedly(::testing::Return("/tmp/"));

            EXPECT_CALL(*mServiceMock, VolatilePath())
              .Times(::testing::AnyNumber())
              .WillRepeatedly(::testing::Return("/tmp/"));

            EXPECT_CALL(*mServiceMock, DataPath())
              .Times(::testing::AnyNumber())
              .WillRepeatedly(::testing::Return("/tmp/"));

            EXPECT_CALL(*mServiceMock, SubSystems())
              .Times(::testing::AnyNumber())
              .WillRepeatedly(::testing::Return(mSubSystemMock));

            EXPECT_CALL(*mServiceMock, AddRef())
              .Times(::testing::AnyNumber());

            EXPECT_CALL(*mServiceMock, Release())
              .Times(::testing::AnyNumber());

            // Initialize plugin factories for L1 testing
            PluginHost::IFactories::Assign(&factoriesImplementation);

            if (!plugin.IsValid()) {
                return Core::ERROR_GENERAL;
            }
            status = Core::ERROR_NONE;
        } catch (const std::exception& e) {
            TEST_LOG("Exception in createResources: %s", e.what());
            status = Core::ERROR_GENERAL;
        } catch (...) {
            TEST_LOG("Unknown exception in createResources");
            status = Core::ERROR_GENERAL;
        }

        return status;
    }

    void releaseResources()
    {
        try {
            if (mServiceMock) {
                delete mServiceMock;
                mServiceMock = nullptr;
            }

            if (mSubSystemMock) {
                delete mSubSystemMock;
                mSubSystemMock = nullptr;
            }
        } catch (const std::exception& e) {
            TEST_LOG("Exception in releaseResources: %s", e.what());
        } catch (...) {
            TEST_LOG("Unknown exception in releaseResources");
        }
    }

    void SetUp() override
    {
        try {
            Core::hresult status = createResources();
            EXPECT_EQ(status, Core::ERROR_NONE);
        } catch (const std::exception& e) {
            TEST_LOG("Exception in SetUp: %s", e.what());
            FAIL() << "SetUp failed with exception: " << e.what();
        } catch (...) {
            FAIL() << "SetUp failed with unknown exception";
        }
    }

    void TearDown() override
    {
        releaseResources();
    }
};

class NotificationTest : public Exchange::IDownloadManager::INotification
{
    private:
        mutable std::atomic<uint32_t> m_refCount;

    public:
        /** @brief Mutex */
        std::mutex m_mutex;

        /** @brief Condition variable */
        std::condition_variable m_condition_variable;

        /** @brief Status signal flag */
        uint32_t m_status_signal = DownloadManager_invalidStatus;

        StatusParams m_status_param;

        NotificationTest() : m_refCount(1)
        {
        }

        virtual ~NotificationTest() override = default;

        // Proper IUnknown implementation for Thunder framework
        virtual void AddRef() const override { 
            m_refCount.fetch_add(1);
        }

        virtual uint32_t Release() const override { 
            uint32_t result = m_refCount.fetch_sub(1) - 1;
            if (result == 0) {
                delete this;
            }
            return result;
        }

        uint32_t WaitForStatusSignal(uint32_t timeout_ms, DownloadManagerTest_status_t status)
        {
            uint32_t status_signal = DownloadManager_invalidStatus;
            std::unique_lock<std::mutex> lock(m_mutex);
            auto now = std::chrono::steady_clock::now();
            auto timeout = std::chrono::milliseconds(timeout_ms);
            if (m_condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout)
            {
                 TEST_LOG("Timeout waiting for request status event");
                 return m_status_signal;
            }
            status_signal = m_status_signal;
            m_status_signal = DownloadManager_invalidStatus;
        return status_signal;
        }
    private:
        BEGIN_INTERFACE_MAP(NotificationTest)
        INTERFACE_ENTRY(Exchange::IDownloadManager::INotification)
        END_INTERFACE_MAP

        void SetStatusParams(const StatusParams& statusParam)
        {
            m_status_param = statusParam;
        }

        void OnAppDownloadStatus(const string& downloadStatus) override {
            m_status_signal = DownloadManager_AppDownloadStatus;

            std::unique_lock<std::mutex> lock(m_mutex);

            JsonArray list;
            list.FromString(downloadStatus);

            if (list.Length() > 0) {
                JsonObject obj = list[0].Object();
                m_status_param.downloadId = obj["downloadId"].String();
                m_status_param.fileLocator = obj["fileLocator"].String();

                if (obj.HasLabel("failReason")) {
                    string reason = obj["failReason"].String();
                    if (reason == "DOWNLOAD_FAILURE") {
                        m_status_param.reason = Exchange::IDownloadManager::FailReason::DOWNLOAD_FAILURE;
                    } else if (reason == "DISK_PERSISTENCE_FAILURE") {
                        m_status_param.reason = Exchange::IDownloadManager::FailReason::DISK_PERSISTENCE_FAILURE;
                    }
                }
            }

            // Validate that JSON structure was properly parsed
            EXPECT_TRUE(list.Length() > 0) << "JSON list should contain at least one element";
            if (list.Length() > 0) {
                // Validate that downloadId field exists in JSON (even if empty)
                JsonObject obj = list[0].Object();
                EXPECT_TRUE(obj.HasLabel("downloadId")) << "JSON should contain downloadId field";
                EXPECT_TRUE(obj.HasLabel("fileLocator")) << "JSON should contain fileLocator field";
            }

            m_condition_variable.notify_one();
        }
    };

// Dedicated test fixture for DownloadManagerImplementation direct testing
class DownloadManagerImplementationTest : public ::testing::Test {
protected:
    ServiceMock* mServiceMock = nullptr;
    SubSystemMock* mSubSystemMock = nullptr;
    Core::ProxyType<Plugin::DownloadManagerImplementation> mDownloadManagerImpl;

    DownloadManagerImplementationTest() {
        // Create implementation object
        mDownloadManagerImpl = Core::ProxyType<Plugin::DownloadManagerImplementation>::Create();
    }

    virtual ~DownloadManagerImplementationTest() override = default;

    void SetUp() override {
        // Create mocks
        mServiceMock = new NiceMock<ServiceMock>;
        mSubSystemMock = new NiceMock<SubSystemMock>;

        // Configure ServiceMock expectations for proper plugin operation
        EXPECT_CALL(*mServiceMock, ConfigLine())
            .WillRepeatedly(::testing::Return("{\"downloadDir\":\"/tmp/downloads/\",\"downloadId\":3000}"));

        EXPECT_CALL(*mServiceMock, SubSystems())
            .WillRepeatedly(::testing::Return(mSubSystemMock));

        EXPECT_CALL(*mServiceMock, AddRef())
            .Times(::testing::AnyNumber());

        EXPECT_CALL(*mServiceMock, Release())
            .Times(::testing::AnyNumber());

        // Configure SubSystemMock - start with INTERNET active for successful downloads
        EXPECT_CALL(*mSubSystemMock, IsActive(PluginHost::ISubSystem::INTERNET))
            .WillRepeatedly(::testing::Return(true));
    }

    void TearDown() override {
        // Clean up implementation if initialized
        if (mDownloadManagerImpl.IsValid()) {
            // Deinitialize if it was initialized
            mDownloadManagerImpl->Deinitialize(mServiceMock);
            mDownloadManagerImpl.Release();
        }

        // Clean up mocks
        if (mServiceMock) {
            delete mServiceMock;
            mServiceMock = nullptr;
        }

        if (mSubSystemMock) {
            delete mSubSystemMock;
            mSubSystemMock = nullptr;
        }
    }

    Plugin::DownloadManagerImplementation* getRawImpl() {
        if (mDownloadManagerImpl.IsValid()) {
            return &(*mDownloadManagerImpl);
        }
        return nullptr;
    }
};

/* Test Case for DownloadManagerImplementation - All IDownloadManager APIs with Plugin Lifecycle
 * 
 * Test all IDownloadManager APIs with proper Initialize/Deinitialize cycle and plugin state management
 * This test demonstrates complete plugin lifecycle and comprehensive API coverage
 */
TEST_F(DownloadManagerImplementationTest, AllIDownloadManagerAPIs) {
    ASSERT_TRUE(mDownloadManagerImpl.IsValid()) << "DownloadManagerImplementation should be created successfully";
    Plugin::DownloadManagerImplementation* impl = getRawImpl();
    ASSERT_NE(impl, nullptr) << "Implementation pointer should be valid";

    // === PHASE 1: PLUGIN INITIALIZATION ===
    // Initialize the plugin - this should succeed with proper mocks
    Core::hresult initResult = impl->Initialize(mServiceMock);
    EXPECT_EQ(Core::ERROR_NONE, initResult) << "Initialize should succeed with proper ServiceMock";

    // Add small delay to ensure thread startup if initialization succeeded
    if (initResult == Core::ERROR_NONE) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // === PHASE 2: DOWNLOAD API TESTING ===
    Exchange::IDownloadManager::Options options;
    options.priority = false;  // Regular priority
    options.retries = 3;       // Retry attempts
    options.rateLimit = 1024;  // Rate limit in KB/s

    string downloadId;

    // Test successful download request (should succeed with INTERNET active)
    Core::hresult downloadResult = impl->Download("http://example.com/test.zip", options, downloadId);
    TEST_LOG("Download (valid URL) returned: %u, downloadId: %s", downloadResult, downloadId.c_str());
    EXPECT_EQ(Core::ERROR_NONE, downloadResult) << "Download should succeed with valid URL and active internet";
    EXPECT_FALSE(downloadId.empty()) << "Download should return valid downloadId";

    // Test download with empty URL - should fail
    string downloadId2;
    Core::hresult downloadResult2 = impl->Download("", options, downloadId2);
    TEST_LOG("Download (empty URL) returned: %u", downloadResult2);
    EXPECT_NE(Core::ERROR_NONE, downloadResult2) << "Download should fail with empty URL";

    // Test download without internet - temporarily disable internet subsystem
    EXPECT_CALL(*mSubSystemMock, IsActive(PluginHost::ISubSystem::INTERNET))
        .WillOnce(::testing::Return(false));

    string downloadId3;
    Core::hresult downloadResult3 = impl->Download("http://example.com/test2.zip", options, downloadId3);
    TEST_LOG("Download (no internet) returned: %u", downloadResult3);
    EXPECT_EQ(Core::ERROR_UNAVAILABLE, downloadResult3) << "Download should fail when internet not available";
    EXPECT_TRUE(downloadId3.empty()) << "DownloadId should be empty on failure";

    // Restore internet for remaining tests
    EXPECT_CALL(*mSubSystemMock, IsActive(PluginHost::ISubSystem::INTERNET))
        .WillRepeatedly(::testing::Return(true));

    // Test priority download - should succeed and return valid downloadId
    Exchange::IDownloadManager::Options priorityOptions;
    priorityOptions.priority = true;   // High priority
    priorityOptions.retries = 3;
    priorityOptions.rateLimit = 1024;

    string priorityDownloadId;
    Core::hresult priorityResult = impl->Download("http://example.com/priority.zip", priorityOptions, priorityDownloadId);
    TEST_LOG("Download (priority) returned: %u, downloadId: %s", priorityResult, priorityDownloadId.c_str());
    EXPECT_EQ(Core::ERROR_NONE, priorityResult) << "Priority download should succeed";
    EXPECT_FALSE(priorityDownloadId.empty()) << "Priority download should return valid downloadId";

    // Test regular download - should succeed and return different downloadId
    string regularDownloadId;
    Core::hresult regularResult = impl->Download("http://example.com/regular.zip", options, regularDownloadId);
    TEST_LOG("Download (regular) returned: %u, downloadId: %s", regularResult, regularDownloadId.c_str());
    EXPECT_EQ(Core::ERROR_NONE, regularResult) << "Regular download should succeed";
    EXPECT_FALSE(regularDownloadId.empty()) << "Regular download should return valid downloadId";

    // Verify downloadIds are different (incremented)
    EXPECT_NE(priorityDownloadId, regularDownloadId) << "Each download should get unique downloadId";

    // === PHASE 3: DOWNLOAD CONTROL APIS ===

    // Test Pause with invalid ID
    Core::hresult pauseResult = impl->Pause("invalid_download_id");
    TEST_LOG("Pause (invalid ID) returned: %u", pauseResult);
    EXPECT_NE(Core::ERROR_NONE, pauseResult) << "Pause should fail with invalid downloadId";

    // Test Pause with empty ID
    Core::hresult pauseResult2 = impl->Pause("");
    TEST_LOG("Pause (empty ID) returned: %u", pauseResult2);
    EXPECT_NE(Core::ERROR_NONE, pauseResult2) << "Pause should fail with empty downloadId";

    // Test Resume with invalid ID
    Core::hresult resumeResult = impl->Resume("invalid_download_id");
    TEST_LOG("Resume (invalid ID) returned: %u", resumeResult);
    EXPECT_NE(Core::ERROR_NONE, resumeResult) << "Resume should fail with invalid downloadId";

    // Test Resume with empty ID  
    Core::hresult resumeResult2 = impl->Resume("");
    TEST_LOG("Resume (empty ID) returned: %u", resumeResult2);
    EXPECT_NE(Core::ERROR_NONE, resumeResult2) << "Resume should fail with empty downloadId";

    // Test Cancel with invalid ID
    Core::hresult cancelResult = impl->Cancel("invalid_download_id");
    TEST_LOG("Cancel (invalid ID) returned: %u", cancelResult);
    EXPECT_NE(Core::ERROR_NONE, cancelResult) << "Cancel should fail with invalid downloadId";

    // Test Cancel with empty ID
    Core::hresult cancelResult2 = impl->Cancel("");
    TEST_LOG("Cancel (empty ID) returned: %u", cancelResult2);
    EXPECT_NE(Core::ERROR_NONE, cancelResult2) << "Cancel should fail with empty downloadId";

    // === PHASE 4: PROGRESS AND STATUS APIs ===

    uint8_t percent = 0;

    // Test Progress with invalid ID
    Core::hresult progressResult = impl->Progress("invalid_download_id", percent);
    TEST_LOG("Progress (invalid ID) returned: %u, percent: %u", progressResult, percent);
    EXPECT_NE(Core::ERROR_NONE, progressResult) << "Progress should fail with invalid downloadId";

    // Test Progress with empty ID
    Core::hresult progressResult2 = impl->Progress("", percent);
    TEST_LOG("Progress (empty ID) returned: %u, percent: %u", progressResult2, percent);
    EXPECT_NE(Core::ERROR_NONE, progressResult2) << "Progress should fail with empty downloadId";

    // === PHASE 5: FILE MANAGEMENT APIS ===

    // Test Delete with invalid file locator
    Core::hresult deleteResult = impl->Delete("nonexistent_file.zip");
    TEST_LOG("Delete (invalid file) returned: %u", deleteResult);
    EXPECT_NE(Core::ERROR_NONE, deleteResult) << "Delete should fail with nonexistent file";

    // Test Delete with empty file locator
    Core::hresult deleteResult2 = impl->Delete("");
    TEST_LOG("Delete (empty locator) returned: %u", deleteResult2);
    EXPECT_NE(Core::ERROR_NONE, deleteResult2) << "Delete should fail with empty file locator";

    // Test GetStorageDetails - should succeed (stub implementation)
    uint32_t quotaKB = 0, usedKB = 0;
    Core::hresult storageResult = impl->GetStorageDetails(quotaKB, usedKB);
    TEST_LOG("GetStorageDetails returned: %u, quota: %u KB, used: %u KB", storageResult, quotaKB, usedKB);
    EXPECT_EQ(Core::ERROR_NONE, storageResult) << "GetStorageDetails should succeed (stub implementation)";

    // Test RateLimit API if it exists - additional coverage
    if (!downloadId.empty()) {
        Core::hresult rateLimitResult = impl->RateLimit(downloadId, 512);
        TEST_LOG("RateLimit (valid ID, 512 KB/s) returned: %u", rateLimitResult);
        // Don't assert on this as it depends on download state

        // Test RateLimit with invalid ID
        Core::hresult rateLimitResult2 = impl->RateLimit("invalid_id", 1024);
        TEST_LOG("RateLimit (invalid ID) returned: %u", rateLimitResult2);
        EXPECT_NE(Core::ERROR_NONE, rateLimitResult2) << "RateLimit should fail with invalid downloadId";
    }

    // === PHASE 6: PLUGIN DEACTIVATION ===
    // Deinitialize will be called automatically in TearDown()
}

/* Test Case: Plugin::DownloadManager APIs
 * Tests plugin creation, Information API, and lifecycle methods
 */
TEST_F(DownloadManagerTest, PluginDownloadManagerAPIs) {
    // Test plugin creation
    ASSERT_TRUE(plugin.IsValid()) << "Plugin should be created successfully";

    // Test interface inheritance
    Plugin::DownloadManager* rawPlugin = &(*plugin);
    ASSERT_NE(rawPlugin, nullptr) << "Raw plugin pointer should be valid";

    PluginHost::JSONRPC* jsonrpcPtr = dynamic_cast<PluginHost::JSONRPC*>(rawPlugin);
    EXPECT_NE(jsonrpcPtr, nullptr) << "Plugin should inherit from PluginHost::JSONRPC";

    PluginHost::IPlugin* pluginPtr = dynamic_cast<PluginHost::IPlugin*>(rawPlugin);
    EXPECT_NE(pluginPtr, nullptr) << "Plugin should inherit from PluginHost::IPlugin";

    // Test Information() API - should always return empty string
    std::string infoResult = plugin->Information();
    EXPECT_TRUE(infoResult.empty()) << "Information() should always return empty string";

    // Test Initialize() method - Returns string (empty = success, non-empty = error message)
    std::string initResult = plugin->Initialize(mServiceMock);
    bool initSucceeded = initResult.empty();

    if (initSucceeded) {
        TEST_LOG("Plugin Initialize: SUCCESS");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    } else {
        TEST_LOG("Plugin Initialize: FAILED (expected in L1) - %s", initResult.c_str());
    }

    // Test Deinitialize() method - returns void, should not crash
    EXPECT_NO_THROW(plugin->Deinitialize(mServiceMock)) << "Deinitialize should not throw exceptions";

    // Allow time for cleanup
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

/* Test Case: Unregister a notification that is no longer in the list
 * Covers: Unregister failure path (lines 92-93 of DownloadManagerImplementation.cpp)
 * Validates that Unregister returns Core::ERROR_GENERAL when the notification is not found.
 *
 * Strategy: Register the notification legitimately (impl calls AddRef, refcount: 1->2),
 * then Unregister it once (success, impl calls Release, refcount: 2->1, removed from list),
 * then attempt a second Unregister - the notification is no longer in the list, so the
 * else branch (LOGERR + ERROR_GENERAL) at lines 92-93 is taken.
 * Finally release our creation reference (refcount: 1->0, delete this).
 */
TEST_F(DownloadManagerImplementationTest, UnregisterNotificationNotFound) {
    Plugin::DownloadManagerImplementation* impl = getRawImpl();
    ASSERT_NE(impl, nullptr) << "Implementation pointer should be valid";

    Core::hresult initResult = impl->Initialize(mServiceMock);
    EXPECT_EQ(Core::ERROR_NONE, initResult) << "Initialize should succeed";

    // Register the notification - impl calls AddRef (refcount: 1->2)
    NotificationTest* notification = new NotificationTest();
    Core::hresult regResult = impl->Register(notification);
    EXPECT_EQ(Core::ERROR_NONE, regResult) << "Register should succeed";

    // First Unregister - succeeds, removes from list, impl calls Release (refcount: 2->1)
    Core::hresult firstUnregResult = impl->Unregister(notification);
    TEST_LOG("First Unregister returned: %u", firstUnregResult);
    EXPECT_EQ(Core::ERROR_NONE, firstUnregResult) << "First Unregister should succeed";

    // Second Unregister - notification no longer in list -> LOGERR + ERROR_GENERAL (lines 92-93)
    Core::hresult secondUnregResult = impl->Unregister(notification);
    TEST_LOG("Second Unregister (not in list) returned: %u", secondUnregResult);
    EXPECT_EQ(Core::ERROR_GENERAL, secondUnregResult) << "Second Unregister should return ERROR_GENERAL";

    // Re-register: impl calls AddRef -> refcount = 2, notification back in list
    Core::hresult reRegResult = impl->Register(notification);
    EXPECT_EQ(Core::ERROR_NONE, reRegResult) << "Re-register should succeed";

    // Drop the caller's creation reference: refcount = 1 (impl still holds its reference)
    // The object is NOT deleted here because refcount > 0.
    notification->Release();
}

/* Test Case: Delete an existing file (success path)
 * Covers: Delete success path (lines 348-349 of DownloadManagerImplementation.cpp)
 * Validates that Delete returns Core::ERROR_NONE when the file exists and no download is active for it.
 */
TEST_F(DownloadManagerImplementationTest, DeleteExistingFile) {
    Plugin::DownloadManagerImplementation* impl = getRawImpl();
    ASSERT_NE(impl, nullptr) << "Implementation pointer should be valid";

    Core::hresult initResult = impl->Initialize(mServiceMock);
    EXPECT_EQ(Core::ERROR_NONE, initResult) << "Initialize should succeed";

    // Create a temporary file that will be deleted by the plugin
    const string tempFilePath = "/tmp/test_dm_delete_target.bin";
    FILE* fp = fopen(tempFilePath.c_str(), "wb");
    ASSERT_NE(fp, nullptr) << "Should be able to create temp file for delete test";
    fclose(fp);

    // Delete the existing file - no active download, so the else branch is taken, remove() succeeds
    Core::hresult result = impl->Delete(tempFilePath);
    TEST_LOG("Delete (existing file) returned: %u", result);
    EXPECT_EQ(Core::ERROR_NONE, result) << "Delete should return ERROR_NONE when file exists and is not actively downloading";
}

/* Test Case: RateLimit when no download is active
 * Covers: RateLimit failure path with null mCurrentDownload (line 418 of DownloadManagerImplementation.cpp)
 * Validates that RateLimit returns Core::ERROR_GENERAL when no download is currently active.
 */
TEST_F(DownloadManagerImplementationTest, RateLimitNoActiveDownload) {
    Plugin::DownloadManagerImplementation* impl = getRawImpl();
    ASSERT_NE(impl, nullptr) << "Implementation pointer should be valid";

    Core::hresult initResult = impl->Initialize(mServiceMock);
    EXPECT_EQ(Core::ERROR_NONE, initResult) << "Initialize should succeed";

    // Call RateLimit with no queued or active download - mCurrentDownload is null
    // The condition (!downloadId.empty() && mCurrentDownload != nullptr && mHttpClient) is false
    // because mCurrentDownload is null, so the else branch (line 418 LOGERR) is taken
    Core::hresult result = impl->RateLimit("3001", 512);
    TEST_LOG("RateLimit (no active download) returned: %u", result);
    EXPECT_EQ(Core::ERROR_GENERAL, result) << "RateLimit should return ERROR_GENERAL when no download is currently active";
}

/* Test Case: Notification reference is released during implementation destruction
 * Covers: Destructor notification iterator cleanup (line 50 of DownloadManagerImplementation.cpp)
 * Registers a notification without unregistering it so the destructor (triggered by TearDown's
 * mDownloadManagerImpl.Release()) must iterate the list and call Release() on each entry.
 */
TEST_F(DownloadManagerImplementationTest, NotificationReleasedOnDestruction) {
    Plugin::DownloadManagerImplementation* impl = getRawImpl();
    ASSERT_NE(impl, nullptr) << "Implementation pointer should be valid";

    Core::hresult initResult = impl->Initialize(mServiceMock);
    EXPECT_EQ(Core::ERROR_NONE, initResult) << "Initialize should succeed";

    // Register a notification - Register calls AddRef on it (refCount goes from 1 to 2)
    NotificationTest* notification = new NotificationTest();
    Core::hresult regResult = impl->Register(notification);
    EXPECT_EQ(Core::ERROR_NONE, regResult) << "Register should succeed";

    // Release our (creation) reference - impl now holds the only reference (refCount = 1)
    // When the impl destructor runs (via TearDown's mDownloadManagerImpl.Release()), it will
    // iterate mDownloadManagerNotification and call Release() on each entry (line 50),
    // dropping the refCount to 0 and deleting the notification object.
    notification->Release();

    // TearDown calls impl->Deinitialize(mServiceMock) then mDownloadManagerImpl.Release(),
    // which triggers ~DownloadManagerImplementation and the cleanup at line 50.
}

/* Test Case: Control APIs (Pause, Resume, Cancel, Progress) with a matching active download ID
 * Also covers: priority queue job selection and DiskError switch case.
 *
 * Covers (DownloadManagerImplementation.cpp):
 *   - Lines 252-253: Pause success path (mHttpClient->pause() + LOGINFO)
 *   - Lines 282-283: Resume success path (mHttpClient->resume() + LOGINFO)
 *   - Lines 312-315: Cancel success path (mCurrentDownload->cancel() + mHttpClient->cancel())
 *   - Lines 370-371: Progress success path (mHttpClient->getProgress() + LOGINFO)
 *   - Line 342:      Delete in-progress warning (LOGWARN when file is actively downloading)
 *   - Lines 579-581: Priority queue job selection log in pickDownloadJob
 *   - Lines 521-524: DiskError switch case when cancel aborts the curl transfer
 *
 * Covers (DownloadManagerHttpClient.h inline functions):
 *   - pause(), resume(), cancel(), getProgress()
 *
 * Strategy: Queue a single priority download so the thread picks it up immediately via the
 * priority queue. Sleep briefly after queuing to give the downloader thread time to call
 * pickDownloadJob (logging line 579) and enter curl_easy_perform. Then call Pause, Resume,
 * Progress, Delete (with the download's file locator), and Cancel using the exact downloadId
 * returned by Download(). Cancel signals bCancel=true; the next progressCb invocation
 * returns non-zero, which causes curl to abort with CURLE_WRITE_ERROR -> DiskError status
 * -> lines 521-524 are reached in the switch statement.
 */
TEST_F(DownloadManagerImplementationTest, ActiveDownloadControlAndDeleteInProgress) {
    Plugin::DownloadManagerImplementation* impl = getRawImpl();
    ASSERT_NE(impl, nullptr) << "Implementation pointer should be valid";

    Core::hresult initResult = impl->Initialize(mServiceMock);
    EXPECT_EQ(Core::ERROR_NONE, initResult) << "Initialize should succeed";

    // Queue a single high-priority download. The downloader thread wakes immediately and
    // calls pickDownloadJob(), which takes from the priority queue (line 579 log).
    Exchange::IDownloadManager::Options priorityOptions;
    priorityOptions.priority = true;
    priorityOptions.retries = 1;
    priorityOptions.rateLimit = 512;

    string activeDownloadId;
    Core::hresult dlResult = impl->Download("http://example.com/active_test.zip", priorityOptions, activeDownloadId);
    EXPECT_EQ(Core::ERROR_NONE, dlResult) << "Download should be queued successfully";
    EXPECT_FALSE(activeDownloadId.empty()) << "Should receive a valid downloadId";
    TEST_LOG("Priority download queued with id: %s", activeDownloadId.c_str());

    // Construct the expected file locator (mDownloadPath + "package" + id)
    // mDownloadPath is "/tmp/downloads/" from the fixture ConfigLine
    const string expectedFileLocator = string("/tmp/downloads/package") + activeDownloadId;

    // Allow the downloader thread time to call pickDownloadJob (setting mCurrentDownload)
    // and enter curl_easy_perform so mCurrentDownload is non-null and matches activeDownloadId
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // --- Pause with the matching active download ID ---
    // If mCurrentDownload == activeDownloadId: calls mHttpClient->pause() (lines 252-253)
    // If download already finished: returns ERROR_GENERAL (acceptable - timing-dependent)
    Core::hresult pauseResult = impl->Pause(activeDownloadId);
    TEST_LOG("Pause (matching id=%s) returned: %u", activeDownloadId.c_str(), pauseResult);
    // Accept both outcomes; when the match path is taken, pauseResult == ERROR_NONE
    if (Core::ERROR_NONE == pauseResult) {
        TEST_LOG("Pause matched active download - success path covered");
    }

    // --- Resume with the matching active download ID ---
    // If mCurrentDownload == activeDownloadId: calls mHttpClient->resume() (lines 282-283)
    Core::hresult resumeResult = impl->Resume(activeDownloadId);
    TEST_LOG("Resume (matching id=%s) returned: %u", activeDownloadId.c_str(), resumeResult);
    if (Core::ERROR_NONE == resumeResult) {
        TEST_LOG("Resume matched active download - success path covered");
    }

    // --- Progress with the matching active download ID ---
    // If mCurrentDownload == activeDownloadId: reads mHttpClient->getProgress() (lines 370-371)
    uint8_t percent = 0xFF;
    Core::hresult progressResult = impl->Progress(activeDownloadId, percent);
    TEST_LOG("Progress (matching id=%s) returned: %u, percent: %u", activeDownloadId.c_str(), progressResult, percent);
    if (Core::ERROR_NONE == progressResult) {
        TEST_LOG("Progress matched active download - success path covered, percent: %u", percent);
        EXPECT_LE(percent, 100) << "Progress percent should be 0-100 when active";
    }

    // --- Delete with the in-progress file locator ---
    // If mCurrentDownload is active AND fileLocator matches: LOGWARN (line 342)
    // Otherwise: remove() is attempted on the file locator
    Core::hresult deleteInProgressResult = impl->Delete(expectedFileLocator);
    TEST_LOG("Delete (in-progress locator=%s) returned: %u", expectedFileLocator.c_str(), deleteInProgressResult);
    // When the match is found, result stays ERROR_GENERAL (LOGWARN path, no result assignment)
    // When not matching, remove() on a non-existent file returns ENOENT -> ERROR_GENERAL as well

    // --- Cancel with the matching active download ID ---
    // If mCurrentDownload == activeDownloadId: sets isCancelled=true and calls mHttpClient->cancel()
    // (lines 312-315). bCancel=true causes progressCb to return non-zero -> curl CURLE_WRITE_ERROR
    // -> DiskError status -> switch case DISK_PERSISTENCE_FAILURE (lines 521-524)
    Core::hresult cancelResult = impl->Cancel(activeDownloadId);
    TEST_LOG("Cancel (matching id=%s) returned: %u", activeDownloadId.c_str(), cancelResult);
    if (Core::ERROR_NONE == cancelResult) {
        TEST_LOG("Cancel matched active download - success path covered");
    }

    // Allow time for the downloader thread to process the cancellation and complete the job
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
}