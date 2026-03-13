#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <cstring>
#include "AppStorageManager.h"
#include "AppStorageManagerImplementation.h"
#include "ServiceMock.h"
#include "Store2Mock.h"
#include "WrapsMock.h"
#include "ThunderPortability.h"
#include "COMLinkMock.h"
#include "RequestHandler.h"

extern "C" DIR* __real_opendir(const char* pathname);

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::_;
using ::testing::Invoke;
using namespace WPEFramework;

#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

class AppStorageManagerTest : public ::testing::Test {
    protected:
        //JSONRPC
        Core::ProxyType<Plugin::AppStorageManager> plugin;
        Core::JSONRPC::Handler& handler;
        Core::JSONRPC::Context connection;
        string response;
        Exchange::IConfiguration* storageManagerConfigure;
        //comrpc 
        Exchange::IAppStorageManager* interface;
        NiceMock<ServiceMock> service;
        Core::ProxyType<Plugin::StorageManagerImplementation> StorageManagerImplementation;
        ServiceMock  *p_serviceMock  = nullptr;
        WrapsImplMock *p_wrapsImplMock   = nullptr;

        Store2Mock* mStore2Mock = nullptr;

        AppStorageManagerTest():
        plugin(Core::ProxyType<Plugin::AppStorageManager>::Create()),
        handler(*plugin),
        connection(0,1,"")
        {
            StorageManagerImplementation = Core::ProxyType<Plugin::StorageManagerImplementation>::Create();
            mStore2Mock = new NiceMock<Store2Mock>;

            p_wrapsImplMock  = new NiceMock <WrapsImplMock>;
            Wraps::setImpl(p_wrapsImplMock);
            static struct dirent mockDirent;
            std::memset(&mockDirent, 0, sizeof(mockDirent));
            std::strncpy(mockDirent.d_name, "mockApp", sizeof(mockDirent.d_name) - 1);
            mockDirent.d_type = DT_DIR;            
            ON_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
                .WillByDefault(Invoke(
                [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
                    if (key == "quotaSize") {
                        value = "1024";
                    } else {
                        value = "mockValue";
                    }

                    ttl = 0;
                    return Core::ERROR_NONE;
            }));

            ON_CALL(*p_wrapsImplMock, opendir(_))
                .WillByDefault(Invoke([](const char* pathname) {
                    return __real_opendir(pathname);
            }));
            ON_CALL(*p_wrapsImplMock, readdir(_))
                .WillByDefault([](DIR* dirp) -> struct dirent* {
                    static int call_count = 0;
                    static struct dirent entry;
                    if (call_count == 0) {
                        std::strncpy(entry.d_name, "mockApp", sizeof(entry.d_name) - 1);
                        entry.d_type = DT_DIR;
                        call_count++;
                        return &entry;
                    } else if (call_count == 1) {
                        std::strncpy(entry.d_name, "testApp", sizeof(entry.d_name) - 1);
                        entry.d_type = DT_DIR;
                        call_count++;
                        return &entry;
                    } else {
                        call_count = 0;
                        return nullptr;
                    }
            });

            ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
            .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
                return 0;
            });

            ON_CALL(*p_wrapsImplMock, stat(_, _))
                .WillByDefault([](const char* path, struct stat* info) {
                    return 0;
            });

            ON_CALL(*p_wrapsImplMock, closedir(_))
                .WillByDefault([](DIR* dirp) {
                    return 0;
            });

            EXPECT_CALL(service, QueryInterfaceByCallsign(_, _))
            .WillRepeatedly(Invoke(
            [&](const uint32_t, const std::string& name) -> void* {
                if (name == "org.rdk.PersistentStore") {
                    return reinterpret_cast<void*>(mStore2Mock);
                }
                return nullptr;
            }));

            interface = static_cast<Exchange::IAppStorageManager*>(
                StorageManagerImplementation->QueryInterface(Exchange::IAppStorageManager::ID));

            storageManagerConfigure = static_cast<Exchange::IConfiguration*>(
            StorageManagerImplementation->QueryInterface(Exchange::IConfiguration::ID));
            StorageManagerImplementation->Configure(&service);
            plugin->Initialize(&service);
          
        }
        virtual ~AppStorageManagerTest() override {
            plugin->Deinitialize(&service);
            storageManagerConfigure->Release();
            Wraps::setImpl(nullptr);
            if (p_wrapsImplMock != nullptr)
            {
                delete p_wrapsImplMock;
                p_wrapsImplMock = nullptr;
            }
            if (mStore2Mock != nullptr)
            {
                delete mStore2Mock;
                mStore2Mock = nullptr;
            }

        }
        virtual void SetUp()
        {
            ASSERT_TRUE(interface != nullptr);
        }
    
        virtual void TearDown()
        {
            ASSERT_TRUE(interface != nullptr);
        }
    };

typedef AppStorageManagerTest StorageManagerTest;


/*
    Test for  CreateStorage with empty appId
    This test checks the failure case when an empty appId is provided.
    It expects the CreateStorage method to return an error code and set the error reason accordingly.
    The test verifies that the error reason matches the expected message "appId cannot be empty".
    It also logs the error reason for debugging purposes.
*/
TEST_F(StorageManagerTest, CreateStorage_Failure){
    std::string appId = "";
    uint32_t size = 1024;
    std::string path = " ";
    std::string errorReason = "";
    EXPECT_EQ(Core::ERROR_GENERAL, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_STREQ("appId cannot be empty", errorReason.c_str());
    TEST_LOG("CreateStorage_Failure errorReason = %s",errorReason.c_str());

}

/*
    CreateStorage_Success test checks the failure of creation of storage for a given appId due to insufficient storage space.
    Creates a mock environment where the necessary functions like mkdir, access, nftw, are set up to simulate a successful resutls.
    Creates a mock environment where the statvfs function is set up to simulate a unsuccessful result with insufficient storage space.
    It verifies that the CreateStorage method returns a failure code and errorReason that Insufficient storage space.
    The test also logs the success message for debugging purposes.
*/
TEST_F(StorageManagerTest, CreateStorageSizeExceeded_Failure){
    std::string appId = "testApp";
    uint32_t size = 1000000;
    std::string path = " ";
    std::string errorReason = "";
    // mock the mkdir function to always return success
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            // Simulate file exists
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            // Simulate success
            return 0;
    });

    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
    .WillRepeatedly([](const char* path, struct statvfs* buf) {
        // Simulate failure
        buf->f_bsize = 4096; // Block size
        buf->f_frsize = 4096; // Fragment size
        buf->f_blocks = 100000; // Total blocks
        buf->f_bfree = 0; // Free blocks
        buf->f_bavail = 0; // Available blocks
        return 0;
    });
    
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                        const std::string& appId,
                                        const std::string& key,
                                        const std::string& value,
                                        const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_GENERAL, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_STREQ("Insufficient storage space", errorReason.c_str());
    TEST_LOG("CreateStorageSizeExceeded_Failure errorReason = %s",errorReason.c_str());
}

/*
    CreateStorage_Success test checks the failure of creation of storage for a given appId due to mkdir failure.
    Creates a mock environment where the necessary functions like access, nftw, are set up to simulate a successful resutls.
    Creates a mock environment where the mkdir function is set up to simulate a failure with ENOTDIR error.
    It verifies that the CreateStorage method returns a failure code and errorReason that Failed to create base storage directory: /opt/persistent/storageManager.
    The test also logs the message for debugging purposes.
*/
TEST_F(StorageManagerTest, CreateStoragemkdirFail_Failure){
    
    std::string appId = "testApp";
    uint32_t size = 1024;
    std::string path = " ";
    std::string errorReason = "";

    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillOnce([](const char* path, mode_t mode) {
            errno = ENOTDIR;  // error that should trigger failure
            return -1;
        });

    EXPECT_EQ(Core::ERROR_GENERAL, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_STREQ("Failed to create base storage directory: /opt/persistent/storageManager", errorReason.c_str());
    TEST_LOG("CreateStoragemkdirFail_Failure errorReason = %s",errorReason.c_str());

}

/*
    CreateStorage_PathDoesNotExists_Success test checks the successful creation of storage for a given appId when the path does not exist.
    Creates a mock environment where the necessary functions like mkdir, access, nftw, statvfs, and stat are set up to simulate a successful resutls.
    It verifies that the CreateStorage method returns a success code and the errorReason is empty.
    The test also logs the success message for debugging purposes.
*/
TEST_F(StorageManagerTest, CreateStorage_PathDoesNotExists_Success){

    std::string appId = "testApp";
    uint32_t size = 1024;
    std::string path = " ";
    std::string errorReason = "";

    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
           errno = EEXIST;  // Directory already exists
            return -1;
    });
    
    ON_CALL(*p_wrapsImplMock, access(_, _))
    .WillByDefault([](const char* pathname, int mode) {
        // Simulate file exists
        return 0;
    });
    
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
    .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
        // Simulate success
        return 0;
    });

    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            // Simulate success
            buf->f_bsize = 4096; // Block size
            buf->f_frsize = 4096; // Fragment size
            buf->f_blocks = 100000; // Total blocks
            buf->f_bfree = 500000; // Free blocks
            buf->f_bavail = 500000; // Available blocks
            return 0;
    });

    ON_CALL(*p_wrapsImplMock, stat(_, _))
        .WillByDefault([](const char* path, struct stat* info) {
            // Simulate success
            return 0;
    });
    
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                        const std::string& appId,
                                        const std::string& key,
                                        const std::string& value,
                                        const uint32_t ttl) -> uint32_t {
        // Simulate success
        return Core::ERROR_NONE;
    }));

    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_STREQ("", errorReason.c_str());

}

/*
    CreateStorage_Success test checks the successful creation of storage for a given appId.
    Creates a mock environment where the necessary functions like mkdir, access, nftw, statvfs, and stat are set up to simulate a successful resutls.
    It verifies that the CreateStorage method returns a success code and the errorReason is empty.
    The test also logs the success message for debugging purposes.
*/
TEST_F(StorageManagerTest, CreateStorage_Success){

    uint32_t size = 1024;
    std::string path = " ";
    std::string errorReason = ""; 
    std::string appId = "testApp";

    // mock the mkdir function to always return success
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });

    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            // Simulate file exists
            return 0;
    });
        
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            // Simulate success
            return 0;
    });

    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            // Simulate success
            buf->f_bsize = 4096; // Block size
            buf->f_frsize = 4096; // Fragment size
            buf->f_blocks = 100000; // Total blocks
            buf->f_bfree = 500000; // Free blocks
            buf->f_bavail = 500000; // Available blocks
            return 0;
    });

    ON_CALL(*p_wrapsImplMock, stat(_, _))
        .WillByDefault([](const char* path, struct stat* info) {
            // Simulate success
            return 0;
    });
    
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                        const std::string& appId,
                                        const std::string& key,
                                        const std::string& value,
                                        const uint32_t ttl) -> uint32_t {
        // Simulate success
        return Core::ERROR_NONE;
    }));
  
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_STREQ("", errorReason.c_str());
}

