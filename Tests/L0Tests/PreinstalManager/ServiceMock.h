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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <dirent.h>
#include <fcntl.h>
#include <functional>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include <core/core.h>
#include <plugins/IShell.h>
#include <interfaces/IPreinstallManager.h>
#include <interfaces/IAppPackageManager.h>
#include <interfaces/IConfiguration.h>

namespace L0Test {

// ============================================================
// NotificationFake
// implements Exchange::IPreinstallManager::INotification
// Used to verify events are delivered to subscribers.
// ============================================================
class NotificationFake final : public WPEFramework::Exchange::IPreinstallManager::INotification {
public:
    NotificationFake()
        : _refCount(1)
        , callCount(0)
    {
    }

    ~NotificationFake() override = default;

    NotificationFake(const NotificationFake&) = delete;
    NotificationFake& operator=(const NotificationFake&) = delete;

    // IUnknown
    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t n = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == n) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IPreinstallManager::INotification::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IPreinstallManager::INotification*>(this);
        }
        return nullptr;
    }

    // INotification
    void OnAppInstallationStatus(const std::string& jsonresponse) override
    {
        {
            std::lock_guard<std::mutex> lock(_mtx);
            lastPayload = jsonresponse;
            callCount.fetch_add(1, std::memory_order_relaxed);
        }
        _cv.notify_all();
    }

    // Sync helper: blocks until callCount reaches expectedCount or timeout.
    bool WaitForCall(const uint32_t expectedCount, const int timeoutMs = 2000)
    {
        std::unique_lock<std::mutex> lock(_mtx);
        return _cv.wait_for(lock,
            std::chrono::milliseconds(timeoutMs),
            [&] { return callCount.load(std::memory_order_relaxed) >= expectedCount; });
    }

    // Observability
    std::atomic<uint32_t> callCount;
    std::string lastPayload;

private:
    mutable std::atomic<uint32_t> _refCount;
    std::mutex _mtx;
    std::condition_variable _cv;
};

// ============================================================
// PackageIteratorFake
// implements Exchange::IPackageInstaller::IPackageIterator
// Provides a controllable list of installed packages.
// ============================================================
class PackageIteratorFake final : public WPEFramework::Exchange::IPackageInstaller::IPackageIterator {
public:
    using Package      = WPEFramework::Exchange::IPackageInstaller::Package;
    using InstallState = WPEFramework::Exchange::IPackageInstaller::InstallState;

    PackageIteratorFake()
        : _refCount(1)
        , _index(0)
    {
    }

    ~PackageIteratorFake() override = default;

    void AddEntry(const std::string& packageId,
                  const std::string& version,
                  const InstallState state = InstallState::INSTALLED)
    {
        Package p;
        p.packageId = packageId;
        p.version   = version;
        p.state     = state;
        _entries.push_back(p);
    }

    // IUnknown
    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t n = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == n) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IPackageInstaller::IPackageIterator::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IPackageInstaller::IPackageIterator*>(this);
        }
        return nullptr;
    }

    // IPackageIterator - methods used by production code
    bool Next(Package& entry) override
    {
        if (_index < _entries.size()) {
            entry = _entries[_index++];
            return true;
        }
        return false;
    }

    // Additional iterator methods (implement stubs for full interface compliance)
    bool IsValid() const override
    {
        return (_index > 0) && (_index <= _entries.size());
    }

    uint32_t Count() const override
    {
        return static_cast<uint32_t>(_entries.size());
    }

    void Reset(const uint32_t position = 0) override
    {
        _index = static_cast<size_t>(position);
    }

    Package Current() const override
    {
        if (IsValid()) {
            return _entries[_index - 1];
        }
        return Package{};
    }

    bool Previous(Package& entry) override
    {
        if (_index == 0) {
            return false;
        }
        entry = _entries[--_index];
        return true;
    }

private:
    mutable std::atomic<uint32_t> _refCount;
    std::vector<Package> _entries;
    size_t _index;
};

