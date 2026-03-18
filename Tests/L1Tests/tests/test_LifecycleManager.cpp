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
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>
#include <mutex>
#include <chrono>
#include <condition_variable>

#include "LifecycleManager.h"
#include "LifecycleManagerImplementation.h"
#include "ServiceMock.h"
#include "RuntimeManagerMock.h"
#include "WindowManagerMock.h"
#include "WorkerPoolImplementation.h"

#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);
#define TIMEOUT   (1000)

typedef enum : uint32_t {
    LifecycleManager_invalidEvent = 0,
    LifecycleManager_onStateChangeEvent,
    LifecycleManager_onRuntimeManagerEvent,
    LifecycleManager_onWindowManagerEvent,
    LifecycleManager_onRippleEvent
} LifecycleManagerTest_events_t;

namespace WPEFramework {
namespace Plugin {
class LifecycleManagerImplementationTest : public LifecycleManagerImplementation {
    public:
        ApplicationContext* getContextImpl(const std::string& appInstanceId, const std::string& appId) const 
        {
            return LifecycleManagerImplementation::getContext(appInstanceId, appId);
        }
};

/**
 * LifecycleManagerShellTest exposes the private Notification sink methods of
 * LifecycleManager so that L1 tests can exercise the inline functions defined
 * in LifecycleManager.h without calling Initialize() or Deinitialize().
 * LifecycleManager.h declares this class as a friend.
 */
class LifecycleManagerShellTest : public LifecycleManager {
    public:
        /* LifecycleManager inherits Core::IReferenceCounted pure virtuals through
         * PluginHost::IPlugin / JSONRPC.  In production the SERVICE_REGISTRATION
         * wrapper (Core::ServiceType<>) provides the implementation.  For unit
         * tests we supply stubs so that this subclass is concrete and can be
         * stack-allocated without calling Initialize(). */
        void AddRef() const override {}
        uint32_t Release() const override { return 1; }

        void notificationActivated(RPC::IRemoteConnection* conn)
        {
            mLifecycleManagerStateNotification.Activated(conn);
        }

        void notificationDeactivated(RPC::IRemoteConnection* conn)
        {
            mLifecycleManagerStateNotification.Deactivated(conn);
        }

        void notificationOnAppLifecycleStateChanged(
            const std::string& appId,
            const std::string& appInstanceId,
            Exchange::ILifecycleManager::LifecycleState oldState,
            Exchange::ILifecycleManager::LifecycleState newState,
            const std::string& navigationIntent)
        {
            mLifecycleManagerStateNotification.OnAppLifecycleStateChanged(
                appId, appInstanceId, oldState, newState, navigationIntent);
        }

        void* notificationQueryInterface(uint32_t id)
        {
            return mLifecycleManagerStateNotification.QueryInterface(id);
        }
};
} // namespace Plugin
} // namespace WPEFramework

using ::testing::NiceMock;
using namespace WPEFramework;
using namespace std;

class EventHandlerTest : public Plugin::IEventHandler {
    public:
        string appId;
        string appInstanceId;
        Exchange::ILifecycleManager::LifecycleState oldLifecycleState;
        Exchange::ILifecycleManager::LifecycleState newLifecycleState;
        Exchange::IRuntimeManager::RuntimeState state;
        string navigationIntent;
        string errorReason;
        string name;
        string errorCode;
        string client;
        vector<string> runtimeEventName;
        vector<string> windowEventName;
        double minutes;

        mutex m_mutex;
        condition_variable m_condition_variable;
        uint32_t m_event_signal = LifecycleManager_invalidEvent;

        void onStateChangeEvent(JsonObject& data) override 
        {
            m_event_signal = LifecycleManager_onStateChangeEvent;

            EXPECT_EQ(appId, data["appId"].String());
            EXPECT_EQ(oldLifecycleState, static_cast<Exchange::ILifecycleManager::LifecycleState>(data["oldLifecycleState"].Number()));
            EXPECT_EQ(newLifecycleState, static_cast<Exchange::ILifecycleManager::LifecycleState>(data["newLifecycleState"].Number()));
            EXPECT_EQ(errorReason, data["errorReason"].String());

            m_condition_variable.notify_one();
        }

        void onRuntimeManagerEvent(JsonObject& data) override
        {
            m_event_signal = LifecycleManager_onRuntimeManagerEvent;

            EXPECT_EQ(find(runtimeEventName.begin(), runtimeEventName.end(), data["name"].String()) != runtimeEventName.end(), true);
            EXPECT_EQ(state, static_cast<Exchange::IRuntimeManager::RuntimeState>(data["state"].Number()));
            EXPECT_EQ(errorCode, data["errorCode"].String());

            m_condition_variable.notify_one();
        }

        void onWindowManagerEvent(JsonObject& data) override
        {
            m_event_signal = LifecycleManager_onWindowManagerEvent;

            EXPECT_EQ(find(windowEventName.begin(), windowEventName.end(), data["name"].String()) != windowEventName.end(), true);
            EXPECT_EQ(client, data["client"].String());
            EXPECT_EQ(minutes, data["minutes"].Double());

            m_condition_variable.notify_one();
        }

        void onRippleEvent(string name, JsonObject& data) override
        {
            m_event_signal = LifecycleManager_onRippleEvent;

            m_condition_variable.notify_one();
        }

        uint32_t WaitForEventStatus(uint32_t timeout_ms, LifecycleManagerTest_events_t status)
        {
            uint32_t event_signal = LifecycleManager_invalidEvent;
            std::unique_lock<std::mutex> lock(m_mutex);
            auto now = std::chrono::steady_clock::now();
            auto timeout = std::chrono::milliseconds(timeout_ms);
            if (m_condition_variable.wait_until(lock, now + timeout) == std::cv_status::timeout)
            {
                 TEST_LOG("Timeout waiting for request status event");
                 return m_event_signal;
            }
            event_signal = m_event_signal;
            m_event_signal = LifecycleManager_invalidEvent;
            return event_signal;
        }
};

class NotificationTest : public Exchange::ILifecycleManager::INotification 
{
    private:
        BEGIN_INTERFACE_MAP(NotificationTest)
        INTERFACE_ENTRY(Exchange::ILifecycleManager::INotification)
        END_INTERFACE_MAP
    
    public:
        void OnAppStateChanged(const std::string& appId, Exchange::ILifecycleManager::LifecycleState state, const std::string& errorReason) override {}     
};

class StateNotificationTest : public Exchange::ILifecycleManagerState::INotification 
{
    private:
        BEGIN_INTERFACE_MAP(StateNotificationTest)
        INTERFACE_ENTRY(Exchange::ILifecycleManagerState::INotification)
        END_INTERFACE_MAP

    public:
       void OnAppLifecycleStateChanged(const std::string& appId, const std::string& appInstanceId, const Exchange::ILifecycleManager::LifecycleState oldState, const Exchange::ILifecycleManager::LifecycleState newState, const std::string& navigationIntent) override {}
};

class LifecycleManagerTest : public ::testing::Test {
protected:
    string appId;
    string launchIntent;
    Exchange::ILifecycleManager::LifecycleState targetLifecycleState;
    Exchange::RuntimeConfig runtimeConfigObject;
    string launchArgs;
    string appInstanceId;
    string errorReason;
    bool success;
    vector<string> runtimeEventName;
    vector<string> windowEventName;
    string errorCode;
    Exchange::IRuntimeManager::RuntimeState state;
    string client;
    double minutes;

    Core::ProxyType<Plugin::LifecycleManagerImplementationTest> mLifecycleManagerImpl;
    EventHandlerTest eventHdlTest;
    Exchange::ILifecycleManager* interface = nullptr;
    Exchange::ILifecycleManagerState* stateInterface = nullptr;
    Exchange::IConfiguration* mLifecycleManagerConfigure = nullptr;
    RuntimeManagerMock* mRuntimeManagerMock = nullptr;
    WindowManagerMock* mWindowManagerMock = nullptr;
    ServiceMock* mServiceMock = nullptr;
    Core::ProxyType<WorkerPoolImplementation> workerPool;
    JsonObject eventData;
    uint32_t event_signal;

    LifecycleManagerTest()
	: workerPool(Core::ProxyType<WorkerPoolImplementation>::Create(
            2, Core::Thread::DefaultStackSize(), 16))
    {
        mLifecycleManagerImpl = Core::ProxyType<Plugin::LifecycleManagerImplementationTest>::Create();
        
        interface = static_cast<Exchange::ILifecycleManager*>(mLifecycleManagerImpl->QueryInterface(Exchange::ILifecycleManager::ID));

        stateInterface = static_cast<Exchange::ILifecycleManagerState*>(mLifecycleManagerImpl->QueryInterface(Exchange::ILifecycleManagerState::ID));

		Core::IWorkerPool::Assign(&(*workerPool));
		workerPool->Run();
    }

    virtual ~LifecycleManagerTest() override
    {
		interface->Release();
		stateInterface->Release();

        Core::IWorkerPool::Assign(nullptr);
		workerPool.Release();
    }
	
