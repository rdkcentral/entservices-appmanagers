/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
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
 */

#pragma once

/*
 * L0 test ServiceMock for PreinstallManager:
 * - Implements PluginHost::IShell so the plugin can Initialize/Deinitialize.
 * - Implements PluginHost::IShell::ICOMLink so IShell::Root<T>() can instantiate
 *   an in-proc PreinstallManagerImplementation fake via COMLink()->Instantiate(...).
 * - Provides a configurable IPackageInstaller fake for StartPreinstall tests.
 *
 * IMPORTANT:
 * - Release() does NOT delete 'this'; ServiceMock is stack-allocated in tests.
 * - Use selfDelete=true only for heap-allocated shells.
 */

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>

#include <core/core.h>
#include <plugins/plugins.h>
#include <plugins/IShell.h>
#include <interfaces/IPreinstallManager.h>
#include <interfaces/IAppPackageManager.h>
#include <interfaces/IConfiguration.h>

namespace L0Test {

using string = std::string;

// ─────────────────────────────────────────────────────────────────────────────
// Fake IPackageInstaller::IPackageIterator
// ─────────────────────────────────────────────────────────────────────────────

class FakePackageIterator : public WPEFramework::Exchange::IPackageInstaller::IPackageIterator {
public:
    using Package = WPEFramework::Exchange::IPackageInstaller::Package;

    explicit FakePackageIterator(std::vector<Package> packages)
        : _packages(std::move(packages))
        , _index(0)
        , _refCount(1)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t n = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == n) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IPackageInstaller::IPackageIterator::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IPackageInstaller::IPackageIterator*>(this);
        }
        return nullptr;
    }

    bool Next(Package& out) override
    {
        if (_index < _packages.size()) {
            out = _packages[_index++];
            return true;
        }
        return false;
    }

    bool Previous(Package& out) override
    {
        if (_index == 0) {
            return false;
        }
        --_index;
        out = _packages[_index];
        return true;
    }

    void Reset(const uint32_t pos) override
    {
        _index = (pos < static_cast<uint32_t>(_packages.size())) ? pos
                                                                  : static_cast<uint32_t>(_packages.size());
    }

    bool IsValid() const override
    {
        return (_index > 0) && (_index <= static_cast<uint32_t>(_packages.size()));
    }

    uint32_t Count() const override
    {
        return static_cast<uint32_t>(_packages.size());
    }

    Package Current() const override
    {
        if (_index > 0 && _index <= static_cast<uint32_t>(_packages.size())) {
            return _packages[_index - 1];
        }
        return Package {};
    }

private:
    std::vector<Package> _packages;
    uint32_t _index;
    mutable std::atomic<uint32_t> _refCount;
};

// ─────────────────────────────────────────────────────────────────────────────
// Fake IPackageInstaller
// ─────────────────────────────────────────────────────────────────────────────

class FakePackageInstaller : public WPEFramework::Exchange::IPackageInstaller {
public:
    using InstallState  = WPEFramework::Exchange::IPackageInstaller::InstallState;
    using FailReason    = WPEFramework::Exchange::IPackageInstaller::FailReason;
    using Package       = WPEFramework::Exchange::IPackageInstaller::Package;

    struct InstallCall {
        std::string packageId;
        std::string version;
        std::string fileLocator;
    };

    FakePackageInstaller()
        : installCalls(0)
        , registerCalls(0)
        , unregisterCalls(0)
        , _refCount(1)
        , _listPackagesResult(WPEFramework::Core::ERROR_NONE)
        , _installResult(WPEFramework::Core::ERROR_NONE)
        , _installFailReason(FailReason::NONE)
        , _getConfigResult(WPEFramework::Core::ERROR_NONE)
    {
    }