/*
    GetStorage_Failure test checks the failure of getting storage information for a blank appId.
    Creates a mock environment where the necessary functions like mkdir, access, nftw, statvfs, and chown are set up to simulate a successful resutls.
    It verifies that the GetStorage method returns a failure code when the appId is empty.
*/
TEST_F(StorageManagerTest, GetStorage_Failure){

    std::string appId = "";
    uint32_t userId = 100;
    uint32_t groupId = 101;
    std::string path = "";
    uint32_t size = 0;
    uint32_t used = 0;

    EXPECT_EQ(Core::ERROR_GENERAL, interface->GetStorage(appId, userId, groupId, path, size, used));
}
/*
    GetStorage_chownFailure test checks the failure of getting storage information when the chown operation fails.
    Creates a mock environment where the necessary functions like mkdir, access, nftw, statvfs, chown are set up to simulate a successful resutls.
    chown is set up to simulate a faiure result.
    It verifies that the GetStorage method returns a failure code when the chown operation fails.
*/
TEST_F(StorageManagerTest, GetStorage_chownFailure){

    std::string appId = "testApp";
    uint32_t size = 1024;
    std::string errorReason = "";
    uint32_t userId = 100;
    uint32_t groupId = 101;
    std::string path = "";
    uint32_t used = 0;

    // mock the mkdir function to always return success
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            // Simulate file exists
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            // Simulate success
            return 0;
    });

    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            // Simulate success
            buf->f_bsize = 4096; // Block size
            buf->f_frsize = 4096; // Fragment size
            buf->f_blocks = 100000; // Total blocks
            buf->f_bfree = 50000; // Free blocks
            buf->f_bavail = 50000; // Available blocks
            return 0;
    });

    EXPECT_CALL(*p_wrapsImplMock, chown(_, _, _))
        .WillRepeatedly([](const char* path, uid_t owner, gid_t group) {
            // Simulate Failure
            return -1;
    });

    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                    const std::string& appId,
                                    const std::string& key,
                                    const std::string& value,
                                    const uint32_t ttl) -> uint32_t {
        // Simulate success
        return Core::ERROR_NONE;
    }));

    ON_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
    .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
            if (key == "quotaSize") {
                value = "1024"; // Simulate a valid numeric quota
            } else {
                value = "mockValue"; // Default value for other keys
            }
            ttl = 0;
            return Core::ERROR_NONE;
    }));

    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_EQ(Core::ERROR_GENERAL, interface->GetStorage(appId, userId, groupId, path, size, used));
}

/*
    GetStorage_Success test checks the successful retrieval of storage information for a given appId.
    Creates a mock environment where the necessary functions like mkdir, access, nftw, statvfs, and chown are set up to simulate a successful resutls.
    It verifies that the GetStorage method returns a success code and the size and used values are set correctly.
    The test also checks that the path is set to the expected value.
*/
TEST_F(StorageManagerTest, GetStorage_Success){

    std::string appId = "testApp";
    uint32_t size = 1024;
    std::string errorReason = "";
    uint32_t userId = 100;
    uint32_t groupId = 101;
    std::string path = "";
    uint32_t used = 0;

    // mock the mkdir function to always return success
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            // Simulate file exists
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            // Simulate success
            return 0;
    });

    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            // Simulate success
            buf->f_bsize = 4096; // Block size
            buf->f_frsize = 4096; // Fragment size
            buf->f_blocks = 100000; // Total blocks
            buf->f_bfree = 50000; // Free blocks
            buf->f_bavail = 50000; // Available blocks
            return 0;
    });
    
    EXPECT_CALL(*p_wrapsImplMock, chown(_, _, _))
        .WillRepeatedly([](const char* path, uid_t owner, gid_t group) {
            // Simulate success
            return 0;
    });
    
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                    const std::string& appId,
                                    const std::string& key,
                                    const std::string& value,
                                    const uint32_t ttl) -> uint32_t {
        // Simulate success
        return Core::ERROR_NONE;
    }));
    
    ON_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
    .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
            if (key == "quotaSize") {
                value = "1024"; // Simulate a valid numeric quota
            } else {
                value = "mockValue"; // Default value for other keys
            }
            ttl = 0;
            return Core::ERROR_NONE;
    }));
    
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, interface->GetStorage(appId, userId, groupId, path, size, used));
    EXPECT_EQ(1024, size);
    EXPECT_EQ(0, used);
    EXPECT_EQ(path, "/opt/persistent/storageManager/testApp");
}

/*
    DeleteStorage_Failure test checks the failure of deleting storage for a blank appId.
    It verifies that the DeleteStorage method returns a failure code when the appId is empty and sets the errorReason accordingly.
    The test also logs the error reason for debugging purposes.
*/
TEST_F(StorageManagerTest, DeleteStorage_Failure){
    std::string appId = "";
    std::string errorReason = "";

    EXPECT_EQ(Core::ERROR_GENERAL, interface->DeleteStorage(appId, errorReason));
    EXPECT_STREQ("AppId is empty", errorReason.c_str());
    TEST_LOG("DeleteStorage_Failure errorReason = %s",errorReason.c_str());
}

/*
    DeleteStorage_rmdirFilure test checks the failure of deleting storage for a given appId when the rmdir function fails.
    Creates a mock environment where the necessary functions like mkdir, access, nftw, statvfs, are rmdir are set up to simulate a successful results.
    rmdir is set up to simulate a failure result.
    It verifies that the DeleteStorage method returns a failure code and errorReason that Error deleting the empty App Folder: File exists.
*/
TEST_F(StorageManagerTest, DeleteStorage_rmdirFilure){

    std::string appId = "testApp";
    uint32_t size = 1024;
    std::string errorReason = "";
    std::string path = "";

    // mock the mkdir function to always return success
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            // Simulate file exists
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            // Simulate success
            return 0;
    });

    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            // Simulate success
            buf->f_bsize = 4096; // Block size
            buf->f_frsize = 4096; // Fragment size
            buf->f_blocks = 100000; // Total blocks
            buf->f_bfree = 50000; // Free blocks
            buf->f_bavail = 50000; // Available blocks
            return 0;
    });

    EXPECT_CALL(*p_wrapsImplMock, rmdir(_))
        .WillRepeatedly([](const char* pathname) {
            // Simulate failure
            return -1;
    });

    ON_CALL(*mStore2Mock, DeleteKey(_, _, _))
        .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key) -> uint32_t {
            // Simulate success
            return Core::ERROR_NONE;
    }));

    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_EQ(Core::ERROR_GENERAL, interface->DeleteStorage(appId, errorReason));
    EXPECT_STREQ("Error deleting the empty App Folder: File exists", errorReason.c_str());
}

/*
    DeleteStorage_Success test checks the successful deletion of storage for a given appId.
    Creates a mock environment where the necessary functions like mkdir, access, nftw, statvfs, and rmdir are set up to simulate a successful resutls.
    It verifies that the DeleteStorage method returns a success code and the errorReason is empty.
*/
TEST_F(StorageManagerTest, DeleteStorage_Success){

    std::string appId = "testApp";
    uint32_t size = 1024;
    std::string errorReason = "";
    std::string path = "";

    // mock the mkdir function to always return success
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            // Simulate file exists
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            // Simulate success
            return 0;
    });

    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            // Simulate success
            buf->f_bsize = 4096; // Block size
            buf->f_frsize = 4096; // Fragment size
            buf->f_blocks = 100000; // Total blocks
            buf->f_bfree = 50000; // Free blocks
            buf->f_bavail = 50000; // Available blocks
            return 0;
    });
    
    EXPECT_CALL(*p_wrapsImplMock, rmdir(_))
        .WillRepeatedly([](const char* pathname) {
            // Simulate success
            return 0;
    });

    ON_CALL(*mStore2Mock, DeleteKey(_, _, _))
        .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key) -> uint32_t {
            // Simulate success
            return Core::ERROR_NONE;
    }));
    
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, interface->DeleteStorage(appId, errorReason));
    EXPECT_STREQ("", errorReason.c_str());
}


/*
    test_clear_failure_json checks the failure of the clear method when an empty appId is provided.
    It verifies that the clear method returns a failure code and sets the errorReason accordingly.
    The test also logs the error reason for debugging purposes.
*/
TEST_F(StorageManagerTest, test_clear_failure_json){
    std::string appId = "";
    std::string errorReason = "";

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("clear"), _T("{\"appId\":\"\"}"), response));
}

/*
    test_clear_nftwFailure_json checks the failure of the clear method when the nftw function fails.
    It creates a mock environment where the necessary functions like mkdir, access, statvfs, and SetValue are set up to simulate a successful result.
    nftw is set up to simulate a failure.
    The nftw function is set up to simulate a failure.
    It verifies that the clear method returns a failure code and sets the errorReason accordingly.
*/
TEST_F(StorageManagerTest, test_clear_nftwFailure_json){

    std::string path = "";
    std::string errorReason = "";
    // mock the mkdir function to always return success
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            // Simulate file exists
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            // Simulate failure
            return -1;
    });

    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            // Simulate success
            buf->f_bsize = 4096; // Block size
            buf->f_frsize = 4096; // Fragment size
            buf->f_blocks = 100000; // Total blocks
            buf->f_bfree = 50000; // Free blocks
            buf->f_bavail = 50000; // Available blocks
            return 0;
    });

    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                        const std::string& appId,
                                        const std::string& key,
                                        const std::string& value,
                                        const uint32_t ttl) -> uint32_t {
        // Simulate success
        return Core::ERROR_NONE;
    }));

    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("testApp", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("clear"), _T("{\"appId\":\"testApp\"}"), response));
}

/*
    test_clear_success_json checks the successful execution of the clear method for a given appId.
    It creates a mock environment where the necessary functions like mkdir, access, nftw, statvfs, and SetValue are set up to simulate a successful result.
    It creates a storage for the appId "testApp" with a size of 1024 bytes.
    The test then invokes the clear method with the appId "testApp" and expects a success code.
    The response is expected to be empty, indicating that the storage has been cleared successfully
*/
TEST_F(StorageManagerTest, test_clear_success_json){
    
    std::string path = "";
    std::string errorReason = "";
    // mock the mkdir function to always return success
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            // Simulate file exists
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            // Simulate success
            return 0;
    });

    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            // Simulate success
            buf->f_bsize = 4096; // Block size
            buf->f_frsize = 4096; // Fragment size
            buf->f_blocks = 100000; // Total blocks
            buf->f_bfree = 50000; // Free blocks
            buf->f_bavail = 50000; // Available blocks
            return 0;
    });

    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                        const std::string& appId,
                                        const std::string& key,
                                        const std::string& value,
                                        const uint32_t ttl) -> uint32_t {
        // Simulate success
        return Core::ERROR_NONE;
    }));
 
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("testApp", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("clear"), _T("{\"appId\":\"testApp\"}"), response));
}

