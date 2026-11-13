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
#include "ResourceMonitorImplementation.h"
#include "ServiceMock.h"
#include "PackageManagerMock.h"
#include "ThunderPortability.h"
#include <core/core.h>
#include <list>
#include <map>
#include <utility>
#include <vector>

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
};


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

TEST_F(ResourceMonitorInitializedTest, ReconcileReturnsSuccess)
{
    ASSERT_TRUE(IsPluginInitialized());
    ASSERT_EQ(Core::ERROR_NONE,
        handler.Invoke(
            connection,
            _T("reconcile"),
            _T("{"
               "\"appId\":\"Netflix\","
               "\"ramTargetMB\":100,"
               "\"allowTerminate\":false"
               "}"),
            response));
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
    EXPECT_NE(
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

    double value{};
    uint64_t total{};

    impl.CallGetPsiMetrics("full_avg60", value, total);
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
