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

#include "FakePersistentStore.h"

namespace L0Test {

FakePersistentStore::FakePersistentStore()
    : _getValueSuccess(true)
    , _setValueSuccess(true)
    , _deleteKeySuccess(true)
    , _getValueResult()
    , _getValueCalls(0)
    , _setValueCalls(0)
    , _deleteKeyCalls(0)
    , _refCount(1)
{
}

void FakePersistentStore::Reset()
{
    _storage.clear();
    _getValueSuccess = true;
    _setValueSuccess = true;
    _deleteKeySuccess = true;
    _getValueResult.clear();
    _getValueCalls = 0;
    _setValueCalls = 0;
    _deleteKeyCalls = 0;
    _lastKey.clear();
    _lastNamespace.clear();
    _lastValue.clear();
}

void FakePersistentStore::SetGetValueResult(bool success, const std::string& value)
{
    _getValueSuccess = success;
    _getValueResult = value;
}

void FakePersistentStore::SetSetValueResult(bool success)
{
    _setValueSuccess = success;
}

void FakePersistentStore::SetDeleteKeyResult(bool success)
{
    _deleteKeySuccess = success;
}

void FakePersistentStore::AddRef() const
{
    _refCount.fetch_add(1, std::memory_order_relaxed);
}

uint32_t FakePersistentStore::Release() const
{
    const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
    if (0U == remaining) {
        delete this;
        return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
    }
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FakePersistentStore::Register(INotification* notification)
{
    (void)notification; // Unused in fake implementation
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FakePersistentStore::Unregister(INotification* notification)
{
    (void)notification; // Unused in fake implementation
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FakePersistentStore::GetValue(const ScopeType scope, const string& ns, const string& key, string& value, uint32_t& ttl)
{
    (void)scope; // Unused in fake implementation
    _getValueCalls++;
    _lastNamespace = ns;
    _lastKey = key;

    if (!_getValueSuccess) {
        return WPEFramework::Core::ERROR_UNKNOWN_KEY;
    }

    // Check if we have a configured result
    if (!_getValueResult.empty()) {
        value = _getValueResult;
        ttl = 0; // No TTL in fake store
        return WPEFramework::Core::ERROR_NONE;
    }

    // Check actual storage
    auto nsIt = _storage.find(ns);
    if (nsIt != _storage.end()) {
        auto keyIt = nsIt->second.find(key);
        if (keyIt != nsIt->second.end()) {
            value = keyIt->second;
            ttl = 0; // No TTL in fake store
            return WPEFramework::Core::ERROR_NONE;
        }
    }

    return WPEFramework::Core::ERROR_UNKNOWN_KEY;
}

uint32_t FakePersistentStore::SetValue(const ScopeType scope, const string& ns, const string& key, const string& value, const uint32_t ttl)
{
    (void)scope; // Unused in fake implementation
    (void)ttl;   // Unused in fake implementation
    _setValueCalls++;
    _lastNamespace = ns;
    _lastKey = key;
    _lastValue = value;

    if (!_setValueSuccess) {
        return WPEFramework::Core::ERROR_GENERAL;
    }

    _storage[ns][key] = value;
    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FakePersistentStore::DeleteKey(const ScopeType scope, const string& ns, const string& key)
{
    (void)scope; // Unused in fake implementation
    _deleteKeyCalls++;
    _lastNamespace = ns;
    _lastKey = key;

    if (!_deleteKeySuccess) {
        return WPEFramework::Core::ERROR_GENERAL;
    }

    auto nsIt = _storage.find(ns);
    if (nsIt != _storage.end()) {
        nsIt->second.erase(key);
        if (nsIt->second.empty()) {
            _storage.erase(nsIt);
        }
    }

    return WPEFramework::Core::ERROR_NONE;
}

uint32_t FakePersistentStore::DeleteNamespace(const ScopeType scope, const string& ns)
{
    (void)scope; // Unused in fake implementation
    _storage.erase(ns);
    return WPEFramework::Core::ERROR_NONE;
}

} // namespace L0Test
