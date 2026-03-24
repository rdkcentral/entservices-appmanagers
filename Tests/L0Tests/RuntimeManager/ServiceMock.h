#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

#include <core/core.h>
#include <plugins/plugins.h>
#include <plugins/IShell.h>
#include <interfaces/IOCIContainer.h>
#include <interfaces/IRDKWindowManager.h>
#include "IEventHandler.h"

namespace L0Test {

using string = std::string;

class ServiceMock final : public WPEFramework::PluginHost::IShell {
public:
    using InstantiateHandler = std::function<void*(const WPEFramework::RPC::Object&, const uint32_t, uint32_t&)>;

    struct Config {
        explicit Config(WPEFramework::Exchange::IOCIContainer*     oci = nullptr,
                        WPEFramework::Exchange::IRDKWindowManager* wm  = nullptr)
            : oci(oci)
            , wm(wm)
        {
        }
        WPEFramework::Exchange::IOCIContainer*     oci;
        WPEFramework::Exchange::IRDKWindowManager* wm;
    };

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
        _instantiateHandler = handler;
    }

    // Observability hooks for L0 assertions.
    mutable std::atomic<uint32_t> addRefCalls { 0 };
    mutable std::atomic<uint32_t> releaseCalls { 0 };
    mutable std::atomic<uint32_t> remoteConnectionCalls { 0 };
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
    std::string Callsign() const override { return "org.rdk.RuntimeManager"; }
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
        if (WPEFramework::Exchange::IOCIContainer::ID == id
            && "org.rdk.OCIContainer" == callsign
            && nullptr != _cfg.oci)
        {
            _cfg.oci->AddRef();
            return static_cast<WPEFramework::Exchange::IOCIContainer*>(_cfg.oci);
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
        remoteConnectionCalls++;
        return nullptr;
    }

private:
    mutable std::atomic<uint32_t> _refCount;
    InstantiateHandler _instantiateHandler;
    Config _cfg;
    COMLinkMock _comLink;
};

// ──────────────────────────────────────────────────────────────────────────────
// Plugin-specific fake stubs (following AppGateway ServiceMock.h conventions)
// ──────────────────────────────────────────────────────────────────────────────

/* Minimal IOCIContainer stub used by DobbyEventListener tests. */
class FakeOCIContainer final : public WPEFramework::Exchange::IOCIContainer {
public:
    FakeOCIContainer()
        : _refCount(1)
        , registerCalls(0)
        , unregisterCalls(0)
        , _registerReturnCode(WPEFramework::Core::ERROR_NONE)
        , _storedNotification(nullptr)
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

    WPEFramework::Core::hresult Register(WPEFramework::Exchange::IOCIContainer::INotification* notification) override
    {
        registerCalls++;
        _storedNotification = notification;
        return _registerReturnCode;
    }

    WPEFramework::Core::hresult Unregister(WPEFramework::Exchange::IOCIContainer::INotification* /*notification*/) override
    {
        unregisterCalls++;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult ListContainers(string& /*containers*/, bool& /*success*/, string& /*errorReason*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetContainerInfo(const string& /*id*/, string& /*info*/, bool& /*success*/, string& /*err*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetContainerState(const string& /*id*/, ContainerState& /*state*/, bool& /*success*/, string& /*err*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult StartContainer(const string& /*id*/, const string& /*bundle*/, const string& /*cmd*/, const string& /*ws*/, int32_t& /*desc*/, bool& /*success*/, string& /*err*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult StartContainerFromDobbySpec(const string& /*id*/, const string& /*spec*/, const string& /*cmd*/, const string& /*ws*/, int32_t& /*desc*/, bool& /*success*/, string& /*err*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult StopContainer(const string& /*id*/, bool /*force*/, bool& /*success*/, string& /*err*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult PauseContainer(const string& /*id*/, bool& /*success*/, string& /*err*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult ResumeContainer(const string& /*id*/, bool& /*success*/, string& /*err*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult HibernateContainer(const string& /*id*/, const string& /*opts*/, bool& /*success*/, string& /*err*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult WakeupContainer(const string& /*id*/, bool& /*success*/, string& /*err*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult ExecuteCommand(const string& /*id*/, const string& /*opts*/, const string& /*cmd*/, bool& /*success*/, string& /*err*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Annotate(const string& /*id*/, const string& /*key*/, const string& /*val*/, bool& /*success*/, string& /*err*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult RemoveAnnotation(const string& /*id*/, const string& /*key*/, bool& /*success*/, string& /*err*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Mount(const string& /*id*/, const string& /*src*/, const string& /*tgt*/, const string& /*type*/, const string& /*opts*/, bool& /*success*/, string& /*err*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Unmount(const string& /*id*/, const string& /*tgt*/, bool& /*success*/, string& /*err*/) override { return WPEFramework::Core::ERROR_NONE; }

    void SetRegisterReturnCode(WPEFramework::Core::hresult code) { _registerReturnCode = code; }

    WPEFramework::Exchange::IOCIContainer::INotification* GetStoredNotification() const
    {
        return _storedNotification;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> registerCalls;
    std::atomic<uint32_t> unregisterCalls;

private:
    WPEFramework::Core::hresult _registerReturnCode;
    WPEFramework::Exchange::IOCIContainer::INotification* _storedNotification;
};

} // end namespace L0Test (FakeEventHandler defined in WPEFramework::Plugin below)

/* Minimal IEventHandler stub counting calls per event type.
 * Defined inside WPEFramework::Plugin so bare JsonObject resolves naturally. */
namespace WPEFramework {
    namespace Plugin {
        class FakeEventHandler final : public IEventHandler {
        public:
            FakeEventHandler()
                : startedCalls(0)
                , stoppedCalls(0)
                , failureCalls(0)
                , stateChangedCalls(0)
            {
            }

