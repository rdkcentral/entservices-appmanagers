#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "AppManager.h"
#include "FactoriesImplementation.h"
#include "ServiceMock.h"
#include "LifecycleManagerMock.h"
#include "PackageManagerMock.h"
#include "StorageManagerMock.h"
#include "Store2Mock.h"
#include "TelemetryMetricsMock.h"

using namespace WPEFramework;
using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Test;

class AppManagerTest : public Test {
protected:
    Core::ProxyType<Plugin::AppManager> plugin;
    Core::JSONRPC::Handler& handler;
    Core::JSONRPC::Connection connection;
    string response;
    NiceMock<ServiceMock> service;
    NiceMock<LifecycleManagerMock>* lifecycleManagerMock;
    NiceMock<PackageManagerHandlerMock>* packageManagerHandlerMock;
    NiceMock<PackageManagerInstallerMock>* packageManagerInstallerMock;
    NiceMock<StorageManagerMock>* storageManagerMock;
    NiceMock<Store2Mock>* store2Mock;
    NiceMock<TelemetryMetricsMock>* telemetryMetricsMock;

    AppManagerTest()
        : plugin(Core::ProxyType<Plugin::AppManager>::Create())
        , handler(*(plugin))
        , connection(1, 0)
    {
    }

    virtual ~AppManagerTest() = default;

    void SetUp() override
    {
        lifecycleManagerMock = new NiceMock<LifecycleManagerMock>();
        packageManagerHandlerMock = new NiceMock<PackageManagerHandlerMock>();
        packageManagerInstallerMock = new NiceMock<PackageManagerInstallerMock>();
        storageManagerMock = new NiceMock<StorageManagerMock>();
        store2Mock = new NiceMock<Store2Mock>();
        telemetryMetricsMock = new NiceMock<TelemetryMetricsMock>();

        ON_CALL(service, QueryInterfaceByCallsign(::testing::_, ::testing::StrEq("org.rdk.LifecycleManager")))
            .WillByDefault(::testing::Invoke(
                [&](const uint32_t, const string&) {
                    return lifecycleManagerMock;
                }));

        ON_CALL(service, QueryInterfaceByCallsign(::testing::_, ::testing::StrEq("org.rdk.PackageManager")))
            .WillByDefault(::testing::Invoke(
                [&](const uint32_t, const string&) {
                    return packageManagerHandlerMock;
                }));

        ON_CALL(service, QueryInterfaceByCallsign(::testing::_, ::testing::StrEq("org.rdk.StorageManager")))
            .WillByDefault(::testing::Invoke(
                [&](const uint32_t, const string&) {
                    return storageManagerMock;
                }));

        ON_CALL(service, QueryInterfaceByCallsign(::testing::_, ::testing::StrEq("org.rdk.PersistentStore")))
            .WillByDefault(::testing::Invoke(
                [&](const uint32_t, const string&) {
                    return store2Mock;
                }));

        ON_CALL(service, QueryInterfaceByCallsign(::testing::_, ::testing::StrEq("org.rdk.TelemetryMetrics")))
            .WillByDefault(::testing::Invoke(
                [&](const uint32_t, const string&) {
                    return telemetryMetricsMock;
                }));

        EXPECT_CALL(*lifecycleManagerMock, AddRef())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Return());

