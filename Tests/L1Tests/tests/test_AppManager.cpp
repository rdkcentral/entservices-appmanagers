/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2024 RDK Management
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
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <mntent.h>
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>
#include <mutex>

#include "AppManager.h"
#include "AppManagerImplementation.h"
#include "AppInfoManager.h"
#include "ServiceMock.h"
#include "LifecycleManagerMock.h"
#include "PackageManagerMock.h"
#include "StorageManagerMock.h"
#include "Store2Mock.h"
#include "COMLinkMock.h"
#include "ThunderPortability.h"
#include "Module.h"
#include "WorkerPoolImplementation.h"
#include "WrapsMock.h"
#include "FactoriesImplementation.h"


#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

#define TIMEOUT   (50000)
#define DRAIN_TIMEOUT   (500)
#define JOB_DRAIN_TIMEOUT   (10000)
#define APPMANAGER_APP_ID           "com.test.app"
#define APPMANAGER_EMPTY_APP_ID     ""
#define APPMANAGER_APP_VERSION      "1.2.8"
#define APPMANAGER_APP_DIGEST       ""
#define APPMANAGER_APP_STATE        Exchange::IPackageInstaller::InstallState::INSTALLED
#define APPMANAGER_APP_STATE_STR    "INTERACTIVE_APP"
#define APPMANAGER_APP_SIZE         0
#define APPMANAGER_WRONG_APP_ID     "com.wrongtest.app"
#define APPMANAGER_APP_INTENT       "test.intent"
#define APPMANAGER_APP_LAUNCHARGS   "test.arguments"
#define APPMANAGER_APP_INSTANCE     "testAppInstance"
#define APPMANAGER_APP_UNPACKEDPATH "/media/apps/sky/packages/Hulu/data.img"
#define PERSISTENT_STORE_KEY        "DUMMY"
#define PERSISTENT_STORE_VALUE      "DUMMY_VALUE"
#define APPMANAGER_PACKAGEID        "testPackageID"
#define APPMANAGER_INSTALLSTATUS_INSTALLED    "INSTALLED"
#define APPMANAGER_INSTALLSTATUS_UNINSTALLED    "UNINSTALLED"
#define TEST_JSON_INSTALLED_PACKAGE R"([{"packageId":"YouTube","version":"100.1.30+rialto","state":"INSTALLED"}])"

typedef enum : uint32_t {
    AppManager_StateInvalid = 0x00000000,
    AppManager_onAppLifecycleStateChanged = 0x00000001,
    AppManager_onAppInstalled = 0x00000002,
    AppManager_onAppLaunchRequest = 0x00000004,
    AppManager_onAppUnloaded = 0x00000008
} AppManagerL1test_async_events_t;

struct ExpectedAppLifecycleEvent {
    std::string appId;
    std::string appInstanceId;
    std::string intent;
    std::string version;
    std::string source;
    Exchange::IAppManager::AppLifecycleState newState;
    Exchange::IAppManager::AppLifecycleState oldState;
    Exchange::IAppManager::AppErrorReason errorReason;
};
using ::testing::NiceMock;
using namespace WPEFramework;
using namespace WPEFramework::Plugin;

namespace {
const string callSign = _T("AppManager");
}
class AppManagerTest : public ::testing::Test {
protected:
    ServiceMock* mServiceMock = nullptr;
    LifecycleManagerMock* mLifecycleManagerMock = nullptr;
    LifecycleManagerStateMock* mLifecycleManagerStateMock = nullptr;
    PackageManagerMock* mPackageManagerMock = nullptr;
    PackageInstallerMock* mPackageInstallerMock = nullptr;
    Store2Mock* mStore2Mock = nullptr;
    StorageManagerMock* mStorageManagerMock = nullptr;
    WrapsImplMock *p_wrapsImplMock   = nullptr;
    Core::JSONRPC::Message message;
    FactoriesImplementation factoriesImplementation;
    PLUGINHOST_DISPATCHER *dispatcher;


    Core::ProxyType<Plugin::AppManager> plugin;
    Plugin::AppManagerImplementation *mAppManagerImpl;
    Exchange::IPackageInstaller::INotification* mPackageManagerNotification_cb = nullptr;
    Exchange::ILifecycleManagerState::INotification* mLifecycleManagerStateNotification_cb = nullptr;
    Exchange::IAppManager::INotification* mAppManagerNotification = nullptr;

    Core::ProxyType<WorkerPoolImplementation> workerPool;
    Core::JSONRPC::Handler& mJsonRpcHandler;
    DECL_CORE_JSONRPC_CONX connection;
    string mJsonRpcResponse;
    std::mutex mPreLoadMutex;
    std::condition_variable mPreLoadCV;
    bool mPreLoadSpawmCalled = false;

    void createAppManagerImpl()
    {
        mServiceMock = new NiceMock<ServiceMock>;

        TEST_LOG("In createAppManagerImpl!");
        EXPECT_EQ(string(""), plugin->Initialize(mServiceMock));
        mAppManagerImpl = Plugin::AppManagerImplementation::getInstance();
    }

    void releaseAppManagerImpl()
    {
        TEST_LOG("In releaseAppManagerImpl!");
        AppInfoManager::getInstance().clear();
        plugin->Deinitialize(mServiceMock);
        delete mServiceMock;
        mAppManagerImpl = nullptr;
    }

    Core::hresult createResources()
    {
        Core::hresult status = Core::ERROR_GENERAL;
        mServiceMock = new NiceMock<ServiceMock>;
        mLifecycleManagerMock = new NiceMock<LifecycleManagerMock>;
        mLifecycleManagerStateMock = new NiceMock<LifecycleManagerStateMock>;
        mPackageManagerMock = new NiceMock<PackageManagerMock>;
        mPackageInstallerMock = new NiceMock<PackageInstallerMock>;
        mStorageManagerMock = new NiceMock<StorageManagerMock>;
        mStore2Mock = new NiceMock<Store2Mock>;
        p_wrapsImplMock  = new NiceMock <WrapsImplMock>;
        Wraps::setImpl(p_wrapsImplMock);

        PluginHost::IFactories::Assign(&factoriesImplementation);
        dispatcher = static_cast<PLUGINHOST_DISPATCHER*>(
        plugin->QueryInterface(PLUGINHOST_DISPATCHER_ID));
        dispatcher->Activate(mServiceMock);
        TEST_LOG("In createResources!");
        EXPECT_CALL(*mServiceMock, QueryInterfaceByCallsign(::testing::_, ::testing::_))
          .Times(::testing::AnyNumber())
          .WillRepeatedly(::testing::Invoke(
              [&](const uint32_t id, const std::string& name) -> void* {
                if (name == "org.rdk.LifecycleManager") {
                    if(id == Exchange::ILifecycleManager::ID) {
                        return reinterpret_cast<void*>(mLifecycleManagerMock);
                    }
                    else if(id == Exchange::ILifecycleManagerState::ID) {
                        return reinterpret_cast<void*>(mLifecycleManagerStateMock);
                    }
                } else if (name == "org.rdk.PersistentStore") {
                   return reinterpret_cast<void*>(mStore2Mock);
                } else if (name == "org.rdk.AppStorageManager") {
                    return reinterpret_cast<void*>(mStorageManagerMock);
                } else if (name == "org.rdk.PackageManagerRDKEMS") {
                    if (id == Exchange::IPackageHandler::ID) {
                        return reinterpret_cast<void*>(mPackageManagerMock);
                    }
                    else if (id == Exchange::IPackageInstaller::ID){
                        return reinterpret_cast<void*>(mPackageInstallerMock);
                    }
                }
            return nullptr;
        }));

        EXPECT_CALL(*mPackageInstallerMock, Register(::testing::_))
            .WillOnce(::testing::Invoke(
                [&](Exchange::IPackageInstaller::INotification* notification) {
                    mPackageManagerNotification_cb = notification;
                    return Core::ERROR_NONE;
                }));

        ON_CALL(*mLifecycleManagerStateMock, Register(::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](Exchange::ILifecycleManagerState::INotification* notification) {
                    mLifecycleManagerStateNotification_cb = notification;
                    return Core::ERROR_NONE;
                }));

        ON_CALL(*p_wrapsImplMock, stat(::testing::_, ::testing::_))
        .WillByDefault(::testing::Return(-1));
        
        EXPECT_EQ(string(""), plugin->Initialize(mServiceMock));
        mAppManagerImpl = Plugin::AppManagerImplementation::getInstance();
        TEST_LOG("createResources - All done!");
        status = Core::ERROR_NONE;

        return status;
    }

    void releaseResources()
    {
        TEST_LOG("In releaseResources!");

        if (mLifecycleManagerStateMock != nullptr && mLifecycleManagerStateNotification_cb != nullptr)
        {
            ON_CALL(*mLifecycleManagerStateMock, Unregister(::testing::_))
                .WillByDefault(::testing::Invoke([&]() {
                    return 0;
                }));
            mLifecycleManagerStateNotification_cb = nullptr;
        }
        if (mPackageInstallerMock != nullptr && mPackageManagerNotification_cb != nullptr)
        {
            ON_CALL(*mPackageInstallerMock, Unregister(::testing::_))
                .WillByDefault(::testing::Invoke([&]() {
                    return 0;
                }));
            mPackageManagerNotification_cb = nullptr;
        }

        if (mLifecycleManagerMock != nullptr)
        {
            EXPECT_CALL(*mLifecycleManagerMock, Release())
                .WillOnce(::testing::Invoke(
                [&]() {
                     delete mLifecycleManagerMock;
                     return 0;
                    }));
        }
        if (mLifecycleManagerStateMock != nullptr)
        {
            EXPECT_CALL(*mLifecycleManagerStateMock, Unregister(::testing::_))
                .WillOnce(::testing::Invoke(
                [&]() {
                     return 0;
                    }));
            EXPECT_CALL(*mLifecycleManagerStateMock, Release())
                .WillOnce(::testing::Invoke(
                [&]() {
                     delete mLifecycleManagerStateMock;
                     return 0;
                    }));
        }
        if (mPackageManagerMock != nullptr)
        {
            EXPECT_CALL(*mPackageManagerMock, Release())
                .WillOnce(::testing::Invoke(
                [&]() {
                     delete mPackageManagerMock;
                     return 0;
                    }));
        }
        if (mPackageInstallerMock != nullptr)
        {
            EXPECT_CALL(*mPackageInstallerMock, Release())
                .WillOnce(::testing::Invoke(
                [&]() {
                     delete mPackageInstallerMock;
                     return 0;
                    }));
        }
        if (mStore2Mock != nullptr)
        {
            EXPECT_CALL(*mStore2Mock, Release())
                .WillOnce(::testing::Invoke(
                [&]() {
                     delete mStore2Mock;
                     return 0;
                    }));
        }

        Wraps::setImpl(nullptr);
        if (p_wrapsImplMock != nullptr)
        {
            delete p_wrapsImplMock;
            p_wrapsImplMock = nullptr;
        }

        if (mStorageManagerMock != nullptr)
        {
            EXPECT_CALL(*mStorageManagerMock, Release())
                .WillOnce(::testing::Invoke(
                [&]() {
                     delete mStorageManagerMock;
                     return 0;
            })); 
        }
        dispatcher->Deactivate();
        dispatcher->Release();

        AppInfoManager::getInstance().clear();
        plugin->Deinitialize(mServiceMock);
        delete mServiceMock;
        mAppManagerImpl = nullptr;
    }
    AppManagerTest()
        : plugin(Core::ProxyType<Plugin::AppManager>::Create()),
        workerPool(Core::ProxyType<WorkerPoolImplementation>::Create(2, Core::Thread::DefaultStackSize(), 16)),
        mJsonRpcHandler(*plugin),
        INIT_CONX(1, 0)
    {
        Core::IWorkerPool::Assign(&(*workerPool));
        workerPool->Run();
    }

    virtual ~AppManagerTest() override
    {
        TEST_LOG("Delete ~AppManagerTest Instance!");
        Core::IWorkerPool::Assign(nullptr);
        workerPool.Release();
    }
    std::string GetPackageInfoInJSON()
    {
        std::string apps_str = "";
        JsonObject package;
        JsonArray installedAppsArray;

        package["appId"] = APPMANAGER_APP_ID;
        package["versionString"] = APPMANAGER_APP_VERSION;
        package["type"] = APPMANAGER_APP_STATE_STR;
        package["lastActiveTime"] = "";
        package["lastActiveIndex"] = "";

        installedAppsArray.Add(package);
        installedAppsArray.ToString(apps_str);

        return apps_str;
    }

     auto FillPackageIterator()
    {
        std::list<Exchange::IPackageInstaller::Package> packageList;
        Exchange::IPackageInstaller::Package package_1;

        package_1.packageId = APPMANAGER_APP_ID;
        package_1.version = APPMANAGER_APP_VERSION;
        package_1.digest = APPMANAGER_APP_DIGEST;
        package_1.state = APPMANAGER_APP_STATE;
        package_1.sizeKb = APPMANAGER_APP_SIZE;

        packageList.emplace_back(package_1);
        return Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
    }

     auto FillLoadedAppsIterator()
    {
        std::list<WPEFramework::Exchange::IAppManager::LoadedAppInfo> loadedAppInfoList;

        WPEFramework::Exchange::IAppManager::LoadedAppInfo app_1, app_2;
        app_1.appId = "NexTennis";
        app_1.appInstanceId = "0295effd-2883-44ed-b614-471e3f682762";
        app_1.activeSessionId = "";
        app_1.targetLifecycleState = WPEFramework::Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE; 
        app_1.lifecycleState = WPEFramework::Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE;

        app_2.appId = "uktv";
        app_2.appInstanceId = "67fa75b6-0c85-43d4-a591-fd29e7214be5";
        app_2.activeSessionId = "";
        app_2.targetLifecycleState = WPEFramework::Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE; 
        app_2.lifecycleState = WPEFramework::Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE;

        loadedAppInfoList.emplace_back(app_1);
        loadedAppInfoList.emplace_back(app_2);
        return Core::Service<RPC::IteratorType<Exchange::IAppManager::ILoadedAppInfoIterator>>::Create<Exchange::IAppManager::ILoadedAppInfoIterator>(loadedAppInfoList);
    }

    void LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState state)
    {
        const std::string launchArgs = APPMANAGER_APP_LAUNCHARGS;
        TEST_LOG("LaunchAppPreRequisite with state: %d", state);

        EXPECT_CALL(*mPackageInstallerMock, ListPackages(::testing::_))
        .WillRepeatedly([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
            auto mockIterator = FillPackageIterator(); // Fill the package Info
            packages = mockIterator;
            return Core::ERROR_NONE;
        });

        EXPECT_CALL(*mPackageManagerMock, Lock(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly([&](const string &packageId, const string &version, const Exchange::IPackageHandler::LockReason &lockReason, uint32_t &lockId /* @out */, string &unpackedPath /* @out */, Exchange::RuntimeConfig &configMetadata /* @out */, Exchange::IPackageHandler::ILockIterator*& appMetadata /* @out */) {
            lockId = 1;
            unpackedPath = APPMANAGER_APP_UNPACKEDPATH;
            return Core::ERROR_NONE;
        });

        EXPECT_CALL(*mLifecycleManagerMock, IsAppLoaded(::testing::_, ::testing::_))
        .WillRepeatedly([&](const std::string &appId, bool &loaded) {
            loaded = true;
            return Core::ERROR_NONE;
        });

        EXPECT_CALL(*mLifecycleManagerMock, SetTargetAppState(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly([&](const string& appInstanceId , const Exchange::ILifecycleManager::LifecycleState targetLifecycleState , const string& launchIntent) {
            return Core::ERROR_NONE;
        });
        EXPECT_CALL(*mLifecycleManagerMock, SpawnApp(APPMANAGER_APP_ID, ::testing::_, ::testing::_, ::testing::_, launchArgs, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce([&](const string& appId, const string& launchIntent, const Exchange::ILifecycleManager::LifecycleState targetLifecycleState,
            const Exchange::RuntimeConfig& runtimeConfigObject, const string& launchArgs, string& appInstanceId, string& errorReason, bool& success) {
            appInstanceId = APPMANAGER_APP_INSTANCE;
            errorReason = "";
            success = true;
            return Core::ERROR_NONE;
        });
    }

    void preLaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState state)
    {
        const std::string launchArgs = APPMANAGER_APP_LAUNCHARGS;
        TEST_LOG("LaunchAppPreRequisite with state: %d", state);

        EXPECT_CALL(*mPackageInstallerMock, ListPackages(::testing::_))
        .WillRepeatedly([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
            auto mockIterator = FillPackageIterator(); // Fill the package Info
            packages = mockIterator;
            return Core::ERROR_NONE;
        });

        EXPECT_CALL(*mPackageManagerMock, Lock(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly([&](const string &packageId, const string &version, const Exchange::IPackageHandler::LockReason &lockReason, uint32_t &lockId /* @out */, string &unpackedPath /* @out */, Exchange::RuntimeConfig &configMetadata /* @out */, Exchange::IPackageHandler::ILockIterator*& appMetadata /* @out */) {
            lockId = 1;
            unpackedPath = APPMANAGER_APP_UNPACKEDPATH;
            return Core::ERROR_NONE;
        });

        EXPECT_CALL(*mLifecycleManagerMock, IsAppLoaded(::testing::_, ::testing::_))
        .WillRepeatedly([&](const std::string &appId, bool &loaded) {
            loaded = true;
            return Core::ERROR_NONE;
        });

        EXPECT_CALL(*mLifecycleManagerMock, SetTargetAppState(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly([&](const string& appInstanceId , const Exchange::ILifecycleManager::LifecycleState targetLifecycleState , const string& launchIntent) {
            return Core::ERROR_NONE;
        });
        EXPECT_CALL(*mLifecycleManagerMock, SpawnApp(APPMANAGER_APP_ID, ::testing::_, ::testing::_, ::testing::_, launchArgs, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce([&](const string& appId, const string& launchIntent, const Exchange::ILifecycleManager::LifecycleState targetLifecycleState,
            const Exchange::RuntimeConfig& runtimeConfigObject, const string& launchArgs, string& appInstanceId, string& errorReason, bool& success) {
            {
                std::lock_guard<std::mutex> lock(mPreLoadMutex);
                mPreLoadSpawmCalled = true;
            }
            appInstanceId = APPMANAGER_APP_INSTANCE;
            errorReason = "";
            success = true;
            mPreLoadCV.notify_one();
            return Core::ERROR_NONE;
        });
    }

    void SetupUnloadApp()
    {
        EXPECT_CALL(*mLifecycleManagerMock, UnloadApp(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly([&](const string& appInstanceId, string& errorReason, bool& success) {
            success = true;
            return Core::ERROR_NONE;
        });
    }
};

class NotificationHandler : public Exchange::IAppManager::INotification {
    private:

        BEGIN_INTERFACE_MAP(Notification)
        INTERFACE_ENTRY(Exchange::IAppManager::INotification)
        END_INTERFACE_MAP

    public:
        /** @brief Mutex */
        std::mutex m_mutex;

        /** @brief Condition variable */
        std::condition_variable m_condition_variable;

        /** @brief Event signalled flag */
        uint32_t m_event_signalled = AppManager_StateInvalid;

        ExpectedAppLifecycleEvent m_expectedEvent;
        NotificationHandler(){}
        ~NotificationHandler(){}
        void SetExpectedEvent(const ExpectedAppLifecycleEvent& expectedEvent)
        {
            m_expectedEvent = expectedEvent;
        }

        void OnAppLifecycleStateChanged(const string& appId, const string& appInstanceId, const Exchange::IAppManager::AppLifecycleState newState, const Exchange::IAppManager::AppLifecycleState oldState, const Exchange::IAppManager::AppErrorReason errorReason)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_event_signalled |= AppManager_onAppLifecycleStateChanged;
            EXPECT_EQ(m_expectedEvent.appId, appId);
            EXPECT_EQ(m_expectedEvent.appInstanceId, appInstanceId);
            EXPECT_EQ(m_expectedEvent.newState, newState);
            EXPECT_EQ(m_expectedEvent.oldState, oldState);
            EXPECT_EQ(m_expectedEvent.errorReason, errorReason);

            m_condition_variable.notify_one();
        }

        void OnAppInstalled(const string& appId, const string& version)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            EXPECT_STREQ(m_expectedEvent.appId.c_str(), appId.c_str());
            EXPECT_STREQ(m_expectedEvent.version.c_str(), version.c_str());
            m_event_signalled |= AppManager_onAppInstalled;

            m_condition_variable.notify_one();
        }

        void OnAppLaunchRequest(const string& appId, const string& intent, const string& source)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            EXPECT_STREQ(m_expectedEvent.appId.c_str(), appId.c_str());
            EXPECT_STREQ(m_expectedEvent.intent.c_str(), intent.c_str());
            EXPECT_STREQ(m_expectedEvent.source.c_str(), source.c_str());
            m_event_signalled |= AppManager_onAppLaunchRequest;

            m_condition_variable.notify_one();
        }

        void OnAppUnloaded(const string& appId, const string& appInstanceId)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            EXPECT_STREQ(m_expectedEvent.appId.c_str(), appId.c_str());
            EXPECT_STREQ(m_expectedEvent.appInstanceId.c_str(), appInstanceId.c_str());
            m_event_signalled |= AppManager_onAppUnloaded;

            m_condition_variable.notify_one();
        }

        uint32_t WaitForRequestStatus(uint32_t timeout_ms, AppManagerL1test_async_events_t expected_status)
        {
            uint32_t signalled = AppManager_StateInvalid;
            std::unique_lock<std::mutex> lock(m_mutex);
            auto now = std::chrono::steady_clock::now();
            auto timeout = std::chrono::milliseconds(timeout_ms);
            while (!(expected_status & m_event_signalled))
            {
              if (m_condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout)
              {
                 TEST_LOG("Timeout waiting for request status event");
                 break;
              }
            }
            signalled = m_event_signalled;
            m_event_signalled = AppManager_StateInvalid; // reset for next use
            return signalled;
        }
    };

/*******************************************************************************************************************
 * Test Case for RegisteredMethodsUsingJsonRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Verifying whether all methods exists or not
 * Releasing the AppManager interface and all related test resources
 ********************************************************************************************************************/
TEST_F(AppManagerTest, RegisteredMethodsUsingJsonRpcSuccess)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("getInstalledApps")));
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("isInstalled")));

    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("getLoadedApps")));
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("launchApp")));
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("preloadApp")));
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("closeApp")));
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("terminateApp")));
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("startSystemApp")));
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("stopSystemApp")));
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("killApp")));
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("sendIntent")));
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("clearAppData")));
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("clearAllAppData")));
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("getAppMetadata")));
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("getAppProperty")));
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("setAppProperty")));
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("getMaxRunningApps")));
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("getMaxHibernatedApps")));
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("getMaxHibernatedFlashUsage")));
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Exists(_T("getMaxInactiveRamUsage")));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for GetInstalledAppsUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Calling FillPackageIterator() to fill one package info in the package iterator
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Verifying the return of the API
 * Verifying whether it returns the mocked package list filled or not
 * Releasing the AppManager Interface object and all related test resources
 */