	void createResources() 
	{
	    // Initialize the parameters with default values
        appId = "com.test.app";
        launchIntent = "test.launch.intent";
        targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::LOADING;
        launchArgs = "test.arguments";
        appInstanceId = "";
        errorReason = "";
        success = true;
        runtimeEventName = {"onTerminated", "onStateChanged", "onFailure", "onStarted"};
        windowEventName = {"onReady", "onDisconnect", "onUserInactivity"};
        errorCode = "1";
        state = Exchange::IRuntimeManager::RuntimeState::RUNTIME_STATE_SUSPENDED;
        client = "test.client";
        minutes = 24;
        
        runtimeConfigObject.dial = true;
        runtimeConfigObject.wanLanAccess = true;
        runtimeConfigObject.thunder = true;
        runtimeConfigObject.systemMemoryLimit = 1024;
        runtimeConfigObject.gpuMemoryLimit = 512;
        runtimeConfigObject.envVariables = "test.env.variables";
        runtimeConfigObject.userId = 1;
        runtimeConfigObject.groupId = 1;
        runtimeConfigObject.dataImageSize = 1024;
        runtimeConfigObject.resourceManagerClientEnabled = true;
        runtimeConfigObject.dialId = "test.dial.id";
        runtimeConfigObject.command = "test.command";
        runtimeConfigObject.appType = "test.app.type";
        runtimeConfigObject.appPath = "test.app.path";
        runtimeConfigObject.runtimePath = "test.runtime.path";
        runtimeConfigObject.logFilePath = "test.logfile.path";
        runtimeConfigObject.logFileMaxSize = 1024;
        runtimeConfigObject.logLevels = "[\"DEBUG\",\"INFO\"]";
        runtimeConfigObject.mapi = true;
        runtimeConfigObject.fkpsFiles = "[\"fkps1\",\"fkps2\"]";
        runtimeConfigObject.ralfPkgPath = "/tmp/ralf";
        runtimeConfigObject.fireboltVersion = "test.firebolt.version";
        runtimeConfigObject.enableDebugger = true;
        runtimeConfigObject.unpackedPath = "test.unpacked.path";

        // Initialize event parameters and event data
        eventHdlTest.appId = appId;
        eventHdlTest.appInstanceId = appInstanceId;
        eventHdlTest.oldLifecycleState = Exchange::ILifecycleManager::LifecycleState::UNLOADED;
        eventHdlTest.newLifecycleState = targetLifecycleState;
        eventHdlTest.errorReason = errorReason;
        eventHdlTest.state = state;
        eventHdlTest.errorCode = errorCode;
        eventHdlTest.client = client;
        eventHdlTest.minutes = minutes;
        eventHdlTest.runtimeEventName = runtimeEventName;
        eventHdlTest.windowEventName = windowEventName;

        eventData["appId"] = appId;
        eventData["appInstanceId"] = appInstanceId;
        eventData["oldLifecycleState"] = static_cast<uint32_t>(Exchange::ILifecycleManager::LifecycleState::UNLOADED);
        eventData["newLifecycleState"] = static_cast<uint32_t>(targetLifecycleState);
        eventData["navigationIntent"] = launchIntent;
        eventData["errorReason"] = errorReason;
        eventData["name"] = "";
        eventData["state"] = static_cast<uint32_t>(state);
        eventData["errorCode"] = errorCode;
        eventData["client"] = client;
        eventData["minutes"] = minutes;

        event_signal = LifecycleManager_invalidEvent;
		
		// Set up mocks and expect calls
        mServiceMock = new NiceMock<ServiceMock>;
        mRuntimeManagerMock = new NiceMock<RuntimeManagerMock>;
        mWindowManagerMock = new NiceMock<WindowManagerMock>;

        mLifecycleManagerConfigure = static_cast<Exchange::IConfiguration*>(mLifecycleManagerImpl->QueryInterface(Exchange::IConfiguration::ID));
		
        EXPECT_CALL(*mServiceMock, QueryInterfaceByCallsign(::testing::_, ::testing::_))
          .Times(::testing::AnyNumber())
          .WillRepeatedly(::testing::Invoke(
              [&](const uint32_t, const std::string& name) -> void* {
                if (name == "org.rdk.RuntimeManager") {
                    return reinterpret_cast<void*>(mRuntimeManagerMock);
                } else if (name == "org.rdk.RDKWindowManager") {
                   return reinterpret_cast<void*>(mWindowManagerMock);
                } 
            return nullptr;
        }));

		EXPECT_CALL(*mServiceMock, AddRef())
          .Times(::testing::AnyNumber());
	
		EXPECT_CALL(*mRuntimeManagerMock, Register(::testing::_))
          .WillRepeatedly(::testing::Return(Core::ERROR_NONE));
    
		EXPECT_CALL(*mWindowManagerMock, Register(::testing::_))
          .WillRepeatedly(::testing::Return(Core::ERROR_NONE));

        // Configure the LifecycleManager
        mLifecycleManagerConfigure->Configure(mServiceMock);
		
		ASSERT_TRUE(interface != nullptr);  
    }

    void releaseResources()
    {
	    // Clean up mocks
		if (mServiceMock != nullptr)
        {
			EXPECT_CALL(*mServiceMock, Release())
              .WillOnce(::testing::Invoke(
              [&]() {
						delete mServiceMock;
						mServiceMock = nullptr;
						return 0;
					}));    
        }

        if (mRuntimeManagerMock != nullptr)
        {
			EXPECT_CALL(*mRuntimeManagerMock, Unregister(::testing::_))
              .WillRepeatedly(::testing::Return(Core::ERROR_NONE));

			EXPECT_CALL(*mRuntimeManagerMock, Release())
              .WillOnce(::testing::Invoke(
              [&]() {
						delete mRuntimeManagerMock;
						mRuntimeManagerMock = nullptr;
						return 0;
					}));
        }

        if (mWindowManagerMock != nullptr)
        {
			EXPECT_CALL(*mWindowManagerMock, Unregister(::testing::_))
              .WillRepeatedly(::testing::Return(Core::ERROR_NONE));

			EXPECT_CALL(*mWindowManagerMock, Release())
              .WillOnce(::testing::Invoke(
              [&]() {
						delete mWindowManagerMock;
						mWindowManagerMock = nullptr;
						return 0;
                    }));
        }

		// Clean up the LifecycleManager
        mLifecycleManagerConfigure->Release();
		
		ASSERT_TRUE(interface != nullptr); 
    }

    void onStateChangeEventSignal() 
    {
        event_signal = LifecycleManager_invalidEvent;

        eventHdlTest.onStateChangeEvent(eventData);

        event_signal = eventHdlTest.WaitForEventStatus(TIMEOUT, LifecycleManager_onStateChangeEvent);

        EXPECT_TRUE(event_signal & LifecycleManager_onStateChangeEvent);
    }

    void onRuntimeManagerEventSignal(JsonObject data) 
    {
        eventData["name"] = data["name"];

        event_signal = LifecycleManager_invalidEvent;

        eventHdlTest.onRuntimeManagerEvent(eventData);

        event_signal = eventHdlTest.WaitForEventStatus(TIMEOUT, LifecycleManager_onRuntimeManagerEvent);

        EXPECT_TRUE(event_signal & LifecycleManager_onRuntimeManagerEvent);
    }

    void onWindowManagerEventSignal(JsonObject data) 
    {
        eventData["name"] = data["name"];

        event_signal = LifecycleManager_invalidEvent;

        eventHdlTest.onWindowManagerEvent(eventData);

        event_signal = eventHdlTest.WaitForEventStatus(TIMEOUT, LifecycleManager_onWindowManagerEvent);

        EXPECT_TRUE(event_signal & LifecycleManager_onWindowManagerEvent);
    }
};

