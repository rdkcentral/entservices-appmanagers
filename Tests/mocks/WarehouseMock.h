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
#include <interfaces/IWarehouse.h>

using ::WPEFramework::Core::hresult;
using ::WPEFramework::Exchange::IWarehouse;

class WarehouseMock : public IWarehouse {
public:
    WarehouseMock() = default;
    virtual ~WarehouseMock() = default;

    MOCK_METHOD(hresult, Register, (INotification * notification), (override));
    MOCK_METHOD(hresult, Unregister, (INotification * notification), (override));
    MOCK_METHOD(hresult, ExecuteHardwareTest, (WarehouseSuccess& success), (override));
    MOCK_METHOD(hresult, GetHardwareTestResults, (bool& success, string& testResults), (override));
    MOCK_METHOD(hresult, InternalReset, (const string& passPhrase, WarehouseSuccessErr& successErr), (override));
    MOCK_METHOD(hresult, IsClean, (const int age, bool &clean, IStringIterator*& files, bool &success, string& error), (override));
    MOCK_METHOD(hresult, LightReset, (WarehouseSuccessErr& successErr), (override));
    MOCK_METHOD(hresult, ResetDevice, (const bool suppressReboot, const string& resetType, WarehouseSuccessErr& successErr), (override));
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfacenumber), (override));
};

class WarehouseNotificationMock : public IWarehouse::INotification {
public:
    WarehouseNotificationMock() = default;
    virtual ~WarehouseNotificationMock() = default;

    MOCK_METHOD(void, ResetDone, (const bool success, const string& error), (override));
};
