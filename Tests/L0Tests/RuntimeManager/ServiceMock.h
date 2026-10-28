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

class ServiceMock final : public Thunder::PluginHost::IShell {
public:
    using InstantiateHandler = std::function<void*(const Thunder::RPC::Object&, const uint32_t, uint32_t&)>;

    struct Config {
        explicit Config(Thunder::Exchange::IOCIContainer*     oci = nullptr,
                        Thunder::Exchange::IRDKWindowManager* wm  = nullptr)
            : oci(oci)
            , wm(wm)
            , configLine("{}")
        {
        }
        Thunder::Exchange::IOCIContainer*     oci;
        Thunder::Exchange::IRDKWindowManager* wm;
        std::string                                configLine;
    };

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

    uint32_t AddRef() const override
    {
        addRefCalls++;
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t Release() const override
    {
        releaseCalls++;
        const uint32_t newCount = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (0U == newCount) ? Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED : Thunder::Core::ERROR_NONE;
    }

    std::string Versions() const override { return ""; }
    std::string Locator() const override { return ""; }
    std::string ClassName() const override { return ""; }
    std::string Callsign() const override { return "org.rdk.RuntimeManager"; }
    std::string WebPrefix() const override { return ""; }
    std::string ConfigLine() const override { return _cfg.configLine; }
    std::string PersistentPath() const override { return "/tmp"; }
    std::string VolatilePath() const override { return "/tmp"; }
    std::string DataPath() const override { return "/tmp"; }
    state State() const override { return state::ACTIVATED; }

    Thunder::Core::hresult Activate(const reason) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Deactivate(const reason) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Unavailable(const reason) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult ConfigLine(const std::string&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult SystemRootPath(const std::string&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Hibernate(const uint32_t) override { return Thunder::Core::ERROR_NONE; }
    std::string SystemPath() const override { return ""; }
    std::string PluginPath() const override { return ""; }
    startup Startup() const override { return static_cast<startup>(0); }
    Thunder::Core::hresult Startup(const startup) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Resumed(const bool) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Metadata(std::string& info) const override { info.clear(); return Thunder::Core::ERROR_NONE; }

    bool Resumed() const override { return false; }
    bool IsSupported(const uint8_t) const override { return false; }
    void EnableWebServer(const std::string&, const std::string&) override {}
    void DisableWebServer() override {}
    Thunder::PluginHost::ISubSystem* SubSystems() override { return nullptr; }
    uint32_t Submit(const uint32_t, const Thunder::Core::ProxyType<Thunder::Core::JSON::IElement>&) override { return Thunder::Core::ERROR_NONE; }
    void Notify(const std::string&) override {}
    void* QueryInterface(const uint32_t) override { return nullptr; }
    void* QueryInterfaceByCallsign(const uint32_t id, const std::string& callsign) override
    {
        if (Thunder::Exchange::IOCIContainer::ID == id
            && "org.rdk.OCIContainer" == callsign
            && nullptr != _cfg.oci)
        {
            _cfg.oci->AddRef();
            return static_cast<Thunder::Exchange::IOCIContainer*>(_cfg.oci);
        }
        if (Thunder::Exchange::IRDKWindowManager::ID == id
            && "org.rdk.RDKWindowManager" == callsign
            && nullptr != _cfg.wm)
        {
            _cfg.wm->AddRef();
            return static_cast<Thunder::Exchange::IRDKWindowManager*>(_cfg.wm);
        }
        return nullptr;
    }
    void Register(Thunder::PluginHost::IPlugin::INotification*) override {}
    void Unregister(Thunder::PluginHost::IPlugin::INotification*) override {}
    std::string Model() const override { return ""; }
    bool Background() const override { return false; }
    std::string Accessor() const override { return ""; }
    std::string ProxyStubPath() const override { return ""; }
    std::string HashKey() const override { return ""; }
    std::string Substitute(const std::string&) const override { return ""; }

    Thunder::PluginHost::IShell::ICOMLink* COMLink() override
    {
        return &_comLink;
    }

    reason Reason() const override { return reason::REQUESTED; }
    std::string SystemRootPath() const override { return ""; }

    Thunder::RPC::IRemoteConnection* RemoteConnection(const uint32_t) const
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
class FakeOCIContainer final : public Thunder::Exchange::IOCIContainer {
public:
    FakeOCIContainer()
        : _refCount(1)
        , registerCalls(0)
        , unregisterCalls(0)
        , _registerReturnCode(Thunder::Core::ERROR_NONE)
        , _storedNotification(nullptr)
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

    void* QueryInterface(const uint32_t /*id*/) override { return nullptr; }

    Thunder::Core::hresult Register(Thunder::Exchange::IOCIContainer::INotification* notification) override
    {
        registerCalls++;
        _storedNotification = notification;
        return _registerReturnCode;
    }

    Thunder::Core::hresult Unregister(Thunder::Exchange::IOCIContainer::INotification* /*notification*/) override
    {
        unregisterCalls++;
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult ListContainers(string& /*containers*/, bool& /*success*/, string& /*errorReason*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetContainerInfo(const string& /*id*/, string& /*info*/, bool& /*success*/, string& /*err*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetContainerState(const string& /*id*/, ContainerState& /*state*/, bool& /*success*/, string& /*err*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult StartContainer(const string& /*id*/, const string& /*bundle*/, const string& /*cmd*/, const string& /*ws*/, int32_t& /*desc*/, bool& /*success*/, string& /*err*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult StartContainerFromDobbySpec(const string& /*id*/, const string& /*spec*/, const string& /*cmd*/, const string& /*ws*/, int32_t& /*desc*/, bool& /*success*/, string& /*err*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult StopContainer(const string& /*id*/, bool /*force*/, bool& /*success*/, string& /*err*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult PauseContainer(const string& /*id*/, bool& /*success*/, string& /*err*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult ResumeContainer(const string& /*id*/, bool& /*success*/, string& /*err*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult HibernateContainer(const string& /*id*/, const string& /*opts*/, bool& /*success*/, string& /*err*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult WakeupContainer(const string& /*id*/, bool& /*success*/, string& /*err*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult ExecuteCommand(const string& /*id*/, const string& /*opts*/, const string& /*cmd*/, bool& /*success*/, string& /*err*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Annotate(const string& /*id*/, const string& /*key*/, const string& /*val*/, bool& /*success*/, string& /*err*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult RemoveAnnotation(const string& /*id*/, const string& /*key*/, bool& /*success*/, string& /*err*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Mount(const string& /*id*/, const string& /*src*/, const string& /*tgt*/, const string& /*type*/, const string& /*opts*/, bool& /*success*/, string& /*err*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Unmount(const string& /*id*/, const string& /*tgt*/, bool& /*success*/, string& /*err*/) override { return Thunder::Core::ERROR_NONE; }

    void SetRegisterReturnCode(Thunder::Core::hresult code) { _registerReturnCode = code; }

    Thunder::Exchange::IOCIContainer::INotification* GetStoredNotification() const
    {
        return _storedNotification;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> registerCalls;
    std::atomic<uint32_t> unregisterCalls;

private:
    Thunder::Core::hresult _registerReturnCode;
    Thunder::Exchange::IOCIContainer::INotification* _storedNotification;
};

} // end namespace L0Test (FakeEventHandler defined in Thunder::Plugin below)

/* Minimal IEventHandler stub counting calls per event type.
 * Defined inside Thunder::Plugin so bare JsonObject resolves naturally. */
namespace Thunder {
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
} // namespace Thunder

namespace L0Test {
    using FakeEventHandler = Thunder::Plugin::FakeEventHandler;

/* Minimal IRDKWindowManager stub used by WindowManagerConnector tests.
 * Implements all pure-virtual methods declared in IRDKWindowManager.h.
 */
class FakeWindowManager final : public Thunder::Exchange::IRDKWindowManager {
public:
    FakeWindowManager()
        : _refCount(1)
        , registerCalls(0)
        , unregisterCalls(0)
        , releaseCalls(0)
        , createDisplayCalls(0)
        , _createDisplayReturnCode(Thunder::Core::ERROR_NONE)
        , _storedNotification(nullptr)
    {
    }

    uint32_t AddRef() const override
    {
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t Release() const override
    {
        releaseCalls++;
        const uint32_t r = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (r == 0u) ? Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED
                         : Thunder::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t /*id*/) override { return nullptr; }

    Thunder::Core::hresult Register(Thunder::Exchange::IRDKWindowManager::INotification* notification) override
    {
        registerCalls++;
        _storedNotification = notification;
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult Unregister(Thunder::Exchange::IRDKWindowManager::INotification* /*notification*/) override
    {
        unregisterCalls++;
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult Initialize(Thunder::PluginHost::IShell* /*service*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult Deinitialize(Thunder::PluginHost::IShell* /*service*/) override { return Thunder::Core::ERROR_NONE; }

    Thunder::Core::hresult CreateDisplay(const string& /*appInstanceId*/,
                                              const string& /*displayName*/,
                                              const uint32_t /*displayWidth*/,
                                              const uint32_t /*displayHeight*/,
                                              const bool /*virtualDisplay*/,
                                              const uint32_t /*virtualWidth*/,
                                              const uint32_t /*virtualHeight*/,
                                              const uint32_t /*ownerId*/,
                                              const uint32_t /*groupId*/,
                                              const bool /*topmost*/,
                                              const bool /*focus*/,
                                              const string& /*capabilities*/) override
    {
        createDisplayCalls++;
        return _createDisplayReturnCode;
    }

    Thunder::Core::hresult GetApps(Thunder::Exchange::IRDKWindowManager::IStringIterator*& appsIds) const override { appsIds = nullptr; return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult AddKeyIntercept(const string& /*intercept*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult AddKeyIntercepts(const string& /*clientId*/, const string& /*intercepts*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult RemoveKeyIntercept(const string& /*clientId*/, const uint32_t /*keyCode*/, const string& /*modifiers*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult AddKeyListener(const string& /*keyListeners*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult RemoveKeyListener(const string& /*keyListeners*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult InjectKey(uint32_t /*keyCode*/, const string& /*modifiers*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GenerateKey(const string& /*keys*/, const string& /*client*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult EnableInactivityReporting(const bool /*enable*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult SetInactivityInterval(const uint32_t /*interval*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult ResetInactivityTime() override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult EnableKeyRepeats(bool /*enable*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetKeyRepeatsEnabled(bool& /*keyRepeat*/) const override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult IgnoreKeyInputs(bool /*ignore*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult EnableInputEvents(const string& /*clients*/, bool /*enable*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult KeyRepeatConfig(const string& /*input*/, const string& /*keyConfig*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult SetFocus(const string& /*client*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult SetVisible(const string& /*client*/, bool /*visible*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult RenderReady(const string& /*client*/, bool& /*status*/) const override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult EnableDisplayRender(const string& /*client*/, bool /*enable*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetLastKeyInfo(uint32_t& /*keyCode*/, uint32_t& /*modifiers*/, uint64_t& /*timestampInSeconds*/) const override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult SetZOrder(const string& /*appInstanceId*/, const int32_t /*zOrder*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetZOrder(const string& /*appInstanceId*/, int32_t& /*zOrder*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetVisibility(const std::string& /*client*/, bool& /*visible*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetScreenshot() override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult StartVncServer() override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult StopVncServer() override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetFocused(string& /*client*/) const override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult SetAlias(const string& /*clientId*/, const string& /*alias*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult ShowSplashScreen(const bool /*show*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult SetBounds(const string& /*clientId*/, const uint32_t /*x*/, const uint32_t /*y*/, const uint32_t /*width*/, const uint32_t /*height*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetBounds(const string& /*clientId*/, uint32_t& /*x*/, uint32_t& /*y*/, uint32_t& /*width*/, uint32_t& /*height*/) const override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult SetScale(const string& /*clientId*/, const double /*scaleX*/, const double /*scaleY*/) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetScale(const string& /*clientId*/, double& /*scaleX*/, double& /*scaleY*/) const override { return Thunder::Core::ERROR_NONE; }

    void SetCreateDisplayReturnCode(Thunder::Core::hresult code) { _createDisplayReturnCode = code; }

    Thunder::Exchange::IRDKWindowManager::INotification* GetStoredNotification() const
    {
        return _storedNotification;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> registerCalls;
    std::atomic<uint32_t> unregisterCalls;
    mutable std::atomic<uint32_t> releaseCalls;
    std::atomic<uint32_t> createDisplayCalls;

private:
    Thunder::Core::hresult _createDisplayReturnCode;
    Thunder::Exchange::IRDKWindowManager::INotification* _storedNotification;
};

} // namespace L0Test
