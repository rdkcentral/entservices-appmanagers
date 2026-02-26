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

// std inc
#include <gmock/gmock.h>

// interface inc
#include <interfaces/IPowerManager.h>

// service / internal inc
#include "PluginInterfaceBuilder.h"

using ::testing::NiceMock;

namespace Core = WPEFramework::Core;
namespace Plugin = WPEFramework::Plugin;
namespace Exchange = WPEFramework::Exchange;

class PowerManagerMock : public WPEFramework::Exchange::IPowerManager {

public:
    MOCK_METHOD(Core::hresult, Register, (Exchange::IPowerManager::IRebootNotification * notification), (override));
    MOCK_METHOD(Core::hresult, Unregister, (const Exchange::IPowerManager::IRebootNotification * notification), (override));
    MOCK_METHOD(Core::hresult, Register, (Exchange::IPowerManager::IModePreChangeNotification * notification), (override));
    MOCK_METHOD(Core::hresult, Unregister, (const Exchange::IPowerManager::IModePreChangeNotification* notification), (override));
    MOCK_METHOD(Core::hresult, Register, (Exchange::IPowerManager::IModeChangedNotification * notification), (override));
    MOCK_METHOD(Core::hresult, Unregister, (const Exchange::IPowerManager::IModeChangedNotification* notification), (override));
    MOCK_METHOD(Core::hresult, Register, (Exchange::IPowerManager::IDeepSleepTimeoutNotification * notification), (override));
    MOCK_METHOD(Core::hresult, Unregister, (const Exchange::IPowerManager::IDeepSleepTimeoutNotification* notification), (override));
    MOCK_METHOD(Core::hresult, Register, (Exchange::IPowerManager::INetworkStandbyModeChangedNotification * notification), (override));
    MOCK_METHOD(Core::hresult, Unregister, (const Exchange::IPowerManager::INetworkStandbyModeChangedNotification* notification), (override));
    MOCK_METHOD(Core::hresult, Register, (Exchange::IPowerManager::IThermalModeChangedNotification * notification), (override));
    MOCK_METHOD(Core::hresult, Unregister, (const Exchange::IPowerManager::IThermalModeChangedNotification* notification), (override));

    MOCK_METHOD(Core::hresult, SetPowerState, (const int keyCode, const PowerState powerState, const string& reason), (override));
    MOCK_METHOD(Core::hresult, GetPowerState, (PowerState & currentState, PowerState& previousState), (override, const));
    MOCK_METHOD(Core::hresult, GetThermalState, (float& currentTemperature), (override, const));
    MOCK_METHOD(Core::hresult, SetTemperatureThresholds, (float high, float critical), (override));
    MOCK_METHOD(Core::hresult, GetTemperatureThresholds, (float& high, float& critical), (override, const));
    MOCK_METHOD(Core::hresult, SetOvertempGraceInterval, (const int graceInterval), (override));
    MOCK_METHOD(Core::hresult, GetOvertempGraceInterval, (int& graceInterval), (override, const));
    MOCK_METHOD(Core::hresult, SetDeepSleepTimer, (const int timeOut), (override));
    MOCK_METHOD(Core::hresult, GetLastWakeupReason, (WakeupReason & wakeupReason), (override, const));
    MOCK_METHOD(Core::hresult, GetLastWakeupKeyCode, (int& keycode), (override, const));
    MOCK_METHOD(Core::hresult, Reboot, (const string& rebootRequestor, const string& rebootReasonCustom, const string& rebootReasonOther), (override));
    MOCK_METHOD(Core::hresult, SetNetworkStandbyMode, (const bool standbyMode), (override));
    MOCK_METHOD(Core::hresult, GetNetworkStandbyMode, (bool& standbyMode), (override));
    MOCK_METHOD(Core::hresult, SetWakeupSourceConfig, (Exchange::IPowerManager::IWakeupSourceConfigIterator*), (override));
    MOCK_METHOD(Core::hresult, GetWakeupSourceConfig, (Exchange::IPowerManager::IWakeupSourceConfigIterator*&), (override, const));
    MOCK_METHOD(Core::hresult, GetPowerStateBeforeReboot, (PowerState & powerStateBeforeReboot), (override));
    MOCK_METHOD(Core::hresult, AddPowerModePreChangeClient, (const string& clientName, uint32_t& clientId ), (override));
    MOCK_METHOD(Core::hresult, RemovePowerModePreChangeClient, (const uint32_t clientId ), (override));
    MOCK_METHOD(Core::hresult, PowerModePreChangeComplete, (const uint32_t clientId, const int transactionId), (override));
    MOCK_METHOD(Core::hresult, DelayPowerModeChangeBy, (const uint32_t clientId, const int transactionId, const int delayPeriod), (override));

    BEGIN_INTERFACE_MAP(PowerManagerMock)
    INTERFACE_ENTRY(Exchange::IPowerManager)
    END_INTERFACE_MAP

    static std::map<std::string, Core::ProxyType<Exchange::IPowerManager>>& mockInstances()
    {
        static std::map<std::string, Core::ProxyType<Exchange::IPowerManager>> mocks;
        return mocks;
    }

    static std::string testId()
    {
        auto* testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
        if (!testInfo) {
            return {};
        }

        // unique std::string {testClass#testName}
        return std::string(testInfo->test_suite_name()) + "#" + std::string(testInfo->name());
    }

    static Exchange::IPowerManager* Get()
    {
        std::string id = testId();

        // ASSERT_FALSE(id.empty()) << "testId should have been valid for all testcases";
        ASSERT(!id.empty());

        auto& mocks = mockInstances();

        auto it = mocks.find(id);
        if (it == mocks.end()) {
            // create new
            mocks.insert(std::pair<std::string, Core::ProxyType<PowerManagerMock>>(id, Core::ProxyType<PowerManagerMock>::Create()));
            it = mocks.find(id);
            ASSERT(it != mocks.end());
        }

        return &(*(it->second));
    }

    static PowerManagerMock& Mock()
    {
        /*PowerManagerMock *mock = dynamic_cast<PowerManagerMock *>(Get());*/
        // TODO: avoid static_cast
        PowerManagerMock* mock = static_cast<PowerManagerMock*>(Get());
        /*ASSERT_NE(mock, nullptr);*/
        return *mock;
    }

    static void Delete(void)
    {
        std::string id = testId();

        ASSERT_FALSE(id.empty()) << "testId should have been valid for all testcases";

        auto& mocks = mockInstances();

        auto it = mocks.find(id);
        if (it != mocks.end()) {
            // ProxyType<> Release gets called on destructor
            mocks.erase(it);
        }
    }
};

