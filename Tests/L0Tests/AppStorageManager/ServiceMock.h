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

#include "Module.h"
#include <interfaces/IAppStorageManager.h>
#include <com/com.h>
#include <atomic>
#include <string>

namespace WPEFramework {
namespace Plugin {
    class StorageManagerImplementation;
}
}

namespace L0Test {

/**
 * @brief Minimal IShell mock for AppStorageManager L0 tests
 * Following RuntimeManager pattern with Config struct
 */
class ServiceMock : public WPEFramework::PluginHost::IShell {
public:
    struct Config {
        explicit Config(WPEFramework::Exchange::IStore2* store = nullptr)
            : persistentStore(store)
            , configLine("{\"path\":\"/tmp/appdata\"}")
        {
        }
        WPEFramework::Exchange::IStore2* persistentStore;
        std::string configLine;
    };

    explicit ServiceMock(Config cfg = Config())
        : _refCount(1)
        , _cfg(cfg)
        , _fakeImpl(nullptr)
        , _configLineCallCount(0)
        , _queryInterfaceByCallsignCallCount(0)
        , _comLink(*this)
    {
    }

    ~ServiceMock() override = default;

    // Configuration helper for backward compatibility with tests that set impl directly
    void SetRootCreationResult(WPEFramework::Exchange::IAppStorageManager* impl)
    {
        _fakeImpl = impl;
    }

    uint32_t GetConfigLineCallCount() const { return _configLineCallCount; }
    uint32_t GetQueryInterfaceByCallsignCallCount() const { return _queryInterfaceByCallsignCallCount; }

    // IShell interface implementation
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

    void* QueryInterface(const uint32_t interfaceNumber) override
    {
        if (interfaceNumber == WPEFramework::PluginHost::IShell::ID) {
            AddRef();
            return static_cast<WPEFramework::PluginHost::IShell*>(this);
        }
        return nullptr;
    }

    string ConfigLine() const override
    {
        _configLineCallCount++;
        return _cfg.configLine;
    }

    void* QueryInterfaceByCallsign(const uint32_t id, const string& name) override
    {
        _queryInterfaceByCallsignCallCount++;
        
        if (name == "org.rdk.PersistentStore" && id == WPEFramework::Exchange::IStore2::ID) {
            if (_cfg.persistentStore != nullptr) {
                _cfg.persistentStore->AddRef();
                return _cfg.persistentStore;
            }
        }
        return nullptr;
    }

    // Minimal required IShell methods (stubs)
    string Locator() const override { return string(); }
    string ProxyStubPath() const override { return string(); }
    string Accessor() const override { return string(); }
    string VolatilePath() const override { return string("/tmp"); }
    string PersistentPath() const override { return string("/tmp"); }
    string DataPath() const override { return string("/tmp"); }
    string HashKey() const override { return string(); }
    string Callsign() const override { return string("AppStorageManager"); }
    string Model() const override { return string(); }
    bool Background() const override { return false; }
    string Substitute(const string& input) const override { return input; }
    state State() const override { return ACTIVATED; }
    reason Reason() const override { return reason::REQUESTED; }

    void Notify(const string&) override {}
    void Register(WPEFramework::PluginHost::IPlugin::INotification*) override {}
    void Unregister(WPEFramework::PluginHost::IPlugin::INotification*) override {}

    // Additional IShell pure virtual methods
    void EnableWebServer(const string& URLPath, const string& fileSystemPath) override
    {
        (void)URLPath;
        (void)fileSystemPath;
    }

    void DisableWebServer() override {}

    string WebPrefix() const override { return string("/Service/AppStorageManager"); }
    string ClassName() const override { return string("AppStorageManager"); }
    string Versions() const override { return string(); }
    string SystemPath() const override { return string("/usr/bin"); }
    string PluginPath() const override { return string("/usr/lib/wpeframework/plugins"); }
    string SystemRootPath() const override { return string("/"); }

    WPEFramework::Core::hresult SystemRootPath(const string& systemRootPath) override
    {
        (void)systemRootPath;
        return WPEFramework::Core::ERROR_NONE;
    }

    startup Startup() const override { return startup::ACTIVATED; }

    WPEFramework::Core::hresult Startup(const startup value) override
    {
        (void)value;
        return WPEFramework::Core::ERROR_NONE;
    }

    bool Resumed() const override { return false; }

    WPEFramework::Core::hresult Resumed(const bool value) override
    {
        (void)value;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult ConfigLine(const string& config) override
    {
        _cfg.configLine = config;
        _configLineCallCount++;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Metadata(string& info) const override
    {
        info = string();
        return WPEFramework::Core::ERROR_NONE;
    }

    bool IsSupported(const uint8_t version) const override
    {
        (void)version;
        return true;
    }

    WPEFramework::PluginHost::ISubSystem* SubSystems() override
    {
        return nullptr;
    }

    WPEFramework::Core::hresult Hibernate(const uint32_t timeout) override
    {
        (void)timeout;
        return WPEFramework::Core::ERROR_NONE;
    }

    uint32_t Submit(const uint32_t, const WPEFramework::Core::ProxyType<WPEFramework::Core::JSON::IElement>&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Activate(const reason) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Deactivate(const reason) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unavailable(const reason) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    class COMLinkMock : public ICOMLink {
    public:
        COMLinkMock(ServiceMock& parent) : _parent(parent) {}
        ~COMLinkMock() override = default;

        void Register(WPEFramework::RPC::IRemoteConnection::INotification*) override {}
        void Unregister(const WPEFramework::RPC::IRemoteConnection::INotification*) override {}

        void Register(INotification*) override {}
        void Unregister(INotification*) override {}

        WPEFramework::RPC::IRemoteConnection* RemoteConnection(const uint32_t) override { return nullptr; }

        void* Instantiate(const WPEFramework::RPC::Object& object, const uint32_t interfaceId, uint32_t& connectionId) override
        {
            if (_parent._instantiateHandler) {
                return _parent._instantiateHandler(object, interfaceId, connectionId);
            }
            connectionId = 0;
            return nullptr;
        }

    private:
        ServiceMock& _parent;
    };

    ICOMLink* COMLink() override
    {
        return &_comLink;
    }

private:
    COMLinkMock _comLink;
    mutable std::atomic<uint32_t> _refCount;
    Config _cfg;
    InstantiateHandler _instantiateHandler;
    mutable uint32_t _configLineCallCount;
    mutable uint32_t _queryInterfaceByCallsignCallCount;
};

} // namespace L0Test