        EXPECT_CALL(*lifecycleManagerMock, Release())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Return(Core::ERROR_NONE));

        EXPECT_CALL(*packageManagerHandlerMock, AddRef())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Return());

        EXPECT_CALL(*packageManagerHandlerMock, Release())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Return(Core::ERROR_NONE));

        EXPECT_CALL(*packageManagerInstallerMock, AddRef())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Return());

        EXPECT_CALL(*packageManagerInstallerMock, Release())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Return(Core::ERROR_NONE));

        EXPECT_CALL(*storageManagerMock, AddRef())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Return());

        EXPECT_CALL(*storageManagerMock, Release())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Return(Core::ERROR_NONE));

        EXPECT_CALL(*store2Mock, AddRef())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Return());

        EXPECT_CALL(*store2Mock, Release())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Return(Core::ERROR_NONE));

        EXPECT_CALL(*telemetryMetricsMock, AddRef())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Return());

        EXPECT_CALL(*telemetryMetricsMock, Release())
            .Times(::testing::AnyNumber())
            .WillRepeatedly(::testing::Return(Core::ERROR_NONE));

        EXPECT_EQ(string(""), plugin->Initialize(&service));
    }

    void TearDown() override
    {
        plugin->Deinitialize(&service);

        if (lifecycleManagerMock != nullptr) {
            delete lifecycleManagerMock;
            lifecycleManagerMock = nullptr;
        }
        if (packageManagerHandlerMock != nullptr) {
            delete packageManagerHandlerMock;
            packageManagerHandlerMock = nullptr;
        }
        if (packageManagerInstallerMock != nullptr) {
            delete packageManagerInstallerMock;
            packageManagerInstallerMock = nullptr;
        }
        if (storageManagerMock != nullptr) {
            delete storageManagerMock;
            storageManagerMock = nullptr;
        }
        if (store2Mock != nullptr) {
            delete store2Mock;
            store2Mock = nullptr;
        }
        if (telemetryMetricsMock != nullptr) {
            delete telemetryMetricsMock;
            telemetryMetricsMock = nullptr;
        }
    }
};

TEST_F(AppManagerTest, LaunchApp_Success)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                Exchange::IPackageInstaller::Package pkg;
                pkg.id = "testApp";
                pkg.version = "1.0.0";
                packageList.push_back(pkg);
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    EXPECT_CALL(*packageManagerHandlerMock, Lock(_, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<2>(123),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, IsAppLoaded(_, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<1>(false),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, SpawnApp(_, _, _, _, _, _, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<5>("instance123"),
            ::testing::SetArgReferee<7>(true),
            ::testing::Return(Core::ERROR_NONE)));

    string params = "{\"appId\":\"testApp\",\"intent\":\"{}\",\"launchArgs\":\"\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("launchApp"), params, response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
}

TEST_F(AppManagerTest, LaunchApp_InvalidParams_EmptyAppId)
{
    string params = "{\"appId\":\"\",\"intent\":\"{}\",\"launchArgs\":\"\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("launchApp"), params, response));
}

TEST_F(AppManagerTest, LaunchApp_PackageNotInstalled)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    string params = "{\"appId\":\"unknownApp\",\"intent\":\"{}\",\"launchArgs\":\"\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("launchApp"), params, response));
}

TEST_F(AppManagerTest, LaunchApp_LockFailed)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                Exchange::IPackageInstaller::Package pkg;
                pkg.id = "testApp";
                pkg.version = "1.0.0";
                packageList.push_back(pkg);
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    EXPECT_CALL(*packageManagerHandlerMock, Lock(_, _, _))
        .WillOnce(::testing::Return(Core::ERROR_GENERAL));

    string params = "{\"appId\":\"testApp\",\"intent\":\"{}\",\"launchArgs\":\"\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("launchApp"), params, response));
}

