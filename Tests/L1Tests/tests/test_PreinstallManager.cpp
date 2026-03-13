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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <atomic>
#include <cstring>
#include <dirent.h>
#include <future>
#include <list>
#include <string>
#include <thread>
#include <vector>

#include "COMLinkMock.h"
#include "FactoriesImplementation.h"
#include "Module.h"
#include "PackageManagerMock.h"
#include "PreinstallManager.h"
#include "PreinstallManagerImplementation.h"
#include "RemoteConnectionMock.h"
#include "ServiceMock.h"
#include "ThunderPortability.h"
#include "WorkerPoolImplementation.h"
#include "WrapsMock.h"

#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr)

#define PREINSTALL_MANAGER_TEST_PACKAGE_ID "com.test.preinstall.app"
#define PREINSTALL_MANAGER_TEST_VERSION "1.0.0"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgReferee;
using namespace WPEFramework;

class PreinstallManagerTest : public ::testing::Test {
protected:
    ServiceMock* mServiceMock = nullptr;
    PackageInstallerMock* mPackageInstallerMock = nullptr;
    WrapsImplMock* mWrapsImplMock = nullptr;

    FactoriesImplementation mFactoriesImplementation;
    PLUGINHOST_DISPATCHER* mDispatcher = nullptr;

    Core::ProxyType<Plugin::PreinstallManager> mPlugin;
    Plugin::PreinstallManagerImplementation* mPreinstallManagerImpl = nullptr;
    Exchange::IPackageInstaller::INotification* mPackageInstallerNotificationCb = nullptr;

    Core::ProxyType<WorkerPoolImplementation> mWorkerPool;

    std::vector<std::string> mDirectoryEntries;
    size_t mDirectoryEntryIndex = 0;

    PreinstallManagerTest()
        : mPlugin(Core::ProxyType<Plugin::PreinstallManager>::Create())
        , mWorkerPool(Core::ProxyType<WorkerPoolImplementation>::Create(2, Core::Thread::DefaultStackSize(), 16))
    {
        Core::IWorkerPool::Assign(&(*mWorkerPool));
        mWorkerPool->Run();
    }

    ~PreinstallManagerTest() override
    {
        TEST_LOG("Delete ~PreinstallManagerTest Instance");
        Core::IWorkerPool::Assign(nullptr);
        mWorkerPool.Release();
    }

    void TearDown() override
    {
        ReleaseResources();
    }

    void SetDirectoryEntries(const std::vector<std::string>& entries)
    {
        mDirectoryEntries = entries;
        mDirectoryEntryIndex = 0;

        ON_CALL(*mWrapsImplMock, opendir(_))
            .WillByDefault(Invoke([this](const char* pathname) -> DIR* {
                TEST_LOG("opendir path = %s", (nullptr == pathname) ? "null" : pathname);
                mDirectoryEntryIndex = 0;
                return reinterpret_cast<DIR*>(0x1);
            }));

        ON_CALL(*mWrapsImplMock, readdir(_))
            .WillByDefault(Invoke([this](DIR*) -> struct dirent* {
                static struct dirent entry;
                if (mDirectoryEntryIndex >= mDirectoryEntries.size()) {
                    return nullptr;
                }

                std::memset(&entry, 0, sizeof(entry));
                std::strncpy(entry.d_name,
                             mDirectoryEntries[mDirectoryEntryIndex].c_str(),
                             sizeof(entry.d_name) - 1);
                entry.d_type = DT_DIR;
                ++mDirectoryEntryIndex;
                return &entry;
            }));

        ON_CALL(*mWrapsImplMock, closedir(_))
            .WillByDefault(Return(0));
    }

    auto BuildPackageIterator(const std::vector<Exchange::IPackageInstaller::Package>& packages)
    {
        std::list<Exchange::IPackageInstaller::Package> packageList;
        for (const auto& package : packages) {
            packageList.emplace_back(package);
        }

        return Core::Service<RPC::IteratorType<Exchange::IPackageInstaller::IPackageIterator>>::Create<Exchange::IPackageInstaller::IPackageIterator>(packageList);
    }

