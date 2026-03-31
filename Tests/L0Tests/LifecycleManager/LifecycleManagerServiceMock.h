/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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
#include <interfaces/ILifecycleManager.h>
#include <interfaces/ILifecycleManagerState.h>
#include <interfaces/IConfiguration.h>
#include <interfaces/IRuntimeManager.h>
#include <interfaces/IRDKWindowManager.h>

namespace L0Test {

using string = std::string;

// ──────────────────────────────────────────────────────────────────────────────
// Fake IRuntimeManager stub — allows RuntimeManagerHandler::initialize() to
// succeed so that StateTransitionHandler::initialize() is always reached and
// the semaphore is properly initialised before terminate() is ever called.
// Follows the same conventions as RuntimeManager/ServiceMock.h FakeOCIContainer:
// proper _refCount tracking, no self-delete (caller owns lifetime on stack).
// ──────────────────────────────────────────────────────────────────────────────

class FakeRuntimeManager final : public WPEFramework::Exchange::IRuntimeManager {
public:
    FakeRuntimeManager()
        : _refCount(1)
        , registerCalls(0)
        , unregisterCalls(0)
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

    void* QueryInterface(const uint32_t /*id*/) override { return nullptr; }

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

    WPEFramework::Core::hresult Run(
        const string& /*appId*/, const string& /*appInstanceId*/,
        const uint32_t /*userId*/, const uint32_t /*groupId*/,
        IValueIterator* const& /*ports*/, IStringIterator* const& /*paths*/,
        IStringIterator* const& /*debugSettings*/,
        const WPEFramework::Exchange::RuntimeConfig& /*runtimeConfigObject*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Hibernate(const string& /*appInstanceId*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Wake(const string& /*appInstanceId*/, RuntimeState /*runtimeState*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Suspend(const string& /*appInstanceId*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Resume(const string& /*appInstanceId*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Terminate(const string& /*appInstanceId*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Kill(const string& /*appInstanceId*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetInfo(const string& /*appInstanceId*/, string& /*info*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Annotate(const string& /*appInstanceId*/, const string& /*key*/, const string& /*value*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Mount() override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Unmount() override { return WPEFramework::Core::ERROR_NONE; }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> registerCalls;
    std::atomic<uint32_t> unregisterCalls;
};

// ──────────────────────────────────────────────────────────────────────────────
// Fake IRDKWindowManager stub — allows WindowManagerHandler::initialize() to
// succeed so the full Configure() chain completes without crash.
// Follows RuntimeManager/ServiceMock.h FakeWindowManager conventions:
// proper _refCount tracking, 11-param CreateDisplay matching the installed
// IRDKWindowManager interface, no self-delete (stack-allocated by tests).
// ──────────────────────────────────────────────────────────────────────────────

class FakeWindowManager final : public WPEFramework::Exchange::IRDKWindowManager {
public:
    FakeWindowManager()
        : _refCount(1)
        , registerCalls(0)
        , unregisterCalls(0)
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

    void* QueryInterface(const uint32_t /*id*/) override { return nullptr; }

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

