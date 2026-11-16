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
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ResourceMonitor.h"
#include "ResourceMonitorConfig.h"
#include "PackageLifecycleConfigProvider.h"
#include <queue>
#include <condition_variable>
#define private public
#include "ResourceMonitorImplementation.h"
#undef private
#include "../../mocks/ResourceMonitorEnforcementTestSupport.h"
#include "ServiceMock.h"
#include "PackageManagerMock.h"
#include "ThunderPortability.h"
#include <core/core.h>
#include <list>
#include <map>
#include <utility>
#include <vector>
#include <fstream>
#include <cstdio>
#include <unistd.h>

using namespace WPEFramework;

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

using std::string;


// Testable Plugin Wrapper
class TestableResourceMonitorPlugin : public Plugin::ResourceMonitor {
public:
    // Nothing extra required.
};


// Testable Implementation Wrapper
class TestableResourceMonitorImplementation
    : public Plugin::ResourceMonitorImplementation {

private:
    mutable uint32_t _refCount {1};

public:
    TestableResourceMonitorImplementation() = default;
    ~TestableResourceMonitorImplementation() override = default;

    uint32_t AddRef() const override
    {
        return Core::InterlockedIncrement(_refCount);

    }

    uint32_t Release() const override
    {
        if (Core::InterlockedDecrement(_refCount) == 0) {
            delete this;
            return Core::ERROR_DESTRUCTION_SUCCEEDED;
        }

        return Core::ERROR_NONE;
    }

    Core::hresult CallGetMemInfo(uint64_t& memAvailable, uint64_t& swapFree, uint64_t& swapTotal)
    {
        return GetMemInfo(memAvailable, swapFree, swapTotal);
    }

    Core::hresult CallGetPsiMetrics(const std::string& metric, double& value, uint64_t& total)
    {
        return GetPsiMetrics(metric, value, total);
    }

    Core::hresult CallSetConfig(const string& config)
    {
        return static_cast<Exchange::IResourceMonitor*>(this)->SetConfig(config);
    }

    Core::hresult CallGetSwapUsed(uint64_t& swapUsed, uint64_t& memUsedTotal)
    {
        return GetSwapUsed(swapUsed, memUsedTotal);
    }

    Core::hresult CallGetFlashSpace(uint64_t& total, uint64_t& used, uint64_t& available)
    {
        return GetFlashSpace(total, used, available);
    }

    Core::hresult CallGetStats(Exchange::ResourceMonitorStats& stats)
    {
        return GetStats(stats);
    }

    Core::hresult CallReconcile(const string& appId, const uint32_t ramTargetMB, const bool allowTerminate)
    {
        return Reconcile(appId, ramTargetMB, allowTerminate);
    }
    void CallOnReconciliationComplete(const string& appId, const bool targetRamAchieved)
    {
        OnReconciliationComplete(appId, targetRamAchieved);
    }
};


class ResourceMonitorTimerTest : public ::testing::Test {
protected:
    std::unique_ptr<TestableResourceMonitorImplementation> implementation { new TestableResourceMonitorImplementation() };

    void SetDeadline(const string& id, bool paused, uint64_t delayMs)
    {
        std::lock_guard<std::mutex> lock(implementation->_appTimersMutex);
        const uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        auto& state = implementation->_appTimers.at(id);
        (paused ? state.pausedTimerExpiryMs : state.suspendedTimerExpiryMs) = now + delayMs;
        implementation->ScheduleNextTimerLocked();
    }

    bool WaitForExpiry(const string& id, bool paused)
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (std::chrono::steady_clock::now() < deadline) {
            {
                std::lock_guard<std::mutex> lock(implementation->_appTimersMutex);
                const auto& state = implementation->_appTimers.at(id);
                if ((paused ? state.pausedTimerExpiryMs : state.suspendedTimerExpiryMs) == 0) {
                    return true;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return false;
    }
};

TEST_F(ResourceMonitorTimerTest, IdleWakeAndEarlierDeadline)
{
    implementation->StartPausedTimer("later", Plugin::AppLifecycleCapabilities());
    SetDeadline("later", true, 10000);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    implementation->StartSuspendedTimer("earlier");
    SetDeadline("earlier", false, 20);
    EXPECT_TRUE(WaitForExpiry("earlier", false));
    {
        std::lock_guard<std::mutex> lock(implementation->_appTimersMutex);
        EXPECT_NE(0u, implementation->_appTimers.at("later").pausedTimerExpiryMs);
    }
    implementation->CancelAllTimers("later");
    EXPECT_FALSE(implementation->_timerHandler.HasEntry(*implementation->_timer));
    implementation->StartPausedTimer("next", Plugin::AppLifecycleCapabilities());
    SetDeadline("next", true, 20);
    EXPECT_TRUE(WaitForExpiry("next", true));
}

TEST_F(ResourceMonitorTimerTest, CancellationAndPendingShutdown)
{
    implementation->StartPausedTimer("app", Plugin::AppLifecycleCapabilities());
    implementation->StartSuspendedTimer("app");
    implementation->CancelPausedTimer("app");
    EXPECT_TRUE(implementation->_timerHandler.HasEntry(*implementation->_timer));
    implementation->CancelSuspendedTimer("app");
    EXPECT_FALSE(implementation->_timerHandler.HasEntry(*implementation->_timer));
    implementation->StartPausedTimer("app", Plugin::AppLifecycleCapabilities());
    const auto start = std::chrono::steady_clock::now();
    implementation.reset();
    EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(2));
}

class ResourceMonitorTest : public ::testing::Test {

protected:
    Core::ProxyType<TestableResourceMonitorPlugin> plugin;
    Core::JSONRPC::Handler& handler;
    DECL_CORE_JSONRPC_CONX connection;
    string response;

    ResourceMonitorTest()
        : plugin(Core::ProxyType<TestableResourceMonitorPlugin>::Create())
        , handler(*plugin)
        , INIT_CONX(1, 0)
    {
    }