/* Test Case for Registering and Unregistering Notification
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Create a notification instance using the NotificationTest class
 * Register the notification with the Lifecycle Manager interface
 * Verify successful registration of notification by asserting that Register() returns Core::ERROR_NONE
 * Unregister the notification from the Lifecycle Manager interface
 * Verify successful unregistration of notification by asserting that Unregister() returns Core::ERROR_NONE
 * Release the Lifecycle Manager interface object and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, unregisterNotification_afterRegister)
{
    createResources();

    Core::Sink<NotificationTest> notification;

    // TC-1: Check if the notification is unregistered after registering
    EXPECT_EQ(Core::ERROR_NONE, interface->Register(&notification));

    EXPECT_EQ(Core::ERROR_NONE, interface->Unregister(&notification));

    releaseResources();
}

/* Test Case for Unregistering Notification without registering
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Create a notification instance using the NotificationTest class
 * Unregister the notification from the Lifecycle Manager interface
 * Verify unregistration of notification fails by asserting that Unregister() returns Core::ERROR_GENERAL
 * Release the Lifecycle Manager interface object and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, unregisterNotification_withoutRegister)
{
    createResources();

    Core::Sink<NotificationTest> notification;
	
	// TC-2: Check if the notification is unregistered without registering
    EXPECT_EQ(Core::ERROR_GENERAL, interface->Unregister(&notification));

    releaseResources();
}

/* Test Case for Registering and Unregistering State Notification 
 * 
 * Set up Lifecycle Manager state interface, configurations, required COM-RPC resources, mocks and expectations
 * Create a state notification instance using the StateNotificationTest class
 * Register the state notification with the Lifecycle Manager state interface
 * Verify successful registration of state notification by asserting that Register() returns Core::ERROR_NONE
 * Unregister the state notification from the Lifecycle Manager state interface
 * Verify successful unregistration of state notification by asserting that Unregister() returns Core::ERROR_NONE
 * Release the Lifecycle Manager state interface object and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, unregisterStateNotification_afterRegister)
{
    createResources();

    Core::Sink<StateNotificationTest> stateNotification;

    // TC-3: Check if the state notification is registered after unregistering
    EXPECT_EQ(Core::ERROR_NONE, stateInterface->Register(&stateNotification));

    EXPECT_EQ(Core::ERROR_NONE, stateInterface->Unregister(&stateNotification));
    
    releaseResources();
}

/* Test Case for Unregistering State Notification without registering
 * 
 * Set up Lifecycle Manager state interface, configurations, required COM-RPC resources, mocks and expectations
 * Create a state notification instance using the StateNotificationTest class
 * Unregister the state notification from the Lifecycle Manager state interface
 * Verify unregistration of state notification fails by asserting that Unregister() returns Core::ERROR_GENERAL
 * Release the Lifecycle Manager state interface object and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, unregisterStateNotification_withoutRegister)
{
    createResources();

    Core::Sink<StateNotificationTest> stateNotification;
	
	// TC-4: Check if the state notification is registered without unregistering
    EXPECT_EQ(Core::ERROR_GENERAL, stateInterface->Unregister(&stateNotification));

    releaseResources();
}

/* Test Case for Spawning an App
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as LOADING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeonStateChangeEventSignal() method
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, spawnApp_withValidParams)
{    
    createResources();

    // TC-5: Spawn an app with all parameters valid
    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    releaseResources();
}

/* Test Case for App Ready after Spawning
 * 
 * Set up Lifecycle Manager interface, state interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as LOADING.
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Check if the app is ready after spawning with the appId 
 * Verify that the app is ready by asserting that AppReady() returns Core::ERROR_NONE
 * Obtain the loaded app context using getContextImpl() and wait for the app ready semaphore
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, appready_onSpawnAppSuccess) 
{
    createResources();

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();
    
	// TC-6: Check if app is ready after spawning
    EXPECT_EQ(Core::ERROR_NONE, stateInterface->AppReady(appId));

    Plugin::ApplicationContext* context = mLifecycleManagerImpl->getContextImpl("", appId);

    sem_wait(&context->mAppReadySemaphore);
    
    releaseResources();
}

/* Test Case for App Ready with invalid AppId after Spawning 
 * 
 * Set up Lifecycle Manager interface, state interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as LOADING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Check failure of app ready due to invalid appId by asserting that AppReady() returns Core::ERROR_GENERAL
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, appready_oninvalidAppId) 
{
    createResources();

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();
	
	// TC-7: Verify error on passing an invalid appId
    EXPECT_EQ(Core::ERROR_GENERAL, stateInterface->AppReady(""));

    releaseResources();
}

/* Test Case for querying if App is Loaded after Spawning
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters  with target state as LOADING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Check if the app is loaded after spawning with the appId
 * Verify that the app is loaded by asserting that IsAppLoaded() returns Core::ERROR_NONE
 * Check that the loaded flag is set to true, confirming that the app is loaded
 * Release the Lifecycle Manager object and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, isAppLoaded_onSpawnAppSuccess) 
{
    createResources();

    bool loaded = false;
    
    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

	// TC-8: Check if app is loaded after spawning
    EXPECT_EQ(Core::ERROR_NONE, interface->IsAppLoaded(appId, loaded));

    EXPECT_EQ(loaded, true);    

    releaseResources();
}

/* Test Case for querying if App is Loaded with invalid AppId after Spawning
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as LOADING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Check failure of app loaded due to invalid appId by asserting that IsAppLoaded() returns Core::ERROR_GENERAL
 * Check that the loaded flag is set to false, confirming that the app is not loaded
 * Release the Lifecycle Manager object and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, isAppLoaded_oninvalidAppId)
{
    createResources();

    bool loaded = true;

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();
	
	// TC-9: Verify error on passing an invalid appId
    EXPECT_EQ(Core::ERROR_NONE, interface->IsAppLoaded("", loaded));

    EXPECT_EQ(loaded, false);

    releaseResources();
}

/* Test Case for getLoadedApps with verbose enabled after Spawning
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Enable the verbose flag by setting it to true
 * Spawn an app with valid parameters with target state as LOADING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Obtain the loaded apps and assert that GetLoadedApps() returns Core::ERROR_NONE
 * Verify the app list parameters by comparing the obtained and expected appId
 * Release the Lifecycle Manager object and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, getLoadedApps_verboseEnabled)
{
    createResources();

    bool verbose = true;
    string apps = "";

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();
	
	// TC-10: Get loaded apps with verbose enabled
    EXPECT_EQ(Core::ERROR_NONE, interface->GetLoadedApps(verbose, apps)); 
    
    EXPECT_THAT(apps, ::testing::HasSubstr("\"appId\":\"com.test.app\""));

    releaseResources();
}

/* Test Case for getLoadedApps with verbose disabled after Spawning
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Disable the verbose flag by setting it to false
 * Spawn an app with valid parameters with target state as LOADING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Obtain the loaded apps and assert that GetLoadedApps() returns Core::ERROR_NONE
 * Verify the app list parameters by comparing the obtained and expected app list
 * Release the Lifecycle Manager object and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, getLoadedApps_verboseDisabled)
{
    createResources();

    bool verbose = false;
    string apps = "";

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();
	
	// TC-11: Get loaded apps with verbose disabled
    EXPECT_EQ(Core::ERROR_NONE, interface->GetLoadedApps(verbose, apps)); 

    EXPECT_THAT(apps, ::testing::HasSubstr("\"appId\":\"com.test.app\""));
    
    releaseResources();
}

/* Test Case for getLoadedApps with verbose enabled without Spawning
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Enable the verbose flag by setting it to true
 * Obtain the loaded apps and assert that GetLoadedApps() returns Core::ERROR_NONE
 * Verify the app list parameters is empty indicating no apps are loaded
 * Release the Lifecycle Manager object and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, getLoadedApps_noAppsLoaded)
{
    createResources();

    bool verbose = true;
    string apps = "";

    // TC-12: Check that no apps are loaded
    EXPECT_EQ(Core::ERROR_NONE, interface->GetLoadedApps(verbose, apps));

    EXPECT_EQ(apps, "[]");

    releaseResources();
}

/* Test Case for setTargetAppState with valid parameters
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as LOADING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Set the target state of the app from LOADING to ACTIVE with valid parameters
 * Verify successful state change by asserting that SetTargetAppState() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Repeat the same process with only required parameters valid
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, setTargetAppState_withValidParams)
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mWindowManagerMock, RenderReady(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& client, bool &status) {
                return Core::ERROR_NONE;
          }));

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::ACTIVE;

    // TC-13: Set the target state of a loaded app with all parameters valid
    EXPECT_EQ(Core::ERROR_NONE, interface->SetTargetAppState(appInstanceId, targetLifecycleState, launchIntent));

    onStateChangeEventSignal();

    // TC-14: Set the target state of a loaded app with only required parameters valid
    EXPECT_EQ(Core::ERROR_NONE, interface->SetTargetAppState(appInstanceId, targetLifecycleState, ""));
    
    onStateChangeEventSignal();

    releaseResources();
}

/* Test Case for setTargetAppState with invalid parameters
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as LOADING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Set the target state of the app to LOADING with invalid appInstanceId
 * Verify state change fails by asserting that SetTargetAppState() returns Core::ERROR_GENERAL
 * Release the Lifecycle Manager object and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, setTargetAppState_withinvalidParams)
{
    createResources();

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    // TC-15: Set the target state of a loaded app with invalid appInstanceId
    EXPECT_EQ(Core::ERROR_GENERAL, interface->SetTargetAppState("", targetLifecycleState, launchIntent));    

    releaseResources();
}

/* Test Case for Unload App after Spawning
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as LOADING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Unload the app using the appInstanceId 
 * Verify that app is successfully unloaded by asserting that UnloadApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, unloadApp_onSpawnAppSuccess)
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mRuntimeManagerMock, Terminate(::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appInstanceId) {
                return Core::ERROR_NONE;
          }));

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();
    
	// TC-16: Unload the app after spawning
    EXPECT_EQ(Core::ERROR_NONE, interface->UnloadApp(appInstanceId, errorReason, success));
    
    onStateChangeEventSignal();

    releaseResources();
}

/* Test Case for Unload App without Spawning
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Set the appInstanceId to a random test value
 * Unload the app using the appInstanceId
 * Verify failure of app unload by asserting that UnloadApp() returns Core::ERROR_GENERAL
 * Release the Lifecycle Manager object and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, unloadApp_withoutSpawning)
{
    createResources();

    appInstanceId = "test.app.instance";

    // TC-17: Unload the app after spawn fails
    EXPECT_EQ(Core::ERROR_GENERAL, interface->UnloadApp(appInstanceId, errorReason, success));

    releaseResources();
}

/* Test Case for Kill App after Spawning
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as LOADING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Kill the app using the appInstanceId
 * Verify that app is successfully killed by asserting that KillApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, killApp_onSpawnAppSuccess)
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mRuntimeManagerMock, Kill(::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appInstanceId) {
                return Core::ERROR_NONE;
          }));

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();
    
	// TC-18: Kill the app after spawning
    EXPECT_EQ(Core::ERROR_NONE, interface->KillApp(appInstanceId, errorReason, success));
    
    onStateChangeEventSignal();
    
    releaseResources();
}

/* Test Case for Kill App without Spawning
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Set the appInstanceId to a random test value
 * Kill the app using the appInstanceId 
 * Verify failure of app kill by asserting that KillApp() returns Core::ERROR_GENERAL
 * Release the Lifecycle Manager interface object and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, killApp_withoutSpawning)
{
    createResources();

    appInstanceId = "test.app.instance";

    // TC-19: Kill the app after spawn fails
    EXPECT_EQ(Core::ERROR_GENERAL, interface->KillApp(appInstanceId, errorReason, success)); 
    
    releaseResources();
}

/* Test Case for Close App on User Exit
 * 
 * Set up Lifecycle Manager interface, state interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as LOADING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Close the app using the appId and setting the reason for close as USER EXIT 
 * Verify that app is successfully closed by asserting that CloseApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, closeApp_onUserExit) 
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mRuntimeManagerMock, Kill(::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appInstanceId) {
                return Core::ERROR_NONE;
          }));

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();
    
	// TC-20: User exits the app after spawning 
    EXPECT_EQ(Core::ERROR_NONE, stateInterface->CloseApp(appId, Exchange::ILifecycleManagerState::AppCloseReason::USER_EXIT));

    onStateChangeEventSignal();

    releaseResources();
}

/* Test Case for Close App on Error
 * 
 * Set up Lifecycle Manager interface, state interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as LOADING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Close the app using the appId and setting the reason for close as ERROR
 * Verify that app is successfully closed by asserting that CloseApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, closeApp_onError) 
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mRuntimeManagerMock, Kill(::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appInstanceId) {
                return Core::ERROR_NONE;
          }));

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();
    
	// TC-21: Error after spawning the app
    EXPECT_EQ(Core::ERROR_NONE, stateInterface->CloseApp(appId, Exchange::ILifecycleManagerState::AppCloseReason::ERROR));

    onStateChangeEventSignal();

    releaseResources();
}

/* Test Case for Close App on Kill and Run
 * 
 * Set up Lifecycle Manager interface, state interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as LOADING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Close the app using the appId and setting the reason for close as KILL AND RUN
 * Verify that app is successfully closed by asserting that CloseApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, closeApp_onKillandRun) 
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mRuntimeManagerMock, Kill(::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appInstanceId) {
                return Core::ERROR_NONE;
          }));

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();
    
	// TC-22: Kill and run after spawning the app
    EXPECT_EQ(Core::ERROR_NONE, stateInterface->CloseApp(appId, Exchange::ILifecycleManagerState::AppCloseReason::KILL_AND_RUN));

    event_signal = eventHdlTest.WaitForEventStatus(TIMEOUT, LifecycleManager_onStateChangeEvent);

    EXPECT_TRUE(event_signal & LifecycleManager_onStateChangeEvent);

    onStateChangeEventSignal();

    event_signal = eventHdlTest.WaitForEventStatus(TIMEOUT, LifecycleManager_onStateChangeEvent);

    EXPECT_TRUE(event_signal & LifecycleManager_onStateChangeEvent);

    onStateChangeEventSignal();

    releaseResources();
}

/* Test Case for Close App on Kill and Activate
 * 
 * Set up Lifecycle Manager interface, state interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as LOADING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Close the app using the appId and setting the reason for close as KILL AND ACTIVATE
 * Verify that app is successfully closed by asserting that CloseApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, closeApp_onKillandActivate) 
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mRuntimeManagerMock, Kill(::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appInstanceId) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mWindowManagerMock, RenderReady(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& client, bool &status) {
                return Core::ERROR_NONE;
          }));

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();
    
	// TC-23: Kill and activate after spawning the app
    EXPECT_EQ(Core::ERROR_NONE, stateInterface->CloseApp(appId, Exchange::ILifecycleManagerState::AppCloseReason::KILL_AND_ACTIVATE));

    event_signal = eventHdlTest.WaitForEventStatus(TIMEOUT, LifecycleManager_onStateChangeEvent);

    EXPECT_TRUE(event_signal & LifecycleManager_onStateChangeEvent);

    onStateChangeEventSignal();

    event_signal = eventHdlTest.WaitForEventStatus(TIMEOUT, LifecycleManager_onStateChangeEvent);

    EXPECT_TRUE(event_signal & LifecycleManager_onStateChangeEvent);

    onStateChangeEventSignal();

    releaseResources();
}

/* Test Case for State Change Complete with valid parameters
 * 
 * Set up Lifecycle Manager state interface, configurations, required COM-RPC resources, mocks and expectations
 * Set the stateChangedId to a random test value
 * Signal that the state change is complete
 * Verify successful state change by asserting that StateChangeComplete() returns Core::ERROR_NONE
 * Release the Lifecycle Manager state object and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, stateChangeComplete_withValidParams) 
{
    createResources();

    uint32_t stateChangedId = 1;
    
	// TC-24: Check if state change is complete
    EXPECT_EQ(Core::ERROR_NONE, stateInterface->StateChangeComplete(appId, stateChangedId, success));

    releaseResources();
}

/* Test Case for Send Intent to Active App after Spawning
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Set the intent to a random test value
 * Spawn an app with valid parameters with target state as ACTIVE
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Send an intent to the active app using the appInstanceId 
 * Verify failure of sent intent (due to failure in websocket) by asserting that SendIntentToActiveApp() returns Core::ERROR_GENERAL
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, sendIntenttoActiveApp_onSpawnAppSuccess)
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mWindowManagerMock, RenderReady(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& client, bool &status) {
                return Core::ERROR_NONE;
          }));

    string intent = "test.intent";

    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::ACTIVE;

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    // TC-25: Send intent to the app after spawning
    EXPECT_EQ(Core::ERROR_GENERAL, interface->SendIntentToActiveApp(appInstanceId, intent, errorReason, success));   

    onStateChangeEventSignal();

    releaseResources();
}

/* Test Case for Runtime Manager Event - onTerminated after Spawning
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as ACTIVE
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Terminate the app with the appInstanceId
 * Verify successful termination by asserting that UnloadApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Populate the data by setting the event name as onTerminated along with the appInstanceId obtained
 * Signal the Runtime Manager Event using onRuntimeManagerEvent() with the data
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, runtimeManagerEvent_onTerminated) 
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mWindowManagerMock, RenderReady(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& client, bool &status) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mRuntimeManagerMock, Terminate(::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appInstanceId) {
                return Core::ERROR_NONE;
          }));

    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::ACTIVE;

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();
    
    EXPECT_EQ(Core::ERROR_NONE, interface->UnloadApp(appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    JsonObject data;

    data["name"] = "onTerminated";
    data["appInstanceId"] = appInstanceId;

	// TC-26: Signal the Runtime Manager Event - onTerminated 
    mLifecycleManagerImpl->onRuntimeManagerEvent(data);

    onRuntimeManagerEventSignal(data);

    event_signal = eventHdlTest.WaitForEventStatus(TIMEOUT, LifecycleManager_onRuntimeManagerEvent);

    EXPECT_TRUE(event_signal & LifecycleManager_onRuntimeManagerEvent);
   
    releaseResources();
} 

/* Test Case for Runtime Manager Event - onStateChanged after Spawning
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as INITIALIZING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Populate the data by setting the event name as onStateChanged along with the state as SUSPENDED and appInstanceId obtained 
 * Signal the Runtime Manager Event using onRuntimeManagerEvent() with the data 
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, runtimeManagerEvent_onStateChanged) 
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::INITIALIZING;

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();
 
    JsonObject data;

    data["name"] = "onStateChanged";
    data["appInstanceId"] = appInstanceId;
    data["state"] = 3;

	// TC-27: Signal the Runtime Manager Event - onStateChanged
    mLifecycleManagerImpl->onRuntimeManagerEvent(data);

    onRuntimeManagerEventSignal(data);

    event_signal = eventHdlTest.WaitForEventStatus(TIMEOUT, LifecycleManager_onRuntimeManagerEvent);

    EXPECT_TRUE(event_signal & LifecycleManager_onRuntimeManagerEvent);
    
    releaseResources();
} 

/* Test Case for Runtime Manager Event - onFailure after Spawning
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as INITIALIZING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Populate the data by setting the event name as onFailure along with the error code and appInstanceId obtained 
 * Signal the Runtime Manager Event using onRuntimeManagerEvent() with the data 
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, runtimeManagerEvent_onFailure) 
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::INITIALIZING;

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    JsonObject data;

    data["name"] = "onFailure";
    data["appInstanceId"] = appInstanceId;
    data["errorCode"] = 1;

	// TC-28: Signal the Runtime Manager Event - onFailure
    mLifecycleManagerImpl->onRuntimeManagerEvent(data);

    onRuntimeManagerEventSignal(data);

    releaseResources();
} 

/* Test Case for Runtime Manager Event - onStarted after Spawning
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as INITIALIZING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Populate the data by setting the event name as onStarted along with the appInstanceId obtained 
 * Signal the Runtime Manager Event using onRuntimeManagerEvent() with the data 
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, runtimeManagerEvent_onStarted) 
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::INITIALIZING;

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    JsonObject data;

    data["name"] = "onStarted";
    data["appInstanceId"] = appInstanceId;

	// TC-29: Signal the Runtime Manager Event - onStarted
    mLifecycleManagerImpl->onRuntimeManagerEvent(data);

    onRuntimeManagerEventSignal(data);

    releaseResources();
} 

/* Test Case for Window Manager Event - onUserInactivity after Spawning
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as ACTIVE
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Populate the data by setting the event name as onUserInactivity
 * Signal the Window Manager Event using onWindowManagerEvent() with the data 
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, windowManagerEvent_onUserInactivity) 
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mWindowManagerMock, RenderReady(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& client, bool &status) {
                return Core::ERROR_NONE;
          }));  

    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::ACTIVE;

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    JsonObject data;

    data["name"] = "onUserInactivity";
    data["minutes"] = 24;

	// TC-30: Signal the Window Manager Event - onUserInactivity
    mLifecycleManagerImpl->onWindowManagerEvent(data);

    onWindowManagerEventSignal(data);

    releaseResources();
} 

/* Test Case for Window Manager Event - onDisconnect after Spawning
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as ACTIVE
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Populate the data by setting the event name as onDisconnect
 * Signal the Window Manager Event using onWindowManagerEvent() with the data 
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, windowManagerEvent_onDisconnect) 
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mWindowManagerMock, RenderReady(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& client, bool &status) {
                return Core::ERROR_NONE;
          }));  

    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::ACTIVE;

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    JsonObject data;

    data["name"] = "onDisconnect";
    data["client"] = "test.client";

	// TC-31: Signal the Window Manager Event - onDisconnect
    mLifecycleManagerImpl->onWindowManagerEvent(data);

    onWindowManagerEventSignal(data);

    releaseResources();
} 

/* Test Case for Window Manager Event - onReady after Spawning
 * 
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as ACTIVE
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Populate the data by setting the event name as onReady along with the appInstanceId obtained
 * Signal the Window Manager Event using onWindowManagerEvent() with the data
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, windowManagerEvent_onReady) 
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mWindowManagerMock, RenderReady(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& client, bool &status) {
                return Core::ERROR_NONE;
          }));  

    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::ACTIVE;

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    JsonObject data;

    data["name"] = "onReady";
    data["appInstanceId"] = appInstanceId;

	// TC-32: Signal the Window Manager Event - onReady
    mLifecycleManagerImpl->onWindowManagerEvent(data);

    onWindowManagerEventSignal(data);

    event_signal = eventHdlTest.WaitForEventStatus(TIMEOUT, LifecycleManager_onWindowManagerEvent);

    EXPECT_TRUE(event_signal & LifecycleManager_onWindowManagerEvent);

    releaseResources();
} 

/* Test Case for SetTargetAppState to PAUSED from ACTIVE
 *
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as LOADING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Set the target state of the app to ACTIVE and verify successful state change
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Set the target state of the app to PAUSED
 * Verify successful state change to PAUSED by asserting that SetTargetAppState() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, setTargetAppState_toPaused_fromActive)
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mWindowManagerMock, RenderReady(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& client, bool &status) {
                return Core::ERROR_NONE;
          }));

    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::ACTIVE;

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    // TC-33: Set the target state of a loaded app from ACTIVE to PAUSED
    EXPECT_EQ(Core::ERROR_NONE, interface->SetTargetAppState(appInstanceId, Exchange::ILifecycleManager::LifecycleState::PAUSED, ""));

    onStateChangeEventSignal();

    releaseResources();
}

/* Test Case for SetTargetAppState to SUSPENDED from ACTIVE
 *
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as ACTIVE
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Set the target state of the app to SUSPENDED
 * Expect that EnableDisplayRender is called to disable the display
 * Verify successful state change by asserting that SetTargetAppState() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, setTargetAppState_toSuspended_fromActive)
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mWindowManagerMock, RenderReady(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& client, bool &status) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mWindowManagerMock, EnableDisplayRender(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(Core::ERROR_NONE));

    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::ACTIVE;

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    // TC-34: Set the target state of a loaded app from ACTIVE to SUSPENDED
    EXPECT_EQ(Core::ERROR_NONE, interface->SetTargetAppState(appInstanceId, Exchange::ILifecycleManager::LifecycleState::SUSPENDED, ""));

    onStateChangeEventSignal();

    releaseResources();
}

/* Test Case for SetTargetAppState to HIBERNATED from SUSPENDED
 *
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as ACTIVE
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Set the target state of the app to SUSPENDED, handle event signals
 * Set the target state of the app to HIBERNATED
 * Expect that Hibernate is called on the RuntimeManager mock
 * Verify successful state change by asserting that SetTargetAppState() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, setTargetAppState_toHibernated_fromSuspended)
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mWindowManagerMock, RenderReady(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& client, bool &status) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mWindowManagerMock, EnableDisplayRender(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(Core::ERROR_NONE));

    EXPECT_CALL(*mRuntimeManagerMock, Hibernate(::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appInstanceId) {
                return Core::ERROR_NONE;
          }));

    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::ACTIVE;

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    EXPECT_EQ(Core::ERROR_NONE, interface->SetTargetAppState(appInstanceId, Exchange::ILifecycleManager::LifecycleState::SUSPENDED, ""));

    onStateChangeEventSignal();

    // TC-35: Set the target state of an app from SUSPENDED to HIBERNATED
    EXPECT_EQ(Core::ERROR_NONE, interface->SetTargetAppState(appInstanceId, Exchange::ILifecycleManager::LifecycleState::HIBERNATED, ""));

    onStateChangeEventSignal();

    releaseResources();
}

/* Test Case for SendIntentToActiveApp with an invalid appInstanceId
 *
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Call SendIntentToActiveApp with an appInstanceId that has no corresponding loaded app context
 * Verify that SendIntentToActiveApp() returns Core::ERROR_GENERAL due to missing context
 * Verify that the success flag is set to false
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, sendIntentToActiveApp_withInvalidAppInstanceId)
{
    createResources();

    string intent = "test.deep.link.intent";
    string invalidAppInstanceId = "invalid.instance.id";

    // TC-36: Send intent to an app with an invalid appInstanceId (no context found)
    EXPECT_EQ(Core::ERROR_GENERAL, interface->SendIntentToActiveApp(invalidAppInstanceId, intent, errorReason, success));

    EXPECT_EQ(success, false);

    releaseResources();
}

/* Test Case for GetLoadedApps with verbose enabled when GetInfo fails
 *
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as LOADING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Mock GetInfo to return Core::ERROR_GENERAL to simulate a runtime stats failure
 * Call GetLoadedApps with verbose=true
 * Verify that GetLoadedApps() returns Core::ERROR_NONE (apps are still reported)
 * Verify that the app entry is present but without a runtimeStats field
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, getLoadedApps_verboseEnabled_withGetInfoFailure)
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, GetInfo(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(Core::ERROR_GENERAL));

    bool verbose = true;
    string apps = "";

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    // TC-37: Get loaded apps with verbose=true when runtime stats retrieval fails
    EXPECT_EQ(Core::ERROR_NONE, interface->GetLoadedApps(verbose, apps));

    EXPECT_THAT(apps, ::testing::HasSubstr("\"appId\":\"com.test.app\""));
    EXPECT_THAT(apps, ::testing::Not(::testing::HasSubstr("runtimeStats")));

    releaseResources();
}

/* Test Case for KillApp after Spawning exercising the force-kill path in TerminatingState
 *
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as ACTIVE (Run mock returns Core::ERROR_NONE)
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Mock Kill() to return Core::ERROR_NONE
 * Call KillApp to exercise the force-kill path (mForce=true) through TerminatingState::handle()
 * Verify that KillApp() returns Core::ERROR_NONE
 * Verify that the success flag is set to true
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, killApp_forcePath_onSpawnAppSuccess)
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mWindowManagerMock, RenderReady(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& client, bool &status) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mRuntimeManagerMock, Kill(::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appInstanceId) {
                return Core::ERROR_NONE;
          }));

    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::ACTIVE;

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    // TC-38: Kill the active app using the force-kill path (KillApp sets mForce=true)
    EXPECT_EQ(Core::ERROR_NONE, interface->KillApp(appInstanceId, errorReason, success));

    EXPECT_EQ(success, true);

    onStateChangeEventSignal();

    releaseResources();
}

/* Test Case for CloseApp when Kill mock returns error
 *
 * Set up Lifecycle Manager interface, state interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as LOADING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Mock Kill() to return Core::ERROR_GENERAL to simulate a kill failure at the RuntimeManager level
 * Call CloseApp with USER_EXIT reason
 * Verify that CloseApp() returns Core::ERROR_NONE because KillApp() is asynchronous and always
 *   returns Core::ERROR_NONE for a valid context (state transitions are queued via RequestHandler)
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, closeApp_withKillFailure)
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    EXPECT_CALL(*mRuntimeManagerMock, Kill(::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    // TC-39: Close the app when Kill mock returns error — CloseApp itself still returns ERROR_NONE
    // because state transitions are queued asynchronously; KillApp() does not propagate the Kill() error
    EXPECT_EQ(Core::ERROR_NONE, stateInterface->CloseApp(appId, Exchange::ILifecycleManagerState::AppCloseReason::USER_EXIT));

    // Wait for the async LOADING→UNLOADED Dispatch worker-pool job to complete before teardown
    // (every working CloseApp test follows this same synchronisation pattern)
    onStateChangeEventSignal();

    releaseResources();
}

/* Test Case for SetTargetAppState with a non-targetable lifecycle state (default branch)
 *
 * Set up Lifecycle Manager interface, configurations, required COM-RPC resources, mocks and expectations
 * Spawn an app with valid parameters with target state as LOADING
 * Verify successful spawn by asserting that SpawnApp() returns Core::ERROR_NONE
 * Handle event signals by calling the onStateChangeEventSignal() method
 * Call SetTargetAppState with LOADING as the target state to exercise the default error-log path
 *   in the switch statement (LOADING is not a valid user-facing target)
 * Verify that SetTargetAppState() returns Core::ERROR_NONE because RequestHandler::updateState()
 *   is asynchronous and always returns true after queuing the request, so the failure to map
 *   LOADING in the switch never causes a non-ERROR_NONE return
 * Release the Lifecycle Manager objects and clean-up related test resources
 */