TEST_F(AppManagerTest, GetInstalledAppsUsingComRpcSuccess)
{
    Core::hresult status;
    std::string apps = "";
    std::string jsonStr = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(*mPackageInstallerMock, ListPackages(::testing::_))
    .WillRepeatedly([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
        auto mockIterator = FillPackageIterator(); // Fill the package Info
        packages = mockIterator;
        return Core::ERROR_NONE;
    });

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->GetInstalledApps(apps));
    jsonStr = GetPackageInfoInJSON();
    EXPECT_STREQ(jsonStr.c_str(), apps.c_str());

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* * Test Case for GetInstalledAppsUsingJSONRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Calling FillPackageIterator() to fill one package info in the package iterator
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Verifying the return of the API
 * Verifying whether it returns the mocked package list filled or not
 * Releasing the AppManager Interface object and all related test resources
 */
TEST_F(AppManagerTest, GetInstalledAppsUsingJSONRpcSuccess)
{
    Core::hresult status;
    std::string apps = "";
    std::string jsonStr = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_CALL(*mPackageInstallerMock, ListPackages(::testing::_))
    .WillRepeatedly([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
        auto mockIterator = FillPackageIterator(); // Fill the package Info
        packages = mockIterator;
        return Core::ERROR_NONE;
    });
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("getInstalledApps"), _T("{\"apps\": \"\"}"), mJsonRpcResponse));
    jsonStr = GetPackageInfoInJSON();
    EXPECT_STREQ(
    "[{\"appId\":\"com.test.app\",\"versionString\":\"1.2.8\",\"type\":\"INTERACTIVE_APP\",\"lastActiveTime\":\"\",\"lastActiveIndex\":\"\"}]",
    mJsonRpcResponse.c_str());

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }

}

/*
 * Test Case for GetInstalledAppsUsingComRpcFailurePackageManagerObjectIsNull
 * Setting up only AppManager Plugin and creating required COM-RPC resources
 * PackageManager Interface object is not created and hence the API should return error
 * Releasing the AppManager Interface object only
 */
TEST_F(AppManagerTest, GetInstalledAppsUsingComRpcFailurePackageManagerObjectIsNull)
{
    std::string apps = APPMANAGER_APP_ID;

    createAppManagerImpl();
    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->GetInstalledApps(apps));
    releaseAppManagerImpl();
}

/*
 * Test Case for GetInstalledAppsUsingComRpcFailurePackageListEmpty
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for ListPackages() to simulate getting empty package list
 * Verifying the return of the API
 * Releasing the AppManager Interface object and all related test resources
 */
TEST_F(AppManagerTest, GetInstalledAppsUsingComRpcFailurePackageListEmpty)
{
    Core::hresult status;
    std::string apps = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(*mPackageInstallerMock, ListPackages(::testing::_))
    .WillOnce([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
        packages = nullptr; // make Package list empty
        return Core::ERROR_NONE;
    });

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->GetInstalledApps(apps));
    EXPECT_EQ(apps, "");

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for GetInstalledAppsUsingComRpcFailureListPackagesReturnError
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Calling FillPackageIterator() to fill one package info in the package iterator
 * Setting Mock for ListPackages() to simulate error return
 * Verifying the return of the API
 * Releasing the AppManager Interface object and all related test resources
 */

TEST_F(AppManagerTest, GetInstalledAppsUsingComRpcFailureListPackagesReturnError)
{
    Core::hresult status;
    std::string apps = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(*mPackageInstallerMock, ListPackages(::testing::_))
    .WillOnce([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
        auto mockIterator = FillPackageIterator(); // Fill the package Info
        packages = mockIterator;
        return Core::ERROR_GENERAL;
    });

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->GetInstalledApps(apps));
    EXPECT_EQ(apps, "");

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for IsInstalledUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Calling FillPackageIterator() to fill one package info in the package iterator
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Verifying the return of the API as well the installed flag
 * Releasing the AppManager Interface object and all related test resources
 */
TEST_F(AppManagerTest, IsInstalledUsingComRpcSuccess)
{
    Core::hresult status;
    bool installed;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(*mPackageInstallerMock, ListPackages(::testing::_))
    .WillOnce([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
        auto mockIterator = FillPackageIterator(); // Fill the package Info
        packages = mockIterator;
        return Core::ERROR_NONE;
    });

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->IsInstalled(APPMANAGER_APP_ID, installed));
    EXPECT_EQ(installed, true);

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* * Test Case for IsInstalledUsingJSONRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Calling FillPackageIterator() to fill one package info in the package iterator
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Verifying the return of the API as well the installed flag
 * Releasing the AppManager Interface object and all related test resources
 */
TEST_F(AppManagerTest, IsInstalledUsingJSONRpcSuccess)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_CALL(*mPackageInstallerMock, ListPackages(::testing::_))
    .WillOnce([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
        auto mockIterator = FillPackageIterator(); // Fill the package Info
        packages = mockIterator;
        return Core::ERROR_NONE;
    });
    std::string request = "{\"appId\": \"" + std::string(APPMANAGER_APP_ID) + "\"}";
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("isInstalled"), request, mJsonRpcResponse));
    EXPECT_STREQ("true", mJsonRpcResponse.c_str());
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}
/*
 * Test Case for IsInstalledUsingComRpcFailureWrongAppID
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Calling FillPackageIterator() to fill one package info in the package iterator
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Verifying the return of the API as well the installed flag
 * Releasing the AppManager Interface object and all related test resources
 */

TEST_F(AppManagerTest, IsInstalledUsingComRpcFailureWrongAppID)
{
    Core::hresult status;
    bool installed;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(*mPackageInstallerMock, ListPackages(::testing::_))
    .WillOnce([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
        auto mockIterator = FillPackageIterator(); // Fill the package Info
        packages = mockIterator;
        return Core::ERROR_NONE;
    });

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->IsInstalled(APPMANAGER_WRONG_APP_ID, installed));
    EXPECT_EQ(installed, false);

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for IsInstalledUsingComRpcFailurePackageListEmpty
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Verifying the return of the API as well the installed flag
 * Releasing the AppManager Interface object and all related test resources
 */
TEST_F(AppManagerTest, IsInstalledUsingComRpcFailurePackageListEmpty)
{
    Core::hresult status;
    bool installed;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    // When package list is empty
    EXPECT_CALL(*mPackageInstallerMock, ListPackages(::testing::_))
    .WillOnce([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
        packages = nullptr;
        return Core::ERROR_NONE;
    });

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->IsInstalled(APPMANAGER_APP_ID, installed));
    EXPECT_EQ(installed, false);

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for IsInstalledUsingComRpcFailureEmptyAppID
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Calling FillPackageIterator() to fill one package info in the package iterator
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Verifying the return of the API as well the installed flag
 * Releasing the AppManager Interface object and all related test resources
 */

TEST_F(AppManagerTest, IsInstalledUsingComRpcFailureEmptyAppID)
{
    Core::hresult status;
    bool installed;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->IsInstalled("", installed));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for IsInstalledUsingComRpcFailureListPackagesReturnError
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Calling FillPackageIterator() to fill one package info in the package iterator
 * Setting Mock for ListPackages() to simulate error return
 * Releasing the AppManager Interface object and all related test resources
 */
TEST_F(AppManagerTest, IsInstalledUsingComRpcFailureListPackagesReturnError)
{
    Core::hresult status;
    bool installed;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(*mPackageInstallerMock, ListPackages(::testing::_))
    .WillOnce([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
        auto mockIterator = FillPackageIterator(); // Fill the package Info
        packages = mockIterator;
        return Core::ERROR_GENERAL;
    });

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->IsInstalled(APPMANAGER_APP_ID, installed));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for IsInstalledUsingComRpcFailurePackageManagerObjectIsNull
 * Setting up only AppManager Plugin and creating required COM-RPC resources
 * PackageManager Interface object is not created and hence the API should return error
 * Setting Mock for ListPackages() to simulate error return
 * Releasing the AppManager Interface object only
 */

TEST_F(AppManagerTest, IsInstalledUsingComRpcFailurePackageManagerObjectIsNull)
{
    bool installed;

    createAppManagerImpl();
    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->IsInstalled(APPMANAGER_APP_ID, installed));
    releaseAppManagerImpl();
}

/*
 * Test Case for LaunchAppUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and getting the app instance id
 * Verifying the return of the API
 * Setting Mock for OnAppLifecycleStateChanged() to simulate the app lifecycle state change
 * Registering the notification handler to receive the app lifecycle state change event
 * Simulating the app lifecycle state change event by calling OnAppLifecycleStateChanged() with expected parameters
 * Waiting for the notification handler to receive the event and verifying the received event
 * Verifying the received event matches the expected event
 * Unregistering the notification handler
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, LaunchAppUsingComRpcSuccess)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = APPMANAGER_APP_INSTANCE;
    expectedEvent.newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE;
    expectedEvent.oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_PAUSED;
    expectedEvent.errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_NONE;
    expectedEvent.intent = APPMANAGER_APP_INTENT;
    expectedEvent.source = "";
    Core::Sink<NotificationHandler> notification;
    uint32_t signalled = AppManager_StateInvalid;
    AppInfoManager::getInstance().upsert(APPMANAGER_APP_ID, [](Plugin::AppInfo& a) {
        a.setAppInstanceId(APPMANAGER_APP_INSTANCE);
        a.setAppNewState(Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE);
    });

    /* Register the notification handler */
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);
    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->LaunchApp(APPMANAGER_APP_ID, APPMANAGER_APP_INTENT, APPMANAGER_APP_LAUNCHARGS));
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLaunchRequest);
    EXPECT_TRUE(signalled & AppManager_onAppLaunchRequest);
    /* Reset the signalled flag for next use */
    signalled = AppManager_StateInvalid;
    mLifecycleManagerStateNotification_cb->OnAppLifecycleStateChanged(
        APPMANAGER_APP_ID,
        APPMANAGER_APP_INSTANCE,
        Exchange::ILifecycleManager::LifecycleState::PAUSED,  // Old state
        Exchange::ILifecycleManager::LifecycleState::ACTIVE,     // New state
        "start"
    );
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLifecycleStateChanged);
    EXPECT_TRUE(signalled & AppManager_onAppLifecycleStateChanged);
    mAppManagerImpl->Unregister(&notification);

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* * Test Case for LaunchAppUsingJSONRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Verifying the return of the API
 * Setting Mock for OnAppLifecycleStateChanged() to simulate the app lifecycle state change
 * Registering the notification handler to receive the app lifecycle state change event
 * Simulating the app lifecycle state change event by calling OnAppLifecycleStateChanged() with expected parameters
 * Waiting for the notification handler to receive the event and verifying the received event
 * Verifying the received event matches the expected event
 * Unregistering the notification handler
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, LaunchAppUsingJSONRpcSuccess)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    Core::Event onAppLaunchRequest(false, true);
    std::string request = "{\"appId\": \"" + std::string(APPMANAGER_APP_ID) +
                          "\", \"intent\": \"" + std::string(APPMANAGER_APP_INTENT) +
                          "\", \"launchArgs\": \"" + std::string(APPMANAGER_APP_LAUNCHARGS) + "\"}";

    EXPECT_CALL(*mServiceMock, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                std::string text;
                EXPECT_TRUE(json->ToString(text));
                const std::string expectedJson = "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AppManager.onAppLaunchRequest\",\"params\":{\"appId\":\"com.test.app\",\"intent\":\"test.intent\",\"source\":\"\"}}";
                EXPECT_EQ(text, expectedJson);
                onAppLaunchRequest.SetEvent();
                return Core::ERROR_NONE;
            }));
    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    EVENT_SUBSCRIBE(0, _T("onAppLaunchRequest"), _T("org.rdk.AppManager"), message);
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("launchApp"), request, mJsonRpcResponse));
    EXPECT_EQ(Core::ERROR_NONE, onAppLaunchRequest.Lock());
    EVENT_UNSUBSCRIBE(0, _T("onAppLaunchRequest"), _T("org.rdk.AppManager"), message);

    Core::Event onAppLifecycleStateChanged(false, true);
    EXPECT_CALL(*mServiceMock, Submit(::testing::_, ::testing::_))
    .Times(1)
    .WillOnce(::testing::Invoke(
        [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
            std::string text;
            EXPECT_TRUE(json->ToString(text));
            const std::string expectedJson = "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AppManager.onAppLifecycleStateChanged\",\"params\":{\"appId\":\"com.test.app\",\"appInstanceId\":\"testAppInstance\",\"newState\":\"APP_STATE_ACTIVE\",\"oldState\":\"APP_STATE_PAUSED\",\"errorReason\":\"APP_ERROR_NONE\"}}";
            EXPECT_EQ(text, expectedJson);
            onAppLifecycleStateChanged.SetEvent();

            return Core::ERROR_NONE;
        }));
    ASSERT_NE(mLifecycleManagerStateNotification_cb, nullptr)
        << "LifecycleManagerState notification callback is not registered";
    EVENT_SUBSCRIBE(0, _T("onAppLifecycleStateChanged"), _T("org.rdk.AppManager"), message);
    mLifecycleManagerStateNotification_cb->OnAppLifecycleStateChanged(
        APPMANAGER_APP_ID,
        APPMANAGER_APP_INSTANCE,
        Exchange::ILifecycleManager::LifecycleState::PAUSED,  // Old state
        Exchange::ILifecycleManager::LifecycleState::ACTIVE,     // New state
        "start"
    );
    EXPECT_EQ(Core::ERROR_NONE, onAppLifecycleStateChanged.Lock());
    EVENT_UNSUBSCRIBE(0, _T("onAppLifecycleStateChanged"), _T("org.rdk.AppManager"), message);
    if (status == Core::ERROR_NONE) {
        releaseResources();
    }
}