    ~FakePackageInstaller() override = default;

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t n = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == n) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IPackageInstaller::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IPackageInstaller*>(this);
        }
        return nullptr;
    }

    // ── Controllable settings ──────────────────────────────────────────────

    void SetListPackagesResult(WPEFramework::Core::hresult r) { _listPackagesResult = r; }
    void SetListPackagesPackages(std::vector<Package> pkgs)   { _listPackagesPackages = std::move(pkgs); }
    void SetInstallResult(WPEFramework::Core::hresult r)      { _installResult = r; }
    void SetInstallFailReason(FailReason fr)                  { _installFailReason = fr; }
    void SetGetConfigResult(WPEFramework::Core::hresult r)    { _getConfigResult = r; }
    void SetGetConfigPackageId(const std::string& id)         { _getConfigPackageId = id; }
    void SetGetConfigVersion(const std::string& v)            { _getConfigVersion = v; }
    // Set to true to return null iterator even when ListPackages returns NONE
    void SetNullIterator(bool n)                              { _nullIterator = n; }

    // ── IPackageInstaller interface ────────────────────────────────────────

    WPEFramework::Core::hresult Install(
        const std::string& packageId,
        const std::string& version,
        IKeyValueIterator* const& /*additionalMetadata*/,
        const std::string& fileLocator,
        FailReason& outFailReason) override
    {
        installCalls++;
        InstallCall c;
        c.packageId   = packageId;
        c.version     = version;
        c.fileLocator = fileLocator;
        lastInstallCalls.push_back(c);
        outFailReason = _installFailReason;
        return _installResult;
    }

    WPEFramework::Core::hresult ListPackages(IPackageIterator*& packages) override
    {
        if (_listPackagesResult != WPEFramework::Core::ERROR_NONE || _nullIterator) {
            packages = nullptr;
            return _listPackagesResult;
        }
        packages = new FakePackageIterator(_listPackagesPackages);
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult GetConfigForPackage(
        const std::string& /*fileLocator*/,
        std::string& packageId,
        std::string& version,
        WPEFramework::Exchange::RuntimeConfig& /*config*/) override
    {
        if (_getConfigResult == WPEFramework::Core::ERROR_NONE) {
            packageId = _getConfigPackageId;
            version   = _getConfigVersion;
        }
        return _getConfigResult;
    }

    WPEFramework::Core::hresult PackageState(
        const std::string& /*packageId*/,
        const std::string& /*version*/,
        InstallState& state) override
    {
        state = InstallState::INSTALLED;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Register(INotification* notification) override
    {
        registerCalls++;
        _notifications.push_back(notification);
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unregister(INotification* notification) override
    {
        unregisterCalls++;
        _notifications.remove(notification);
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Uninstall(
        const std::string& /*packageId*/,
        std::string& /*errorReason*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Config(
        const std::string& /*packageId*/,
        const std::string& /*version*/,
        WPEFramework::Exchange::RuntimeConfig& /*configMetadata*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    // ── Observability ──────────────────────────────────────────────────────
    mutable std::atomic<uint32_t> installCalls;
    mutable std::atomic<uint32_t> registerCalls;
    mutable std::atomic<uint32_t> unregisterCalls;
    std::vector<InstallCall> lastInstallCalls;

protected:
    // Registered IPackageInstaller::INotification listeners.
    // Protected so subclasses can trigger callbacks for dispatch-pipeline tests.
    std::list<INotification*> _notifications;

    void FireNotification(const std::string& json)
    {
        for (auto* n : _notifications) {
            n->OnAppInstallationStatus(json);
        }
    }

private:
    mutable std::atomic<uint32_t> _refCount;
    WPEFramework::Core::hresult _listPackagesResult;
    std::vector<Package> _listPackagesPackages;
    bool _nullIterator { false };
    WPEFramework::Core::hresult _installResult;
    FailReason _installFailReason;
    WPEFramework::Core::hresult _getConfigResult;
    std::string _getConfigPackageId;
    std::string _getConfigVersion;
};

// ─────────────────────────────────────────────────────────────────────────────
// ServiceMock — implements PluginHost::IShell
// ─────────────────────────────────────────────────────────────────────────────

class ServiceMock final : public WPEFramework::PluginHost::IShell {
public:
    using InstantiateHandler = std::function<void*(const WPEFramework::RPC::Object&, const uint32_t, uint32_t&)>;

    struct Config {
        Config()
            : configLine("{}")
            , callsign("org.rdk.PreinstallManager")
            , packageInstaller(nullptr)
        {
        }
        std::string configLine;
        std::string callsign;
        // optional IPackageInstaller to return for QueryInterfaceByCallsign()
        WPEFramework::Exchange::IPackageInstaller* packageInstaller;
    };

    class COMLinkMock final : public WPEFramework::PluginHost::IShell::ICOMLink {
    public:
        explicit COMLinkMock(ServiceMock& parent)
            : _parent(parent)
        {
        }

        ~COMLinkMock() override = default;

        void Register(WPEFramework::RPC::IRemoteConnection::INotification*) override {}
        void Unregister(const WPEFramework::RPC::IRemoteConnection::INotification*) override {}
        void Register(INotification*) override {}
        void Unregister(INotification*) override {}

        WPEFramework::RPC::IRemoteConnection* RemoteConnection(const uint32_t) override
        {
            return nullptr;
        }

        void* Instantiate(const WPEFramework::RPC::Object& object,
                          const uint32_t interfaceId,
                          uint32_t& connectionId) override
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

    explicit ServiceMock(Config cfg = Config())
        : _refCount(1)
        , _cfg(std::move(cfg))
        , _comLink(*this)
    {
    }

    ~ServiceMock() override = default;

    ServiceMock(const ServiceMock&) = delete;
    ServiceMock& operator=(const ServiceMock&) = delete;

    void SetInstantiateHandler(InstantiateHandler h)
    {
        _instantiateHandler = std::move(h);
    }

    // ── Observability ──────────────────────────────────────────────────────
    mutable std::atomic<uint32_t> addRefCalls   { 0 };
    mutable std::atomic<uint32_t> releaseCalls  { 0 };
    mutable std::atomic<uint32_t> instantiateCalls { 0 };

    // ── Core::IUnknown ─────────────────────────────────────────────────────
    void AddRef() const override
    {
        addRefCalls++;
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        releaseCalls++;
        const uint32_t n = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (0U == n) ? WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED
                         : WPEFramework::Core::ERROR_NONE;
    }

    // ── PluginHost::IShell ─────────────────────────────────────────────────
    std::string Versions() const override                          { return ""; }
    std::string Locator() const override                           { return ""; }
    std::string ClassName() const override                         { return ""; }
    std::string Callsign() const override                          { return _cfg.callsign; }
    std::string WebPrefix() const override                         { return ""; }
    std::string ConfigLine() const override                        { return _cfg.configLine; }
    std::string PersistentPath() const override                    { return "/tmp"; }
    std::string VolatilePath() const override                      { return "/tmp"; }
    std::string DataPath() const override                          { return "/tmp"; }
    std::string SystemPath() const override                        { return ""; }
    std::string PluginPath() const override                        { return ""; }
    std::string Model() const override                             { return ""; }
    std::string Accessor() const override                          { return ""; }
    std::string ProxyStubPath() const override                     { return ""; }
    std::string HashKey() const override                           { return ""; }
    std::string Substitute(const std::string&) const override      { return ""; }
    std::string SystemRootPath() const override                    { return ""; }
    bool Background() const override                               { return false; }
    bool Resumed() const override                                  { return false; }
    bool IsSupported(const uint8_t) const override                 { return false; }

    state State() const override                                   { return state::ACTIVATED; }
    startup Startup() const override                               { return static_cast<startup>(0); }
    reason Reason() const override                                 { return reason::REQUESTED; }

    WPEFramework::Core::hresult Activate(const reason) override            { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Deactivate(const reason) override          { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Unavailable(const reason) override         { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult ConfigLine(const std::string&) override    { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SystemRootPath(const std::string&) override{ return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Hibernate(const uint32_t) override         { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Startup(const startup) override            { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Resumed(const bool) override               { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Metadata(std::string& info) const override { info.clear(); return WPEFramework::Core::ERROR_NONE; }

    void EnableWebServer(const std::string&, const std::string&) override {}
    void DisableWebServer() override {}

    WPEFramework::PluginHost::ISubSystem* SubSystems() override { return nullptr; }

    uint32_t Submit(const uint32_t,
                    const WPEFramework::Core::ProxyType<WPEFramework::Core::JSON::IElement>&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    void Notify(const std::string&) override {}

    void* QueryInterface(const uint32_t /*id*/) override { return nullptr; }

    void* QueryInterfaceByCallsign(const uint32_t id, const std::string& callsign) override
    {
        if (id == WPEFramework::Exchange::IPackageInstaller::ID
            && callsign == "org.rdk.PackageManagerRDKEMS"
            && nullptr != _cfg.packageInstaller)
        {
            _cfg.packageInstaller->AddRef();
            return static_cast<WPEFramework::Exchange::IPackageInstaller*>(_cfg.packageInstaller);
        }
        return nullptr;
    }

    void Register(WPEFramework::PluginHost::IPlugin::INotification*) override {}
    void Unregister(WPEFramework::PluginHost::IPlugin::INotification*) override {}

    WPEFramework::RPC::IRemoteConnection* RemoteConnection(const uint32_t) override
    {
        return nullptr;
    }

    WPEFramework::PluginHost::IShell::ICOMLink* COMLink() override
    {
        return &_comLink;
    }

private:
    mutable std::atomic<uint32_t> _refCount;
    InstantiateHandler _instantiateHandler;
    Config _cfg;
    COMLinkMock _comLink;
};

// ─────────────────────────────────────────────────────────────────────────────
// FakeNotification — IPreinstallManager::INotification that counts calls
// ─────────────────────────────────────────────────────────────────────────────

class FakePreinstallNotification final
    : public WPEFramework::Exchange::IPreinstallManager::INotification {
public:
    FakePreinstallNotification()
        : _refCount(1)
        , callCount(0)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t n = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == n) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IPreinstallManager::INotification::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IPreinstallManager::INotification*>(this);
        }
        return nullptr;
    }

    void OnAppInstallationStatus(const std::string& jsonresponse) override
    {
        callCount++;
        lastJson = jsonresponse;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> callCount;
    std::string lastJson;
};

} // namespace L0Test
