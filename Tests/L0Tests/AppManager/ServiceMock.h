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

/**
 * @file ServiceMock.h
 *
 * AppManager-specific L0 service mock and fake stubs.
 *
 * Pattern follows RuntimeManager and AppGateway ServiceMock conventions:
 *  - ServiceMock: minimal PluginHost::IShell implementation with
 *    configurable Root<T> / QueryInterfaceByCallsign behaviour.
 *  - FakeAppManager: stub Exchange::IAppManager returning controlled hresult values.
 *  - FakeConfiguration: stub Exchange::IConfiguration.
 *  - FakeNotification: records IAppManager::INotification callbacks.
 *  - FakePackageInstaller: returns configurable package lists.
 *  - FakeAppStorageManager: stub for ClearAppData / ClearAllAppData.
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <list>
#include <string>
#include <vector>

#include <core/core.h>
#include <plugins/plugins.h>
#include <plugins/IShell.h>
#include <interfaces/IAppManager.h>
#include <interfaces/IStore2.h>
#include <interfaces/IConfiguration.h>
#include <interfaces/IPackageManager.h>
#include <interfaces/IAppStorageManager.h>

namespace L0Test {

using string = std::string;

class BasicStore2 final : public WPEFramework::Exchange::IStore2 {
public:
    void AddRef() const override { _refCount.fetch_add(1, std::memory_order_relaxed); }

    uint32_t Release() const override
    {
        const uint32_t count = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (0U == count) ? WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED : WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (WPEFramework::Exchange::IStore2::ID == id) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IStore2*>(this);
        }
        return nullptr;
    }

    uint32_t Register(INotification*) override { return WPEFramework::Core::ERROR_NONE; }
    uint32_t Unregister(INotification*) override { return WPEFramework::Core::ERROR_NONE; }
    uint32_t SetValue(const ScopeType, const string&, const string&, const string&, const uint32_t) override { return WPEFramework::Core::ERROR_NONE; }
    uint32_t GetValue(const ScopeType, const string&, const string&, string& value, uint32_t& ttl) override
    {
        value.clear();
        ttl = 0;
        return WPEFramework::Core::ERROR_NONE;
    }
    uint32_t DeleteKey(const ScopeType, const string&, const string&) override { return WPEFramework::Core::ERROR_NONE; }
    uint32_t DeleteNamespace(const ScopeType, const string&) override { return WPEFramework::Core::ERROR_NONE; }

private:
    mutable std::atomic<uint32_t> _refCount { 1 };
};

class BasicPackageHandler final : public WPEFramework::Exchange::IPackageHandler {
public:
    void AddRef() const override { _refCount.fetch_add(1, std::memory_order_relaxed); }

    uint32_t Release() const override
    {
        const uint32_t count = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (0U == count) ? WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED : WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (WPEFramework::Exchange::IPackageHandler::ID == id) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IPackageHandler*>(this);
        }
        return nullptr;
    }

    WPEFramework::Core::hresult Lock(const string&, const string&, const LockReason&, uint32_t& lockId,
                                     string& unpackedPath, WPEFramework::Exchange::RuntimeConfig& configMetadata,
                                     WPEFramework::Exchange::IPackageHandler::ILockIterator*& appMetadata) override
    {
        lockId = 0;
        unpackedPath.clear();
        configMetadata = {};
        appMetadata = nullptr;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unlock(const string&, const string&) override { return WPEFramework::Core::ERROR_NONE; }

    WPEFramework::Core::hresult GetLockedInfo(const string&, const string&, string& unpackedPath,
                                              WPEFramework::Exchange::RuntimeConfig& configMetadata,
                                              string& gatewayMetadataPath, bool& locked) override
    {
        unpackedPath.clear();
        gatewayMetadataPath.clear();
        configMetadata = {};
        locked = false;
        return WPEFramework::Core::ERROR_NONE;
    }

private:
    mutable std::atomic<uint32_t> _refCount { 1 };
};

class BasicPackageInstaller final : public WPEFramework::Exchange::IPackageInstaller {
public:
    void AddRef() const override { _refCount.fetch_add(1, std::memory_order_relaxed); }

    uint32_t Release() const override
    {
        const uint32_t count = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (0U == count) ? WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED : WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (WPEFramework::Exchange::IPackageInstaller::ID == id) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IPackageInstaller*>(this);
        }
        return nullptr;
    }

    WPEFramework::Core::hresult Register(INotification*) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Unregister(INotification*) override { return WPEFramework::Core::ERROR_NONE; }

    WPEFramework::Core::hresult Install(const string&, const string&, IKeyValueIterator* const&, const string&, FailReason&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Uninstall(const string&, string&) override { return WPEFramework::Core::ERROR_NONE; }

    WPEFramework::Core::hresult ListPackages(IPackageIterator*& packages) override
    {
        packages = nullptr;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Config(const string&, const string&, WPEFramework::Exchange::RuntimeConfig&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult PackageState(const string&, const string&, WPEFramework::Exchange::IPackageInstaller::InstallState& state) override
    {
        state = WPEFramework::Exchange::IPackageInstaller::InstallState::UNINSTALLED;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult GetConfigForPackage(const string&, string&, string&, WPEFramework::Exchange::RuntimeConfig&) override
    {
        return WPEFramework::Core::ERROR_GENERAL;
    }

private:
    mutable std::atomic<uint32_t> _refCount { 1 };
};

class BasicAppStorageManager final : public WPEFramework::Exchange::IAppStorageManager {
public:
    void AddRef() const override { _refCount.fetch_add(1, std::memory_order_relaxed); }

    uint32_t Release() const override
    {
        const uint32_t count = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (0U == count) ? WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED : WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (WPEFramework::Exchange::IAppStorageManager::ID == id) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IAppStorageManager*>(this);
        }
        return nullptr;
    }

    WPEFramework::Core::hresult CreateStorage(const string&, const uint32_t&, string& path, string&) override
    {
        path.clear();
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult GetStorage(const string&, const int32_t&, const int32_t&, string& path, uint32_t& size, uint32_t& used) override
    {
        path.clear();
        size = 0;
        used = 0;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult DeleteStorage(const string&, string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Clear(const string&, string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult ClearAll(const string&, string&) override { return WPEFramework::Core::ERROR_NONE; }

private:
    mutable std::atomic<uint32_t> _refCount { 1 };
};

// ─────────────────────────────────────────────────────────────────────────────
// ServiceMock: minimal PluginHost::IShell
// ─────────────────────────────────────────────────────────────────────────────

class ServiceMock final : public WPEFramework::PluginHost::IShell {
public:
    using InstantiateHandler = std::function<void*(const WPEFramework::RPC::Object&, const uint32_t, uint32_t&)>;

    struct Config {
        std::string configLine;
        Config() : configLine("{}") {}
    };

    class COMLinkMock final : public WPEFramework::PluginHost::IShell::ICOMLink {
    public:
        explicit COMLinkMock(ServiceMock& parent) : _parent(parent) {}
        ~COMLinkMock() override = default;

        void Register(WPEFramework::RPC::IRemoteConnection::INotification*) override {}
        void Unregister(const WPEFramework::RPC::IRemoteConnection::INotification*) override {}
        void Register(INotification*) override {}
        void Unregister(INotification*) override {}
        WPEFramework::RPC::IRemoteConnection* RemoteConnection(const uint32_t) override { return nullptr; }

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

    void SetInstantiateHandler(InstantiateHandler handler)
    {
        _instantiateHandler = std::move(handler);
    }

    // Observability counters
    mutable std::atomic<uint32_t> addRefCalls { 0 };
    mutable std::atomic<uint32_t> releaseCalls { 0 };
    mutable std::atomic<uint32_t> instantiateCalls { 0 };

    void AddRef() const override
    {
        addRefCalls++;
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        releaseCalls++;
        const uint32_t newCount = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (0U == newCount)
            ? WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED
            : WPEFramework::Core::ERROR_NONE;
    }

    std::string Versions() const override { return ""; }
    std::string Locator() const override { return ""; }
    std::string ClassName() const override { return ""; }
    std::string Callsign() const override { return "org.rdk.AppManager"; }
    std::string WebPrefix() const override { return ""; }
    std::string ConfigLine() const override { return _cfg.configLine; }
    std::string PersistentPath() const override { return "/tmp"; }
    std::string VolatilePath() const override { return "/tmp"; }
    std::string DataPath() const override { return "/tmp"; }
    state State() const override { return state::ACTIVATED; }

    WPEFramework::Core::hresult Activate(const reason) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Deactivate(const reason) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Unavailable(const reason) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult ConfigLine(const std::string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SystemRootPath(const std::string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Hibernate(const uint32_t) override { return WPEFramework::Core::ERROR_NONE; }
    std::string SystemPath() const override { return ""; }
    std::string PluginPath() const override { return ""; }
    startup Startup() const override { return static_cast<startup>(0); }
    WPEFramework::Core::hresult Startup(const startup) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Resumed(const bool) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Metadata(std::string& info) const override { info.clear(); return WPEFramework::Core::ERROR_NONE; }
    bool Resumed() const override { return false; }
    bool IsSupported(const uint8_t) const override { return false; }
    void EnableWebServer(const std::string&, const std::string&) override {}
    void DisableWebServer() override {}
    WPEFramework::PluginHost::ISubSystem* SubSystems() override { return nullptr; }
    uint32_t Submit(const uint32_t, const WPEFramework::Core::ProxyType<WPEFramework::Core::JSON::IElement>&) override { return WPEFramework::Core::ERROR_NONE; }
    void Notify(const std::string&) override {}
    void* QueryInterface(const uint32_t) override { return nullptr; }
    void* QueryInterfaceByCallsign(const uint32_t id, const std::string& callsign) override
    {
        if ((WPEFramework::Exchange::IStore2::ID == id) && ("org.rdk.PersistentStore" == callsign)) {
            _store2.AddRef();
            return static_cast<WPEFramework::Exchange::IStore2*>(&_store2);
        }
        if ((WPEFramework::Exchange::IPackageHandler::ID == id) && ("org.rdk.PackageManagerRDKEMS" == callsign)) {
            _packageHandler.AddRef();
            return static_cast<WPEFramework::Exchange::IPackageHandler*>(&_packageHandler);
        }
        if ((WPEFramework::Exchange::IPackageInstaller::ID == id) && ("org.rdk.PackageManagerRDKEMS" == callsign)) {
            _packageInstaller.AddRef();
            return static_cast<WPEFramework::Exchange::IPackageInstaller*>(&_packageInstaller);
        }
        if ((WPEFramework::Exchange::IAppStorageManager::ID == id) && ("org.rdk.AppStorageManager" == callsign)) {
            _appStorageManager.AddRef();
            return static_cast<WPEFramework::Exchange::IAppStorageManager*>(&_appStorageManager);
        }

        return nullptr;
    }
    void Register(WPEFramework::PluginHost::IPlugin::INotification*) override {}
    void Unregister(WPEFramework::PluginHost::IPlugin::INotification*) override {}
    std::string Model() const override { return ""; }
    bool Background() const override { return false; }
    std::string Accessor() const override { return ""; }
    std::string ProxyStubPath() const override { return ""; }
    std::string HashKey() const override { return ""; }
    std::string Substitute(const std::string&) const override { return ""; }

    WPEFramework::PluginHost::IShell::ICOMLink* COMLink() override { return &_comLink; }

    reason Reason() const override { return reason::REQUESTED; }
    std::string SystemRootPath() const override { return ""; }

    WPEFramework::RPC::IRemoteConnection* RemoteConnection(const uint32_t) const { return nullptr; }

private:
    mutable std::atomic<uint32_t> _refCount;
    InstantiateHandler _instantiateHandler;
    Config _cfg;
    COMLinkMock _comLink;
    BasicStore2 _store2;
    BasicPackageHandler _packageHandler;
    BasicPackageInstaller _packageInstaller;
    BasicAppStorageManager _appStorageManager;
};

// ─────────────────────────────────────────────────────────────────────────────
// FakeConfiguration: stub Exchange::IConfiguration
// ─────────────────────────────────────────────────────────────────────────────

class FakeConfiguration final : public WPEFramework::Exchange::IConfiguration {
public:
    explicit FakeConfiguration(WPEFramework::Core::hresult configureResult = WPEFramework::Core::ERROR_NONE)
        : _refCount(1)
        , configureCalls(0)
        , _configureResult(configureResult)
    {
    }

    void AddRef() const override { _refCount.fetch_add(1, std::memory_order_relaxed); }

    uint32_t Release() const override
    {
        const uint32_t r = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == r) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (WPEFramework::Exchange::IConfiguration::ID == id) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IConfiguration*>(this);
        }
        return nullptr;
    }

    uint32_t Configure(WPEFramework::PluginHost::IShell*) override
    {
        configureCalls++;
        return _configureResult;
    }

    void SetConfigureResult(WPEFramework::Core::hresult result) { _configureResult = result; }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> configureCalls;
    WPEFramework::Core::hresult _configureResult;
};

// ─────────────────────────────────────────────────────────────────────────────
// FakeAppManagerImpl: stub Exchange::IAppManager + Exchange::IConfiguration
// Returned by ServiceMock::Root<Exchange::IAppManager>() via InstantiateHandler
// ─────────────────────────────────────────────────────────────────────────────

class FakeAppManagerImpl final : public WPEFramework::Exchange::IAppManager,
                                  public WPEFramework::Exchange::IConfiguration {
public:
    explicit FakeAppManagerImpl(WPEFramework::Core::hresult configureResult = WPEFramework::Core::ERROR_NONE)
        : _refCount(1)
        , configureCalls(0)
        , registerCalls(0)
        , unregisterCalls(0)
        , _configureResult(configureResult)
    {
    }

    void AddRef() const override { _refCount.fetch_add(1, std::memory_order_relaxed); }

    uint32_t Release() const override
    {
        const uint32_t r = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == r) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (WPEFramework::Exchange::IAppManager::ID == id) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IAppManager*>(this);
        }
        if (WPEFramework::Exchange::IConfiguration::ID == id) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IConfiguration*>(this);
        }
        return nullptr;
    }

    // IConfiguration
    uint32_t Configure(WPEFramework::PluginHost::IShell*) override
    {
        configureCalls++;
        return _configureResult;
    }

    // IAppManager stubs
    WPEFramework::Core::hresult Register(WPEFramework::Exchange::IAppManager::INotification* n) override
    {
        registerCalls++;
        if (n) { n->AddRef(); _notifications.push_back(n); }
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unregister(WPEFramework::Exchange::IAppManager::INotification* n) override
    {
        unregisterCalls++;
        auto it = std::find(_notifications.begin(), _notifications.end(), n);
        if (it != _notifications.end()) {
            (*it)->Release();
            _notifications.erase(it);
            return WPEFramework::Core::ERROR_NONE;
        }
        return WPEFramework::Core::ERROR_GENERAL;
    }

    WPEFramework::Core::hresult LaunchApp(const string&, const string&, const string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult CloseApp(const string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult TerminateApp(const string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult KillApp(const string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetLoadedApps(WPEFramework::Exchange::IAppManager::ILoadedAppInfoIterator*& iter) override { iter = nullptr; return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SendIntent(const string&, const string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult PreloadApp(const string&, const string&, string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetAppProperty(const string&, const string&, string& value) override { value = ""; return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SetAppProperty(const string&, const string&, const string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetInstalledApps(string& apps) override { apps = "[]"; return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult IsInstalled(const string&, bool& installed) override { installed = false; return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult StartSystemApp(const string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult StopSystemApp(const string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult ClearAppData(const string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult ClearAllAppData() override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetAppMetadata(const string&, const string&, string& result) override { result = ""; return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetMaxRunningApps(int32_t& v) const override { v = 5; return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetMaxHibernatedApps(int32_t& v) const override { v = 3; return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetMaxHibernatedFlashUsage(int32_t& v) const override { v = 1024; return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetMaxInactiveRamUsage(int32_t& v) const override { v = 2048; return WPEFramework::Core::ERROR_NONE; }

    void SetConfigureResult(WPEFramework::Core::hresult r) { _configureResult = r; }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> configureCalls;
    std::atomic<uint32_t> registerCalls;
    std::atomic<uint32_t> unregisterCalls;

private:
    WPEFramework::Core::hresult _configureResult;
    std::list<WPEFramework::Exchange::IAppManager::INotification*> _notifications;
};

// FakeAppManagerImplNoConfig: like FakeAppManagerImpl but does NOT expose IConfiguration
class FakeAppManagerImplNoConfig final : public WPEFramework::Exchange::IAppManager {
public:
    FakeAppManagerImplNoConfig() : _refCount(1) {}

    void AddRef() const override { _refCount.fetch_add(1, std::memory_order_relaxed); }

    uint32_t Release() const override
    {
        const uint32_t r = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == r) { delete this; return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED; }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (WPEFramework::Exchange::IAppManager::ID == id) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IAppManager*>(this);
        }
        return nullptr; // IConfiguration intentionally absent
    }

    WPEFramework::Core::hresult Register(WPEFramework::Exchange::IAppManager::INotification*) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Unregister(WPEFramework::Exchange::IAppManager::INotification*) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult LaunchApp(const string&, const string&, const string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult CloseApp(const string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult TerminateApp(const string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult KillApp(const string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetLoadedApps(WPEFramework::Exchange::IAppManager::ILoadedAppInfoIterator*& iter) override { iter = nullptr; return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SendIntent(const string&, const string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult PreloadApp(const string&, const string&, string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetAppProperty(const string&, const string&, string& value) override { value = ""; return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SetAppProperty(const string&, const string&, const string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetInstalledApps(string& apps) override { apps = "[]"; return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult IsInstalled(const string&, bool& installed) override { installed = false; return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult StartSystemApp(const string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult StopSystemApp(const string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult ClearAppData(const string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult ClearAllAppData() override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetAppMetadata(const string&, const string&, string& result) override { result = ""; return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetMaxRunningApps(int32_t& v) const override { v = 0; return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetMaxHibernatedApps(int32_t& v) const override { v = 0; return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetMaxHibernatedFlashUsage(int32_t& v) const override { v = 0; return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetMaxInactiveRamUsage(int32_t& v) const override { v = 0; return WPEFramework::Core::ERROR_NONE; }

    mutable std::atomic<uint32_t> _refCount;
};

// ─────────────────────────────────────────────────────────────────────────────
// FakeAppManagerNotification: counts IAppManager::INotification callbacks
// ─────────────────────────────────────────────────────────────────────────────

class FakeAppManagerNotification final : public WPEFramework::Exchange::IAppManager::INotification {
public:
    FakeAppManagerNotification()
        : _refCount(1)
        , onInstalledCalls(0)
        , onUninstalledCalls(0)
        , onLifecycleCalls(0)
        , onLaunchRequestCalls(0)
        , onUnloadedCalls(0)
    {
    }

    void AddRef() const override { _refCount.fetch_add(1, std::memory_order_relaxed); }

    uint32_t Release() const override
    {
        const uint32_t r = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == r) { delete this; return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED; }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (WPEFramework::Exchange::IAppManager::INotification::ID == id) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IAppManager::INotification*>(this);
        }
        return nullptr;
    }

    void OnAppInstalled(const string& appId, const string& version) override
    {
        lastAppId = appId;
        lastVersion = version;
        onInstalledCalls++;
    }

    void OnAppUninstalled(const string& appId) override
    {
        lastAppId = appId;
        onUninstalledCalls++;
    }

    void OnAppLifecycleStateChanged(const string& appId,
                                     const string& appInstanceId,
                                     WPEFramework::Exchange::IAppManager::AppLifecycleState newState,
                                     WPEFramework::Exchange::IAppManager::AppLifecycleState oldState,
                                     WPEFramework::Exchange::IAppManager::AppErrorReason errorReason) override
    {
        lastAppId = appId;
        lastAppInstanceId = appInstanceId;
        lastNewState = newState;
        lastOldState = oldState;
        lastErrorReason = errorReason;
        onLifecycleCalls++;
    }

    void OnAppLaunchRequest(const string& appId, const string& intent, const string& source) override
    {
        lastAppId = appId;
        lastIntent = intent;
        lastSource = source;
        onLaunchRequestCalls++;
    }

    void OnAppUnloaded(const string& appId, const string& appInstanceId) override
    {
        lastAppId = appId;
        lastAppInstanceId = appInstanceId;
        onUnloadedCalls++;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> onInstalledCalls;
    std::atomic<uint32_t> onUninstalledCalls;
    std::atomic<uint32_t> onLifecycleCalls;
    std::atomic<uint32_t> onLaunchRequestCalls;
    std::atomic<uint32_t> onUnloadedCalls;

    string lastAppId;
    string lastVersion;
    string lastAppInstanceId;
    string lastIntent;
    string lastSource;
    WPEFramework::Exchange::IAppManager::AppLifecycleState lastNewState
        { WPEFramework::Exchange::IAppManager::APP_STATE_UNKNOWN };
    WPEFramework::Exchange::IAppManager::AppLifecycleState lastOldState
        { WPEFramework::Exchange::IAppManager::APP_STATE_UNKNOWN };
    WPEFramework::Exchange::IAppManager::AppErrorReason lastErrorReason
        { WPEFramework::Exchange::IAppManager::APP_ERROR_NONE };
};

// ─────────────────────────────────────────────────────────────────────────────
// FakeAppStorageManager: stub Exchange::IAppStorageManager
// ─────────────────────────────────────────────────────────────────────────────

class FakeAppStorageManager final : public WPEFramework::Exchange::IAppStorageManager {
public:
    FakeAppStorageManager()
        : _refCount(1)
        , createStorageCalls(0)
        , getStorageCalls(0)
        , deleteStorageCalls(0)
        , clearStorageCalls(0)
        , clearAllStorageCalls(0)
        , _result(WPEFramework::Core::ERROR_NONE)
    {
    }

    void AddRef() const override { _refCount.fetch_add(1, std::memory_order_relaxed); }

    uint32_t Release() const override
    {
        const uint32_t r = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == r) { delete this; return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED; }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (WPEFramework::Exchange::IAppStorageManager::ID == id) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IAppStorageManager*>(this);
        }
        return nullptr;
    }

    WPEFramework::Core::hresult CreateStorage(const string& appId, const uint32_t& size, string& path, string& errorReason) override
    {
        createStorageCalls++;
        lastCreateAppId = appId;
        path = "/storage/" + appId;
        return _result;
    }

    WPEFramework::Core::hresult GetStorage(const string& appId, const int32_t& userId, const int32_t& groupId, string& path, uint32_t& size, uint32_t& used) override
    {
        getStorageCalls++;
        path = "/storage/" + appId;
        size = 1024;
        used = 512;
        return _result;
    }

    WPEFramework::Core::hresult DeleteStorage(const string& appId, string& errorReason) override
    {
        deleteStorageCalls++;
        lastDeleteAppId = appId;
        return _result;
    }

    WPEFramework::Core::hresult Clear(const string& appId, string& errorReason) override
    {
        clearStorageCalls++;
        lastClearAppId = appId;
        return _result;
    }

    WPEFramework::Core::hresult ClearAll(const string& exemptionAppIds, string& errorReason) override
    {
        clearAllStorageCalls++;
        return _result;
    }

    void SetResult(WPEFramework::Core::hresult r) { _result = r; }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> createStorageCalls;
    std::atomic<uint32_t> getStorageCalls;
    std::atomic<uint32_t> deleteStorageCalls;
    std::atomic<uint32_t> clearStorageCalls;
    std::atomic<uint32_t> clearAllStorageCalls;
    string lastCreateAppId;
    string lastDeleteAppId;
    string lastClearAppId;
    WPEFramework::Core::hresult _result;
};

} // namespace L0Test
