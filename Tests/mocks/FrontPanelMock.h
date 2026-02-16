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
#include <interfaces/IFrontPanel.h>

using ::WPEFramework::Core::hresult;
using ::WPEFramework::Exchange::IFrontPanel;
using namespace WPEFramework;

class FrontPanelMock : public IFrontPanel {
public:
    FrontPanelMock() = default;
    virtual ~FrontPanelMock() = default;
    
    MOCK_METHOD(hresult, Configure, (WPEFramework::PluginHost::IShell* service), (override));
    MOCK_METHOD(hresult, GetBrightness, (const string& index, uint32_t &brightness, bool &success), (override));
    MOCK_METHOD(hresult, GetFrontPanelLights, (IFrontPanelLightsListIterator*& supportedLights, string &supportedLightsInfo, bool &success), (override));
    MOCK_METHOD(hresult, PowerLedOff, (const string& index, FrontPanelSuccess &success), (override));
    MOCK_METHOD(hresult, PowerLedOn, (const string& index, FrontPanelSuccess &success), (override));
    MOCK_METHOD(hresult, SetBlink, (const string& blinkInfo, FrontPanelSuccess &success), (override));
    MOCK_METHOD(hresult, SetBrightness, (const string& index, const uint32_t brightness, FrontPanelSuccess &success), (override));
    MOCK_METHOD(hresult, SetLED, (const string& ledIndicator, const uint32_t brightness, const string& color, const uint32_t red, const uint32_t green, const uint32_t blue, FrontPanelSuccess &success), (override));
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfacenumber), (override));
};