/* * Test Case for LaunchAppUsingCOMRPCSuspendedSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Verifying the return of the API by passing the app in suspended state
 * Releasing the AppManager interface and all related test resources
 */
//TODO
/*
TEST_F(AppManagerTest, LaunchAppUsingCOMRPCSuspendedSuccess)
{
    Core::hresult status;
    uint32_t signalled = AppManager_StateInvalid;
    Core::Sink<NotificationHandler> notification;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = APPMANAGER_APP_INSTANCE;
    expectedEvent.newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_SUSPENDED;
    expectedEvent.oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_PAUSED;
    expectedEvent.errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_NONE;
    expectedEvent.intent = APPMANAGER_APP_INTENT;
    expectedEvent.source = "";

    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);
    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::SUSPENDED);
    ON_CALL(*p_wrapsImplMock, stat(::testing::_, ::testing::_))
        .WillByDefault([](const char* path, struct stat* info) {
            // Simulate a successful stat call
            if (info != nullptr) {
                info->st_mode = S_IFREG | 0644; // Regular file with read/write permissions
            }
            // Simulate success
            return 0;
    });

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->LaunchApp(APPMANAGER_APP_ID, APPMANAGER_APP_INTENT, APPMANAGER_APP_LAUNCHARGS));
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLifecycleStateChanged);
    EXPECT_TRUE(signalled & AppManager_onAppLifecycleStateChanged);

    mAppManagerImpl->Unregister(&notification);
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}
*/
/*
 * Test Case for LaunchAppUsingComRpcFailureWrongAppID
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Verifying the return of the API by passing the wrong app id
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, LaunchAppUsingComRpcFailureWrongAppID)
{
    Core::hresult status;
    uint32_t signalled = AppManager_StateInvalid;
    Core::Sink<NotificationHandler> notification;
    ExpectedAppLifecycleEvent expectedEvent;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    bool installed = false;
    EXPECT_CALL(*mPackageInstallerMock, ListPackages(::testing::_))
        .WillRepeatedly([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
            auto mockIterator = FillPackageIterator(); // Fill the package Info
            packages = mockIterator;
            return Core::ERROR_GENERAL;
        });
    status = mAppManagerImpl->IsInstalled(APPMANAGER_WRONG_APP_ID, installed);
    //EXPECT_EQ(Core::ERROR_NONE, status);

    if (installed) {
    expectedEvent.appId = APPMANAGER_WRONG_APP_ID;
    expectedEvent.appInstanceId = "";
    expectedEvent.newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
    expectedEvent.oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED;
    expectedEvent.errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_NOT_INSTALLED;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::ACTIVE);   

   EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->LaunchApp(APPMANAGER_WRONG_APP_ID, APPMANAGER_APP_INTENT, APPMANAGER_APP_LAUNCHARGS));
        signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLifecycleStateChanged);
        EXPECT_TRUE(signalled & AppManager_onAppLifecycleStateChanged);
        mAppManagerImpl->Unregister(&notification);
    } else {
        LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::ACTIVE);
        EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->LaunchApp(APPMANAGER_WRONG_APP_ID, APPMANAGER_APP_INTENT, APPMANAGER_APP_LAUNCHARGS));
        // No event expected if not installed
    }
        releaseResources();    
}

/* * Test Case for LaunchAppUsingComRpcFailureEmptyAppID
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Setting Mock for ListPackages() to simulate getting empty package list
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, LaunchAppUsingComRpcFailureEmptyAppID)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, mAppManagerImpl->LaunchApp("", APPMANAGER_APP_INTENT, APPMANAGER_APP_LAUNCHARGS));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* * Test Case for LaunchAppUsingComRpcSpawnAppFailure
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Verifying the return of the API by passing the empty app id
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, LaunchAppUsingComRpcSpawnAppFailure)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.intent = APPMANAGER_APP_INTENT;
    expectedEvent.source = "";
    uint32_t signalled = AppManager_StateInvalid;
    Core::Sink<NotificationHandler> notification;

    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);
    // Mock the necessary calls
    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    EXPECT_CALL(*mLifecycleManagerMock, SpawnApp(APPMANAGER_APP_ID, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
    .WillOnce([&](const string& appId, const string& intent, const Exchange::ILifecycleManager::LifecycleState state,
        const Exchange::RuntimeConfig& runtimeConfigObject, const string& launchArgs, string& appInstanceId, string& error, bool& success) {
        error = "Failed to create LifecycleInterfaceConnector";
        success = false;
        return Core::ERROR_GENERAL;
    });

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->LaunchApp(APPMANAGER_APP_ID, APPMANAGER_APP_INTENT, APPMANAGER_APP_LAUNCHARGS));
    // Verify that the notification handler received the expected event
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLaunchRequest);
    EXPECT_TRUE(signalled & AppManager_onAppLaunchRequest);

    mAppManagerImpl->Unregister(&notification);
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for LaunchAppUsingComRpcFailureIsAppLoadedReturnError
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate error return
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, LaunchAppUsingComRpcFailureIsAppLoadedReturnError)
{
    Core::hresult status;
    uint32_t signalled = AppManager_StateInvalid;
    Core::Sink<NotificationHandler> notification;
    ExpectedAppLifecycleEvent expectedEvent;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = "";
    expectedEvent.newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
    expectedEvent.oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED;
    expectedEvent.errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_PACKAGE_LOCK;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    EXPECT_CALL(*mLifecycleManagerMock, IsAppLoaded(::testing::_, ::testing::_))
    .WillOnce([&](const std::string &appId, bool &loaded) {
        loaded = false;
        return Core::ERROR_GENERAL;
    });
    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->LaunchApp(APPMANAGER_APP_ID, APPMANAGER_APP_INTENT, APPMANAGER_APP_LAUNCHARGS));


    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLifecycleStateChanged);
    EXPECT_TRUE(signalled & AppManager_onAppLifecycleStateChanged);

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for LaunchAppUsingComRpcFailureLifecycleManagerRemoteObjectIsNull
 * Setting up only AppManager Plugin and creating required COM-RPC resources
 * LifecycleManager Interface object is not created and hence the API should return error
 * Verifying the return of the API
 * Releasing the AppManager Interface object only
 */
TEST_F(AppManagerTest, LaunchAppUsingComRpcFailureLifecycleManagerRemoteObjectIsNull)
{
    //uint32_t signalled = AppManager_StateInvalid;
    Core::Sink<NotificationHandler> notification;
    ExpectedAppLifecycleEvent expectedEvent;

    createAppManagerImpl();

    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = "";
    expectedEvent.newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
    expectedEvent.oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED;
    expectedEvent.errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_NOT_INSTALLED;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->LaunchApp(APPMANAGER_APP_ID, APPMANAGER_APP_INTENT, APPMANAGER_APP_LAUNCHARGS));
    //there wont be an event notification in this case as per #12126
   /* signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLifecycleStateChanged);
      EXPECT_TRUE(signalled & AppManager_onAppLifecycleStateChanged);*/

    releaseAppManagerImpl();
}
/*
 * Test Case for PreloadAppUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, PreloadAppUsingComRpcSuccess)
{
    Core::hresult status;
    std::string error = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    mPreLoadSpawmCalled = false;

    preLaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::PAUSED);
    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->PreloadApp(APPMANAGER_APP_ID, APPMANAGER_APP_LAUNCHARGS, error));
    {
        std::unique_lock<std::mutex> lock(mPreLoadMutex);
        ASSERT_TRUE(mPreLoadCV.wait_for(lock, std::chrono::seconds(10), [&]{ return mPreLoadSpawmCalled; }));
    }

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* * Test Case for PreloadAppUsingJSONRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, PreloadAppUsingJSONRpcSuccess)
{
    Core::hresult status;
    std::string error = "";
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    mPreLoadSpawmCalled = false;

    preLaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::PAUSED);
    std::string request = "{\"appId\": \"" + std::string(APPMANAGER_APP_ID) + "\", \"launchArgs\": \"" + std::string(APPMANAGER_APP_LAUNCHARGS) + "\"}";
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("preloadApp"), request, mJsonRpcResponse));
    {
        std::unique_lock<std::mutex> lock(mPreLoadMutex);
        ASSERT_TRUE(mPreLoadCV.wait_for(lock, std::chrono::seconds(10), [&]{ return mPreLoadSpawmCalled; }));
    }

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}


/*
 * Test Case for PreloadAppUsingComRpcFailureWrongAppID
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Verifying the return of the API by passing the wrong app id
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, PreloadAppUsingComRpcFailureWrongAppID)
{
    Core::hresult status;
    std::string error = "";
    uint32_t signalled = AppManager_StateInvalid;
    Core::Sink<NotificationHandler> notification;
    ExpectedAppLifecycleEvent expectedEvent;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    expectedEvent.appId = APPMANAGER_WRONG_APP_ID;
    expectedEvent.appInstanceId = "";
    expectedEvent.newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
    expectedEvent.oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED;
    expectedEvent.errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_NOT_INSTALLED;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::PAUSED);

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->PreloadApp(APPMANAGER_WRONG_APP_ID, APPMANAGER_APP_LAUNCHARGS, error));

    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLifecycleStateChanged);
    EXPECT_TRUE(signalled & AppManager_onAppLifecycleStateChanged);

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for PreloadAppUsingComRpcFailureIsAppLoadedReturnError
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate error return
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Verifying the return of the API by passing the wrong app id
 * Releasing the AppManager interface and all related test resources
 */

TEST_F(AppManagerTest, PreloadAppUsingComRpcFailureIsAppLoadedReturnError)
{
    Core::hresult status;
    std::string error = "";
    uint32_t signalled = AppManager_StateInvalid;
    Core::Sink<NotificationHandler> notification;
    ExpectedAppLifecycleEvent expectedEvent;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = "";
    expectedEvent.newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
    expectedEvent.oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED;
    expectedEvent.errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_PACKAGE_LOCK;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::PAUSED);

    EXPECT_CALL(*mLifecycleManagerMock, IsAppLoaded(::testing::_, ::testing::_))
    .WillOnce([&](const std::string &appId, bool &loaded) {
        loaded = false;
        return Core::ERROR_GENERAL;
    });
    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->PreloadApp(APPMANAGER_APP_ID, APPMANAGER_APP_LAUNCHARGS, error));

    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLifecycleStateChanged);
    EXPECT_TRUE(signalled & AppManager_onAppLifecycleStateChanged);

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for PreloadAppUsingComRpcFailureLifecycleManagerRemoteObjectIsNull
 * Setting up only AppManager Plugin and creating required COM-RPC resources
 * LifecycleManager Interface object is not created and hence the API should return error
 * Verifying the return of the API
 * Releasing the AppManager Interface object only
 */
TEST_F(AppManagerTest, PreloadAppUsingComRpcFailureLifecycleManagerRemoteObjectIsNull)
{
    std::string error = "";
    uint32_t signalled = AppManager_StateInvalid;
    Core::Sink<NotificationHandler> notification;
    ExpectedAppLifecycleEvent expectedEvent;

    createAppManagerImpl();

    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = "";
    expectedEvent.newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
    expectedEvent.oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED;
    expectedEvent.errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_NOT_INSTALLED;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->PreloadApp(APPMANAGER_APP_ID, APPMANAGER_APP_LAUNCHARGS, error));

    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLifecycleStateChanged);
    EXPECT_TRUE(signalled & AppManager_onAppLifecycleStateChanged);

    releaseAppManagerImpl();
}

/*
 * Test Case for CloseAppUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Verifying the return of the API
 * Setting Mock for OnAppLifecycleStateChanged() to simulate the app lifecycle state change
 * Registering the notification handler to receive the app lifecycle state change event
 * Simulating the app lifecycle state change event by calling OnAppLifecycleStateChanged() with expected parameters
 * Waiting for the notification handler to receive the event and verifying the received event
 * Verifying the received event matches the expected event
 * Unregistering the notification handler
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, CloseAppUsingComRpcSuccess)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = APPMANAGER_APP_INSTANCE;
    expectedEvent.newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_PAUSED;
    expectedEvent.oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE;
    expectedEvent.intent = APPMANAGER_APP_INTENT;
    expectedEvent.source = "";
    expectedEvent.errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_NONE;
    uint32_t signalled = AppManager_StateInvalid;
    Core::Sink<NotificationHandler> notification;
    AppInfoManager::getInstance().upsert(APPMANAGER_APP_ID, [](Plugin::AppInfo& a) {
        a.setAppInstanceId(APPMANAGER_APP_INSTANCE);
        a.setAppNewState(Exchange::IAppManager::AppLifecycleState::APP_STATE_PAUSED);
    });

    /* Register the notification handler */
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);
    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::PAUSED);
    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->LaunchApp(APPMANAGER_APP_ID, APPMANAGER_APP_INTENT, APPMANAGER_APP_LAUNCHARGS));

    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLaunchRequest);
    EXPECT_TRUE(signalled & AppManager_onAppLaunchRequest);

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->CloseApp(APPMANAGER_APP_ID));
    // Simulate the app lifecycle state change event
    mLifecycleManagerStateNotification_cb->OnAppLifecycleStateChanged(
        APPMANAGER_APP_ID,
        APPMANAGER_APP_INSTANCE,
        Exchange::ILifecycleManager::LifecycleState::ACTIVE,  // Old state
        Exchange::ILifecycleManager::LifecycleState::PAUSED,     // New state
        ""
    );
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLifecycleStateChanged);
    EXPECT_TRUE(signalled & AppManager_onAppLifecycleStateChanged);
    
    mAppManagerImpl->Unregister(&notification);
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* * Test Case for CloseAppUsingJSONRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Verifying the return of the API
 * Setting Mock for OnAppLifecycleStateChanged() to simulate the app lifecycle state change
 * Registering the notification handler to receive the app lifecycle state change event
 * Simulating the app lifecycle state change event by calling OnAppLifecycleStateChanged() with expected parameters
 * Waiting for the notification handler to receive the event and verifying the received event
 * Verifying the received event matches the expected event
 * Unregistering the notification handler
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, CloseAppUsingJSONRpcSuccess)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    Core::Event onAppLaunchRequest(false, true);

    AppInfoManager::getInstance().upsert(APPMANAGER_APP_ID, [](Plugin::AppInfo& a) {
        a.setAppInstanceId(APPMANAGER_APP_INSTANCE);
        a.setAppNewState(Exchange::IAppManager::AppLifecycleState::APP_STATE_PAUSED);
    });
    std::string requestClose = "{\"appId\": \"" + std::string(APPMANAGER_APP_ID) + "\"}";
    std::string requestLaunch = "{\"appId\": \"" + std::string(APPMANAGER_APP_ID) +
                          "\", \"intent\": \"" + std::string(APPMANAGER_APP_INTENT) +
                          "\", \"launchArgs\": \"" + std::string(APPMANAGER_APP_LAUNCHARGS) + "\"}";

    EXPECT_CALL(*mServiceMock, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                std::string text;
                EXPECT_TRUE(json->ToString(text));
                const std::string expectedJson = "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AppManager.onAppLaunchRequest\",\"params\":{\"appId\":\"com.test.app\",\"intent\":\"test.intent\",\"source\":\"\"}}";
                EXPECT_EQ(text, expectedJson);
                onAppLaunchRequest.SetEvent();
                return Core::ERROR_NONE;
            }));

    EVENT_SUBSCRIBE(0, _T("onAppLaunchRequest"), _T("org.rdk.AppManager"), message);
    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("launchApp"), requestLaunch, mJsonRpcResponse));
    EXPECT_EQ(Core::ERROR_NONE, onAppLaunchRequest.Lock());
    EVENT_UNSUBSCRIBE(0, _T("onAppLaunchRequest"), _T("org.rdk.AppManager"), message);
    
    // Simulate the app lifecycle state change event
    EVENT_SUBSCRIBE(0, _T("onAppLifecycleStateChanged"), _T("org.rdk.AppManager"), message);
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("closeApp"), requestClose, mJsonRpcResponse));
    Core::Event onAppLifecycleStateChanged(false, true);
    EXPECT_CALL(*mServiceMock, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                std::string text;
                EXPECT_TRUE(json->ToString(text));
                const std::string expectedJson = "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AppManager.onAppLifecycleStateChanged\",\"params\":{\"appId\":\"com.test.app\",\"appInstanceId\":\"testAppInstance\",\"newState\":\"APP_STATE_PAUSED\",\"oldState\":\"APP_STATE_ACTIVE\",\"errorReason\":\"APP_ERROR_NONE\"}}";
                EXPECT_EQ(text, expectedJson);
                onAppLifecycleStateChanged.SetEvent();
                return Core::ERROR_NONE;
            }));
    
    ASSERT_NE(mLifecycleManagerStateNotification_cb, nullptr)
    << "LifecycleManagerState notification callback is not registered";
    mLifecycleManagerStateNotification_cb->OnAppLifecycleStateChanged(
        APPMANAGER_APP_ID,
        APPMANAGER_APP_INSTANCE,
        Exchange::ILifecycleManager::LifecycleState::ACTIVE,  // Old state
        Exchange::ILifecycleManager::LifecycleState::PAUSED,     // New state
        ""
    );
    EXPECT_EQ(Core::ERROR_NONE, onAppLifecycleStateChanged.Lock());
    EVENT_UNSUBSCRIBE(0, _T("onAppLifecycleStateChanged"), _T("org.rdk.AppManager"), message);

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for CloseAppUsingComRpcFailureWrongAppID
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Verifying the return of the API by passing the wrong app id
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, CloseAppUsingComRpcFailureWrongAppID)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    Core::Sink<NotificationHandler> notification;
    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.intent = APPMANAGER_APP_INTENT;
    expectedEvent.source = "";

    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);
    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->LaunchApp(APPMANAGER_APP_ID, APPMANAGER_APP_INTENT, APPMANAGER_APP_LAUNCHARGS));
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLaunchRequest);
    EXPECT_TRUE(signalled & AppManager_onAppLaunchRequest);

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->CloseApp(APPMANAGER_WRONG_APP_ID));

    mAppManagerImpl->Unregister(&notification);
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for CloseAppUsingComRpcFailureSetTargetAppStateReturnError
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate error return
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, CloseAppUsingComRpcFailureSetTargetAppStateReturnError)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    Core::Sink<NotificationHandler> notification;
    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.intent = APPMANAGER_APP_INTENT;
    expectedEvent.source = "";

    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);
    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->LaunchApp(APPMANAGER_APP_ID, APPMANAGER_APP_INTENT, APPMANAGER_APP_LAUNCHARGS));
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLaunchRequest);
    EXPECT_TRUE(signalled & AppManager_onAppLaunchRequest);

    EXPECT_CALL(*mLifecycleManagerMock, SetTargetAppState(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly([&](const string& appInstanceId , const Exchange::ILifecycleManager::LifecycleState targetLifecycleState , const string& launchIntent) {
            return Core::ERROR_GENERAL;
        });

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->CloseApp(APPMANAGER_APP_ID));

    mAppManagerImpl->Unregister(&notification);
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for CloseAppUsingComRpcFailureLifecycleManagerRemoteObjectIsNull
 * Setting up only AppManager Plugin and creating required COM-RPC resources
 * LifecycleManager Interface object is not created and hence the API should return error
 * Verifying the return of the API
 * Releasing the AppManager Interface object only
 */

TEST_F(AppManagerTest, CloseAppUsingComRpcFailureLifecycleManagerRemoteObjectIsNull)
{
    createAppManagerImpl();

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->CloseApp(APPMANAGER_APP_ID));

    releaseAppManagerImpl();
}

