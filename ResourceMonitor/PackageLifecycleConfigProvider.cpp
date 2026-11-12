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

#include "PackageLifecycleConfigProvider.h"
#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

    namespace {
        const char* const kPackageManagerCallsign = "org.rdk.AppPackageManager";

        bool IsConnectionError(const Core::hresult result)
        {
            return (result == Core::ERROR_CONNECTION_CLOSED) || (result == Core::ERROR_UNAVAILABLE) ||
                (result == Core::ERROR_TIMEDOUT);
        }
    }

    PackageManagerLifecycleSource::PackageManagerLifecycleSource(PluginHost::IShell* service)
        : _lock()
        , _service(service)
        , _installer(nullptr)
        , _handler(nullptr)
    {
        if (_service != nullptr) {
            _service->AddRef();
        }
    }

    PackageManagerLifecycleSource::~PackageManagerLifecycleSource()
    {
        if (_installer != nullptr) {
            _installer->Release();
        }
        if (_handler != nullptr) {
            _handler->Release();
        }
        if (_service != nullptr) {
            _service->Release();
        }
    }

    bool PackageManagerLifecycleSource::Acquire(Exchange::IPackageInstaller*& installer, Exchange::IPackageHandler*& handler)
    {
        installer = nullptr;
        handler = nullptr;
        Core::SafeSyncType<Core::CriticalSection> guard(_lock);
        if (_service == nullptr) {
            return false;
        }
        if (_installer == nullptr) {
            _installer = _service->QueryInterfaceByCallsign<Exchange::IPackageInstaller>(kPackageManagerCallsign);
        }
        if (_handler == nullptr) {
            _handler = _service->QueryInterfaceByCallsign<Exchange::IPackageHandler>(kPackageManagerCallsign);
        }
        if ((_installer == nullptr) || (_handler == nullptr)) {
            LOGWARN("%s is unavailable; lifecycle capabilities unavailable", kPackageManagerCallsign);
            return false;
        }
        _installer->AddRef();
        _handler->AddRef();
        installer = _installer;
        handler = _handler;
        return true;
    }

    void PackageManagerLifecycleSource::InvalidateOnConnectionError(const Core::hresult result)
    {
        if (!IsConnectionError(result)) {
            return;
        }
        Exchange::IPackageInstaller* installer = nullptr;
        Exchange::IPackageHandler* handler = nullptr;
        {
            Core::SafeSyncType<Core::CriticalSection> guard(_lock);
            installer = _installer;
            handler = _handler;
            _installer = nullptr;
            _handler = nullptr;
        }
        if (installer != nullptr) {
            installer->Release();
        }
        if (handler != nullptr) {
            handler->Release();
        }
    }

    Core::hresult PackageManagerLifecycleSource::ListPackages(std::vector<PackageEntry>& packages)
    {
        packages.clear();
        Exchange::IPackageInstaller* installer = nullptr;
        Exchange::IPackageHandler* handler = nullptr;
        if (!Acquire(installer, handler)) {
            return Core::ERROR_UNAVAILABLE;
        }
        handler->Release();

        Exchange::IPackageInstaller::IPackageIterator* iterator = nullptr;
        const Core::hresult result = installer->ListPackages(iterator);
        installer->Release();
        if ((result == Core::ERROR_NONE) && (iterator != nullptr)) {
            Exchange::IPackageInstaller::Package package;
            while (iterator->Next(package)) {
                packages.push_back(PackageEntry { package.packageId, package.version });
            }
        }
        if (iterator != nullptr) {
            iterator->Release();
        }
        InvalidateOnConnectionError(result);
        return result;
    }

    Core::hresult PackageManagerLifecycleSource::GetLockedInfo(const std::string& packageId, const std::string& version,
        bool& locked, std::string& runtimeConfigPayload)
    {
        locked = false;
        runtimeConfigPayload.clear();
        Exchange::IPackageInstaller* installer = nullptr;
        Exchange::IPackageHandler* handler = nullptr;
        if (!Acquire(installer, handler)) {
            return Core::ERROR_UNAVAILABLE;
        }
        installer->Release();

        std::string unpackedPath;
        std::string gatewayMetadataPath;
        const Core::hresult result = handler->GetLockedInfo(packageId, version, unpackedPath, runtimeConfigPayload,
            gatewayMetadataPath, locked);
        handler->Release();
        InvalidateOnConnectionError(result);
        return result;
    }

    PackageLifecycleConfigProvider::PackageLifecycleConfigProvider(std::unique_ptr<IPackageLifecycleSource> source)
        : _source(std::move(source))
    {
    }

    bool PackageLifecycleConfigProvider::ResolveLockedPayload(const std::string& appId, const std::string& version, std::string& payload)
    {
        if (!_source) {
            return false;
        }

        if (!version.empty()) {
            bool locked = false;
            const Core::hresult result = _source->GetLockedInfo(appId, version, locked, payload);
            if ((result != Core::ERROR_NONE) || !locked) {
                LOGWARN("No locked package for %s:%s (result=%u)", appId.c_str(), version.c_str(), result);
                return false;
            }
            return true;
        }

        std::vector<IPackageLifecycleSource::PackageEntry> packages;
        const Core::hresult listResult = _source->ListPackages(packages);
        if (listResult != Core::ERROR_NONE) {
            LOGWARN("Package list unavailable for %s (result=%u)", appId.c_str(), listResult);
            return false;
        }

        uint32_t lockedCount = 0;
        for (const auto& package : packages) {
            if (package.packageId != appId) {
                continue;
            }
            bool locked = false;
            std::string candidate;
            const Core::hresult result = _source->GetLockedInfo(package.packageId, package.version, locked, candidate);
            if (IsConnectionError(result)) {
                LOGWARN("Package Manager unavailable while resolving %s (result=%u)", appId.c_str(), result);
                return false;
            }
            if ((result == Core::ERROR_NONE) && locked) {
                ++lockedCount;
                payload = std::move(candidate);
            }
        }

        if (lockedCount == 1) {
            return true;
        }
        if (lockedCount == 0) {
            LOGINFO("No locked version of %s; lifecycle capabilities unavailable", appId.c_str());
        } else {
            LOGWARN("%u locked versions of %s; lifecycle capabilities unavailable without an explicit version",
                lockedCount, appId.c_str());
        }
        payload.clear();
        return false;
    }

    Core::hresult PackageLifecycleConfigProvider::GetLifecycleCapabilities(const std::string& appId,
        AppLifecycleCapabilities& capabilities, const std::string& version)
    {
        capabilities = AppLifecycleCapabilities();
        if (appId.empty()) {
            return Core::ERROR_INVALID_PARAMETER;
        }
        std::string payload;
        if (ResolveLockedPayload(appId, version, payload)) {
            AppLifecycleConfig::Parse(payload, capabilities);
        }
        return Core::ERROR_NONE;
    }

    Core::hresult PackageLifecycleConfigProvider::IsLifecycleStateSupported(const std::string& appId,
        const AppLifecycleState state, bool& supported, const std::string& version)
    {
        AppLifecycleCapabilities capabilities;
        const Core::hresult result = GetLifecycleCapabilities(appId, capabilities, version);
        supported = (result == Core::ERROR_NONE) && capabilities.IsSupported(state);
        return result;
    }

    Core::hresult PackageLifecycleConfigProvider::GetSuspendedStateConfig(const std::string& appId,
        SuspendedStateConfig& config, const std::string& version)
    {
        AppLifecycleCapabilities capabilities;
        const Core::hresult result = GetLifecycleCapabilities(appId, capabilities, version);
        config = capabilities.suspended;
        return result;
    }

} // namespace Plugin
} // namespace WPEFramework