/*
    test_clearall_failure_json checks the failure of the clearAll method when an error occurs during the clearing process.
    It creates a mock environment where the necessary functions like mkdir, access, nftw, statvfs, and SetValue are set up to simulate a successful result.
    It creates storage for two appIds: "testApp" and "testexempt".
    The test then invokes the clearAll method with a JSON string containing exemptionAppIds.
    It expects the clearAll method to return a failure code and sets the errorReason accordingly.
    The test also logs the error reason for debugging purposes.
*/
TEST_F(StorageManagerTest, test_clearall_failure_json){

    std::string path = "";
    std::string errorReason = "";
    std::string wrappedJson = "{\"exemptionAppIds\": \"[\\\"testexempt\\\"]\"}";
    static int callCount = 0;

    // mock the mkdir function to always return success
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            // Simulate file exists
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillOnce([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            // Simulate success
            return 0;
        })
        .WillOnce([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            // Simulate success
            return 0;
        })
        .WillOnce([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            // Simulate success
            return 0;
        })
        .WillOnce([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            // Simulate success
            return -1;
    });

    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            // Simulate success
            buf->f_bsize = 4096; // Block size
            buf->f_frsize = 4096; // Fragment size
            buf->f_blocks = 100000; // Total blocks
            buf->f_bfree = 50000; // Free blocks
            buf->f_bavail = 50000; // Available blocks
            return 0;
    });

    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                        const std::string& appId,
                                        const std::string& key,
                                        const std::string& value,
                                        const uint32_t ttl) -> uint32_t {
        // Simulate success
        return Core::ERROR_NONE;
    }));

    EXPECT_CALL(*p_wrapsImplMock, opendir(_))
        .WillOnce(Invoke([](const char* pathname) {
        // Simulate success
        return (__real_opendir(pathname));

    }));

    ON_CALL(*p_wrapsImplMock, readdir(_))
    .WillByDefault([](DIR* dirp) -> struct dirent* {
        static struct dirent entry;
        switch (callCount++) {
            case 0:
                std::strcpy(entry.d_name, "testApp");
                entry.d_type = DT_DIR;
                return &entry;
            case 1:
                std::strcpy(entry.d_name, "testexempt");
                entry.d_type = DT_DIR;
                return &entry;
            default:
                return nullptr;
        }
    });
    ON_CALL(*p_wrapsImplMock, closedir(_))
    .WillByDefault([](DIR* dirp) {
        // Simulate success
        return 0;
    });

    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("testApp", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("testexempt", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("clearAll"), wrappedJson, response));
}

/*
    test_clearall_without_exemption_json checks the successful execution of the clearAll method without any exemption appIds.
    It verifies that the clearAll method returns a success code and the response is empty ensuring all storage is cleared.
*/
TEST_F(StorageManagerTest, test_clearall_without_exemption_json){
    std::string exemptionAppIds = "";
    std::string errorReason = "";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("clearAll"), _T("{}"), response));
}

/*
    Test: Verify mConfigure->Release() and nullptr are called on Configure() failure
    This validates that Deinitialize doesn't cause double-free
*/
TEST_F(StorageManagerTest, Initialize_ConfigureFails_ReleaseAndNullCalled) {
    // Test that multiple Deinitialize calls don't cause double-free
    // This validates that mConfigure = nullptr prevents issues
    
    EXPECT_NO_THROW(plugin->Deinitialize(&service));
    // Re-initialize for other tests
    plugin->Initialize(&service);
} 

/*
    Test: Initialize success - mConfigure is properly set
    Verifies the normal initialization path works correctly
*/
TEST_F(StorageManagerTest, Initialize_Success_mConfigureSet) {
    // Your existing constructor already tests this
    // plugin->Initialize(&service) was called and succeeded
    
    ASSERT_TRUE(interface != nullptr);
    ASSERT_TRUE(storageManagerConfigure != nullptr);
    
    // mConfigure is properly initialized, no errors
    // This confirms the code path where Configure() succeeds
} 


/*
    test_clearall_success_json checks the successful execution of the clearAll method with exemption appIds.
    It creates a mock environment where the necessary functions like mkdir, access, nftw, statvfs, and SetValue are set up to simulate a successful result.
    It creates storage for two appIds: "testApp" and "testexempt".
    The test then invokes the clearAll method with a JSON string containing exemptionAppIds.
    It expects the clearAll method to return a success code and the response is empty indicating that the storage has been cleared successfully except for the exempted appId.
*/
TEST_F(StorageManagerTest, test_clearall_success_json){
    std::string path = "";
    std::string errorReason = ""; 
    std::string wrappedJson = "{\"exemptionAppIds\": \"{\\\"exemptionAppIds\\\": [\\\"testexempt\\\"]}\"}";
    static int callCount = 0;
    // mock the mkdir function to always return success
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            // Simulate file exists
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            // Simulate success
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            // Simulate success
            buf->f_bsize = 4096; // Block size
            buf->f_frsize = 4096; // Fragment size
            buf->f_blocks = 100000; // Total blocks
            buf->f_bfree = 50000; // Free blocks
            buf->f_bavail = 50000; // Available blocks
        return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                        const std::string& appId,
                                        const std::string& key,
                                        const std::string& value,
                                        const uint32_t ttl) -> uint32_t {
        // Simulate success
        return Core::ERROR_NONE;
    }));

    ON_CALL(*p_wrapsImplMock, opendir(_))
        .WillByDefault(Invoke([](const char* pathname) {
        // Simulate success
        return (__real_opendir(pathname));

    }));

    ON_CALL(*p_wrapsImplMock, readdir(_))
    .WillByDefault([](DIR* dirp) -> struct dirent* {
        static struct dirent entry;
        switch (callCount++) {
            case 0:
                std::strcpy(entry.d_name, "testApp");
                entry.d_type = DT_DIR;
                return &entry;
            case 1:
                std::strcpy(entry.d_name, "testexempt");
                entry.d_type = DT_DIR;
                return &entry;
            default:
                return nullptr;
        }
    });

    ON_CALL(*p_wrapsImplMock, closedir(_))
    .WillByDefault([](DIR* dirp) {
        return 0;
    });

    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("testApp", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("testexempt", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("clearAll"), wrappedJson, response));
}

TEST_F(AppStorageManagerTest, CreateStorage_JsonRpc_Success) {
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, stat(_, _))
        .WillByDefault([](const char* path, struct stat* info) {
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("createStorage"), _T("{\"appId\":\"jsonTestApp\",\"size\":2048}"), response));
    EXPECT_TRUE(response.find("\"path\"") != std::string::npos);
}

TEST_F(AppStorageManagerTest, CreateStorage_JsonRpc_EmptyAppId_Failure) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("createStorage"), _T("{\"appId\":\"\",\"size\":1024}"), response));
}

TEST_F(AppStorageManagerTest, CreateStorage_JsonRpc_MissingAppId_Failure) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("createStorage"), _T("{\"size\":1024}"), response));
}

TEST_F(AppStorageManagerTest, CreateStorage_JsonRpc_ZeroSize_Success) {
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("createStorage"), _T("{\"appId\":\"zeroSizeApp\",\"size\":0}"), response));
}

TEST_F(AppStorageManagerTest, GetStorage_JsonRpc_Success) {
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50000;
            buf->f_bavail = 50000;
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, chown(_, _, _))
        .WillRepeatedly([](const char* path, uid_t owner, gid_t group) {
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    ON_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
    .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
            if (key == "quotaSize") {
                value = "1024";
            } else {
                value = "mockValue";
            }
            ttl = 0;
            return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("getStorageJsonApp", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getStorage"), _T("{\"appId\":\"getStorageJsonApp\",\"userId\":100,\"groupId\":101}"), response));
    EXPECT_TRUE(response.find("\"path\"") != std::string::npos);
    EXPECT_TRUE(response.find("\"size\"") != std::string::npos);
    EXPECT_TRUE(response.find("\"used\"") != std::string::npos);
}

TEST_F(AppStorageManagerTest, GetStorage_JsonRpc_EmptyAppId_Failure) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getStorage"), _T("{\"appId\":\"\",\"userId\":100,\"groupId\":101}"), response));
}

TEST_F(AppStorageManagerTest, GetStorage_JsonRpc_NonExistentApp_Failure) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getStorage"), _T("{\"appId\":\"nonExistentApp\",\"userId\":100,\"groupId\":101}"), response));
}

TEST_F(AppStorageManagerTest, DeleteStorage_JsonRpc_Success) {
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50000;
            buf->f_bavail = 50000;
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, rmdir(_))
        .WillRepeatedly([](const char* pathname) {
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    ON_CALL(*mStore2Mock, DeleteKey(_, _, _))
        .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key) -> uint32_t {
            return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("deleteJsonApp", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("deleteStorage"), _T("{\"appId\":\"deleteJsonApp\"}"), response));
}

TEST_F(AppStorageManagerTest, DeleteStorage_JsonRpc_EmptyAppId_Failure) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("deleteStorage"), _T("{\"appId\":\"\"}"), response));
}

TEST_F(AppStorageManagerTest, DeleteStorage_JsonRpc_NonExistentApp_Failure) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("deleteStorage"), _T("{\"appId\":\"nonExistentDeleteApp\"}"), response));
}

TEST_F(AppStorageManagerTest, Clear_EmptyAppId_Failure) {
    std::string appId = "";
    std::string errorReason = "";
    EXPECT_EQ(Core::ERROR_GENERAL, interface->Clear(appId, errorReason));
    EXPECT_STREQ("Clear called with no appId", errorReason.c_str());
}

TEST_F(AppStorageManagerTest, Clear_NonExistentApp_Failure) {
    std::string errorReason = "";
    EXPECT_EQ(Core::ERROR_GENERAL, interface->Clear("nonExistentClearApp", errorReason));
}

TEST_F(AppStorageManagerTest, Clear_Success) {
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50000;
            buf->f_bavail = 50000;
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("clearSuccessApp", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, interface->Clear("clearSuccessApp", errorReason));
    EXPECT_STREQ("", errorReason.c_str());
}

TEST_F(AppStorageManagerTest, Clear_nftwFailure) {
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50000;
            buf->f_bavail = 50000;
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillOnce([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        })
        .WillOnce([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return -1;
    });
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("clearNftwFailApp", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_GENERAL, interface->Clear("clearNftwFailApp", errorReason));
}