/*
 * Test Case for TerminateAppUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and getting the appinstance id
 * Calling LaunchApp first so that all the prerequisite will be filled
 * Setting Mock for UnloadApp() to simulate unloading the app
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, TerminateAppUsingComRpcSuccess)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    Core::Sink<NotificationHandler> notification;
    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.intent = APPMANAGER_APP_INTENT;
    expectedEvent.source = "";

    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);
    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->LaunchApp(APPMANAGER_APP_ID, APPMANAGER_APP_INTENT, APPMANAGER_APP_LAUNCHARGS));
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLaunchRequest);
    EXPECT_TRUE(signalled & AppManager_onAppLaunchRequest);
    SetupUnloadApp();

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->TerminateApp(APPMANAGER_APP_ID));

    mAppManagerImpl->Unregister(&notification);
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* * Test Case for TerminateAppUsingJSONRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and getting the appinstance id
 * Calling LaunchApp first so that all the prerequisite will be filled
 * Setting Mock for UnloadApp() to simulate unloading the app
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, TerminateAppUsingJSONRpcSuccess)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    Core::Event onAppLaunchRequest(false, true);
    std::string requestLaunch = "{\"appId\": \"" + std::string(APPMANAGER_APP_ID) +
                        "\", \"intent\": \"" + std::string(APPMANAGER_APP_INTENT) +
                        "\", \"launchArgs\": \"" + std::string(APPMANAGER_APP_LAUNCHARGS) + "\"}";
    EXPECT_CALL(*mServiceMock, Submit(::testing::_, ::testing::_))
    .Times(1)
    .WillOnce(::testing::Invoke(
        [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
            std::string text;
            EXPECT_TRUE(json->ToString(text));
            const std::string expectedJson = "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AppManager.onAppLaunchRequest\",\"params\":{\"appId\":\"com.test.app\",\"intent\":\"test.intent\",\"source\":\"\"}}";
            EXPECT_EQ(text, expectedJson);
            onAppLaunchRequest.SetEvent();
            return Core::ERROR_NONE;
        }
    ));
    EVENT_SUBSCRIBE(0, _T("onAppLaunchRequest"), _T("org.rdk.AppManager"), message);
    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("launchApp"), requestLaunch, mJsonRpcResponse));
    EXPECT_EQ(Core::ERROR_NONE, onAppLaunchRequest.Lock());
    EVENT_UNSUBSCRIBE(0, _T("onAppLaunchRequest"), _T("org.rdk.AppManager"), message);
    SetupUnloadApp();

    std::string request = "{\"appId\": \"" + std::string(APPMANAGER_APP_ID) + "\"}";
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("terminateApp"), request, mJsonRpcResponse));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for TerminateAppUsingComRpcFailureWrongAppID
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Verifying the return of the API by passing the wrong app id
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, TerminateAppUsingComRpcFailureWrongAppID)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->TerminateApp(APPMANAGER_WRONG_APP_ID));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for TerminateAppUsingComRpcFailureEmptyAppID
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Verifying the return of the API by passing the empty app id
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, TerminateAppUsingComRpcFailureEmptyAppID)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->TerminateApp(""));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for TerminateAppUsingComRpcFailureUnloadAppReturnError
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Verifying the return of the API by no calling launch so that mInfo will be empty
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, TerminateAppUsingComRpcFailureUnloadAppReturnError)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->TerminateApp(APPMANAGER_APP_ID));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for TerminateAppUsingComRpcFailureLifecycleManagerRemoteObjectIsNull
 * Setting up only AppManager Plugin and creating required COM-RPC resources
 * LifecycleManager Interface object is not created and hence the API should return error
 * Verifying the return of the API
 * Releasing the AppManager Interface object only
 */

TEST_F(AppManagerTest, TerminateAppUsingComRpcFailureLifecycleManagerRemoteObjectIsNull)
{
    createAppManagerImpl();
    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->TerminateApp(APPMANAGER_APP_ID));
    releaseAppManagerImpl();
}

/*
 * Test Case for StartSystemAppUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Implementation missing TODO: Testcase need to be added once implementation is done
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, StartSystemAppUsingComRpcSuccess)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->StartSystemApp(APPMANAGER_APP_ID));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* * Test Case for StartSystemAppUsingJSONRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Implementation missing TODO: Testcase need to be added once implementation is done
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, StartSystemAppUsingJSONRpcSuccess)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    std::string request = "{\"appId\": \"" + std::string(APPMANAGER_APP_ID) + "\"}";
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("startSystemApp"), request, mJsonRpcResponse));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for StopSystemAppUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Implementation missing TODO: Testcase need to be added once implementation is done
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, StopSystemAppUsingComRpcSuccess)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->StopSystemApp(APPMANAGER_APP_ID));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* * Test Case for StopSystemAppUsingJSONRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Implementation missing TODO: Testcase need to be added once implementation is done
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, StopSystemAppUsingJSONRpcSuccess)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    std::string request = "{\"appId\": \"" + std::string(APPMANAGER_APP_ID) + "\"}";
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("stopSystemApp"), request, mJsonRpcResponse));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for KillAppUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Calling LaunchApp first so that all the prerequisite will be filled
 * Setting Mock for UnloadApp() to simulate unloading the app
 * Setting Mock for Unlock() to simulate reset the lock flag
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, KillAppUsingComRpcSuccess)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    Core::Sink<NotificationHandler> notification;
    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.intent = APPMANAGER_APP_INTENT;
    expectedEvent.source = "";

    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);
    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->LaunchApp(APPMANAGER_APP_ID, APPMANAGER_APP_INTENT, APPMANAGER_APP_LAUNCHARGS));
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLaunchRequest);
    EXPECT_TRUE(signalled & AppManager_onAppLaunchRequest);
    SetupUnloadApp();

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->KillApp(APPMANAGER_APP_ID));

    mAppManagerImpl->Unregister(&notification);
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* Test Case for KillAppUsingJSONRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Calling LaunchApp first so that all the prerequisite will be filled
 * Setting Mock for UnloadApp() to simulate unloading the app
 * Setting Mock for Unlock() to simulate reset the lock flag
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, KillAppUsingJSONRpcSuccess)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    Core::Event onAppLaunchRequest(false, true);
    std::string requestLaunch = "{\"appId\": \"" + std::string(APPMANAGER_APP_ID) +
                          "\", \"intent\": \"" + std::string(APPMANAGER_APP_INTENT) +
                          "\", \"launchArgs\": \"" + std::string(APPMANAGER_APP_LAUNCHARGS) + "\"}";
    std::string request = "{\"appId\": \"" + std::string(APPMANAGER_APP_ID) + "\"}";

    EXPECT_CALL(*mServiceMock, Submit(::testing::_, ::testing::_))
    .Times(1)
    .WillOnce(::testing::Invoke(
        [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
            std::string text;
            EXPECT_TRUE(json->ToString(text));
            const std::string expectedJson = "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AppManager.onAppLaunchRequest\",\"params\":{\"appId\":\"com.test.app\",\"intent\":\"test.intent\",\"source\":\"\"}}";
            EXPECT_EQ(text, expectedJson);
            onAppLaunchRequest.SetEvent();
            return Core::ERROR_NONE;
        }
    ));

    EVENT_SUBSCRIBE(0, _T("onAppLaunchRequest"), _T("org.rdk.AppManager"), message);
    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("launchApp"), requestLaunch, mJsonRpcResponse));
    EXPECT_EQ(Core::ERROR_NONE, onAppLaunchRequest.Lock());
    EVENT_UNSUBSCRIBE(0, _T("onAppLaunchRequest"), _T("org.rdk.AppManager"), message);
    
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("killApp"), request, mJsonRpcResponse));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for TerminateAppUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Verifying the return of the API by passing the wrong app id
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, KillAppUsingComRpcFailureWrongAppID)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->KillApp(APPMANAGER_WRONG_APP_ID));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* * Test Case for KillAppUsingComRpcFailureEmptyAppID
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Verifying the return of the API by passing the empty app id
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, KillAppUsingComRpcFailureEmptyAppID)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->KillApp(""));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for KillAppUsingComRpcFailureLifecycleManagerRemoteObjectIsNull
 * Setting up only AppManager Plugin and creating required COM-RPC resources
 * LifecycleManager Interface object is not created and hence the API should return error
 * Verifying the return of the API
 * Releasing the AppManager Interface object only
 */

TEST_F(AppManagerTest, KillAppUsingComRpcFailureLifecycleManagerRemoteObjectIsNull)
{
    createAppManagerImpl();

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->KillApp(APPMANAGER_WRONG_APP_ID));

    releaseAppManagerImpl();
}

/*
 * Test Case for SendIntentUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Calling LaunchApp first so that all the prerequisite will be filled
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
 TEST_F(AppManagerTest, SendIntentUsingComRpcSuccess)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    Core::Sink<NotificationHandler> notification;
    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.intent = APPMANAGER_APP_INTENT;
    expectedEvent.source = "";

    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);
    EXPECT_CALL(*mLifecycleManagerMock, SendIntentToActiveApp(::testing::_, ::testing::_, ::testing::_, ::testing::_))
    .WillOnce([&](const string& appInstanceId, const string& intent, string& errorReason, bool& success) {
        success = true;
        return Core::ERROR_NONE;
    });

    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->LaunchApp(APPMANAGER_APP_ID, APPMANAGER_APP_INTENT, APPMANAGER_APP_LAUNCHARGS));
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLaunchRequest);
    EXPECT_TRUE(signalled & AppManager_onAppLaunchRequest);

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->SendIntent(APPMANAGER_APP_ID, APPMANAGER_APP_INTENT));

    mAppManagerImpl->Unregister(&notification);
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for SendIntentUsingJSONRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Calling LaunchApp first so that all the prerequisite will be filled
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, SendIntentUsingJSONRpcSuccess)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    Core::Event onAppLaunchRequest(false, true);
    std::string requestLaunch = "{\"appId\": \"" + std::string(APPMANAGER_APP_ID) +
                          "\", \"intent\": \"" + std::string(APPMANAGER_APP_INTENT) +
                          "\", \"launchArgs\": \"" + std::string(APPMANAGER_APP_LAUNCHARGS) + "\"}";
    std::string requestintent = "{\"appId\": \"" + std::string(APPMANAGER_APP_ID) + "\", \"intent\": \"" + std::string(APPMANAGER_APP_INTENT) + "\"}";

    EXPECT_CALL(*mServiceMock, Submit(::testing::_, ::testing::_))
    .Times(1)
    .WillOnce(::testing::Invoke(
        [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
            std::string text;
            EXPECT_TRUE(json->ToString(text));
            const std::string expectedJson = "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AppManager.onAppLaunchRequest\",\"params\":{\"appId\":\"com.test.app\",\"intent\":\"test.intent\",\"source\":\"\"}}";
            EXPECT_EQ(text, expectedJson);
            onAppLaunchRequest.SetEvent();
            return Core::ERROR_NONE;
        }
    ));

    EVENT_SUBSCRIBE(0, _T("onAppLaunchRequest"), _T("org.rdk.AppManager"), message);
    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("launchApp"), requestLaunch, mJsonRpcResponse));
    EXPECT_EQ(Core::ERROR_NONE, onAppLaunchRequest.Lock());
    EVENT_UNSUBSCRIBE(0, _T("onAppLaunchRequest"), _T("org.rdk.AppManager"), message);
    
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("sendIntent"), requestintent, mJsonRpcResponse));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for SendIntentUsingComRpcFailureWrongAppID
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Verifying the return of the API by passing the wrong app id
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, SendIntentUsingComRpcFailureWrongAppID)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->SendIntent(APPMANAGER_WRONG_APP_ID, APPMANAGER_APP_INTENT));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for SendIntentUsingComRpcFailureEmptyAppID
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Verifying the return of the API by passing the empty app id
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, SendIntentUsingComRpcFailureEmptyAppID)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->SendIntent("", APPMANAGER_APP_INTENT));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for SendIntentUsingComRpcFailureLifecycleManagerRemoteObjectIsNull
 * Setting up only AppManager Plugin and creating required COM-RPC resources
 * LifecycleManager Interface object is not created and hence the API should return error
 * Verifying the return of the API
 * Releasing the AppManager Interface object only
 */

TEST_F(AppManagerTest, SendIntentUsingComRpcFailureLifecycleManagerRemoteObjectIsNull)
{
    createAppManagerImpl();
    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->SendIntent(APPMANAGER_APP_ID, APPMANAGER_APP_INTENT));
    releaseAppManagerImpl();
}

