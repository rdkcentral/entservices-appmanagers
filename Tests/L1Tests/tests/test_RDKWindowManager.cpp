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
#include <future>
#include <chrono>

#include "RDKWindowManager.h"
#define private public
#include "RDKWindowManagerImplementation.h"
#undef private
#include "ServiceMock.h"
#include "COMLinkMock.h"
#include "WindowManagerMock.h"
#include "RdkWindowManager.h"
#include "RdkWindowManagerMock.h"
#include "ISubSystemMock.h"
#include "ThunderPortability.h"
#include "WorkerPoolImplementation.h"
#include "FactoriesImplementation.h"

#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

using namespace WPEFramework;
using ::testing::NiceMock;
using ::testing::_;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::AtLeast;
using ::testing::NotNull;
using ::testing::IsNull;

namespace {
    const std::string TEST_CLIENT_ID      = "testApp";
    const std::string TEST_APP_INSTANCE_ID = "testAppInstance";
    const std::string TEST_DISPLAY_NAME   = "testDisplay";
}

namespace WPEFramework {
namespace Plugin {
    std::string toLower(const std::string& clientName);
}
}

extern int gCurrentFramerate;

class RDKWindowManagerTest : public ::testing::Test {
protected:
    Core::ProxyType<Plugin::RDKWindowManager> plugin;
    Core::JSONRPC::Handler& handler;
    DECL_CORE_JSONRPC_CONX connection;
    string response;

    FactoriesImplementation factoriesImplementation;
    Core::ProxyType<WorkerPoolImplementation> workerPool;
    NiceMock<ServiceMock> serviceMock;
    NiceMock<COMLinkMock> comlinkMock;
    NiceMock<WindowManagerMock>* windowManagerMock = nullptr;
    PLUGINHOST_DISPATCHER* dispatcher = nullptr;

    RDKWindowManagerTest()
        : plugin(Core::ProxyType<Plugin::RDKWindowManager>::Create()),
          handler(*plugin),
          INIT_CONX(1, 0),
          workerPool(Core::ProxyType<WorkerPoolImplementation>::Create(
              2, Core::Thread::DefaultStackSize(), 16))
    {
        Core::IWorkerPool::Assign(&(*workerPool));
        workerPool->Run();
    }

    ~RDKWindowManagerTest() override
    {
        Core::IWorkerPool::Assign(nullptr);
        workerPool.Release();
    }

    void SetUp() override
    {
        windowManagerMock = new NiceMock<WindowManagerMock>();

        PluginHost::IFactories::Assign(&factoriesImplementation);

        dispatcher = static_cast<PLUGINHOST_DISPATCHER*>(
            plugin->QueryInterface(PLUGINHOST_DISPATCHER_ID));
        dispatcher->Activate(&serviceMock);

        ON_CALL(serviceMock, AddRef()).WillByDefault(Return());
        ON_CALL(serviceMock, Release()).WillByDefault(Return(1u));
        ON_CALL(serviceMock, Callsign()).WillByDefault(Return("org.rdk.RDKWindowManager"));
        ON_CALL(serviceMock, Locator()).WillByDefault(Return("RDKWindowManager"));
        ON_CALL(serviceMock, SystemRootPath()).WillByDefault(Return(""));
        ON_CALL(serviceMock, ConfigLine()).WillByDefault(Return("{\"root\":{\"mode\":\"Local\"}}"));
        ON_CALL(serviceMock, COMLink()).WillByDefault(Return(&comlinkMock));

        ON_CALL(comlinkMock, RemoteConnection(_)).WillByDefault(Return(nullptr));

#ifdef USE_THUNDER_R4
        ON_CALL(comlinkMock, Instantiate(_, _, _))
            .WillByDefault(Invoke(
                [this](const WPEFramework::RPC::Object&, const uint32_t, uint32_t& connectionId) -> void* {
                    connectionId = 0;
                    return windowManagerMock;
                }));
#else
        ON_CALL(comlinkMock, Instantiate(_, _, _, _, _))
            .WillByDefault(Invoke(
                [this](const WPEFramework::RPC::Object&, const uint32_t, uint32_t& connectionId,
                       const string&, const string&) -> void* {
                    connectionId = 0;
                    return windowManagerMock;
                }));
#endif

        ON_CALL(*windowManagerMock, AddRef()).WillByDefault(Return());
        ON_CALL(*windowManagerMock, Release()).WillByDefault(Return(0u));
        ON_CALL(*windowManagerMock, QueryInterface(_)).WillByDefault(Return(nullptr));

        EXPECT_CALL(*windowManagerMock, Initialize(_))
            .WillOnce(Return(Core::ERROR_NONE));
        EXPECT_CALL(*windowManagerMock, Register(_))
            .WillOnce(Return(Core::ERROR_NONE));

        EXPECT_EQ(string(""), plugin->Initialize(&serviceMock));
    }

    void TearDown() override
    {
        EXPECT_CALL(*windowManagerMock, Unregister(_))
            .WillOnce(Return(Core::ERROR_NONE));
        EXPECT_CALL(*windowManagerMock, Deinitialize(_))
            .WillOnce(Return(Core::ERROR_NONE));

        plugin->Deinitialize(&serviceMock);

        dispatcher->Deactivate();
        dispatcher->Release();

        PluginHost::IFactories::Assign(nullptr);

        delete windowManagerMock;
        windowManagerMock = nullptr;
    }
};

class RDKWindowManagerImplementationTest : public ::testing::Test {
protected:
    Core::ProxyType<Plugin::RDKWindowManagerImplementation> windowManagerImplementation;
    FactoriesImplementation factoriesImplementation;
    Core::ProxyType<WorkerPoolImplementation> workerPool;
    NiceMock<ServiceMock> serviceMock;
    NiceMock<SubSystemMock> subSystemMock;
    NiceMock<RdkWindowManager::CompositorControllerImplMock> compositorMock;
    NiceMock<RdkWindowManager::RdkWindowManagerImplMock> rdkWindowManagerMock;

    RDKWindowManagerImplementationTest()
        : windowManagerImplementation(Core::ProxyType<Plugin::RDKWindowManagerImplementation>::Create())
        , workerPool(Core::ProxyType<WorkerPoolImplementation>::Create(
              2, Core::Thread::DefaultStackSize(), 16))
    {
        Core::IWorkerPool::Assign(&(*workerPool));
        workerPool->Run();
    }

    void SetUp() override
    {
        gCurrentFramerate = 60;
        RdkWindowManager::CompositorController::setImpl(&compositorMock);
        RdkWindowManager::RdkWindowManagerWrapper::setImpl(&rdkWindowManagerMock);
        PluginHost::IFactories::Assign(&factoriesImplementation);

        ON_CALL(rdkWindowManagerMock, milliseconds()).WillByDefault(Return(0.0));
        ON_CALL(rdkWindowManagerMock, microseconds()).WillByDefault(Return(0.0));
    }

    void TearDown() override
    {
        PluginHost::IFactories::Assign(nullptr);
        RdkWindowManager::CompositorController::setImpl(nullptr);
        RdkWindowManager::RdkWindowManagerWrapper::setImpl(nullptr);

        Core::IWorkerPool::Assign(nullptr);
        workerPool.Release();
    }
};