            void onOCIContainerStartedEvent(std::string /*name*/, JsonObject& /*data*/) override
            {
                startedCalls++;
            }

            void onOCIContainerStoppedEvent(std::string /*name*/, JsonObject& /*data*/) override
            {
                stoppedCalls++;
            }

            void onOCIContainerFailureEvent(std::string /*name*/, JsonObject& /*data*/) override
            {
                failureCalls++;
            }

            void onOCIContainerStateChangedEvent(std::string /*name*/, JsonObject& /*data*/) override
            {
                stateChangedCalls++;
            }

            std::atomic<uint32_t> startedCalls;
            std::atomic<uint32_t> stoppedCalls;
            std::atomic<uint32_t> failureCalls;
            std::atomic<uint32_t> stateChangedCalls;
        };
    } // namespace Plugin
} // namespace WPEFramework

namespace L0Test {
    using FakeEventHandler = WPEFramework::Plugin::FakeEventHandler;

/* Minimal IRDKWindowManager stub used by WindowManagerConnector tests.
 * Implements all pure-virtual methods declared in IRDKWindowManager.h.
 */
class FakeWindowManager final : public WPEFramework::Exchange::IRDKWindowManager {
public:
    FakeWindowManager()
        : _refCount(1)
        , registerCalls(0)
        , unregisterCalls(0)
        , releaseCalls(0)
        , createDisplayCalls(0)
        , _createDisplayReturnCode(WPEFramework::Core::ERROR_NONE)
        , _storedNotification(nullptr)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        releaseCalls++;
        const uint32_t r = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (r == 0u) ? WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED
                         : WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t /*id*/) override { return nullptr; }

    WPEFramework::Core::hresult Register(WPEFramework::Exchange::IRDKWindowManager::INotification* notification) override
    {
        registerCalls++;
        _storedNotification = notification;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unregister(WPEFramework::Exchange::IRDKWindowManager::INotification* /*notification*/) override
    {
        unregisterCalls++;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Initialize(WPEFramework::PluginHost::IShell* /*service*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Deinitialize(WPEFramework::PluginHost::IShell* /*service*/) override { return WPEFramework::Core::ERROR_NONE; }

    WPEFramework::Core::hresult CreateDisplay(const string& /*params*/) override
    {
        createDisplayCalls++;
        return _createDisplayReturnCode;
    }

    WPEFramework::Core::hresult GetApps(string& /*appsIds*/) const override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult AddKeyIntercept(const string& /*intercept*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult AddKeyIntercepts(const string& /*clientId*/, const string& /*intercepts*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult RemoveKeyIntercept(const string& /*clientId*/, const uint32_t /*keyCode*/, const string& /*modifiers*/) override { return WPEFramework::Core::ERROR_NONE; }
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
    WPEFramework::Core::hresult RenderReady(const string& /*client*/, bool& /*status*/) const override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult EnableDisplayRender(const string& /*client*/, bool /*enable*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetLastKeyInfo(uint32_t& /*keyCode*/, uint32_t& /*modifiers*/, uint64_t& /*timestampInSeconds*/) const override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SetZOrder(const string& /*appInstanceId*/, const int32_t /*zOrder*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetZOrder(const string& /*appInstanceId*/, int32_t& /*zOrder*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetVisibility(const std::string& /*client*/, bool& /*visible*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetScreenshot() override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult StartVncServer() override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult StopVncServer() override { return WPEFramework::Core::ERROR_NONE; }

    void SetCreateDisplayReturnCode(WPEFramework::Core::hresult code) { _createDisplayReturnCode = code; }

    WPEFramework::Exchange::IRDKWindowManager::INotification* GetStoredNotification() const
    {
        return _storedNotification;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> registerCalls;
    std::atomic<uint32_t> unregisterCalls;
    mutable std::atomic<uint32_t> releaseCalls;
    std::atomic<uint32_t> createDisplayCalls;

private:
    WPEFramework::Core::hresult _createDisplayReturnCode;
    WPEFramework::Exchange::IRDKWindowManager::INotification* _storedNotification;
};

} // namespace L0Test
