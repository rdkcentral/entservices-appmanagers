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

#pragma once

#include "Module.h"
#include "AppLifecycleConfig.h"
#include <interfaces/IAppPackageManager.h>

#include <memory>
#include <string>
#include <vector>

namespace WPEFramework {
namespace Plugin {

    // Access to the Package Manager data needed to resolve lifecycle capabilities.
    // Implementations must be safe to call from several threads at once.
    class IPackageLifecycleSource {
    public:
        struct PackageEntry {
            std::string packageId;
            std::string version;
        };

        virtual ~IPackageLifecycleSource() = default;
        virtual Core::hresult ListPackages(std::vector<PackageEntry>& packages) = 0;
        virtual Core::hresult GetLockedInfo(const std::string& packageId, const std::string& version,
            bool& locked, std::string& runtimeConfigPayload) = 0;
    };

    // Production source: org.rdk.AppPackageManager over COM-RPC, connected lazily on first use.
    class PackageManagerLifecycleSource : public IPackageLifecycleSource {
    public:
        explicit PackageManagerLifecycleSource(PluginHost::IShell* service);
        ~PackageManagerLifecycleSource() override;

        PackageManagerLifecycleSource(const PackageManagerLifecycleSource&) = delete;
        PackageManagerLifecycleSource& operator=(const PackageManagerLifecycleSource&) = delete;

        Core::hresult ListPackages(std::vector<PackageEntry>& packages) override;
        Core::hresult GetLockedInfo(const std::string& packageId, const std::string& version,
            bool& locked, std::string& runtimeConfigPayload) override;

    private:
        bool Acquire(Exchange::IPackageInstaller*& installer, Exchange::IPackageHandler*& handler);
        void InvalidateOnConnectionError(const Core::hresult result);

        Core::CriticalSection _lock;
        PluginHost::IShell* _service;
        Exchange::IPackageInstaller* _installer;
        Exchange::IPackageHandler* _handler;
    };

    // Resolves an application's lifecycle capabilities from Package Manager.
    //
    // Usage contract (see the app-lifecycle-capabilities spec and design):
    // - Safe to call from several threads; never takes ResourceMonitorImplementation locks, so callers
    //   must not hold _adminLock, _reconcileMutex or _reactiveMonitorMutex while calling.
    // - appId is used unchanged as the Package Manager packageId. Without a version, exactly one locked
    //   version must exist; otherwise the result is UNAVAILABLE.
    // - Enforcement should snapshot the result per appInstanceId at launch and treat UNAVAILABLE as
    //   "do not enforce" (legacy packages, Package Manager outages, ambiguous versions).
    // - Every outcome returns Core::ERROR_NONE with the matching status, except an empty appId,
    //   which returns Core::ERROR_INVALID_PARAMETER.
    class PackageLifecycleConfigProvider {
    public:
        explicit PackageLifecycleConfigProvider(std::unique_ptr<IPackageLifecycleSource> source);

        PackageLifecycleConfigProvider(const PackageLifecycleConfigProvider&) = delete;
        PackageLifecycleConfigProvider& operator=(const PackageLifecycleConfigProvider&) = delete;

        Core::hresult GetLifecycleCapabilities(const std::string& appId, AppLifecycleCapabilities& capabilities,
            const std::string& version = std::string());
        Core::hresult IsLifecycleStateSupported(const std::string& appId, const AppLifecycleState state,
            bool& supported, const std::string& version = std::string());
        Core::hresult GetSuspendedStateConfig(const std::string& appId, SuspendedStateConfig& config,
            const std::string& version = std::string());

    private:
        bool ResolveLockedPayload(const std::string& appId, const std::string& version, std::string& payload);

        std::unique_ptr<IPackageLifecycleSource> _source;
    };

} // namespace Plugin
} // namespace WPEFramework