    WPEFramework::Core::hresult Initialize(WPEFramework::PluginHost::IShell* /*service*/) override   { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Deinitialize(WPEFramework::PluginHost::IShell* /*service*/) override { return WPEFramework::Core::ERROR_NONE; }

    WPEFramework::Core::hresult CreateDisplay(
        const string& /*appInstanceId*/, const string& /*displayName*/,
        const uint32_t /*displayWidth*/, const uint32_t /*displayHeight*/,
        const bool /*virtualDisplay*/,
        const uint32_t /*virtualWidth*/, const uint32_t /*virtualHeight*/,
        const uint32_t /*ownerId*/, const uint32_t /*groupId*/,
        const bool /*topmost*/, const bool /*focus*/) override { return WPEFramework::Core::ERROR_NONE; }

    WPEFramework::Core::hresult GetApps(string& /*appsIds*/) const override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult AddKeyIntercept(const string& /*intercept*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult AddKeyIntercepts(const string& /*clientId*/, const string& /*intercepts*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult RemoveKeyIntercept(const string& /*clientId*/, uint32_t /*keyCode*/, const string& /*modifiers*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult AddKeyListener(const string& /*keyListeners*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult RemoveKeyListener(const string& /*keyListeners*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult InjectKey(uint32_t /*keyCode*/, const string& /*modifiers*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GenerateKey(const string& /*keys*/, const string& /*client*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult EnableInactivityReporting(const bool /*enable*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SetInactivityInterval(const uint32_t /*interval*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult ResetInactivityTime() override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult EnableKeyRepeats(bool /*enable*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetKeyRepeatsEnabled(bool& /*keyRepeat*/) const override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult IgnoreKeyInputs(bool /*ignore*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult EnableInputEvents(const string& /*clients*/, bool /*enable*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult KeyRepeatConfig(const string& /*input*/, const string& /*keyConfig*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SetFocus(const string& /*client*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SetVisible(const string& /*client*/, bool /*visible*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetVisibility(const string& /*client*/, bool& /*visible*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult RenderReady(const string& /*client*/, bool& /*status*/) const override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult EnableDisplayRender(const string& /*client*/, bool /*enable*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetLastKeyInfo(uint32_t& /*keyCode*/, uint32_t& /*modifiers*/, uint64_t& /*timestampInSeconds*/) const override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SetZOrder(const string& /*appInstanceId*/, const int32_t /*zOrder*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetZOrder(const string& /*appInstanceId*/, int32_t& /*zOrder*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult StartVncServer() override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult StopVncServer() override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetScreenshot() override { return WPEFramework::Core::ERROR_NONE; }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> registerCalls;
    std::atomic<uint32_t> unregisterCalls;
};

/**
 * @brief Minimal IShell stub for LifecycleManager L0 tests.
 *
 * The LifecycleManager plugin calls service->Root<ILifecycleManager>() which
 * resolves to ICOMLink::Instantiate() under the hood.  This mock provides
 * configurable Instantiate() and RemoteConnection() behaviour.
 */
class LcmServiceMock final : public WPEFramework::PluginHost::IShell {
public:
    using InstantiateHandler = std::function<void*(const WPEFramework::RPC::Object&, const uint32_t, uint32_t&)>;

    struct Config {
        explicit Config(
            WPEFramework::Exchange::IRuntimeManager*   rtm = nullptr,
            WPEFramework::Exchange::IRDKWindowManager* wm  = nullptr)
            : rtm(rtm)
            , wm(wm)
        {
        }
        WPEFramework::Exchange::IRuntimeManager*   rtm;
        WPEFramework::Exchange::IRDKWindowManager* wm;
    };

    class COMLinkMock final : public WPEFramework::PluginHost::IShell::ICOMLink {
    public:
        explicit COMLinkMock(LcmServiceMock& parent)
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
            return _parent._fakeConnection;
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
        LcmServiceMock& _parent;
    };

    explicit LcmServiceMock(Config cfg = Config())
        : _refCount(1)
        , _cfg(cfg)
        , _comLink(*this)
        , _fakeConnection(nullptr)
    {
    }

    ~LcmServiceMock() override = default;

    LcmServiceMock(const LcmServiceMock&) = delete;
    LcmServiceMock& operator=(const LcmServiceMock&) = delete;

    void SetInstantiateHandler(InstantiateHandler handler)
    {
        _instantiateHandler = handler;
    }

    void SetFakeConnection(WPEFramework::RPC::IRemoteConnection* conn)
    {
        _fakeConnection = conn;
    }

    // Observability counters for L0 assertions.
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
        return (0U == newCount) ? WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED : WPEFramework::Core::ERROR_NONE;
    }

    std::string Versions() const override { return ""; }
    std::string Locator() const override { return ""; }
    std::string ClassName() const override { return ""; }
    std::string Callsign() const override { return "org.rdk.LifecycleManager"; }
    std::string WebPrefix() const override { return ""; }
    std::string ConfigLine() const override { return "{}"; }
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
        if (WPEFramework::Exchange::IRuntimeManager::ID == id
            && "org.rdk.RuntimeManager" == callsign
            && nullptr != _cfg.rtm)
        {
            _cfg.rtm->AddRef();
            return static_cast<WPEFramework::Exchange::IRuntimeManager*>(_cfg.rtm);
        }
        if (WPEFramework::Exchange::IRDKWindowManager::ID == id
            && "org.rdk.RDKWindowManager" == callsign
            && nullptr != _cfg.wm)
        {
            _cfg.wm->AddRef();
            return static_cast<WPEFramework::Exchange::IRDKWindowManager*>(_cfg.wm);
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

    WPEFramework::PluginHost::IShell::ICOMLink* COMLink() override
    {
        return &_comLink;
    }

    reason Reason() const override { return reason::REQUESTED; }
    std::string SystemRootPath() const override { return ""; }

    WPEFramework::RPC::IRemoteConnection* RemoteConnection(const uint32_t) const
    {
        return _fakeConnection;
    }

private:
    mutable std::atomic<uint32_t> _refCount;
    InstantiateHandler _instantiateHandler;
    Config _cfg;
    COMLinkMock _comLink;
    WPEFramework::RPC::IRemoteConnection* _fakeConnection;
};

// ──────────────────────────────────────────────────────────────────────────────
// Fake ILifecycleManager::INotification stub for notification callback tests
// ──────────────────────────────────────────────────────────────────────────────

class FakeLcmNotification final : public WPEFramework::Exchange::ILifecycleManager::INotification {
public:
    FakeLcmNotification()
        : _refCount(1)
        , onAppStateChangedCount(0)
        , lastAppId("")
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
        if (id == WPEFramework::Exchange::ILifecycleManager::INotification::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::ILifecycleManager::INotification*>(this);
        }
        return nullptr;
    }

    void OnAppStateChanged(
        const std::string& appId,
        const WPEFramework::Exchange::ILifecycleManager::LifecycleState newState,
        const std::string& errorReason) override
    {
        onAppStateChangedCount++;
        lastAppId = appId;
        lastState = newState;
        lastError = errorReason;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> onAppStateChangedCount;
    std::string lastAppId;
    WPEFramework::Exchange::ILifecycleManager::LifecycleState lastState {};
    std::string lastError;
};

// ──────────────────────────────────────────────────────────────────────────────
// Fake ILifecycleManagerState::INotification stub
// ──────────────────────────────────────────────────────────────────────────────

class FakeLcmStateNotification final : public WPEFramework::Exchange::ILifecycleManagerState::INotification {
public:
    FakeLcmStateNotification()
        : _refCount(1)
        , onStateChangedCount(0)
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
        if (id == WPEFramework::Exchange::ILifecycleManagerState::INotification::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::ILifecycleManagerState::INotification*>(this);
        }
        return nullptr;
    }

    void OnAppLifecycleStateChanged(
        const std::string& appId,
        const std::string& appInstanceId,
        const WPEFramework::Exchange::ILifecycleManager::LifecycleState oldState,
        const WPEFramework::Exchange::ILifecycleManager::LifecycleState newState,
        const std::string& navigationIntent) override
    {
        onStateChangedCount++;
        lastAppId = appId;
        lastAppInstanceId = appInstanceId;
        lastOldState = oldState;
        lastNewState = newState;
        lastNavigationIntent = navigationIntent;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> onStateChangedCount;
    std::string lastAppId;
    std::string lastAppInstanceId;
    WPEFramework::Exchange::ILifecycleManager::LifecycleState lastOldState {};
    WPEFramework::Exchange::ILifecycleManager::LifecycleState lastNewState {};
    std::string lastNavigationIntent;
};

// ──────────────────────────────────────────────────────────────────────────────
// Fake ILifecycleManager implementation with ILifecycleManagerState + IConfiguration
// Used by shell tests where service->Root<ILifecycleManager>() must succeed.
// ──────────────────────────────────────────────────────────────────────────────

class FakeLifecycleManagerImpl final
    : public WPEFramework::Exchange::ILifecycleManager
    , public WPEFramework::Exchange::ILifecycleManagerState
    , public WPEFramework::Exchange::IConfiguration {
public:
    FakeLifecycleManagerImpl()
        : _refCount(1)
        , registerStateCalls(0)
        , unregisterStateCalls(0)
        , configureCalls(0)
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
        if (id == WPEFramework::Exchange::ILifecycleManager::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::ILifecycleManager*>(this);
        }
        if (id == WPEFramework::Exchange::ILifecycleManagerState::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::ILifecycleManagerState*>(this);
        }
        if (id == WPEFramework::Exchange::IConfiguration::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IConfiguration*>(this);
        }
        return nullptr;
    }

    // IConfiguration
    uint32_t Configure(WPEFramework::PluginHost::IShell*) override
    {
        configureCalls++;
        return WPEFramework::Core::ERROR_NONE;
    }

    // ILifecycleManager
    WPEFramework::Core::hresult Register(WPEFramework::Exchange::ILifecycleManager::INotification*) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Unregister(WPEFramework::Exchange::ILifecycleManager::INotification*) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetLoadedApps(const bool, std::string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult IsAppLoaded(const std::string&, bool&) const override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SpawnApp(const std::string&, const std::string&,
        const WPEFramework::Exchange::ILifecycleManager::LifecycleState,
        const WPEFramework::Exchange::RuntimeConfig&,
        const std::string&, std::string&, std::string&, bool&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SetTargetAppState(const std::string&,
        const WPEFramework::Exchange::ILifecycleManager::LifecycleState,
        const std::string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult UnloadApp(const std::string&, std::string&, bool&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult KillApp(const std::string&, std::string&, bool&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SendIntentToActiveApp(const std::string&, const std::string&, std::string&, bool&) override { return WPEFramework::Core::ERROR_NONE; }

    // ILifecycleManagerState
    WPEFramework::Core::hresult Register(WPEFramework::Exchange::ILifecycleManagerState::INotification* n) override
    {
        registerStateCalls++;
        _storedStateNotification = n;
        return WPEFramework::Core::ERROR_NONE;
    }
    WPEFramework::Core::hresult Unregister(WPEFramework::Exchange::ILifecycleManagerState::INotification*) override
    {
        unregisterStateCalls++;
        return WPEFramework::Core::ERROR_NONE;
    }
    WPEFramework::Core::hresult AppReady(const std::string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult StateChangeComplete(const std::string&, const uint32_t, const bool) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult CloseApp(const std::string&,
        const WPEFramework::Exchange::ILifecycleManagerState::AppCloseReason) override { return WPEFramework::Core::ERROR_NONE; }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> registerStateCalls;
    std::atomic<uint32_t> unregisterStateCalls;
    std::atomic<uint32_t> configureCalls;
    WPEFramework::Exchange::ILifecycleManagerState::INotification* _storedStateNotification { nullptr };
};

} // namespace L0Test

// ---------------------------------------------------------------------------
// EnumerateType<T>::Table() explicit specialisations required by the
// LifecycleManager plugin sources (LifecycleManager.cpp includes the
// generated JLifecycleManagerState.h which uses Core::JSON::EnumType<T>).
// entservices-apis installs only headers, so these definitions must be
// provided here.  Marked inline to be safe if ever included by multiple TUs.
// ---------------------------------------------------------------------------
namespace WPEFramework {
namespace Core {

template <>
inline const EnumerateConversion<WPEFramework::Exchange::ILifecycleManager::LifecycleState>*
EnumerateType<WPEFramework::Exchange::ILifecycleManager::LifecycleState>::Table(const uint16_t index)
{
    using LS = WPEFramework::Exchange::ILifecycleManager::LifecycleState;
    static const EnumerateConversion<LS> table[] = {
        { LS::UNLOADED,     "UNLOADED"     },
        { LS::LOADING,      "LOADING"      },
        { LS::INITIALIZING, "INITIALIZING" },
        { LS::PAUSED,       "PAUSED"       },
        { LS::ACTIVE,       "ACTIVE"       },
        { LS::SUSPENDED,    "SUSPENDED"    },
        { LS::HIBERNATED,   "HIBERNATED"   },
        { LS::TERMINATING,  "TERMINATING"  },
    };
    return (index < (sizeof(table) / sizeof(table[0])) ? &table[index] : nullptr);
}

template <>
inline const EnumerateConversion<WPEFramework::Exchange::ILifecycleManagerState::AppCloseReason>*
EnumerateType<WPEFramework::Exchange::ILifecycleManagerState::AppCloseReason>::Table(const uint16_t index)
{
    using ACR = WPEFramework::Exchange::ILifecycleManagerState::AppCloseReason;
    static const EnumerateConversion<ACR> table[] = {
        { ACR::USER_EXIT, "USER_EXIT" },
    };
    return (index < (sizeof(table) / sizeof(table[0])) ? &table[index] : nullptr);
}

} // namespace Core
} // namespace WPEFramework