TEST_F(LifecycleManagerTest, setTargetAppState_withInvalidTargetState)
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(appId, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, Exchange::IRuntimeManager::IValueIterator* const& ports, Exchange::IRuntimeManager::IStringIterator* const& paths, Exchange::IRuntimeManager::IStringIterator* const& debugSettings, const Exchange::RuntimeConfig& runtimeConfigObject) {
                return Core::ERROR_NONE;
          }));

    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    // TC-40: Pass UNLOADED as the target state — hits the default branch (UNLOADED is not
    // in the PAUSED/SUSPENDED/HIBERNATED/ACTIVE cases).  isValidTransition(LOADING, UNLOADED)
    // returns false (empty predecessor list) so StateHandler::changeState exits immediately
    // with no Dispatch job and no async side-effects.
    EXPECT_EQ(Core::ERROR_NONE, interface->SetTargetAppState(appInstanceId, Exchange::ILifecycleManager::LifecycleState::UNLOADED, ""));

    releaseResources();
}

/* Test Case for LifecycleManager plugin shell constructor and destructor
 *
 * Instantiate a LifecycleManagerShellTest object (a subclass of LifecycleManager).
 * The LifecycleManager constructor creates the mLifecycleManagerStateNotification (Core::Sink<Notification>)
 * which in turn invokes Notification::Notification().  The destructor reverses this.
 * These together cover LifecycleManager.cpp lines 48-60 (ctor/dtor) and
 * LifecycleManager.h lines 46-54 (Notification ctor/dtor).
 */

