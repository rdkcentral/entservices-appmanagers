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
#include "ISubSystem.h"

using namespace WPEFramework::PluginHost;
using namespace std;

class SubSystemMock : public ISubSystem {
public:
    SubSystemMock() = default;
    virtual ~SubSystemMock() = default;

    MOCK_METHOD(void, Register, (ISubSystem::INotification* notification), (override));
    MOCK_METHOD(void, Unregister, (ISubSystem::INotification* notification), (override));
    MOCK_METHOD(void, Set, (const subsystem type, WPEFramework::Core::IUnknown* information), (override));
    MOCK_METHOD(const WPEFramework::Core::IUnknown*, Get, (const subsystem type), (const, override));
    MOCK_METHOD(bool, IsActive, (const subsystem type), (const, override));
    MOCK_METHOD(string, BuildTreeHash, (), (const, override));
    MOCK_METHOD(string, Version, (), (const, override));

    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfaceNummer), (override));
};
