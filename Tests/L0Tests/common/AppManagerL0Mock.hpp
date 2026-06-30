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

inline WPEFramework::Exchange::IPackageInstaller::IPackageIterator* CreatePackageIterator(
    const std::vector<WPEFramework::Exchange::IPackageInstaller::Package>& packages)
{
    return WPEFramework::Core::Service<
        WPEFramework::RPC::IteratorType<WPEFramework::Exchange::IPackageInstaller::IPackageIterator>>::Create<
        WPEFramework::Exchange::IPackageInstaller::IPackageIterator>(packages);
}

class MockRemoteConnection final : public WPEFramework::RPC::IRemoteConnection {
public:
    explicit MockRemoteConnection(const uint32_t id = 1)
        : _refCount(1)
        , _id(id)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
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
        return WPEFramework::Core::ERROR_NONE;
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
    : public WPEFramework::Exchange::IAppManager::INotification
    , public WPEFramework::RPC::IRemoteConnection::INotification {
public:
    MockAppManagerNotification()
        : _refCount(1)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IAppManager::INotification::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IAppManager::INotification*>(this);
        }
        if (id == WPEFramework::RPC::IRemoteConnection::INotification::ID) {
            AddRef();
            return static_cast<WPEFramework::RPC::IRemoteConnection::INotification*>(this);
        }
        return nullptr;
    }

    void Activated(WPEFramework::RPC::IRemoteConnection*) override
    {
        activatedCount++;
    }

    void Deactivated(WPEFramework::RPC::IRemoteConnection*) override
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

    void OnAppLifecycleStateChanged(const string&, const string&, const WPEFramework::Exchange::IAppManager::AppLifecycleState,
        const WPEFramework::Exchange::IAppManager::AppLifecycleState, const WPEFramework::Exchange::IAppManager::AppErrorReason) override
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

class MockPackageHandler final : public WPEFramework::Exchange::IPackageHandler {
public:
    using LockHandler = std::function<WPEFramework::Core::hresult(
        const string&, const string&, const LockReason&, uint32_t&, string&, WPEFramework::Exchange::RuntimeConfig&, ILockIterator*&)>;
    using UnlockHandler = std::function<WPEFramework::Core::hresult(const string&, const string&)>;