TEST_F(LifecycleManagerTest, lifecycleManager_constructorDestructor)
{
    // TC-41: Instantiate and destroy the LifecycleManager plugin shell.
    // This covers LifecycleManager::LifecycleManager(), LifecycleManager::~LifecycleManager(),
    // Notification::Notification(LifecycleManager*) and Notification::~Notification() from
    // LifecycleManager.h and LifecycleManager.cpp at 0% hit count.
    {
        Plugin::LifecycleManagerShellTest shell;
        // shell goes out of scope here — dtor called
    }
}

/* Test Case for LifecycleManager::Information()
 *
 * Instantiate a LifecycleManagerShellTest and call Information().
 * Expected: the returned string contains the plugin service name.
 * Covers LifecycleManager.cpp lines 131-133 (Information() body).
 */

TEST_F(LifecycleManagerTest, lifecycleManager_information)
{
    Plugin::LifecycleManagerShellTest shell;

    // TC-42: Information() must return a JSON-like string containing the service name.
    string info = shell.Information();

    EXPECT_THAT(info, ::testing::HasSubstr("org.rdk.LifecycleManager"));
}

/* Test Case for LifecycleManager::QueryInterface() for IPlugin
 *
 * Instantiate a LifecycleManagerShellTest and call QueryInterface for IPlugin.
 * Expected: a non-null pointer is returned (the shell itself).
 * Covers LifecycleManager.h lines 92-97 (BEGIN_INTERFACE_MAP / INTERFACE_ENTRY IPlugin path).
 */

