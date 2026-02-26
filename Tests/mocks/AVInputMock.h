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
#include <interfaces/IAVInput.h>

using ::WPEFramework::Core::hresult;
using ::WPEFramework::Exchange::IAVInput;

class AVInputMock : public IAVInput {
public:
    AVInputMock() = default;
    virtual ~AVInputMock() = default;

    MOCK_METHOD(hresult, RegisterDevicesChangedNotification, (IDevicesChangedNotification * notification), (override));
    MOCK_METHOD(hresult, UnregisterDevicesChangedNotification, (IDevicesChangedNotification * notification), (override));
    MOCK_METHOD(hresult, RegisterSignalChangedNotification, (ISignalChangedNotification * notification), (override));
    MOCK_METHOD(hresult, UnregisterSignalChangedNotification, (ISignalChangedNotification * notification), (override));
    MOCK_METHOD(hresult, RegisterInputStatusChangedNotification, (IInputStatusChangedNotification * notification), (override));
    MOCK_METHOD(hresult, UnregisterInputStatusChangedNotification, (IInputStatusChangedNotification * notification), (override));
    MOCK_METHOD(hresult, RegisterVideoStreamInfoUpdateNotification, (IVideoStreamInfoUpdateNotification * notification), (override));
    MOCK_METHOD(hresult, UnregisterVideoStreamInfoUpdateNotification, (IVideoStreamInfoUpdateNotification * notification), (override));
    MOCK_METHOD(hresult, RegisterGameFeatureStatusUpdateNotification, (IGameFeatureStatusUpdateNotification * notification), (override));
    MOCK_METHOD(hresult, UnregisterGameFeatureStatusUpdateNotification, (IGameFeatureStatusUpdateNotification * notification), (override));
    MOCK_METHOD(hresult, RegisterAviContentTypeUpdateNotification, (IAviContentTypeUpdateNotification * notification), (override));
    MOCK_METHOD(hresult, UnregisterAviContentTypeUpdateNotification, (IAviContentTypeUpdateNotification * notification), (override));

    MOCK_METHOD(hresult, NumberOfInputs, (uint32_t& numberOfInputs, bool& success), (override));
    MOCK_METHOD(hresult, WriteEDID, (const string& portId, const string& message, WPEFramework::Exchange::IAVInput::SuccessResult& successResult), (override));
    MOCK_METHOD(hresult, ReadEDID, (const string& portId, string& EDID, bool& success), (override));
    MOCK_METHOD(hresult, GetRawSPD, (const string& portId, string& HDMISPD, bool& success), (override));
    MOCK_METHOD(hresult, GetSPD, (const string& portId, string& HDMISPD, bool& success), (override));
    MOCK_METHOD(hresult, SetEdidVersion, (const string& portId, const string& edidVersion, WPEFramework::Exchange::IAVInput::SuccessResult& successResult), (override));
    MOCK_METHOD(hresult, GetEdidVersion, (const string& portId, string& edidVersion, bool& success), (override));
    MOCK_METHOD(hresult, SetEdid2AllmSupport, (const string& portId, const bool allmSupport, WPEFramework::Exchange::IAVInput::SuccessResult& successResult), (override));
    MOCK_METHOD(hresult, GetEdid2AllmSupport, (const string& portId, bool& allmSupport, bool& success), (override));
    MOCK_METHOD(hresult, SetVRRSupport, (const string& portId, const bool vrrSupport, SuccessResult& successResult), (override));
    MOCK_METHOD(hresult, GetVRRSupport, (const string& portId, bool& vrrSupport, bool& success), (override));
    MOCK_METHOD(hresult, GetHdmiVersion, (const string& portId, string& HdmiCapabilityVersion, bool& success), (override));
    MOCK_METHOD(hresult, SetMixerLevels, (const int primaryVolume, const int inputVolume, WPEFramework::Exchange::IAVInput::SuccessResult& successResult), (override));
    MOCK_METHOD(hresult, StartInput, (const string& portId, const string& typeOfInput, const bool audioMix, const int planeType, const bool topMost, SuccessResult& successResult), (override));
    MOCK_METHOD(hresult, StopInput, (const string& typeOfInput, WPEFramework::Exchange::IAVInput::SuccessResult& successResult), (override));
    MOCK_METHOD(hresult, SetVideoRectangle, (const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h, const string& typeOfInput, WPEFramework::Exchange::IAVInput::SuccessResult& successResult), (override));
    MOCK_METHOD(hresult, CurrentVideoMode, (string& currentVideoMode, bool& success), (override));
    MOCK_METHOD(hresult, ContentProtected, (bool& isContentProtected, bool& success), (override));
    MOCK_METHOD(hresult, GetSupportedGameFeatures, (IStringIterator*& features, bool& success), (override));
    MOCK_METHOD(hresult, GetGameFeatureStatus, (const string& portId, const string& gameFeature, bool& mode, bool& success), (override));
    MOCK_METHOD(hresult, GetVRRFrameRate, (const string& portId, double& currentVRRVideoFrameRate, bool& success), (override));
    
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfacenumber), (override));
};

class DevicesChangedNotificationMock : public IAVInput::IDevicesChangedNotification {
public:
    DevicesChangedNotificationMock() = default;
    virtual ~DevicesChangedNotificationMock() = default;

    MOCK_METHOD(void, OnDevicesChanged, (WPEFramework::Exchange::IAVInput::IInputDeviceIterator* const devices), (override));
};

class SignalChangedNotificationMock : public IAVInput::ISignalChangedNotification {
public:
    SignalChangedNotificationMock() = default;
    virtual ~SignalChangedNotificationMock() = default;

    MOCK_METHOD(void, OnSignalChanged, (const int id, const string& locator, const string& signalStatus), (override));
};

class InputStatusChangedNotificationMock : public IAVInput::IInputStatusChangedNotification {
public:
    InputStatusChangedNotificationMock() = default;
    virtual ~InputStatusChangedNotificationMock() = default;

    MOCK_METHOD(void, OnInputStatusChanged, (const int id, const string& locator, const string& status, const int plane), (override));
};

class VideoStreamInfoUpdateNotificationMock : public IAVInput::IVideoStreamInfoUpdateNotification {
public:
    VideoStreamInfoUpdateNotificationMock() = default;
    virtual ~VideoStreamInfoUpdateNotificationMock() = default;

    MOCK_METHOD(void, VideoStreamInfoUpdate, (const int id, const string& locator, const int width, const int height, const bool progressive, const int frameRateN, const int frameRateD), (override));
};

class GameFeatureStatusUpdateNotificationMock : public IAVInput::IGameFeatureStatusUpdateNotification {
public:
    GameFeatureStatusUpdateNotificationMock() = default;
    virtual ~GameFeatureStatusUpdateNotificationMock() = default;

    MOCK_METHOD(void, GameFeatureStatusUpdate, (const int id, const string& gameFeature, const bool mode), (override));
};

class AviContentTypeUpdateNotificationMock : public IAVInput::IAviContentTypeUpdateNotification {
public:
    AviContentTypeUpdateNotificationMock() = default;
    virtual ~AviContentTypeUpdateNotificationMock() = default;

    MOCK_METHOD(void, AviContentTypeUpdate, (const int id, const int aviContentType), (override));
};