/* =====================================================================
 * RegisteredMethods
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, RegisteredMethodsExist)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("createDisplay")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getApps")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("addKeyIntercept")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("addKeyIntercepts")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("removeKeyIntercept")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("addKeyListener")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("removeKeyListener")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("injectKey")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("generateKey")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("enableInactivityReporting")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setInactivityInterval")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("resetInactivityTime")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("enableKeyRepeats")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getKeyRepeatsEnabled")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("ignoreKeyInputs")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("enableInputEvents")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("keyRepeatConfig")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setFocus")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setVisible")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getVisibility")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("renderReady")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("enableDisplayRender")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getLastKeyInfo")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setZOrder")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getZOrder")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("startVncServer")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("stopVncServer")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getScreenshot")));
}

/* =====================================================================
 * CreateDisplay
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, CreateDisplay_Success)
{
    const string displayParams = R"({"client":"testApp","displayName":"testDisplay","displayWidth":1920,"displayHeight":1080})";
    EXPECT_CALL(*windowManagerMock, CreateDisplay(_, _, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("createDisplay"),
            _T("{\"displayParams\":\"") + displayParams + _T("\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, CreateDisplay_Failure)
{
    const string displayParams = R"({"client":"","displayName":""})";
    EXPECT_CALL(*windowManagerMock, CreateDisplay(_, _, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("createDisplay"),
            _T("{\"displayParams\":\"") + displayParams + _T("\"}"),
            response));
}

/* =====================================================================
 * GetApps
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, GetApps_Success)
{
    const string expectedApps = R"(["testApp","anotherApp"])";
    EXPECT_CALL(*windowManagerMock, GetApps(_))
        .WillOnce(Invoke([&](string& appsIds) {
            appsIds = expectedApps;
            return Core::ERROR_NONE;
        }));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("getApps"), _T("{}"), response));
}

TEST_F(RDKWindowManagerTest, GetApps_Failure)
{
    EXPECT_CALL(*windowManagerMock, GetApps(_))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("getApps"), _T("{}"), response));
}

/* =====================================================================
 * AddKeyIntercept
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, AddKeyIntercept_Success)
{
    const string interceptParams = R"({"client":"testApp","keyCode":65,"modifiers":["ctrl"],"focusOnly":false,"propagate":true})";
    EXPECT_CALL(*windowManagerMock, AddKeyIntercept(_))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("addKeyIntercept"),
            _T("{\"intercept\":\"") + interceptParams + _T("\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, AddKeyIntercept_Failure)
{
    const string interceptParams = R"({"client":"","keyCode":0,"modifiers":[]})";
    EXPECT_CALL(*windowManagerMock, AddKeyIntercept(_))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("addKeyIntercept"),
            _T("{\"intercept\":\"") + interceptParams + _T("\"}"),
            response));
}

/* =====================================================================
 * AddKeyIntercepts
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, AddKeyIntercepts_Success)
{
    const string intercepts = R"([{"keyCode":65,"modifiers":["ctrl"],"focusOnly":false,"propagate":true}])";
    EXPECT_CALL(*windowManagerMock, AddKeyIntercepts(_, _))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("addKeyIntercepts"),
            _T("{\"clientId\":\"testApp\",\"intercepts\":\"") + intercepts + _T("\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, AddKeyIntercepts_Failure)
{
    EXPECT_CALL(*windowManagerMock, AddKeyIntercepts(_, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("addKeyIntercepts"),
            _T("{\"clientId\":\"testApp\",\"intercepts\":\"[]\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, AddKeyIntercepts_Success_Additional)
{
    const string intercepts = R"([{"keyCode":65,"modifiers":["ctrl"],"focusOnly":false,"propagate":true}])";
    EXPECT_CALL(*windowManagerMock, AddKeyIntercepts(_, _))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("addKeyIntercepts"),
            _T("{\"clientId\":\"testApp\",\"intercepts\":\"") + intercepts + _T("\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, AddKeyIntercepts_MissingClientId_Failure)
{
    EXPECT_CALL(*windowManagerMock, AddKeyIntercepts(_, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("addKeyIntercepts"),
            _T("{\"intercepts\":\"[]\"}"),
            response));
}

/* =====================================================================
 * RemoveKeyIntercept
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, RemoveKeyIntercept_Success)
{
    EXPECT_CALL(*windowManagerMock, RemoveKeyIntercept(TEST_CLIENT_ID, 65u, _))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("removeKeyIntercept"),
            _T("{\"clientId\":\"testApp\",\"keyCode\":65,\"modifiers\":\"[\\\"ctrl\\\"]\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, RemoveKeyIntercept_Failure)
{
    EXPECT_CALL(*windowManagerMock, RemoveKeyIntercept(_, _, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("removeKeyIntercept"),
            _T("{\"clientId\":\"testApp\",\"keyCode\":65,\"modifiers\":\"[]\"}"),
            response));
}

/* =====================================================================
 * AddKeyListener
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, AddKeyListener_Success)
{
    const string keyListeners = R"({"client":"testApp","keys":[{"keyCode":65,"nativeKeyCode":65,"modifiers":["ctrl"],"activate":true,"propagate":false}]})";
    EXPECT_CALL(*windowManagerMock, AddKeyListener(_))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("addKeyListener"),
            _T("{\"keyListeners\":\"") + keyListeners + _T("\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, AddKeyListener_Failure)
{
    EXPECT_CALL(*windowManagerMock, AddKeyListener(_))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("addKeyListener"),
            _T("{\"keyListeners\":\"{}\"}"),
            response));
}

/* =====================================================================
 * RemoveKeyListener
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, RemoveKeyListener_Success)
{
    const string keyListeners = R"({"client":"testApp","keys":[{"keyCode":65,"modifiers":["ctrl"]}]})";
    EXPECT_CALL(*windowManagerMock, RemoveKeyListener(_))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("removeKeyListener"),
            _T("{\"keyListeners\":\"") + keyListeners + _T("\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, RemoveKeyListener_Failure)
{
    EXPECT_CALL(*windowManagerMock, RemoveKeyListener(_))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("removeKeyListener"),
            _T("{\"keyListeners\":\"{}\"}"),
            response));
}

/* =====================================================================
 * InjectKey
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, InjectKey_Success)
{
    EXPECT_CALL(*windowManagerMock, InjectKey(65u, _))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("injectKey"),
            _T("{\"keyCode\":65,\"modifiers\":\"[\\\"ctrl\\\"]\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, InjectKey_Failure)
{
    EXPECT_CALL(*windowManagerMock, InjectKey(_, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("injectKey"),
            _T("{\"keyCode\":0,\"modifiers\":\"[]\"}"),
            response));
}

/* =====================================================================
 * GenerateKey
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, GenerateKey_Success)
{
    const string keys = R"([{"keyCode":65,"modifiers":["ctrl"],"client":"testApp","delay":0}])";
    EXPECT_CALL(*windowManagerMock, GenerateKey(_, _))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("generateKey"),
            _T("{\"keys\":\"") + keys + _T("\",\"client\":\"testApp\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, GenerateKey_Failure)
{
    EXPECT_CALL(*windowManagerMock, GenerateKey(_, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("generateKey"),
            _T("{\"keys\":\"[]\",\"client\":\"testApp\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, GenerateKey_Success_Additional)
{
    const string keys = R"([{"keyCode":65,"modifiers":["ctrl"],"delay":0}])";
    EXPECT_CALL(*windowManagerMock, GenerateKey(_, _))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("generateKey"),
            _T("{\"keys\":\"") + keys + _T("\",\"client\":\"testApp\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, GenerateKey_MissingKeys_Failure)
{
    EXPECT_CALL(*windowManagerMock, GenerateKey(_, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("generateKey"),
            _T("{\"client\":\"testApp\"}"),
            response));
}

/* =====================================================================
 * EnableInactivityReporting
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, EnableInactivityReporting_Enable_Success)
{
    EXPECT_CALL(*windowManagerMock, EnableInactivityReporting(true))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("enableInactivityReporting"),
            _T("{\"enable\":true}"),
            response));
}

TEST_F(RDKWindowManagerTest, EnableInactivityReporting_Disable_Success)
{
    EXPECT_CALL(*windowManagerMock, EnableInactivityReporting(false))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("enableInactivityReporting"),
            _T("{\"enable\":false}"),
            response));
}

TEST_F(RDKWindowManagerTest, EnableInactivityReporting_Failure)
{
    EXPECT_CALL(*windowManagerMock, EnableInactivityReporting(_))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("enableInactivityReporting"),
            _T("{\"enable\":true}"),
            response));
}

/* =====================================================================
 * SetInactivityInterval
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, SetInactivityInterval_Success)
{
    EXPECT_CALL(*windowManagerMock, SetInactivityInterval(60u))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("setInactivityInterval"),
            _T("{\"interval\":60}"),
            response));
}

TEST_F(RDKWindowManagerTest, SetInactivityInterval_ZeroInterval_Success)
{
    EXPECT_CALL(*windowManagerMock, SetInactivityInterval(0u))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("setInactivityInterval"),
            _T("{\"interval\":0}"),
            response));
}

TEST_F(RDKWindowManagerTest, SetInactivityInterval_Failure)
{
    EXPECT_CALL(*windowManagerMock, SetInactivityInterval(_))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("setInactivityInterval"),
            _T("{\"interval\":60}"),
            response));
}

/* =====================================================================
 * ResetInactivityTime
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, ResetInactivityTime_Success)
{
    EXPECT_CALL(*windowManagerMock, ResetInactivityTime())
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("resetInactivityTime"), _T("{}"), response));
}

TEST_F(RDKWindowManagerTest, ResetInactivityTime_Failure)
{
    EXPECT_CALL(*windowManagerMock, ResetInactivityTime())
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("resetInactivityTime"), _T("{}"), response));
}

/* =====================================================================
 * EnableKeyRepeats
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, EnableKeyRepeats_Enable_Success)
{
    EXPECT_CALL(*windowManagerMock, EnableKeyRepeats(true))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("enableKeyRepeats"),
            _T("{\"enable\":true}"),
            response));
}

TEST_F(RDKWindowManagerTest, EnableKeyRepeats_Disable_Success)
{
    EXPECT_CALL(*windowManagerMock, EnableKeyRepeats(false))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("enableKeyRepeats"),
            _T("{\"enable\":false}"),
            response));
}

TEST_F(RDKWindowManagerTest, EnableKeyRepeats_Failure)
{
    EXPECT_CALL(*windowManagerMock, EnableKeyRepeats(_))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("enableKeyRepeats"),
            _T("{\"enable\":true}"),
            response));
}

/* =====================================================================
 * GetKeyRepeatsEnabled
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, GetKeyRepeatsEnabled_ReturnsTrue_Success)
{
    EXPECT_CALL(*windowManagerMock, GetKeyRepeatsEnabled(_))
        .WillOnce(Invoke([](bool& keyRepeat) {
            keyRepeat = true;
            return Core::ERROR_NONE;
        }));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("getKeyRepeatsEnabled"), _T("{}"), response));
}

TEST_F(RDKWindowManagerTest, GetKeyRepeatsEnabled_ReturnsFalse_Success)
{
    EXPECT_CALL(*windowManagerMock, GetKeyRepeatsEnabled(_))
        .WillOnce(Invoke([](bool& keyRepeat) {
            keyRepeat = false;
            return Core::ERROR_NONE;
        }));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("getKeyRepeatsEnabled"), _T("{}"), response));

    JsonObject result;
    result.FromString(response);
    EXPECT_FALSE(result["keyRepeat"].Boolean());
}

TEST_F(RDKWindowManagerTest, GetKeyRepeatsEnabled_ReturnsTrue_Success_Additional)
{
    EXPECT_CALL(*windowManagerMock, GetKeyRepeatsEnabled(_))
        .WillOnce(Invoke([](bool& keyRepeat) {
            keyRepeat = true;
            return Core::ERROR_NONE;
        }));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("getKeyRepeatsEnabled"), _T("{}"), response));
}

TEST_F(RDKWindowManagerTest, GetKeyRepeatsEnabled_Failure)
{
    EXPECT_CALL(*windowManagerMock, GetKeyRepeatsEnabled(_))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("getKeyRepeatsEnabled"), _T("{}"), response));
}

/* =====================================================================
 * IgnoreKeyInputs
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, IgnoreKeyInputs_Ignore_Success)
{
    EXPECT_CALL(*windowManagerMock, IgnoreKeyInputs(true))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("ignoreKeyInputs"),
            _T("{\"ignore\":true}"),
            response));
}

TEST_F(RDKWindowManagerTest, IgnoreKeyInputs_Unignore_Success)
{
    EXPECT_CALL(*windowManagerMock, IgnoreKeyInputs(false))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("ignoreKeyInputs"),
            _T("{\"ignore\":false}"),
            response));
}

TEST_F(RDKWindowManagerTest, IgnoreKeyInputs_Failure)
{
    EXPECT_CALL(*windowManagerMock, IgnoreKeyInputs(_))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("ignoreKeyInputs"),
            _T("{\"ignore\":true}"),
            response));
}

/* =====================================================================
 * EnableInputEvents
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, EnableInputEvents_Enable_Success)
{
    const string clients = R"(["testApp","anotherApp"])";
    EXPECT_CALL(*windowManagerMock, EnableInputEvents(_, _))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("enableInputEvents"),
            _T("{\"clients\":\"") + clients + _T("\",\"enable\":true}"),
            response));
}

TEST_F(RDKWindowManagerTest, EnableInputEvents_Disable_Success)
{
    const string clients = R"(["testApp"])";
    EXPECT_CALL(*windowManagerMock, EnableInputEvents(_, false))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("enableInputEvents"),
            _T("{\"clients\":\"") + clients + _T("\",\"enable\":false}"),
            response));
}

TEST_F(RDKWindowManagerTest, EnableInputEvents_Failure)
{
    EXPECT_CALL(*windowManagerMock, EnableInputEvents(_, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("enableInputEvents"),
            _T("{\"clients\":\"[]\",\"enable\":true}"),
            response));
}

TEST_F(RDKWindowManagerTest, EnableInputEvents_Enable_Success_Additional)
{
    const string clients = R"(["testApp","anotherApp"])";
    EXPECT_CALL(*windowManagerMock, EnableInputEvents(_, _))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("enableInputEvents"),
            _T("{\"clients\":\"") + clients + _T("\",\"enable\":true}"),
            response));
}

TEST_F(RDKWindowManagerTest, EnableInputEvents_MissingClients_Failure)
{
    EXPECT_CALL(*windowManagerMock, EnableInputEvents(_, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("enableInputEvents"),
            _T("{\"enable\":true}"),
            response));
}

/* =====================================================================
 * KeyRepeatConfig
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, KeyRepeatConfig_Default_Success)
{
    const string keyConfig = R"({"enabled":true,"initialDelay":500,"repeatInterval":100})";
    EXPECT_CALL(*windowManagerMock, KeyRepeatConfig(_, _))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("keyRepeatConfig"),
            _T("{\"input\":\"default\",\"keyConfig\":\"") + keyConfig + _T("\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, KeyRepeatConfig_Keyboard_Success)
{
    const string keyConfig = R"({"enabled":true,"initialDelay":250,"repeatInterval":50})";
    EXPECT_CALL(*windowManagerMock, KeyRepeatConfig(_, _))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("keyRepeatConfig"),
            _T("{\"input\":\"keyboard\",\"keyConfig\":\"") + keyConfig + _T("\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, KeyRepeatConfig_Failure)
{
    EXPECT_CALL(*windowManagerMock, KeyRepeatConfig(_, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("keyRepeatConfig"),
            _T("{\"input\":\"default\",\"keyConfig\":\"{}\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, KeyRepeatConfig_Default_Success_Additional)
{
    const string keyConfig = R"({"enabled":true,"initialDelay":500,"repeatInterval":100})";
    EXPECT_CALL(*windowManagerMock, KeyRepeatConfig(_, _))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("keyRepeatConfig"),
            _T("{\"input\":\"default\",\"keyConfig\":\"") + keyConfig + _T("\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, KeyRepeatConfig_Keyboard_Success_Additional)
{
    const string keyConfig = R"({"enabled":true,"initialDelay":250,"repeatInterval":50})";
    EXPECT_CALL(*windowManagerMock, KeyRepeatConfig(_, _))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("keyRepeatConfig"),
            _T("{\"input\":\"keyboard\",\"keyConfig\":\"") + keyConfig + _T("\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, KeyRepeatConfig_MissingInput_Failure)
{
    EXPECT_CALL(*windowManagerMock, KeyRepeatConfig(_, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("keyRepeatConfig"),
            _T("{\"keyConfig\":\"{}\"}"),
            response));
}

/* =====================================================================
 * SetFocus
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, SetFocus_Success)
{
    EXPECT_CALL(*windowManagerMock, SetFocus(TEST_CLIENT_ID))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("setFocus"),
            _T("{\"client\":\"testApp\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, SetFocus_Failure_ClientNotFound)
{
    EXPECT_CALL(*windowManagerMock, SetFocus(_))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("setFocus"),
            _T("{\"client\":\"nonExistentApp\"}"),
            response));
}

/* =====================================================================
 * SetVisible
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, SetVisible_Show_Success)
{
    EXPECT_CALL(*windowManagerMock, SetVisible(TEST_CLIENT_ID, true))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("setVisible"),
            _T("{\"client\":\"testApp\",\"visible\":true}"),
            response));
}

TEST_F(RDKWindowManagerTest, SetVisible_Hide_Success)
{
    EXPECT_CALL(*windowManagerMock, SetVisible(TEST_CLIENT_ID, false))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("setVisible"),
            _T("{\"client\":\"testApp\",\"visible\":false}"),
            response));
}

TEST_F(RDKWindowManagerTest, SetVisible_Failure)
{
    EXPECT_CALL(*windowManagerMock, SetVisible(_, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("setVisible"),
            _T("{\"client\":\"testApp\",\"visible\":true}"),
            response));
}

/* =====================================================================
 * GetVisibility
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, GetVisibility_Visible_Success)
{
    EXPECT_CALL(*windowManagerMock, GetVisibility(TEST_CLIENT_ID, _))
        .WillOnce(Invoke([](const std::string&, bool& visible) {
            visible = true;
            return Core::ERROR_NONE;
        }));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("getVisibility"),
            _T("{\"client\":\"testApp\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, GetVisibility_Hidden_Success)
{
    EXPECT_CALL(*windowManagerMock, GetVisibility(TEST_CLIENT_ID, _))
        .WillOnce(Invoke([](const std::string&, bool& visible) {
            visible = false;
            return Core::ERROR_NONE;
        }));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("getVisibility"),
            _T("{\"client\":\"testApp\"}"),
            response));

    JsonObject result;
    result.FromString(response);
    EXPECT_FALSE(result["visible"].Boolean());
}

TEST_F(RDKWindowManagerTest, GetVisibility_Failure)
{
    EXPECT_CALL(*windowManagerMock, GetVisibility(_, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("getVisibility"),
            _T("{\"client\":\"testApp\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, GetVisibility_Visible_Success_Additional)
{
    EXPECT_CALL(*windowManagerMock, GetVisibility(TEST_CLIENT_ID, _))
        .WillOnce(Invoke([](const std::string&, bool& visible) {
            visible = true;
            return Core::ERROR_NONE;
        }));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("getVisibility"),
            _T("{\"client\":\"testApp\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, GetVisibility_MissingClient_Failure)
{
    EXPECT_CALL(*windowManagerMock, GetVisibility(_, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("getVisibility"), _T("{}"), response));
}

/* =====================================================================
 * RenderReady
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, RenderReady_Ready_Success)
{
    EXPECT_CALL(*windowManagerMock, RenderReady(TEST_CLIENT_ID, _))
        .WillOnce(Invoke([](const string&, bool& status) {
            status = true;
            return Core::ERROR_NONE;
        }));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("renderReady"),
            _T("{\"client\":\"testApp\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, RenderReady_NotReady_Success)
{
    EXPECT_CALL(*windowManagerMock, RenderReady(TEST_CLIENT_ID, _))
        .WillOnce(Invoke([](const string&, bool& status) {
            status = false;
            return Core::ERROR_NONE;
        }));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("renderReady"),
            _T("{\"client\":\"testApp\"}"),
            response));

    JsonObject result;
    result.FromString(response);
    EXPECT_FALSE(result["status"].Boolean());
}

TEST_F(RDKWindowManagerTest, RenderReady_Failure)
{
    EXPECT_CALL(*windowManagerMock, RenderReady(_, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("renderReady"),
            _T("{\"client\":\"testApp\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, RenderReady_Ready_Success_Additional)
{
    EXPECT_CALL(*windowManagerMock, RenderReady(TEST_CLIENT_ID, _))
        .WillOnce(Invoke([](const string&, bool& status) {
            status = true;
            return Core::ERROR_NONE;
        }));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("renderReady"),
            _T("{\"client\":\"testApp\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, RenderReady_MissingClient_Failure)
{
    EXPECT_CALL(*windowManagerMock, RenderReady(_, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("renderReady"), _T("{}"), response));
}

/* =====================================================================
 * EnableDisplayRender
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, EnableDisplayRender_Enable_Success)
{
    EXPECT_CALL(*windowManagerMock, EnableDisplayRender(TEST_CLIENT_ID, true))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("enableDisplayRender"),
            _T("{\"client\":\"testApp\",\"enable\":true}"),
            response));
}

TEST_F(RDKWindowManagerTest, EnableDisplayRender_Disable_Success)
{
    EXPECT_CALL(*windowManagerMock, EnableDisplayRender(TEST_CLIENT_ID, false))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("enableDisplayRender"),
            _T("{\"client\":\"testApp\",\"enable\":false}"),
            response));
}

TEST_F(RDKWindowManagerTest, EnableDisplayRender_Failure)
{
    EXPECT_CALL(*windowManagerMock, EnableDisplayRender(_, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("enableDisplayRender"),
            _T("{\"client\":\"testApp\",\"enable\":true}"),
            response));
}

/* =====================================================================
 * GetLastKeyInfo
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, GetLastKeyInfo_Success)
{
    const uint32_t expectedKeyCode   = 65u;
    const uint32_t expectedModifiers = 1u;
    const uint64_t expectedTimestamp = 1700000000u;

    EXPECT_CALL(*windowManagerMock, GetLastKeyInfo(_, _, _))
        .WillOnce(Invoke([&](uint32_t& keyCode, uint32_t& modifiers, uint64_t& timestampInSeconds) {
            keyCode           = expectedKeyCode;
            modifiers         = expectedModifiers;
            timestampInSeconds = expectedTimestamp;
            return Core::ERROR_NONE;
        }));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("getLastKeyInfo"), _T("{}"), response));

    JsonObject result;
    result.FromString(response);
    EXPECT_EQ(static_cast<uint32_t>(result["keyCode"].Number()), expectedKeyCode);
    EXPECT_EQ(static_cast<uint32_t>(result["modifiers"].Number()), expectedModifiers);
}

TEST_F(RDKWindowManagerTest, GetLastKeyInfo_NoKeyPressAvailable_Failure)
{
    EXPECT_CALL(*windowManagerMock, GetLastKeyInfo(_, _, _))
        .WillOnce(Return(Core::ERROR_UNAVAILABLE));

    EXPECT_EQ(Core::ERROR_UNAVAILABLE,
        handler.Invoke(connection, _T("getLastKeyInfo"), _T("{}"), response));
}

TEST_F(RDKWindowManagerTest, GetLastKeyInfo_GeneralFailure)
{
    EXPECT_CALL(*windowManagerMock, GetLastKeyInfo(_, _, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("getLastKeyInfo"), _T("{}"), response));
}

/* =====================================================================
 * SetZOrder
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, SetZOrder_Success)
{
    EXPECT_CALL(*windowManagerMock, SetZOrder(TEST_APP_INSTANCE_ID, 5))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("setZOrder"),
            _T("{\"appInstanceId\":\"testAppInstance\",\"zOrder\":5}"),
            response));
}

TEST_F(RDKWindowManagerTest, SetZOrder_NegativeZOrder_Success)
{
    EXPECT_CALL(*windowManagerMock, SetZOrder(TEST_APP_INSTANCE_ID, -1))
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("setZOrder"),
            _T("{\"appInstanceId\":\"testAppInstance\",\"zOrder\":-1}"),
            response));
}

TEST_F(RDKWindowManagerTest, SetZOrder_Failure)
{
    EXPECT_CALL(*windowManagerMock, SetZOrder(_, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("setZOrder"),
            _T("{\"appInstanceId\":\"testAppInstance\",\"zOrder\":5}"),
            response));
}

/* =====================================================================
 * GetZOrder
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, GetZOrder_Success)
{
    EXPECT_CALL(*windowManagerMock, GetZOrder(TEST_APP_INSTANCE_ID, _))
        .WillOnce(Invoke([](const string&, int32_t& zOrder) {
            zOrder = 3;
            return Core::ERROR_NONE;
        }));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("getZOrder"),
            _T("{\"appInstanceId\":\"testAppInstance\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, GetZOrder_Failure)
{
    EXPECT_CALL(*windowManagerMock, GetZOrder(_, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("getZOrder"),
            _T("{\"appInstanceId\":\"testAppInstance\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, GetZOrder_Success_Additional)
{
    EXPECT_CALL(*windowManagerMock, GetZOrder(TEST_APP_INSTANCE_ID, _))
        .WillOnce(Invoke([](const string&, int32_t& zOrder) {
            zOrder = 3;
            return Core::ERROR_NONE;
        }));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("getZOrder"),
            _T("{\"appInstanceId\":\"testAppInstance\"}"),
            response));
}

TEST_F(RDKWindowManagerTest, GetZOrder_MissingAppInstanceId_Failure)
{
    EXPECT_CALL(*windowManagerMock, GetZOrder(_, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("getZOrder"), _T("{}"), response));
}

TEST_F(RDKWindowManagerTest, CreateDisplay_MissingDisplayParams_Failure)
{
    EXPECT_CALL(*windowManagerMock, CreateDisplay(_, _, _, _, _, _, _, _, _, _, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("createDisplay"), _T("{}"), response));
}

TEST_F(RDKWindowManagerTest, AddKeyIntercept_MissingIntercept_Failure)
{
    EXPECT_CALL(*windowManagerMock, AddKeyIntercept(_))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("addKeyIntercept"), _T("{}"), response));
}

TEST_F(RDKWindowManagerTest, AddKeyListener_MissingKeyListeners_Failure)
{
    EXPECT_CALL(*windowManagerMock, AddKeyListener(_))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("addKeyListener"), _T("{}"), response));
}

TEST_F(RDKWindowManagerTest, RemoveKeyListener_MissingKeyListeners_Failure)
{
    EXPECT_CALL(*windowManagerMock, RemoveKeyListener(_))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("removeKeyListener"), _T("{}"), response));
}

TEST_F(RDKWindowManagerTest, RemoveKeyIntercept_MissingClientId_Failure)
{
    EXPECT_CALL(*windowManagerMock, RemoveKeyIntercept(_, _, _))
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("removeKeyIntercept"),
            _T("{\"keyCode\":65,\"modifiers\":\"[]\"}"),
            response));
}

/* =====================================================================
 * StartVncServer
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, StartVncServer_Success)
{
    EXPECT_CALL(*windowManagerMock, StartVncServer())
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("startVncServer"), _T("{}"), response));
}

TEST_F(RDKWindowManagerTest, StartVncServer_Failure)
{
    EXPECT_CALL(*windowManagerMock, StartVncServer())
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("startVncServer"), _T("{}"), response));
}

/* =====================================================================
 * StopVncServer
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, StopVncServer_Success)
{
    EXPECT_CALL(*windowManagerMock, StopVncServer())
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("stopVncServer"), _T("{}"), response));
}

TEST_F(RDKWindowManagerTest, StopVncServer_Failure)
{
    EXPECT_CALL(*windowManagerMock, StopVncServer())
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("stopVncServer"), _T("{}"), response));
}

/* =====================================================================
 * GetScreenshot
 * ===================================================================== */