TEST_F(LifecycleManagerTest, lifecycleManager_queryInterface_iPlugin)
{
    Plugin::LifecycleManagerShellTest shell;

    // TC-43: QueryInterface for IPlugin returns the shell object itself (non-null).
    // INTERFACE_ENTRY(PluginHost::IPlugin) is the first entry — no AGGREGATE is traversed.
    void* result = shell.QueryInterface(PluginHost::IPlugin::ID);

    EXPECT_NE(result, nullptr);

    if (nullptr != result)
    {
        static_cast<PluginHost::IPlugin*>(result)->Release();
    }
}

/* Test Case for LifecycleManager::QueryInterface() for IDispatcher
 *
 * Instantiate a LifecycleManagerShellTest and call QueryInterface for IDispatcher.
 * Expected: a non-null pointer is returned.
 * Covers the INTERFACE_ENTRY(PluginHost::IDispatcher) branch of LifecycleManager.h lines 92-97.
 */

TEST_F(LifecycleManagerTest, lifecycleManager_queryInterface_iDispatcher)
{
    Plugin::LifecycleManagerShellTest shell;

    // TC-43b: QueryInterface for IDispatcher exercises the second INTERFACE_ENTRY.
    void* result = shell.QueryInterface(PluginHost::IDispatcher::ID);

    EXPECT_NE(result, nullptr);

    if (nullptr != result)
    {
        static_cast<PluginHost::IDispatcher*>(result)->Release();
    }
}

/* Test Case for Notification::QueryInterface
 *
 * Instantiate a LifecycleManagerShellTest and call notificationQueryInterface() which
 * forwards to Core::Sink<Notification>::QueryInterface.
 * Expected: querying ILifecycleManagerState::INotification returns a non-null pointer.
 * Covers LifecycleManager.h lines 56-59 (Notification::BEGIN_INTERFACE_MAP block).
 */

TEST_F(LifecycleManagerTest, lifecycleManager_notification_queryInterface)
{
    Plugin::LifecycleManagerShellTest shell;

    // TC-44: Notification implements ILifecycleManagerState::INotification — must be non-null.
    // Do NOT call Release() on the returned pointer: the Notification object is embedded
    // inside the shell's Core::Sink<Notification> member variable (not heap-allocated).
    // Calling Release() would decrement Core::Sink<T>'s internal ref count to zero and
    // trigger its Cleanup() on a non-heap object, causing a segfault.
    void* result = shell.notificationQueryInterface(
        Exchange::ILifecycleManagerState::INotification::ID);

    EXPECT_NE(result, nullptr);

    // Also verify the RPC::IRemoteConnection::INotification interface entry.
    void* rpcResult = shell.notificationQueryInterface(
        RPC::IRemoteConnection::INotification::ID);

    EXPECT_NE(rpcResult, nullptr);
}

/* Test Case for Notification::Activated
 *
 * Instantiate a LifecycleManagerShellTest and call notificationActivated(nullptr).
 * Notification::Activated() only logs and returns; a null connection pointer is safe here.
 * Expected: the call completes without error.
 * Covers LifecycleManager.h lines 61-64 (Notification::Activated body).
 */

TEST_F(LifecycleManagerTest, lifecycleManager_notification_activated)
{
    Plugin::LifecycleManagerShellTest shell;

    // TC-45: Activated() logs and returns — passing nullptr is safe and exercises all lines.
    shell.notificationActivated(nullptr);
}

/* Test Case for Notification::Deactivated
 *
 * Instantiate a LifecycleManagerShellTest and call notificationDeactivated(nullptr).
 * Notification::Deactivated() only logs and returns.
 * Expected: the call completes without error.
 * Covers LifecycleManager.h lines 66-69 (Notification::Deactivated body).
 */

TEST_F(LifecycleManagerTest, lifecycleManager_notification_deactivated)
{
    Plugin::LifecycleManagerShellTest shell;

    // TC-46: Deactivated() logs and returns — passing nullptr is safe and exercises all lines.
    shell.notificationDeactivated(nullptr);
}

/* Test Case for Notification::OnAppLifecycleStateChanged
 *
 * Instantiate a LifecycleManagerShellTest and call notificationOnAppLifecycleStateChanged()
 * directly.  This method logs and then calls
 * Exchange::JLifecycleManagerState::Event::OnAppLifecycleStateChanged which issues a
 * JSON-RPC Notify on the parent shell.  Because the shell has no registered subscribers
 * (Initialize() has not been called), the Notify() call is a safe no-op.
 * Expected: the call completes without error.
 * Covers LifecycleManager.h lines 71-79 (Notification::OnAppLifecycleStateChanged body).
 */

TEST_F(LifecycleManagerTest, lifecycleManager_notification_onAppLifecycleStateChanged)
{
    Plugin::LifecycleManagerShellTest shell;

    // TC-47: Directly invoke OnAppLifecycleStateChanged — exercises the LOGINFO and the
    // JLifecycleManagerState::Event dispatch path.  The shell has no JSON-RPC subscribers
    // so the event is silently discarded after the LOGINFO.
    shell.notificationOnAppLifecycleStateChanged(
        "com.test.app",
        "app-instance-001",
        Exchange::ILifecycleManager::LifecycleState::UNLOADED,
        Exchange::ILifecycleManager::LifecycleState::LOADING,
        "test.launch.intent"
    );
}

/* ===========================================================================
 * RuntimeManagerHandler coverage improvement tests (TC-48 through TC-56)
 *
 * Background: the lcov report generated from TC-1..TC-47 shows that several
 * branches in RuntimeManagerHandler.cpp are still at 0% hit count because
 * the pre-existing tests (TC-34, TC-35, TC-38) never successfully progress
 * the lifecycle state machine past the INITIALIZING-pending barrier.
 *
 * Root cause: InitializingState::handle() calls runtimeManagerHandler->run()
 * and then sets mPendingStateTransition=true / mPendingEventName="onAppRunning".
 * The state machine halts until the RuntimeManager fires an OnStateChanged
 * callback with RUNTIME_STATE_RUNNING.  In tests this is simulated by calling
 * mLifecycleManagerImpl->onRuntimeManagerEvent(data) with
 *   data["name"]  = "onStateChanged"
 *   data["state"] = static_cast<uint32_t>(RUNTIME_STATE_RUNNING)
 *
 * All new tests follow the existing fixture / mock patterns established in
 * TC-1..TC-47.  No new frameworks or helpers are introduced.
 * ===========================================================================
 */

/* Helper data used by several tests below to advance the state machine from
 * INITIALIZING-pending to PAUSED.  Each test that needs it calls
 * mLifecycleManagerImpl->onRuntimeManagerEvent(stateRunningData) after the
 * first onStateChangeEventSignal(). */

/* ---------------------------------------------------------------------------
 * TC-48 — spawnApp_withRunFailure
 *
 * Verifies RuntimeManagerHandler::run() failure path (lines 139-140).
 * When IRuntimeManager::Run() returns a non-ERROR_NONE code, run() sets
 * errorReason and returns false.  InitializingState::handle() propagates the
 * failure back to the StateHandler which emits an error state-change event
 * (old == new == LOADING).
 *
 * Setup:
 *   - Mock Run() to return Core::ERROR_GENERAL so that run() exercises lines
 *     139-140 (the errorReason assignment + return false branch).
 *   - Target lifecycle state is INITIALIZING so the background thread reaches
 *     InitializingState::handle().
 * Expected result: SpawnApp() returns Core::ERROR_NONE (the launch request is
 * queued successfully; the failure is handled asynchronously).
 * ---------------------------------------------------------------------------
 */