TEST_F(AppManagerTest, LaunchApp_SpawnAppFailed)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                Exchange::IPackageInstaller::Package pkg;
                pkg.id = "testApp";
                pkg.version = "1.0.0";
                packageList.push_back(pkg);
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    EXPECT_CALL(*packageManagerHandlerMock, Lock(_, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<2>(123),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, IsAppLoaded(_, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<1>(false),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, SpawnApp(_, _, _, _, _, _, _, _))
        .WillOnce(::testing::Return(Core::ERROR_GENERAL));

    string params = "{\"appId\":\"testApp\",\"intent\":\"{}\",\"launchArgs\":\"\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("launchApp"), params, response));
}

TEST_F(AppManagerTest, CloseApp_Success)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                Exchange::IPackageInstaller::Package pkg;
                pkg.id = "testApp";
                pkg.version = "1.0.0";
                packageList.push_back(pkg);
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    EXPECT_CALL(*packageManagerHandlerMock, Lock(_, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<2>(123),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, IsAppLoaded(_, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<1>(false),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, SpawnApp(_, _, _, _, _, _, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<5>("instance123"),
            ::testing::SetArgReferee<7>(true),
            ::testing::Return(Core::ERROR_NONE)));

    string launchParams = "{\"appId\":\"testApp\",\"intent\":\"{}\",\"launchArgs\":\"\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("launchApp"), launchParams, response));

    EXPECT_CALL(*lifecycleManagerMock, SetTargetAppState(_, _, _))
        .WillOnce(::testing::Return(Core::ERROR_NONE));

    string closeParams = "{\"appId\":\"testApp\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("closeApp"), closeParams, response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
}

TEST_F(AppManagerTest, CloseApp_InvalidParams_EmptyAppId)
{
    string params = "{\"appId\":\"\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("closeApp"), params, response));
}

TEST_F(AppManagerTest, CloseApp_AppNotLoaded)
{
    string params = "{\"appId\":\"unknownApp\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("closeApp"), params, response));
}

TEST_F(AppManagerTest, TerminateApp_Success)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                Exchange::IPackageInstaller::Package pkg;
                pkg.id = "testApp";
                pkg.version = "1.0.0";
                packageList.push_back(pkg);
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    EXPECT_CALL(*packageManagerHandlerMock, Lock(_, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<2>(123),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, IsAppLoaded(_, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<1>(false),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, SpawnApp(_, _, _, _, _, _, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<5>("instance123"),
            ::testing::SetArgReferee<7>(true),
            ::testing::Return(Core::ERROR_NONE)));

    string launchParams = "{\"appId\":\"testApp\",\"intent\":\"{}\",\"launchArgs\":\"\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("launchApp"), launchParams, response));

    EXPECT_CALL(*lifecycleManagerMock, UnloadApp(_, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<2>(true),
            ::testing::Return(Core::ERROR_NONE)));

    string terminateParams = "{\"appId\":\"testApp\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("terminateApp"), terminateParams, response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
}

TEST_F(AppManagerTest, TerminateApp_InvalidParams_EmptyAppId)
{
    string params = "{\"appId\":\"\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("terminateApp"), params, response));
}

TEST_F(AppManagerTest, TerminateApp_AppNotLoaded)
{
    string params = "{\"appId\":\"unknownApp\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("terminateApp"), params, response));
}

TEST_F(AppManagerTest, TerminateApp_UnloadFailed)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                Exchange::IPackageInstaller::Package pkg;
                pkg.id = "testApp";
                pkg.version = "1.0.0";
                packageList.push_back(pkg);
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    EXPECT_CALL(*packageManagerHandlerMock, Lock(_, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<2>(123),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, IsAppLoaded(_, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<1>(false),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, SpawnApp(_, _, _, _, _, _, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<5>("instance123"),
            ::testing::SetArgReferee<7>(true),
            ::testing::Return(Core::ERROR_NONE)));

    string launchParams = "{\"appId\":\"testApp\",\"intent\":\"{}\",\"launchArgs\":\"\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("launchApp"), launchParams, response));

    EXPECT_CALL(*lifecycleManagerMock, UnloadApp(_, _, _))
        .WillOnce(::testing::Return(Core::ERROR_GENERAL));

    string terminateParams = "{\"appId\":\"testApp\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("terminateApp"), terminateParams, response));
}

TEST_F(AppManagerTest, KillApp_Success)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                Exchange::IPackageInstaller::Package pkg;
                pkg.id = "testApp";
                pkg.version = "1.0.0";
                packageList.push_back(pkg);
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    EXPECT_CALL(*packageManagerHandlerMock, Lock(_, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<2>(123),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, IsAppLoaded(_, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<1>(false),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, SpawnApp(_, _, _, _, _, _, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<5>("instance123"),
            ::testing::SetArgReferee<7>(true),
            ::testing::Return(Core::ERROR_NONE)));

    string launchParams = "{\"appId\":\"testApp\",\"intent\":\"{}\",\"launchArgs\":\"\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("launchApp"), launchParams, response));

    EXPECT_CALL(*lifecycleManagerMock, KillApp(_, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<2>(true),
            ::testing::Return(Core::ERROR_NONE)));

    string killParams = "{\"appId\":\"testApp\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("killApp"), killParams, response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
}

TEST_F(AppManagerTest, KillApp_InvalidParams_EmptyAppId)
{
    string params = "{\"appId\":\"\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("killApp"), params, response));
}

TEST_F(AppManagerTest, KillApp_AppNotLoaded)
{
    string params = "{\"appId\":\"unknownApp\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("killApp"), params, response));
}

TEST_F(AppManagerTest, KillApp_KillFailed)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                Exchange::IPackageInstaller::Package pkg;
                pkg.id = "testApp";
                pkg.version = "1.0.0";
                packageList.push_back(pkg);
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    EXPECT_CALL(*packageManagerHandlerMock, Lock(_, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<2>(123),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, IsAppLoaded(_, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<1>(false),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, SpawnApp(_, _, _, _, _, _, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<5>("instance123"),
            ::testing::SetArgReferee<7>(true),
            ::testing::Return(Core::ERROR_NONE)));

    string launchParams = "{\"appId\":\"testApp\",\"intent\":\"{}\",\"launchArgs\":\"\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("launchApp"), launchParams, response));

    EXPECT_CALL(*lifecycleManagerMock, KillApp(_, _, _))
        .WillOnce(::testing::Return(Core::ERROR_GENERAL));

    string killParams = "{\"appId\":\"testApp\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("killApp"), killParams, response));
}

TEST_F(AppManagerTest, SendIntent_Success)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                Exchange::IPackageInstaller::Package pkg;
                pkg.id = "testApp";
                pkg.version = "1.0.0";
                packageList.push_back(pkg);
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    EXPECT_CALL(*packageManagerHandlerMock, Lock(_, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<2>(123),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, IsAppLoaded(_, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<1>(false),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, SpawnApp(_, _, _, _, _, _, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<5>("instance123"),
            ::testing::SetArgReferee<7>(true),
            ::testing::Return(Core::ERROR_NONE)));

    string launchParams = "{\"appId\":\"testApp\",\"intent\":\"{}\",\"launchArgs\":\"\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("launchApp"), launchParams, response));

    EXPECT_CALL(*lifecycleManagerMock, SendIntentToActiveApp(_, _, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<3>(true),
            ::testing::Return(Core::ERROR_NONE)));

    string sendIntentParams = "{\"appId\":\"testApp\",\"intent\":\"{\\\"action\\\":\\\"test\\\"}\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendIntent"), sendIntentParams, response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
}

TEST_F(AppManagerTest, SendIntent_InvalidParams_EmptyAppId)
{
    string params = "{\"appId\":\"\",\"intent\":\"{}\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("sendIntent"), params, response));
}

TEST_F(AppManagerTest, SendIntent_AppNotLoaded)
{
    string params = "{\"appId\":\"unknownApp\",\"intent\":\"{}\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("sendIntent"), params, response));
}

TEST_F(AppManagerTest, PreloadApp_Success)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                Exchange::IPackageInstaller::Package pkg;
                pkg.id = "testApp";
                pkg.version = "1.0.0";
                packageList.push_back(pkg);
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    EXPECT_CALL(*packageManagerHandlerMock, Lock(_, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<2>(123),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, SpawnApp(_, _, _, _, _, _, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<5>("instance123"),
            ::testing::SetArgReferee<7>(true),
            ::testing::Return(Core::ERROR_NONE)));

    string params = "{\"appId\":\"testApp\",\"launchArgs\":\"\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("preloadApp"), params, response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
}

TEST_F(AppManagerTest, PreloadApp_InvalidParams_EmptyAppId)
{
    string params = "{\"appId\":\"\",\"launchArgs\":\"\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("preloadApp"), params, response));
}

TEST_F(AppManagerTest, PreloadApp_PackageNotInstalled)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    string params = "{\"appId\":\"unknownApp\",\"launchArgs\":\"\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("preloadApp"), params, response));
}

TEST_F(AppManagerTest, StartSystemApp_Success)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                Exchange::IPackageInstaller::Package pkg;
                pkg.id = "systemApp";
                pkg.version = "1.0.0";
                packageList.push_back(pkg);
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    EXPECT_CALL(*packageManagerHandlerMock, Lock(_, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<2>(123),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, IsAppLoaded(_, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<1>(false),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, SpawnApp(_, _, _, _, _, _, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<5>("instance456"),
            ::testing::SetArgReferee<7>(true),
            ::testing::Return(Core::ERROR_NONE)));

    string params = "{\"appId\":\"systemApp\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startSystemApp"), params, response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
}

TEST_F(AppManagerTest, StartSystemApp_InvalidParams_EmptyAppId)
{
    string params = "{\"appId\":\"\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("startSystemApp"), params, response));
}

TEST_F(AppManagerTest, StopSystemApp_Success)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                Exchange::IPackageInstaller::Package pkg;
                pkg.id = "systemApp";
                pkg.version = "1.0.0";
                packageList.push_back(pkg);
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    EXPECT_CALL(*packageManagerHandlerMock, Lock(_, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<2>(123),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, IsAppLoaded(_, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<1>(false),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*lifecycleManagerMock, SpawnApp(_, _, _, _, _, _, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<5>("instance456"),
            ::testing::SetArgReferee<7>(true),
            ::testing::Return(Core::ERROR_NONE)));

    string startParams = "{\"appId\":\"systemApp\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("startSystemApp"), startParams, response));

    EXPECT_CALL(*lifecycleManagerMock, UnloadApp(_, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<2>(true),
            ::testing::Return(Core::ERROR_NONE)));

    string stopParams = "{\"appId\":\"systemApp\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("stopSystemApp"), stopParams, response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
}

TEST_F(AppManagerTest, StopSystemApp_InvalidParams_EmptyAppId)
{
    string params = "{\"appId\":\"\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("stopSystemApp"), params, response));
}

TEST_F(AppManagerTest, GetLoadedApps_Success)
{
    EXPECT_CALL(*lifecycleManagerMock, GetLoadedApps(_, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<1>("[{\"appId\":\"testApp\",\"appInstanceID\":\"instance123\",\"activeSessionId\":\"session1\",\"lifecycleState\":2,\"targetLifecycleState\":2}]"),
            ::testing::Return(Core::ERROR_NONE)));

    string params = "{}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getLoadedApps"), params, response));
}

TEST_F(AppManagerTest, GetLoadedApps_EmptyList)
{
    EXPECT_CALL(*lifecycleManagerMock, GetLoadedApps(_, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<1>("[]"),
            ::testing::Return(Core::ERROR_NONE)));

    string params = "{}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getLoadedApps"), params, response));
}

