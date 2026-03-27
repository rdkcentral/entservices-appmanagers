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

/**
 * @file L0ShellMock.hpp
 * @brief Reusable base PluginHost::IShell mock for L0 tests.
 *
 * All plugin L0 test suites (PreinstallManager, RuntimeManager, etc.) that need
 * an IShell stand-in can extend this class and override only what they need.
 *
 * Features:
 *   - Implements every IShell pure-virtual with safe no-op defaults.
 *   - Provides a configurable InstantiateHandler so Root<T>() can return a fake
 *     implementation object during Initialize() tests.
 *   - Provides a mutable ConfigLine so JSON config parsing can be exercised.
 *   - Exposes call-count atomics (addRefCalls, releaseCalls, etc.) for assertions.
 *
 * Usage:
 *   class MyPluginServiceMock : public L0Test::L0ShellMock {
 *   public:
 *       void* QueryInterfaceByCallsign(const uint32_t id,
 *                                      const std::string& callsign) override {
 *           if (callsign == "org.rdk.MyDep" && _dep) {
 *               _dep->AddRef();
 *               return static_cast<void*>(_dep);
 *           }
 *           return nullptr;
 *       }
 *   };
 */

#include <atomic>
#include <functional>
#include <string>

#include <core/core.h>
#include <plugins/plugins.h>

namespace L0Test {

/**
 * @brief Base IShell mock with configurable instantiation and call tracking.
 *
 * PUBLIC_INTERFACE
 */
class L0ShellMock : public WPEFramework::PluginHost::IShell {
public:
    using InstantiateHandler =
        std::function<void*(const WPEFramework::RPC::Object&, const uint32_t, uint32_t&)>;

    // ── Inner COMLink mock ──────────────────────────────────────────────────
    class COMLinkMock final : public WPEFramework::PluginHost::IShell::ICOMLink {
    public:
        explicit COMLinkMock(L0ShellMock& parent) : _parent(parent) {}
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
        L0ShellMock& _parent;
    };

    // ── Construction ────────────────────────────────────────────────────────
    explicit L0ShellMock()
        : _refCount(1)
        , _comLink(*this)
        , _configLine("{}")
        , _callsign("org.rdk.Plugin")
    {
    }

    ~L0ShellMock() override = default;

    L0ShellMock(const L0ShellMock&)            = delete;
    L0ShellMock& operator=(const L0ShellMock&) = delete;

    // ── Test helpers ────────────────────────────────────────────────────────
    /** Set the handler invoked when Root<T>() / Instantiate() is called during Initialize(). */
    void SetInstantiateHandler(InstantiateHandler handler)
    {
        _instantiateHandler = std::move(handler);
    }

    /** Set the JSON config string returned by ConfigLine(). */
    void SetConfigLine(const std::string& cfg)
    {
        _configLine = cfg;
    }

    /** Set the callsign returned by Callsign(). */
    void SetCallsign(const std::string& cs)
    {
        _callsign = cs;
    }

    // ── Observability ────────────────────────────────────────────────────────
    mutable std::atomic<uint32_t> addRefCalls    { 0 };
    mutable std::atomic<uint32_t> releaseCalls   { 0 };
    mutable std::atomic<uint32_t> remoteConnectionCalls { 0 };
    mutable std::atomic<uint32_t> instantiateCalls { 0 };

    // ── IUnknown ────────────────────────────────────────────────────────────
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

    // ── IShell ──────────────────────────────────────────────────────────────
    std::string Versions()       const override { return ""; }
    std::string Locator()        const override { return ""; }
    std::string ClassName()      const override { return ""; }
    std::string Callsign()       const override { return _callsign; }
    std::string WebPrefix()      const override { return ""; }
    std::string ConfigLine()     const override { return _configLine; }
    std::string PersistentPath() const override { return "/tmp"; }
    std::string VolatilePath()   const override { return "/tmp"; }
    std::string DataPath()       const override { return "/tmp"; }
    std::string SystemPath()     const override { return ""; }
    std::string PluginPath()     const override { return ""; }
    std::string Model()          const override { return ""; }
    std::string Accessor()       const override { return ""; }
    std::string ProxyStubPath()  const override { return ""; }
    std::string HashKey()        const override { return ""; }
    std::string SystemRootPath() const override { return ""; }

    std::string Substitute(const std::string&) const override { return ""; }

    state State() const override { return state::ACTIVATED; }

    WPEFramework::Core::hresult Activate(const reason) override
        { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Deactivate(const reason) override
        { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Unavailable(const reason) override
        { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult ConfigLine(const std::string&) override
        { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SystemRootPath(const std::string&) override
        { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Hibernate(const uint32_t) override
        { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Resumed(const bool) override
        { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Startup(const startup) override
        { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Metadata(std::string& info) const override
        { info.clear(); return WPEFramework::Core::ERROR_NONE; }

    bool Resumed()                   const override { return false; }
    bool IsSupported(const uint8_t)  const override { return false; }
    bool Background()                const override { return false; }

    startup Startup() const override { return static_cast<startup>(0); }
    reason  Reason()  const override { return reason::REQUESTED; }

    void EnableWebServer(const std::string&, const std::string&) override {}
    void DisableWebServer() override {}
    void Notify(const std::string&) override {}

    WPEFramework::PluginHost::ISubSystem* SubSystems() override { return nullptr; }

    uint32_t Submit(const uint32_t,
                    const WPEFramework::Core::ProxyType<WPEFramework::Core::JSON::IElement>&) override
        { return WPEFramework::Core::ERROR_NONE; }

    void* QueryInterface(const uint32_t) override { return nullptr; }

    void* QueryInterfaceByCallsign(const uint32_t /*id*/,
                                   const std::string& /*callsign*/) override
        { return nullptr; }

    void Register(WPEFramework::PluginHost::IPlugin::INotification*) override {}
    void Unregister(WPEFramework::PluginHost::IPlugin::INotification*) override {}

    WPEFramework::PluginHost::IShell::ICOMLink* COMLink() override { return &_comLink; }

    WPEFramework::RPC::IRemoteConnection* RemoteConnection(const uint32_t) const
    {
        remoteConnectionCalls++;
        return nullptr;
    }

private:
    mutable std::atomic<uint32_t> _refCount;
    InstantiateHandler             _instantiateHandler;
    COMLinkMock                    _comLink;
    std::string                    _configLine;
    std::string                    _callsign;
};

} // namespace L0Test