    Core::hresult CreateResources(const std::string& configLine = R"({\"appPreinstallDirectory\":\"/tmp/preinstall\"})")
    {
        mServiceMock = new NiceMock<ServiceMock>;
        mPackageInstallerMock = new NiceMock<PackageInstallerMock>;
        mWrapsImplMock = new NiceMock<WrapsImplMock>;

        testing::Mock::AllowLeak(mServiceMock);
        testing::Mock::AllowLeak(mPackageInstallerMock);
        testing::Mock::AllowLeak(mWrapsImplMock);

        Wraps::setImpl(mWrapsImplMock);

        PluginHost::IFactories::Assign(&mFactoriesImplementation);

        ON_CALL(*mServiceMock, ConfigLine())
            .WillByDefault(Return(configLine));

        EXPECT_CALL(*mServiceMock, QueryInterfaceByCallsign(_, _))
            .Times(AnyNumber())
            .WillRepeatedly(Invoke(
                [this](const uint32_t id, const std::string& name) -> void* {
                    if (("org.rdk.PackageManagerRDKEMS" == name) && (Exchange::IPackageInstaller::ID == id)) {
                        return static_cast<void*>(static_cast<Exchange::IPackageInstaller*>(mPackageInstallerMock));
                    }
                    return nullptr;
                }));

        EXPECT_CALL(*mPackageInstallerMock, Register(_))
            .Times(AnyNumber())
            .WillRepeatedly(Invoke(
                [this](Exchange::IPackageInstaller::INotification* notification) {
                    mPackageInstallerNotificationCb = notification;
                    return Core::ERROR_NONE;
                }));

        ON_CALL(*mPackageInstallerMock, Unregister(_))
            .WillByDefault(Return(Core::ERROR_NONE));

        ON_CALL(*mWrapsImplMock, stat(_, _))
            .WillByDefault(Return(-1));

        const std::string message = mPlugin->Initialize(mServiceMock);
        if (!message.empty()) {
            return Core::ERROR_GENERAL;
        }

        mPreinstallManagerImpl = Plugin::PreinstallManagerImplementation::getInstance();
        return (nullptr != mPreinstallManagerImpl) ? Core::ERROR_NONE : Core::ERROR_GENERAL;
    }

    void ReleaseResources()
    {
        Wraps::setImpl(nullptr);

        if ((nullptr != mPlugin.operator->()) && (nullptr != mServiceMock)) {
            mPlugin->Deinitialize(mServiceMock);
        }

        if (nullptr != mDispatcher) {
            mDispatcher->Release();
            mDispatcher = nullptr;
        }

        // These are COM-style mock objects referenced through AddRef/Release
        // paths in plugin code; avoid raw deletes to prevent ownership races.
        mWrapsImplMock = nullptr;
        mPackageInstallerMock = nullptr;
        mServiceMock = nullptr;

        mPreinstallManagerImpl = nullptr;
        mPackageInstallerNotificationCb = nullptr;
    }
};

class MockNotificationTest : public Exchange::IPreinstallManager::INotification {
public:
    MockNotificationTest() = default;
    ~MockNotificationTest() override = default;

    MOCK_METHOD(void, OnAppInstallationStatus, (const std::string& jsonresponse), (override));
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));

    BEGIN_INTERFACE_MAP(MockNotificationTest)
    INTERFACE_ENTRY(Exchange::IPreinstallManager::INotification)
    END_INTERFACE_MAP
};

TEST_F(PreinstallManagerTest, CreatePreinstallManagerPlugin)
{
    EXPECT_TRUE(mPlugin.IsValid());
}

TEST_F(PreinstallManagerTest, RegisterNotification)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources());

    auto mockNotification = Core::ProxyType<MockNotificationTest>::Create();
    testing::Mock::AllowLeak(mockNotification.operator->());

    const Core::hresult status = mPreinstallManagerImpl->Register(mockNotification.operator->());
    EXPECT_EQ(Core::ERROR_NONE, status);

    EXPECT_EQ(Core::ERROR_NONE, mPreinstallManagerImpl->Unregister(mockNotification.operator->()));
    ReleaseResources();
}

TEST_F(PreinstallManagerTest, UnregisterNotification)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources());

    auto mockNotification = Core::ProxyType<MockNotificationTest>::Create();
    testing::Mock::AllowLeak(mockNotification.operator->());

    EXPECT_EQ(Core::ERROR_NONE, mPreinstallManagerImpl->Register(mockNotification.operator->()));
    EXPECT_EQ(Core::ERROR_NONE, mPreinstallManagerImpl->Unregister(mockNotification.operator->()));

    ReleaseResources();
}

