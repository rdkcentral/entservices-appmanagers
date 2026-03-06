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
#include <interfaces/IDeviceDiagnostics.h>

using ::WPEFramework::Core::hresult;
using ::WPEFramework::Exchange::IDeviceDiagnostics;

class DeviceDiagnosticsMock : public IDeviceDiagnostics {
public:
    DeviceDiagnosticsMock() = default;
    virtual ~DeviceDiagnosticsMock() = default;

    MOCK_METHOD(hresult, Register, (INotification * notification), (override));
    MOCK_METHOD(hresult, Unregister, (INotification * notification), (override));
    MOCK_METHOD(hresult, GetConfiguration, (IStringIterator* const& names, IDeviceDiagnosticsParamListIterator*& paramList, bool &success), (override));
    MOCK_METHOD(hresult, GetMilestones, (IStringIterator * &milestones, bool &success), (override));
    MOCK_METHOD(hresult, LogMilestone, (const string& marker, bool &success), (override));
    MOCK_METHOD(hresult, GetAVDecoderStatus, (AvDecoderStatusResult & AVDecoderStatus), (override));
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfacenumber), (override));
};

class DeviceDiagnosticsNotificationMock : public IDeviceDiagnostics::INotification {
public:
    DeviceDiagnosticsNotificationMock() = default;
    virtual ~DeviceDiagnosticsNotificationMock() = default;

    MOCK_METHOD(void, OnAVDecoderStatusChanged, (const string& AVDecoderStatus), (override));
};