TEST_F(RDKWindowManagerTest, GetScreenshot_Success)
{
    EXPECT_CALL(*windowManagerMock, GetScreenshot())
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(connection, _T("getScreenshot"), _T("{}"), response));
}

TEST_F(RDKWindowManagerTest, GetScreenshot_Failure)
{
    EXPECT_CALL(*windowManagerMock, GetScreenshot())
        .WillOnce(Return(Core::ERROR_GENERAL));

    EXPECT_EQ(Core::ERROR_GENERAL,
        handler.Invoke(connection, _T("getScreenshot"), _T("{}"), response));
}

/* =====================================================================
 * Direct Implementation Coverage (No notifications/events)
 * ===================================================================== */
TEST_F(RDKWindowManagerImplementationTest, Impl_CreateDisplay_EmptyParams_Failure)
{
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->CreateDisplay("", "", 0, 0, false, 0, 0, 0, 0, false, false));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_GetApps_Success)
{
    string apps;
    EXPECT_CALL(compositorMock, getClients(_))
        .WillOnce(Invoke([](std::vector<std::string>& clients) {
            clients = { "testapp", "anotherapp" };
            return true;
        }));

    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->GetApps(apps));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_GetApps_Failure)
{
    string apps;
    EXPECT_CALL(compositorMock, getClients(_))
        .WillOnce(Return(false));

    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->GetApps(apps));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_EnableInputEvents_InvalidJson_Failure)
{
    EXPECT_CALL(compositorMock, enableInputEvents(_, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->EnableInputEvents("invalid-json", true));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_EnableInputEvents_Wildcard_Success)
{
    const string clients = R"({"clients":["*"]})";
    EXPECT_CALL(compositorMock, getClients(_))
        .WillOnce(Invoke([](std::vector<std::string>& list) {
            list = { "testapp", "anotherapp" };
            return true;
        }));
    EXPECT_CALL(compositorMock, enableInputEvents(_, true))
        .Times(2)
        .WillRepeatedly(Return(true));

    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->EnableInputEvents(clients, true));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_KeyRepeatConfig_Default_Success)
{
    const string keyConfig = R"({"enabled":true,"initialDelay":500,"repeatInterval":100})";
    EXPECT_CALL(compositorMock, setKeyRepeatConfig(true, 500, 100)).Times(1);

    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->KeyRepeatConfig("default", keyConfig));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_KeyRepeatConfig_InvalidInput_Failure)
{
    const string keyConfig = R"({"enabled":true,"initialDelay":500,"repeatInterval":100})";
    EXPECT_CALL(compositorMock, setKeyRepeatConfig(_, _, _)).Times(0);

    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->KeyRepeatConfig("mouse", keyConfig));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_SetFocus_EmptyClient_Failure)
{
    EXPECT_CALL(compositorMock, setFocus(_)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->SetFocus(""));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_SetFocus_Success)
{
    EXPECT_CALL(compositorMock, setFocus(TEST_CLIENT_ID)).WillOnce(Return(true));
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->SetFocus(TEST_CLIENT_ID));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_GetVisibility_Success)
{
    bool visible = false;
    EXPECT_CALL(compositorMock, getVisibility(TEST_CLIENT_ID, _))
        .WillOnce(Invoke([](const std::string&, bool& value) {
            value = true;
            return true;
        }));

    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->GetVisibility(TEST_CLIENT_ID, visible));
    EXPECT_TRUE(visible);
}

