/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2026 RDK Management
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

#include <map>
#include <atomic>
#include <string>
#include <interfaces/IStore2.h>

namespace L0Test {

using string = std::string;

/**
 * FakePersistentStore - Mock implementation of IStore2 for testing
 */
class FakePersistentStore : public WPEFramework::Exchange::IStore2 {
public:
    FakePersistentStore() : _refCount(1) {}

    void AddRef() const override {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override {
        if (id == WPEFramework::Exchange::IStore2::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IStore2*>(this);
        }
        return nullptr;
    }

    WPEFramework::Core::hresult SetValue(const ScopeType scope, const string& ns, const string& key, 
                                         const string& value, const uint32_t ttl) override {
        std::string fullKey = MakeKey(scope, ns, key);
        _store[fullKey] = value;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult GetValue(const ScopeType scope, const string& ns, const string& key, 
                                         string& value, uint32_t& ttl) override {
        std::string fullKey = MakeKey(scope, ns, key);
        auto it = _store.find(fullKey);
        if (it != _store.end()) {
            value = it->second;
            ttl = 0;
            return WPEFramework::Core::ERROR_NONE;
        }
        return WPEFramework::Core::ERROR_UNKNOWN_KEY;
    }

    WPEFramework::Core::hresult DeleteKey(const ScopeType scope, const string& ns, const string& key) override {
        std::string fullKey = MakeKey(scope, ns, key);
        _store.erase(fullKey);
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult DeleteNamespace(const ScopeType scope, const string& ns) override {
        std::string prefix = std::to_string(static_cast<int>(scope)) + ":" + ns + ":";
        auto it = _store.begin();
        while (it != _store.end()) {
            if (it->first.find(prefix) == 0) {
                it = _store.erase(it);
            } else {
                ++it;
            }
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult GetKeys(ScopeType scope, const string& ns, WPEFramework::RPC::IStringIterator*& iterator) override {
        // Stub implementation
        (void)scope;
        (void)ns;
        (void)iterator;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult GetNamespaces(ScopeType scope, WPEFramework::RPC::IStringIterator*& iterator) override {
        // Stub implementation
        (void)scope;
        (void)iterator;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult GetStorageSizes(ScopeType scope, WPEFramework::RPC::IValueIterator*& iterator) override {
        // Stub implementation
        (void)scope;
        (void)iterator;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult FlushCache() override {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Register(INotification*) override {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unregister(INotification*) override {
        return WPEFramework::Core::ERROR_NONE;
    }

private:
    std::string MakeKey(ScopeType scope, const string& ns, const string& key) const {
        return std::to_string(static_cast<int>(scope)) + ":" + ns + ":" + key;
    }

    mutable std::atomic<uint32_t> _refCount;
    std::map<std::string, std::string> _store;
};

} // namespace L0Test