/*
 * Test Case for ClearAppDataUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Implementation missing TODO: Testcase need to be added once implementation is done
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, ClearAppDataUsingComRpcSuccess)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_CALL(*mStorageManagerMock, Clear(::testing::_, ::testing::_))
        .WillOnce([&](const string& appId, const string& errorReason) {
            return Core::ERROR_NONE; // Simulating successful clear of app data
        });
    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->ClearAppData(APPMANAGER_APP_ID));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for ClearAllAppDataUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Implementation missing TODO: Testcase need to be added once implementation is done
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, ClearAllAppDataUsingComRpcSuccess)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_CALL(*mStorageManagerMock, ClearAll(::testing::_, ::testing::_))
    .WillOnce([&](const string& exemptionAppIds, const string& errorReason) {
        return Core::ERROR_NONE; // Simulating successful clear of app data
    });
    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->ClearAllAppData());

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for GetAppMetadataUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Implementation missing TODO: Testcase need to be added once implementation is done
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetAppMetadataUsingComRpcSuccess)
{
    Core::hresult status;
    const std::string dummymetaData = "";
    std::string dummyResult = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->GetAppMetadata(APPMANAGER_APP_ID, dummymetaData, dummyResult));
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* * Test Case for GetAppMetadataUsingJSONRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Implementation missing TODO: Testcase need to be added once implementation is done
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetAppMetadataUsingJSONRpcSuccess)
{
    Core::hresult status;
    const std::string dummymetaData = "";
    std::string dummyResult = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    std::string request = "{\"appId\": \"" + std::string(APPMANAGER_APP_ID) + "\", \"metadata\": \"" + dummymetaData + "\"}";
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("getAppMetadata"), request, mJsonRpcResponse));
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for GetAppPropertyUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for GetValue() to simulate getting value from persistent store
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetAppPropertyUsingComRpcSuccess)
{
    Core::hresult status;
    const std::string key = PERSISTENT_STORE_KEY;
    std::string value = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(*mStore2Mock, GetValue(Exchange::IStore2::ScopeType::DEVICE, APPMANAGER_APP_ID, key, ::testing::_, ::testing::_))
    .WillOnce([&](const Exchange::IStore2::ScopeType scope, const string& ns, const string& key, string& value, uint32_t& ttl) {
        value = PERSISTENT_STORE_VALUE;
        return Core::ERROR_NONE;
    });

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->GetAppProperty(APPMANAGER_APP_ID, key, value));
    EXPECT_STREQ(value.c_str(), PERSISTENT_STORE_VALUE);

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* * Test Case for GetAppPropertyUsingJSONRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Setting Mock for GetValue() to simulate getting value from persistent store
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetAppPropertyUsingJSONRpcSuccess)
{
    Core::hresult status;
    const std::string key = PERSISTENT_STORE_KEY;
    std::string value = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    std::string request = "{\"appId\": \"" + std::string(APPMANAGER_APP_ID) + "\", \"key\": \"" + key + "\"}";

    EXPECT_CALL(*mStore2Mock, GetValue(Exchange::IStore2::ScopeType::DEVICE, APPMANAGER_APP_ID, key, ::testing::_, ::testing::_))
    .WillOnce([&](const Exchange::IStore2::ScopeType scope, const string& ns, const string& key, string& value, uint32_t& ttl) {
        value = PERSISTENT_STORE_VALUE;
        return Core::ERROR_NONE;
    });
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("getAppProperty"), request, mJsonRpcResponse));
    EXPECT_STREQ(mJsonRpcResponse.c_str(), (std::string("\"") + PERSISTENT_STORE_VALUE + "\"").c_str());

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}
/*
 * Test Case for GetAppPropertyUsingComRpcFailureEmptyAppID
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Verifying the return of the API by passing the empty app id
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetAppPropertyUsingComRpcFailureEmptyAppID)
{
    Core::hresult status;
    const std::string key = "";
    std::string value = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->GetAppProperty(APPMANAGER_EMPTY_APP_ID, key, value));
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for GetAppPropertyUsingComRpcFailureEmptyKey
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Verifying the return of the API by passing the empty key
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetAppPropertyUsingComRpcFailureEmptyKey)
{
    Core::hresult status;
    const std::string key = "";
    std::string value = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->GetAppProperty(APPMANAGER_APP_ID, key, value));
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for GetAppPropertyUsingComRpcFailureGetAppPropertyReturnError
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for GetValue() to simulate error return
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetAppPropertyUsingComRpcFailureGetAppPropertyReturnError)
{
    Core::hresult status;
    const std::string key = PERSISTENT_STORE_KEY;
    std::string value = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(*mStore2Mock, GetValue(Exchange::IStore2::ScopeType::DEVICE, APPMANAGER_APP_ID, key, ::testing::_, ::testing::_))
    .WillOnce([&](const Exchange::IStore2::ScopeType scope, const string& ns, const string& key, string& value, uint32_t& ttl) {
        return Core::ERROR_GENERAL;
    });
    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->GetAppProperty(APPMANAGER_APP_ID, key, value));
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for GetAppPropertyUsingComRpcFailureLifecycleManagerRemoteObjectIsNull
 * Setting up only AppManager Plugin and creating required COM-RPC resources
 * LifecycleManager Interface object is not created and hence the API should return error
 * Verifying the return of the API
 * Releasing the AppManager Interface object only
 */

TEST_F(AppManagerTest, GetAppPropertyUsingComRpcFailureLifecycleManagerRemoteObjectIsNull)
{
    const std::string key = PERSISTENT_STORE_KEY;
    std::string value = "";

    createAppManagerImpl();

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->GetAppProperty(APPMANAGER_APP_ID, key, value));
    releaseAppManagerImpl();
}

/*
 * Test Case for SetAppPropertyUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for SetValue() to simulate setting value from persistent store
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, SetAppPropertyUsingComRpcSuccess)
{
    Core::hresult status;
    const std::string key = PERSISTENT_STORE_KEY;
    std::string value = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(*mStore2Mock, SetValue(Exchange::IStore2::ScopeType::DEVICE, APPMANAGER_APP_ID, key, value, 0))
    .WillOnce([&](const Exchange::IStore2::ScopeType scope, const string& ns, const string& key, const string& value, const uint32_t ttl) {
        return Core::ERROR_NONE;
    });

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->SetAppProperty(APPMANAGER_APP_ID, key, value));
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* Test Case for SetAppPropertyUsingJSONRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Setting Mock for SetValue() to simulate setting value from persistent store
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, SetAppPropertyUsingJSONRpcSuccess)
{
    Core::hresult status;
    const std::string key = PERSISTENT_STORE_KEY;
    std::string value = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    std::string request = "{\"appId\": \"" + std::string(APPMANAGER_APP_ID) + "\", \"key\": \"" + key + "\", \"value\": \"" + value + "\"}";

    EXPECT_CALL(*mStore2Mock, SetValue(Exchange::IStore2::ScopeType::DEVICE, APPMANAGER_APP_ID, key, value, 0))
    .WillOnce([&](const Exchange::IStore2::ScopeType scope, const string& ns, const string& key, const string& value, const uint32_t ttl) {
        return Core::ERROR_NONE;
    });
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("setAppProperty"), request, mJsonRpcResponse));
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for SetAppPropertyUsingComRpcFailureEmptyAppID
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Verifying the return of the API by passing the empty app id
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, SetAppPropertyUsingComRpcFailureEmptyAppID)
{
    Core::hresult status;
    const std::string key = PERSISTENT_STORE_KEY;
    std::string value = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->SetAppProperty(APPMANAGER_EMPTY_APP_ID, key, value));
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for SetAppPropertyUsingComRpcFailureEmptyKey
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Verifying the return of the API by passing the empty key
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, SetAppPropertyUsingComRpcFailureEmptyKey)
{
    Core::hresult status;
    const std::string key = "";
    std::string value = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->SetAppProperty(APPMANAGER_APP_ID, key, value));
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for SetAppPropertyUsingComRpcFailureSetValueReturnError
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for SetValue() to simulate error return
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, SetAppPropertyUsingComRpcFailureSetValueReturnError)
{
    Core::hresult status;
    const std::string key = PERSISTENT_STORE_KEY;
    std::string value = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(*mStore2Mock, SetValue(Exchange::IStore2::ScopeType::DEVICE, APPMANAGER_APP_ID, key, value, 0))
    .WillOnce([&](const Exchange::IStore2::ScopeType scope, const string& ns, const string& key, const string& value, const uint32_t ttl) {
        return Core::ERROR_GENERAL;
    });
    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->SetAppProperty(APPMANAGER_APP_ID, key, value));

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for SetAppPropertyUsingComRpcFailureLifecycleManagerRemoteObjectIsNull
 * Setting up only AppManager Plugin and creating required COM-RPC resources
 * LifecycleManager Interface object is not created and hence the API should return error
 * Verifying the return of the API
 * Releasing the AppManager Interface object only
 */
TEST_F(AppManagerTest, SetAppPropertyUsingComRpcFailureLifecycleManagerRemoteObjectIsNull)
{
    const std::string key = PERSISTENT_STORE_KEY;
    std::string value = "";

    createAppManagerImpl();

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->SetAppProperty(APPMANAGER_APP_ID, key, value));

    releaseAppManagerImpl();
}

/*
 * Test Case for GetMaxRunningAppUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Implementation missing TODO: Testcase need to be added once implementation is done
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetMaxRunningAppUsingComRpcSuccess)
{
    Core::hresult status;
    int32_t maxRunningApps = 0;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->GetMaxRunningApps(maxRunningApps));
    EXPECT_EQ(maxRunningApps, -1);
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* * GetMaxRunningAppUsingJSONRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Implementation missing TODO: Testcase need to be added once implementation is done
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetMaxRunningAppUsingJSONRpcSuccess)
{
    Core::hresult status;
    //int32_t maxRunningApps = 0;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    std::string request = "{}";
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("getMaxRunningApps"), request, mJsonRpcResponse));
    EXPECT_EQ(mJsonRpcResponse, "-1");

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for GetMaxHibernatedAppsUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Implementation missing TODO: Testcase need to be added once implementation is done
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetMaxHibernatedAppsUsingComRpcSuccess)
{
    Core::hresult status;
    int32_t maxHibernatedApps = 0;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->GetMaxHibernatedApps(maxHibernatedApps));
    EXPECT_EQ(maxHibernatedApps, -1);
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* * GetMaxHibernatedAppsUsingJSONRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Implementation missing TODO: Testcase need to be added once implementation is done
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetMaxHibernatedAppsUsingJSONRpcSuccess)
{
    Core::hresult status;
    //int32_t maxHibernatedApps = 0;
    
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    std::string request = "{}";
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("getMaxHibernatedApps"), request, mJsonRpcResponse));
    EXPECT_EQ(mJsonRpcResponse, "-1");

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for GetMaxHibernatedFlashUsageUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Implementation missing TODO: Testcase need to be added once implementation is done
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetMaxHibernatedFlashUsageUsingComRpcSuccess)
{
    Core::hresult status;
    int32_t maxHibernatedFlashUsage = 0;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->GetMaxHibernatedFlashUsage(maxHibernatedFlashUsage));
    EXPECT_EQ(maxHibernatedFlashUsage, -1);
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* GetMaxHibernatedFlashUsageUsingJSONRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Implementation missing TODO: Testcase need to be added once implementation is done
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetMaxHibernatedFlashUsageUsingJSONRpcSuccess)
{
    Core::hresult status;
    //int32_t maxHibernatedFlashUsage = 0;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    std::string request = "{}";
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("getMaxHibernatedFlashUsage"), request, mJsonRpcResponse));
    EXPECT_EQ(std::stoi(mJsonRpcResponse), -1);
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for GetMaxInactiveRamUsageUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Implementation missing TODO: Testcase need to be added once implementation is done
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetMaxInactiveRamUsageUsingComRpcSuccess)
{
    Core::hresult status;
    int32_t maxInactiveRamUsage = 0;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->GetMaxInactiveRamUsage(maxInactiveRamUsage));
    EXPECT_EQ(maxInactiveRamUsage, -1);
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* * GetMaxInactiveRamUsageUsingJSONRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Implementation missing TODO: Testcase need to be added once implementation is done
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetMaxInactiveRamUsageUsingJSONRpcSuccess)
{
    Core::hresult status;
    //int32_t maxInactiveRamUsage = 0;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    std::string request = "{}";
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("getMaxInactiveRamUsage"), request, mJsonRpcResponse));
    EXPECT_EQ(std::stoi(mJsonRpcResponse), -1);

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for updateCurrentActionUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for ListPackages() to simulate getting installed package list
 * Setting Mock for Lock() to simulate lockId and unpacked path
 * Setting Mock for IsAppLoaded() to simulate the package is loaded or not
 * Setting Mock for SetTargetAppState() to simulate setting the state
 * Setting Mock for SpawnApp() to simulate spawning a app and gettign the appinstance id
 * Calling LaunchApp first so that all the prerequisite will be filled
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, updateCurrentActionUsingComRpcSuccess)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    Core::Event onAppLaunchRequest(false, true);

    EXPECT_CALL(*mServiceMock, Submit(::testing::_, ::testing::_))
    .Times(1)
    .WillOnce(::testing::Invoke(
        [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
            std::string text;
            EXPECT_TRUE(json->ToString(text));
            TEST_LOG("VEEKSHA - updateCurrentActionUsingComRpcSuccess - JSON-RPC response: %s", text.c_str());
            const std::string expectedJson = "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.AppManager.onAppLaunchRequest\",\"params\":{\"appId\":\"com.test.app\",\"intent\":\"test.intent\",\"source\":\"\"}}";
            EXPECT_EQ(text, expectedJson);
            onAppLaunchRequest.SetEvent();
            return Core::ERROR_NONE;
        }
    ));
    EVENT_SUBSCRIBE(0, _T("onAppLaunchRequest"), _T("org.rdk.AppManager"), message);
    LaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::ACTIVE);
    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->LaunchApp(APPMANAGER_APP_ID, APPMANAGER_APP_INTENT, APPMANAGER_APP_LAUNCHARGS));
    EXPECT_EQ(Core::ERROR_NONE, onAppLaunchRequest.Lock());
    EVENT_UNSUBSCRIBE(0, _T("onAppLaunchRequest"), _T("org.rdk.AppManager"), message);

    mAppManagerImpl->updateCurrentAction(APPMANAGER_APP_ID, Plugin::AppManagerImplementation::CurrentAction::APP_ACTION_LAUNCH);

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for updateCurrentActionUsingComRpcFailureAppIDNotExist
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Verifying the return of the API when app id doesn't exist
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, updateCurrentActionUsingComRpcFailureAppIDNotExist)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    mAppManagerImpl->updateCurrentAction(APPMANAGER_APP_ID, Plugin::AppManagerImplementation::CurrentAction::APP_ACTION_LAUNCH);

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* * Test Case for GetLoadedAppsJsonRpc
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required JSON-RPC resources
 * Setting Mock for GetLoadedApps() to simulate getting loaded apps
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetLoadedAppsJsonRpc)
{
    Core::hresult status;

    status = createResources();
    std::string nexTennisAppId = "NexTennis";
    AppInfoManager::getInstance().upsert(nexTennisAppId, [&](AppInfo& a) {
        a.setAppInstanceId(nexTennisAppId);
        a.getPackageInfoMutable().type = AppManagerTypes::APPLICATION_TYPE_INTERACTIVE;
    });
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_CALL(*mLifecycleManagerMock, GetLoadedApps(::testing::_, ::testing::_)
    ).WillOnce([&](const bool verbose, std::string &apps) {
        apps = R"([
            {"appId":"NexTennis","appInstanceID":"0295effd-2883-44ed-b614-471e3f682762","activeSessionId":"","targetLifecycleState":6,"lifecycleState":6},
            {"appId":"uktv","appInstanceID":"67fa75b6-0c85-43d4-a591-fd29e7214be5","activeSessionId":"","targetLifecycleState":6,"lifecycleState":6}
        ])";
        return Core::ERROR_NONE;
    });        
    // Simulate a JSON-RPC call
    EXPECT_EQ(Core::ERROR_NONE, mJsonRpcHandler.Invoke(connection, _T("getLoadedApps"), _T("{}"), mJsonRpcResponse));
    EXPECT_STREQ(mJsonRpcResponse.c_str(),
    "[{\"appId\":\"NexTennis\",\"appInstanceId\":\"0295effd-2883-44ed-b614-471e3f682762\",\"activeSessionId\":\"\",\"type\":\"INTERACTIVE_APP\",\"targetLifecycleState\":\"APP_STATE_HIBERNATED\",\"lifecycleState\":\"APP_STATE_HIBERNATED\"},{\"appId\":\"uktv\",\"appInstanceId\":\"67fa75b6-0c85-43d4-a591-fd29e7214be5\",\"activeSessionId\":\"\",\"type\":\"\",\"targetLifecycleState\":\"APP_STATE_HIBERNATED\",\"lifecycleState\":\"APP_STATE_HIBERNATED\"}]");
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* * Test Case for GetLoadedAppsCOMRPCSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PersistentStore/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Setting Mock for GetLoadedApps() to simulate getting loaded apps
 * Verifying the return of the API
 * Releasing the AppManager interface and all related test resources
 */
/*
TEST_F(AppManagerTest, GetLoadedAppsCOMRPCSuccess)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    EXPECT_CALL(*mLifecycleManagerMock, GetLoadedApps(::testing::_, ::testing::_)
    ).WillOnce([&](const bool verbose, Exchange::IAppManager::ILoadedAppInfoIterator*& apps) {
        auto mockIterator = FillLoadedAppsIterator(); // Fill the loaded apps
        apps = mockIterator;
        return Core::ERROR_NONE;
    });
    std::string apps;
    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->GetLoadedApps(apps));
    EXPECT_STREQ(apps.c_str(),
        "[{\"appId\":\"NTV\",\"type\":\"\",\"appInstanceId\":\"0295effd-2883-44ed-b614-471e3f682762\",\"activeSessionId\":\"\",\"targetLifecycleState\":9,\"lifecycleState\":9},"
        "{\"appId\":\"NexTennis\",\"type\":\"\",\"appInstanceId\":\"0295effd-2883-44ed-b614-471e3f682762\",\"activeSessionId\":\"\",\"targetLifecycleState\":8,\"lifecycleState\":8},"
        "{\"appId\":\"uktv\",\"type\":\"\",\"appInstanceId\":\"67fa75b6-0c85-43d4-a591-fd29e7214be5\",\"activeSessionId\":\"\",\"targetLifecycleState\":7,\"lifecycleState\":7},"
        "{\"appId\":\"YouTube\",\"type\":\"\",\"appInstanceId\":\"12345678-1234-1234-1234-123456789012\",\"activeSessionId\":\"\",\"targetLifecycleState\":6,\"lifecycleState\":6},"
        "{\"appId\":\"Netflix\",\"type\":\"\",\"appInstanceId\":\"87654321-4321-4321-4321-210987654321\",\"activeSessionId\":\"\",\"targetLifecycleState\":4,\"lifecycleState\":4},"
        "{\"appId\":\"Spotify\",\"type\":\"\",\"appInstanceId\":\"abcdefab-cdef-abcd-efab-cdefabcdefab\",\"activeSessionId\":\"\",\"targetLifecycleState\":3,\"lifecycleState\":3},"
        "{\"appId\":\"Hulu\",\"type\":\"\",\"appInstanceId\":\"fedcbafe-dcba-fedc-ba98-7654321fedcb\",\"activeSessionId\":\"\",\"targetLifecycleState\":2,\"lifecycleState\":2},"
        "{\"appId\":\"AmazonPrime\",\"type\":\"\",\"appInstanceId\":\"12345678-90ab-cdef-1234-567890abcdef\",\"activeSessionId\":\"\",\"targetLifecycleState\":1,\"lifecycleState\":1}]");

    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }

}
*/
/* * Test Case for OnAppInstallationStatusChangedSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState/PackageManagerRDKEMS Plugin and creating required COM-RPC resources
 * Registering the notification handler to receive the app installation status change event.
 * Simulating the callback for app installation status change
 * Wait for the notification to be signalled
 * Verifying the return of the API
 * Unregistering the notification
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, OnAppInstallationStatusChangedSuccess)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = "YouTube";
    expectedEvent.version = "100.1.30+rialto";
    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    ASSERT_NE(mPackageManagerNotification_cb, nullptr) << "PackageManager notification callback is not registered";
    mPackageManagerNotification_cb->OnAppInstallationStatus(TEST_JSON_INSTALLED_PACKAGE);

    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppInstalled);
    EXPECT_TRUE(signalled & AppManager_onAppInstalled);

    mAppManagerImpl->Unregister(&notification);
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/* * Test Case for OnApplicationStateChangedSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState Plugin and creating required COM-RPC resources
 * Registering the notification handler to receive the application state change event.
 * Simulating the callback for application state change
 * Wait for the notification to be signalled
 * Verifying the return of the API, ensuring that the OnAppLifecycleStateChanged callback is not called/invoked
 * Unregistering the notification
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, OnApplicationStateChangedSuccess)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    uint32_t signalled = AppManager_StateInvalid;
    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);

    ASSERT_NE(mLifecycleManagerStateNotification_cb, nullptr)
        << "LifecycleManagerState notification callback is not registered";

    mLifecycleManagerStateNotification_cb->OnAppLifecycleStateChanged(
        "YouTube",
        "12345678-1234-1234-1234-123456789012",
        Exchange::ILifecycleManager::LifecycleState::ACTIVE,        // Old state
        Exchange::ILifecycleManager::LifecycleState::TERMINATING,   // New state
        "start"
    );
    /* Ensure that the OnAppLifecycleStateChanged callback is not called/invoked */
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLifecycleStateChanged);
    EXPECT_FALSE(signalled & AppManager_onAppLifecycleStateChanged);

    mAppManagerImpl->Unregister(&notification);
    if(status == Core::ERROR_NONE) {
        releaseResources();
    }
}