    MockPackageHandler()
        : _refCount(1)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IPackageHandler::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IPackageHandler*>(this);
        }
        return nullptr;
    }

    WPEFramework::Core::hresult Lock(const string& packageId, const string& version, const LockReason& lockReason,
        uint32_t& lockId, string& unpackedPath, WPEFramework::Exchange::RuntimeConfig& configMetadata, ILockIterator*& appMetadata) override
    {
        lockCount++;
        if (lockHandler) {
            return lockHandler(packageId, version, lockReason, lockId, unpackedPath, configMetadata, appMetadata);
        }
        lockId = 1;
        unpackedPath = "/tmp/app";
        appMetadata = nullptr;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unlock(const string& packageId, const string& version) override
    {
        unlockCount++;
        if (unlockHandler) {
            return unlockHandler(packageId, version);
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult GetLockedInfo(const string&, const string&, string&, WPEFramework::Exchange::RuntimeConfig&, string&, bool&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> lockCount { 0 };
    std::atomic<uint32_t> unlockCount { 0 };
    LockHandler lockHandler;
    UnlockHandler unlockHandler;
};

class MockPackageInstaller final : public WPEFramework::Exchange::IPackageInstaller {
public:
    using Package = WPEFramework::Exchange::IPackageInstaller::Package;
    using ListHandler = std::function<WPEFramework::Core::hresult(IPackageIterator*&)>;
    using ConfigHandler = std::function<WPEFramework::Core::hresult(const string&, string&, string&, WPEFramework::Exchange::RuntimeConfig&)>;

    MockPackageInstaller()
        : _refCount(1)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
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

    WPEFramework::Core::hresult Register(INotification*) override
    {
        registerCount++;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unregister(INotification*) override
    {
        unregisterCount++;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Install(const string&, const string&, IKeyValueIterator* const&, const string&, FailReason&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Uninstall(const string&, string&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult ListPackages(IPackageIterator*& packages) override
    {
        listPackagesCount++;
        if (listHandler) {
            return listHandler(packages);
        }
        packages = CreatePackageIterator(installedPackages);
        return (nullptr != packages) ? WPEFramework::Core::ERROR_NONE : WPEFramework::Core::ERROR_GENERAL;
    }

    WPEFramework::Core::hresult Config(const string&, const string&, WPEFramework::Exchange::RuntimeConfig&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult PackageState(const string&, const string&, InstallState&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult GetConfigForPackage(const string& fileLocator, string& id, string& version, WPEFramework::Exchange::RuntimeConfig& config) override
    {
        getConfigCount++;
        if (configHandler) {
            return configHandler(fileLocator, id, version, config);
        }
        return WPEFramework::Core::ERROR_GENERAL;
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

class MockStore2 final : public WPEFramework::Exchange::IStore2 {
public:
    MockStore2()
        : _refCount(1)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IStore2::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IStore2*>(this);
        }
        return nullptr;
    }

    uint32_t Register(INotification*) override { return WPEFramework::Core::ERROR_NONE; }
    uint32_t Unregister(INotification*) override { return WPEFramework::Core::ERROR_NONE; }

    uint32_t SetValue(const ScopeType, const string& ns, const string& key, const string& value, const uint32_t ttl) override
    {
        (void)ttl;
        data[ns + ":" + key] = value;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t GetValue(const ScopeType, const string& ns, const string& key, string& value, uint32_t& ttl) override
    {
        (void)ttl;
        const auto it = data.find(ns + ":" + key);
        if (it != data.end()) {
            value = it->second;
            return WPEFramework::Core::ERROR_NONE;
        }
        value.clear();
        return WPEFramework::Core::ERROR_GENERAL;
    }

    uint32_t DeleteKey(const ScopeType, const string& ns, const string& key) override
    {
        data.erase(ns + ":" + key);
        return WPEFramework::Core::ERROR_NONE;
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
        return WPEFramework::Core::ERROR_NONE;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::map<std::string, std::string> data;
};

class MockStorageManager final : public WPEFramework::Exchange::IAppStorageManager {
public:
    MockStorageManager()
        : _refCount(1)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IAppStorageManager::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IAppStorageManager*>(this);
        }
        return nullptr;
    }

    WPEFramework::Core::hresult CreateStorage(const string&, const uint32_t&, string&, string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetStorage(const string&, const int32_t&, const int32_t&, string&, uint32_t&, uint32_t&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult DeleteStorage(const string&, string&) override { return WPEFramework::Core::ERROR_NONE; }

    using ClearHandler = std::function<WPEFramework::Core::hresult(const string&, string&)>;

    WPEFramework::Core::hresult Clear(const string& appId, string& errorReason) override
    {
        if (clearHandler) { return clearHandler(appId, errorReason); }
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult ClearAll(const string& exemptedAppIds, string& errorReason) override
    {
        if (clearAllHandler) { return clearAllHandler(exemptedAppIds, errorReason); }
        return WPEFramework::Core::ERROR_NONE;
    }

    mutable std::atomic<uint32_t> _refCount;
    ClearHandler clearHandler;
    ClearHandler clearAllHandler;
};

class MockLifecycleManagerState final : public WPEFramework::Exchange::ILifecycleManagerState {
public:
    MockLifecycleManagerState()
        : _refCount(1)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::ILifecycleManagerState::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::ILifecycleManagerState*>(this);
        }
        return nullptr;
    }

    WPEFramework::Core::hresult Register(INotification* notification) override
    {
        registeredNotification = notification;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unregister(INotification*) override
    {
        registeredNotification = nullptr;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult AppReady(const string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult StateChangeComplete(const string&, const uint32_t, const bool) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult CloseApp(const string&, const AppCloseReason) override { return WPEFramework::Core::ERROR_NONE; }

    mutable std::atomic<uint32_t> _refCount;
    INotification* registeredNotification { nullptr };
};

class MockLifecycleManager final : public WPEFramework::Exchange::ILifecycleManager {
public:
    MockLifecycleManager()
        : _refCount(1)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::ILifecycleManager::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::ILifecycleManager*>(this);
        }
        return nullptr;
    }

    WPEFramework::Core::hresult Register(INotification* notification) override
    {
        registeredNotification = notification;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unregister(INotification*) override
    {
        registeredNotification = nullptr;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult GetLoadedApps(const bool, std::string& apps) override
    {
        if (getLoadedAppsHandler) {
            return getLoadedAppsHandler(apps);
        }
        apps = loadedAppsJson;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult IsAppLoaded(const std::string& appId, bool& loaded) const override
    {
        if (isAppLoadedHandler) {
            return isAppLoadedHandler(appId, loaded);
        }
        loaded = false;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult SpawnApp(const string& appId, const string& launchIntent, const LifecycleState targetLifecycleState,
        const WPEFramework::Exchange::RuntimeConfig& runtimeConfigObject, const string& launchArgs, string& appInstanceId,
        string& errorReason, bool& success) override
    {
        (void)runtimeConfigObject;
        if (spawnAppHandler) {
            return spawnAppHandler(appId, launchIntent, targetLifecycleState, runtimeConfigObject, launchArgs, appInstanceId, errorReason, success);
        }
        appInstanceId = "instance-1";
        errorReason.clear();
        success = true;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t SetTargetAppState(const string& appInstanceId, const LifecycleState targetLifecycleState, const string& launchIntent) override
    {
        if (setTargetAppStateHandler) {
            return setTargetAppStateHandler(appInstanceId, targetLifecycleState, launchIntent);
        }
        (void)appInstanceId;
        (void)targetLifecycleState;
        (void)launchIntent;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult UnloadApp(const string& appInstanceId, string& errorReason, bool& success) override
    {
        if (unloadAppHandler) {
            return unloadAppHandler(appInstanceId, errorReason, success);
        }
        errorReason.clear();
        success = true;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult KillApp(const string& appInstanceId, string& errorReason, bool& success) override
    {
        if (killAppHandler) {
            return killAppHandler(appInstanceId, errorReason, success);
        }
        errorReason.clear();
        success = true;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult SendIntentToActiveApp(const string& appInstanceId, const string& intent, string& errorReason, bool& success) override
    {
        if (sendIntentHandler) {
            return sendIntentHandler(appInstanceId, intent, errorReason, success);
        }
        errorReason.clear();
        success = true;
        return WPEFramework::Core::ERROR_NONE;
    }

    mutable std::atomic<uint32_t> _refCount;
    INotification* registeredNotification { nullptr };
    std::function<WPEFramework::Core::hresult(const std::string&, bool&)> isAppLoadedHandler;
    std::function<WPEFramework::Core::hresult(std::string&)> getLoadedAppsHandler;
    std::function<WPEFramework::Core::hresult(const string&, const string&, const LifecycleState, const WPEFramework::Exchange::RuntimeConfig&, const string&, string&, string&, bool&)> spawnAppHandler;
    std::function<uint32_t(const string&, const LifecycleState, const string&)> setTargetAppStateHandler;
    std::function<WPEFramework::Core::hresult(const string&, string&, bool&)> unloadAppHandler;
    std::function<WPEFramework::Core::hresult(const string&, string&, bool&)> killAppHandler;
    std::function<WPEFramework::Core::hresult(const string&, const string&, string&, bool&)> sendIntentHandler;
    std::string loadedAppsJson;
};

class AppManagerServiceMock final : public WPEFramework::PluginHost::IShell {
public:
    using InstantiateHandler = std::function<void*(const WPEFramework::RPC::Object&, const uint32_t, uint32_t&)>;

    struct Config {
        explicit Config(WPEFramework::Exchange::IPackageInstaller* installer = nullptr)
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

        WPEFramework::Exchange::IPackageInstaller* installer;
        WPEFramework::Exchange::IPackageHandler* packageHandler;
        WPEFramework::Exchange::IStore2* store2;
        WPEFramework::Exchange::IAppStorageManager* storageManager;
        WPEFramework::Exchange::ILifecycleManager* lifecycleManager;
        WPEFramework::Exchange::ILifecycleManagerState* lifecycleManagerState;
        WPEFramework::RPC::IRemoteConnection* remoteConnection;
        std::string configLine;
    };

    class COMLinkMock final : public WPEFramework::PluginHost::IShell::ICOMLink {
    public:
        explicit COMLinkMock(AppManagerServiceMock& parent)
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
            return _parent._cfg.remoteConnection;
        }

        void* Instantiate(const WPEFramework::RPC::Object& object, const uint32_t interfaceId, uint32_t& connectionId) override
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

    void AddRef() const override
    {
        addRefCalls++;
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        releaseCalls++;
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (0U == remaining) ? WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED : WPEFramework::Core::ERROR_NONE;
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

    WPEFramework::Core::hresult Activate(const reason) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Deactivate(const reason) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Unavailable(const reason) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult ConfigLine(const std::string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SystemRootPath(const std::string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Hibernate(const uint32_t) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Startup(const startup) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Resumed(const bool) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Metadata(std::string& info) const override { info.clear(); return WPEFramework::Core::ERROR_NONE; }

    void EnableWebServer(const std::string&, const std::string&) override {}
    void DisableWebServer() override {}

    WPEFramework::PluginHost::ISubSystem* SubSystems() override { return nullptr; }
    uint32_t Submit(const uint32_t, const WPEFramework::Core::ProxyType<WPEFramework::Core::JSON::IElement>&) override { return WPEFramework::Core::ERROR_NONE; }
    void Notify(const std::string&) override {}
    void* QueryInterface(const uint32_t) override { return nullptr; }

    void* QueryInterfaceByCallsign(const uint32_t id, const std::string& callsign) override
    {
        if (std::string("org.rdk.AppPackageManager") == callsign) {
            if (WPEFramework::Exchange::IPackageHandler::ID == id && nullptr != _cfg.packageHandler) {
                _cfg.packageHandler->AddRef();
                return static_cast<WPEFramework::Exchange::IPackageHandler*>(_cfg.packageHandler);
            }
            if (WPEFramework::Exchange::IPackageInstaller::ID == id && nullptr != _cfg.installer) {
                _cfg.installer->AddRef();
                return static_cast<WPEFramework::Exchange::IPackageInstaller*>(_cfg.installer);
            }
        } else if (std::string("org.rdk.PersistentStore") == callsign) {
            if (WPEFramework::Exchange::IStore2::ID == id && nullptr != _cfg.store2) {
                _cfg.store2->AddRef();
                return static_cast<WPEFramework::Exchange::IStore2*>(_cfg.store2);
            }
        } else if (std::string("org.rdk.AppStorageManager") == callsign) {
            if (WPEFramework::Exchange::IAppStorageManager::ID == id && nullptr != _cfg.storageManager) {
                _cfg.storageManager->AddRef();
                return static_cast<WPEFramework::Exchange::IAppStorageManager*>(_cfg.storageManager);
            }
        } else if (std::string("org.rdk.LifecycleManager") == callsign) {
            if (WPEFramework::Exchange::ILifecycleManager::ID == id && nullptr != _cfg.lifecycleManager) {
                _cfg.lifecycleManager->AddRef();
                return static_cast<WPEFramework::Exchange::ILifecycleManager*>(_cfg.lifecycleManager);
            }
            if (WPEFramework::Exchange::ILifecycleManagerState::ID == id && nullptr != _cfg.lifecycleManagerState) {
                _cfg.lifecycleManagerState->AddRef();
                return static_cast<WPEFramework::Exchange::ILifecycleManagerState*>(_cfg.lifecycleManagerState);
            }
        }
        return nullptr;
    }

    void Register(WPEFramework::PluginHost::IPlugin::INotification*) override {}
    void Unregister(WPEFramework::PluginHost::IPlugin::INotification*) override {}

    reason Reason() const override { return reason::REQUESTED; }

    WPEFramework::RPC::IRemoteConnection* RemoteConnection(const uint32_t) const
    {
        return _cfg.remoteConnection;
    }

    WPEFramework::PluginHost::IShell::ICOMLink* COMLink() override
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