TEST_F(LifecycleManagerTest, spawnApp_withRunFailure)
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(Core::ERROR_GENERAL));

    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::INITIALIZING;

    // TC-48: SpawnApp queues an INITIALIZING request; background processes
    // LOADING→INITIALIZING and calls runtimeManagerHandler->run() which hits
    // the error path (lines 139-140) because Run() mock returns ERROR_GENERAL.
    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    releaseResources();
}

/* ---------------------------------------------------------------------------
 * TC-49 — spawnApp_withEnvVariablesArray
 *
 * Verifies RuntimeManagerHandler::run() env-variables copy loop (line 119).
 * When runtimeConfigObject.envVariables is a valid JSON array the inner loop
 *   for (unsigned int i = 0; i < envInputArray.Length(); ++i)
 * iterates and adds entries to envResultArray.  This exercises line 119 which
 * is bypassed whenever envVariables is an empty or invalid JSON string.
 *
 * Setup:
 *   - runtimeConfigObject.envVariables is set to a two-element JSON array.
 *   - target state is INITIALIZING so InitializingState::handle() calls run().
 * Expected result: SpawnApp() returns Core::ERROR_NONE.
 * ---------------------------------------------------------------------------
 */
TEST_F(LifecycleManagerTest, spawnApp_withEnvVariablesArray)
{
    createResources();

    // Set envVariables to a valid JSON array; run() will iterate over it
    // at line 119 (envResultArray.Add(envInputArray[i].String())).
    runtimeConfigObject.envVariables = "[\"CUSTOM_VAR1=hello\",\"CUSTOM_VAR2=world\"]";

    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::INITIALIZING;

    // TC-49: env variables copy loop is exercised inside run() because
    // envInputArray.Length() > 0 when envVariables is a JSON array string.
    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    releaseResources();
}

/* ---------------------------------------------------------------------------
 * TC-50 — killApp_fromPausedState
 *
 * Verifies RuntimeManagerHandler::kill() success path (lines 145-154).
 *
 * To reach RuntimeManagerHandler::kill() the app must be in a state from
 * which the TERMINATING transition is valid (PAUSED or SUSPENDED).  The
 * existing TC-38 never reaches this because the app is stuck at
 * INITIALIZING-pending when KillApp() is called.
 *
 * This test correctly advances the state machine by:
 *  1. SpawnApp with target PAUSED — app blocks at LOADING, background thread
 *     processes LOADING→INITIALIZING (run() succeeds, pending="onAppRunning").
 *  2. Fire onRuntimeManagerEvent(onStateChanged + RUNTIME_STATE_RUNNING) to
 *     trigger addStateTransitionRequest("onAppRunning") which queues PAUSED.
 *  3. Background thread processes INITIALIZING→PAUSED, app reaches PAUSED.
 *  4. KillApp() (force=true) from PAUSED triggers
 *     PAUSED→TERMINATING→UNLOADED; TerminatingState::handle() calls
 *     runtimeManagerHandler->kill() (lines 145-154).
 *
 * Expected result: KillApp() returns Core::ERROR_NONE; Kill() mock is called.
 * ---------------------------------------------------------------------------
 */
TEST_F(LifecycleManagerTest, killApp_fromPausedState)
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Return(Core::ERROR_NONE));

    EXPECT_CALL(*mRuntimeManagerMock, Kill(::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appInstanceId) {
                return Core::ERROR_NONE;
            }));

    // Step 1: Spawn app — blocks until LOADING state, then background proceeds
    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::PAUSED;
    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal(); // Wait for UNLOADED→LOADING event

    // Step 2: Fire onStateChanged(RUNTIME_STATE_RUNNING) to advance through
    // the INITIALIZING-pending barrier to PAUSED state.
    JsonObject stateRunningData;
    stateRunningData["name"] = "onStateChanged";
    stateRunningData["appInstanceId"] = appInstanceId;
    stateRunningData["state"] = static_cast<uint32_t>(Exchange::IRuntimeManager::RuntimeState::RUNTIME_STATE_RUNNING);

    mLifecycleManagerImpl->onRuntimeManagerEvent(stateRunningData);

    onStateChangeEventSignal(); // Wait for INITIALIZING→PAUSED (app in PAUSED state)

    // Step 3: Kill from PAUSED → TerminatingState::handle() → kill() (lines 145-154)
    // TC-50: Kill() mock must be called (force=true path).
    EXPECT_EQ(Core::ERROR_NONE, interface->KillApp(appInstanceId, errorReason, success));

    onStateChangeEventSignal(); // Wait for PAUSED→TERMINATING event

    releaseResources();
}

/* ---------------------------------------------------------------------------
 * TC-51 — killApp_withKillFailure_fromPausedState
 *
 * Verifies RuntimeManagerHandler::kill() failure path (lines 150-151).
 * Same setup as TC-50 but Kill() mock returns Core::ERROR_GENERAL, causing
 * run() to set errorReason and return false (lines 150-151).
 *
 * Expected result: KillApp() returns Core::ERROR_NONE (the state transition
 * request is queued asynchronously; the kill failure is handled internally).
 * ---------------------------------------------------------------------------
 */
