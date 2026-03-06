/*
* If not stated otherwise in this file or this component's LICENSE
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
#include <interfaces/IRuntimeManager.h>

using ::WPEFramework::Exchange::IRuntimeManager;

class RuntimeManagerMock : public IRuntimeManager {
public:
    RuntimeManagerMock() = default;
    virtual ~RuntimeManagerMock() = default;

    MOCK_METHOD(WPEFramework::Core::hresult, Register, (INotification *notification), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Unregister, (INotification *notification), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Run, (const string& appId, const string& appInstanceId, const uint32_t userId, const uint32_t groupId, IValueIterator* const& ports, IStringIterator* const& paths, IStringIterator* const& debugSettings, const WPEFramework::Exchange::RuntimeConfig& runtimeConfigObject), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Hibernate, (const string& appInstanceId), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Wake, (const string& appInstanceId, const RuntimeState runtimeState), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Suspend, (const string& appInstanceId), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Resume, (const string& appInstanceId), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Terminate, (const string& appInstanceId), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Kill, (const string& appInstanceId), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, GetInfo, (const string& appInstanceId, string& info), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Annotate, (const string& appInstanceId, const string& key, const string& value), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Mount, (), (override));
    MOCK_METHOD(WPEFramework::Core::hresult, Unmount, (), (override));

    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfaceNummer), (override));

};


class RuntimeManagerNotificationMock : public IRuntimeManager::INotification {
    public:
    RuntimeManagerNotificationMock() = default;
    virtual ~RuntimeManagerNotificationMock() = default;
};