/*
 * Test Case for handleOnAppLifecycleStateChangedUsingComRpcSuccess
 * Setting up AppManager/LifecycleManager/LifecycleManagerState Plugin and creating required COM-RPC Mock resources
 * Registering the notification handler to receive the app lifecycle state change event.
 * Simulating the callback for application lifecycle state change
 * Wait for the notification to be signalled
 * Verifying the return of the API
 * Unregistering the notification
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, handleOnAppLifecycleStateChangedUsingComRpcSuccess)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = APPMANAGER_APP_INSTANCE;
    expectedEvent.oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED;
    expectedEvent.newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
    expectedEvent.errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_NONE;

    /* Notification registration*/
    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);
    mAppManagerImpl->handleOnAppLifecycleStateChanged(
        APPMANAGER_APP_ID,
        APPMANAGER_APP_INSTANCE,
        Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN,
        Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED,
        Exchange::IAppManager::AppErrorReason::APP_ERROR_NONE);

    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLifecycleStateChanged);
    EXPECT_TRUE(signalled & AppManager_onAppLifecycleStateChanged);

    mAppManagerImpl->Unregister(&notification);
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

TEST_F(AppManagerTest, handleOnAppUnloadedUsingComRpcSuccess)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = APPMANAGER_APP_INSTANCE;

    /* Notification registration*/
    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);
    mAppManagerImpl->handleOnAppUnloaded(APPMANAGER_APP_ID, APPMANAGER_APP_INSTANCE);

    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppUnloaded);
    EXPECT_TRUE(signalled & AppManager_onAppUnloaded);

    mAppManagerImpl->Unregister(&notification);
    if(status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for UnregisterNotificationNotFoundFailure
 * Setting up AppManager Plugin with full resources
 * Attempting to Unregister a notification that was never registered
 * Verifying the return status is Core::ERROR_GENERAL (notification not found path)
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, UnregisterNotificationNotFoundFailure)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    Core::Sink<NotificationHandler> notification;
    /* Do NOT register the notification before unregistering */
    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->Unregister(&notification));

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for handleOnAppUnloadedUsingComRpcFailureEmptyAppId
 * Setting up AppManager Plugin with full resources
 * Calling handleOnAppUnloaded() with an empty appId
 * Verifying the error path (appId not present or empty) is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, handleOnAppUnloadedUsingComRpcFailureEmptyAppId)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    /* Calling with empty appId - exercises the LOGERR("appId not present or empty") path */
    mAppManagerImpl->handleOnAppUnloaded("", APPMANAGER_APP_INSTANCE);

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for handleOnAppLaunchRequestUsingComRpcFailureEmptyAppId
 * Setting up AppManager Plugin with full resources
 * Calling handleOnAppLaunchRequest() with an empty appId
 * Verifying the error path (appId not present or empty) is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, handleOnAppLaunchRequestUsingComRpcFailureEmptyAppId)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    /* Calling with empty appId - exercises the LOGERR("appId not present or empty") path */
    mAppManagerImpl->handleOnAppLaunchRequest("", APPMANAGER_APP_INTENT, "test.source");

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for PreloadAppUsingComRpcFailureEmptyAppID
 * Setting up AppManager Plugin with full resources
 * Calling PreloadApp() with an empty appId
 * Verifying Core::ERROR_INVALID_PARAMETER is returned and error string is set
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, PreloadAppUsingComRpcFailureEmptyAppID)
{
    Core::hresult status;
    std::string error = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, mAppManagerImpl->PreloadApp("", APPMANAGER_APP_LAUNCHARGS, error));
    EXPECT_FALSE(error.empty());

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for ClearAppDataUsingComRpcFailureEmptyAppID
 * Setting up AppManager Plugin with full resources
 * Calling ClearAppData() with an empty appId
 * Verifying Core::ERROR_GENERAL is returned (appId is empty path)
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, ClearAppDataUsingComRpcFailureEmptyAppID)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->ClearAppData(""));

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for ClearAppDataUsingComRpcFailureStorageManagerReturnError
 * Setting up AppManager Plugin with full resources
 * Setting Mock for Clear() to simulate error return from StorageManager
 * Verifying Core::ERROR_GENERAL is returned (failed to clear app data path)
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, ClearAppDataUsingComRpcFailureStorageManagerReturnError)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(*mStorageManagerMock, Clear(::testing::_, ::testing::_))
        .WillOnce([&](const string& appId, string& errorReason) {
            errorReason = "Storage error";
            return Core::ERROR_GENERAL;
        });

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->ClearAppData(APPMANAGER_APP_ID));

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for ClearAppDataUsingComRpcFailureStorageManagerObjectIsNull
 * Setting up only AppManager Plugin (no StorageManager)
 * Calling ClearAppData() when StorageManager remote object is null
 * Verifying Core::ERROR_GENERAL is returned (StorageManager Remote Object is null path)
 * Releasing the AppManager interface only
 */
TEST_F(AppManagerTest, ClearAppDataUsingComRpcFailureStorageManagerObjectIsNull)
{
    createAppManagerImpl();

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->ClearAppData(APPMANAGER_APP_ID));

    releaseAppManagerImpl();
}

/*
 * Test Case for ClearAllAppDataUsingComRpcFailureClearAllReturnError
 * Setting up AppManager Plugin with full resources
 * Setting Mock for ClearAll() to simulate error return from StorageManager
 * Verifying Core::ERROR_GENERAL is returned (failed to clear all app data path)
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, ClearAllAppDataUsingComRpcFailureClearAllReturnError)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(*mStorageManagerMock, ClearAll(::testing::_, ::testing::_))
        .WillOnce([&](const string& exemptionAppIds, string& errorReason) {
            errorReason = "Storage error";
            return Core::ERROR_GENERAL;
        });

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->ClearAllAppData());

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for ClearAllAppDataUsingComRpcFailureStorageManagerObjectIsNull
 * Setting up only AppManager Plugin (no StorageManager)
 * Calling ClearAllAppData() when StorageManager remote object is null
 * Verifying Core::ERROR_GENERAL is returned (StorageManager Remote Object is null path)
 * Releasing the AppManager interface only
 */
TEST_F(AppManagerTest, ClearAllAppDataUsingComRpcFailureStorageManagerObjectIsNull)
{
    createAppManagerImpl();

    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->ClearAllAppData());

    releaseAppManagerImpl();
}

