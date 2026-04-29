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

/**
 * @file ServiceMock.h
 *
 * Minimal PluginHost::IShell mock for PreinstallManager L0 tests.
 * Provides:
 *  - L0Test::ServiceMock   - full IShell surface with observable counters and a
 *                            configurable Instantiate handler for lifecycle tests.
 *  - L0Test::FakePackageInstaller - configurable IPackageInstaller stub whose
 *                            behaviour can be controlled via std::function handlers.
 *  - L0Test::FakePreinstallNotification - counting IPreinstallManager::INotification.
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <list>
#include <string>

#include <core/core.h>
#include <plugins/plugins.h>
#include <plugins/IShell.h>
#include <interfaces/IPreinstallManager.h>
#include <interfaces/IAppPackageManager.h>

namespace L0Test {

using string = std::string;

// ─────────────────────────────────────────────────────────────────────────────
// ServiceMock — full PluginHost::IShell implementation for L0 tests
// ─────────────────────────────────────────────────────────────────────────────

class ServiceMock final : public WPEFramework::PluginHost::IShell {
public:
    using InstantiateHandler = std::function<void*(const WPEFramework::RPC::Object&, const uint32_t, uint32_t&)>;

    struct Config {
        explicit Config(WPEFramework::Exchange::IPackageInstaller* installer = nullptr)
            : installer(installer)
            , configLine("{\"appPreinstallDirectory\":\"/tmp/preinstall\"}")
        {
        }

        WPEFramework::Exchange::IPackageInstaller* installer;
        std::string                                configLine;
    };

    // ── Inner COMLinkMock ─────────────────────────────────────────────────────
    class COMLinkMock final : public WPEFramework::PluginHost::IShell::ICOMLink {
    public:
        explicit COMLinkMock(ServiceMock& parent)
            : _parent(parent)
        {
        }

        ~COMLinkMock() override = default;

        void Register(WPEFramework::RPC::IRemoteConnection::INotification*) override
        {
        }

        void Unregister(const WPEFramework::RPC::IRemoteConnection::INotification*) override
        {
        }

        void Register(INotification*) override
        {
        }

        void Unregister(INotification*) override
        {
        }

        WPEFramework::RPC::IRemoteConnection* RemoteConnection(const uint32_t) override
        {
            return nullptr;
        }

        void* Instantiate(const WPEFramework::RPC::Object& object, const uint32_t interfaceId, uint32_t& connectionId) override
        {
            _parent.instantiateCalls++;
            if (_parent._instantiateHandler) {
                return _parent._instantiateHandler(object, interfaceId, connectionId);
            }
            connectionId = 0;
            return nullptr;
        }

    private:
        ServiceMock& _parent;
    };

    // ── Construction / destruction ────────────────────────────────────────────
    explicit ServiceMock(Config cfg = Config())
        : _refCount(1)
        , _cfg(cfg)
        , _comLink(*this)
    {
    }

    ~ServiceMock() override = default;
    ServiceMock(const ServiceMock&) = delete;
    ServiceMock& operator=(const ServiceMock&) = delete;

    void SetInstantiateHandler(InstantiateHandler handler)
    {
        _instantiateHandler = std::move(handler);
    }

    // ── Observability counters ────────────────────────────────────────────────
    mutable std::atomic<uint32_t> addRefCalls    { 0 };
    mutable std::atomic<uint32_t> releaseCalls   { 0 };
    mutable std::atomic<uint32_t> remoteConnectionCalls { 0 };
    mutable std::atomic<uint32_t> instantiateCalls      { 0 };

    // ── IUnknown ──────────────────────────────────────────────────────────────
    void AddRef() const override
    {
        addRefCalls++;
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        releaseCalls++;
        const uint32_t newCount = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (0U == newCount) ? WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED
                                : WPEFramework::Core::ERROR_NONE;
    }

    // ── IShell surface — identity / paths ────────────────────────────────────
    std::string Versions()       const override { return ""; }
    std::string Locator()        const override { return ""; }
    std::string ClassName()      const override { return ""; }
    std::string Callsign()       const override { return "org.rdk.PreinstallManager"; }
    std::string WebPrefix()      const override { return ""; }
    std::string ConfigLine()     const override { return _cfg.configLine; }
    std::string PersistentPath() const override { return "/tmp"; }
    std::string VolatilePath()   const override { return "/tmp"; }
    std::string DataPath()       const override { return "/tmp"; }
    std::string SystemPath()     const override { return ""; }
    std::string PluginPath()     const override { return ""; }
    std::string SystemRootPath() const override { return ""; }
    std::string Model()          const override { return ""; }
    std::string Accessor()       const override { return ""; }
    std::string ProxyStubPath()  const override { return ""; }
    std::string HashKey()        const override { return ""; }
    std::string Substitute(const std::string&) const override { return ""; }

    state State() const override { return state::ACTIVATED; }
    bool  Resumed()    const override { return false; }
    bool  Background() const override { return false; }
    bool  IsSupported(const uint8_t) const override { return false; }

    startup Startup() const override { return static_cast<startup>(0); }

    WPEFramework::Core::hresult Activate(const reason)          override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Deactivate(const reason)        override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Unavailable(const reason)       override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult ConfigLine(const std::string&)  override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SystemRootPath(const std::string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Hibernate(const uint32_t)       override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Startup(const startup)          override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Resumed(const bool)             override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Metadata(std::string& info) const override
    {
        info.clear();
        return WPEFramework::Core::ERROR_NONE;
    }

    void EnableWebServer(const std::string&, const std::string&) override {}
    void DisableWebServer() override {}

    WPEFramework::PluginHost::ISubSystem* SubSystems() override { return nullptr; }

    uint32_t Submit(const uint32_t, const WPEFramework::Core::ProxyType<WPEFramework::Core::JSON::IElement>&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    void Notify(const std::string&) override {}

    void* QueryInterface(const uint32_t) override { return nullptr; }

    void* QueryInterfaceByCallsign(const uint32_t id, const std::string& callsign) override
    {
        if (WPEFramework::Exchange::IPackageInstaller::ID == id
            && "org.rdk.PackageManagerRDKEMS" == callsign
            && nullptr != _cfg.installer)
        {
            _cfg.installer->AddRef();
            return static_cast<WPEFramework::Exchange::IPackageInstaller*>(_cfg.installer);
        }
        return nullptr;
    }

    void Register(WPEFramework::PluginHost::IPlugin::INotification*) override {}
    void Unregister(WPEFramework::PluginHost::IPlugin::INotification*) override {}

    reason Reason() const override { return reason::REQUESTED; }

    WPEFramework::RPC::IRemoteConnection* RemoteConnection(const uint32_t) const
    {
        remoteConnectionCalls++;
        return nullptr;
    }

    WPEFramework::PluginHost::IShell::ICOMLink* COMLink() override
    {
        return &_comLink;
    }

private:
    mutable std::atomic<uint32_t> _refCount;
    InstantiateHandler             _instantiateHandler;
    Config                         _cfg;
    COMLinkMock                    _comLink;
};

// ─────────────────────────────────────────────────────────────────────────────
// FakePackageInstaller
//
// All methods are no-ops by default. Per-test behaviour is set via
// std::function handlers so each test can configure the fake independently.
// ─────────────────────────────────────────────────────────────────────────────

class FakePackageInstaller final : public WPEFramework::Exchange::IPackageInstaller {
public:
    using Package           = WPEFramework::Exchange::IPackageInstaller::Package;
    using FailReason        = WPEFramework::Exchange::IPackageInstaller::FailReason;
    using IPackageIterator  = WPEFramework::Exchange::IPackageInstaller::IPackageIterator;
    using IKeyValueIterator = WPEFramework::Exchange::IPackageInstaller::IKeyValueIterator;
    using INotification     = WPEFramework::Exchange::IPackageInstaller::INotification;
    using RuntimeConfig     = WPEFramework::Exchange::RuntimeConfig;
    using InstallState      = WPEFramework::Exchange::IPackageInstaller::InstallState;

    FakePackageInstaller()
        : _refCount(1)
    {
    }

    ~FakePackageInstaller() = default;
    FakePackageInstaller(const FakePackageInstaller&) = delete;
    FakePackageInstaller& operator=(const FakePackageInstaller&) = delete;

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t r = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (r == 0u) ? WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED
                         : WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IPackageInstaller::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IPackageInstaller*>(this);
        }
        return nullptr;
    }

    // IPackageInstaller methods ─────────────────────────────────────────────

    WPEFramework::Core::hresult GetConfigForPackage(const string& fileLocator,
                                                     string& id,
                                                     string& version,
                                                     RuntimeConfig& config) override
    {
        getConfigCallCount++;
        if (getConfigHandler) {
            return getConfigHandler(fileLocator, id, version, config);
        }
        return WPEFramework::Core::ERROR_GENERAL;
    }

    WPEFramework::Core::hresult ListPackages(IPackageIterator*& packages) override
    {
        listPackagesCallCount++;
        if (listPackagesHandler) {
            return listPackagesHandler(packages);
        }
        packages = nullptr;
        return WPEFramework::Core::ERROR_GENERAL;
    }

    WPEFramework::Core::hresult Install(const string& packageId,
                                         const string& version,
                                         IKeyValueIterator* const& additionalMetadata,
                                         const string& fileLocator,
                                         FailReason& failReason) override
    {
        installCallCount++;
        if (installHandler) {
            return installHandler(packageId, version, additionalMetadata, fileLocator, failReason);
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Uninstall(const string& /*packageId*/,
                                           string& /*errorReason*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Config(const string& /*packageId*/,
                                        const string& /*version*/,
                                        RuntimeConfig& /*configMetadata*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult PackageState(const string& /*packageId*/,
                                              const string& /*version*/,
                                              InstallState& /*state*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Register(INotification* /*notification*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unregister(INotification* /*notification*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    // ── Observability ─────────────────────────────────────────────────────────
    mutable std::atomic<uint32_t> _refCount          { 1 };
    std::atomic<uint32_t>         getConfigCallCount  { 0 };
    std::atomic<uint32_t>         listPackagesCallCount{ 0 };
    std::atomic<uint32_t>         installCallCount    { 0 };

    // ── Configurable handlers ─────────────────────────────────────────────────
    std::function<WPEFramework::Core::hresult(const string&, string&, string&, RuntimeConfig&)>
        getConfigHandler;

    std::function<WPEFramework::Core::hresult(IPackageIterator*&)>
        listPackagesHandler;

    std::function<WPEFramework::Core::hresult(const string&, const string&,
                                               IKeyValueIterator* const&,
                                               const string&, FailReason&)>
        installHandler;
};

// ─────────────────────────────────────────────────────────────────────────────
// FakePreinstallNotification
//
// Counts calls to OnPreinstallationComplete and OnAppInstallationStatus.
// ─────────────────────────────────────────────────────────────────────────────

class FakePreinstallNotification final
    : public WPEFramework::Exchange::IPreinstallManager::INotification {
public:
    FakePreinstallNotification()
        : _refCount(1)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t r = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (r == 0u) ? WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED
                         : WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IPreinstallManager::INotification::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IPreinstallManager::INotification*>(this);
        }
        return nullptr;
    }

    void OnPreinstallationComplete() override
    {
        onPreinstallationCompleteCount++;
    }

    void OnAppInstallationStatus(const string& jsonresponse) override
    {
        onAppInstallationStatusCount++;
        lastStatusJson = jsonresponse;
    }

    // ── Observability ─────────────────────────────────────────────────────────
    mutable std::atomic<uint32_t> _refCount                     { 1 };
    std::atomic<uint32_t>         onPreinstallationCompleteCount { 0 };
    std::atomic<uint32_t>         onAppInstallationStatusCount   { 0 };
    std::string                   lastStatusJson;
};

// ─────────────────────────────────────────────────────────────────────────────
// Helper: MakePackageIterator
//
// Creates a real WPEFramework IPackageIterator from a list of Package structs,
// matching the pattern used in the production code and L1 tests.
// ─────────────────────────────────────────────────────────────────────────────
inline WPEFramework::Exchange::IPackageInstaller::IPackageIterator*
MakePackageIterator(std::list<WPEFramework::Exchange::IPackageInstaller::Package> pkgs)
{
    using IterType = WPEFramework::RPC::IteratorType<
        WPEFramework::Exchange::IPackageInstaller::IPackageIterator>;
    return WPEFramework::Core::Service<IterType>::Create<
        WPEFramework::Exchange::IPackageInstaller::IPackageIterator>(pkgs);
}

} // namespace L0Test