TEST_F(RDKWindowManagerImplementationTest, Impl_RenderReady_EmptyClient_Failure)
{
    bool status = false;
    EXPECT_CALL(compositorMock, renderReady(_)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->RenderReady("", status));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_RenderReady_Success)
{
    bool status = false;
    EXPECT_CALL(compositorMock, renderReady(TEST_CLIENT_ID)).WillOnce(Return(true));

    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->RenderReady(TEST_CLIENT_ID, status));
    EXPECT_TRUE(status);
}

TEST_F(RDKWindowManagerImplementationTest, Impl_GetLastKeyInfo_Success)
{
    uint32_t keyCode = 0;
    uint32_t modifiers = 0;
    uint64_t ts = 0;

    EXPECT_CALL(compositorMock, getLastKeyPress(_, _, _))
        .WillOnce(Invoke([](uint32_t& outKey, uint32_t& outMods, uint64_t& outTs) {
            outKey = 65;
            outMods = 1;
            outTs = 12345;
            return true;
        }));

    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->GetLastKeyInfo(keyCode, modifiers, ts));
    EXPECT_EQ(65u, keyCode);
    EXPECT_EQ(1u, modifiers);
    EXPECT_EQ(12345u, ts);
}

TEST_F(RDKWindowManagerImplementationTest, Impl_GetLastKeyInfo_Failure)
{
    uint32_t keyCode = 0;
    uint32_t modifiers = 0;
    uint64_t ts = 0;

    EXPECT_CALL(compositorMock, getLastKeyPress(_, _, _)).WillOnce(Return(false));
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->GetLastKeyInfo(keyCode, modifiers, ts));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_StartStopVncServer_Branches)
{
    EXPECT_CALL(compositorMock, startVncServer())
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    EXPECT_CALL(compositorMock, stopVncServer())
        .WillOnce(Return(true))
        .WillOnce(Return(false));

    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->StartVncServer());
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->StartVncServer());
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->StopVncServer());
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->StopVncServer());
}

