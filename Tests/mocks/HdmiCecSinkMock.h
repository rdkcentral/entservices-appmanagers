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
#include <interfaces/IHdmiCecSink.h>

using ::WPEFramework::Core::hresult;
using ::WPEFramework::Exchange::IHdmiCecSink;

class HdmiCecSinkMock : public IHdmiCecSink {
public:
    HdmiCecSinkMock() = default;
    virtual ~HdmiCecSinkMock() = default;

    MOCK_METHOD(hresult, Register, (INotification *notification), (override));
    MOCK_METHOD(hresult, Unregister, (INotification *notification), (override));
    MOCK_METHOD(hresult, Configure, (WPEFramework::PluginHost::IShell* service), (override));
    MOCK_METHOD(hresult, GetActiveRoute, (bool &available, uint8_t &length, IHdmiCecSinkActivePathIterator*& pathList, string &ActiveRoute, bool &success), (override));
    MOCK_METHOD(hresult, GetActiveSource, (bool &available, uint8_t &logicalAddress, string &physicalAddress, string &deviceType, string &cecVersion, string &osdName, string &vendorID, string &powerStatus, string &port, bool &success), (override));
    MOCK_METHOD(hresult, GetAudioDeviceConnectedStatus, (bool &connected, bool &success), (override));
    MOCK_METHOD(hresult, GetDeviceList, (uint32_t &numberofdevices, IHdmiCecSinkDeviceListIterator*& deviceList, bool &success), (override));
    MOCK_METHOD(hresult, GetEnabled, (bool &enabled, bool &success), (override));
    MOCK_METHOD(hresult, GetOSDName, (string &name, bool &success), (override));
    MOCK_METHOD(hresult, GetVendorId, (string &vendorid, bool &success), (override));
    MOCK_METHOD(hresult, PrintDeviceList, (bool &printed, bool &success), (override));
    MOCK_METHOD(hresult, RequestActiveSource, (HdmiCecSinkSuccess &success), (override));
    MOCK_METHOD(hresult, RequestShortAudioDescriptor, (HdmiCecSinkSuccess &success), (override));
    MOCK_METHOD(hresult, SendAudioDevicePowerOnMessage, (HdmiCecSinkSuccess &success), (override));
    MOCK_METHOD(hresult, SendGetAudioStatusMessage, (HdmiCecSinkSuccess &success), (override));
    MOCK_METHOD(hresult, SendKeyPressEvent, (const uint32_t &logicalAddress, const uint32_t &keyCode, HdmiCecSinkSuccess &success), (override));
    MOCK_METHOD(hresult, SendUserControlPressed, (const uint32_t &logicalAddress, const uint32_t &keyCode, HdmiCecSinkSuccess &success), (override));
    MOCK_METHOD(hresult, SendUserControlReleased, (const uint32_t &logicalAddress, HdmiCecSinkSuccess &success), (override));
    MOCK_METHOD(hresult, SendStandbyMessage, (HdmiCecSinkSuccess &success), (override));
    MOCK_METHOD(hresult, SetActivePath, (const string &activePath, HdmiCecSinkSuccess &success), (override));
    MOCK_METHOD(hresult, SetActiveSource, (HdmiCecSinkSuccess &success), (override));
    MOCK_METHOD(hresult, SetEnabled, (const bool &enabled, HdmiCecSinkSuccess &success), (override));
    MOCK_METHOD(hresult, SetMenuLanguage, (const string &language, HdmiCecSinkSuccess &success), (override));
    MOCK_METHOD(hresult, SetOSDName, (const string &name, HdmiCecSinkSuccess &success), (override));
    MOCK_METHOD(hresult, SetRoutingChange, (const string &oldPort, const string &newPort, HdmiCecSinkSuccess &success), (override));
    MOCK_METHOD(hresult, SetupARCRouting, (const bool &enabled, HdmiCecSinkSuccess &success), (override));
    MOCK_METHOD(hresult, SetVendorId, (const string &vendorid, HdmiCecSinkSuccess &success), (override));
    MOCK_METHOD(hresult, SetLatencyInfo, (const string &videoLatency, const string &lowLatencyMode, const string &audioOutputCompensated, const string &audioOutputDelay, HdmiCecSinkSuccess &success), (override));



    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfacenumber), (override));
    
};

class HdmiCecSinkNotificationMock : public IHdmiCecSink::INotification {
    public:
    HdmiCecSinkNotificationMock() = default;
    virtual ~HdmiCecSinkNotificationMock() = default;

    MOCK_METHOD(void, ArcInitiationEvent, (const string success), (override));
    MOCK_METHOD(void, ArcTerminationEvent, (const string success), (override));
    MOCK_METHOD(void, OnActiveSourceChange, (const int logicalAddress, const string physicalAddress), (override));
    MOCK_METHOD(void, OnDeviceAdded, (const int logicalAddress), (override));
    MOCK_METHOD(void, OnDeviceInfoUpdated, (const int logicalAddress), (override));
    MOCK_METHOD(void, OnDeviceRemoved, (const int logicalAddress), (override));
    MOCK_METHOD(void, OnImageViewOnMsg, (const int logicalAddress), (override));
    MOCK_METHOD(void, OnInActiveSource, (const int logicalAddress, const string physicalAddress), (override));
    MOCK_METHOD(void, OnTextViewOnMsg, (const int logicalAddress), (override));
    MOCK_METHOD(void, OnWakeupFromStandby, (const int logicalAddress), (override));
    MOCK_METHOD(void, ReportAudioDeviceConnectedStatus, (const string status, const string audioDeviceConnected), (override));
    MOCK_METHOD(void, ReportAudioStatusEvent, (const int muteStatus, const int volumeLevel), (override));
    MOCK_METHOD(void, ReportFeatureAbortEvent, (const int logicalAddress, const int opcode, const int FeatureAbortReason), (override));
    MOCK_METHOD(void, ReportCecEnabledEvent, (const string cecEnable), (override));
    MOCK_METHOD(void, SetSystemAudioModeEvent, (const string audioMode), (override));
    MOCK_METHOD(void, ShortAudiodescriptorEvent, (const string& jsonresponse), (override));
    MOCK_METHOD(void, StandbyMessageReceived, (const int logicalAddress), (override));
    MOCK_METHOD(void, ReportAudioDevicePowerStatus, (const int powerStatus), (override));


};