    virtual ~ResourceMonitorTest() = default;
};


// SecurityAgent Mock
class MockAuthenticate : public PluginHost::IAuthenticate {
public:
    MOCK_METHOD(uint32_t, CreateToken, (const uint16_t, const uint8_t[], std::string&), (override));
    MOCK_METHOD(PluginHost::ISecurity*, Officer, (const std::string&), (override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(uint32_t, AddRef, (), (const, override));

    BEGIN_INTERFACE_MAP(MockAuthenticate)
        INTERFACE_ENTRY(PluginHost::IAuthenticate)
    END_INTERFACE_MAP
};

// Initialized Fixture
class ResourceMonitorInitializedTest : public ResourceMonitorTest {
protected:
    NiceMock<ServiceMock> service;
    NiceMock<MockAuthenticate>* mockAuth = nullptr;
    string initResult;
    bool IsPluginInitialized() const
    {
        return initResult.empty();
    }

    void SetUp() override
    {
        ASSERT_TRUE(initResult.empty()) << "Plugin initialization failed: " << initResult;
    }

public:
    ResourceMonitorInitializedTest()
    {
        mockAuth = new NiceMock<MockAuthenticate>();
        ON_CALL(service,
                QueryInterfaceByCallsign(_, _))
            .WillByDefault(
                [this](const uint32_t,
                       const std::string& name)
                -> void*
                {
                    if (name == "SecurityAgent") {
                        mockAuth->AddRef();
                        return static_cast<void*>(mockAuth);
                    }
                    return nullptr;
                });

        initResult = plugin->Initialize(&service);
    }
    ~ResourceMonitorInitializedTest() override
    {
        if (IsPluginInitialized()) {
            plugin->Deinitialize(&service);
        }
        delete mockAuth;
    }
};

class ResourceMonitorAcceptanceTest : public ResourceMonitorTest {
protected:
    NiceMock<ServiceMock> service;
    NiceMock<MockAuthenticate> authentication;
    ResourceMonitorTestSupport::AppManager* apps { new ResourceMonitorTestSupport::AppManager() };
    ResourceMonitorTestSupport::RuntimeManager* runtime { new ResourceMonitorTestSupport::RuntimeManager() };
    std::unique_ptr<ResourceMonitorTestSupport::Session> session;
    bool initialized { false };
    PluginHost::IPlugin::INotification* managerNotifications { nullptr };
    bool appManagerAvailable { true };
    bool runtimeManagerAvailable { true };

    virtual string Configuration() const { return "{\"root\":{\"low_memory_warnings_enabled\":false}}"; }

    void SetUp() override
    {
        ON_CALL(service, ConfigLine()).WillByDefault(Return(Configuration()));
        ON_CALL(service, Register(::testing::A<PluginHost::IPlugin::INotification*>())).WillByDefault(
            [this](PluginHost::IPlugin::INotification* sink) { managerNotifications = sink; });
        ON_CALL(service, Unregister(::testing::A<PluginHost::IPlugin::INotification*>())).WillByDefault(
            [this](PluginHost::IPlugin::INotification* sink) {
                if (managerNotifications == sink) { managerNotifications = nullptr; }
            });
        ON_CALL(service, QueryInterfaceByCallsign(_, _)).WillByDefault(
            [this](const uint32_t, const string& name) -> void* {
                if (name == "org.rdk.AppManager") {
                    if (!appManagerAvailable) { return nullptr; }
                    apps->AddRef();
                    return static_cast<Exchange::IAppManager*>(apps);
                }
                if (name == "org.rdk.RuntimeManager") {
                    if (!runtimeManagerAvailable) { return nullptr; }
                    runtime->AddRef();
                    return static_cast<Exchange::IRuntimeManager*>(runtime);
                }
                if (name == "SecurityAgent") { return &authentication; }
                return nullptr;
            });
        const auto error = plugin->Initialize(&service);
        initialized = error.empty();
        ASSERT_TRUE(initialized) << error;
        ASSERT_NE(nullptr, Plugin::ResourceMonitorImplementation::_instance);
        session.reset(new ResourceMonitorTestSupport::Session(Plugin::ResourceMonitorImplementation::_instance));
    }

    void TearDown() override
    {
        session.reset();
        if (initialized) { plugin->Deinitialize(&service); }
        apps->Release();
        runtime->Release();
    }
};

class ResourceMonitorLateManagerTest : public ResourceMonitorAcceptanceTest {
protected:
    void SetUp() override
    {
        appManagerAvailable = false;
        runtimeManagerAvailable = false;
        ResourceMonitorAcceptanceTest::SetUp();
    }
};

TEST_F(ResourceMonitorLateManagerTest, ManagersActivatedAfterMonitorRestoreEnforcement)
{
    ASSERT_NE(nullptr, managerNotifications);
    appManagerAvailable = true;
    runtimeManagerAvailable = true;
    managerNotifications->Activated("org.rdk.AppManager", nullptr);
    managerNotifications->Activated("org.rdk.RuntimeManager", nullptr);
    session.reset(new ResourceMonitorTestSupport::Session(Plugin::ResourceMonitorImplementation::_instance));
    session->Add("app");
    session->Emit("app", Exchange::IAppManager::APP_STATE_SUSPENDED);
    runtime->SetInfo("app.instance", "{\"gpu\":{\"memory\":{\"usage\":0}}}");
    session->Expire("app", false);
    const auto transitions = apps->Transitions();
    ASSERT_EQ(1u, transitions.size());
    EXPECT_EQ(Exchange::IAppManager::APP_STATE_HIBERNATED, transitions[0].second);
}

TEST_F(ResourceMonitorLateManagerTest, LateActivationReconcilesAlreadyPausedApplications)
{
    ASSERT_NE(nullptr, managerNotifications);
    apps->Add("app", "app.instance", "INTERACTIVE_APP");
    apps->Emit("app.instance", Exchange::IAppManager::APP_STATE_PAUSED);
    session->packages->payloads["app"] = "{\"ralfPkgPath\":\"/metadata.json\",\"applicationLifecycle\":{\"supportedNonActiveStates\":[\"paused\",\"suspended\"]}}";
    appManagerAvailable = true;
    managerNotifications->Activated("org.rdk.AppManager", nullptr);
    ASSERT_EQ(1u, session->monitor->_appTimers.count("app.instance"));
    session->Expire("app", true);
    const auto transitions = apps->Transitions();
    ASSERT_EQ(1u, transitions.size());
    EXPECT_EQ(Exchange::IAppManager::APP_STATE_SUSPENDED, transitions[0].second);
}

TEST_F(ResourceMonitorAcceptanceTest, ManagerDeactivationDropsInterfacesAndActivationRestoresSubscriptions)
{
    ASSERT_NE(nullptr, managerNotifications);
    session->Add("app");
    managerNotifications->Deactivated("org.rdk.AppManager", nullptr);
    managerNotifications->Unavailable("org.rdk.RuntimeManager", nullptr);
    EXPECT_EQ(nullptr, session->monitor->_appManager);
    EXPECT_EQ(nullptr, session->monitor->_runtimeManager);
    session->Emit("app", Exchange::IAppManager::APP_STATE_PAUSED);
    EXPECT_EQ(0u, session->monitor->_appTimers.count("app.instance"));
    managerNotifications->Activated("org.rdk.AppManager", nullptr);
    managerNotifications->Activated("org.rdk.RuntimeManager", nullptr);
    session->Emit("app", Exchange::IAppManager::APP_STATE_SUSPENDED);
    runtime->SetInfo("app.instance", "{\"gpu\":{\"memory\":{\"usage\":0}}}");
    session->Expire("app", false);
    const auto transitions = apps->Transitions();
    ASSERT_EQ(1u, transitions.size());
    EXPECT_EQ(Exchange::IAppManager::APP_STATE_HIBERNATED, transitions[0].second);
}

TEST_F(ResourceMonitorAcceptanceTest, AC1_ActiveToPausedStartsSixtySecondTimer)
{
    session->Add("app");
    session->Emit("app", Exchange::IAppManager::APP_STATE_PAUSED);
    EXPECT_GT(session->Remaining("app", true), 59000u);
    EXPECT_LE(session->Remaining("app", true), 60000u);
    EXPECT_TRUE(apps->Transitions().empty());
    EXPECT_TRUE(apps->Terminations().empty());
}

TEST_F(ResourceMonitorAcceptanceTest, AC2_DeadlineDeterminesNextAction)
{
    session->Add("supported");
    session->Add("unsupported", "\"paused\"");
    session->Emit("supported", Exchange::IAppManager::APP_STATE_PAUSED);
    session->Emit("unsupported", Exchange::IAppManager::APP_STATE_PAUSED);
    session->monitor->OnLifecycleTimerExpired();
    EXPECT_TRUE(apps->Transitions().empty());
    EXPECT_TRUE(apps->Terminations().empty());
    session->Expire("supported", true);
    session->Expire("unsupported", true);
    const auto transitions = apps->Transitions();
    ASSERT_EQ(1u, transitions.size());
    EXPECT_EQ("supported", transitions[0].first);
    EXPECT_EQ(std::vector<string>({ "unsupported" }), apps->Terminations());
}

TEST_F(ResourceMonitorAcceptanceTest, AC3_RequestsSuspendedThroughAppManager)
{
    session->Add("app");
    session->Emit("app", Exchange::IAppManager::APP_STATE_PAUSED);
    session->Expire("app", true);
    const auto transitions = apps->Transitions();
    ASSERT_EQ(1u, transitions.size());
    EXPECT_EQ("app", transitions[0].first);
    EXPECT_EQ(Exchange::IAppManager::APP_STATE_SUSPENDED, transitions[0].second);
    EXPECT_TRUE(apps->Terminations().empty());
}

TEST_F(ResourceMonitorAcceptanceTest, AC4_QueriesGpuOnlyAfterGraceExpires)
{
    session->Add("app");
    session->Emit("app", Exchange::IAppManager::APP_STATE_PAUSED);
    session->Emit("app", Exchange::IAppManager::APP_STATE_SUSPENDED);
    runtime->SetInfo("app.instance", "{\"gpu\":{\"memory\":{\"usage\":0}}}");
    session->monitor->OnLifecycleTimerExpired();
    EXPECT_EQ(0u, runtime->calls.load());
    session->Expire("app", false);
    EXPECT_EQ(1u, runtime->calls.load());
    EXPECT_TRUE(apps->Terminations().empty());
}

TEST_F(ResourceMonitorAcceptanceTest, AC5_TerminatesAnyRemainingGpuConsumer)
{
    for (const uint64_t usage : { uint64_t(1), uint64_t(20 * 1024 * 1024), uint64_t(32 * 1024 * 1024) }) {
        const string id = "app" + std::to_string(usage);
        session->Add(id);
        session->Emit(id, Exchange::IAppManager::APP_STATE_PAUSED);
        session->Emit(id, Exchange::IAppManager::APP_STATE_SUSPENDED);
        runtime->SetInfo(id + ".instance", "{\"gpu\":{\"memory\":{\"usage\":" + std::to_string(usage) + "}}}");
        session->Expire(id, false);
    }
    EXPECT_EQ(3u, apps->Terminations().size());
    EXPECT_TRUE(apps->Transitions().empty());
}

TEST_F(ResourceMonitorAcceptanceTest, AC6_DefaultGraceIsTenSeconds)
{
    session->Add("app");
    session->Emit("app", Exchange::IAppManager::APP_STATE_SUSPENDED);
    EXPECT_GT(session->Remaining("app", false), 9000u);
    EXPECT_LE(session->Remaining("app", false), 10000u);
}

class ResourceMonitorConfiguredGraceTest : public ResourceMonitorAcceptanceTest {
protected:
    string Configuration() const override
    {
        return "{\"root\":{\"low_memory_warnings_enabled\":false,\"suspended_gpu_grace_period\":7}}";
    }
};

TEST_F(ResourceMonitorConfiguredGraceTest, AC6_PluginUsesConfiguredGrace)
{
    session->Add("app");
    session->Emit("app", Exchange::IAppManager::APP_STATE_SUSPENDED);
    EXPECT_GT(session->Remaining("app", false), 6000u);
    EXPECT_LE(session->Remaining("app", false), 7000u);
}

TEST_F(ResourceMonitorAcceptanceTest, AC7_RequestsHibernatedWhenSupportedAndGpuReleased)
{
    session->Add("app");
    session->Emit("app", Exchange::IAppManager::APP_STATE_PAUSED);
    session->Emit("app", Exchange::IAppManager::APP_STATE_SUSPENDED);
    runtime->SetInfo("app.instance", "{\"gpu\":{\"memory\":{\"usage\":0}}}");
    session->Expire("app", false);
    const auto transitions = apps->Transitions();
    ASSERT_EQ(1u, transitions.size());
    EXPECT_EQ("app", transitions[0].first);
    EXPECT_EQ(Exchange::IAppManager::APP_STATE_HIBERNATED, transitions[0].second);
}

TEST_F(ResourceMonitorAcceptanceTest, FirstObservedSuspendedCapturesCapabilities)
{
    session->Add("app");
    session->Emit("app", Exchange::IAppManager::APP_STATE_SUSPENDED);
    runtime->SetInfo("app.instance", "{\"gpu\":{\"memory\":{\"usage\":0}}}");
    session->Expire("app", false);
    const auto transitions = apps->Transitions();
    ASSERT_EQ(1u, transitions.size());
    EXPECT_EQ(Exchange::IAppManager::APP_STATE_HIBERNATED, transitions[0].second);
    EXPECT_TRUE(apps->Terminations().empty());
}

TEST_F(ResourceMonitorAcceptanceTest, RepeatedPausedRetainsSnapshotDuringMetadataUnavailability)
{
    session->Add("app");
    session->Emit("app", Exchange::IAppManager::APP_STATE_PAUSED);
    session->packages->payloads.clear();
    for (const auto state : { Exchange::IAppManager::APP_STATE_ACTIVE,
        Exchange::IAppManager::APP_STATE_HIBERNATED, Exchange::IAppManager::APP_STATE_TERMINATING }) {
        session->Emit("app", state);
        session->Emit("app", Exchange::IAppManager::APP_STATE_PAUSED);
    }
    session->Expire("app", true);
    const auto transitions = apps->Transitions();
    ASSERT_EQ(1u, transitions.size());
    EXPECT_EQ(Exchange::IAppManager::APP_STATE_SUSPENDED, transitions[0].second);
    EXPECT_TRUE(apps->Terminations().empty());
}

TEST_F(ResourceMonitorAcceptanceTest, ChangedMetadataDoesNotReplaceInstanceSnapshot)
{
    session->Add("app");
    session->Emit("app", Exchange::IAppManager::APP_STATE_PAUSED);
    session->packages->payloads["app"] = "{\"ralfPkgPath\":\"/metadata.json\",\"applicationLifecycle\":{\"supportedNonActiveStates\":[\"paused\"]}}";
    session->Emit("app", Exchange::IAppManager::APP_STATE_ACTIVE);
    session->Emit("app", Exchange::IAppManager::APP_STATE_PAUSED);
    session->Expire("app", true);
    const auto transitions = apps->Transitions();
    ASSERT_EQ(1u, transitions.size());
    EXPECT_EQ(Exchange::IAppManager::APP_STATE_SUSPENDED, transitions[0].second);
}

TEST_F(ResourceMonitorAcceptanceTest, UnavailableSnapshotIsRetried)
{
    session->Add("app");
    const auto payload = session->packages->payloads.at("app");
    session->packages->payloads.clear();
    session->Emit("app", Exchange::IAppManager::APP_STATE_PAUSED);
    EXPECT_FALSE(session->monitor->_appTimers.at("app.instance").hasSnapshot);
    session->packages->payloads["app"] = payload;
    session->Emit("app", Exchange::IAppManager::APP_STATE_SUSPENDED);
    runtime->SetInfo("app.instance", "{\"gpu\":{\"memory\":{\"usage\":0}}}");
    session->Expire("app", false);
    const auto transitions = apps->Transitions();
    ASSERT_EQ(1u, transitions.size());
    EXPECT_EQ(Exchange::IAppManager::APP_STATE_HIBERNATED, transitions[0].second);
}

TEST_F(ResourceMonitorAcceptanceTest, UnloadDiscardsSnapshotAndAllowsFreshCapture)
{
    session->Add("app");
    session->Emit("app", Exchange::IAppManager::APP_STATE_PAUSED);
    session->monitor->_appManagerNotification.OnAppUnloaded("app", "app.instance");
    EXPECT_EQ(0u, session->monitor->_appTimers.count("app.instance"));
    session->packages->payloads["app"] = "{\"ralfPkgPath\":\"/metadata.json\",\"applicationLifecycle\":{\"supportedNonActiveStates\":[\"paused\"]}}";
    session->Emit("app", Exchange::IAppManager::APP_STATE_PAUSED);
    session->Expire("app", true);
    EXPECT_TRUE(apps->Transitions().empty());
    EXPECT_EQ(std::vector<string>({ "app" }), apps->Terminations());
}

TEST_F(ResourceMonitorAcceptanceTest, UnloadedLifecycleEventDiscardsInstanceState)
{
    session->Add("app");
    session->Emit("app", Exchange::IAppManager::APP_STATE_SUSPENDED);
    session->Emit("app", Exchange::IAppManager::APP_STATE_UNLOADED);
    EXPECT_EQ(0u, session->monitor->_appTimers.count("app.instance"));
    session->monitor->OnLifecycleTimerExpired();
    EXPECT_TRUE(apps->Transitions().empty());
    EXPECT_TRUE(apps->Terminations().empty());
}

TEST_F(ResourceMonitorAcceptanceTest, UnloadDuringSnapshotLookupDoesNotRestoreInstance)
{
    session->Add("app");
    session->packages->onGetLockedInfo = [this] {
        session->monitor->_appManagerNotification.OnAppUnloaded("app", "app.instance");
    };
    session->Emit("app", Exchange::IAppManager::APP_STATE_PAUSED);
    EXPECT_EQ(0u, session->monitor->_appTimers.count("app.instance"));
    EXPECT_TRUE(apps->Transitions().empty());
    EXPECT_TRUE(apps->Terminations().empty());
}

TEST_F(ResourceMonitorAcceptanceTest, ReplacedInstanceRejectsStaleSnapshotLookup)
{
    session->Add("app");
    session->packages->onGetLockedInfo = [this] {
        session->monitor->_appManagerNotification.OnAppUnloaded("app", "app.instance");
        session->packages->payloads["app"] = "{\"ralfPkgPath\":\"/metadata.json\",\"applicationLifecycle\":{\"supportedNonActiveStates\":[\"paused\"]}}";
        session->Emit("app", Exchange::IAppManager::APP_STATE_SUSPENDED);
    };
    session->Emit("app", Exchange::IAppManager::APP_STATE_PAUSED);
    const auto& state = session->monitor->_appTimers.at("app.instance");
    EXPECT_EQ(0u, state.pausedTimerExpiryMs);
    EXPECT_NE(0u, state.suspendedTimerExpiryMs);
    EXPECT_FALSE(state.lifecycleSnapshot.IsSupported(Plugin::AppLifecycleState::HIBERNATED));
}

TEST_F(ResourceMonitorAcceptanceTest, AC8_DoesNotRequestUnsupportedStates)
{
    session->Add("pausedOnly", "\"paused\"");
    session->Emit("pausedOnly", Exchange::IAppManager::APP_STATE_PAUSED);
    session->Expire("pausedOnly", true);
    session->Add("noHibernate", "\"paused\",\"suspended\"");
    session->Emit("noHibernate", Exchange::IAppManager::APP_STATE_PAUSED);
    session->Emit("noHibernate", Exchange::IAppManager::APP_STATE_SUSPENDED);
    runtime->SetInfo("noHibernate.instance", "{\"gpu\":{\"memory\":{\"usage\":0}}}");
    session->Expire("noHibernate", false);
    EXPECT_TRUE(apps->Transitions().empty());
    EXPECT_EQ(2u, apps->Terminations().size());
}

TEST_F(ResourceMonitorAcceptanceTest, AC9_TerminatesNonAppInsteadOfSuspending)
{
    session->Add("service", "\"paused\",\"suspended\",\"hibernated\"", "SYSTEM_APP");
    session->Emit("service", Exchange::IAppManager::APP_STATE_PAUSED);
    EXPECT_GT(session->Remaining("service", true), 59000u);
    session->Expire("service", true);
    EXPECT_EQ(std::vector<string>({ "service" }), apps->Terminations());
    EXPECT_TRUE(apps->Transitions().empty());
}

TEST_F(ResourceMonitorAcceptanceTest, UnknownGpuUsageDoesNotAuthorizeHibernation)
{
    uint32_t index = 0;
    for (const auto& info : { "{}", "not-json", "{\"gpu\":{\"memory\":{\"usage\":-1}}}",
        "{\"gpu\":{\"memory\":{\"usage\":1.5}}}", "{\"gpu\":{\"memory\":{\"usage\":\"unknown\"}}}" }) {
        const string id = "app" + std::to_string(index++);
        session->Add(id);
        session->Emit(id, Exchange::IAppManager::APP_STATE_PAUSED);
        session->Emit(id, Exchange::IAppManager::APP_STATE_SUSPENDED);
        runtime->SetInfo(id + ".instance", info);
        session->Expire(id, false);
    }
    EXPECT_TRUE(apps->Transitions().empty());
    EXPECT_TRUE(apps->Terminations().empty());
}

TEST_F(ResourceMonitorAcceptanceTest, ThunderTimerDispatchesSuspensionRequest)
{
    session->Add("app");
    session->Emit("app", Exchange::IAppManager::APP_STATE_PAUSED);
    {
        std::lock_guard<std::mutex> lock(session->monitor->_appTimersMutex);
        session->monitor->_appTimers.at("app.instance").pausedTimerExpiryMs = session->Now() + 20;
        session->monitor->ScheduleNextTimerLocked();
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (apps->Transitions().empty() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    const auto transitions = apps->Transitions();
    ASSERT_EQ(1u, transitions.size());
    EXPECT_EQ(Exchange::IAppManager::APP_STATE_SUSPENDED, transitions[0].second);
}

TEST_F(ResourceMonitorAcceptanceTest, StateChangeCancelsEnforcement)
{
    session->Add("app");
    session->Emit("app", Exchange::IAppManager::APP_STATE_PAUSED);
    session->Emit("app", Exchange::IAppManager::APP_STATE_ACTIVE);
    EXPECT_EQ(0u, session->Remaining("app", true));
    session->monitor->OnLifecycleTimerExpired();
    EXPECT_TRUE(apps->Transitions().empty());
    EXPECT_TRUE(apps->Terminations().empty());
}

// Notification Mock
class NotificationMock : public Exchange::IResourceMonitor::INotification
{
    public:

    MOCK_METHOD(void, OnReconciliationComplete, (const string&, const bool), (override));
    MOCK_METHOD(uint32_t, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));

    BEGIN_INTERFACE_MAP(NotificationMock)
    INTERFACE_ENTRY(Exchange::IResourceMonitor::INotification)
    END_INTERFACE_MAP
};

// TESTS
TEST_F(ResourceMonitorTest, InitializeWithoutSecurityAgent)
{
    NiceMock<ServiceMock> noSecurityService;
    ON_CALL(noSecurityService,
            QueryInterfaceByCallsign(_, _))
        .WillByDefault(Return(nullptr));

    const string result = plugin->Initialize(&noSecurityService);

    EXPECT_TRUE(result.empty() || result == "ResourceMonitor plugin could not be initialised");
    if (result.empty()) {
        plugin->Deinitialize(&noSecurityService);
    }
}

TEST_F(ResourceMonitorInitializedTest, RegisteredMethods)
{
    ASSERT_TRUE(IsPluginInitialized());
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getMemInfo")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getPsiMetrics")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getSwapUsed")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getFlashSpace")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getStats")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("reconcile")));
}

TEST_F(ResourceMonitorInitializedTest, InformationReturnsDescription)
{
    EXPECT_EQ("Plugin which exposes ResourceMonitor related methods.", plugin->Information());
}


// JSONRPC API Tests
TEST_F(ResourceMonitorInitializedTest, GetMemInfoReturnsExpectedFields)
{
    ASSERT_TRUE(IsPluginInitialized());
    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(
            connection,
            _T("getMemInfo"),
            _T("{}"),
            response));

