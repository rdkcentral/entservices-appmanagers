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

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

#include <core/core.h>
#include <plugins/plugins.h>
#include <plugins/IShell.h>
#include <interfaces/IDownloadManager.h>

#include "common/L0ServiceMock.hpp"

namespace L0Test {

using string = std::string;

// ─────────────────────────────────────────────────────────────────────────────
// FakeSubSystem
// Controls whether the INTERNET subsystem is "active" so that
// DownloadManagerImplementation::Download() can be steered to both the
// success and the ERROR_UNAVAILABLE code paths.
// ─────────────────────────────────────────────────────────────────────────────

class FakeSubSystem final : public WPEFramework::PluginHost::ISubSystem {
public:
    explicit FakeSubSystem(bool internetActive = true)
        : _refCount(1)
        , _internetActive(internetActive)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t r = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == r) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t /*id*/) override { return nullptr; }

    bool IsActive(const subsystem type) const override
    {
        if (WPEFramework::PluginHost::ISubSystem::INTERNET == type) {
            return _internetActive;
        }
        return false;
    }

    void SetInternetActive(bool v) { _internetActive = v; }

    // Pure-virtual stubs
    void Register(WPEFramework::PluginHost::ISubSystem::INotification*) override {}
    void Unregister(WPEFramework::PluginHost::ISubSystem::INotification*) override {}
    void Set(const WPEFramework::PluginHost::ISubSystem::subsystem, WPEFramework::Core::IUnknown*) override {}
    const WPEFramework::Core::IUnknown* Get(const WPEFramework::PluginHost::ISubSystem::subsystem) const override { return nullptr; }
    string BuildTreeHash() const override { return ""; }
    string Version() const override { return ""; }

    mutable std::atomic<uint32_t> _refCount;
    bool _internetActive;
};

// ─────────────────────────────────────────────────────────────────────────────
// ServiceMock
// Minimal PluginHost::IShell implementation for DownloadManager L0 tests.
// Supports:
//   - Controllable ConfigLine (JSON string consumed by Initialize())
//   - Controllable INTERNET sub-system via FakeSubSystem
//   - InstantiateHandler hook for Root<IDownloadManager>() in shell plugin tests
//   - Observability counters: addRefCalls, releaseCalls, instantiateCalls
// ─────────────────────────────────────────────────────────────────────────────