// ============================================================
// PackageInstallerFake
// implements Exchange::IPackageInstaller
// Provides configurable behavior for all tested code paths.
// ============================================================
class PackageInstallerFake final : public WPEFramework::Exchange::IPackageInstaller {
public:
    using Package            = WPEFramework::Exchange::IPackageInstaller::Package;
    using FailReason         = WPEFramework::Exchange::IPackageInstaller::FailReason;
    using InstallState       = WPEFramework::Exchange::IPackageInstaller::InstallState;
    using IKeyValueIterator  = WPEFramework::Exchange::IPackageInstaller::IKeyValueIterator;

    PackageInstallerFake()
        : _refCount(1)
        , getConfigShouldSucceed(true)
        , returnPackageId("com.test.app")
        , returnVersion("1.0.0")
        , listPackagesShouldSucceed(true)
        , installResult(WPEFramework::Core::ERROR_NONE)
        , installFailReason(FailReason::NONE)
        , registerCalls(0)
        , unregisterCalls(0)
        , getConfigCalls(0)
        , listPackagesCalls(0)
        , installCalls(0)
    {
    }

    ~PackageInstallerFake() override = default;

    PackageInstallerFake(const PackageInstallerFake&) = delete;
    PackageInstallerFake& operator=(const PackageInstallerFake&) = delete;

