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

class ServiceMock final : public Thunder::PluginHost::IShell {
public:
    using InstantiateHandler = std::function<void*(const Thunder::RPC::Object&, const uint32_t, uint32_t&)>;

    struct Config {
        explicit Config(Thunder::Exchange::IPackageInstaller* installer = nullptr)
            : installer(installer)
            , configLine("{\"appPreinstallDirectory\":\"/tmp/preinstall\"}")
        {
        }

        Thunder::Exchange::IPackageInstaller* installer;
        std::string                                configLine;
    };

    // ── Inner COMLinkMock ─────────────────────────────────────────────────────
    class COMLinkMock final : public Thunder::PluginHost::IShell::ICOMLink {
    public:
        explicit COMLinkMock(ServiceMock& parent)
            : _parent(parent)
        {
        }

        ~COMLinkMock() override = default;

        void Register(Thunder::RPC::IRemoteConnection::INotification*) override
        {
        }

        void Unregister(const Thunder::RPC::IRemoteConnection::INotification*) override
        {
        }

        void Register(INotification*) override
        {
        }

        void Unregister(INotification*) override
        {
        }

        Thunder::RPC::IRemoteConnection* RemoteConnection(const uint32_t) override
        {
            return nullptr;
        }

        void* Instantiate(const Thunder::RPC::Object& object, const uint32_t interfaceId, uint32_t& connectionId) override
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
    uint32_t AddRef() const override
    {
        addRefCalls++;
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t Release() const override
    {
        releaseCalls++;
        const uint32_t newCount = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (0U == newCount) ? Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED
                                : Thunder::Core::ERROR_NONE;
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

    Thunder::Core::hresult Activate(const reason)          override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Deactivate(const reason)        override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Unavailable(const reason)       override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult ConfigLine(const std::string&)  override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult SystemRootPath(const std::string&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Hibernate(const uint32_t)       override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Startup(const startup)          override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Resumed(const bool)             override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Metadata(std::string& info) const override
    {
        info.clear();
        return Thunder::Core::ERROR_NONE;
    }

    void EnableWebServer(const std::string&, const std::string&) override {}
    void DisableWebServer() override {}

    Thunder::PluginHost::ISubSystem* SubSystems() override { return nullptr; }

    uint32_t Submit(const uint32_t, const Thunder::Core::ProxyType<Thunder::Core::JSON::IElement>&) override
    {
        return Thunder::Core::ERROR_NONE;
    }

    void Notify(const std::string&) override {}

    void* QueryInterface(const uint32_t) override { return nullptr; }

    void* QueryInterfaceByCallsign(const uint32_t id, const std::string& callsign) override
    {
        if (Thunder::Exchange::IPackageInstaller::ID == id
            && "org.rdk.AppPackageManager" == callsign
            && nullptr != _cfg.installer)
        {
            _cfg.installer->AddRef();
            return static_cast<Thunder::Exchange::IPackageInstaller*>(_cfg.installer);
        }
        return nullptr;
    }

    void Register(Thunder::PluginHost::IPlugin::INotification*) override {}
    void Unregister(Thunder::PluginHost::IPlugin::INotification*) override {}

    reason Reason() const override { return reason::REQUESTED; }

    Thunder::RPC::IRemoteConnection* RemoteConnection(const uint32_t) const
    {
        remoteConnectionCalls++;
        return nullptr;
    }

    Thunder::PluginHost::IShell::ICOMLink* COMLink() override
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

class FakePackageInstaller final : public Thunder::Exchange::IPackageInstaller {
public:
    using Package           = Thunder::Exchange::IPackageInstaller::Package;
    using FailReason        = Thunder::Exchange::IPackageInstaller::FailReason;
    using IPackageIterator  = Thunder::Exchange::IPackageInstaller::IPackageIterator;
    using IKeyValueIterator = Thunder::Exchange::IPackageInstaller::IKeyValueIterator;
    using INotification     = Thunder::Exchange::IPackageInstaller::INotification;
    using InstallState      = Thunder::Exchange::IPackageInstaller::InstallState;

    FakePackageInstaller()
        : _refCount(1)
    {
    }

    ~FakePackageInstaller() = default;
    FakePackageInstaller(const FakePackageInstaller&) = delete;
    FakePackageInstaller& operator=(const FakePackageInstaller&) = delete;

    uint32_t AddRef() const override
    {
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t Release() const override
    {
        const uint32_t r = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (r == 0u) ? Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED
                         : Thunder::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == Thunder::Exchange::IPackageInstaller::ID) {
            AddRef();
            return static_cast<Thunder::Exchange::IPackageInstaller*>(this);
        }
        return nullptr;
    }

    // IPackageInstaller methods ─────────────────────────────────────────────

    Thunder::Core::hresult GetConfigForPackage(const string& fileLocator,
                                                     string& id,
                                                     string& version,
                                                     string& config) override
    {
        getConfigCallCount++;
        if (getConfigHandler) {
            return getConfigHandler(fileLocator, id, version, config);
        }
        return Thunder::Core::ERROR_GENERAL;
    }

    Thunder::Core::hresult ListPackages(IPackageIterator*& packages) override
    {
        listPackagesCallCount++;
        if (listPackagesHandler) {
            return listPackagesHandler(packages);
        }
        packages = nullptr;
        return Thunder::Core::ERROR_GENERAL;
    }

    Thunder::Core::hresult Install(const string& packageId,
                                         const string& version,
                                         IKeyValueIterator* const& additionalMetadata,
                                         const string& fileLocator,
                                         FailReason& failReason) override
    {
        installCallCount++;
        if (installHandler) {
            return installHandler(packageId, version, additionalMetadata, fileLocator, failReason);
        }
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult Uninstall(const string& /*packageId*/,
                                           string& /*errorReason*/) override
    {
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult Config(const string& /*packageId*/,
                                        const string& /*version*/,
                                        string& /*configMetadata*/) override
    {
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult PackageState(const string& /*packageId*/,
                                              const string& /*version*/,
                                              InstallState& /*state*/) override
    {
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult Register(INotification* /*notification*/) override
    {
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult Unregister(INotification* /*notification*/) override
    {
        return Thunder::Core::ERROR_NONE;
    }

    // ── Observability ─────────────────────────────────────────────────────────
    mutable std::atomic<uint32_t> _refCount          { 1 };
    std::atomic<uint32_t>         getConfigCallCount  { 0 };
    std::atomic<uint32_t>         listPackagesCallCount{ 0 };
    std::atomic<uint32_t>         installCallCount    { 0 };

    // ── Configurable handlers ─────────────────────────────────────────────────
    std::function<Thunder::Core::hresult(const string&, string&, string&, string&)>
        getConfigHandler;

    std::function<Thunder::Core::hresult(IPackageIterator*&)>
        listPackagesHandler;

    std::function<Thunder::Core::hresult(const string&, const string&,
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
    : public Thunder::Exchange::IPreinstallManager::INotification {
public:
    FakePreinstallNotification()
        : _refCount(1)
    {
    }

    uint32_t AddRef() const override
    {
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t Release() const override
    {
        const uint32_t r = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (r == 0u) ? Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED
                         : Thunder::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == Thunder::Exchange::IPreinstallManager::INotification::ID) {
            AddRef();
            return static_cast<Thunder::Exchange::IPreinstallManager::INotification*>(this);
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
// Creates a real Thunder IPackageIterator from a list of Package structs,
// matching the pattern used in the production code and L1 tests.
// ─────────────────────────────────────────────────────────────────────────────
inline Thunder::Exchange::IPackageInstaller::IPackageIterator*
MakePackageIterator(std::list<Thunder::Exchange::IPackageInstaller::Package> pkgs)
{
    using IterType = Thunder::RPC::IteratorType<
        Thunder::Exchange::IPackageInstaller::IPackageIterator>;
    return Thunder::Core::Service<IterType>::Create<
        Thunder::Exchange::IPackageInstaller::IPackageIterator>(pkgs);
}

} // namespace L0Test