TEST_F(PreinstallManagerTest, UnregisterNotificationNotFound)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources());

    auto mockNotification = Core::ProxyType<MockNotificationTest>::Create();
    testing::Mock::AllowLeak(mockNotification.operator->());

    EXPECT_EQ(Core::ERROR_GENERAL, mPreinstallManagerImpl->Unregister(mockNotification.operator->()));

    ReleaseResources();
}

TEST_F(PreinstallManagerTest, QueryInterface)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources());

    Exchange::IPreinstallManager* preinstallInterface =
        static_cast<Exchange::IPreinstallManager*>(mPreinstallManagerImpl->QueryInterface(Exchange::IPreinstallManager::ID));

    EXPECT_TRUE(nullptr != preinstallInterface);

    if (nullptr != preinstallInterface) {
        preinstallInterface->Release();
    }

    ReleaseResources();
}

TEST_F(PreinstallManagerTest, InformationReturnsEmptyString)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources());

    EXPECT_EQ(std::string(), mPlugin->Information());

    ReleaseResources();
}

TEST_F(PreinstallManagerTest, ConfigureWithNullServiceReturnsError)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources());

    EXPECT_EQ(Core::ERROR_GENERAL, mPreinstallManagerImpl->Configure(nullptr));

    ReleaseResources();
}

TEST_F(PreinstallManagerTest, PackageInstallerNotificationSinkForwardsStatus)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources());

    auto mockNotification = Core::ProxyType<MockNotificationTest>::Create();
    testing::Mock::AllowLeak(mockNotification.operator->());

    std::promise<void> notificationPromise;
    auto notificationFuture = notificationPromise.get_future();
    std::atomic<bool> promiseSet{false};

    EXPECT_CALL(*mockNotification, OnAppInstallationStatus(_))
        .Times(1)
        .WillOnce(Invoke([&notificationPromise, &promiseSet](const std::string&) {
            bool expected = false;
            if (promiseSet.compare_exchange_strong(expected, true)) {
                notificationPromise.set_value();
            }
        }));

    EXPECT_EQ(Core::ERROR_NONE, mPreinstallManagerImpl->Register(mockNotification.operator->()));

    // StartPreinstall triggers package manager object creation and callback registration.
    SetDirectoryEntries({".", ".."});
    EXPECT_EQ(Core::ERROR_NONE, mPreinstallManagerImpl->StartPreinstall(true));

    ASSERT_TRUE(nullptr != mPackageInstallerNotificationCb);
    mPackageInstallerNotificationCb->OnAppInstallationStatus(R"({"packageId":"sink.path.app","version":"1.0.0","status":"SUCCESS"})");

    EXPECT_EQ(std::future_status::ready,
              notificationFuture.wait_for(std::chrono::seconds(2)));

    EXPECT_EQ(Core::ERROR_NONE, mPreinstallManagerImpl->Unregister(mockNotification.operator->()));
}

TEST_F(PreinstallManagerTest, HandleAppInstallationStatusNotification)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources());

    auto mockNotification = Core::ProxyType<MockNotificationTest>::Create();
    testing::Mock::AllowLeak(mockNotification.operator->());

    std::promise<void> notificationPromise;
    auto notificationFuture = notificationPromise.get_future();
    std::atomic<bool> promiseSet{false};

    EXPECT_CALL(*mockNotification, OnAppInstallationStatus(_))
        .Times(1)
        .WillOnce(Invoke([&notificationPromise, &promiseSet](const std::string&) {
            bool expected = false;
            if (promiseSet.compare_exchange_strong(expected, true)) {
                notificationPromise.set_value();
            }
        }));

    EXPECT_EQ(Core::ERROR_NONE, mPreinstallManagerImpl->Register(mockNotification.operator->()));
    mPreinstallManagerImpl->handleOnAppInstallationStatus(R"({"packageId":"testApp","version":"1.0.0","status":"SUCCESS"})");

    EXPECT_EQ(std::future_status::ready,
              notificationFuture.wait_for(std::chrono::seconds(2)));

    EXPECT_EQ(Core::ERROR_NONE, mPreinstallManagerImpl->Unregister(mockNotification.operator->()));
}

TEST_F(PreinstallManagerTest, HandleAppInstallationStatusWithEmptyPayload)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources());

    auto mockNotification = Core::ProxyType<MockNotificationTest>::Create();
    testing::Mock::AllowLeak(mockNotification.operator->());

    EXPECT_CALL(*mockNotification, OnAppInstallationStatus(_)).Times(0);

    EXPECT_EQ(Core::ERROR_NONE, mPreinstallManagerImpl->Register(mockNotification.operator->()));
    mPreinstallManagerImpl->handleOnAppInstallationStatus("");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(Core::ERROR_NONE, mPreinstallManagerImpl->Unregister(mockNotification.operator->()));
    ReleaseResources();
}