TEST_F(AppManagerTest, GetInstalledApps_Success)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                Exchange::IPackageInstaller::Package pkg;
                pkg.id = "testApp";
                pkg.version = "1.0.0";
                packageList.push_back(pkg);
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    string params = "{}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getInstalledApps"), params, response));
}

TEST_F(AppManagerTest, GetInstalledApps_EmptyList)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    string params = "{}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getInstalledApps"), params, response));
}

TEST_F(AppManagerTest, IsInstalled_True)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                Exchange::IPackageInstaller::Package pkg;
                pkg.id = "testApp";
                pkg.version = "1.0.0";
                packageList.push_back(pkg);
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    string params = "{\"appId\":\"testApp\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("isInstalled"), params, response));
}

TEST_F(AppManagerTest, IsInstalled_False)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    string params = "{\"appId\":\"unknownApp\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("isInstalled"), params, response));
}

TEST_F(AppManagerTest, IsInstalled_InvalidParams_EmptyAppId)
{
    string params = "{\"appId\":\"\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("isInstalled"), params, response));
}

TEST_F(AppManagerTest, ClearAppData_Success)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                Exchange::IPackageInstaller::Package pkg;
                pkg.id = "testApp";
                pkg.version = "1.0.0";
                packageList.push_back(pkg);
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    EXPECT_CALL(*storageManagerMock, ClearAppData(_))
        .WillOnce(::testing::Return(Core::ERROR_NONE));

    string params = "{\"appId\":\"testApp\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("clearAppData"), params, response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
}