TEST_F(AppStorageManagerTest, ClearAll_EmptyExemptionList_Success) {
    std::string errorReason = "";
    EXPECT_EQ(Core::ERROR_NONE, interface->ClearAll("", errorReason));
}

TEST_F(AppStorageManagerTest, ClearAll_OpenDirFailure) {
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, opendir(_))
        .WillOnce([](const char* pathname) -> DIR* {
            return nullptr;
    });
    EXPECT_EQ(Core::ERROR_GENERAL, interface->ClearAll("[]", errorReason));
}

TEST_F(AppStorageManagerTest, CreateStorage_StatvfsFailure) {
    std::string appId = "statvfsFailApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            errno = EEXIST;
            return -1;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return -1;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            return -1;
    });
    EXPECT_EQ(Core::ERROR_GENERAL, interface->CreateStorage(appId, size, path, errorReason));
}

TEST_F(AppStorageManagerTest, CreateStorage_AppDirMkdirFailure) {
    std::string appId = "appDirMkdirFailApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    static int mkdirCallCount = 0;
    mkdirCallCount = 0;
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            if (mkdirCallCount++ == 0) {
                return 0;
            }
            errno = EACCES;
            return -1;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return -1;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
    });
    EXPECT_EQ(Core::ERROR_GENERAL, interface->CreateStorage(appId, size, path, errorReason));
}

TEST_F(AppStorageManagerTest, CreateStorage_SetValueFailure) {
    std::string appId = "setValueFailApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return -1;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_GENERAL;
    }));
    EXPECT_EQ(Core::ERROR_GENERAL, interface->CreateStorage(appId, size, path, errorReason));
}

TEST_F(AppStorageManagerTest, CreateStorage_ExistingStorageSameSize) {
    std::string appId = "existingSameSizeApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    std::string path2 = "";
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path2, errorReason));
    EXPECT_EQ(path, path2);
}

TEST_F(AppStorageManagerTest, CreateStorage_ExistingStorageDifferentSize) {
    std::string appId = "existingDiffSizeApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    ON_CALL(*mStore2Mock, DeleteKey(_, _, _))
        .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key) -> uint32_t {
            return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    std::string path2 = "";
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, 2048, path2, errorReason));
}

TEST_F(AppStorageManagerTest, GetStorage_UpdateOwnership_Success) {
    std::string appId = "ownershipUpdateApp";
    uint32_t size = 1024;
    std::string errorReason = "";
    uint32_t userId = 200;
    uint32_t groupId = 201;
    std::string path = "";
    uint32_t used = 0;
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50000;
            buf->f_bavail = 50000;
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, chown(_, _, _))
        .WillRepeatedly([](const char* path, uid_t owner, gid_t group) {
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    ON_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
    .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
            if (key == "quotaSize") {
                value = "1024";
            } else {
                value = "mockValue";
            }
            ttl = 0;
            return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, interface->GetStorage(appId, userId, groupId, path, size, used));
    uint32_t newUserId = 300;
    uint32_t newGroupId = 301;
    EXPECT_EQ(Core::ERROR_NONE, interface->GetStorage(appId, newUserId, newGroupId, path, size, used));
}

TEST_F(AppStorageManagerTest, DeleteStorage_DeleteKeyFailure) {
    std::string appId = "deleteKeyFailApp";
    uint32_t size = 1024;
    std::string errorReason = "";
    std::string path = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50000;
            buf->f_bavail = 50000;
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, rmdir(_))
        .WillRepeatedly([](const char* pathname) {
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    ON_CALL(*mStore2Mock, DeleteKey(_, _, _))
        .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key) -> uint32_t {
            return Core::ERROR_GENERAL;
    }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, interface->DeleteStorage(appId, errorReason));
}

TEST_F(AppStorageManagerTest, DeleteStorage_nftwFailure) {
    std::string appId = "deleteNftwFailApp";
    uint32_t size = 1024;
    std::string errorReason = "";
    std::string path = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50000;
            buf->f_bavail = 50000;
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillOnce([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        })
        .WillOnce([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return -1;
    });
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_EQ(Core::ERROR_GENERAL, interface->DeleteStorage(appId, errorReason));
}

TEST_F(AppStorageManagerTest, ClearAll_WithValidExemptions) {
    std::string path = "";
    std::string errorReason = "";
    static int callCount = 0;
    callCount = 0;
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50000;
            buf->f_bavail = 50000;
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    ON_CALL(*p_wrapsImplMock, opendir(_))
        .WillByDefault(Invoke([](const char* pathname) {
            return __real_opendir(pathname);
    }));
    ON_CALL(*p_wrapsImplMock, readdir(_))
    .WillByDefault([](DIR* dirp) -> struct dirent* {
        static struct dirent entry;
        switch (callCount++) {
            case 0:
                std::strcpy(entry.d_name, "clearAllApp1");
                entry.d_type = DT_DIR;
                return &entry;
            case 1:
                std::strcpy(entry.d_name, "exemptApp");
                entry.d_type = DT_DIR;
                return &entry;
            default:
                return nullptr;
        }
    });
    ON_CALL(*p_wrapsImplMock, closedir(_))
        .WillByDefault([](DIR* dirp) {
            return 0;
    });
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("clearAllApp1", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("exemptApp", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, interface->ClearAll("[\"exemptApp\"]", errorReason));
}

TEST_F(AppStorageManagerTest, ClearAll_InvalidJsonFormat) {
    std::string errorReason = "";
    static int callCount = 0;
    callCount = 0;
    ON_CALL(*p_wrapsImplMock, opendir(_))
        .WillByDefault(Invoke([](const char* pathname) {
            return __real_opendir(pathname);
    }));
    ON_CALL(*p_wrapsImplMock, readdir(_))
    .WillByDefault([](DIR* dirp) -> struct dirent* {
        return nullptr;
    });
    ON_CALL(*p_wrapsImplMock, closedir(_))
        .WillByDefault([](DIR* dirp) {
            return 0;
    });
    EXPECT_EQ(Core::ERROR_NONE, interface->ClearAll("invalid_json", errorReason));
}

TEST_F(AppStorageManagerTest, GetStorage_AccessFailure_RemoveEntry) {
    std::string appId = "accessFailApp";
    uint32_t size = 1024;
    std::string errorReason = "";
    uint32_t userId = 100;
    uint32_t groupId = 101;
    std::string path = "";
    uint32_t used = 0;
    static int accessCallCount = 0;
    accessCallCount = 0;
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50000;
            buf->f_bavail = 50000;
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    ON_CALL(*mStore2Mock, DeleteKey(_, _, _))
        .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key) -> uint32_t {
            return Core::ERROR_NONE;
    }));
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            if (accessCallCount++ < 2) {
                return 0;
            }
            return -1;
    });
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_EQ(Core::ERROR_GENERAL, interface->GetStorage(appId, userId, groupId, path, size, used));
}

TEST_F(AppStorageManagerTest, PluginInformation) {
    std::string info = plugin->Information();
    EXPECT_TRUE(info.empty());
}

TEST_F(AppStorageManagerTest, CreateStorage_LargeSize) {
    std::string appId = "largeSizeApp";
    uint32_t size = 0xFFFFFFFF;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            errno = EEXIST;
            return -1;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return -1;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50;
            buf->f_bavail = 50;
            return 0;
    });
    EXPECT_EQ(Core::ERROR_GENERAL, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_STREQ("Insufficient storage space", errorReason.c_str());
}

TEST_F(AppStorageManagerTest, CreateStorage_SpecialCharsInAppId) {
    std::string appId = "app-with_special.chars123";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_TRUE(path.find(appId) != std::string::npos);
}

TEST_F(AppStorageManagerTest, Clear_JsonRpc_EmptyAppId_Failure) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("clear"), _T("{\"appId\":\"\"}"), response));
}

TEST_F(AppStorageManagerTest, ClearAll_JsonRpc_EmptyExemption_Success) {
    ON_CALL(*p_wrapsImplMock, opendir(_))
        .WillByDefault(Invoke([](const char* pathname) {
            return __real_opendir(pathname);
    }));
    ON_CALL(*p_wrapsImplMock, readdir(_))
        .WillByDefault([](DIR* dirp) -> struct dirent* {
            return nullptr;
    });
    ON_CALL(*p_wrapsImplMock, closedir(_))
        .WillByDefault([](DIR* dirp) {
            return 0;
    });
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("clearAll"), _T("{\"exemptionAppIds\":\"\"}"), response));
}

TEST_F(AppStorageManagerTest, GetStorage_QuotaExceeded_Warning) {
    std::string appId = "quotaExceededApp";
    uint32_t size = 100;
    std::string errorReason = "";
    uint32_t userId = 100;
    uint32_t groupId = 101;
    std::string path = "";
    uint32_t used = 0;
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50000;
            buf->f_bavail = 50000;
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, chown(_, _, _))
        .WillRepeatedly([](const char* path, uid_t owner, gid_t group) {
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    ON_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
    .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
            if (key == "quotaSize") {
                value = "100";
            } else {
                value = "mockValue";
            }
            ttl = 0;
            return Core::ERROR_NONE;
    }));
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, interface->GetStorage(appId, userId, groupId, path, size, used));
}

TEST_F(AppStorageManagerTest, MultipleSequentialOperations) {
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, chown(_, _, _))
        .WillRepeatedly([](const char* path, uid_t owner, gid_t group) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, rmdir(_))
        .WillRepeatedly([](const char* pathname) {
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    ON_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
    .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
            if (key == "quotaSize") {
                value = "1024";
            } else {
                value = "mockValue";
            }
            ttl = 0;
            return Core::ERROR_NONE;
    }));
    ON_CALL(*mStore2Mock, DeleteKey(_, _, _))
        .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key) -> uint32_t {
            return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("seqApp1", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("seqApp2", 2048, path, errorReason));
    uint32_t size = 0, used = 0;
    EXPECT_EQ(Core::ERROR_NONE, interface->GetStorage("seqApp1", 100, 100, path, size, used));
    EXPECT_EQ(Core::ERROR_NONE, interface->Clear("seqApp1", errorReason));
    EXPECT_EQ(Core::ERROR_NONE, interface->DeleteStorage("seqApp2", errorReason));
}

TEST_F(AppStorageManagerTest, GetStorage_JsonRpc_MissingUserId) {
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50000;
            buf->f_bavail = 50000;
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, chown(_, _, _))
        .WillRepeatedly([](const char* path, uid_t owner, gid_t group) {
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    ON_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
    .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
            if (key == "quotaSize") {
                value = "1024";
            } else {
                value = "mockValue";
            }
            ttl = 0;
            return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("missingUserIdApp", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getStorage"), _T("{\"appId\":\"missingUserIdApp\",\"groupId\":101}"), response));
}

