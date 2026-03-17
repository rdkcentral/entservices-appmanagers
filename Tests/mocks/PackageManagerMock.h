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
#pragma once

#include <gmock/gmock.h>
#include <interfaces/IAppPackageManager.h>

using ::WPEFramework::Exchange::IPackageHandler;
using ::WPEFramework::Exchange::IPackageDownloader;
using ::WPEFramework::Exchange::IPackageInstaller;

class PackageDownloaderMock : public IPackageDownloader {
public:
    PackageDownloaderMock() = default;
    virtual ~PackageDownloaderMock() = default;

    MOCK_METHOD(WPEFramework::Core::hresult, Download, (const string& url, const WPEFramework::Exchange::IPackageDownloader::Options& options, WPEFramework::Exchange::IPackageDownloader::DownloadId& downloadId), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Pause, (const string& downloadId), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Resume, (const string& downloadId), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Cancel, (const string& downloadId), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Delete, (const string& fileLocator), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Progress, (const string& downloadId, WPEFramework::Exchange::IPackageDownloader::ProgressInfo& progress), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, GetStorageInformation, (uint32_t& quotaKB, uint32_t& usedKB), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, RateLimit, (const string& downloadId, const uint64_t& limit), (override));

    MOCK_METHOD(WPEFramework::Core::hresult, Register, (WPEFramework::Exchange::IPackageDownloader::INotification* notification), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Unregister, (WPEFramework::Exchange::IPackageDownloader::INotification* notification), (override));

    MOCK_METHOD(WPEFramework::Core::hresult, Initialize, (WPEFramework::PluginHost::IShell* service), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Deinitialize, (WPEFramework::PluginHost::IShell* service), (override));

    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfaceNummer), (override));
};

class PackageManagerMock : public IPackageHandler {
public:
    PackageManagerMock() = default;
    virtual ~PackageManagerMock() = default;

    MOCK_METHOD(WPEFramework::Core::hresult, Lock,(const string &packageId, const string &version, const LockReason &lockReason, uint32_t &lockId, string &unpackedPath, WPEFramework::Exchange::RuntimeConfig &configMetadata, IPackageHandler::ILockIterator*& appMetadata) , (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Unlock, (const string &packageId,const string &version), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, GetLockedInfo, (const string &packageId,const string &version,string &unpackedPath, WPEFramework::Exchange::RuntimeConfig &configMetadata, string &gatewayMetadataPath, bool &locked), (override));

    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfaceNummer), (override));
};


class PackageInstallerMock : public IPackageInstaller  {
public:
    PackageInstallerMock() = default;
    virtual ~PackageInstallerMock() = default;

    MOCK_METHOD(WPEFramework::Core::hresult, Register, (WPEFramework::Exchange::IPackageInstaller::INotification *sink), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Unregister, (WPEFramework::Exchange::IPackageInstaller::INotification *sink), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Install, (const string &packageId, const string &version, WPEFramework::Exchange::IPackageInstaller::IKeyValueIterator* const& additionalMetadata, const string &fileLocator, FailReason &failReason /* @out */), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Uninstall, (const string &packageId, string &errorReason), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, ListPackages, (WPEFramework::Exchange::IPackageInstaller::IPackageIterator*& packages), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Config, (const string &packageId, const string &version, WPEFramework::Exchange::RuntimeConfig &configMetadata), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, PackageState, (const string &packageId, const string &version, WPEFramework::Exchange::IPackageInstaller::InstallState &state), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, GetConfigForPackage, (const string &fileLocator, string& id, string &version, WPEFramework::Exchange::RuntimeConfig &config), (override));

    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfaceNummer), (override));

};