class ServiceMock final : public WPEFramework::PluginHost::IShell,
                          public L0Test::L0ServiceMock {
public:
    using InstantiateHandler =
        std::function<void*(const WPEFramework::RPC::Object&, const uint32_t, uint32_t&)>;

    // Configures which sub-systems are injected and what the config line contains.
    struct Config {
        bool        internetActive;
        std::string configLine;
        Config() : internetActive(true), configLine("{}") {}
    };

    // ── Nested COMLinkMock ────────────────────────────────────────────────────
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

    // ── Construction ─────────────────────────────────────────────────────────
    explicit ServiceMock(Config cfg = Config())
        : _refCount(1)
        , _cfg(cfg)
        , _subSystem(new FakeSubSystem(cfg.internetActive))
        , _comLink(*this)
    {
    }

    ~ServiceMock() override
    {
        _subSystem->Release();
    }

    ServiceMock(const ServiceMock&) = delete;
    ServiceMock& operator=(const ServiceMock&) = delete;

    void SetInstantiateHandler(InstantiateHandler handler)
    {
        _instantiateHandler = handler;
    }

    void SetConfigLine(const std::string& cfg)
    {
        _cfg.configLine = cfg;
    }

    void SetInternetActive(bool v)
    {
        _subSystem->SetInternetActive(v);
    }

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

    void* QueryInterface(const uint32_t /*id*/) override { return nullptr; }

    // ── IShell ────────────────────────────────────────────────────────────────
    std::string Callsign() const override      { return "org.rdk.DownloadManager"; }
    std::string ConfigLine() const override    { return _cfg.configLine; }
    std::string PersistentPath() const override { return "/tmp"; }
    std::string VolatilePath() const override   { return "/tmp"; }
    std::string DataPath() const override       { return "/tmp"; }
    std::string Versions() const override      { return ""; }
    std::string Locator() const override       { return ""; }
    std::string ClassName() const override     { return ""; }
    std::string WebPrefix() const override     { return ""; }
    std::string SystemPath() const override    { return ""; }
    std::string PluginPath() const override    { return ""; }
    std::string Model() const override         { return ""; }
    std::string Accessor() const override      { return ""; }
    std::string ProxyStubPath() const override { return ""; }
    std::string HashKey() const override       { return ""; }
    std::string Substitute(const std::string&) const override { return ""; }
    std::string SystemRootPath() const override { return ""; }

    state State() const override { return state::ACTIVATED; }
    bool Resumed() const override { return false; }
    bool IsSupported(const uint8_t) const override { return false; }
    bool Background() const override { return false; }
    startup Startup() const override { return static_cast<startup>(0); }
    reason Reason() const override { return reason::REQUESTED; }

    WPEFramework::PluginHost::ISubSystem* SubSystems() override
    {
        _subSystem->AddRef();
        return _subSystem;
    }

    WPEFramework::Core::hresult Activate(const reason) override   { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Deactivate(const reason) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Unavailable(const reason) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult ConfigLine(const std::string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SystemRootPath(const std::string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Hibernate(const uint32_t) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Startup(const startup) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Resumed(const bool) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Metadata(std::string& info) const override
    {
        info.clear();
        return WPEFramework::Core::ERROR_NONE;
    }

    void EnableWebServer(const std::string&, const std::string&) override {}
    void DisableWebServer() override {}

    uint32_t Submit(const uint32_t,
                    const WPEFramework::Core::ProxyType<WPEFramework::Core::JSON::IElement>&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    void Notify(const std::string&) override {}

    void Register(WPEFramework::PluginHost::IPlugin::INotification*) override {}
    void Unregister(WPEFramework::PluginHost::IPlugin::INotification*) override {}

    void* QueryInterfaceByCallsign(const uint32_t id, const std::string& callsign) override
    {
        // Delegate to L0ServiceMock registry
        WPEFramework::Core::IUnknown* iface =
            L0Test::L0ServiceMock::QueryInterfaceByCallsign(callsign, id);
        if (nullptr != iface) {
            iface->AddRef();
            return iface;
        }
        return nullptr;
    }

    WPEFramework::RPC::IRemoteConnection* RemoteConnection(const uint32_t id) override
    {
        remoteConnectionCalls++;
        return nullptr;
    }

    WPEFramework::PluginHost::IShell::ICOMLink* COMLink() override
    {
        return &_comLink;
    }

    // Observability counters
    mutable std::atomic<uint32_t> addRefCalls       { 0 };
    mutable std::atomic<uint32_t> releaseCalls      { 0 };
    mutable std::atomic<uint32_t> remoteConnectionCalls { 0 };
    mutable std::atomic<uint32_t> instantiateCalls  { 0 };

private:
    mutable std::atomic<uint32_t> _refCount;
    Config                        _cfg;
    FakeSubSystem*                _subSystem;
    InstantiateHandler            _instantiateHandler;
    COMLinkMock                   _comLink;
};

// ─────────────────────────────────────────────────────────────────────────────
// FakeDownloadManagerImpl
// Minimal IDownloadManager fake returned by the shell Initialize() test when
// Root<IDownloadManager>() needs to succeed without a real out-of-process impl.
// ─────────────────────────────────────────────────────────────────────────────

class FakeDownloadManagerImpl final : public WPEFramework::Exchange::IDownloadManager {
public:
    FakeDownloadManagerImpl()
        : _refCount(1)
        , registerCalls(0)
        , unregisterCalls(0)
        , initializeCalls(0)
        , deinitializeCalls(0)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

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
        if (WPEFramework::Exchange::IDownloadManager::ID == id) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IDownloadManager*>(this);
        }
        return nullptr;
    }

    WPEFramework::Core::hresult Register(INotification* /*notification*/) override
    {
        registerCalls++;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unregister(INotification* /*notification*/) override
    {
        unregisterCalls++;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Initialize(WPEFramework::PluginHost::IShell* /*service*/) override
    {
        initializeCalls++;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Deinitialize(WPEFramework::PluginHost::IShell* /*service*/) override
    {
        deinitializeCalls++;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Download(const string& /*url*/,
                                          const Options& /*options*/,
                                          string& downloadId) override
    {
        downloadId = "2001";
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Pause(const string& /*downloadId*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Resume(const string& /*downloadId*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Cancel(const string& /*downloadId*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Delete(const string& /*fileLocator*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Progress(const string& /*downloadId*/, uint8_t& percent) override
    {
        percent = 55;
        return WPEFramework::Core::ERROR_NONE;
    }
    WPEFramework::Core::hresult GetStorageDetails(uint32_t& quotaKB, uint32_t& usedKB) override
    {
        quotaKB = 0;
        usedKB  = 0;
        return WPEFramework::Core::ERROR_NONE;
    }
    WPEFramework::Core::hresult RateLimit(const string& /*downloadId*/, const uint32_t& /*limit*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> registerCalls;
    std::atomic<uint32_t> unregisterCalls;
    std::atomic<uint32_t> initializeCalls;
    std::atomic<uint32_t> deinitializeCalls;
};

// ─────────────────────────────────────────────────────────────────────────────
// FakeDownloadNotification
// Counts OnAppDownloadStatus calls and stores the last JSON payload for
// assertion in notifications-dispatch tests.
// ─────────────────────────────────────────────────────────────────────────────

class FakeDownloadNotification final
    : public WPEFramework::Exchange::IDownloadManager::INotification
{
public:
    FakeDownloadNotification()
        : _refCount(1)
        , onAppDownloadStatusCount(0)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

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
        if (WPEFramework::Exchange::IDownloadManager::INotification::ID == id) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IDownloadManager::INotification*>(this);
        }
        return nullptr;
    }

    void OnAppDownloadStatus(const std::string& jsonresponse) override
    {
        lastJson = jsonresponse;
        onAppDownloadStatusCount++;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t>         onAppDownloadStatusCount;
    std::string                   lastJson;
};

} // namespace L0Test