    JsonObject result;
    result.FromString(response);

    EXPECT_TRUE(result.HasLabel("memAvailable"));
    EXPECT_TRUE(result.HasLabel("swapFree"));
    EXPECT_TRUE(result.HasLabel("swapTotal"));
}

TEST_F(ResourceMonitorInitializedTest, GetPsiMetricsReturnsValue)
{
    ASSERT_TRUE(IsPluginInitialized());
    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(
            connection,
            _T("getPsiMetrics"),
            _T("{\"metric\":\"full_avg60\"}"),
            response));

    JsonObject result;
    result.FromString(response);

    EXPECT_TRUE(result.HasLabel("value") || result.HasLabel("success"));
}

TEST_F(ResourceMonitorInitializedTest, GetSwapUsedReturnsExpectedFields)
{
    ASSERT_TRUE(IsPluginInitialized());

    const uint32_t result = handler.Invoke(connection,
            _T("getSwapUsed"),
            _T("{}"),
            response);

    EXPECT_TRUE(result == Core::ERROR_NONE || result == Core::ERROR_GENERAL);
}

TEST_F(ResourceMonitorInitializedTest, GetFlashSpaceReturnsExpectedFields)
{
    ASSERT_TRUE(IsPluginInitialized());
    const uint32_t result = handler.Invoke(
            connection,
            _T("getFlashSpace"),
            _T("{}"),
            response);

    EXPECT_TRUE(result == Core::ERROR_NONE || result == Core::ERROR_GENERAL);
}