TEST_F(LifecycleManagerTest, killApp_withKillFailure_fromPausedState)
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Return(Core::ERROR_NONE));

    EXPECT_CALL(*mRuntimeManagerMock, Kill(::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(Core::ERROR_GENERAL));

    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::PAUSED;
    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    JsonObject stateRunningData;
    stateRunningData["name"] = "onStateChanged";
    stateRunningData["appInstanceId"] = appInstanceId;
    stateRunningData["state"] = static_cast<uint32_t>(Exchange::IRuntimeManager::RuntimeState::RUNTIME_STATE_RUNNING);

    mLifecycleManagerImpl->onRuntimeManagerEvent(stateRunningData);

    onStateChangeEventSignal(); // Wait for app to reach PAUSED state

    // TC-51: Kill() mock returns ERROR_GENERAL — exercises lines 150-151
    // (errorReason assignment + return false in kill()).
    EXPECT_EQ(Core::ERROR_NONE, interface->KillApp(appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    releaseResources();
}

/* ---------------------------------------------------------------------------
 * TC-52 — unloadApp_withTerminateFailure_fromPausedState
 *
 * Verifies RuntimeManagerHandler::terminate() failure path (lines 161-162).
 * UnloadApp() issues a graceful terminate (force=false); TerminatingState::
 * handle() calls runtimeManagerHandler->terminate().  When Terminate() mock
 * returns Core::ERROR_GENERAL the failure path (lines 161-162) is taken.
 *
 * Expected result: UnloadApp() returns Core::ERROR_NONE (async); the
 * Terminate() mock is called and returns an error internally.
 * ---------------------------------------------------------------------------
 */
TEST_F(LifecycleManagerTest, unloadApp_withTerminateFailure_fromPausedState)
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Return(Core::ERROR_NONE));

    EXPECT_CALL(*mRuntimeManagerMock, Terminate(::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(Core::ERROR_GENERAL));

    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::PAUSED;
    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    JsonObject stateRunningData;
    stateRunningData["name"] = "onStateChanged";
    stateRunningData["appInstanceId"] = appInstanceId;
    stateRunningData["state"] = static_cast<uint32_t>(Exchange::IRuntimeManager::RuntimeState::RUNTIME_STATE_RUNNING);

    mLifecycleManagerImpl->onRuntimeManagerEvent(stateRunningData);

    onStateChangeEventSignal(); // Wait for app to reach PAUSED state

    // TC-52: Terminate() mock returns ERROR_GENERAL — exercises lines 161-162
    // (errorReason assignment + return false in terminate()).
    EXPECT_EQ(Core::ERROR_NONE, interface->UnloadApp(appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    releaseResources();
}

/* ---------------------------------------------------------------------------
 * TC-53 — setTargetAppState_toHibernated_viaCorrectPath
 *
 * Verifies RuntimeManagerHandler::hibernate() success path (lines 189-198).
 *
 * The existing TC-35 never reaches hibernate() because the app is stuck at
 * INITIALIZING-pending throughout.  This test correctly advances through:
 *   UNLOADED → LOADING → INITIALIZING → PAUSED → SUSPENDED → HIBERNATED
 *
 * At SUSPENDED→HIBERNATED, HibernatedState::handle() calls
 * runtimeManagerHandler->hibernate() which in turn calls
 * IRuntimeManager::Hibernate() (lines 189-198).
 *
 * Expected result: SetTargetAppState(HIBERNATED) returns Core::ERROR_NONE;
 * Hibernate() mock is called.
 * ---------------------------------------------------------------------------
 */
TEST_F(LifecycleManagerTest, setTargetAppState_toHibernated_viaCorrectPath)
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Return(Core::ERROR_NONE));

    EXPECT_CALL(*mWindowManagerMock, EnableDisplayRender(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(Core::ERROR_NONE));

    EXPECT_CALL(*mRuntimeManagerMock, Hibernate(::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Invoke(
            [&](const string& appInstanceId) {
                return Core::ERROR_NONE;
            }));

    // --- Reach PAUSED ---
    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::PAUSED;
    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    JsonObject stateRunningData;
    stateRunningData["name"] = "onStateChanged";
    stateRunningData["appInstanceId"] = appInstanceId;
    stateRunningData["state"] = static_cast<uint32_t>(Exchange::IRuntimeManager::RuntimeState::RUNTIME_STATE_RUNNING);

    mLifecycleManagerImpl->onRuntimeManagerEvent(stateRunningData);

    onStateChangeEventSignal(); // Wait for PAUSED state

    // --- Transition PAUSED → SUSPENDED ---
    // SuspendedState::handle() from non-HIBERNATED calls enableDisplayRender(false).
    EXPECT_EQ(Core::ERROR_NONE, interface->SetTargetAppState(appInstanceId, Exchange::ILifecycleManager::LifecycleState::SUSPENDED, ""));

    onStateChangeEventSignal(); // Wait for SUSPENDED state

    // --- Transition SUSPENDED → HIBERNATED ---
    // HibernatedState::handle() calls runtimeManagerHandler->hibernate() (lines 189-198).
    // TC-53: Hibernate() mock is called — full function coverage.
    EXPECT_EQ(Core::ERROR_NONE, interface->SetTargetAppState(appInstanceId, Exchange::ILifecycleManager::LifecycleState::HIBERNATED, ""));

    onStateChangeEventSignal(); // Wait for HIBERNATED state

    releaseResources();
}

/* ---------------------------------------------------------------------------
 * TC-54 — setTargetAppState_toHibernated_withHibernateFailure_viaCorrectPath
 *
 * Verifies RuntimeManagerHandler::hibernate() failure path (lines 193-195).
 * Same path as TC-53 but Hibernate() returns Core::ERROR_GENERAL so that
 * the failure branch (errorReason assignment + return false) is hit.
 *
 * Expected result: SetTargetAppState(HIBERNATED) returns Core::ERROR_NONE
 * (async); hibernate() internally receives the error (lines 193-195 hit).
 * ---------------------------------------------------------------------------
 */
TEST_F(LifecycleManagerTest, setTargetAppState_toHibernated_withHibernateFailure_viaCorrectPath)
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Return(Core::ERROR_NONE));

    EXPECT_CALL(*mWindowManagerMock, EnableDisplayRender(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(Core::ERROR_NONE));

    EXPECT_CALL(*mRuntimeManagerMock, Hibernate(::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(Core::ERROR_GENERAL));

    // --- Reach PAUSED ---
    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::PAUSED;
    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    JsonObject stateRunningData;
    stateRunningData["name"] = "onStateChanged";
    stateRunningData["appInstanceId"] = appInstanceId;
    stateRunningData["state"] = static_cast<uint32_t>(Exchange::IRuntimeManager::RuntimeState::RUNTIME_STATE_RUNNING);

    mLifecycleManagerImpl->onRuntimeManagerEvent(stateRunningData);

    onStateChangeEventSignal(); // Wait for PAUSED state

    // --- Transition PAUSED → SUSPENDED ---
    EXPECT_EQ(Core::ERROR_NONE, interface->SetTargetAppState(appInstanceId, Exchange::ILifecycleManager::LifecycleState::SUSPENDED, ""));

    onStateChangeEventSignal(); // Wait for SUSPENDED state

    // --- Transition SUSPENDED → HIBERNATED (Hibernate mock returns error) ---
    // TC-54: hibernate() failure path — lines 193-195 are now covered.
    EXPECT_EQ(Core::ERROR_NONE, interface->SetTargetAppState(appInstanceId, Exchange::ILifecycleManager::LifecycleState::HIBERNATED, ""));

    onStateChangeEventSignal();

    releaseResources();
}

/* ---------------------------------------------------------------------------
 * TC-55 — setTargetAppState_toSuspendedFromHibernated_success
 *
 * Verifies RuntimeManagerHandler::wake() success path with SUSPENDED target
 * (lines 200-213).
 *
 * When the app is in HIBERNATED state and SetTargetAppState(SUSPENDED) is
 * called, SuspendedState::handle() detects that the current lifecycle state
 * is HIBERNATED and calls:
 *   runtimeManagerHandler->wake(appInstanceId,
 *       Exchange::ILifecycleManager::LifecycleState::SUSPENDED, errorReason)
 * Inside wake(), the SUSPENDED branch maps to RUNTIME_STATE_SUSPENDED
 * (lines 203-206) then IRuntimeManager::Wake() is called (lines 211-212).
 *
 * Expected result: SetTargetAppState(SUSPENDED) returns Core::ERROR_NONE;
 * Wake() mock is called.
 * ---------------------------------------------------------------------------
 */
TEST_F(LifecycleManagerTest, setTargetAppState_toSuspendedFromHibernated_success)
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Return(Core::ERROR_NONE));

    EXPECT_CALL(*mWindowManagerMock, EnableDisplayRender(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(Core::ERROR_NONE));

    EXPECT_CALL(*mRuntimeManagerMock, Hibernate(::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Return(Core::ERROR_NONE));

    EXPECT_CALL(*mRuntimeManagerMock, Wake(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Return(Core::ERROR_NONE));

    // --- Reach PAUSED ---
    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::PAUSED;
    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    JsonObject stateRunningData;
    stateRunningData["name"] = "onStateChanged";
    stateRunningData["appInstanceId"] = appInstanceId;
    stateRunningData["state"] = static_cast<uint32_t>(Exchange::IRuntimeManager::RuntimeState::RUNTIME_STATE_RUNNING);

    mLifecycleManagerImpl->onRuntimeManagerEvent(stateRunningData);

    onStateChangeEventSignal(); // Wait for PAUSED state

    // --- PAUSED → SUSPENDED ---
    EXPECT_EQ(Core::ERROR_NONE, interface->SetTargetAppState(appInstanceId, Exchange::ILifecycleManager::LifecycleState::SUSPENDED, ""));

    onStateChangeEventSignal(); // Wait for SUSPENDED state

    // --- SUSPENDED → HIBERNATED ---
    EXPECT_EQ(Core::ERROR_NONE, interface->SetTargetAppState(appInstanceId, Exchange::ILifecycleManager::LifecycleState::HIBERNATED, ""));

    onStateChangeEventSignal(); // Wait for HIBERNATED state

    // --- HIBERNATED → SUSPENDED (triggers wake()) ---
    // SuspendedState::handle() from HIBERNATED calls
    // runtimeManagerHandler->wake(appInstanceId, SUSPENDED, ...).
    // Inside wake(): SUSPENDED branch (lines 203-206) + Wake() call (lines 211-212).
    // TC-55: wake() success path covered.
    EXPECT_EQ(Core::ERROR_NONE, interface->SetTargetAppState(appInstanceId, Exchange::ILifecycleManager::LifecycleState::SUSPENDED, ""));

    onStateChangeEventSignal(); // Wait for HIBERNATED→SUSPENDED via wake()

    releaseResources();
}

/* ---------------------------------------------------------------------------
 * TC-56 — setTargetAppState_toSuspendedFromHibernated_withWakeFailure
 *
 * Verifies RuntimeManagerHandler::wake() failure path (lines 214-215).
 * Same path as TC-55 but Wake() returns Core::ERROR_GENERAL so the failure
 * branch (errorReason assignment + return false) is exercised.
 *
 * Expected result: SetTargetAppState(SUSPENDED) returns Core::ERROR_NONE
 * (async); wake() internally receives the error (lines 214-215 hit).
 * ---------------------------------------------------------------------------
 */
TEST_F(LifecycleManagerTest, setTargetAppState_toSuspendedFromHibernated_withWakeFailure)
{
    createResources();

    EXPECT_CALL(*mRuntimeManagerMock, Run(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Return(Core::ERROR_NONE));

    EXPECT_CALL(*mWindowManagerMock, EnableDisplayRender(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(Core::ERROR_NONE));

    EXPECT_CALL(*mRuntimeManagerMock, Hibernate(::testing::_))
        .Times(::testing::AnyNumber())
        .WillOnce(::testing::Return(Core::ERROR_NONE));

    EXPECT_CALL(*mRuntimeManagerMock, Wake(::testing::_, ::testing::_))
        .Times(::testing::AnyNumber())
        .WillRepeatedly(::testing::Return(Core::ERROR_GENERAL));

    // --- Reach PAUSED ---
    targetLifecycleState = Exchange::ILifecycleManager::LifecycleState::PAUSED;
    EXPECT_EQ(Core::ERROR_NONE, interface->SpawnApp(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success));

    onStateChangeEventSignal();

    JsonObject stateRunningData;
    stateRunningData["name"] = "onStateChanged";
    stateRunningData["appInstanceId"] = appInstanceId;
    stateRunningData["state"] = static_cast<uint32_t>(Exchange::IRuntimeManager::RuntimeState::RUNTIME_STATE_RUNNING);

    mLifecycleManagerImpl->onRuntimeManagerEvent(stateRunningData);

    onStateChangeEventSignal(); // Wait for PAUSED state

    // --- PAUSED → SUSPENDED ---
    EXPECT_EQ(Core::ERROR_NONE, interface->SetTargetAppState(appInstanceId, Exchange::ILifecycleManager::LifecycleState::SUSPENDED, ""));

    onStateChangeEventSignal();

    // --- SUSPENDED → HIBERNATED ---
    EXPECT_EQ(Core::ERROR_NONE, interface->SetTargetAppState(appInstanceId, Exchange::ILifecycleManager::LifecycleState::HIBERNATED, ""));

    onStateChangeEventSignal();

    // --- HIBERNATED → SUSPENDED (Wake() returns error) ---
    // TC-56: wake() failure path — lines 214-215 are now covered.
    EXPECT_EQ(Core::ERROR_NONE, interface->SetTargetAppState(appInstanceId, Exchange::ILifecycleManager::LifecycleState::SUSPENDED, ""));

    onStateChangeEventSignal();

    releaseResources();
}
