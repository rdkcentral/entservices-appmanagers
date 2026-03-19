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
 * Minimal PluginHost::IShell implementation for AppManager L0 tests.
 * Mirrors the pattern used in Tests/L0Tests/RuntimeManager/ServiceMock.h
 * but stripped of OCIContainer/WindowManager dependencies.
 *
 * Usage:
 *   L0Test::AppManagerServiceMock service;
 *   service.SetInstantiateHandler([](const WPEFramework::RPC::Object&, const uint32_t, uint32_t& id) -> void* {
 *       id = 42;
 *       return static_cast<void*>(new MyFakeAppManagerImpl());
 *   });
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

#include <core/core.h>
#include <plugins/plugins.h>
#include <plugins/IShell.h>

namespace L0Test {

using string = std::string;

/**
 * @brief Minimal IShell implementation for AppManager L0 tests.
 *
 * - All QueryInterfaceByCallsign calls return nullptr by default (no remote plugins).
 * - Instantiate behaviour is configurable via SetInstantiateHandler to control Root<T>()
 *   outcomes in AppManager::Initialize tests.
 * - AddRef / Release call counts are tracked for assertion in lifecycle tests.
 */
class AppManagerServiceMock final : public WPEFramework::PluginHost::IShell {
public:
    using InstantiateHandler = std::function<void*(
        const WPEFramework::RPC::Object&, const uint32_t, uint32_t&)>;

    // ─────────────────────────────────────────────────────────────────────────
    // Inner COMLinkMock – forwards Instantiate to parent's configurable handler
    // ─────────────────────────────────────────────────────────────────────────
    class COMLinkMock final : public WPEFramework::PluginHost::IShell::ICOMLink {
    public:
        explicit COMLinkMock(AppManagerServiceMock& parent)
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
        AppManagerServiceMock& _parent;
    };

    // ─────────────────────────────────────────────────────────────────────────
    // Construction / destruction
    // ─────────────────────────────────────────────────────────────────────────
    AppManagerServiceMock()
        : _refCount(1)
        , _comLink(*this)
    {
    }

    ~AppManagerServiceMock() override = default;

    AppManagerServiceMock(const AppManagerServiceMock&) = delete;
    AppManagerServiceMock& operator=(const AppManagerServiceMock&) = delete;

    void SetInstantiateHandler(InstantiateHandler handler)
    {
        _instantiateHandler = handler;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Observability counters
    // ─────────────────────────────────────────────────────────────────────────
    mutable std::atomic<uint32_t> addRefCalls { 0 };
    mutable std::atomic<uint32_t> releaseCalls { 0 };
    mutable std::atomic<uint32_t> instantiateCalls { 0 };

    // ─────────────────────────────────────────────────────────────────────────
    // IUnknown
    // ─────────────────────────────────────────────────────────────────────────
    void AddRef() const override
    {
        addRefCalls++;
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        releaseCalls++;
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (remaining == 0U)
            ? WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED
            : WPEFramework::Core::ERROR_NONE;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // IShell pure-virtual stubs with safe defaults
    // ─────────────────────────────────────────────────────────────────────────
    string Versions() const override { return ""; }
    string Locator() const override { return ""; }
    string ClassName() const override { return ""; }
    string Callsign() const override { return "org.rdk.AppManager"; }
    string WebPrefix() const override { return ""; }
    string ConfigLine() const override { return "{}"; }
    string PersistentPath() const override { return "/tmp"; }
    string VolatilePath() const override { return "/tmp"; }
    string DataPath() const override { return "/tmp"; }
    state State() const override { return state::ACTIVATED; }
    bool Resumed() const override { return false; }
    bool IsSupported(const uint8_t) const override { return false; }
    string Model() const override { return ""; }
    bool Background() const override { return false; }
    string Accessor() const override { return ""; }
    string ProxyStubPath() const override { return ""; }
    string HashKey() const override { return ""; }
    string SystemPath() const override { return ""; }
    string PluginPath() const override { return ""; }
    string Substitute(const string&) const override { return ""; }
    string SystemRootPath() const override { return ""; }

    startup Startup() const override { return static_cast<startup>(0); }
    reason Reason() const override { return reason::REQUESTED; }

    WPEFramework::Core::hresult Activate(const reason) override
        { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Deactivate(const reason) override
        { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Unavailable(const reason) override
        { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult ConfigLine(const string&) override
        { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SystemRootPath(const string&) override
        { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Hibernate(const uint32_t) override
        { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Startup(const startup) override
        { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Resumed(const bool) override
        { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Metadata(string& info) const override
        { info.clear(); return WPEFramework::Core::ERROR_NONE; }

    void EnableWebServer(const string&, const string&) override {}
    void DisableWebServer() override {}
    void Notify(const string&) override {}

    WPEFramework::PluginHost::ISubSystem* SubSystems() override { return nullptr; }

    uint32_t Submit(const uint32_t,
                    const WPEFramework::Core::ProxyType<WPEFramework::Core::JSON::IElement>&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t) override { return nullptr; }

    /* All remote plugin queries return nullptr — simulates missing out-of-process plugins. */
    void* QueryInterfaceByCallsign(const uint32_t /*id*/,
                                   const string& /*callsign*/) override
    {
        return nullptr;
    }

    void Register(WPEFramework::PluginHost::IPlugin::INotification*) override {}
    void Unregister(WPEFramework::PluginHost::IPlugin::INotification*) override {}
    void Register(WPEFramework::RPC::IRemoteConnection::INotification*) {}
    void Unregister(const WPEFramework::RPC::IRemoteConnection::INotification*) {}

    WPEFramework::RPC::IRemoteConnection* RemoteConnection(const uint32_t) const
    {
        return nullptr;
    }

    ICOMLink* COMLink() override
    {
        return &_comLink;
    }

private:
    mutable std::atomic<uint32_t> _refCount;
    InstantiateHandler             _instantiateHandler;
    COMLinkMock                    _comLink;
};

} // namespace L0Test