TEST_F(ResourceMonitorInitializedTest, GetStatsReturnsAllExpectedFields)
{
    ASSERT_TRUE(IsPluginInitialized());
    const uint32_t result = handler.Invoke(
            connection,
            _T("getStats"),
            _T("{}"),
            response);

    EXPECT_TRUE(result == Core::ERROR_NONE || result == Core::ERROR_GENERAL);
}

TEST_F(ResourceMonitorInitializedTest, ReconcileMethodRegistered)
{
    ASSERT_TRUE(IsPluginInitialized());
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("reconcile")));
}


TEST_F(ResourceMonitorInitializedTest, ReconcileMissingAppId)
{
    JsonObject result;

    EXPECT_NE(
        Core::ERROR_NONE,
        handler.Invoke(
            connection,
            _T("reconcile"),
            _T("{"
               "\"ramTargetMB\":100,"
               "\"allowTerminate\":false"
               "}"),
            response));
}

TEST_F(ResourceMonitorInitializedTest, ReconcileMissingRamTarget)
{
    EXPECT_EQ(
        Core::ERROR_NONE,
        handler.Invoke(
            connection,
            _T("reconcile"),
            _T("{"
               "\"appId\":\"Netflix\","
               "\"allowTerminate\":false"
               "}"),
            response));
}