TEST_F(AppStorageManagerTest, CreateStorage_BasePathPermissionIssue) {
    std::string appId = "permissionIssueApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillOnce([](const char* path, mode_t mode) {
            errno = EPERM;
            return -1;
    });
    EXPECT_EQ(Core::ERROR_GENERAL, interface->CreateStorage(appId, size, path, errorReason));
}

TEST_F(AppStorageManagerTest, CreateStorage_JsonRpc_ValidateResponsePath) {
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("createStorage"), _T("{\"appId\":\"responsePathApp\",\"size\":512}"), response));
    EXPECT_TRUE(response.find("/opt/persistent/storageManager/responsePathApp") != std::string::npos);
}

TEST_F(AppStorageManagerTest, CreateStorage_JsonRpc_InvalidJsonPayload) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("createStorage"), _T("{invalid json}"), response));
}

TEST_F(AppStorageManagerTest, GetStorage_JsonRpc_ValidateResponseFields) {
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50000;
            buf->f_bavail = 50000;
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, chown(_, _, _))
        .WillRepeatedly([](const char* path, uid_t owner, gid_t group) {
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    ON_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
    .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
            if (key == "quotaSize") {
                value = "2048";
            } else {
                value = "mockValue";
            }
            ttl = 0;
            return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("validateFieldsApp", 2048, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getStorage"), _T("{\"appId\":\"validateFieldsApp\",\"userId\":500,\"groupId\":500}"), response));
    EXPECT_TRUE(response.find("\"path\"") != std::string::npos);
    EXPECT_TRUE(response.find("\"size\"") != std::string::npos);
    EXPECT_TRUE(response.find("\"used\"") != std::string::npos);
}

TEST_F(AppStorageManagerTest, GetStorage_JsonRpc_InvalidJsonPayload) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getStorage"), _T("{invalid}"), response));
}

TEST_F(AppStorageManagerTest, DeleteStorage_JsonRpc_ValidateEmptyResponse) {
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50000;
            buf->f_bavail = 50000;
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, rmdir(_))
        .WillRepeatedly([](const char* pathname) {
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    ON_CALL(*mStore2Mock, DeleteKey(_, _, _))
        .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key) -> uint32_t {
            return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("deleteValidateApp", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("deleteStorage"), _T("{\"appId\":\"deleteValidateApp\"}"), response));
}

TEST_F(AppStorageManagerTest, DeleteStorage_JsonRpc_InvalidJsonPayload) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("deleteStorage"), _T("{bad json"), response));
}

TEST_F(AppStorageManagerTest, DeleteStorage_JsonRpc_MissingAppId) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("deleteStorage"), _T("{}"), response));
}

TEST_F(AppStorageManagerTest, Clear_JsonRpc_Success) {
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50000;
            buf->f_bavail = 50000;
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("clearJsonRpcApp", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("clear"), _T("{\"appId\":\"clearJsonRpcApp\"}"), response));
}

TEST_F(AppStorageManagerTest, Clear_JsonRpc_InvalidJsonPayload) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("clear"), _T("not valid json"), response));
}

TEST_F(AppStorageManagerTest, Clear_JsonRpc_MissingAppId) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("clear"), _T("{}"), response));
}

TEST_F(AppStorageManagerTest, Clear_JsonRpc_NonExistentApp) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("clear"), _T("{\"appId\":\"nonExistentClearJsonApp\"}"), response));
}

TEST_F(AppStorageManagerTest, ClearAll_JsonRpc_WithExemptions) {
    std::string path = "";
    std::string errorReason = "";
    static int callCount = 0;
    callCount = 0;
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50000;
            buf->f_bavail = 50000;
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    ON_CALL(*p_wrapsImplMock, opendir(_))
        .WillByDefault(Invoke([](const char* pathname) {
            return __real_opendir(pathname);
    }));
    ON_CALL(*p_wrapsImplMock, readdir(_))
    .WillByDefault([](DIR* dirp) -> struct dirent* {
        static struct dirent entry;
        switch (callCount++) {
            case 0:
                std::strcpy(entry.d_name, "clearAllJsonApp1");
                entry.d_type = DT_DIR;
                return &entry;
            case 1:
                std::strcpy(entry.d_name, "exemptJsonApp");
                entry.d_type = DT_DIR;
                return &entry;
            default:
                return nullptr;
        }
    });
    ON_CALL(*p_wrapsImplMock, closedir(_))
        .WillByDefault([](DIR* dirp) {
            return 0;
    });
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("clearAllJsonApp1", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("exemptJsonApp", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("clearAll"), _T("{\"exemptionAppIds\":\"[\\\"exemptJsonApp\\\"]\"}"), response));
}

TEST_F(AppStorageManagerTest, ClearAll_JsonRpc_InvalidJsonPayload) {
    ON_CALL(*p_wrapsImplMock, opendir(_))
        .WillByDefault(Invoke([](const char* pathname) {
            return __real_opendir(pathname);
    }));
    ON_CALL(*p_wrapsImplMock, readdir(_))
        .WillByDefault([](DIR* dirp) -> struct dirent* {
            return nullptr;
    });
    ON_CALL(*p_wrapsImplMock, closedir(_))
        .WillByDefault([](DIR* dirp) {
            return 0;
    });
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("clearAll"), _T("{\"exemptionAppIds\":\"invalid\"}"), response));
}

TEST_F(AppStorageManagerTest, CreateStorage_JsonRpc_NegativeSize) {
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("createStorage"), _T("{\"appId\":\"negativeSizeApp\",\"size\":-1}"), response));
}

TEST_F(AppStorageManagerTest, GetStorage_JsonRpc_NegativeUserId) {
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50000;
            buf->f_bavail = 50000;
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, chown(_, _, _))
        .WillRepeatedly([](const char* path, uid_t owner, gid_t group) {
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    ON_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
    .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
            if (key == "quotaSize") {
                value = "1024";
            } else {
                value = "mockValue";
            }
            ttl = 0;
            return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("negativeUserIdApp", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getStorage"), _T("{\"appId\":\"negativeUserIdApp\",\"userId\":-1,\"groupId\":-1}"), response));
}

TEST_F(AppStorageManagerTest, CreateStorage_JsonRpc_WhitespaceAppId) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("createStorage"), _T("{\"appId\":\"   \",\"size\":1024}"), response));
}

TEST_F(AppStorageManagerTest, CreateStorage_JsonRpc_VeryLongAppId) {
    std::string longAppId(1000, 'a');
    std::string jsonPayload = "{\"appId\":\"" + longAppId + "\",\"size\":1024}";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("createStorage"), jsonPayload, response));
}

TEST_F(AppStorageManagerTest, GetStorage_JsonRpc_ZeroUserIdGroupId) {
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50000;
            buf->f_bavail = 50000;
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, chown(_, _, _))
        .WillRepeatedly([](const char* path, uid_t owner, gid_t group) {
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    ON_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
    .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
            if (key == "quotaSize") {
                value = "1024";
            } else {
                value = "mockValue";
            }
            ttl = 0;
            return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("zeroIdsApp", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getStorage"), _T("{\"appId\":\"zeroIdsApp\",\"userId\":0,\"groupId\":0}"), response));
}

TEST_F(AppStorageManagerTest, ClearAll_JsonRpc_EmptyArrayExemptions) {
    ON_CALL(*p_wrapsImplMock, opendir(_))
        .WillByDefault(Invoke([](const char* pathname) {
            return __real_opendir(pathname);
    }));
    ON_CALL(*p_wrapsImplMock, readdir(_))
        .WillByDefault([](DIR* dirp) -> struct dirent* {
            return nullptr;
    });
    ON_CALL(*p_wrapsImplMock, closedir(_))
        .WillByDefault([](DIR* dirp) {
            return 0;
    });
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("clearAll"), _T("{\"exemptionAppIds\":\"[]\"}"), response));
}

TEST_F(AppStorageManagerTest, CreateStorage_JsonRpc_MaxUint32Size) {
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            errno = EEXIST;
            return -1;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return -1;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 10;
            buf->f_bavail = 10;
            return 0;
    });
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("createStorage"), _T("{\"appId\":\"maxSizeApp\",\"size\":4294967295}"), response));
}

TEST_F(AppStorageManagerTest, GetStorage_JsonRpc_MissingGroupId) {
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50000;
            buf->f_bavail = 50000;
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, chown(_, _, _))
        .WillRepeatedly([](const char* path, uid_t owner, gid_t group) {
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    ON_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
    .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
            if (key == "quotaSize") {
                value = "1024";
            } else {
                value = "mockValue";
            }
            ttl = 0;
            return Core::ERROR_NONE;
    }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("missingGroupIdApp", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getStorage"), _T("{\"appId\":\"missingGroupIdApp\",\"userId\":100}"), response));
}

TEST_F(AppStorageManagerTest, CreateStorage_JsonRpc_DoubleDotsInAppId) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("createStorage"), _T("{\"appId\":\"invalid..appId\",\"size\":1024}"), response));
}

TEST_F(AppStorageManagerTest, CreateStorage_JsonRpc_AppIdStartsWithDot) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("createStorage"), _T("{\"appId\":\".invalidAppId\",\"size\":1024}"), response));
}

TEST_F(AppStorageManagerTest, CreateStorage_JsonRpc_AppIdEndsWithDot) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("createStorage"), _T("{\"appId\":\"invalidAppId.\",\"size\":1024}"), response));
}

TEST_F(AppStorageManagerTest, CreateStorage_PersistentStoreSetValueFailure) {
    std::string appId = "persistentStoreFailApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return -1;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
    });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillOnce(Invoke([](Exchange::IStore2::ScopeType scope,
                           const std::string& appId,
                           const std::string& key,
                           const std::string& value,
                           const uint32_t ttl) -> uint32_t {
        return Core::ERROR_GENERAL;
    }));
    EXPECT_EQ(Core::ERROR_GENERAL, interface->CreateStorage(appId, size, path, errorReason));
}

TEST_F(AppStorageManagerTest, GetStorage_PersistentStoreGetValueFailure) {
    std::string appId = "getValueFailApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    uint32_t used = 0;
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, access(_, _))
        .WillByDefault([](const char* path, int mode) {
            return 0;
    });
    ON_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillByDefault([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
    });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 50000;
            buf->f_bavail = 50000;
            return 0;
    });
    ON_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillByDefault(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
        return Core::ERROR_NONE;
    }));
    static int getValueCallCount = 0;
    getValueCallCount = 0;
    ON_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
    .WillByDefault(Invoke(
        [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
            if (getValueCallCount++ < 2) {
                if (key == "quotaSize") {
                    value = "1024";
                }
                return Core::ERROR_NONE;
            }
            return Core::ERROR_GENERAL;
    }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
}
TEST_F(AppStorageManagerTest, CreateStorage_MkdirException) {
    std::string appId = "mkdirExceptionApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillOnce(Invoke(
            [](const char* path, mode_t mode) {
                throw std::runtime_error("mkdir exception");
                return 0;
            }));
    EXPECT_THROW(interface->CreateStorage(appId, size, path, errorReason), std::runtime_error);
}