TEST_F(RDKWindowManagerImplementationTest, Impl_RegisterUnregisterAndDispatch_AllEventFanout)
{
    NiceMock<WindowManagerNotificationMock> n1;
    NiceMock<WindowManagerNotificationMock> n2;

    EXPECT_CALL(n1, AddRef()).Times(1);
    EXPECT_CALL(n2, AddRef()).Times(1);
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->Register(&n1));
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->Register(&n2));
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->Register(&n1));

    EXPECT_CALL(n1, OnUserInactivity(2.5)).Times(1);
    EXPECT_CALL(n2, OnUserInactivity(2.5)).Times(1);
    windowManagerImplementation->Dispatch(Plugin::RDKWindowManagerImplementation::Event::RDK_WINDOW_MANAGER_EVENT_ON_USER_INACTIVITY, JsonValue(2.5));

    EXPECT_CALL(n1, OnDisconnected("app")).Times(1);
    EXPECT_CALL(n2, OnDisconnected("app")).Times(1);
    windowManagerImplementation->Dispatch(Plugin::RDKWindowManagerImplementation::Event::RDK_WINDOW_MANAGER_EVENT_APPLICATION_DISCONNECTED, JsonValue("app"));

    EXPECT_CALL(n1, OnReady("app")).Times(1);
    EXPECT_CALL(n2, OnReady("app")).Times(1);
    windowManagerImplementation->Dispatch(Plugin::RDKWindowManagerImplementation::Event::RDK_WINDOW_MANAGER_EVENT_ON_READY, JsonValue("app"));

    EXPECT_CALL(n1, OnConnected("app")).Times(1);
    EXPECT_CALL(n2, OnConnected("app")).Times(1);
    windowManagerImplementation->Dispatch(Plugin::RDKWindowManagerImplementation::Event::RDK_WINDOW_MANAGER_EVENT_APPLICATION_CONNECTED, JsonValue("app"));

    EXPECT_CALL(n1, OnVisible("app")).Times(1);
    EXPECT_CALL(n2, OnVisible("app")).Times(1);
    windowManagerImplementation->Dispatch(Plugin::RDKWindowManagerImplementation::Event::RDK_WINDOW_MANAGER_EVENT_APPLICATION_VISIBLE, JsonValue("app"));

    EXPECT_CALL(n1, OnHidden("app")).Times(1);
    EXPECT_CALL(n2, OnHidden("app")).Times(1);
    windowManagerImplementation->Dispatch(Plugin::RDKWindowManagerImplementation::Event::RDK_WINDOW_MANAGER_EVENT_APPLICATION_HIDDEN, JsonValue("app"));

    EXPECT_CALL(n1, OnFocus("app")).Times(1);
    EXPECT_CALL(n2, OnFocus("app")).Times(1);
    windowManagerImplementation->Dispatch(Plugin::RDKWindowManagerImplementation::Event::RDK_WINDOW_MANAGER_EVENT_APPLICATION_FOCUS, JsonValue("app"));

    EXPECT_CALL(n1, OnBlur("app")).Times(1);
    EXPECT_CALL(n2, OnBlur("app")).Times(1);
    windowManagerImplementation->Dispatch(Plugin::RDKWindowManagerImplementation::Event::RDK_WINDOW_MANAGER_EVENT_APPLICATION_BLUR, JsonValue("app"));

    EXPECT_CALL(n1, OnScreenshotComplete(true, _)).Times(1);
    EXPECT_CALL(n2, OnScreenshotComplete(true, _)).Times(1);
    windowManagerImplementation->Dispatch(Plugin::RDKWindowManagerImplementation::Event::RDK_WINDOW_MANAGER_EVENT_SCREENSHOT_COMPLETE, JsonValue(true));

    EXPECT_CALL(n1, Release()).Times(1).WillOnce(Return(0u));
    EXPECT_CALL(n2, Release()).Times(1).WillOnce(Return(0u));
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->Unregister(&n1));
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->Unregister(&n2));
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->Unregister(&n2));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_ListenerCallbacks_DispatchAsynchronously)
{
    NiceMock<WindowManagerNotificationMock> notification;
    EXPECT_CALL(notification, AddRef()).Times(1);
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->Register(&notification));

    std::promise<void> inactivePromise;
    auto inactiveFuture = inactivePromise.get_future();
    EXPECT_CALL(notification, OnUserInactivity(1.0))
        .WillOnce(Invoke([&](double) { inactivePromise.set_value(); }));

    Plugin::RDKWindowManagerImplementation::RdkWindowManagerListener listener(windowManagerImplementation.operator->());
    listener.onUserInactive(1.0);
    EXPECT_EQ(std::future_status::ready, inactiveFuture.wait_for(std::chrono::milliseconds(500)));

    std::promise<void> readyPromise;
    auto readyFuture = readyPromise.get_future();
    EXPECT_CALL(notification, OnReady("clientA"))
        .WillOnce(Invoke([&](const std::string&) { readyPromise.set_value(); }));
    listener.onReady("clientA");
    EXPECT_EQ(std::future_status::ready, readyFuture.wait_for(std::chrono::milliseconds(500)));

    EXPECT_CALL(notification, Release()).Times(1).WillOnce(Return(0u));
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->Unregister(&notification));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_ListenerCallbacks_ApplicationEvents)
{
    NiceMock<WindowManagerNotificationMock> notification;
    EXPECT_CALL(notification, AddRef()).Times(1);
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->Register(&notification));

    Plugin::RDKWindowManagerImplementation::RdkWindowManagerListener listener(windowManagerImplementation.operator->());

    std::promise<void> disconnectedPromise;
    auto disconnectedFuture = disconnectedPromise.get_future();
    EXPECT_CALL(notification, OnDisconnected("client1"))
        .WillOnce(Invoke([&](const std::string&) { disconnectedPromise.set_value(); }));
    listener.onApplicationDisconnected("client1");
    EXPECT_EQ(std::future_status::ready, disconnectedFuture.wait_for(std::chrono::milliseconds(500)));

    std::promise<void> connectedPromise;
    auto connectedFuture = connectedPromise.get_future();
    EXPECT_CALL(notification, OnConnected("client2"))
        .WillOnce(Invoke([&](const std::string&) { connectedPromise.set_value(); }));
    listener.onApplicationConnected("client2");
    EXPECT_EQ(std::future_status::ready, connectedFuture.wait_for(std::chrono::milliseconds(500)));

    std::promise<void> visiblePromise;
    auto visibleFuture = visiblePromise.get_future();
    EXPECT_CALL(notification, OnVisible("client3"))
        .WillOnce(Invoke([&](const std::string&) { visiblePromise.set_value(); }));
    listener.onApplicationVisible("client3");
    EXPECT_EQ(std::future_status::ready, visibleFuture.wait_for(std::chrono::milliseconds(500)));

    std::promise<void> hiddenPromise;
    auto hiddenFuture = hiddenPromise.get_future();
    EXPECT_CALL(notification, OnHidden("client4"))
        .WillOnce(Invoke([&](const std::string&) { hiddenPromise.set_value(); }));
    listener.onApplicationHidden("client4");
    EXPECT_EQ(std::future_status::ready, hiddenFuture.wait_for(std::chrono::milliseconds(500)));

    std::promise<void> focusPromise;
    auto focusFuture = focusPromise.get_future();
    EXPECT_CALL(notification, OnFocus("client5"))
        .WillOnce(Invoke([&](const std::string&) { focusPromise.set_value(); }));
    listener.onApplicationFocus("client5");
    EXPECT_EQ(std::future_status::ready, focusFuture.wait_for(std::chrono::milliseconds(500)));

    std::promise<void> blurPromise;
    auto blurFuture = blurPromise.get_future();
    EXPECT_CALL(notification, OnBlur("client6"))
        .WillOnce(Invoke([&](const std::string&) { blurPromise.set_value(); }));
    listener.onApplicationBlur("client6");
    EXPECT_EQ(std::future_status::ready, blurFuture.wait_for(std::chrono::milliseconds(500)));

    EXPECT_CALL(notification, Release()).Times(1).WillOnce(Return(0u));
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->Unregister(&notification));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_AddKeyIntercept_UsesModifierFlags)
{
    const string intercept = R"({"client":"testApp","keyCode":65,"modifiers":["ctrl","shift","alt"],"focusOnly":true,"propagate":true})";
    EXPECT_CALL(compositorMock, addKeyIntercept("testApp", 65u, 7u, true, true))
        .WillOnce(Return(true));

    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->AddKeyIntercept(intercept));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_AddKeyIntercept_MissingClient_Failure)
{
    const string intercept = R"({"keyCode":65})";
    EXPECT_CALL(compositorMock, addKeyIntercept(_, _, _, _, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->AddKeyIntercept(intercept));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_AddKeyIntercept_EmptyPayload_Failure)
{
    EXPECT_CALL(compositorMock, addKeyIntercept(_, _, _, _, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->AddKeyIntercept(""));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_AddKeyIntercept_MissingKeyCode_Failure)
{
    const string intercept = R"({"client":"testApp"})";
    EXPECT_CALL(compositorMock, addKeyIntercept(_, _, _, _, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->AddKeyIntercept(intercept));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_AddKeyIntercept_UsesCallsign_Success)
{
    const string intercept = R"({"callsign":"testApp","keyCode":66,"modifiers":["ctrl"]})";
    EXPECT_CALL(compositorMock, addKeyIntercept("testApp", 66u, 1u, false, false))
        .WillOnce(Return(true));

    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->AddKeyIntercept(intercept));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_AddKeyIntercepts_MixedEntries)
{
    const string intercepts = R"([{"keyCode":65,"modifiers":["ctrl"]},"bad",{"keyCode":66,"focusOnly":true,"propagate":false}])";
    EXPECT_CALL(compositorMock, addKeyIntercept("testApp", 65u, 1u, false, false)).WillOnce(Return(true));
    EXPECT_CALL(compositorMock, addKeyIntercept("testApp", 66u, 0u, true, false)).WillOnce(Return(true));

    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->AddKeyIntercepts("testApp", intercepts));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_AddKeyIntercepts_EmptyClientId_Failure)
{
    EXPECT_CALL(compositorMock, addKeyIntercept(_, _, _, _, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL,
        windowManagerImplementation->AddKeyIntercepts("", R"([{"keyCode":65}])"));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_AddKeyIntercepts_EmptyIntercepts_Failure)
{
    EXPECT_CALL(compositorMock, addKeyIntercept(_, _, _, _, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->AddKeyIntercepts("testApp", ""));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_AddKeyIntercepts_InvalidJson_Failure)
{
    EXPECT_CALL(compositorMock, addKeyIntercept(_, _, _, _, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->AddKeyIntercepts("testApp", "not-json"));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_AddKeyIntercepts_AllInvalidEntries_Failure)
{
    EXPECT_CALL(compositorMock, addKeyIntercept(_, _, _, _, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL,
        windowManagerImplementation->AddKeyIntercepts("testApp", R"(["bad",{"modifiers":["ctrl"]}])"));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_RemoveKeyIntercept_InvalidModifierJson)
{
    EXPECT_CALL(compositorMock, removeKeyIntercept(_, _, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->RemoveKeyIntercept("testApp", 65u, "not-json"));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_RemoveKeyIntercept_EmptyClient_Failure)
{
    EXPECT_CALL(compositorMock, removeKeyIntercept(_, _, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->RemoveKeyIntercept("", 65u, "[]"));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_RemoveKeyIntercept_UsesModifierFlags)
{
    EXPECT_CALL(compositorMock, removeKeyIntercept("testApp", 65u, 3u)).WillOnce(Return(true));
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->RemoveKeyIntercept("testApp", 65u, "[\"ctrl\",\"shift\"]"));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_AddKeyListener_NativeWildcard_Success)
{
    const string listeners = R"({"client":"testApp","keys":[{"nativeKeyCode":"*","modifiers":["alt"],"activate":true,"propagate":false}]})";
    EXPECT_CALL(compositorMock, addNativeKeyListener("testApp", 65536u, 4u, _)).WillOnce(Return(true));

    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->AddKeyListener(listeners));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_AddKeyListener_KeyAndNativePresent_Failure)
{
    const string listeners = R"({"client":"testApp","keys":[{"keyCode":10,"nativeKeyCode":20}]})";
    EXPECT_CALL(compositorMock, addKeyListener(_, _, _, _)).Times(0);
    EXPECT_CALL(compositorMock, addNativeKeyListener(_, _, _, _)).Times(0);

    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->AddKeyListener(listeners));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_AddKeyListener_EmptyPayload_Failure)
{
    EXPECT_CALL(compositorMock, addKeyListener(_, _, _, _)).Times(0);
    EXPECT_CALL(compositorMock, addNativeKeyListener(_, _, _, _)).Times(0);

    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->AddKeyListener(""));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_AddKeyListener_MissingKeys_Failure)
{
    EXPECT_CALL(compositorMock, addKeyListener(_, _, _, _)).Times(0);
    EXPECT_CALL(compositorMock, addNativeKeyListener(_, _, _, _)).Times(0);

    EXPECT_EQ(Core::ERROR_GENERAL,
        windowManagerImplementation->AddKeyListener(R"({"client":"testApp"})"));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_AddKeyListener_MissingClientAndCallsign_Failure)
{
    EXPECT_CALL(compositorMock, addKeyListener(_, _, _, _)).Times(0);
    EXPECT_CALL(compositorMock, addNativeKeyListener(_, _, _, _)).Times(0);

    EXPECT_EQ(Core::ERROR_GENERAL,
        windowManagerImplementation->AddKeyListener(R"({"keys":[{"keyCode":65}]})"));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_AddKeyListener_UsesCallsign_Success)
{
    const string listeners = R"({"callsign":"testApp","keys":[{"keyCode":"*","modifiers":["shift"]}]})";
    EXPECT_CALL(compositorMock, addKeyListener("testApp", 65536u, 2u, _)).WillOnce(Return(true));

    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->AddKeyListener(listeners));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_RemoveKeyListener_NativeWildcard_Success)
{
    const string listeners = R"({"client":"testApp","keys":[{"nativeKeyCode":"*","modifiers":["ctrl"]}]})";
    EXPECT_CALL(compositorMock, removeNativeKeyListener("testApp", 65536u, 1u)).WillOnce(Return(true));

    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->RemoveKeyListener(listeners));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_RemoveKeyListener_EmptyPayload_Failure)
{
    EXPECT_CALL(compositorMock, removeKeyListener(_, _, _)).Times(0);
    EXPECT_CALL(compositorMock, removeNativeKeyListener(_, _, _)).Times(0);

    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->RemoveKeyListener(""));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_RemoveKeyListener_MissingKeys_Failure)
{
    EXPECT_CALL(compositorMock, removeKeyListener(_, _, _)).Times(0);
    EXPECT_CALL(compositorMock, removeNativeKeyListener(_, _, _)).Times(0);

    EXPECT_EQ(Core::ERROR_GENERAL,
        windowManagerImplementation->RemoveKeyListener(R"({"client":"testApp"})"));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_RemoveKeyListener_MissingClientAndCallsign_Failure)
{
    EXPECT_CALL(compositorMock, removeKeyListener(_, _, _)).Times(0);
    EXPECT_CALL(compositorMock, removeNativeKeyListener(_, _, _)).Times(0);

    EXPECT_EQ(Core::ERROR_GENERAL,
        windowManagerImplementation->RemoveKeyListener(R"({"keys":[{"nativeKeyCode":65}]})"));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_RemoveKeyListener_UsesCallsign_Success)
{
    const string listeners = R"({"callsign":"testApp","keys":[{"nativeKeyCode":99,"modifiers":["alt"]}]})";
    EXPECT_CALL(compositorMock, removeNativeKeyListener("testApp", 99u, 4u)).WillOnce(Return(true));

    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->RemoveKeyListener(listeners));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_InjectKey_UsesModifierFlags)
{
    EXPECT_CALL(compositorMock, injectKey(100u, 5u)).WillOnce(Return(true));
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->InjectKey(100u, R"({"modifiers":["ctrl","alt"]})"));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_GenerateKey_VirtualAndClientLookup)
{
    const string keys = R"({"keys":[{"key":"A","delay":0,"client":"TestApp"}]})";
    EXPECT_CALL(compositorMock, getClients(_))
        .WillOnce(Invoke([](std::vector<std::string>& clients) {
            clients = {"testapp"};
            return true;
        }));
    EXPECT_CALL(compositorMock, generateKey("testapp", 0u, 0u, "A"))
        .WillOnce(Return(true));

    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->GenerateKey(keys, ""));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_GenerateKey_TargetClientMissing_Failure)
{
    const string keys = R"({"keys":[{"keyCode":65,"modifiers":["shift"],"delay":0,"client":"unknown"}]})";
    EXPECT_CALL(compositorMock, getClients(_))
        .WillOnce(Invoke([](std::vector<std::string>& clients) {
            clients = {"testapp"};
            return true;
        }));
    EXPECT_CALL(compositorMock, generateKey(_, _, _, _)).Times(0);

    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->GenerateKey(keys, ""));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_GenerateKey_EmptyPayload_Failure)
{
    EXPECT_CALL(compositorMock, generateKey(_, _, _, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->GenerateKey("", "testApp"));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_GenerateKey_MissingKeysField_Failure)
{
    EXPECT_CALL(compositorMock, generateKey(_, _, _, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->GenerateKey(R"({"client":"testApp"})", ""));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_KeyRepeatConfig_MissingEnabled_Failure)
{
    EXPECT_CALL(compositorMock, setKeyRepeatConfig(_, _, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->KeyRepeatConfig("default", R"({"initialDelay":10,"repeatInterval":20})"));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_KeyRepeatConfig_MissingInitialDelay_Failure)
{
    EXPECT_CALL(compositorMock, setKeyRepeatConfig(_, _, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL,
        windowManagerImplementation->KeyRepeatConfig("default", R"({"enabled":true,"repeatInterval":20})"));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_KeyRepeatConfig_MissingRepeatInterval_Failure)
{
    EXPECT_CALL(compositorMock, setKeyRepeatConfig(_, _, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL,
        windowManagerImplementation->KeyRepeatConfig("default", R"({"enabled":true,"initialDelay":20})"));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_SetVisible_EmptyClient_Failure)
{
    EXPECT_CALL(compositorMock, setVisibility(_, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->SetVisible("", true));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_SetFocus_Failure)
{
    EXPECT_CALL(compositorMock, setFocus(TEST_CLIENT_ID)).WillOnce(Return(false));
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->SetFocus(TEST_CLIENT_ID));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_SetVisible_Failure)
{
    EXPECT_CALL(compositorMock, setVisibility(TEST_CLIENT_ID, true)).WillOnce(Return(false));
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->SetVisible(TEST_CLIENT_ID, true));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_GetVisibility_Failure)
{
    bool visible = false;
    EXPECT_CALL(compositorMock, getVisibility(TEST_CLIENT_ID, _)).WillOnce(Return(false));
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->GetVisibility(TEST_CLIENT_ID, visible));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_EnableDisplayRender_EmptyClient_Failure)
{
    EXPECT_CALL(compositorMock, enableDisplayRender(_, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->EnableDisplayRender("", true));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_EnableDisplayRender_Failure)
{
    EXPECT_CALL(compositorMock, enableDisplayRender("testApp", true)).WillOnce(Return(false));
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->EnableDisplayRender("testApp", true));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_SetZOrder_EmptyClient_Failure)
{
    EXPECT_CALL(compositorMock, setZorder(_, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->SetZOrder("", 1));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_SetZOrder_Failure)
{
    EXPECT_CALL(compositorMock, setZorder(TEST_APP_INSTANCE_ID, 9)).WillOnce(Return(false));
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->SetZOrder(TEST_APP_INSTANCE_ID, 9));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_GetZOrder_EmptyClient_Failure)
{
    int32_t zOrder = 0;
    EXPECT_CALL(compositorMock, getZOrder(_, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->GetZOrder("", zOrder));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_GetZOrder_Failure)
{
    int32_t zOrder = 0;
    EXPECT_CALL(compositorMock, getZOrder(TEST_APP_INSTANCE_ID, _)).WillOnce(Return(false));
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->GetZOrder(TEST_APP_INSTANCE_ID, zOrder));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_GetScreenshot_QueuesRequest)
{
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->GetScreenshot());
}

TEST_F(RDKWindowManagerImplementationTest, Impl_EnableInactivityReporting_CallsCompositor)
{
    EXPECT_CALL(compositorMock, enableInactivityReporting(true)).Times(1);
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->EnableInactivityReporting(true));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_SetInactivityInterval_CallsCompositor)
{
    EXPECT_CALL(compositorMock, setInactivityInterval(30.0)).Times(1);
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->SetInactivityInterval(30u));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_ResetInactivityTime_CallsCompositor)
{
    EXPECT_CALL(compositorMock, resetInactivityTime()).Times(1);
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->ResetInactivityTime());
}

