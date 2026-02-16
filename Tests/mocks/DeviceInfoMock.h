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
#include <interfaces/IDeviceInfo.h>
#include <interfaces/IConfiguration.h>

using ::WPEFramework::Core::hresult;
using ::WPEFramework::Exchange::IDeviceInfo;
using ::WPEFramework::Exchange::IDeviceAudioCapabilities;
using ::WPEFramework::Exchange::IDeviceVideoCapabilities;
using ::WPEFramework::Exchange::IConfiguration;

class DeviceInfoImplementationMock : public IDeviceInfo, public IConfiguration {
public:
    DeviceInfoImplementationMock() = default;
    virtual ~DeviceInfoImplementationMock() = default;

    // IDeviceInfo interface
    MOCK_METHOD(hresult, SerialNumber, (DeviceSerialNo& deviceSerialNo), (const, override));
    MOCK_METHOD(hresult, Sku, (DeviceModelNo& deviceModelNo), (const, override));
    MOCK_METHOD(hresult, Make, (DeviceMake& deviceMake), (const, override));
    MOCK_METHOD(hresult, Model, (DeviceModel& deviceModel), (const, override));
    MOCK_METHOD(hresult, DeviceType, (DeviceTypeInfos& deviceTypeInfos), (const, override));
    MOCK_METHOD(hresult, SocName, (DeviceSoc& deviceSoc), (const, override));
    MOCK_METHOD(hresult, DistributorId, (DeviceDistId& deviceDistId), (const, override));
    MOCK_METHOD(hresult, Brand, (DeviceBrand& deviceBrand), (const, override));
    MOCK_METHOD(hresult, ReleaseVersion, (DeviceReleaseVer& deviceReleaseVer), (const, override));
    MOCK_METHOD(hresult, ChipSet, (DeviceChip& deviceChip), (const, override));
    MOCK_METHOD(hresult, FirmwareVersion, (FirmwareversionInfo& firmwareVersionInfo), (const, override));
    MOCK_METHOD(hresult, SystemInfo, (SystemInfos& systemInfo), (const, override));
    MOCK_METHOD(hresult, Addresses, (IAddressesInfoIterator*& addressesInfo), (const, override));
    MOCK_METHOD(hresult, EthMac, (EthernetMac& ethernetMac), (const, override));
    MOCK_METHOD(hresult, EstbMac, (StbMac& stbMac), (const, override));
    MOCK_METHOD(hresult, WifiMac, (WiFiMac& wiFiMac), (const, override));
    MOCK_METHOD(hresult, EstbIp, (StbIp& stbIp), (const, override));
    MOCK_METHOD(hresult, SupportedAudioPorts, (WPEFramework::RPC::IStringIterator*& supportedAudioPorts, bool& success), (const, override));

    // IConfiguration interface
    MOCK_METHOD(uint32_t, Configure, (WPEFramework::PluginHost::IShell* service), (override));

    // IUnknown interface
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfacenumber), (override));
};

class DeviceAudioCapabilitiesMock : public IDeviceAudioCapabilities {
public:
    DeviceAudioCapabilitiesMock() = default;
    virtual ~DeviceAudioCapabilitiesMock() = default;

    // IDeviceAudioCapabilities interface
    MOCK_METHOD(hresult, AudioCapabilities, (const std::string& audioPort, IAudioCapabilityIterator*& audioCapabilities, bool& success), (const, override));
    MOCK_METHOD(hresult, MS12Capabilities, (const std::string& audioPort, IMS12CapabilityIterator*& ms12Capabilities, bool& success), (const, override));
    MOCK_METHOD(hresult, SupportedMS12AudioProfiles, (const std::string& audioPort, WPEFramework::RPC::IStringIterator*& supportedMS12AudioProfiles, bool& success), (const, override));

    // IUnknown interface
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfacenumber), (override));
};

class DeviceVideoCapabilitiesMock : public IDeviceVideoCapabilities {
public:
    DeviceVideoCapabilitiesMock() = default;
    virtual ~DeviceVideoCapabilitiesMock() = default;

    // IDeviceVideoCapabilities interface
    MOCK_METHOD(hresult, SupportedVideoDisplays, (WPEFramework::RPC::IStringIterator*& supportedVideoDisplays, bool& success), (const, override));
    MOCK_METHOD(hresult, HostEDID, (HostEdid& hostEdid), (const, override));
    MOCK_METHOD(hresult, DefaultResolution, (const std::string& videoDisplay, DefaultResln& defaultResln), (const, override));
    MOCK_METHOD(hresult, SupportedResolutions, (const std::string& videoDisplay, WPEFramework::RPC::IStringIterator*& supportedResolutions, bool& success), (const, override));
    MOCK_METHOD(hresult, SupportedHdcp, (const std::string& videoDisplay, SupportedHDCPVer& supportedHDCPVer), (const, override));

    // IUnknown interface
    MOCK_METHOD(void, AddRef, (), (const, override));
    MOCK_METHOD(uint32_t, Release, (), (const, override));
    MOCK_METHOD(void*, QueryInterface, (const uint32_t interfacenumber), (override));
};
