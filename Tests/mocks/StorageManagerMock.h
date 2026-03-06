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

#include <gmock/gmock.h>
#include <interfaces/IStorageManager.h>

using ::WPEFramework::Exchange::IStorageManager;
using namespace WPEFramework;

class StorageManagerMock : public IStorageManager {
public:
    StorageManagerMock() = default;
    virtual ~StorageManagerMock() = default;

    MOCK_METHOD(Core::hresult, CreateStorage,(const string& appId, const uint32_t& size, string& path/* @out */, string& errorReason /* @out */), (override));
    MOCK_METHOD(Core::hresult, GetStorage,(const string& appId, const int32_t& userId, const int32_t& groupId, string& path/* @out */, uint32_t& size /* @out*/, uint32_t& used /* @out*/), (override));
    MOCK_METHOD(Core::hresult, DeleteStorage, (const string& appId, string& errorReason /* @out */), (override));
    MOCK_METHOD(Core::hresult, Clear, (const string& appId, string& errorReason /* @out */), (override));
    MOCK_METHOD(Core::hresult, ClearAll, (const string& exemptionAppIds, string& errorReason /* @out */), (override));
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfaceNummer), (override));
};

