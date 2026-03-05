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
#include <interfaces/IHdmiCecSource.h>

using ::WPEFramework::Core::hresult;
using ::WPEFramework::Exchange::IHdmiCecSource;

class HdmiCecSourceMock : public IHdmiCecSource {
public:
    HdmiCecSourceMock() = default;
    virtual ~HdmiCecSourceMock() = default;

    MOCK_METHOD(hresult, Register, (INotification *notification), (override));
    MOCK_METHOD(hresult, Unregister, (INotification *notification), (override));
    MOCK_METHOD(hresult, Configure, (WPEFramework::PluginHost::IShell* service), (override));
    MOCK_METHOD(hresult, GetActiveSourceStatus, (bool& status, bool& success), (override));
    MOCK_METHOD(hresult, GetEnabled, (bool& enabled, bool& success), (override));
    MOCK_METHOD(hresult, GetOSDName, (string& name, bool& success), (override));
    MOCK_METHOD(hresult, GetOTPEnabled, (bool& enabled, bool& success), (override));
    MOCK_METHOD(hresult, GetVendorId, (string& vendorid, bool& success), (override));
    MOCK_METHOD(hresult, PerformOTPAction, (HdmiCecSourceSuccess& success), (override));
    MOCK_METHOD(hresult, SendKeyPressEvent, (const uint32_t& logicalAddress, const uint32_t& keycode, HdmiCecSourceSuccess& success), (override));
    MOCK_METHOD(hresult, SendStandbyMessage, (HdmiCecSourceSuccess& success), (override));
    MOCK_METHOD(hresult, SetEnabled, (const bool& enabled, HdmiCecSourceSuccess& success), (override));
    MOCK_METHOD(hresult, SetOSDName, (const string& name, HdmiCecSourceSuccess& success), (override));
    MOCK_METHOD(hresult, SetOTPEnabled, (const bool& enabled, HdmiCecSourceSuccess& success), (override));
    MOCK_METHOD(hresult, SetVendorId, (const string& vendorid, HdmiCecSourceSuccess& success), (override));
    MOCK_METHOD(hresult, GetDeviceList, (uint32_t& numberofdevices, IHdmiCecSourceDeviceListIterator*& deviceList, bool& success), (override));
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfacenumber), (override));
    
};


class HdmiCecSourceNotificationMock : public IHdmiCecSource::INotification {
    public:
    HdmiCecSourceNotificationMock() = default;
    virtual ~HdmiCecSourceNotificationMock() = default;

    MOCK_METHOD(void, OnDeviceAdded, (const int logicalAddress), (override));
    MOCK_METHOD(void, OnDeviceRemoved, (const int logicalAddress), (override));
    MOCK_METHOD(void, OnDeviceInfoUpdated, (const int logicalAddress), (override));
    MOCK_METHOD(void, OnActiveSourceStatusUpdated, (const bool status), (override));
    MOCK_METHOD(void, StandbyMessageReceived, (const int logicalAddress), (override));
    MOCK_METHOD(void, OnKeyReleaseEvent, (const int logicalAddress), (override));
    MOCK_METHOD(void, OnKeyPressEvent, (const int logicalAddress, const int keyCode), (override));

};