TEST_F(RDKWindowManagerImplementationTest, Impl_EnableKeyRepeats_Branches)
{
    EXPECT_CALL(compositorMock, enableKeyRepeats(true)).WillOnce(Return(true));
    EXPECT_CALL(compositorMock, enableKeyRepeats(false)).WillOnce(Return(false));
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->EnableKeyRepeats(true));
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->EnableKeyRepeats(false));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_GetKeyRepeatsEnabled_Branches)
{
    bool enabled = false;
    EXPECT_CALL(compositorMock, getKeyRepeatsEnabled(_))
        .WillOnce(Invoke([](bool& outEnabled) {
            outEnabled = true;
            return true;
        }))
        .WillOnce(Return(false));
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->GetKeyRepeatsEnabled(enabled));
    EXPECT_TRUE(enabled);
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->GetKeyRepeatsEnabled(enabled));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_IgnoreKeyInputs_Branches)
{
    EXPECT_CALL(compositorMock, ignoreKeyInputs(true)).WillOnce(Return(true));
    EXPECT_CALL(compositorMock, ignoreKeyInputs(false)).WillOnce(Return(false));
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->IgnoreKeyInputs(true));
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->IgnoreKeyInputs(false));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_EnableInputEvents_EmptyClients_Failure)
{
    EXPECT_CALL(compositorMock, enableInputEvents(_, _)).Times(0);
    EXPECT_EQ(Core::ERROR_GENERAL, windowManagerImplementation->EnableInputEvents("", true));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_EnableInputEvents_InvalidEntryType_Failure)
{
    EXPECT_CALL(compositorMock, enableInputEvents("testApp", true)).WillOnce(Return(true));
    EXPECT_EQ(Core::ERROR_GENERAL,
        windowManagerImplementation->EnableInputEvents(R"({"clients":["testApp",5]})", true));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_Dispatch_UnknownEvent_NoCrash)
{
    windowManagerImplementation->Dispatch(
        static_cast<Plugin::RDKWindowManagerImplementation::Event>(999u), JsonValue("ignored"));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_ListenerCallbacks_NullImplementation_NoCrash)
{
    Plugin::RDKWindowManagerImplementation::RdkWindowManagerListener listener(nullptr);

    listener.onUserInactive(1.0);
    listener.onApplicationDisconnected("client1");
    listener.onReady("client2");
    listener.onApplicationConnected("client3");
    listener.onApplicationVisible("client4");
    listener.onApplicationHidden("client5");
    listener.onApplicationFocus("client6");
    listener.onApplicationBlur("client7");
}