TEST_F(PreinstallManagerTest, StartPreinstallWithForceInstallSuccess)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources());

    SetDirectoryEntries({".", "..", "app1", "app2"});

    EXPECT_CALL(*mPackageInstallerMock, GetConfigForPackage(_, _, _, _))
        .Times(2)
        .WillOnce(DoAll(
            SetArgReferee<1>("com.test.preinstall.app1"),
            SetArgReferee<2>("1.0.0"),
            Return(Core::ERROR_NONE)))
        .WillOnce(DoAll(
            SetArgReferee<1>("com.test.preinstall.app2"),
            SetArgReferee<2>("1.0.1"),
            Return(Core::ERROR_NONE)));

    EXPECT_CALL(*mPackageInstallerMock, Install(_, _, _, _, _))
        .Times(2)
        .WillRepeatedly(Invoke([](const std::string&,
                                  const std::string&,
                                  Exchange::IPackageInstaller::IKeyValueIterator* const&,
                                  const std::string&,
                                  Exchange::IPackageInstaller::FailReason&) {
            return Core::ERROR_NONE;
        }));

    EXPECT_EQ(Core::ERROR_NONE, mPreinstallManagerImpl->StartPreinstall(true));

    ReleaseResources();
}

TEST_F(PreinstallManagerTest, StartPreinstallWithForceInstallInstallFailure)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources());

    SetDirectoryEntries({"appFailure"});

    EXPECT_CALL(*mPackageInstallerMock, GetConfigForPackage(_, _, _, _))
        .WillOnce(DoAll(
            SetArgReferee<1>(PREINSTALL_MANAGER_TEST_PACKAGE_ID),
            SetArgReferee<2>(PREINSTALL_MANAGER_TEST_VERSION),
            Return(Core::ERROR_NONE)));

    EXPECT_CALL(*mPackageInstallerMock, Install(_, _, _, _, _))
        .WillOnce(Invoke([](const std::string&,
                            const std::string&,
                            Exchange::IPackageInstaller::IKeyValueIterator* const&,
                            const std::string&,
                            Exchange::IPackageInstaller::FailReason& failReason) {
            failReason = Exchange::IPackageInstaller::FailReason::SIGNATURE_VERIFICATION_FAILURE;
            return Core::ERROR_GENERAL;
        }));

    EXPECT_EQ(Core::ERROR_GENERAL, mPreinstallManagerImpl->StartPreinstall(true));

    ReleaseResources();
}

TEST_F(PreinstallManagerTest, StartPreinstallHandlesInvalidPackageFromGetConfig)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources());

    SetDirectoryEntries({"badApp"});

    EXPECT_CALL(*mPackageInstallerMock, GetConfigForPackage(_, _, _, _))
        .WillOnce(Invoke([](const std::string&,
                            std::string&,
                            std::string&,
                            WPEFramework::Exchange::RuntimeConfig&) {
            return Core::ERROR_GENERAL;
        }));

    EXPECT_CALL(*mPackageInstallerMock, Install(_, _, _, _, _)).Times(0);

    EXPECT_EQ(Core::ERROR_NONE, mPreinstallManagerImpl->StartPreinstall(true));

    ReleaseResources();
}

TEST_F(PreinstallManagerTest, StartPreinstallWithoutForceInstallSkipsEqualVersion)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources());

    SetDirectoryEntries({"app1"});

    Exchange::IPackageInstaller::Package installedPackage;
    installedPackage.packageId = PREINSTALL_MANAGER_TEST_PACKAGE_ID;
    installedPackage.version = PREINSTALL_MANAGER_TEST_VERSION;
    installedPackage.state = Exchange::IPackageInstaller::InstallState::INSTALLED;

    EXPECT_CALL(*mPackageInstallerMock, ListPackages(_))
        .WillOnce(Invoke([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
            auto iterator = BuildPackageIterator({installedPackage});
            packages = iterator;
            return Core::ERROR_NONE;
        }));

    EXPECT_CALL(*mPackageInstallerMock, GetConfigForPackage(_, _, _, _))
        .WillOnce(Invoke([](const std::string&,
                            std::string& id,
                            std::string& version,
                            WPEFramework::Exchange::RuntimeConfig&) {
            id = PREINSTALL_MANAGER_TEST_PACKAGE_ID;
            version = PREINSTALL_MANAGER_TEST_VERSION;
            return Core::ERROR_NONE;
        }));

    EXPECT_CALL(*mPackageInstallerMock, Install(_, _, _, _, _)).Times(0);

    EXPECT_EQ(Core::ERROR_NONE, mPreinstallManagerImpl->StartPreinstall(false));

    ReleaseResources();
}