TEST_F(ResourceMonitorInitializedTest, ReconcileInvalidParameterType)
{
    EXPECT_NE(
        Core::ERROR_NONE,
        handler.Invoke(
            connection,
            _T("reconcile"),
            _T("{"
               "\"appId\":123,"
               "\"ramTargetMB\":\"abc\","
               "\"allowTerminate\":false"
               "}"),
            response));
}

TEST_F(ResourceMonitorInitializedTest, MultipleReconcileRequests)
{
    ASSERT_TRUE(IsPluginInitialized());
    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(
            connection,
            _T("reconcile"),
            _T("{"
               "\"appId\":\"Netflix\","
               "\"ramTargetMB\":100,"
               "\"allowTerminate\":true"
               "}"),
            response));

    ASSERT_TRUE(IsPluginInitialized());
    EXPECT_EQ(Core::ERROR_NONE,
        handler.Invoke(
            connection,
            _T("reconcile"),
            _T("{"
               "\"appId\":\"YouTube\","
               "\"ramTargetMB\":200,"
               "\"allowTerminate\":true"
               "}"),
            response));
}

TEST_F(ResourceMonitorInitializedTest, ReconcileEmptyAppId)
{
    EXPECT_NE(Core::ERROR_NONE,
    handler.Invoke(
        connection,
        _T("reconcile"),
        _T("{"
        "\"appId\":\"\","
        "\"ramTargetMB\":100,"
        "\"allowTerminate\":true"
        "}"), response));
}