TEST_F(RDKWindowManagerImplementationTest, Impl_InitializeAndDeinitialize_CoversLifecycle)
{
    setenv("WAYLAND_DISPLAY", "wayland-0", 1);

    EXPECT_CALL(serviceMock, AddRef()).Times(1);
    EXPECT_CALL(serviceMock, Release()).Times(1).WillOnce(Return(1u));

    EXPECT_CALL(serviceMock, SubSystems())
        .WillOnce(Return(&subSystemMock));
    EXPECT_CALL(subSystemMock, Set(PluginHost::ISubSystem::PLATFORM, nullptr)).Times(1);
    EXPECT_CALL(subSystemMock, Set(PluginHost::ISubSystem::GRAPHICS, nullptr)).Times(1);
    EXPECT_CALL(subSystemMock, Release()).Times(1).WillOnce(Return(1u));

    EXPECT_CALL(compositorMock, setEventListener(NotNull())).Times(1);
    EXPECT_CALL(compositorMock, setEventListener(IsNull())).Times(1);
    EXPECT_CALL(compositorMock, enableInactivityReporting(true)).Times(1);

    EXPECT_CALL(rdkWindowManagerMock, initialize()).Times(1);
    EXPECT_CALL(rdkWindowManagerMock, draw()).Times(AtLeast(0));
    EXPECT_CALL(rdkWindowManagerMock, update()).Times(AtLeast(0));
    EXPECT_CALL(rdkWindowManagerMock, deinitialize()).Times(1);

    EXPECT_CALL(compositorMock, getClients(_))
        .WillRepeatedly(Invoke([](std::vector<std::string>& clients) {
            clients.clear();
            return true;
        }));

    EXPECT_CALL(compositorMock, createDisplay("testApp", "disp", 1920u, 1080u, false, 0u, 0u, false, false, 0, 0))
        .WillOnce(Return(true));
    EXPECT_CALL(compositorMock, addListener("testApp", NotNull()))
        .WillOnce(Return(true));

    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->Initialize(&serviceMock));
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->CreateDisplay("testApp", "disp", 1920, 1080, false, 0, 0, 0, 0, false, false));
    EXPECT_EQ(Core::ERROR_NONE, windowManagerImplementation->Deinitialize(&serviceMock));
}

TEST_F(RDKWindowManagerImplementationTest, Impl_ToLower_HelperCoverage)
{
    EXPECT_EQ(std::string("myapp"), WPEFramework::Plugin::toLower("MyApp"));
}

TEST_F(RDKWindowManagerTest, Plugin_Information_ReturnsServiceName)
{
    EXPECT_EQ(string("{\"service\": \"org.rdk.RDKWindowManager\"}"), plugin->Information());
}