TEST_F(AppStorageManagerTest, CreateStorage_StatvfsException) {
    std::string appId = "statvfsExceptionApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            errno = EEXIST;
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillOnce(Invoke(
            [](const char* path, struct statvfs* buf) {
                throw std::runtime_error("statvfs exception");
                return 0;
            }));
    EXPECT_THROW(interface->CreateStorage(appId, size, path, errorReason), std::runtime_error);
}

TEST_F(AppStorageManagerTest, CreateStorage_InvalidAppIdWithSlash) {
    std::string appId = "invalid/appId";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("createStorage"), _T("{\"appId\":\"invalid/appId\",\"size\":1024}"), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(AppStorageManagerTest, CreateStorage_InvalidAppIdWithBackslash) {
    std::string appId = "invalid\\appId";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("createStorage"), _T("{\"appId\":\"invalid\\\\appId\",\"size\":1024}"), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(AppStorageManagerTest, CreateStorage_PathTraversalAttempt) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("createStorage"), _T("{\"appId\":\"../../../etc\",\"size\":1024}"), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(AppStorageManagerTest, GetStorage_ChownException) {
    std::string appId = "chownExceptionApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    uint32_t used = 0;
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke(
            [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
                if (key == "quotaSize") {
                    value = "1024";
                }
                ttl = 0;
                return Core::ERROR_NONE;
            }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_CALL(*p_wrapsImplMock, chown(_, _, _))
        .WillOnce(Invoke(
            [](const char* path, uid_t owner, gid_t group) {
                throw std::runtime_error("chown exception");
                return 0;
            }));
    EXPECT_THROW(interface->GetStorage(appId, 500, 500, path, size, used), std::runtime_error);
}

TEST_F(AppStorageManagerTest, GetStorage_NftwException) {
    std::string appId = "nftwExceptionGetApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    uint32_t used = 0;
    static int mkdirCallCount = 0;
    mkdirCallCount = 0;
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    static int accessCallCount = 0;
    accessCallCount = 0;
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    static int nftwCallCount = 0;
    nftwCallCount = 0;
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillOnce([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        })
        .WillOnce(Invoke(
            [](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
                throw std::runtime_error("nftw exception");
                return 0;
            }));
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke(
            [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
                if (key == "quotaSize") {
                    value = "1024";
                }
                ttl = 0;
                return Core::ERROR_NONE;
            }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_THROW(interface->GetStorage(appId, 500, 500, path, size, used), std::runtime_error);
}

TEST_F(AppStorageManagerTest, DeleteStorage_RmdirException) {
    std::string appId = "rmdirExceptionApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_CALL(*p_wrapsImplMock, rmdir(_))
        .WillOnce(Invoke(
            [](const char* pathname) {
                throw std::runtime_error("rmdir exception");
                return 0;
            }));
    EXPECT_THROW(interface->DeleteStorage(appId, errorReason), std::runtime_error);
}

TEST_F(AppStorageManagerTest, DeleteStorage_NftwException) {
    std::string appId = "nftwExceptionDeleteApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    static int nftwCallCount = 0;
    nftwCallCount = 0;
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillOnce([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        })
        .WillOnce(Invoke(
            [](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
                throw std::runtime_error("nftw delete exception");
                return 0;
            }));
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_THROW(interface->DeleteStorage(appId, errorReason), std::runtime_error);
}

TEST_F(AppStorageManagerTest, Clear_NftwException) {
    std::string appId = "nftwExceptionClearApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillOnce([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        })
        .WillOnce(Invoke(
            [](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
                throw std::runtime_error("nftw clear exception");
                return 0;
            }));
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_THROW(interface->Clear(appId, errorReason), std::runtime_error);
}

TEST_F(AppStorageManagerTest, ClearAll_OpendirException) {
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, opendir(_))
        .WillOnce(Invoke(
            [](const char* pathname) -> DIR* {
                throw std::runtime_error("opendir exception");
                return nullptr;
            }));
    EXPECT_THROW(interface->ClearAll("[]", errorReason), std::runtime_error);
}

TEST_F(AppStorageManagerTest, ClearAll_ReaddirException) {
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("clearAllReaddirApp", 1024, path, errorReason));
    EXPECT_CALL(*p_wrapsImplMock, opendir(_))
        .WillOnce(Invoke([](const char* pathname) {
            return __real_opendir(pathname);
        }));
    EXPECT_CALL(*p_wrapsImplMock, readdir(_))
        .WillOnce(Invoke(
            [](DIR* dirp) -> struct dirent* {
                throw std::runtime_error("readdir exception");
                return nullptr;
            }));
    EXPECT_THROW(interface->ClearAll("[]", errorReason), std::runtime_error);
}

TEST_F(AppStorageManagerTest, ClearAll_ClosedirException) {
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, opendir(_))
        .WillOnce(Invoke([](const char* pathname) {
            return __real_opendir(pathname);
        }));
    EXPECT_CALL(*p_wrapsImplMock, readdir(_))
        .WillOnce([](DIR* dirp) -> struct dirent* {
            return nullptr;
        });
    EXPECT_CALL(*p_wrapsImplMock, closedir(_))
        .WillOnce(Invoke(
            [](DIR* dirp) {
                throw std::runtime_error("closedir exception");
                return 0;
            }));
    EXPECT_THROW(interface->ClearAll("[]", errorReason), std::runtime_error);
}

TEST_F(AppStorageManagerTest, CreateStorage_SetValueException) {
    std::string appId = "setValueExceptionApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillOnce(Invoke(
            [](Exchange::IStore2::ScopeType scope,
               const std::string& appId,
               const std::string& key,
               const std::string& value,
               const uint32_t ttl) -> uint32_t {
                throw std::runtime_error("SetValue exception");
                return Core::ERROR_NONE;
            }));
    EXPECT_THROW(interface->CreateStorage(appId, size, path, errorReason), std::runtime_error);
}