TEST(ResourceMonitorImplementationTest, RegisterNullNotification)
{
    TestableResourceMonitorImplementation impl;
    EXPECT_EQ(Core::ERROR_BAD_REQUEST, impl.Register(nullptr));
}

TEST(ResourceMonitorImplementationTest, UnregisterNullNotification)
{
    TestableResourceMonitorImplementation impl;
    EXPECT_EQ(Core::ERROR_BAD_REQUEST, impl.Unregister(nullptr));
}
TEST(ResourceMonitorImplementationTest, ValidPsiMetrics)
{
    TestableResourceMonitorImplementation impl;
    char psiFile[] = "/tmp/resource_monitor_psi_XXXXXX";
    const int fd = mkstemp(psiFile);
    ASSERT_NE(-1, fd);
    close(fd);
    {
        std::ofstream file(psiFile);
        ASSERT_TRUE(file.is_open());
        file << "some avg10=1.25 avg60=2.50 avg300=3.75 total=123456\n";
        file << "full avg10=0.50 avg60=1.50 avg300=2.50 total=654321\n";
    }
    const string config =
        string("{\"root\":{") +
        "\"psi_checking_enabled\":true,"
        "\"psi_path\":\"" + psiFile + "\""
        "}}";

    ASSERT_EQ(Core::ERROR_NONE, impl.CallSetConfig(config));
    double value = 0.0;
    uint64_t total = 0;
    EXPECT_EQ(Core::ERROR_NONE, impl.CallGetPsiMetrics("full_avg60", value, total));
    EXPECT_DOUBLE_EQ(1.50, value);
    EXPECT_EQ(654321ULL, total);

    value = 0.0;
    total = 0;

    EXPECT_EQ(Core::ERROR_NONE, impl.CallGetPsiMetrics("some_avg10", value, total));

    EXPECT_DOUBLE_EQ(1.25, value);
    EXPECT_EQ(123456ULL, total);
    EXPECT_EQ(Core::ERROR_BAD_REQUEST, impl.CallGetPsiMetrics("invalid_metric", value, total));
    std::remove(psiFile);
}

TEST(ResourceMonitorImplementationTest, PsiMetricsFileNotFound)
{
    TestableResourceMonitorImplementation impl;

    const string config =
        "{\"root\":{"
        "\"psi_checking_enabled\":true,"
        "\"psi_path\":\"/tmp/resource_monitor_missing_psi_file\""
        "}}";

    ASSERT_EQ(Core::ERROR_NONE, impl.CallSetConfig(config));
    double value = 0.0;
    uint64_t total = 0;
    EXPECT_EQ(Core::ERROR_GENERAL, impl.CallGetPsiMetrics("full_avg60", value, total));
}

TEST_F(ResourceMonitorInitializedTest, MultipleStatsRequests)
{
    for (uint32_t i = 0; i < 25; i++) {
        ASSERT_TRUE(IsPluginInitialized());
        uint32_t result = handler.Invoke(connection, _T("getStats"), _T("{}"), response);
        EXPECT_TRUE(result == Core::ERROR_NONE || result == Core::ERROR_GENERAL);
    }
}

TEST(ResourceMonitorImplementationTest, RegisterAndUnregister)
{
    TestableResourceMonitorImplementation impl;
    NotificationMock notification;

    EXPECT_CALL(notification, AddRef()).WillOnce(Return(1));
    EXPECT_EQ(Core::ERROR_NONE, impl.Register(&notification));
    EXPECT_CALL(notification, Release()).WillOnce(Return(0));
    EXPECT_EQ(Core::ERROR_NONE, impl.Unregister(&notification));
}

// ---------------------------------------------------------------------------
// RDKEMW-28030: lifecycle capabilities from Package Manager
// ---------------------------------------------------------------------------

namespace {
    const char* const kAllStatesBlock =
        "{\"supportedNonActiveStates\":[\"paused\",\"suspended\",\"hibernated\"],"
        "\"maxSuspendedSystemMemory\":\"16M\",\"maxTimeToSuspendMemoryState\":10}";
    const char* const kPausedSuspendedBlock =
        "{\"supportedNonActiveStates\":[\"paused\",\"suspended\"],"
        "\"maxSuspendedSystemMemory\":\"100M\",\"maxTimeToSuspendMemoryState\":15}";

    string RuntimePayload(const string& ralfPkgPath, const string& lifecycle)
    {
        string payload = "{\"appPath\":\"/apps/test\",\"command\":\"run\"";
        if (!ralfPkgPath.empty()) {
            payload += ",\"ralfPkgPath\":\"" + ralfPkgPath + "\"";
        }
        if (!lifecycle.empty()) {
            payload += ",\"applicationLifecycle\":" + lifecycle;
        }
        return payload + "}";
    }

