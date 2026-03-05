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
#include <interfaces/IFrameRate.h>

using ::WPEFramework::Core::hresult;
using ::WPEFramework::Exchange::IFrameRate;

class FrameRateMock : public IFrameRate, public device::Host::IVideoDeviceEvents {
public:
    FrameRateMock() = default;
    virtual ~FrameRateMock() = default;

    MOCK_METHOD(hresult, Register, (INotification * notification), (override));
    MOCK_METHOD(hresult, Unregister, (INotification * notification), (override));
    MOCK_METHOD(hresult, SetCollectionFrequency, (int frequency, bool& success), (override));
    MOCK_METHOD(hresult, StartFpsCollection, (bool& success), (override));
    MOCK_METHOD(hresult, StopFpsCollection, (bool& success), (override));
    MOCK_METHOD(hresult, UpdateFps, (int newFpsValue, bool& success), (override));
    MOCK_METHOD(hresult, SetFrmMode, (int frmmode, bool& success), (override));
    MOCK_METHOD(hresult, GetFrmMode, (int &frmmode, bool& success), (override));
    MOCK_METHOD(hresult, GetDisplayFrameRate, (string& framerate, bool& success), (override));
    MOCK_METHOD(hresult, SetDisplayFrameRate, (const string& framerate , bool& success), (override));
    MOCK_METHOD(void, OnDisplayFrameratePreChange, (const std::string& frameRate), (override));
    MOCK_METHOD(void, OnDisplayFrameratePostChange, (const std::string& frameRate), (override));
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfacenumber), (override));
};

class FrameRateNotificationMock : public IFrameRate::INotification {
public:
    FrameRateNotificationMock() = default;
    virtual ~FrameRateNotificationMock() = default;

    MOCK_METHOD(void, OnFpsEvent, (int average, int min, int max), (override));
    MOCK_METHOD(void, OnDisplayFrameRateChanging, (const string& displayFrameRate), (override));
    MOCK_METHOD(void, OnDisplayFrameRateChanged, (const string& displayFrameRate), (override));

};