/*
 * Test Case for OnAppInstallationStatusEmptyJsonResponse
 * Setting up AppManager Plugin with full resources
 * Calling OnAppInstallationStatus() with an empty JSON string
 * Verifying the empty jsonresponse error path is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, OnAppInstallationStatusEmptyJsonResponse)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    ASSERT_NE(mPackageManagerNotification_cb, nullptr)
        << "PackageManager notification callback is not registered";
    /* Pass empty string - exercises the LOGERR("jsonresponse string is empty") path */
    mPackageManagerNotification_cb->OnAppInstallationStatus("");

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for OnAppInstallationStatusInvalidJsonResponse
 * Setting up AppManager Plugin with full resources
 * Calling OnAppInstallationStatus() with a non-array JSON string
 * Verifying the JSON parse failure error path is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, OnAppInstallationStatusInvalidJsonResponse)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    ASSERT_NE(mPackageManagerNotification_cb, nullptr)
        << "PackageManager notification callback is not registered";
    /* Pass a non-array JSON string - exercises the LOGERR("Failed to parse JSON string") path */
    mPackageManagerNotification_cb->OnAppInstallationStatus("{\"invalid\": true}");

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for OnAppInstallationStatusMissingPackageId
 * Setting up AppManager Plugin with full resources
 * Calling OnAppInstallationStatus() with JSON missing the packageId field
 * Verifying the "appId is missing or empty" error path in Dispatch is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, OnAppInstallationStatusMissingPackageId)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    ASSERT_NE(mPackageManagerNotification_cb, nullptr)
        << "PackageManager notification callback is not registered";

    uint32_t signalled = AppManager_StateInvalid;
    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);

    /* JSON with no packageId - covers the LOGERR("appId is missing or empty") in Dispatch APP_EVENT_INSTALLATION_STATUS */
    mPackageManagerNotification_cb->OnAppInstallationStatus(R"([{"state":"INSTALLED","version":"1.0.0"}])");

    /* Wait for the async Job to complete; empty appId means no notification fires.
     * Timeout ensures ~Job() has decremented the mAppManagerImpl refcount. */
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppInstalled);
    EXPECT_FALSE(signalled & AppManager_onAppInstalled);

    mAppManagerImpl->Unregister(&notification);
    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for GetInstalledAppsWithLastActiveTimeSuccess
 * Setting up AppManager Plugin with full resources
 * Pre-populating mAppInfo with an entry that has a non-zero lastActiveStateChangeTime
 * Verifying GetInstalledApps() produces output that includes lastActiveTime (strftime path)
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetInstalledAppsWithLastActiveTimeSuccess)
{
    Core::hresult status;
    std::string apps = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    /* Pre-populate AppInfoManager with appId having a valid lastActiveStateChangeTime */
    AppInfoManager::getInstance().upsert(APPMANAGER_APP_ID, [](AppInfo& a) {
        a.setAppInstanceId(APPMANAGER_APP_INSTANCE);
        a.setAppNewState(Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE);
        a.setLastActiveIndex(1);
        /* Set a valid timestamp (non-zero tv_sec) */
        timespec ts{};
        ts.tv_sec = 1700000000;
        ts.tv_nsec = 123456789;
        a.setLastActiveStateChangeTime(ts);
    });

    EXPECT_CALL(*mPackageInstallerMock, ListPackages(::testing::_))
        .WillOnce([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
            auto mockIterator = FillPackageIterator();
            packages = mockIterator;
            return Core::ERROR_NONE;
        });

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->GetInstalledApps(apps));
    /* Verify output is populated */
    EXPECT_FALSE(apps.empty());
    EXPECT_NE(apps.find(APPMANAGER_APP_ID), std::string::npos);

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for GetInstallAppTypeSystemApp
 * Setting up AppManager Plugin with full resources
 * Calling getInstallAppType() with APPLICATION_TYPE_SYSTEM
 * Verifying the SYSTEM_APP branch returns the expected string
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetInstallAppTypeSystemApp)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    std::string result = mAppManagerImpl->getInstallAppType(
        Plugin::AppManagerImplementation::ApplicationType::APPLICATION_TYPE_SYSTEM);
    EXPECT_STREQ("SYSTEM_APP", result.c_str());

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for CheckInstallUninstallBlockWithInstallationBlockedState
 * Setting up AppManager Plugin with full resources
 * Setting ListPackages() to return a package with INSTALLATION_BLOCKED state
 * Calling checkInstallUninstallBlock() and verifying it returns true
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, CheckInstallUninstallBlockWithInstallationBlockedState)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(*mPackageInstallerMock, ListPackages(::testing::_))
        .WillOnce([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
            std::list<Exchange::IPackageInstaller::Package> packageList;
            Exchange::IPackageInstaller::Package pkg;
            pkg.packageId = APPMANAGER_APP_ID;
            pkg.version = APPMANAGER_APP_VERSION;
            pkg.state = Exchange::IPackageInstaller::InstallState::INSTALLATION_BLOCKED;
            packageList.emplace_back(pkg);
            packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
            return Core::ERROR_NONE;
        });

    bool blocked = mAppManagerImpl->checkInstallUninstallBlock(APPMANAGER_APP_ID);
    EXPECT_TRUE(blocked);

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for CheckInstallUninstallBlockWithUninstallBlockedState
 * Setting up AppManager Plugin with full resources
 * Setting ListPackages() to return a package with UNINSTALL_BLOCKED state
 * Calling checkInstallUninstallBlock() and verifying it returns true
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, CheckInstallUninstallBlockWithUninstallBlockedState)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(*mPackageInstallerMock, ListPackages(::testing::_))
        .WillOnce([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
            std::list<Exchange::IPackageInstaller::Package> packageList;
            Exchange::IPackageInstaller::Package pkg;
            pkg.packageId = APPMANAGER_APP_ID;
            pkg.version = APPMANAGER_APP_VERSION;
            pkg.state = Exchange::IPackageInstaller::InstallState::UNINSTALL_BLOCKED;
            packageList.emplace_back(pkg);
            packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
            return Core::ERROR_NONE;
        });

    bool blocked = mAppManagerImpl->checkInstallUninstallBlock(APPMANAGER_APP_ID);
    EXPECT_TRUE(blocked);

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for CheckInstallUninstallBlockWithNoMatchingPackage
 * Setting up AppManager Plugin with full resources
 * Setting ListPackages() to return a package that does not match the queried appId
 * Calling checkInstallUninstallBlock() and verifying it returns false
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, CheckInstallUninstallBlockWithNoMatchingPackage)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(*mPackageInstallerMock, ListPackages(::testing::_))
        .WillOnce([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
            auto mockIterator = FillPackageIterator();
            packages = mockIterator;
            return Core::ERROR_NONE;
        });

    bool blocked = mAppManagerImpl->checkInstallUninstallBlock(APPMANAGER_WRONG_APP_ID);
    EXPECT_FALSE(blocked);

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for CheckInstallUninstallBlockListPackagesReturnError
 * Setting up AppManager Plugin with full resources
 * Setting ListPackages() to return an error
 * Calling checkInstallUninstallBlock() and verifying it returns false
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, CheckInstallUninstallBlockListPackagesReturnError)
{
    Core::hresult status;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_CALL(*mPackageInstallerMock, ListPackages(::testing::_))
        .WillOnce([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
            packages = nullptr;
            return Core::ERROR_GENERAL;
        });

    bool blocked = mAppManagerImpl->checkInstallUninstallBlock(APPMANAGER_APP_ID);
    EXPECT_FALSE(blocked);

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for LaunchAppLockFailureListPackagesFails
 * Setting up AppManager Plugin with full resources
 * Configuring ListPackages() to succeed on the first and third call, and return nullptr on the second call
 * so fetchAppPackageList() fails inside packageLock(), while the IsInstalled() error-path check returns installed=true
 * Waiting for the resulting OnAppLifecycleStateChanged event (APP_ERROR_PACKAGE_LOCK)
 * Verifying that the error path from a packageLock() list failure correctly triggers a state-changed event
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, LaunchAppLockFailureListPackagesFails)
{
    Core::hresult status;
    uint32_t signalled = AppManager_StateInvalid;
    Core::Sink<NotificationHandler> notification;
    ExpectedAppLifecycleEvent expectedEvent;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = "";
    expectedEvent.newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
    expectedEvent.oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED;
    expectedEvent.errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_PACKAGE_LOCK;

    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    /* IsAppLoaded returns not loaded so LaunchApp proceeds into packageLock */
    EXPECT_CALL(*mLifecycleManagerMock, IsAppLoaded(::testing::_, ::testing::_))
        .WillRepeatedly([&](const std::string& appId, bool& loaded) {
            loaded = false;
            return Core::ERROR_NONE;
        });
    /* First call (IsInstalled gate in LaunchApp): return installed so we pass the gate */
    /* Second call (inside packageLock fetchAppPackageList): return nullptr to trigger lock failure */
    /* Third call (IsInstalled in error path): return installed so APP_ERROR_PACKAGE_LOCK is used */
    EXPECT_CALL(*mPackageInstallerMock, ListPackages(::testing::_))
        .WillOnce([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
            auto mockIterator = FillPackageIterator();
            packages = mockIterator;
            return Core::ERROR_NONE;
        })
        .WillOnce([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
            packages = nullptr;
            return Core::ERROR_NONE;
        })
        .WillRepeatedly([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
            auto mockIterator = FillPackageIterator();
            packages = mockIterator;
            return Core::ERROR_NONE;
        });

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->LaunchApp(APPMANAGER_APP_ID, APPMANAGER_APP_INTENT, APPMANAGER_APP_LAUNCHARGS));

    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLifecycleStateChanged);
    EXPECT_TRUE(signalled & AppManager_onAppLifecycleStateChanged);

    mAppManagerImpl->Unregister(&notification);
    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for LaunchAppLockFailureLockReturnError
 * Setting up AppManager Plugin with full resources
 * Configuring ListPackages() to succeed and Lock() to return error
 * Waiting for the resulting OnAppLifecycleStateChanged event (APP_ERROR_PACKAGE_LOCK)
 * Verifying the packageLock failure path (Lock API fails) triggers the correct event
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, LaunchAppLockFailureLockReturnError)
{
    Core::hresult status;
    uint32_t signalled = AppManager_StateInvalid;
    Core::Sink<NotificationHandler> notification;
    ExpectedAppLifecycleEvent expectedEvent;

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = "";
    expectedEvent.newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNKNOWN;
    expectedEvent.oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED;
    expectedEvent.errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_PACKAGE_LOCK;

    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    EXPECT_CALL(*mLifecycleManagerMock, IsAppLoaded(::testing::_, ::testing::_))
        .WillRepeatedly([&](const std::string& appId, bool& loaded) {
            loaded = false;
            return Core::ERROR_NONE;
        });
    EXPECT_CALL(*mPackageInstallerMock, ListPackages(::testing::_))
        .WillRepeatedly([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
            auto mockIterator = FillPackageIterator();
            packages = mockIterator;
            return Core::ERROR_NONE;
        });
    EXPECT_CALL(*mPackageManagerMock, Lock(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly([&](const string& packageId, const string& version,
            const Exchange::IPackageHandler::LockReason& lockReason, uint32_t& lockId,
            string& unpackedPath, Exchange::RuntimeConfig& configMetadata,
            Exchange::IPackageHandler::ILockIterator*& appMetadata) {
            return Core::ERROR_GENERAL;
        });

    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->LaunchApp(APPMANAGER_APP_ID, APPMANAGER_APP_INTENT, APPMANAGER_APP_LAUNCHARGS));

    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLifecycleStateChanged);
    EXPECT_TRUE(signalled & AppManager_onAppLifecycleStateChanged);

    mAppManagerImpl->Unregister(&notification);
    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for GetCustomValuesWithAipathFile
 * Setting up AppManager Plugin with full resources
 * Mocking fopen() to return a non-null FILE pointer simulating /tmp/aipath file exists
 * Calling PreloadApp() which internally triggers getCustomValues() via the worker thread
 * Verifying PreloadApp still returns Core::ERROR_NONE
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetCustomValuesWithAipathFile)
{
    Core::hresult status;
    std::string error = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);
    mPreLoadSpawmCalled = false;

    /* Simulate /tmp/aipath file exists: fopen returns a real read-only file descriptor
     * backed by a temporary file that contains at least one non-empty line.
     */
    FILE* aipathFile = ::tmpfile();
    ASSERT_NE(aipathFile, nullptr);
    /* Write three newline-terminated lines (apppath/runtimepath/command) so getCustomValues() */
    /* can safely call pop_back() on each non-empty string.                        */
    std::fputs("/opt/test/app/path\n", aipathFile);       /* apppath    */
    std::fputs("/opt/test/runtime/path\n", aipathFile);   /* runtimepath */
    std::fputs("test_command --arg\n", aipathFile);       /* command    */
    std::fflush(aipathFile);
    std::rewind(aipathFile);

    ON_CALL(*p_wrapsImplMock, fopen(::testing::_, ::testing::_))
        .WillByDefault([&](const char* pathname, const char* mode) -> FILE* {
            if ((nullptr != pathname) && (std::string(pathname) == "/tmp/aipath")) {
                /* Return the prepared temporary file for /tmp/aipath. */
                return aipathFile;
            }
            /* Delegate to the real fopen for all other paths. */
            return ::fopen(pathname, mode);
        });

    preLaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::PAUSED);
    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->PreloadApp(APPMANAGER_APP_ID, APPMANAGER_APP_LAUNCHARGS, error));
    {
        std::unique_lock<std::mutex> lock(mPreLoadMutex);
        ASSERT_TRUE(mPreLoadCV.wait_for(lock, std::chrono::seconds(10), [&] { return mPreLoadSpawmCalled; }));
    }

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for LifecycleStateChangedLoadingToLoadingKillApp
 * Setting up AppManager Plugin with full resources
 * Pre-populating mAppInfo with an entry in LOADING state
 * Triggering OnAppLifecycleStateChanged with LOADING→LOADING transition
 * Verifying KillApp is called (loading→loading kill path) and the state-changed event fires
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, LifecycleStateChangedLoadingToLoadingKillApp)
{
    Core::hresult status;
    uint32_t signalled = AppManager_StateInvalid;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    /* Pre-populate AppInfoManager so the appId/appInstanceId match is found in APP_STATE_LOADING */
    AppInfoManager::getInstance().upsert(APPMANAGER_APP_ID, [](AppInfo& a) {
        a.setAppInstanceId(APPMANAGER_APP_INSTANCE);
        a.setAppNewState(Exchange::IAppManager::AppLifecycleState::APP_STATE_LOADING);
    });

    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = APPMANAGER_APP_INSTANCE;
    expectedEvent.newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_LOADING;
    expectedEvent.oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_LOADING;
    expectedEvent.errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_NONE;

    /* Expect KillApp to be called as a result of the loading→loading transition */
    EXPECT_CALL(*mLifecycleManagerMock, KillApp(::testing::_, ::testing::_, ::testing::_))
        .WillOnce([&](const string& appInstanceId, string& errorReason, bool& success) {
            success = true;
            return Core::ERROR_NONE;
        });

    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    ASSERT_NE(mLifecycleManagerStateNotification_cb, nullptr)
        << "LifecycleManagerState notification callback is not registered";

    /* Trigger LOADING→LOADING which activates the kill path */
    mLifecycleManagerStateNotification_cb->OnAppLifecycleStateChanged(
        APPMANAGER_APP_ID,
        APPMANAGER_APP_INSTANCE,
        Exchange::ILifecycleManager::LifecycleState::LOADING, /* old */
        Exchange::ILifecycleManager::LifecycleState::LOADING, /* new */
        ""
    );

    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLifecycleStateChanged);
    EXPECT_TRUE(signalled & AppManager_onAppLifecycleStateChanged);

    /* Drain async worker jobs before teardown so Job::~Job() has released
     * mAppManagerImpl references. Do not wait for UNLOADED here because this
     * test does not explicitly simulate a second lifecycle callback. */
    signalled = notification.WaitForRequestStatus(JOB_DRAIN_TIMEOUT, AppManager_onAppInstalled);
    EXPECT_FALSE(signalled & AppManager_onAppInstalled);

    mAppManagerImpl->Unregister(&notification);
    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for DispatchLifecycleEventEmptyAppId
 * Setting up AppManager Plugin with full resources
 * Calling handleOnAppLifecycleStateChanged with an empty appId
 * Verifying the LOGERR("appId not present or empty") path in Dispatch is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, DispatchLifecycleEventEmptyAppId)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    uint32_t signalled = AppManager_StateInvalid;
    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);

    /* Empty appId passed through handleOnAppLifecycleStateChanged -> dispatchEvent ->
     * Dispatch, where the non-empty check at L251 fires the LOGERR path */
    mAppManagerImpl->handleOnAppLifecycleStateChanged(
        "",
        APPMANAGER_APP_INSTANCE,
        Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE,
        Exchange::IAppManager::AppLifecycleState::APP_STATE_LOADING,
        Exchange::IAppManager::AppErrorReason::APP_ERROR_NONE);

    /* Wait for the worker thread to dispatch and fully release its Job reference.
     * Empty appId means no notification fires; the timeout ensures ~Job() has
     * decremented the mAppManagerImpl refcount before releaseResources() runs. */
    signalled = notification.WaitForRequestStatus(DRAIN_TIMEOUT, AppManager_onAppLifecycleStateChanged);
    EXPECT_FALSE(signalled & AppManager_onAppLifecycleStateChanged);

    mAppManagerImpl->Unregister(&notification);
    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for OnAppInstallationStatusInstalledEmptyVersion
 * Setting up AppManager Plugin with full resources
 * Calling OnAppInstallationStatus() with state INSTALLED but no version field
 * Verifying the LOGERR("version is empty for installed app") path (L316) is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, OnAppInstallationStatusInstalledEmptyVersion)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    ASSERT_NE(mPackageManagerNotification_cb, nullptr)
        << "PackageManager notification callback is not registered";

    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = "com.test.app";
    expectedEvent.version = "";
    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    /* JSON with INSTALLED state but no version field - exercises L316 LOGERR path.
     * Note: OnAppInstalled("com.test.app", "") is still dispatched after the LOGERR. */
    mPackageManagerNotification_cb->OnAppInstallationStatus(
        R"([{"packageId":"com.test.app","state":"INSTALLED"}])");

    /* Wait for the async Job to complete and release its mAppManagerImpl reference
     * before releaseResources() tears down the mocks. */
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppInstalled);
    EXPECT_TRUE(signalled & AppManager_onAppInstalled);

    mAppManagerImpl->Unregister(&notification);
    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for OnAppInstallationStatusUninstalled
 * Setting up AppManager Plugin with full resources
 * Calling OnAppInstallationStatus() with state UNINSTALLED
 * Verifying the OnAppUninstalled notification path (L321-324) is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, OnAppInstallationStatusUninstalled)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    ASSERT_NE(mPackageManagerNotification_cb, nullptr)
        << "PackageManager notification callback is not registered";

    uint32_t signalled = AppManager_StateInvalid;
    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);

    /* JSON with UNINSTALLED state - exercises L321-324 path, calls OnAppUninstalled */
    mPackageManagerNotification_cb->OnAppInstallationStatus(
        R"([{"packageId":"com.test.app","state":"UNINSTALLED"}])");

    /* Wait for the async Job to complete and release its mAppManagerImpl reference.
     * OnAppUninstalled fires but is not tracked; timeout ensures ~Job() has run. */
    signalled = notification.WaitForRequestStatus(JOB_DRAIN_TIMEOUT, AppManager_onAppInstalled);
    EXPECT_FALSE(signalled & AppManager_onAppInstalled);

    mAppManagerImpl->Unregister(&notification);
    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for OnAppInstallationStatusUnknownStatus
 * Setting up AppManager Plugin with full resources
 * Calling OnAppInstallationStatus() with an unknown install status
 * Verifying the LOGWARN("install status") path (L328) is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, OnAppInstallationStatusUnknownStatus)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    ASSERT_NE(mPackageManagerNotification_cb, nullptr)
        << "PackageManager notification callback is not registered";

    uint32_t signalled = AppManager_StateInvalid;
    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);

    /* JSON with unknown status ("PENDING") - exercises L328 LOGWARN path, no notification fired */
    mPackageManagerNotification_cb->OnAppInstallationStatus(
        R"([{"packageId":"com.test.app","state":"PENDING"}])");

    /* Wait for the async Job to complete and release its mAppManagerImpl reference.
     * No notification fires for unknown status; timeout ensures ~Job() has run. */
    signalled = notification.WaitForRequestStatus(JOB_DRAIN_TIMEOUT, AppManager_onAppInstalled);
    EXPECT_FALSE(signalled & AppManager_onAppInstalled);

    mAppManagerImpl->Unregister(&notification);
    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for HandleOnAppLaunchRequestEmptyIntent
 * Setting up AppManager Plugin with full resources
 * Calling handleOnAppLaunchRequest() with a valid appId but empty intent
 * Verifying the LOGERR("intent is empty for Launch app") path (L351) is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, HandleOnAppLaunchRequestEmptyIntent)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.intent = "";
    expectedEvent.source = "test.source";
    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    /* Non-empty appId, empty intent - exercises L351 LOGERR path;
     * OnAppLaunchRequest is still dispatched after the LOGERR. */
    mAppManagerImpl->handleOnAppLaunchRequest(APPMANAGER_APP_ID, "", "test.source");

    /* Wait for the async Job to complete before releasing resources */
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLaunchRequest);
    EXPECT_TRUE(signalled & AppManager_onAppLaunchRequest);

    mAppManagerImpl->Unregister(&notification);
    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for HandleOnAppUnloadedEmptyInstanceId
 * Setting up AppManager Plugin with full resources
 * Calling handleOnAppUnloaded() with valid appId but empty appInstanceId
 * Verifying the LOGERR("appInstanceId is empty for Unloaded app") path (L382) is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, HandleOnAppUnloadedEmptyInstanceId)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = "";

    /* Register notification to synchronize with the async worker-pool Job */
    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    /* Non-empty appId, empty appInstanceId - exercises L382 LOGERR path */
    mAppManagerImpl->handleOnAppUnloaded(APPMANAGER_APP_ID, "");

    /* Wait for OnAppUnloaded to be dispatched before releasing resources */
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppUnloaded);
    EXPECT_TRUE(signalled & AppManager_onAppUnloaded);

    mAppManagerImpl->Unregister(&notification);

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for PackageUnlockEntryFoundUnlockSuccess
 * Setting up AppManager Plugin with full resources
 * Pre-populating mAppInfo with an entry matching appId/appInstanceId
 * Verifying packageUnLock calls Unlock() and mAppInfo entry is removed (L729-732, L861-863)
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, PackageUnlockEntryFoundUnlockSuccess)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    /* Pre-populate AppInfoManager so that packageUnLock finds the entry */
    AppInfoManager::getInstance().upsert(APPMANAGER_APP_ID, [](AppInfo& a) {
        a.setAppInstanceId(APPMANAGER_APP_INSTANCE);
        a.getPackageInfoMutable().version = APPMANAGER_APP_VERSION;
    });

    EXPECT_CALL(*mPackageManagerMock, Unlock(APPMANAGER_APP_ID, APPMANAGER_APP_VERSION))
        .WillOnce([&](const string& packageId, const string& version) {
            return Core::ERROR_NONE;
        });

    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = APPMANAGER_APP_INSTANCE;

    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    /* handleOnAppUnloaded triggers packageUnLock then removeAppInfoByAppId */
    mAppManagerImpl->handleOnAppUnloaded(APPMANAGER_APP_ID, APPMANAGER_APP_INSTANCE);

    /* Wait for OnAppUnloaded to be dispatched before checking results */
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppUnloaded);
    EXPECT_TRUE(signalled & AppManager_onAppUnloaded);

    mAppManagerImpl->Unregister(&notification);

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for PackageUnlockEntryFoundUnlockFails
 * Setting up AppManager Plugin with full resources
 * Pre-populating mAppInfo with a matching entry
 * Verifying LOGERR("Failed to PackageManager Unlock") path (L865-867) is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, PackageUnlockEntryFoundUnlockFails)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    /* Pre-populate AppInfoManager so packageUnLock finds the entry */
    AppInfoManager::getInstance().upsert(APPMANAGER_APP_ID, [](AppInfo& a) {
        a.setAppInstanceId(APPMANAGER_APP_INSTANCE);
        a.getPackageInfoMutable().version = APPMANAGER_APP_VERSION;
    });

    EXPECT_CALL(*mPackageManagerMock, Unlock(APPMANAGER_APP_ID, APPMANAGER_APP_VERSION))
        .WillOnce([&](const string& packageId, const string& version) {
            return Core::ERROR_GENERAL;
        });

    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = APPMANAGER_APP_INSTANCE;

    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    /* handleOnAppUnloaded triggers packageUnLock - Unlock returns error covering L865-867 */
    mAppManagerImpl->handleOnAppUnloaded(APPMANAGER_APP_ID, APPMANAGER_APP_INSTANCE);

    /* Wait for OnAppUnloaded to complete before releasing resources */
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppUnloaded);
    EXPECT_TRUE(signalled & AppManager_onAppUnloaded);

    mAppManagerImpl->Unregister(&notification);

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for PackageUnlockHandlerNull
 * Setting up AppManager Plugin without PackageManager mock (mPackageManagerHandlerObject == null)
 * Pre-populating mAppInfo with a matching entry
 * Verifying the LOGERR("PackageManager handler is null") path (L875) is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, PackageUnlockHandlerNull)
{
    /* Use createAppManagerImpl() so mPackageManagerHandlerObject remains null */
    createAppManagerImpl();

    /* Pre-populate AppInfoManager so packageUnLock finds the entry (null handler path) */
    AppInfoManager::getInstance().upsert(APPMANAGER_APP_ID, [](AppInfo& a) {
        a.setAppInstanceId(APPMANAGER_APP_INSTANCE);
        a.getPackageInfoMutable().version = APPMANAGER_APP_VERSION;
    });

    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = APPMANAGER_APP_INSTANCE;

    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    /* handleOnAppUnloaded triggers packageUnLock -> null handler -> L875 LOGERR */
    mAppManagerImpl->handleOnAppUnloaded(APPMANAGER_APP_ID, APPMANAGER_APP_INSTANCE);

    /* Wait for OnAppUnloaded so the Job's AddRef is released before mServiceMock is deleted */
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppUnloaded);
    EXPECT_TRUE(signalled & AppManager_onAppUnloaded);

    mAppManagerImpl->Unregister(&notification);

    releaseAppManagerImpl();
}

