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
#include <interfaces/IUSBDevice.h>

using ::WPEFramework::Exchange::IUSBDevice;
using namespace WPEFramework;

class USBDeviceMock : public IUSBDevice {
public:
    USBDeviceMock() = default;
    virtual ~USBDeviceMock() = default;

    MOCK_METHOD(Core::hresult, Register, (INotification *notification), (override));
    MOCK_METHOD(Core::hresult, Unregister, (INotification *notification), (override));
    MOCK_METHOD(Core::hresult, GetDeviceList, (IUSBDeviceIterator*& devices), (const, override));
    MOCK_METHOD(Core::hresult, GetDeviceInfo, (const string &deviceName, USBDeviceInfo& deviceInfo), (const, override));
    MOCK_METHOD(Core::hresult, BindDriver, (const string &deviceName), (const, override));
    MOCK_METHOD(Core::hresult, UnbindDriver, (const string &deviceName), (const, override));
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfaceNummer), (override));
};


class USBDeviceNotificationMock : public IUSBDevice::INotification {
    public:
    USBDeviceNotificationMock() = default;
    virtual ~USBDeviceNotificationMock() = default;

    MOCK_METHOD(void, OnDevicePluggedIn, (const IUSBDevice::USBDevice &device), (override));
    MOCK_METHOD(void, OnDevicePluggedOut, (const IUSBDevice::USBDevice &device), (override));

};
