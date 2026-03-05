/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2024 RDK Management
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
**/
#pragma once

#include <gmock/gmock.h>
#include <interfaces/IDisplayInfo.h>


using ::WPEFramework::Exchange::IHDRProperties;

using ::WPEFramework::Exchange::IDisplayProperties;

using ::WPEFramework::Exchange::IGraphicsProperties;

class ConnectionPropertiesMock : public ::WPEFramework::Exchange::IConnectionProperties {
public:
    ConnectionPropertiesMock() = default;
    virtual ~ConnectionPropertiesMock() = default;

    MOCK_METHOD(uint32_t, Register, (INotification *notification), (override));
    MOCK_METHOD(uint32_t, Unregister, (INotification *notification), (override));
    MOCK_METHOD(uint32_t, IsAudioPassthrough, (bool& passthru), (const, override));
    MOCK_METHOD(uint32_t, Connected, (bool& isconnected), (const, override));
    MOCK_METHOD(uint32_t, Width, (uint32_t& width), (const, override));
    MOCK_METHOD(uint32_t, Height, (uint32_t& height), (const, override));
    MOCK_METHOD(uint32_t, VerticalFreq, (uint32_t& vf), (const, override));
    MOCK_METHOD(uint32_t, EDID, (uint16_t& length, uint8_t data[]), (const, override));
    MOCK_METHOD(uint32_t, WidthInCentimeters, (uint8_t& width), (const, override));
    MOCK_METHOD(uint32_t, HeightInCentimeters, (uint8_t& height), (const, override));
    MOCK_METHOD(uint32_t, HDCPProtection, (HDCPProtectionType& value), (const, override));
    MOCK_METHOD(uint32_t, HDCPProtection, (const HDCPProtectionType value), (override));
    MOCK_METHOD(uint32_t, PortName, (string& name), (const, override));


    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfaceNummer), (override));

};


class ConnectionPropertiesNotificationMock : public ::WPEFramework::Exchange::IConnectionProperties::INotification {
    public:
    ConnectionPropertiesNotificationMock() = default;
    virtual ~ConnectionPropertiesNotificationMock() = default;
        MOCK_METHOD(void, Updated, (const Source event), (override));

};


class HDRPropertiesMock : public IHDRProperties {
    public:
    HDRPropertiesMock() = default;
    virtual ~HDRPropertiesMock() = default;
    
    MOCK_METHOD(uint32_t, TVCapabilities, (IHDRIterator*& type), (const, override));
    MOCK_METHOD(uint32_t, STBCapabilities, (IHDRIterator*& type ), (const, override));
    MOCK_METHOD(uint32_t, HDRSetting, (HDRType& type), (const, override));

    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfaceNummer), (override));


};

class DisplayPropertiesMock : public IDisplayProperties {
    public:
    DisplayPropertiesMock() = default;
    virtual ~DisplayPropertiesMock() = default;
    
    MOCK_METHOD(uint32_t, ColorSpace, (ColourSpaceType& cs), (const, override));
    MOCK_METHOD(uint32_t, FrameRate, (FrameRateType& rate), (const, override));
    MOCK_METHOD(uint32_t, ColourDepth, (ColourDepthType& colour), (const, override));
    MOCK_METHOD(uint32_t, Colorimetry, (IColorimetryIterator*& colorimetry), (const, override));
    MOCK_METHOD(uint32_t, QuantizationRange, (QuantizationRangeType& qr), (const, override));
    MOCK_METHOD(uint32_t, EOTF, (EotfType& eotf), (const, override));

    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfaceNummer), (override));

};


class GraphicsPropertiesMock : public IGraphicsProperties {
    public:
    GraphicsPropertiesMock() = default;
    virtual ~GraphicsPropertiesMock() = default;
    
    MOCK_METHOD(uint32_t, TotalGpuRam, (uint64_t& total), (const, override));
    MOCK_METHOD(uint32_t, FreeGpuRam, (uint64_t& free), (const, override));

    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfaceNummer), (override));
};