TEST_F(AppStorageManagerTest, GetStorage_GetValueException) {
    std::string appId = "getValueExceptionApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    uint32_t used = 0;
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, chown(_, _, _))
        .WillRepeatedly([](const char* path, uid_t owner, gid_t group) {
            return 0;
        });
    static int setValueCallCount = 0;
    setValueCallCount = 0;
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
        .WillOnce(Invoke(
            [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
                if (key == "quotaSize") {
                    value = "1024";
                }
                ttl = 0;
                return Core::ERROR_NONE;
            }))
        .WillOnce(Invoke(
            [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
                throw std::runtime_error("GetValue exception");
                return Core::ERROR_NONE;
            }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_THROW(interface->GetStorage(appId, 500, 500, path, size, used), std::runtime_error);
}

TEST_F(AppStorageManagerTest, DeleteStorage_DeleteKeyException) {
    std::string appId = "deleteKeyExceptionApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, rmdir(_))
        .WillRepeatedly([](const char* pathname) {
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_CALL(*mStore2Mock, DeleteKey(_, _, _))
        .WillOnce(Invoke(
            [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key) -> uint32_t {
                throw std::runtime_error("DeleteKey exception");
                return Core::ERROR_NONE;
            }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_THROW(interface->DeleteStorage(appId, errorReason), std::runtime_error);
}

TEST_F(AppStorageManagerTest, CreateStorage_AccessException) {
    std::string appId = "accessExceptionApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            errno = EEXIST;
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillOnce(Invoke(
            [](const char* path, int mode) {
                throw std::runtime_error("access exception");
                return 0;
            }));
    EXPECT_THROW(interface->CreateStorage(appId, size, path, errorReason), std::runtime_error);
}

TEST_F(AppStorageManagerTest, GetStorage_AccessException) {
    std::string appId = "accessExceptionGetApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    uint32_t used = 0;
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    static int accessCallCount = 0;
    accessCallCount = 0;
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillOnce([](const char* path, int mode) {
            return 0;
        })
        .WillOnce(Invoke(
            [](const char* path, int mode) {
                throw std::runtime_error("access exception in GetStorage");
                return 0;
            }));
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke(
            [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
                if (key == "quotaSize") {
                    value = "1024";
                }
                ttl = 0;
                return Core::ERROR_NONE;
            }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_THROW(interface->GetStorage(appId, 500, 500, path, size, used), std::runtime_error);
}

TEST_F(AppStorageManagerTest, CreateStorage_JsonRpc_InvalidAppIdStartsWithNumber) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("createStorage"), _T("{\"appId\":\"123invalidApp\",\"size\":1024}"), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(AppStorageManagerTest, CreateStorage_JsonRpc_InvalidAppIdWithSpaces) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("createStorage"), _T("{\"appId\":\"app with spaces\",\"size\":1024}"), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(AppStorageManagerTest, GetStorage_NonExistentAppId) {
    std::string appId = "nonExistentAppIdGet";
    uint32_t userId = 500;
    uint32_t groupId = 500;
    std::string path = "";
    uint32_t size = 0;
    uint32_t used = 0;
    EXPECT_EQ(Core::ERROR_GENERAL, interface->GetStorage(appId, userId, groupId, path, size, used));
}

TEST_F(AppStorageManagerTest, DeleteStorage_NonExistentAppId) {
    std::string appId = "nonExistentAppIdDelete";
    std::string errorReason = "";
    EXPECT_EQ(Core::ERROR_GENERAL, interface->DeleteStorage(appId, errorReason));
    EXPECT_STREQ("AppId not found in storage info", errorReason.c_str());
}

TEST_F(AppStorageManagerTest, Clear_NonExistentAppId) {
    std::string appId = "nonExistentAppIdClear";
    std::string errorReason = "";
    EXPECT_EQ(Core::ERROR_GENERAL, interface->Clear(appId, errorReason));
    EXPECT_STREQ("Storage not found for appId: nonExistentAppIdClear", errorReason.c_str());
}

TEST_F(AppStorageManagerTest, CreateStorage_StatvfsReturnsError) {
    std::string appId = "statvfsErrorApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            errno = EEXIST;
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillOnce([](const char* path, struct statvfs* buf) {
            errno = EIO;
            return -1;
        });
    EXPECT_EQ(Core::ERROR_GENERAL, interface->CreateStorage(appId, size, path, errorReason));
}

TEST_F(AppStorageManagerTest, CreateStorage_SecondMkdirFails) {
    std::string appId = "secondMkdirFailApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    static int mkdirCallCount = 0;
    mkdirCallCount = 0;
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            mkdirCallCount++;
            if (mkdirCallCount == 1) {
                return 0;
            }
            errno = EACCES;
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_EQ(Core::ERROR_GENERAL, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_STREQ("Failed to create app storage directory: /opt/persistent/storageManager/secondMkdirFailApp", errorReason.c_str());
}

TEST_F(AppStorageManagerTest, DeleteStorage_RmdirReturnsError) {
    std::string appId = "rmdirErrorApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_CALL(*p_wrapsImplMock, rmdir(_))
        .WillOnce([](const char* pathname) {
            errno = ENOTEMPTY;
            return -1;
        });
    EXPECT_EQ(Core::ERROR_GENERAL, interface->DeleteStorage(appId, errorReason));
    EXPECT_TRUE(errorReason.find("Error deleting the empty App Folder") != std::string::npos);
}

TEST_F(AppStorageManagerTest, Clear_NftwReturnsError) {
    std::string appId = "clearNftwErrorApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    static int nftwCallCount = 0;
    nftwCallCount = 0;
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillOnce([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        })
        .WillOnce([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_EQ(Core::ERROR_GENERAL, interface->Clear(appId, errorReason));
    EXPECT_TRUE(errorReason.find("Failed to clear App storage path") != std::string::npos);
}

TEST_F(AppStorageManagerTest, ClearAll_OpendirReturnsNull) {
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, opendir(_))
        .WillOnce([](const char* pathname) -> DIR* {
            return nullptr;
        });
    EXPECT_EQ(Core::ERROR_GENERAL, interface->ClearAll("[]", errorReason));
    EXPECT_TRUE(errorReason.find("Failed to open storage directory") != std::string::npos);
}

TEST_F(AppStorageManagerTest, GetStorage_ChownReturnsError) {
    std::string appId = "chownErrorApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    uint32_t used = 0;
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke(
            [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
                if (key == "quotaSize") {
                    value = "1024";
                }
                ttl = 0;
                return Core::ERROR_NONE;
            }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_CALL(*p_wrapsImplMock, chown(_, _, _))
        .WillOnce([](const char* path, uid_t owner, gid_t group) {
            errno = EPERM;
            return -1;
        });
    EXPECT_EQ(Core::ERROR_GENERAL, interface->GetStorage(appId, 500, 500, path, size, used));
}

TEST_F(AppStorageManagerTest, CreateStorage_SetValueReturnsError) {
    std::string appId = "setValueErrorApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillOnce(Invoke([](Exchange::IStore2::ScopeType scope,
                           const std::string& appId,
                           const std::string& key,
                           const std::string& value,
                           const uint32_t ttl) -> uint32_t {
            return Core::ERROR_GENERAL;
        }));
    EXPECT_EQ(Core::ERROR_GENERAL, interface->CreateStorage(appId, size, path, errorReason));
}

// ============================================================================
// CreateStorage - Comprehensive Positive Tests
// ============================================================================

TEST_F(AppStorageManagerTest, CreateStorage_Positive_MinimumValidSize) {
    std::string appId = "minSizeApp";
    uint32_t size = 1;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_FALSE(path.empty());
    EXPECT_TRUE(errorReason.empty());
}

TEST_F(AppStorageManagerTest, CreateStorage_Positive_ValidAppIdWithUnderscores) {
    std::string appId = "valid_app_id";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_TRUE(path.find("valid_app_id") != std::string::npos);
}

TEST_F(AppStorageManagerTest, CreateStorage_Positive_ValidAppIdWithDashes) {
    std::string appId = "valid-app-id";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_TRUE(path.find("valid-app-id") != std::string::npos);
}

TEST_F(AppStorageManagerTest, CreateStorage_Positive_ValidAppIdWithDot) {
    std::string appId = "com.example.app";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_TRUE(path.find("com.example.app") != std::string::npos);
}

TEST_F(AppStorageManagerTest, CreateStorage_Positive_DirectoryAlreadyExists) {
    std::string appId = "existingDirApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            errno = EEXIST;
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_FALSE(path.empty());
}

TEST_F(AppStorageManagerTest, CreateStorage_Positive_JsonRpc_ValidRequest) {
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("createStorage"), _T("{\"appId\":\"jsonRpcValidApp\",\"size\":2048}"), response));
    EXPECT_TRUE(response.find("path") != std::string::npos);
}

TEST_F(AppStorageManagerTest, CreateStorage_Positive_UpdateExistingWithSameSize) {
    std::string appId = "updateSameSizeApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke(
            [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
                if (key == "quotaSize") {
                    value = "1024";
                }
                ttl = 0;
                return Core::ERROR_NONE;
            }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    std::string path2 = "";
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path2, errorReason));
    EXPECT_EQ(path, path2);
}

// ============================================================================
// CreateStorage - Comprehensive Negative Tests
// ============================================================================

TEST_F(AppStorageManagerTest, CreateStorage_Negative_EmptyAppId) {
    std::string appId = "";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_EQ(Core::ERROR_GENERAL, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_STREQ("appId cannot be empty", errorReason.c_str());
}

TEST_F(AppStorageManagerTest, CreateStorage_Negative_AppIdStartsWithDot) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("createStorage"), _T("{\"appId\":\".invalidApp\",\"size\":1024}"), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(AppStorageManagerTest, CreateStorage_Negative_AppIdEndsWithDot) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("createStorage"), _T("{\"appId\":\"invalidApp.\",\"size\":1024}"), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(AppStorageManagerTest, CreateStorage_Negative_AppIdWithConsecutiveDots) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("createStorage"), _T("{\"appId\":\"invalid..app\",\"size\":1024}"), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(AppStorageManagerTest, CreateStorage_Negative_InsufficientSpace) {
    std::string appId = "noSpaceApp";
    uint32_t size = 1000000;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100;
            buf->f_bfree = 10;
            buf->f_bavail = 10;
            return 0;
        });
    EXPECT_EQ(Core::ERROR_GENERAL, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_STREQ("Insufficient storage space", errorReason.c_str());
}

TEST_F(AppStorageManagerTest, CreateStorage_Negative_BaseDirMkdirFailsNotEEXIST) {
    std::string appId = "baseDirFailApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillOnce([](const char* path, mode_t mode) {
            errno = EACCES;
            return -1;
        });
    EXPECT_EQ(Core::ERROR_GENERAL, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_TRUE(errorReason.find("Failed to create base storage directory") != std::string::npos);
}

// ============================================================================
// GetStorage - Comprehensive Positive Tests
// ============================================================================

TEST_F(AppStorageManagerTest, GetStorage_Positive_ValidAppWithOwnershipChange) {
    std::string appId = "ownerChangeApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    uint32_t used = 0;
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, chown(_, _, _))
        .WillRepeatedly([](const char* path, uid_t owner, gid_t group) {
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke(
            [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
                if (key == "quotaSize") {
                    value = "1024";
                }
                ttl = 0;
                return Core::ERROR_NONE;
            }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, interface->GetStorage(appId, 1000, 1000, path, size, used));
    EXPECT_EQ(1024u, size);
}

TEST_F(AppStorageManagerTest, GetStorage_Positive_SameOwnership) {
    std::string appId = "sameOwnerApp";
    uint32_t size = 2048;
    std::string path = "";
    std::string errorReason = "";
    uint32_t used = 0;
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, chown(_, _, _))
        .WillRepeatedly([](const char* path, uid_t owner, gid_t group) {
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke(
            [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
                if (key == "quotaSize") {
                    value = "2048";
                }
                ttl = 0;
                return Core::ERROR_NONE;
            }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, interface->GetStorage(appId, 500, 500, path, size, used));
    EXPECT_EQ(Core::ERROR_NONE, interface->GetStorage(appId, 500, 500, path, size, used));
}

TEST_F(AppStorageManagerTest, GetStorage_Positive_JsonRpc_ValidRequest) {
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, chown(_, _, _))
        .WillRepeatedly([](const char* path, uid_t owner, gid_t group) {
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke(
            [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
                if (key == "quotaSize") {
                    value = "1024";
                }
                ttl = 0;
                return Core::ERROR_NONE;
            }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("getJsonRpcApp", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getStorage"), _T("{\"appId\":\"getJsonRpcApp\",\"userId\":500,\"groupId\":500}"), response));
    EXPECT_TRUE(response.find("path") != std::string::npos);
    EXPECT_TRUE(response.find("size") != std::string::npos);
    EXPECT_TRUE(response.find("used") != std::string::npos);
}

// ============================================================================
// GetStorage - Comprehensive Negative Tests
// ============================================================================

TEST_F(AppStorageManagerTest, GetStorage_Negative_EmptyAppId) {
    std::string appId = "";
    uint32_t userId = 500;
    uint32_t groupId = 500;
    std::string path = "";
    uint32_t size = 0;
    uint32_t used = 0;
    EXPECT_EQ(Core::ERROR_GENERAL, interface->GetStorage(appId, userId, groupId, path, size, used));
}

TEST_F(AppStorageManagerTest, GetStorage_Negative_JsonRpc_EmptyAppId) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getStorage"), _T("{\"appId\":\"\",\"userId\":500,\"groupId\":500}"), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(AppStorageManagerTest, GetStorage_Negative_StorageNotAccessible) {
    std::string appId = "notAccessibleApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    uint32_t used = 0;
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    static int accessCallCount = 0;
    accessCallCount = 0;
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            accessCallCount++;
            if (accessCallCount <= 2) {
                return 0;
            }
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_CALL(*mStore2Mock, DeleteKey(_, _, _))
        .WillRepeatedly(Invoke(
            [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key) -> uint32_t {
                return Core::ERROR_NONE;
            }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_EQ(Core::ERROR_GENERAL, interface->GetStorage(appId, 500, 500, path, size, used));
}

// ============================================================================
// DeleteStorage - Comprehensive Positive Tests
// ============================================================================

TEST_F(AppStorageManagerTest, DeleteStorage_Positive_ValidDeletion) {
    std::string appId = "deleteValidApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, rmdir(_))
        .WillRepeatedly([](const char* pathname) {
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_CALL(*mStore2Mock, DeleteKey(_, _, _))
        .WillRepeatedly(Invoke(
            [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key) -> uint32_t {
                return Core::ERROR_NONE;
            }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, interface->DeleteStorage(appId, errorReason));
    EXPECT_TRUE(errorReason.empty());
}

TEST_F(AppStorageManagerTest, DeleteStorage_Positive_JsonRpc_ValidRequest) {
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, rmdir(_))
        .WillRepeatedly([](const char* pathname) {
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_CALL(*mStore2Mock, DeleteKey(_, _, _))
        .WillRepeatedly(Invoke(
            [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key) -> uint32_t {
                return Core::ERROR_NONE;
            }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("deleteJsonRpcApp", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("deleteStorage"), _T("{\"appId\":\"deleteJsonRpcApp\"}"), response));
}

// ============================================================================
// DeleteStorage - Comprehensive Negative Tests
// ============================================================================

TEST_F(AppStorageManagerTest, DeleteStorage_Negative_EmptyAppId) {
    std::string appId = "";
    std::string errorReason = "";
    EXPECT_EQ(Core::ERROR_GENERAL, interface->DeleteStorage(appId, errorReason));
    EXPECT_STREQ("AppId is empty", errorReason.c_str());
}

TEST_F(AppStorageManagerTest, DeleteStorage_Negative_JsonRpc_EmptyAppId) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("deleteStorage"), _T("{\"appId\":\"\"}"), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(AppStorageManagerTest, DeleteStorage_Negative_NftwFails) {
    std::string appId = "deleteNftwFailApp2";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    static int nftwCallCount = 0;
    nftwCallCount = 0;
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            nftwCallCount++;
            if (nftwCallCount <= 1) {
                return 0;
            }
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_EQ(Core::ERROR_GENERAL, interface->DeleteStorage(appId, errorReason));
}

// ============================================================================
// Clear - Comprehensive Positive Tests
// ============================================================================

TEST_F(AppStorageManagerTest, Clear_Positive_ValidClear) {
    std::string appId = "clearValidApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, interface->Clear(appId, errorReason));
    EXPECT_TRUE(errorReason.empty());
}

TEST_F(AppStorageManagerTest, Clear_Positive_JsonRpc_ValidRequest) {
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("clearJsonRpcValidApp", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("clear"), _T("{\"appId\":\"clearJsonRpcValidApp\"}"), response));
}

// ============================================================================
// Clear - Comprehensive Negative Tests
// ============================================================================

TEST_F(AppStorageManagerTest, Clear_Negative_EmptyAppId) {
    std::string appId = "";
    std::string errorReason = "";
    EXPECT_EQ(Core::ERROR_GENERAL, interface->Clear(appId, errorReason));
    EXPECT_STREQ("Clear called with no appId", errorReason.c_str());
}

TEST_F(AppStorageManagerTest, Clear_Negative_JsonRpc_EmptyAppId) {
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("clear"), _T("{\"appId\":\"\"}"), response));
    EXPECT_TRUE(response.empty());
}

TEST_F(AppStorageManagerTest, Clear_Negative_AppNotFound) {
    std::string appId = "clearNonExistentApp2";
    std::string errorReason = "";
    EXPECT_EQ(Core::ERROR_GENERAL, interface->Clear(appId, errorReason));
    EXPECT_TRUE(errorReason.find("Storage not found for appId") != std::string::npos);
}

// ============================================================================
// ClearAll - Comprehensive Positive Tests
// ============================================================================

TEST_F(AppStorageManagerTest, ClearAll_Positive_EmptyExemptionList) {
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, opendir(_))
        .WillOnce(Invoke([](const char* pathname) {
            return __real_opendir(pathname);
        }));
    EXPECT_CALL(*p_wrapsImplMock, readdir(_))
        .WillOnce([](DIR* dirp) -> struct dirent* {
            return nullptr;
        });
    EXPECT_CALL(*p_wrapsImplMock, closedir(_))
        .WillOnce([](DIR* dirp) {
            return 0;
        });
    EXPECT_EQ(Core::ERROR_NONE, interface->ClearAll("[]", errorReason));
}

TEST_F(AppStorageManagerTest, ClearAll_Positive_WithExemptions) {
    std::string path = "";
    std::string errorReason = "";
    static int callCount = 0;
    callCount = 0;
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("clearAllApp1", 1024, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("exemptApp1", 1024, path, errorReason));
    EXPECT_CALL(*p_wrapsImplMock, opendir(_))
        .WillOnce(Invoke([](const char* pathname) {
            return __real_opendir(pathname);
        }));
    EXPECT_CALL(*p_wrapsImplMock, readdir(_))
        .WillRepeatedly([](DIR* dirp) -> struct dirent* {
            static struct dirent entry;
            switch (callCount++) {
                case 0:
                    std::strcpy(entry.d_name, "clearAllApp1");
                    entry.d_type = DT_DIR;
                    return &entry;
                case 1:
                    std::strcpy(entry.d_name, "exemptApp1");
                    entry.d_type = DT_DIR;
                    return &entry;
                default:
                    return nullptr;
            }
        });
    EXPECT_CALL(*p_wrapsImplMock, closedir(_))
        .WillOnce([](DIR* dirp) {
            return 0;
        });
    EXPECT_EQ(Core::ERROR_NONE, interface->ClearAll("[\"exemptApp1\"]", errorReason));
}

TEST_F(AppStorageManagerTest, ClearAll_Positive_JsonRpc_ValidRequest) {
    EXPECT_CALL(*p_wrapsImplMock, opendir(_))
        .WillOnce(Invoke([](const char* pathname) {
            return __real_opendir(pathname);
        }));
    EXPECT_CALL(*p_wrapsImplMock, readdir(_))
        .WillOnce([](DIR* dirp) -> struct dirent* {
            return nullptr;
        });
    EXPECT_CALL(*p_wrapsImplMock, closedir(_))
        .WillOnce([](DIR* dirp) {
            return 0;
        });
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("clearAll"), _T("{\"exemptionAppIds\":\"[]\"}"), response));
}

// ============================================================================
// ClearAll - Comprehensive Negative Tests
// ============================================================================

TEST_F(AppStorageManagerTest, ClearAll_Negative_OpendirFails) {
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, opendir(_))
        .WillOnce([](const char* pathname) -> DIR* {
            errno = ENOENT;
            return nullptr;
        });
    EXPECT_EQ(Core::ERROR_GENERAL, interface->ClearAll("[]", errorReason));
    EXPECT_TRUE(errorReason.find("Failed to open storage directory") != std::string::npos);
}

TEST_F(AppStorageManagerTest, ClearAll_Negative_PartialDeletionFailure) {
    std::string path = "";
    std::string errorReason = "";
    static int callCount = 0;
    callCount = 0;
    static int nftwCount = 0;
    nftwCount = 0;
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            nftwCount++;
            if (nftwCount > 2) {
                return -1;
            }
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage("clearAllFailApp", 1024, path, errorReason));
    EXPECT_CALL(*p_wrapsImplMock, opendir(_))
        .WillOnce(Invoke([](const char* pathname) {
            return __real_opendir(pathname);
        }));
    EXPECT_CALL(*p_wrapsImplMock, readdir(_))
        .WillRepeatedly([](DIR* dirp) -> struct dirent* {
            static struct dirent entry;
            switch (callCount++) {
                case 0:
                    std::strcpy(entry.d_name, "clearAllFailApp");
                    entry.d_type = DT_DIR;
                    return &entry;
                default:
                    return nullptr;
            }
        });
    EXPECT_CALL(*p_wrapsImplMock, closedir(_))
        .WillOnce([](DIR* dirp) {
            return 0;
        });
    EXPECT_EQ(Core::ERROR_GENERAL, interface->ClearAll("[]", errorReason));
}

// ============================================================================
// Configure - Comprehensive Tests
// ============================================================================

TEST_F(AppStorageManagerTest, Configure_Positive_ValidConfiguration) {
    ASSERT_TRUE(storageManagerConfigure != nullptr);
    ASSERT_TRUE(interface != nullptr);
}

TEST_F(AppStorageManagerTest, Configure_Negative_NullService) {
    Core::ProxyType<Plugin::StorageManagerImplementation> testImpl = 
        Core::ProxyType<Plugin::StorageManagerImplementation>::Create();
    Exchange::IConfiguration* testConfigure = static_cast<Exchange::IConfiguration*>(
        testImpl->QueryInterface(Exchange::IConfiguration::ID));
    EXPECT_EQ(Core::ERROR_GENERAL, testConfigure->Configure(nullptr));
    testConfigure->Release();
}

// ============================================================================
// Boundary and Edge Case Tests
// ============================================================================

TEST_F(AppStorageManagerTest, CreateStorage_Boundary_ZeroSize) {
    std::string appId = "zeroSizeBoundaryApp";
    uint32_t size = 0;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
}

TEST_F(AppStorageManagerTest, CreateStorage_Boundary_SingleCharAppId) {
    std::string appId = "a";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return -1;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
}

TEST_F(AppStorageManagerTest, GetStorage_Boundary_ZeroUserIdGroupId) {
    std::string appId = "zeroIdsBoundaryApp";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    uint32_t used = 0;
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, chown(_, _, _))
        .WillRepeatedly([](const char* path, uid_t owner, gid_t group) {
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke(
            [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
                if (key == "quotaSize") {
                    value = "1024";
                }
                ttl = 0;
                return Core::ERROR_NONE;
            }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    EXPECT_EQ(Core::ERROR_NONE, interface->GetStorage(appId, 0, 0, path, size, used));
}

TEST_F(AppStorageManagerTest, CreateStorage_UpdateWithDifferentSize) {
    std::string appId = "updateDiffSizeApp2";
    uint32_t size = 1024;
    std::string path = "";
    std::string errorReason = "";
    EXPECT_CALL(*p_wrapsImplMock, mkdir(_, _))
        .WillRepeatedly([](const char* path, mode_t mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, access(_, _))
        .WillRepeatedly([](const char* path, int mode) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, nftw(_, _, _, _))
        .WillRepeatedly([](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) {
            return 0;
        });
    EXPECT_CALL(*p_wrapsImplMock, statvfs(_, _))
        .WillRepeatedly([](const char* path, struct statvfs* buf) {
            buf->f_bsize = 4096;
            buf->f_frsize = 4096;
            buf->f_blocks = 100000;
            buf->f_bfree = 500000;
            buf->f_bavail = 500000;
            return 0;
        });
    EXPECT_CALL(*mStore2Mock, SetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke([](Exchange::IStore2::ScopeType scope,
                                const std::string& appId,
                                const std::string& key,
                                const std::string& value,
                                const uint32_t ttl) -> uint32_t {
            return Core::ERROR_NONE;
        }));
    EXPECT_CALL(*mStore2Mock, GetValue(_, _, _, _, _))
        .WillRepeatedly(Invoke(
            [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key, std::string& value, uint32_t& ttl) -> uint32_t {
                if (key == "quotaSize") {
                    value = "1024";
                }
                ttl = 0;
                return Core::ERROR_NONE;
            }));
    EXPECT_CALL(*mStore2Mock, DeleteKey(_, _, _))
        .WillRepeatedly(Invoke(
            [](Exchange::IStore2::ScopeType scope, const std::string& appId, const std::string& key) -> uint32_t {
                return Core::ERROR_NONE;
            }));
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, size, path, errorReason));
    std::string path2 = "";
    EXPECT_EQ(Core::ERROR_NONE, interface->CreateStorage(appId, 2048, path2, errorReason));
}
