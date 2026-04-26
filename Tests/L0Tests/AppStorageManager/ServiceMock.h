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
 */
class ServiceMock : public WPEFramework::PluginHost::IShell {
public:
    ServiceMock()
        : _refCount(1)
        , _configPath("{\"path\":\"/tmp/appdata\"}")
        , _fakeImpl(nullptr)
        , _fakeStore(nullptr)
        , _configLineCallCount(0)
        , _queryInterfaceByCallsignCallCount(0)
    {
    }

    ~ServiceMock() override = default;

    // Configuration helpers
    void SetConfigPath(const std::string& path)
    {
        _configPath = path;
    }

    void SetRootCreationResult(WPEFramework::Exchange::IAppStorageManager* impl)
    {
        _fakeImpl = impl;
    }

    void SetPersistentStoreResult(WPEFramework::Exchange::IStore2* store)
    {
        _fakeStore = store;
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

    string ConfigLine() const override
    {
        _configLineCallCount++;
        return _configPath;
    }

    void* QueryInterfaceByCallsign(const uint32_t id, const string& name) override
    {
        _queryInterfaceByCallsignCallCount++;
        
        if (name == "PersistentStore" && id == WPEFramework::Exchange::IStore2::ID) {
            if (_fakeStore != nullptr) {
                _fakeStore->AddRef();
                return _fakeStore;
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
    string Version() const override { return string("1.0"); }
    string Model() const override { return string(); }
    string Background() const override { return string(); }
    string Substitute(const string& input) const override { return input; }
    state State() const override { return ACTIVATED; }
    reason Reason() const override { return reason::REQUESTED; }

    bool Background(const string&) override { return false; }
    void Notify(const string&) override {}
    void Register(IPlugin::INotification*) override {}
    void Unregister(IPlugin::INotification*) override {}
    void Register(IShell::ICOMLink::INotification*) override {}
    void Unregister(IShell::ICOMLink::INotification*) override {}

    uint32_t Submit(const uint32_t, const WPEFramework::Core::ProxyType<WPEFramework::Core::JSON::IElement>&) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    void Activate(const reason) override {}
    void Deactivate(const reason) override {}
    void Unavailable(const reason) override {}

    ICOMLink* COMLink() override { return this; }

    // ICOMLink interface (minimal)
    void Register(ICOMLink::INotification*) override {}
    void Unregister(ICOMLink::INotification*) override {}
    uint32_t Activate(const string&, const uint32_t) override { return 0; }
    uint32_t Close(const uint32_t) override { return WPEFramework::Core::ERROR_NONE; }

    void* Instantiate(const RPC::Object&, const uint32_t, uint32_t&) override
    {
        if (_fakeImpl != nullptr) {
            _fakeImpl->AddRef();
            return _fakeImpl;
        }
        return nullptr;
    }

private:
    mutable std::atomic<uint32_t> _refCount;
    std::string _configPath;
    WPEFramework::Exchange::IAppStorageManager* _fakeImpl;
    WPEFramework::Exchange::IStore2* _fakeStore;
    mutable uint32_t _configLineCallCount;
    mutable uint32_t _queryInterfaceByCallsignCallCount;
};

} // namespace L0Test