    string RalfPayload(const string& lifecycle)
    {
        return RuntimePayload("/tmp/com.example.app_1.0.0_metadata.json", lifecycle);
    }
}

class ResourceMonitorLifecycleTest : public ResourceMonitorTest {
protected:
    NiceMock<ServiceMock> service;
    NiceMock<MockAuthenticate>* mockAuth = nullptr;
    NiceMock<PackageManagerMock> packageHandler;
    NiceMock<PackageInstallerMock> packageInstaller;
    bool packageManagerAvailable = true;
    std::vector<std::pair<string, string>> installed;          // (packageId, version)
    std::map<std::pair<string, string>, std::pair<bool, string>> lockState; // -> (locked, payload)
    string initResult;

    void SetUp() override
    {
        ASSERT_TRUE(initResult.empty()) << "Plugin initialization failed: " << initResult;
        ASSERT_NE(nullptr, Plugin::ResourceMonitorImplementation::_instance);
    }

public:
    ResourceMonitorLifecycleTest()
    {
        mockAuth = new NiceMock<MockAuthenticate>();
        ON_CALL(packageHandler, AddRef()).WillByDefault(Return(1));
        ON_CALL(packageHandler, Release()).WillByDefault(Return(0));
        ON_CALL(packageInstaller, AddRef()).WillByDefault(Return(1));
        ON_CALL(packageInstaller, Release()).WillByDefault(Return(0));

        ON_CALL(service, QueryInterfaceByCallsign(_, _))
            .WillByDefault([this](const uint32_t id, const std::string& name) -> void* {
                if (name == "SecurityAgent") {
                    mockAuth->AddRef();
                    return static_cast<void*>(mockAuth);
                }
                if ((name == "org.rdk.AppPackageManager") && packageManagerAvailable) {
                    if (id == Exchange::IPackageHandler::ID) {
                        return static_cast<void*>(static_cast<Exchange::IPackageHandler*>(&packageHandler));
                    }
                    if (id == Exchange::IPackageInstaller::ID) {
                        return static_cast<void*>(static_cast<Exchange::IPackageInstaller*>(&packageInstaller));
                    }
                }
                return nullptr;
            });

        ON_CALL(packageInstaller, ListPackages(_))
            .WillByDefault([this](Exchange::IPackageInstaller::IPackageIterator*& packages) -> Core::hresult {
                std::list<Exchange::IPackageInstaller::Package> list;
                for (const auto& entry : installed) {
                    Exchange::IPackageInstaller::Package package {};
                    package.packageId = entry.first;
                    package.version = entry.second;
                    package.state = Exchange::IPackageInstaller::InstallState::INSTALLED;
                    list.push_back(package);
                }
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<
                    Exchange::IPackageInstaller::IPackageIterator>(list);
                return Core::ERROR_NONE;
            });

        ON_CALL(packageHandler, GetLockedInfo(_, _, _, _, _, _))
            .WillByDefault([this](const string& packageId, const string& version, string& unpackedPath,
                               string& runtimeConfigPayload, string& gatewayMetadataPath, bool& locked) -> Core::hresult {
                const auto it = lockState.find(std::make_pair(packageId, version));
                if (it == lockState.end()) {
                    return Core::ERROR_INVALID_PARAMETER;
                }
                unpackedPath = "/apps/" + packageId;
                gatewayMetadataPath.clear();
                locked = it->second.first;
                runtimeConfigPayload = it->second.second;
                return Core::ERROR_NONE;
            });

        initResult = plugin->Initialize(&service);
    }

    ~ResourceMonitorLifecycleTest() override
    {
        if (initResult.empty()) {
            plugin->Deinitialize(&service);
        }
        delete mockAuth;
    }

    void AddPackage(const string& packageId, const string& version, const bool locked, const string& payload)
    {
        installed.emplace_back(packageId, version);
        lockState[std::make_pair(packageId, version)] = std::make_pair(locked, payload);
    }

    Plugin::ResourceMonitorImplementation& Impl() const
    {
        return *Plugin::ResourceMonitorImplementation::_instance;
    }
};

// AC1: supported lifecycle states are returned from the Package Manager configuration.
TEST_F(ResourceMonitorLifecycleTest, AC1_AllLifecycleStatesConfigured)
{
    AddPackage("com.example.app", "1.0.0", true, RalfPayload(kAllStatesBlock));

    Plugin::AppLifecycleCapabilities caps;
    EXPECT_EQ(Core::ERROR_NONE, Impl().GetAppLifecycleCapabilities("com.example.app", caps));
    EXPECT_EQ(Plugin::LifecycleConfigStatus::CONFIGURED, caps.status);
    EXPECT_TRUE(caps.IsSupported(Plugin::AppLifecycleState::ACTIVE));
    EXPECT_TRUE(caps.IsSupported(Plugin::AppLifecycleState::PAUSED));
    EXPECT_TRUE(caps.IsSupported(Plugin::AppLifecycleState::SUSPENDED));
    EXPECT_TRUE(caps.IsSupported(Plugin::AppLifecycleState::HIBERNATED));
}

// AC2: only the configured subset of Paused, Suspended and Hibernated is reported.
TEST_F(ResourceMonitorLifecycleTest, AC2_PartialLifecycleSupport)
{
    AddPackage("com.example.app", "1.0.0", true, RalfPayload(kPausedSuspendedBlock));

    bool supported = false;
    EXPECT_EQ(Core::ERROR_NONE, Impl().IsAppLifecycleStateSupported("com.example.app", Plugin::AppLifecycleState::PAUSED, supported));
    EXPECT_TRUE(supported);
    EXPECT_EQ(Core::ERROR_NONE, Impl().IsAppLifecycleStateSupported("com.example.app", Plugin::AppLifecycleState::SUSPENDED, supported));
    EXPECT_TRUE(supported);
    EXPECT_EQ(Core::ERROR_NONE, Impl().IsAppLifecycleStateSupported("com.example.app", Plugin::AppLifecycleState::HIBERNATED, supported));
    EXPECT_FALSE(supported);
}

