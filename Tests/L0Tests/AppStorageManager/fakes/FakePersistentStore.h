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
#include <interfaces/IStore2.h>
#include <atomic>
#include <map>
#include <string>

namespace L0Test {

/**
 * @brief Fake IStore2 implementation for testing without real persistent storage
 */
class FakePersistentStore : public WPEFramework::Exchange::IStore2 {
public:
    FakePersistentStore();
    ~FakePersistentStore() override = default;

    // Control interface
    void Reset();
    void SetGetValueResult(bool success, const std::string& value);
    void SetSetValueResult(bool success);
    void SetDeleteKeyResult(bool success);

    // Statistics
    uint32_t GetGetValueCallCount() const { return _getValueCalls; }
    uint32_t GetSetValueCallCount() const { return _setValueCalls; }
    uint32_t GetDeleteKeyCallCount() const { return _deleteKeyCalls; }
    std::string GetLastKey() const { return _lastKey; }
    std::string GetLastNamespace() const { return _lastNamespace; }
    std::string GetLastValue() const { return _lastValue; }

    // IStore2 interface
    void AddRef() const override;
    uint32_t Release() const override;

    uint32_t GetValue(const string& ns, const string& key, string& value) override;
    uint32_t SetValue(const string& ns, const string& key, const string& value) override;
    uint32_t DeleteKey(const string& ns, const string& key) override;
    uint32_t DeleteNamespace(const string& ns) override;

private:
    std::map<std::string, std::map<std::string, std::string>> _storage; // namespace -> key -> value
    bool _getValueSuccess;
    bool _setValueSuccess;
    bool _deleteKeySuccess;
    std::string _getValueResult;
    uint32_t _getValueCalls;
    uint32_t _setValueCalls;
    uint32_t _deleteKeyCalls;
    std::string _lastKey;
    std::string _lastNamespace;
    std::string _lastValue;
    mutable std::atomic<uint32_t> _refCount;
};

} // namespace L0Test