TEST_F(AppManagerTest, ClearAppData_InvalidParams_EmptyAppId)
{
    string params = "{\"appId\":\"\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("clearAppData"), params, response));
}

TEST_F(AppManagerTest, ClearAppData_PackageNotInstalled)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    string params = "{\"appId\":\"unknownApp\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("clearAppData"), params, response));
}

TEST_F(AppManagerTest, ClearAllAppData_Success)
{
    EXPECT_CALL(*storageManagerMock, ClearAllAppData())
        .WillOnce(::testing::Return(Core::ERROR_NONE));

    string params = "{}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("clearAllAppData"), params, response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
}

TEST_F(AppManagerTest, GetAppMetadata_Success)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                Exchange::IPackageInstaller::Package pkg;
                pkg.id = "testApp";
                pkg.version = "1.0.0";
                packageList.push_back(pkg);
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    EXPECT_CALL(*packageManagerHandlerMock, Lock(_, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<2>(123),
            ::testing::Return(Core::ERROR_NONE)));

    EXPECT_CALL(*packageManagerHandlerMock, GetPackageMetadata(_, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<1>("{\"name\":\"testApp\"}"),
            ::testing::Return(Core::ERROR_NONE)));

    string params = "{\"appId\":\"testApp\",\"metaData\":\"manifest\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getAppMetadata"), params, response));
}