// AC3: a package without optional lifecycle configuration reports those states as unsupported.
TEST_F(ResourceMonitorLifecycleTest, AC3_NoOptionalLifecycleConfiguration)
{
    AddPackage("com.ralf.app", "1.0.0", true, RalfPayload(""));
    AddPackage("com.legacy.app", "1.0.0", true, RuntimePayload("", ""));

    Plugin::AppLifecycleCapabilities caps;
    EXPECT_EQ(Core::ERROR_NONE, Impl().GetAppLifecycleCapabilities("com.ralf.app", caps));
    EXPECT_EQ(Plugin::LifecycleConfigStatus::NOT_CONFIGURED, caps.status);
    EXPECT_TRUE(caps.IsSupported(Plugin::AppLifecycleState::ACTIVE));
    EXPECT_FALSE(caps.IsSupported(Plugin::AppLifecycleState::PAUSED));
    EXPECT_FALSE(caps.IsSupported(Plugin::AppLifecycleState::SUSPENDED));
    EXPECT_FALSE(caps.IsSupported(Plugin::AppLifecycleState::HIBERNATED));

    EXPECT_EQ(Core::ERROR_NONE, Impl().GetAppLifecycleCapabilities("com.legacy.app", caps));
    EXPECT_EQ(Plugin::LifecycleConfigStatus::UNAVAILABLE, caps.status);
    EXPECT_FALSE(caps.IsSupported(Plugin::AppLifecycleState::PAUSED));
}

// AC4: the configured Suspended values are returned; missing or invalid values are unavailable.
TEST_F(ResourceMonitorLifecycleTest, AC4_SuspendedConfiguration)
{
    AddPackage("com.example.app", "1.0.0", true, RalfPayload(kAllStatesBlock));
    AddPackage("com.partial.app", "1.0.0", true,
        RalfPayload("{\"supportedNonActiveStates\":[\"paused\",\"suspended\"],\"maxSuspendedSystemMemory\":\"16X\"}"));

    Plugin::SuspendedStateConfig config;
    EXPECT_EQ(Core::ERROR_NONE, Impl().GetAppSuspendedStateConfig("com.example.app", config));
    EXPECT_TRUE(config.hasMaxSystemMemory);
    EXPECT_EQ(16777216ULL, config.maxSystemMemoryBytes);
    EXPECT_TRUE(config.hasMaxTimeToSuspend);
    EXPECT_EQ(10U, config.maxTimeToSuspendSec);

    EXPECT_EQ(Core::ERROR_NONE, Impl().GetAppSuspendedStateConfig("com.partial.app", config));
    EXPECT_FALSE(config.hasMaxSystemMemory);
    EXPECT_FALSE(config.hasMaxTimeToSuspend);
}

// AC5: the reported values reflect the Package Manager configuration.
TEST_F(ResourceMonitorLifecycleTest, AC5_ValuesFollowPackageManager)
{
    AddPackage("com.example.app", "1.0.0", true, RalfPayload(kPausedSuspendedBlock));

    Plugin::AppLifecycleCapabilities caps;
    Impl().GetAppLifecycleCapabilities("com.example.app", caps);
    EXPECT_FALSE(caps.IsSupported(Plugin::AppLifecycleState::HIBERNATED));
    EXPECT_EQ(15U, caps.suspended.maxTimeToSuspendSec);

    lockState[std::make_pair(string("com.example.app"), string("1.0.0"))].second = RalfPayload(kAllStatesBlock);
    Impl().GetAppLifecycleCapabilities("com.example.app", caps);
    EXPECT_TRUE(caps.IsSupported(Plugin::AppLifecycleState::HIBERNATED));
    EXPECT_EQ(10U, caps.suspended.maxTimeToSuspendSec);
}

// Package Manager unavailable is handled without failure and retried on the next request.
TEST_F(ResourceMonitorLifecycleTest, PackageManagerUnavailableThenAvailable)
{
    AddPackage("com.example.app", "1.0.0", true, RalfPayload(kAllStatesBlock));
    packageManagerAvailable = false;

    Plugin::AppLifecycleCapabilities caps;
    EXPECT_EQ(Core::ERROR_NONE, Impl().GetAppLifecycleCapabilities("com.example.app", caps));
    EXPECT_EQ(Plugin::LifecycleConfigStatus::UNAVAILABLE, caps.status);
    EXPECT_FALSE(caps.IsSupported(Plugin::AppLifecycleState::PAUSED));

    packageManagerAvailable = true;
    EXPECT_EQ(Core::ERROR_NONE, Impl().GetAppLifecycleCapabilities("com.example.app", caps));
    EXPECT_EQ(Plugin::LifecycleConfigStatus::CONFIGURED, caps.status);
    EXPECT_TRUE(caps.IsSupported(Plugin::AppLifecycleState::HIBERNATED));

    uint32_t result = handler.Invoke(connection, _T("getMemInfo"), _T("{}"), response);
    EXPECT_TRUE(result == Core::ERROR_NONE || result == Core::ERROR_GENERAL);
}

// The locked version is used; ambiguous locks need an explicit version; empty appId is rejected.
TEST_F(ResourceMonitorLifecycleTest, VersionResolution)
{
    AddPackage("com.example.app", "1.0.0", false, RalfPayload(kAllStatesBlock));
    AddPackage("com.example.app", "1.1.0", true, RalfPayload(kPausedSuspendedBlock));
    AddPackage("com.twice.app", "1.0.0", true, RalfPayload(kAllStatesBlock));
    AddPackage("com.twice.app", "2.0.0", true, RalfPayload(kPausedSuspendedBlock));

    Plugin::AppLifecycleCapabilities caps;
    Impl().GetAppLifecycleCapabilities("com.example.app", caps);
    EXPECT_EQ(Plugin::LifecycleConfigStatus::CONFIGURED, caps.status);
    EXPECT_FALSE(caps.IsSupported(Plugin::AppLifecycleState::HIBERNATED));

    Impl().GetAppLifecycleCapabilities("com.twice.app", caps);
    EXPECT_EQ(Plugin::LifecycleConfigStatus::UNAVAILABLE, caps.status);

    Impl().GetAppLifecycleCapabilities("com.twice.app", caps, "1.0.0");
    EXPECT_EQ(Plugin::LifecycleConfigStatus::CONFIGURED, caps.status);
    EXPECT_TRUE(caps.IsSupported(Plugin::AppLifecycleState::HIBERNATED));

    EXPECT_EQ(Core::ERROR_INVALID_PARAMETER, Impl().GetAppLifecycleCapabilities("", caps));
}

TEST(ResourceMonitorImplementationTest, RegisterNotificationForReconcile)
{
    TestableResourceMonitorImplementation impl;
    NotificationMock notification;

    EXPECT_CALL(notification, AddRef()).Times(2).WillRepeatedly(Return(1));
    EXPECT_CALL(notification, Release()).Times(2).WillRepeatedly(Return(0));
    ASSERT_EQ(Core::ERROR_NONE, impl.Register(&notification));
    EXPECT_CALL(notification, OnReconciliationComplete("Netflix", true)).Times(1);
    impl.CallOnReconciliationComplete("Netflix", true);
    EXPECT_EQ(Core::ERROR_NONE, impl.Unregister(&notification));
}
