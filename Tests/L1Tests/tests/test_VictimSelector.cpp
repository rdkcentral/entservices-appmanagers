#include <gtest/gtest.h>

#include "VictimSelector.h"
#include "ServiceMock.h"
#include <interfaces/ITelemetryMetrics.h>
#include <interfaces/IAppManager.h>
#include <interfaces/IRuntimeManager.h>
#include <istream>
#include <mutex>

#define private public
#include "VictimSelectorImplementation.h"
#undef private

namespace {

TEST(VictimSelectorL1, InitializeRejectsNullService)
{
    WPEFramework::Core::ProxyType<WPEFramework::Plugin::VictimSelector> selector =
        WPEFramework::Core::ProxyType<WPEFramework::Plugin::VictimSelector>::Create();

    EXPECT_EQ("VictimSelector received an invalid service", selector->Initialize(nullptr));
}

TEST(VictimSelectorL1, InformationReturnsPluginDescription)
{
    WPEFramework::Core::ProxyType<WPEFramework::Plugin::VictimSelector> selector =
        WPEFramework::Core::ProxyType<WPEFramework::Plugin::VictimSelector>::Create();

    EXPECT_EQ("Victim Selector plugin", selector->Information());
}

class VictimTelemetryMock : public WPEFramework::Exchange::ITelemetryMetrics {
public:
    BEGIN_INTERFACE_MAP(VictimTelemetryMock)
    INTERFACE_ENTRY(WPEFramework::Exchange::ITelemetryMetrics)
    END_INTERFACE_MAP
    MOCK_METHOD(WPEFramework::Core::hresult, Record, (const string&, const string&, const string&), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Publish, (const string&, const string&), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, PublishEvent, (const string&, const string&, bool), (override));
};

class VictimSelectorTelemetryTest : public ::testing::Test {
protected:
    ::testing::NiceMock<ServiceMock> service;
    WPEFramework::Plugin::VictimSelectorImplementation* selector = nullptr;
    VictimTelemetryMock* telemetry = nullptr;
    const string payload = R"({"appId":"victim","reclaimReason":"RAM","systemRamUsage":1024,"gpuMemoryUsage":512,"runningAppsCount":1,"suspendedAppsCount":2})";

    void SetUp() override
    {
        selector = WPEFramework::Core::Service<WPEFramework::Plugin::VictimSelectorImplementation>::Create<WPEFramework::Plugin::VictimSelectorImplementation>();
        telemetry = WPEFramework::Core::Service<VictimTelemetryMock>::Create<VictimTelemetryMock>();
        selector->mService = &service;
        service.AddRef();
        selector->mPendingAppId = "victim";
        selector->mPendingTelemetry = payload;
        selector->mEvictionInProgress = true;
    }

    void TearDown() override
    {
        selector->Release();
        telemetry->Release();
    }

    void Notify(const string& appId, WPEFramework::Exchange::IAppManager::AppErrorReason error)
    {
        selector->onAppLifecycleStateChanged(appId, appId + ".instance",
            WPEFramework::Exchange::IAppManager::APP_STATE_UNLOADED,
            WPEFramework::Exchange::IAppManager::APP_STATE_TERMINATING, error);
    }

    void ExpectTelemetryLookup()
    {
        EXPECT_CALL(service, QueryInterfaceByCallsign(
            WPEFramework::Exchange::ITelemetryMetrics::ID, "org.rdk.TelemetryMetrics"))
            .WillOnce([this](uint32_t, const string&) -> void* {
                telemetry->AddRef();
                return static_cast<WPEFramework::Exchange::ITelemetryMetrics*>(telemetry);
            });
    }
};

TEST_F(VictimSelectorTelemetryTest, SuccessfulEvictionPublishesVerboseMarkerExactlyOnce)
{
    ExpectTelemetryLookup();
    EXPECT_CALL(*telemetry, PublishEvent(payload, "ENTS_INFO_RDKAMVictimSelection", true))
        .WillOnce(::testing::Return(WPEFramework::Core::ERROR_NONE));
    Notify("unrelated", WPEFramework::Exchange::IAppManager::APP_ERROR_NONE);
    EXPECT_FALSE(selector->mPendingTelemetry.empty());
    Notify("victim", WPEFramework::Exchange::IAppManager::APP_ERROR_NONE);
    Notify("victim", WPEFramework::Exchange::IAppManager::APP_ERROR_NONE);
    EXPECT_TRUE(selector->mPendingTelemetry.empty());
    EXPECT_FALSE(selector->mEvictionInProgress);
}

TEST_F(VictimSelectorTelemetryTest, FailedEvictionDiscardsSnapshotWithoutPublishing)
{
    EXPECT_CALL(service, QueryInterfaceByCallsign(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(*telemetry, PublishEvent(::testing::_, ::testing::_, ::testing::_)).Times(0);
    Notify("victim", WPEFramework::Exchange::IAppManager::APP_ERROR_UNKNOWN);
    EXPECT_TRUE(selector->mPendingTelemetry.empty());
    EXPECT_FALSE(selector->mEvictionInProgress);
}

TEST_F(VictimSelectorTelemetryTest, PublishingFailureDoesNotLeaveEvictionPending)
{
    ExpectTelemetryLookup();
    EXPECT_CALL(*telemetry, PublishEvent(payload, "ENTS_INFO_RDKAMVictimSelection", true))
        .WillOnce(::testing::Return(WPEFramework::Core::ERROR_GENERAL));
    Notify("victim", WPEFramework::Exchange::IAppManager::APP_ERROR_NONE);
    EXPECT_FALSE(selector->mEvictionInProgress);
    EXPECT_TRUE(selector->mPendingAppId.empty());
}

} // namespace