TEST_F(AppManagerTest, GetAppMetadata_InvalidParams_EmptyAppId)
{
    string params = "{\"appId\":\"\",\"metaData\":\"manifest\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getAppMetadata"), params, response));
}

TEST_F(AppManagerTest, GetAppMetadata_PackageNotInstalled)
{
    EXPECT_CALL(*packageManagerHandlerMock, GetInstalledPackages(_))
        .WillOnce(::testing::Invoke(
            [&](Exchange::IPackageInstaller::IPackageIterator*& packages) -> uint32_t {
                std::vector<Exchange::IPackageInstaller::Package> packageList;
                packages = Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
                return Core::ERROR_NONE;
            }));

    string params = "{\"appId\":\"unknownApp\",\"metaData\":\"manifest\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getAppMetadata"), params, response));
}

TEST_F(AppManagerTest, GetAppProperty_Success)
{
    EXPECT_CALL(*store2Mock, GetValue(_, _, _))
        .WillOnce(::testing::DoAll(
            ::testing::SetArgReferee<2>("propertyValue"),
            ::testing::Return(Core::ERROR_NONE)));

    string params = "{\"appId\":\"testApp\",\"key\":\"propertyKey\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getAppProperty"), params, response));
}

TEST_F(AppManagerTest, GetAppProperty_InvalidParams_EmptyAppId)
{
    string params = "{\"appId\":\"\",\"key\":\"propertyKey\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getAppProperty"), params, response));
}

TEST_F(AppManagerTest, GetAppProperty_InvalidParams_EmptyKey)
{
    string params = "{\"appId\":\"testApp\",\"key\":\"\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("getAppProperty"), params, response));
}

TEST_F(AppManagerTest, SetAppProperty_Success)
{
    EXPECT_CALL(*store2Mock, SetValue(_, _, _))
        .WillOnce(::testing::Return(Core::ERROR_NONE));

    string params = "{\"appId\":\"testApp\",\"key\":\"propertyKey\",\"value\":\"propertyValue\"}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setAppProperty"), params, response));
    EXPECT_EQ(response, _T("{\"success\":true}"));
}

TEST_F(AppManagerTest, SetAppProperty_InvalidParams_EmptyAppId)
{
    string params = "{\"appId\":\"\",\"key\":\"propertyKey\",\"value\":\"propertyValue\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setAppProperty"), params, response));
}

TEST_F(AppManagerTest, SetAppProperty_InvalidParams_EmptyKey)
{
    string params = "{\"appId\":\"testApp\",\"key\":\"\",\"value\":\"propertyValue\"}";
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("setAppProperty"), params, response));
}

TEST_F(AppManagerTest, GetMaxRunningApps_Success)
{
    string params = "{}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getMaxRunningApps"), params, response));
}

TEST_F(AppManagerTest, GetMaxHibernatedApps_Success)
{
    string params = "{}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getMaxHibernatedApps"), params, response));
}

TEST_F(AppManagerTest, GetMaxHibernatedFlashUsage_Success)
{
    string params = "{}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getMaxHibernatedFlashUsage"), params, response));
}

TEST_F(AppManagerTest, GetMaxInactiveRamUsage_Success)
{
    string params = "{}";
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getMaxInactiveRamUsage"), params, response));
}