#pragma once

#include <atomic>
#include <functional>
#include <string>

#include <core/core.h>
#include <plugins/IShell.h>
#include <plugins/ISubSystem.h>
#include <interfaces/IAppStorageManager.h>
#include <interfaces/ITelemetryMetrics.h>

namespace L0Test {

using string = std::string;

class FakeStorageManager final : public WPEFramework::Exchange::IAppStorageManager {
public:
    FakeStorageManager()
        : _refCount(1)
        , createStorageCalls(0)
        , deleteStorageCalls(0)
        , createStorageResult(WPEFramework::Core::ERROR_NONE)
        , deleteStorageResult(WPEFramework::Core::ERROR_NONE)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (0U == remaining) ? WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED : WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IAppStorageManager::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IAppStorageManager*>(this);
        }
        return nullptr;
    }

    WPEFramework::Core::hresult CreateStorage(const std::string& appId, const uint32_t& size, std::string& path, std::string& errorReason) override
    {
        (void) appId;
        (void) size;
        createStorageCalls++;
        if (createStorageResult == WPEFramework::Core::ERROR_NONE) {
            path = "/tmp";
            errorReason.clear();
        }
        return createStorageResult;
    }

    WPEFramework::Core::hresult GetStorage(const std::string&, const int32_t&, const int32_t&, std::string& path, uint32_t& size, uint32_t& used) override
    {
        path = "/tmp";
        size = 1024;
        used = 0;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult DeleteStorage(const std::string&, std::string& errorReason) override
    {
        deleteStorageCalls++;
        if (deleteStorageResult == WPEFramework::Core::ERROR_NONE) {
            errorReason.clear();
        }
        return deleteStorageResult;
    }

    WPEFramework::Core::hresult Clear(const std::string&, std::string& errorReason) override
    {
        errorReason.clear();
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult ClearAll(const std::string&, std::string& errorReason) override
    {
        errorReason.clear();
        return WPEFramework::Core::ERROR_NONE;
    }

    mutable std::atomic<uint32_t> createStorageCalls;
    mutable std::atomic<uint32_t> deleteStorageCalls;
    WPEFramework::Core::hresult createStorageResult;
    WPEFramework::Core::hresult deleteStorageResult;

private:
    mutable std::atomic<uint32_t> _refCount;
};

class ServiceMock final : public WPEFramework::PluginHost::IShell {
public:
    using InstantiateHandler = std::function<void*(const WPEFramework::RPC::Object&, const uint32_t, uint32_t&)>;

    class FakeSubSystem final : public WPEFramework::PluginHost::ISubSystem {
    public:
        FakeSubSystem()
            : _refCount(1)
            , internetActive(true)
        {
        }

        void AddRef() const override
        {
            _refCount.fetch_add(1, std::memory_order_relaxed);
        }

        uint32_t Release() const override
        {
            const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
            return (0U == remaining) ? WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED : WPEFramework::Core::ERROR_NONE;
        }

        void* QueryInterface(const uint32_t id) override
        {
            if (id == WPEFramework::PluginHost::ISubSystem::ID) {
                AddRef();
                return static_cast<WPEFramework::PluginHost::ISubSystem*>(this);
            }
            return nullptr;
        }

        void Register(WPEFramework::PluginHost::ISubSystem::INotification*) override {}
        void Unregister(WPEFramework::PluginHost::ISubSystem::INotification*) override {}

        void Set(const subsystem type, WPEFramework::Core::IUnknown*) override
        {
            if (type == INTERNET) {
                internetActive = true;
            } else if (type == NOT_INTERNET) {
                internetActive = false;
            }
        }

        const WPEFramework::Core::IUnknown* Get(const subsystem) const override
        {
            return nullptr;
        }

        bool IsActive(const subsystem type) const override
        {
            if (type == INTERNET) {
                return internetActive;
            }
            return false;
        }

        string BuildTreeHash() const override { return ""; }
        string Version() const override { return ""; }

        mutable std::atomic<uint32_t> _refCount;
        bool internetActive;
    };

    class FakeTelemetryMetrics final : public WPEFramework::Exchange::ITelemetryMetrics {
    public:
        FakeTelemetryMetrics()
            : _refCount(1)
            , recordCalls(0)
            , publishCalls(0)
        {
        }

        void AddRef() const override
        {
            _refCount.fetch_add(1, std::memory_order_relaxed);
        }

        uint32_t Release() const override
        {
            const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
            return (0U == remaining) ? WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED : WPEFramework::Core::ERROR_NONE;
        }

        void* QueryInterface(const uint32_t id) override
        {
            if (id == WPEFramework::Exchange::ITelemetryMetrics::ID) {
                AddRef();
                return static_cast<WPEFramework::Exchange::ITelemetryMetrics*>(this);
            }
            return nullptr;
        }

        WPEFramework::Core::hresult Record(const string& id, const string& metrics, const string& name) override
        {
            (void) id;
            (void) metrics;
            (void) name;
            recordCalls++;
            return WPEFramework::Core::ERROR_NONE;
        }

        WPEFramework::Core::hresult Publish(const string& id, const string& name) override
        {
            (void) id;
            (void) name;
            publishCalls++;
            return WPEFramework::Core::ERROR_NONE;
        }

        mutable std::atomic<uint32_t> _refCount;
        std::atomic<uint32_t> recordCalls;
        std::atomic<uint32_t> publishCalls;
    };

    class COMLinkMock final : public WPEFramework::PluginHost::IShell::ICOMLink {
    public:
        explicit COMLinkMock(ServiceMock& parent)
            : _parent(parent)
        {
        }

        void Register(WPEFramework::RPC::IRemoteConnection::INotification*) override {}
        void Unregister(const WPEFramework::RPC::IRemoteConnection::INotification*) override {}
        void Register(INotification*) override {}
        void Unregister(INotification*) override {}

        WPEFramework::RPC::IRemoteConnection* RemoteConnection(const uint32_t) override
        {
            return nullptr;
        }

        void* Instantiate(const WPEFramework::RPC::Object& object, const uint32_t waitTime, uint32_t& connectionId) override
        {
            instantiateCalls++;
            if (_parent._instantiateHandler) {
                return _parent._instantiateHandler(object, waitTime, connectionId);
            }
            connectionId = 0;
            return nullptr;
        }

        std::atomic<uint32_t> instantiateCalls { 0 };

    private:
        ServiceMock& _parent;
    };

    struct Config {
        explicit Config(WPEFramework::Exchange::IAppStorageManager* storage = nullptr,
                        WPEFramework::PluginHost::ISubSystem* subSystem = nullptr,
                        WPEFramework::Exchange::ITelemetryMetrics* telemetry = nullptr)
            : storage(storage)
            , subSystem(subSystem)
            , telemetry(telemetry)
            , configLine("{\"downloadDir\":\"/tmp/\"}")
        {
        }

        WPEFramework::Exchange::IAppStorageManager* storage;
        WPEFramework::PluginHost::ISubSystem* subSystem;
        WPEFramework::Exchange::ITelemetryMetrics* telemetry;
        std::string configLine;
    };

    explicit ServiceMock(Config cfg = Config())
        : _refCount(1)
        , _cfg(cfg)
        , _comLink(*this)
    {
    }

    ~ServiceMock() override = default;

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
    std::string Callsign() const override { return "org.rdk.PackageManager"; }
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
    WPEFramework::Core::hresult Metadata(std::string& info) const override
    {
        info.clear();
        return WPEFramework::Core::ERROR_NONE;
    }

    bool Resumed() const override { return false; }
    bool IsSupported(const uint8_t) const override { return false; }
    void EnableWebServer(const std::string&, const std::string&) override {}
    void DisableWebServer() override {}
    uint32_t Submit(const uint32_t, const WPEFramework::Core::ProxyType<WPEFramework::Core::JSON::IElement>&) override { return WPEFramework::Core::ERROR_NONE; }
    void Notify(const std::string&) override {}
    void* QueryInterface(const uint32_t) override { return nullptr; }

    void* QueryInterfaceByCallsign(const uint32_t id, const std::string& callsign) override
    {
        if (id == WPEFramework::Exchange::IAppStorageManager::ID &&
            callsign == "org.rdk.AppStorageManager" &&
            _cfg.storage != nullptr) {
            _cfg.storage->AddRef();
            return static_cast<WPEFramework::Exchange::IAppStorageManager*>(_cfg.storage);
        }
        if (id == WPEFramework::Exchange::ITelemetryMetrics::ID &&
            callsign == "org.rdk.TelemetryMetrics" &&
            _cfg.telemetry != nullptr) {
            _cfg.telemetry->AddRef();
            return static_cast<WPEFramework::Exchange::ITelemetryMetrics*>(_cfg.telemetry);
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

    WPEFramework::PluginHost::ISubSystem* SubSystems() override
    {
        return _cfg.subSystem;
    }

    void SetInstantiateHandler(InstantiateHandler handler)
    {
        _instantiateHandler = handler;
    }

    mutable std::atomic<uint32_t> addRefCalls { 0 };
    mutable std::atomic<uint32_t> releaseCalls { 0 };

private:
    mutable std::atomic<uint32_t> _refCount;
    InstantiateHandler _instantiateHandler;
    Config _cfg;
    COMLinkMock _comLink;
};

} // namespace L0Test