    // IUnknown
    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t n = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == n) {
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

    // IPackageInstaller
    WPEFramework::Core::hresult Register(WPEFramework::Exchange::IPackageInstaller::INotification* /*sink*/) override
    {
        registerCalls.fetch_add(1, std::memory_order_relaxed);
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unregister(WPEFramework::Exchange::IPackageInstaller::INotification* /*sink*/) override
    {
        unregisterCalls.fetch_add(1, std::memory_order_relaxed);
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult GetConfigForPackage(const std::string& /*fileLocator*/,
                                                     std::string& id,
                                                     std::string& version,
                                                     WPEFramework::Exchange::RuntimeConfig& /*config*/) override
    {
        getConfigCalls.fetch_add(1, std::memory_order_relaxed);
        if (!getConfigShouldSucceed) {
            return WPEFramework::Core::ERROR_GENERAL;
        }
        id      = returnPackageId;
        version = returnVersion;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult ListPackages(WPEFramework::Exchange::IPackageInstaller::IPackageIterator*& packages) override
    {
        listPackagesCalls.fetch_add(1, std::memory_order_relaxed);
        if (!listPackagesShouldSucceed) {
            packages = nullptr;
            return WPEFramework::Core::ERROR_GENERAL;
        }
        auto* iter = new PackageIteratorFake();
        for (const auto& entry : installedPackages) {
            iter->AddEntry(entry.packageId, entry.version, entry.state);
        }
        packages = iter;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Install(const std::string& packageId,
                                        const std::string& version,
                                        IKeyValueIterator* const& /*additionalMetadata*/,
                                        const std::string& /*fileLocator*/,
                                        FailReason& failReason) override
    {
        installCalls.fetch_add(1, std::memory_order_relaxed);
        {
            std::lock_guard<std::mutex> lock(_installMtx);
            lastInstalledPackageId = packageId;
            lastInstalledVersion   = version;
        }
        if (installResult != WPEFramework::Core::ERROR_NONE) {
            failReason = installFailReason;
        }
        return installResult;
    }

    WPEFramework::Core::hresult Uninstall(const std::string& /*packageId*/,
                                           std::string& /*errorReason*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Config(const std::string& /*packageId*/,
                                        const std::string& /*version*/,
                                        WPEFramework::Exchange::RuntimeConfig& /*configMetadata*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult PackageState(const std::string& /*packageId*/,
                                              const std::string& /*version*/,
                                              InstallState& /*state*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    // ==== Configurable behavior ====
    bool          getConfigShouldSucceed;
    std::string   returnPackageId;
    std::string   returnVersion;
    bool          listPackagesShouldSucceed;
    std::vector<Package> installedPackages;
    WPEFramework::Core::hresult installResult;
    FailReason    installFailReason;

    // ==== Observability ====
    std::atomic<uint32_t> registerCalls;
    std::atomic<uint32_t> unregisterCalls;
    std::atomic<uint32_t> getConfigCalls;
    std::atomic<uint32_t> listPackagesCalls;
    std::atomic<uint32_t> installCalls;
    std::string           lastInstalledPackageId;
    std::string           lastInstalledVersion;

private:
    mutable std::atomic<uint32_t> _refCount;
    std::mutex _installMtx;
};

// ============================================================
// ServiceMock
// implements PluginHost::IShell
// Supports both plugin-level tests (via InstantiateHandler / COMLink)
// and implementation-level tests (via QueryInterfaceByCallsign for IPackageInstaller).
// ============================================================
class ServiceMock final : public WPEFramework::PluginHost::IShell {
public:
    using InstantiateHandler = std::function<void*(const WPEFramework::RPC::Object&, const uint32_t, uint32_t&)>;

    class COMLinkMock final : public WPEFramework::PluginHost::IShell::ICOMLink {
    public:
        explicit COMLinkMock(ServiceMock& parent)
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
            _parent.instantiateCalls.fetch_add(1, std::memory_order_relaxed);
            if (_parent._instantiateHandler) {
                return _parent._instantiateHandler(object, interfaceId, connectionId);
            }
            connectionId = 0;
            return nullptr;
        }

    private:
        ServiceMock& _parent;
    };

    ServiceMock()
        : _refCount(1)
        , _comLink(*this)
        , _configLine("{}")
        , _packageInstaller(nullptr)
        , _packageInstallerEnabled(false)
        , addRefCalls(0)
        , releaseCalls(0)
        , instantiateCalls(0)
        , deactivateCalls(0)
        , registerRpcNotifCalls(0)
        , unregisterRpcNotifCalls(0)
        , capturedRpcNotification(nullptr)
    {
    }

    ~ServiceMock() override = default;

    ServiceMock(const ServiceMock&) = delete;
    ServiceMock& operator=(const ServiceMock&) = delete;

    void SetInstantiateHandler(InstantiateHandler handler)
    {
        _instantiateHandler = handler;
    }

    void SetConfigLine(const std::string& line)
    {
        _configLine = line;
    }

    void SetPackageInstaller(WPEFramework::Exchange::IPackageInstaller* p)
    {
        _packageInstaller        = p;
        _packageInstallerEnabled = (p != nullptr);
    }

    // ==== Observability ====
    mutable std::atomic<uint32_t> addRefCalls;
    mutable std::atomic<uint32_t> releaseCalls;
    mutable std::atomic<uint32_t> instantiateCalls;
    mutable std::atomic<uint32_t> deactivateCalls;
    mutable std::atomic<uint32_t> registerRpcNotifCalls;
    mutable std::atomic<uint32_t> unregisterRpcNotifCalls;

    // Captured RPC notification (to simulate remote process events in tests)
    WPEFramework::RPC::IRemoteConnection::INotification* capturedRpcNotification;

    // ==== IUnknown ====
    void AddRef() const override
    {
        addRefCalls.fetch_add(1, std::memory_order_relaxed);
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        releaseCalls.fetch_add(1, std::memory_order_relaxed);
        const uint32_t n = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        return (0U == n) ? WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED
                         : WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t) override
    {
        return nullptr;
    }

    // ==== IShell read-only methods ====
    std::string ConfigLine() const override       { return _configLine; }
    std::string Versions() const override         { return ""; }
    std::string Locator() const override          { return ""; }
    std::string ClassName() const override        { return ""; }
    std::string Callsign() const override         { return "org.rdk.PreinstallManager"; }
    std::string WebPrefix() const override        { return ""; }
    std::string PersistentPath() const override   { return "/tmp"; }
    std::string VolatilePath() const override     { return "/tmp"; }
    std::string DataPath() const override         { return "/tmp"; }
    state       State() const override            { return state::ACTIVATED; }
    std::string Model() const override            { return ""; }
    bool        Background() const override       { return false; }
    std::string Accessor() const override         { return ""; }
    std::string ProxyStubPath() const override    { return ""; }
    std::string HashKey() const override          { return ""; }
    std::string Substitute(const std::string&) const override { return ""; }
    std::string SystemPath() const override       { return ""; }
    std::string PluginPath() const override       { return ""; }
    bool        Resumed() const override          { return false; }
    bool        IsSupported(const uint8_t) const override { return false; }
    startup     Startup() const override          { return static_cast<startup>(0); }
    reason      Reason() const override           { return reason::REQUESTED; }
    std::string SystemRootPath() const override   { return ""; }

    // ==== IShell mutating methods ====
    WPEFramework::Core::hresult Activate(const reason) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Deactivate(const reason) override
    {
        deactivateCalls.fetch_add(1, std::memory_order_relaxed);
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unavailable(const reason) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult ConfigLine(const std::string& line) override
    {
        _configLine = line;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult SystemRootPath(const std::string&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Hibernate(const uint32_t) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Startup(const startup) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Resumed(const bool) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Metadata(std::string& info) const override
    {
        info.clear();
        return WPEFramework::Core::ERROR_NONE;
    }

    void EnableWebServer(const std::string&, const std::string&) override {}
    void DisableWebServer() override {}

    WPEFramework::PluginHost::ISubSystem* SubSystems() override { return nullptr; }

    uint32_t Submit(const uint32_t,
                    const WPEFramework::Core::ProxyType<WPEFramework::Core::JSON::IElement>&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    void Notify(const std::string&) override {}

    // ==== QueryInterfaceByCallsign — returns PackageInstaller when requested ====
    void* QueryInterfaceByCallsign(const uint32_t interfaceId, const std::string& callsign) override
    {
        if (_packageInstallerEnabled && (_packageInstaller != nullptr)) {
            if (callsign == "org.rdk.PackageManagerRDKEMS" &&
                interfaceId == WPEFramework::Exchange::IPackageInstaller::ID) {
                _packageInstaller->AddRef();
                return static_cast<void*>(_packageInstaller);
            }
        }
        return nullptr;
    }

    // ==== Plugin notification registration ====
    void Register(WPEFramework::PluginHost::IPlugin::INotification*) override   {}
    void Unregister(WPEFramework::PluginHost::IPlugin::INotification*) override {}

    // ==== RPC connection notification registration ====
    // PreinstallManager::Initialize calls service->Register(&mPreinstallManagerNotification).
    // Capture the pointer so tests can simulate remote process events.
    void Register(WPEFramework::RPC::IRemoteConnection::INotification* notif) override
    {
        registerRpcNotifCalls.fetch_add(1, std::memory_order_relaxed);
        capturedRpcNotification = notif;
    }

    void Unregister(const WPEFramework::RPC::IRemoteConnection::INotification*) override
    {
        unregisterRpcNotifCalls.fetch_add(1, std::memory_order_relaxed);
    }

    // ==== RemoteConnection — returns nullptr (no OOP in L0) ====
    WPEFramework::RPC::IRemoteConnection* RemoteConnection(const uint32_t) const
    {
        return nullptr;
    }

    WPEFramework::PluginHost::IShell::ICOMLink* COMLink() override
    {
        return &_comLink;
    }

private:
    mutable std::atomic<uint32_t> _refCount;
    InstantiateHandler _instantiateHandler;
    COMLinkMock _comLink;
    std::string _configLine;
    WPEFramework::Exchange::IPackageInstaller* _packageInstaller;
    bool _packageInstallerEnabled;
};

// ============================================================
// FakePreinstallImpl
// implements IPreinstallManager + IConfiguration
// Used for plugin-level tests where Initialize() is expected to succeed.
// ============================================================
class FakePreinstallImpl final
    : public WPEFramework::Exchange::IPreinstallManager
    , public WPEFramework::Exchange::IConfiguration {
public:
    FakePreinstallImpl()
        : _refCount(1)
        , configureResult(WPEFramework::Core::ERROR_NONE)
    {
    }

    ~FakePreinstallImpl() override = default;

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t n = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == n) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IPreinstallManager::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IPreinstallManager*>(this);
        }
        if (id == WPEFramework::Exchange::IConfiguration::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IConfiguration*>(this);
        }
        return nullptr;
    }

    // IPreinstallManager
    WPEFramework::Core::hresult Register(WPEFramework::Exchange::IPreinstallManager::INotification* n) override
    {
        if (nullptr != n) { n->AddRef(); }
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unregister(WPEFramework::Exchange::IPreinstallManager::INotification* n) override
    {
        if (nullptr != n) { n->Release(); }
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult StartPreinstall(const bool) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    // IConfiguration
    uint32_t Configure(WPEFramework::PluginHost::IShell*) override
    {
        return configureResult;
    }

    uint32_t configureResult;

private:
    mutable std::atomic<uint32_t> _refCount;
};

// ============================================================
// FakePreinstallImplNoConfig
// implements IPreinstallManager only (no IConfiguration)
// Used for plugin-level tests where Initialize() should fail because
// IConfiguration is missing.
// ============================================================
class FakePreinstallImplNoConfig final : public WPEFramework::Exchange::IPreinstallManager {
public:
    FakePreinstallImplNoConfig()
        : _refCount(1)
    {
    }

    ~FakePreinstallImplNoConfig() override = default;

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t n = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == n) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IPreinstallManager::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IPreinstallManager*>(this);
        }
        // Intentionally does NOT expose IConfiguration
        return nullptr;
    }

    WPEFramework::Core::hresult Register(WPEFramework::Exchange::IPreinstallManager::INotification* n) override
    {
        if (nullptr != n) { n->AddRef(); }
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unregister(WPEFramework::Exchange::IPreinstallManager::INotification* n) override
    {
        if (nullptr != n) { n->Release(); }
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult StartPreinstall(const bool) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

private:
    mutable std::atomic<uint32_t> _refCount;
};

// ============================================================
// FakeRPCConnection
// implements RPC::IRemoteConnection
// Used to simulate an out-of-process failure notification with a
// specific connection ID to test the Deactivated() code path.
// ============================================================
class FakeRPCConnection final : public WPEFramework::RPC::IRemoteConnection {
public:
    explicit FakeRPCConnection(const uint32_t id)
        : _refCount(1)
        , _id(id)
    {
    }

    ~FakeRPCConnection() override = default;

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t n = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == n) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::RPC::IRemoteConnection::ID) {
            AddRef();
            return static_cast<WPEFramework::RPC::IRemoteConnection*>(this);
        }
        return nullptr;
    }

    uint32_t Id() const override { return _id; }
    void Terminate() override {}
    void* Acquire(const uint32_t, const WPEFramework::Core::TUID, const uint32_t) override { return nullptr; }

private:
    mutable std::atomic<uint32_t> _refCount;
    uint32_t _id;
};

// ============================================================
// TempDir
// RAII helper: creates a temp directory on construction, removes all
// its files and the directory itself on destruction.
// Also shared with other plugins' L0 tests — placed here to be reusable.
// ============================================================
struct TempDir {
    explicit TempDir(const std::string& path)
        : _path(path)
    {
        ::mkdir(_path.c_str(), 0777);
    }

    ~TempDir()
    {
        Cleanup();
    }

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    // Create an empty regular file inside the directory.
    void AddFile(const std::string& name)
    {
        const std::string fullPath = _path + "/" + name;
        const int fd = ::open(fullPath.c_str(), O_CREAT | O_RDWR, 0644);
        if (fd >= 0) {
            ::close(fd);
        }
    }

    const std::string& Path() const { return _path; }

private:
    void Cleanup()
    {
        DIR* dir = ::opendir(_path.c_str());
        if (nullptr == dir) {
            return;
        }
        struct dirent* entry = nullptr;
        while (nullptr != (entry = ::readdir(dir))) {
            const std::string name(entry->d_name);
            if (name == "." || name == "..") {
                continue;
            }
            const std::string fullPath = _path + "/" + name;
            ::unlink(fullPath.c_str());
            ::rmdir(fullPath.c_str());
        }
        ::closedir(dir);
        ::rmdir(_path.c_str());
    }

    std::string _path;
};

} // namespace L0Test