TEST_F(PreinstallManagerTest, StartPreinstallWithoutForceInstallInstallsNewerVersion)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources());

    SetDirectoryEntries({"app1"});

    Exchange::IPackageInstaller::Package installedPackage;
    installedPackage.packageId = PREINSTALL_MANAGER_TEST_PACKAGE_ID;
    installedPackage.version = "0.9.0";
    installedPackage.state = Exchange::IPackageInstaller::InstallState::INSTALLED;

    EXPECT_CALL(*mPackageInstallerMock, ListPackages(_))
        .WillOnce(Invoke([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
            auto iterator = BuildPackageIterator({installedPackage});
            packages = iterator;
            return Core::ERROR_NONE;
        }));

    EXPECT_CALL(*mPackageInstallerMock, GetConfigForPackage(_, _, _, _))
        .WillOnce(Invoke([](const std::string&,
                            std::string& id,
                            std::string& version,
                            WPEFramework::Exchange::RuntimeConfig&) {
            id = PREINSTALL_MANAGER_TEST_PACKAGE_ID;
            version = PREINSTALL_MANAGER_TEST_VERSION;
            return Core::ERROR_NONE;
        }));

    EXPECT_CALL(*mPackageInstallerMock, Install(_, _, _, _, _))
        .Times(1)
        .WillOnce(Return(Core::ERROR_NONE));

    EXPECT_EQ(Core::ERROR_NONE, mPreinstallManagerImpl->StartPreinstall(false));

    ReleaseResources();
}

TEST_F(PreinstallManagerTest, StartPreinstallWithoutForceInstallInvalidInstalledVersion)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources());

    SetDirectoryEntries({"app1"});

    Exchange::IPackageInstaller::Package installedPackage;
    installedPackage.packageId = PREINSTALL_MANAGER_TEST_PACKAGE_ID;
    installedPackage.version = "abc";
    installedPackage.state = Exchange::IPackageInstaller::InstallState::INSTALLED;

    EXPECT_CALL(*mPackageInstallerMock, ListPackages(_))
        .WillOnce(Invoke([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
            auto iterator = BuildPackageIterator({installedPackage});
            packages = iterator;
            return Core::ERROR_NONE;
        }));

    EXPECT_CALL(*mPackageInstallerMock, GetConfigForPackage(_, _, _, _))
        .WillOnce(Invoke([](const std::string&,
                            std::string& id,
                            std::string& version,
                            WPEFramework::Exchange::RuntimeConfig&) {
            id = PREINSTALL_MANAGER_TEST_PACKAGE_ID;
            version = PREINSTALL_MANAGER_TEST_VERSION;
            return Core::ERROR_NONE;
        }));

    EXPECT_CALL(*mPackageInstallerMock, Install(_, _, _, _, _)).Times(0);

    EXPECT_EQ(Core::ERROR_NONE, mPreinstallManagerImpl->StartPreinstall(false));

    ReleaseResources();
}

TEST_F(PreinstallManagerTest, StartPreinstallWithoutForceInstallListPackagesFailure)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources());

    SetDirectoryEntries({"app1"});

    EXPECT_CALL(*mPackageInstallerMock, ListPackages(_))
        .WillOnce(Invoke([](Exchange::IPackageInstaller::IPackageIterator*& packages) {
            packages = nullptr;
            return Core::ERROR_GENERAL;
        }));

    EXPECT_CALL(*mPackageInstallerMock, GetConfigForPackage(_, _, _, _))
        .WillOnce(Invoke([](const std::string&,
                            std::string& id,
                            std::string& version,
                            WPEFramework::Exchange::RuntimeConfig&) {
            id = PREINSTALL_MANAGER_TEST_PACKAGE_ID;
            version = PREINSTALL_MANAGER_TEST_VERSION;
            return Core::ERROR_NONE;
        }));

    EXPECT_EQ(Core::ERROR_GENERAL, mPreinstallManagerImpl->StartPreinstall(false));

    ReleaseResources();
}

