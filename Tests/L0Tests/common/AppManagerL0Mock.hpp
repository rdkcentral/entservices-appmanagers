/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
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
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <core/core.h>
#include <interfaces/IAppManager.h>
#include <interfaces/IAppPackageManager.h>
#include <interfaces/IAppStorageManager.h>
#include <interfaces/IConfiguration.h>
#include <interfaces/ILifecycleManager.h>
#include <interfaces/ILifecycleManagerState.h>
#include <interfaces/IStore2.h>
#include <plugins/IShell.h>
#include <plugins/plugins.h>

namespace L0Test {

using string = std::string;

inline Thunder::Exchange::IPackageInstaller::IPackageIterator* CreatePackageIterator(
    const std::vector<Thunder::Exchange::IPackageInstaller::Package>& packages)
{
    return Thunder::Core::Service<
        Thunder::RPC::IteratorType<Thunder::Exchange::IPackageInstaller::IPackageIterator>>::Create<
        Thunder::Exchange::IPackageInstaller::IPackageIterator>(packages);
}

class MockRemoteConnection final : public Thunder::RPC::IRemoteConnection {
public:
    explicit MockRemoteConnection(const uint32_t id = 1)
        : _refCount(1)
        , _id(id)
    {
    }

    uint32_t AddRef() const override
    {
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return Thunder::Core::ERROR_NONE;
    }

    uint32_t Id() const override
    {
        return _id;
    }

    uint32_t RemoteId() const override
    {
        return _id;
    }

    void* Acquire(uint32_t, const std::string&, uint32_t, uint32_t) override
    {
        return nullptr;
    }

    void Terminate() override
    {
        _terminated = true;
    }

    uint32_t Launch() override
    {
        return Thunder::Core::ERROR_NONE;
    }

    void PostMortem() override
    {
    }

    void* QueryInterface(const uint32_t) override
    {
        return nullptr;
    }

    bool terminated() const
    {
        return _terminated;
    }

private:
    mutable std::atomic<uint32_t> _refCount;
    uint32_t _id;
    bool _terminated { false };
};

class MockAppManagerNotification final
    : public Thunder::Exchange::IAppManager::INotification
    , public Thunder::RPC::IRemoteConnection::INotification {
public:
    MockAppManagerNotification()
        : _refCount(1)
    {
    }

    uint32_t AddRef() const override
    {
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return Thunder::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == Thunder::Exchange::IAppManager::INotification::ID) {
            AddRef();
            return static_cast<Thunder::Exchange::IAppManager::INotification*>(this);
        }
        if (id == Thunder::RPC::IRemoteConnection::INotification::ID) {
            AddRef();
            return static_cast<Thunder::RPC::IRemoteConnection::INotification*>(this);
        }
        return nullptr;
    }

    void Activated(Thunder::RPC::IRemoteConnection*) override
    {
        activatedCount++;
    }

    void Deactivated(Thunder::RPC::IRemoteConnection*) override
    {
        deactivatedCount++;
    }

    void OnAppInstalled(const string&, const string&) override
    {
        installedCount++;
    }

    void OnAppUninstalled(const string&) override
    {
        uninstalledCount++;
    }

    void OnAppLifecycleStateChanged(const string&, const string&, const Thunder::Exchange::IAppManager::AppLifecycleState,
        const Thunder::Exchange::IAppManager::AppLifecycleState, const Thunder::Exchange::IAppManager::AppErrorReason) override
    {
        lifecycleCount++;
    }

    void OnAppLaunchRequest(const string&, const string&, const string&) override
    {
        launchRequestCount++;
    }

    void OnAppUnloaded(const string&, const string&) override
    {
        unloadedCount++;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> activatedCount { 0 };
    std::atomic<uint32_t> deactivatedCount { 0 };
    std::atomic<uint32_t> installedCount { 0 };
    std::atomic<uint32_t> uninstalledCount { 0 };
    std::atomic<uint32_t> lifecycleCount { 0 };
    std::atomic<uint32_t> launchRequestCount { 0 };
    std::atomic<uint32_t> unloadedCount { 0 };
};

class MockPackageHandler final : public Thunder::Exchange::IPackageHandler {
public:
    using LockHandler = std::function<Thunder::Core::hresult(
        const string&, const string&, const LockReason&, uint32_t&, string&, std::string&, ILockIterator*&)>;
    using UnlockHandler = std::function<Thunder::Core::hresult(const string&, const string&)>;

    MockPackageHandler()
        : _refCount(1)
    {
    }