/*
 * Test Case for CloseAppEmptyAppId
 * Setting up AppManager Plugin with full resources
 * Calling CloseApp() with an empty appId
 * Verifying the LOGERR("appId is empty") error path (L1004) is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, CloseAppEmptyAppId)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    /* Empty appId - exercises L1004 LOGERR path */
    EXPECT_EQ(Core::ERROR_GENERAL, mAppManagerImpl->CloseApp(""));

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for AppManagerInformation
 * Setting up AppManager Plugin with full resources
 * Calling Information() on the plugin
 * Verifying it returns an empty string (L176-179)
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, AppManagerInformation)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(string(""), plugin->Information());

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for LICMapAppLifecycleStateUnloaded
 * Setting up AppManager Plugin with full resources
 * Pre-populating mAppInfo and triggering OnAppLifecycleStateChanged with LOADING->UNLOADED
 * Verifying mapAppLifecycleState UNLOADED path (L760-763) is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, LICMapAppLifecycleStateUnloaded)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    /* Pre-populate AppInfoManager so the appId/appInstanceId match is found */
    AppInfoManager::getInstance().upsert(APPMANAGER_APP_ID, [](AppInfo& a) {
        a.setAppInstanceId(APPMANAGER_APP_INSTANCE);
        a.setAppNewState(Exchange::IAppManager::AppLifecycleState::APP_STATE_LOADING);
    });

    ASSERT_NE(mLifecycleManagerStateNotification_cb, nullptr)
        << "LifecycleManagerState notification callback is not registered";

    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = APPMANAGER_APP_INSTANCE;
    expectedEvent.newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED;
    expectedEvent.oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_LOADING;
    expectedEvent.errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_ABORT;

    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    /* LOADING -> UNLOADED triggers mapAppLifecycleState(UNLOADED) path */
    mLifecycleManagerStateNotification_cb->OnAppLifecycleStateChanged(
        APPMANAGER_APP_ID,
        APPMANAGER_APP_INSTANCE,
        Exchange::ILifecycleManager::LifecycleState::LOADING,   /* old */
        Exchange::ILifecycleManager::LifecycleState::UNLOADED,  /* new */
        ""
    );

    /* Wait for APP_EVENT_UNLOADED (the last of the two dispatched jobs) */
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppUnloaded);
    EXPECT_TRUE(signalled & AppManager_onAppUnloaded);

    mAppManagerImpl->Unregister(&notification);

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for LICMapAppLifecycleStateInitializing
 * Setting up AppManager Plugin with full resources
 * Pre-populating mAppInfo and triggering OnAppLifecycleStateChanged with LOADING->INITIALIZING
 * Verifying mapAppLifecycleState INITIALIZING path (L776-779) is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, LICMapAppLifecycleStateInitializing)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    AppInfoManager::getInstance().upsert(APPMANAGER_APP_ID, [](AppInfo& a) {
        a.setAppInstanceId(APPMANAGER_APP_INSTANCE);
        a.setAppNewState(Exchange::IAppManager::AppLifecycleState::APP_STATE_LOADING);
    });

    ASSERT_NE(mLifecycleManagerStateNotification_cb, nullptr)
        << "LifecycleManagerState notification callback is not registered";

    /* LOADING -> INITIALIZING triggers mapAppLifecycleState(INITIALIZING) path */
    mLifecycleManagerStateNotification_cb->OnAppLifecycleStateChanged(
        APPMANAGER_APP_ID,
        APPMANAGER_APP_INSTANCE,
        Exchange::ILifecycleManager::LifecycleState::LOADING,       /* old */
        Exchange::ILifecycleManager::LifecycleState::INITIALIZING,  /* new */
        ""
    );

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for LICMapAppLifecycleStateSuspended
 * Setting up AppManager Plugin with full resources
 * Pre-populating mAppInfo and triggering OnAppLifecycleStateChanged with ACTIVE->SUSPENDED
 * Verifying mapAppLifecycleState SUSPENDED path (L780-783) is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, LICMapAppLifecycleStateSuspended)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    AppInfoManager::getInstance().upsert(APPMANAGER_APP_ID, [](AppInfo& a) {
        a.setAppInstanceId(APPMANAGER_APP_INSTANCE);
        a.setAppNewState(Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE);
    });

    ASSERT_NE(mLifecycleManagerStateNotification_cb, nullptr)
        << "LifecycleManagerState notification callback is not registered";

    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = APPMANAGER_APP_INSTANCE;
    expectedEvent.newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_SUSPENDED;
    expectedEvent.oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE;
    expectedEvent.errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_NONE;

    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    /* ACTIVE -> SUSPENDED triggers mapAppLifecycleState(SUSPENDED) path */
    mLifecycleManagerStateNotification_cb->OnAppLifecycleStateChanged(
        APPMANAGER_APP_ID,
        APPMANAGER_APP_INSTANCE,
        Exchange::ILifecycleManager::LifecycleState::ACTIVE,    /* old */
        Exchange::ILifecycleManager::LifecycleState::SUSPENDED, /* new */
        ""
    );

    /* Wait for OnAppLifecycleStateChanged to be dispatched before releasing resources */
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLifecycleStateChanged);
    EXPECT_TRUE(signalled & AppManager_onAppLifecycleStateChanged);

    mAppManagerImpl->Unregister(&notification);

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for LICMapAppLifecycleStateUnknown
 * Setting up AppManager Plugin with full resources
 * Triggering OnAppLifecycleStateChanged with an out-of-range state value
 * Verifying the default/unknown state path (L792-795) in mapAppLifecycleState is covered
 * Also verifying the early-return path (L809-813) when newAppState is UNKNOWN
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, LICMapAppLifecycleStateUnknown)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    ASSERT_NE(mLifecycleManagerStateNotification_cb, nullptr)
        << "LifecycleManagerState notification callback is not registered";

    /* Cast an out-of-range value to LifecycleState to trigger the default branch */
    const auto unknownState = static_cast<Exchange::ILifecycleManager::LifecycleState>(999);

    mLifecycleManagerStateNotification_cb->OnAppLifecycleStateChanged(
        APPMANAGER_APP_ID,
        APPMANAGER_APP_INSTANCE,
        Exchange::ILifecycleManager::LifecycleState::ACTIVE, /* old */
        unknownState,                                        /* new - triggers default+early return */
        ""
    );

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for LICOnAppStateChangedWithErrorReason
 * Setting up AppManager Plugin with full resources
 * Pre-populating mAppInfo with an entry for the appId
 * Calling LifecycleInterfaceConnector::OnAppStateChanged with a non-empty errorReason
 * Verifying the errorReason handling path (L931-937) and mapErrorReason (L941-963) are covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, LICOnAppStateChangedWithErrorReason)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    /* Pre-populate AppInfoManager so the appId lookup succeeds (covers L920-924) */
    AppInfoManager::getInstance().upsert(APPMANAGER_APP_ID, [](AppInfo& a) {
        a.setAppInstanceId(APPMANAGER_APP_INSTANCE);
        a.setAppNewState(Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE);
    });

    ASSERT_NE(Plugin::LifecycleInterfaceConnector::_instance, nullptr)
        << "LifecycleInterfaceConnector instance is not set";

    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = APPMANAGER_APP_INSTANCE;
    expectedEvent.newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED;
    expectedEvent.oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE;
    expectedEvent.errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_CREATE_DISPLAY;

    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    /* Call with non-empty errorReason to exercise L931-937 and mapErrorReason */
    Plugin::LifecycleInterfaceConnector::_instance->OnAppStateChanged(
        APPMANAGER_APP_ID,
        Exchange::ILifecycleManager::LifecycleState::ACTIVE,
        "ERROR_CREATE_DISPLAY");

    /* Wait for OnAppLifecycleStateChanged to be dispatched before releasing resources */
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLifecycleStateChanged);
    EXPECT_TRUE(signalled & AppManager_onAppLifecycleStateChanged);

    mAppManagerImpl->Unregister(&notification);

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for LICOnAppStateChangedAppIdNotFound
 * Setting up AppManager Plugin with full resources
 * Calling LifecycleInterfaceConnector::OnAppStateChanged with an appId not in mAppInfo
 * Verifying the LOGERR("appId not found in database") path (L927) is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, LICOnAppStateChangedAppIdNotFound)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    /* mAppInfo is empty - appId lookup fails, covering L927 LOGERR path */
    ASSERT_NE(Plugin::LifecycleInterfaceConnector::_instance, nullptr)
        << "LifecycleInterfaceConnector instance is not set";

    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = "";
    expectedEvent.newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED;
    expectedEvent.oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED;
    expectedEvent.errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_DOBBY_SPEC;

    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    Plugin::LifecycleInterfaceConnector::_instance->OnAppStateChanged(
        APPMANAGER_APP_ID,
        Exchange::ILifecycleManager::LifecycleState::ACTIVE,
        "ERROR_DOBBY_SPEC");

    /* Wait for OnAppLifecycleStateChanged to be dispatched before releasing resources */
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLifecycleStateChanged);
    EXPECT_TRUE(signalled & AppManager_onAppLifecycleStateChanged);

    mAppManagerImpl->Unregister(&notification);

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for LICOnAppStateChangedMapErrorReasonInvalidParam
 * Setting up AppManager Plugin with full resources
 * Pre-populating mAppInfo and calling OnAppStateChanged with ERROR_INVALID_PARAM
 * Verifying the mapErrorReason ERROR_INVALID_PARAM branch (L954-956) is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, LICOnAppStateChangedMapErrorReasonInvalidParam)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    AppInfoManager::getInstance().upsert(APPMANAGER_APP_ID, [](AppInfo& a) {
        a.setAppInstanceId(APPMANAGER_APP_INSTANCE);
        a.setAppNewState(Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE);
    });

    ASSERT_NE(Plugin::LifecycleInterfaceConnector::_instance, nullptr)
        << "LifecycleInterfaceConnector instance is not set";

    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = APPMANAGER_APP_INSTANCE;
    expectedEvent.newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED;
    expectedEvent.oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE;
    expectedEvent.errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_INVALID_PARAM;

    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    /* ERROR_INVALID_PARAM exercises the third branch in mapErrorReason */
    Plugin::LifecycleInterfaceConnector::_instance->OnAppStateChanged(
        APPMANAGER_APP_ID,
        Exchange::ILifecycleManager::LifecycleState::ACTIVE,
        "ERROR_INVALID_PARAM");

    /* Wait for OnAppLifecycleStateChanged to be dispatched before releasing resources */
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLifecycleStateChanged);
    EXPECT_TRUE(signalled & AppManager_onAppLifecycleStateChanged);

    mAppManagerImpl->Unregister(&notification);

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for LICOnAppStateChangedMapErrorReasonUnknown
 * Setting up AppManager Plugin with full resources
 * Pre-populating mAppInfo and calling OnAppStateChanged with an unrecognized errorReason
 * Verifying the mapErrorReason fallback (L958-961) APP_ERROR_UNKNOWN path is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, LICOnAppStateChangedMapErrorReasonUnknown)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    AppInfoManager::getInstance().upsert(APPMANAGER_APP_ID, [](AppInfo& a) {
        a.setAppInstanceId(APPMANAGER_APP_INSTANCE);
        a.setAppNewState(Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE);
    });

    ASSERT_NE(Plugin::LifecycleInterfaceConnector::_instance, nullptr)
        << "LifecycleInterfaceConnector instance is not set";

    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = APPMANAGER_APP_INSTANCE;
    expectedEvent.newState = Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED;
    expectedEvent.oldState = Exchange::IAppManager::AppLifecycleState::APP_STATE_ACTIVE;
    expectedEvent.errorReason = Exchange::IAppManager::AppErrorReason::APP_ERROR_UNKNOWN;

    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    /* Unrecognized error string exercises the else/fallback branch in mapErrorReason */
    Plugin::LifecycleInterfaceConnector::_instance->OnAppStateChanged(
        APPMANAGER_APP_ID,
        Exchange::ILifecycleManager::LifecycleState::ACTIVE,
        "ERROR_UNKNOWN_REASON");

    /* Wait for OnAppLifecycleStateChanged to be dispatched before releasing resources */
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppLifecycleStateChanged);
    EXPECT_TRUE(signalled & AppManager_onAppLifecycleStateChanged);

    mAppManagerImpl->Unregister(&notification);

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for LICRemoveAppInfoByAppIdFound
 * Setting up AppManager Plugin with full resources
 * Pre-populating mAppInfo and calling LifecycleInterfaceConnector::removeAppInfoByAppId
 * Verifying the entry-found removal path (L989-993) in LIC removeAppInfoByAppId is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, LICRemoveAppInfoByAppIdFound)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    /* Pre-populate AppInfoManager so the lookup finds the entry */
    AppInfoManager::getInstance().upsert(APPMANAGER_APP_ID, [](AppInfo& a) {
        a.setAppInstanceId(APPMANAGER_APP_INSTANCE);
    });

    ASSERT_NE(Plugin::LifecycleInterfaceConnector::_instance, nullptr)
        << "LifecycleInterfaceConnector instance is not set";

    /* Directly call the LIC removeAppInfoByAppId to exercise the found path */
    Plugin::LifecycleInterfaceConnector::_instance->removeAppInfoByAppId(APPMANAGER_APP_ID);

    /* Entry should have been erased */
    EXPECT_FALSE(AppInfoManager::getInstance().exists(APPMANAGER_APP_ID));

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for LICRemoveAppInfoByAppIdNotFound
 * Setting up AppManager Plugin with full resources
 * Calling LifecycleInterfaceConnector::removeAppInfoByAppId with an unknown appId
 * Verifying the LOGERR("AppInfo for appId not found") path (L996) is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, LICRemoveAppInfoByAppIdNotFound)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    ASSERT_NE(Plugin::LifecycleInterfaceConnector::_instance, nullptr)
        << "LifecycleInterfaceConnector instance is not set";

    /* mAppInfo is empty - exercises the LOGERR("AppInfo for appId not found") path */
    Plugin::LifecycleInterfaceConnector::_instance->removeAppInfoByAppId(APPMANAGER_APP_INSTANCE);

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for AppManagerImplRemoveAppInfoByAppIdFound
 * Setting up AppManager Plugin with full resources
 * Pre-populating mAppInfo and calling AppManagerImplementation::handleOnAppUnloaded
 * Verifying the removeAppInfoByAppId entry-found path (L729-733) is covered
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, AppManagerImplRemoveAppInfoByAppIdFound)
{
    Core::hresult status;
    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    /* Pre-populate AppInfoManager for the appId */
    AppInfoManager::getInstance().upsert(APPMANAGER_APP_ID, [](AppInfo& a) {
        a.setAppInstanceId(APPMANAGER_APP_INSTANCE);
        a.getPackageInfoMutable().version = APPMANAGER_APP_VERSION;
    });

    EXPECT_CALL(*mPackageManagerMock, Unlock(APPMANAGER_APP_ID, APPMANAGER_APP_VERSION))
        .WillOnce([&](const string& packageId, const string& version) {
            return Core::ERROR_NONE;
        });

    uint32_t signalled = AppManager_StateInvalid;
    ExpectedAppLifecycleEvent expectedEvent;
    expectedEvent.appId = APPMANAGER_APP_ID;
    expectedEvent.appInstanceId = APPMANAGER_APP_INSTANCE;

    Core::Sink<NotificationHandler> notification;
    mAppManagerImpl->Register(&notification);
    notification.SetExpectedEvent(expectedEvent);

    /* Calling handleOnAppUnloaded triggers Dispatch(APP_EVENT_UNLOADED)
     * which calls packageUnLock and then removeAppInfoByAppId(appId) */
    mAppManagerImpl->handleOnAppUnloaded(APPMANAGER_APP_ID, APPMANAGER_APP_INSTANCE);

    /* Wait for OnAppUnloaded to be dispatched before checking results */
    signalled = notification.WaitForRequestStatus(TIMEOUT, AppManager_onAppUnloaded);
    EXPECT_TRUE(signalled & AppManager_onAppUnloaded);

    mAppManagerImpl->Unregister(&notification);

    /* Verify the entry has been removed */
    EXPECT_FALSE(AppInfoManager::getInstance().exists(APPMANAGER_APP_ID));

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

/*
 * Test Case for GetCustomValuesWithAipathFileHasContent
 * Setting up AppManager Plugin with full resources
 * Mocking fopen to return a populated temp file for /tmp/aipath
 * Calling PreloadApp() to trigger getCustomValues with file content (L1597-1631)
 * Releasing the AppManager interface and all related test resources
 */
TEST_F(AppManagerTest, GetCustomValuesWithAipathFileHasContent)
{
    Core::hresult status;
    std::string error = "";

    status = createResources();
    EXPECT_EQ(Core::ERROR_NONE, status);

    /* Configure fopen mock to return a real temp file with 3 lines of content */
    ON_CALL(*p_wrapsImplMock, fopen(::testing::_, ::testing::_))
        .WillByDefault([&](const char* pathname, const char* mode) -> FILE* {
            if (nullptr != pathname && std::string(pathname) == "/tmp/aipath") {
                FILE* tmp = tmpfile();
                if (nullptr != tmp) {
                    fputs("/app/custom/path\n/runtime/custom/path\ncustom-launch-command\n", tmp);
                    rewind(tmp);
                }
                return tmp;
            }
            /* Delegate to real fopen for all other paths to keep the mock narrowly scoped */
            return ::fopen(pathname, mode);
        });

    preLaunchAppPreRequisite(Exchange::ILifecycleManager::LifecycleState::PAUSED);
    EXPECT_EQ(Core::ERROR_NONE, mAppManagerImpl->PreloadApp(APPMANAGER_APP_ID, APPMANAGER_APP_LAUNCHARGS, error));
    {
        std::unique_lock<std::mutex> lock(mPreLoadMutex);
        ASSERT_TRUE(mPreLoadCV.wait_for(lock, std::chrono::seconds(10), [&] { return mPreLoadSpawmCalled; }));
    }

    if (status == Core::ERROR_NONE)
    {
        releaseResources();
    }
}