TEST_F(PreinstallManagerTest, StartPreinstallFailsWhenPreinstallDirectoryOpenFails)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources(R"({"appPreinstallDirectory":"/tmp/preinstall-negative"})"));

    EXPECT_CALL(*mWrapsImplMock, opendir(_))
        .WillRepeatedly(Return(nullptr));

    EXPECT_CALL(*mPackageInstallerMock, Install(_, _, _, _, _)).Times(0);

    EXPECT_EQ(Core::ERROR_GENERAL, mPreinstallManagerImpl->StartPreinstall(true));

    ReleaseResources();
}

TEST_F(PreinstallManagerTest, StartPreinstallWithoutForceInstallReleasesIteratorOnListFailure)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources());

    SetDirectoryEntries({"app1"});

    Exchange::IPackageInstaller::Package installedPackage;
    installedPackage.packageId = PREINSTALL_MANAGER_TEST_PACKAGE_ID;
    installedPackage.version = PREINSTALL_MANAGER_TEST_VERSION;
    installedPackage.state = Exchange::IPackageInstaller::InstallState::INSTALLED;

    auto iterator = BuildPackageIterator({installedPackage});

    EXPECT_CALL(*mPackageInstallerMock, ListPackages(_))
        .WillOnce(Invoke([&](Exchange::IPackageInstaller::IPackageIterator*& packages) {
            packages = iterator;
            return Core::ERROR_GENERAL;
        }));

    EXPECT_EQ(Core::ERROR_GENERAL, mPreinstallManagerImpl->StartPreinstall(false));

    ReleaseResources();
}

TEST_F(PreinstallManagerTest, StartPreinstallInstallFailureWithUnknownFailReason)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources());

    SetDirectoryEntries({"appUnknownFailReason"});

    EXPECT_CALL(*mPackageInstallerMock, GetConfigForPackage(_, _, _, _))
        .WillOnce(DoAll(
            SetArgReferee<1>(PREINSTALL_MANAGER_TEST_PACKAGE_ID),
            SetArgReferee<2>(PREINSTALL_MANAGER_TEST_VERSION),
            Return(Core::ERROR_NONE)));

    EXPECT_CALL(*mPackageInstallerMock, Install(_, _, _, _, _))
        .WillOnce(Invoke([](const std::string&,
                            const std::string&,
                            Exchange::IPackageInstaller::IKeyValueIterator* const&,
                            const std::string&,
                            Exchange::IPackageInstaller::FailReason& failReason) {
            failReason = static_cast<Exchange::IPackageInstaller::FailReason>(255);
            return Core::ERROR_GENERAL;
        }));

    EXPECT_EQ(Core::ERROR_GENERAL, mPreinstallManagerImpl->StartPreinstall(true));

    ReleaseResources();
}

TEST_F(PreinstallManagerTest, StartPreinstallFailsWhenPackageManagerUnavailable)
{
    mServiceMock = new NiceMock<ServiceMock>;

    EXPECT_CALL(*mServiceMock, QueryInterfaceByCallsign(_, _))
        .WillRepeatedly(Return(nullptr));

    EXPECT_EQ(std::string(""), mPlugin->Initialize(mServiceMock));

    mPreinstallManagerImpl = Plugin::PreinstallManagerImplementation::getInstance();
    ASSERT_NE(nullptr, mPreinstallManagerImpl);

    EXPECT_EQ(Core::ERROR_GENERAL, mPreinstallManagerImpl->StartPreinstall(true));

    mPlugin->Deinitialize(mServiceMock);
    delete mServiceMock;
    mServiceMock = nullptr;
    mPreinstallManagerImpl = nullptr;
}

TEST_F(PreinstallManagerTest, DeinitializeWithRemoteConnectionObject)
{
    ASSERT_EQ(Core::ERROR_NONE, CreateResources());

    auto comLinkMock = new NiceMock<COMLinkMock>;
    auto remoteConnectionMock = new NiceMock<RemoteConnectionMock>;

    EXPECT_CALL(*mServiceMock, COMLink())
        .Times(AnyNumber())
        .WillRepeatedly(Return(comLinkMock));

    EXPECT_CALL(*comLinkMock, RemoteConnection(_))
        .Times(AnyNumber())
        .WillRepeatedly(Return(remoteConnectionMock));

    EXPECT_CALL(*remoteConnectionMock, Release())
        .Times(AnyNumber())
        .WillRepeatedly(Return(0));

    ReleaseResources();

    delete remoteConnectionMock;
    delete comLinkMock;
}