    uint32_t AddRef() const override
    {
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return Thunder::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == Thunder::Exchange::IPackageHandler::ID) {
            AddRef();
            return static_cast<Thunder::Exchange::IPackageHandler*>(this);
        }
        return nullptr;
    }

    Thunder::Core::hresult Lock(const string& packageId, const string& version, const LockReason& lockReason,
        uint32_t& lockId, string& unpackedPath, std::string& configMetadata, ILockIterator*& appMetadata) override
    {
        lockCount++;
        if (lockHandler) {
            return lockHandler(packageId, version, lockReason, lockId, unpackedPath, configMetadata, appMetadata);
        }
        lockId = 1;
        unpackedPath = "/tmp/app";
        appMetadata = nullptr;
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult Unlock(const string& packageId, const string& version) override
    {
        unlockCount++;
        if (unlockHandler) {
            return unlockHandler(packageId, version);
        }
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult GetLockedInfo(const string&, const string&, string&, std::string&, string&, bool&) override
    {
        return Thunder::Core::ERROR_NONE;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> lockCount { 0 };
    std::atomic<uint32_t> unlockCount { 0 };
    LockHandler lockHandler;
    UnlockHandler unlockHandler;
};

class MockPackageInstaller final : public Thunder::Exchange::IPackageInstaller {
public:
    using Package = Thunder::Exchange::IPackageInstaller::Package;
    using ListHandler = std::function<Thunder::Core::hresult(IPackageIterator*&)>;
    using ConfigHandler = std::function<Thunder::Core::hresult(const string&, string&, string&, std::string&)>;

    MockPackageInstaller()
        : _refCount(1)
    {
    }

    uint32_t AddRef() const override
    {
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return Thunder::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == Thunder::Exchange::IPackageInstaller::ID) {
            AddRef();
            return static_cast<Thunder::Exchange::IPackageInstaller*>(this);
        }
        return nullptr;
    }

    Thunder::Core::hresult Register(INotification*) override
    {
        registerCount++;
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult Unregister(INotification*) override
    {
        unregisterCount++;
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult Install(const string&, const string&, IKeyValueIterator* const&, const string&, FailReason&) override
    {
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult Uninstall(const string&, string&) override
    {
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult ListPackages(IPackageIterator*& packages) override
    {
        listPackagesCount++;
        if (listHandler) {
            return listHandler(packages);
        }
        packages = CreatePackageIterator(installedPackages);
        return (nullptr != packages) ? Thunder::Core::ERROR_NONE : Thunder::Core::ERROR_GENERAL;
    }

    Thunder::Core::hresult Config(const string&, const string&, std::string&) override
    {
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult PackageState(const string&, const string&, InstallState&) override
    {
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult GetConfigForPackage(const string& fileLocator, string& id, string& version, std::string& config) override
    {
        getConfigCount++;
        if (configHandler) {
            return configHandler(fileLocator, id, version, config);
        }
        return Thunder::Core::ERROR_GENERAL;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> registerCount { 0 };
    std::atomic<uint32_t> unregisterCount { 0 };
    std::atomic<uint32_t> listPackagesCount { 0 };
    std::atomic<uint32_t> getConfigCount { 0 };
    std::vector<Package> installedPackages;
    ListHandler listHandler;
    ConfigHandler configHandler;
};

class MockStore2 final : public Thunder::Exchange::IStore2 {
public:
    MockStore2()
        : _refCount(1)
    {
    }

    uint32_t AddRef() const override
    {
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return Thunder::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == Thunder::Exchange::IStore2::ID) {
            AddRef();
            return static_cast<Thunder::Exchange::IStore2*>(this);
        }
        return nullptr;
    }

    uint32_t Register(INotification*) override { return Thunder::Core::ERROR_NONE; }
    uint32_t Unregister(INotification*) override { return Thunder::Core::ERROR_NONE; }

    uint32_t SetValue(const ScopeType, const string& ns, const string& key, const string& value, const uint32_t ttl) override
    {
        (void)ttl;
        data[ns + ":" + key] = value;
        return Thunder::Core::ERROR_NONE;
    }

    uint32_t GetValue(const ScopeType, const string& ns, const string& key, string& value, uint32_t& ttl) override
    {
        (void)ttl;
        const auto it = data.find(ns + ":" + key);
        if (it != data.end()) {
            value = it->second;
            return Thunder::Core::ERROR_NONE;
        }
        value.clear();
        return Thunder::Core::ERROR_GENERAL;
    }

    uint32_t DeleteKey(const ScopeType, const string& ns, const string& key) override
    {
        data.erase(ns + ":" + key);
        return Thunder::Core::ERROR_NONE;
    }

    uint32_t DeleteNamespace(const ScopeType, const string& ns) override
    {
        for (auto it = data.begin(); it != data.end(); ) {
            if (0U == it->first.rfind(ns + ":", 0)) {
                it = data.erase(it);
            } else {
                ++it;
            }
        }
        return Thunder::Core::ERROR_NONE;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::map<std::string, std::string> data;
};

class MockStorageManager final : public Thunder::Exchange::IAppStorageManager {
public:
    MockStorageManager()
        : _refCount(1)
    {
    }

    uint32_t AddRef() const override
    {
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return Thunder::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == Thunder::Exchange::IAppStorageManager::ID) {
            AddRef();
            return static_cast<Thunder::Exchange::IAppStorageManager*>(this);
        }
        return nullptr;
    }

    Thunder::Core::hresult CreateStorage(const string&, const uint32_t&, string&, string&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetStorage(const string&, const int32_t&, const int32_t&, string&, uint32_t&, uint32_t&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult DeleteStorage(const string&, string&) override { return Thunder::Core::ERROR_NONE; }

    using ClearHandler = std::function<Thunder::Core::hresult(const string&, string&)>;

    Thunder::Core::hresult Clear(const string& appId, string& errorReason) override
    {
        if (clearHandler) { return clearHandler(appId, errorReason); }
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult ClearAll(const string& exemptedAppIds, string& errorReason) override
    {
        if (clearAllHandler) { return clearAllHandler(exemptedAppIds, errorReason); }
        return Thunder::Core::ERROR_NONE;
    }

    mutable std::atomic<uint32_t> _refCount;
    ClearHandler clearHandler;
    ClearHandler clearAllHandler;
};

class MockLifecycleManagerState final : public Thunder::Exchange::ILifecycleManagerState {
public:
    MockLifecycleManagerState()
        : _refCount(1)
    {
    }

    uint32_t AddRef() const override
    {
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return Thunder::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == Thunder::Exchange::ILifecycleManagerState::ID) {
            AddRef();
            return static_cast<Thunder::Exchange::ILifecycleManagerState*>(this);
        }
        return nullptr;
    }

    Thunder::Core::hresult Register(INotification* notification) override
    {
        registeredNotification = notification;
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult Unregister(INotification*) override
    {
        registeredNotification = nullptr;
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult AppReady(const string&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult StateChangeComplete(const string&, const uint32_t, const bool) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult CloseApp(const string&, const AppCloseReason) override { return Thunder::Core::ERROR_NONE; }

    mutable std::atomic<uint32_t> _refCount;
    INotification* registeredNotification { nullptr };
};

class MockLifecycleManager final : public Thunder::Exchange::ILifecycleManager {
public:
    MockLifecycleManager()
        : _refCount(1)
    {
    }

    uint32_t AddRef() const override
    {
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return Thunder::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == Thunder::Exchange::ILifecycleManager::ID) {
            AddRef();
            return static_cast<Thunder::Exchange::ILifecycleManager*>(this);
        }
        return nullptr;
    }

    Thunder::Core::hresult Register(INotification* notification) override
    {
        registeredNotification = notification;
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult Unregister(INotification*) override
    {
        registeredNotification = nullptr;
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult GetLoadedApps(const bool, std::string& apps) override
    {
        if (getLoadedAppsHandler) {
            return getLoadedAppsHandler(apps);
        }
        apps = loadedAppsJson;
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult IsAppLoaded(const std::string& appId, bool& loaded) const override
    {
        if (isAppLoadedHandler) {
            return isAppLoadedHandler(appId, loaded);
        }
        loaded = false;
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult SpawnApp(const string& appId, const string& launchIntent, const LifecycleState targetLifecycleState,
        const std::string& runtimeConfigObject, const string& launchArgs, string& appInstanceId,
        string& errorReason, bool& success) override
    {
        (void)runtimeConfigObject;
        if (spawnAppHandler) {
            return spawnAppHandler(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success);
        }
        appInstanceId = "instance-1";
        errorReason.clear();
        success = true;
        return Thunder::Core::ERROR_NONE;
    }

    uint32_t SetTargetAppState(const string& appInstanceId, const LifecycleState targetLifecycleState, const string& launchIntent) override
    {
        if (setTargetAppStateHandler) {
            return setTargetAppStateHandler(appInstanceId, targetLifecycleState, launchIntent);
        }
        (void)appInstanceId;
        (void)targetLifecycleState;
        (void)launchIntent;
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult UnloadApp(const string& appInstanceId, string& errorReason, bool& success) override
    {
        if (unloadAppHandler) {
            return unloadAppHandler(appInstanceId, errorReason, success);
        }
        errorReason.clear();
        success = true;
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult KillApp(const string& appInstanceId, string& errorReason, bool& success) override
    {
        if (killAppHandler) {
            return killAppHandler(appInstanceId, errorReason, success);
        }
        errorReason.clear();
        success = true;
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult SendIntentToActiveApp(const string& appInstanceId, const string& intent, string& errorReason, bool& success) override
    {
        if (sendIntentHandler) {
            return sendIntentHandler(appInstanceId, intent, errorReason, success);
        }
        errorReason.clear();
        success = true;
        return Thunder::Core::ERROR_NONE;
    }

    mutable std::atomic<uint32_t> _refCount;
    INotification* registeredNotification { nullptr };
    std::function<Thunder::Core::hresult(const std::string&, bool&)> isAppLoadedHandler;
    std::function<Thunder::Core::hresult(std::string&)> getLoadedAppsHandler;
    std::function<Thunder::Core::hresult(const string&, const string&, const LifecycleState, const std::string&, const string&, string&, string&, bool&)> spawnAppHandler;
    std::function<uint32_t(const string&, const LifecycleState, const string&)> setTargetAppStateHandler;
    std::function<Thunder::Core::hresult(const string&, string&, bool&)> unloadAppHandler;
    std::function<Thunder::Core::hresult(const string&, string&, bool&)> killAppHandler;
    std::function<Thunder::Core::hresult(const string&, const string&, string&, bool&)> sendIntentHandler;
    std::string loadedAppsJson;
};

class AppManagerServiceMock final : public Thunder::PluginHost::IShell {
public:
    using InstantiateHandler = std::function<void*(const Thunder::RPC::Object&, const uint32_t, uint32_t&)>;

    struct Config {
        explicit Config(Thunder::Exchange::IPackageInstaller* installer = nullptr)
            : installer(installer)
            , packageHandler(nullptr)
            , store2(nullptr)
            , storageManager(nullptr)
            , lifecycleManager(nullptr)
            , lifecycleManagerState(nullptr)
            , remoteConnection(nullptr)
            , configLine("{\"appPreinstallDirectory\":\"/tmp/preinstall\"}")
        {
        }

        Thunder::Exchange::IPackageInstaller* installer;
        Thunder::Exchange::IPackageHandler* packageHandler;
        Thunder::Exchange::IStore2* store2;
        Thunder::Exchange::IAppStorageManager* storageManager;
        Thunder::Exchange::ILifecycleManager* lifecycleManager;
        Thunder::Exchange::ILifecycleManagerState* lifecycleManagerState;
        Thunder::RPC::IRemoteConnection* remoteConnection;
        std::string configLine;
    };

    class COMLinkMock final : public Thunder::PluginHost::IShell::ICOMLink {
    public:
        explicit COMLinkMock(AppManagerServiceMock& parent)
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
            return _parent._cfg.remoteConnection;
        }

        void* Instantiate(const Thunder::RPC::Object& object, const uint32_t interfaceId, uint32_t& connectionId) override
        {
            _parent.instantiateCalls++;
            if (_parent._instantiateHandler) {
                return _parent._instantiateHandler(object, interfaceId, connectionId);
            }
            connectionId = 1;
            return nullptr;
        }

    private:
        AppManagerServiceMock& _parent;
    };

    explicit AppManagerServiceMock(Config cfg = Config())
        : _cfg(cfg)
        , _refCount(1)
        , _comLink(*this)
    {
    }

    ~AppManagerServiceMock() override = default;
    AppManagerServiceMock(const AppManagerServiceMock&) = delete;
    AppManagerServiceMock& operator=(const AppManagerServiceMock&) = delete;

    void SetInstantiateHandler(InstantiateHandler handler)
    {
        _instantiateHandler = std::move(handler);
    }

    mutable std::atomic<uint32_t> addRefCalls { 0 };
    mutable std::atomic<uint32_t> releaseCalls { 0 };
    mutable std::atomic<uint32_t> instantiateCalls { 0 };

    uint32_t AddRef() const override
    {
        addRefCalls++;
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t Release() const override
    {
        releaseCalls++;
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (0U == remaining) ? Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED : Thunder::Core::ERROR_NONE;
    }

    std::string Versions() const override { return std::string(); }
    std::string Locator() const override { return std::string(); }
    std::string ClassName() const override { return std::string(); }
    std::string Callsign() const override { return std::string("org.rdk.AppManager"); }
    std::string WebPrefix() const override { return std::string(); }
    std::string ConfigLine() const override { return _cfg.configLine; }
    std::string PersistentPath() const override { return std::string("/tmp"); }
    std::string VolatilePath() const override { return std::string("/tmp"); }
    std::string DataPath() const override { return std::string("/tmp"); }
    std::string SystemPath() const override { return std::string(); }
    std::string PluginPath() const override { return std::string(); }
    std::string SystemRootPath() const override { return std::string(); }
    std::string Model() const override { return std::string(); }
    std::string Accessor() const override { return std::string(); }
    std::string ProxyStubPath() const override { return std::string(); }
    std::string HashKey() const override { return std::string(); }
    std::string Substitute(const std::string&) const override { return std::string(); }

    state State() const override { return state::ACTIVATED; }
    bool Resumed() const override { return false; }
    bool Background() const override { return false; }
    bool IsSupported(const uint8_t) const override { return false; }
    startup Startup() const override { return static_cast<startup>(0); }

    Thunder::Core::hresult Activate(const reason) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Deactivate(const reason) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Unavailable(const reason) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult ConfigLine(const std::string&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult SystemRootPath(const std::string&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Hibernate(const uint32_t) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Startup(const startup) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Resumed(const bool) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Metadata(std::string& info) const override { info.clear(); return Thunder::Core::ERROR_NONE; }

    void EnableWebServer(const std::string&, const std::string&) override {}
    void DisableWebServer() override {}

    Thunder::PluginHost::ISubSystem* SubSystems() override { return nullptr; }
    uint32_t Submit(const uint32_t, const Thunder::Core::ProxyType<Thunder::Core::JSON::IElement>&) override { return Thunder::Core::ERROR_NONE; }
    void Notify(const std::string&) override {}
    void* QueryInterface(const uint32_t) override { return nullptr; }

    void* QueryInterfaceByCallsign(const uint32_t id, const std::string& callsign) override
    {
        if (std::string("org.rdk.AppPackageManager") == callsign) {
            if (Thunder::Exchange::IPackageHandler::ID == id && nullptr != _cfg.packageHandler) {
                _cfg.packageHandler->AddRef();
                return static_cast<Thunder::Exchange::IPackageHandler*>(_cfg.packageHandler);
            }
            if (Thunder::Exchange::IPackageInstaller::ID == id && nullptr != _cfg.installer) {
                _cfg.installer->AddRef();
                return static_cast<Thunder::Exchange::IPackageInstaller*>(_cfg.installer);
            }
        } else if (std::string("org.rdk.PersistentStore") == callsign) {
            if (Thunder::Exchange::IStore2::ID == id && nullptr != _cfg.store2) {
                _cfg.store2->AddRef();
                return static_cast<Thunder::Exchange::IStore2*>(_cfg.store2);
            }
        } else if (std::string("org.rdk.AppStorageManager") == callsign) {
            if (Thunder::Exchange::IAppStorageManager::ID == id && nullptr != _cfg.storageManager) {
                _cfg.storageManager->AddRef();
                return static_cast<Thunder::Exchange::IAppStorageManager*>(_cfg.storageManager);
            }
        } else if (std::string("org.rdk.LifecycleManager") == callsign) {
            if (Thunder::Exchange::ILifecycleManager::ID == id && nullptr != _cfg.lifecycleManager) {
                _cfg.lifecycleManager->AddRef();
                return static_cast<Thunder::Exchange::ILifecycleManager*>(_cfg.lifecycleManager);
            }
            if (Thunder::Exchange::ILifecycleManagerState::ID == id && nullptr != _cfg.lifecycleManagerState) {
                _cfg.lifecycleManagerState->AddRef();
                return static_cast<Thunder::Exchange::ILifecycleManagerState*>(_cfg.lifecycleManagerState);
            }
        }
        return nullptr;
    }

    void Register(Thunder::PluginHost::IPlugin::INotification*) override {}
    void Unregister(Thunder::PluginHost::IPlugin::INotification*) override {}

    reason Reason() const override { return reason::REQUESTED; }

    Thunder::RPC::IRemoteConnection* RemoteConnection(const uint32_t) const
    {
        return _cfg.remoteConnection;
    }

    Thunder::PluginHost::IShell::ICOMLink* COMLink() override
    {
        return &_comLink;
    }

    Config _cfg;

private:
    mutable std::atomic<uint32_t> _refCount;
    InstantiateHandler _instantiateHandler;
    COMLinkMock _comLink;
};

} // namespace L0Test
