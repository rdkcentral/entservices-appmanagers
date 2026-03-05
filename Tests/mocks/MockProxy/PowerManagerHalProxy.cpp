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

#include "../plat_power.h"
#include "../deepSleepMgr.h"
#include "../thunder/Module.h"
#ifndef MODULE_NAME
#define MODULE_NAME RdkServicesL2Test
#endif
#include <core/core.h>
#include <interfaces/IPowerManagerHal.h>
#include <cassert> 
#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

class PowerManagerHalMockProxy {
private:
PowerManagerHalMockProxy();  // Constructor
~PowerManagerHalMockProxy();

static PowerManagerHalMockProxy& Instance() {
    static PowerManagerHalMockProxy instance;
    return instance;
}

public:
    // Interface methods
    static pmStatus_t PlatINIT();
    static pmStatus_t PlatAPI_SetPowerState(PWRMgr_PowerState_t);
    static pmStatus_t PlatAPI_GetPowerState(PWRMgr_PowerState_t*);
    static pmStatus_t PlatAPI_SetWakeupSrc(PWRMGR_WakeupSrcType_t, bool);
    static pmStatus_t PlatAPI_GetWakeupSrc(PWRMGR_WakeupSrcType_t, bool*);
    static pmStatus_t PlatReset(PWRMgr_PowerState_t);
    static pmStatus_t PlatTERM();

    static DeepSleep_Return_Status_t PlatDS_INIT();
    static DeepSleep_Return_Status_t PlatDS_SetDeepSleep(uint32_t, bool*, bool);
    static DeepSleep_Return_Status_t PlatDS_DeepSleepWakeup();
    static DeepSleep_Return_Status_t PlatDS_GetLastWakeupReason(DeepSleep_WakeupReason_t*);
    static DeepSleep_Return_Status_t PlatDS_GetLastWakeupKeyCode(DeepSleepMgr_WakeupKeyCode_Param_t*);
    static DeepSleep_Return_Status_t PlatDS_TERM();

    uint32_t Initialize();
    // Static member to hold the implementation
    static WPEFramework::Exchange::IPowerManagerHal* _mockPluginPowerManagerHal;

private:
    PowerManagerHalMockProxy(const PowerManagerHalMockProxy&) = delete;
    PowerManagerHalMockProxy& operator=(const PowerManagerHalMockProxy&) = delete;

private:
    WPEFramework::Core::ProxyType<WPEFramework::RPC::InvokeServerType<1, 0, 4>> _engine;
    WPEFramework::Core::ProxyType<WPEFramework::RPC::CommunicatorClient> _communicatorClient;
};

WPEFramework::Exchange::IPowerManagerHal* PowerManagerHalMockProxy::_mockPluginPowerManagerHal = nullptr;

uint32_t PowerManagerHalMockProxy::Initialize() {
    TEST_LOG("Initializing COM RPC engine and communicator client");

    if (!_engine.IsValid()) {
        _engine = WPEFramework::Core::ProxyType<WPEFramework::RPC::InvokeServerType<1, 0, 4>>::Create();
        if (!_engine.IsValid()) {
            TEST_LOG("Failed to create _engine");
            return WPEFramework::Core::ERROR_GENERAL;
        }
    }
    if (!_communicatorClient.IsValid()) {
        _communicatorClient = WPEFramework::Core::ProxyType<WPEFramework::RPC::CommunicatorClient>::Create(
            WPEFramework::Core::NodeId("/tmp/communicator"),
            WPEFramework::Core::ProxyType<WPEFramework::Core::IIPCServer>(_engine)
        );
        if (!_communicatorClient.IsValid()) {
            TEST_LOG("Failed to create _communicatorClient");
            return WPEFramework::Core::ERROR_GENERAL;
        }
    }
        TEST_LOG("Connecting the COM-RPC socket");
        _mockPluginPowerManagerHal = _communicatorClient->Open<WPEFramework::Exchange::IPowerManagerHal>(_T("org.rdk.MockPlugin"), ~0, 3000);
        if (_mockPluginPowerManagerHal) {
            TEST_LOG("Connected to MockPlugin successfully");
        } else {
            TEST_LOG("Failed to create MockPlugin Controller");
        }

    return WPEFramework::Core::ERROR_NONE;
}

PowerManagerHalMockProxy::PowerManagerHalMockProxy()
{
    TEST_LOG("Inside PowerManagerHalMockProxy constructor");
}

PowerManagerHalMockProxy::~PowerManagerHalMockProxy() {
    TEST_LOG("Inside PowerManagerHalMockProxy destructor");
    if (_mockPluginPowerManagerHal) {
        _mockPluginPowerManagerHal->Release();
        _mockPluginPowerManagerHal = nullptr;
    }

    // Close the communicator client if it's valid
    if (_communicatorClient.IsValid()) {
        _communicatorClient->Close(WPEFramework::Core::infinite);
    }

    // Release the communicator client and engine
    _communicatorClient.Release();
    _engine.Release();
}

pmStatus_t PowerManagerHalMockProxy::PlatINIT()
{
    TEST_LOG("Inside PlatINIT");

    // Check if the mock plugin has been initialized
    if (!_mockPluginPowerManagerHal) {
        uint32_t result = Instance().Initialize();
        if (result != WPEFramework::Core::ERROR_NONE) {
            TEST_LOG("Mock plugin initialization failed with status: %u", result);
            return PWRMGR_INIT_FAILURE;
        }
    }

    if (_mockPluginPowerManagerHal) {
        TEST_LOG("Calling PlatINIT");

        // Simulate calling the PlatINIT method from the mock plugin
        pmStatus_t status = (pmStatus_t)_mockPluginPowerManagerHal->PLAT_INIT();

        if (status != PWRMGR_SUCCESS) {
            TEST_LOG("PlatINIT failed with status: %u", status);
            return status;
        }

        TEST_LOG("PlatINIT successful");
        return PWRMGR_SUCCESS;
    }

    TEST_LOG("Mock plugin is uninitialized");
    return PWRMGR_TERM_FAILURE;
}

pmStatus_t PowerManagerHalMockProxy::PlatAPI_SetPowerState(PWRMgr_PowerState_t newState)
{
    TEST_LOG("Inside PlatAPI_SetPowerState");

    // Check if the mock plugin has been initialized
    if (!_mockPluginPowerManagerHal) {
        uint32_t result = PowerManagerHalMockProxy::Instance().Initialize();
        if (result != WPEFramework::Core::ERROR_NONE) {
            TEST_LOG("Mock plugin initialization failed with status: %u", result);
            return PWRMGR_INIT_FAILURE;
        }
    }

    if (_mockPluginPowerManagerHal) {
        TEST_LOG("Calling PlatAPI_SetPowerState");

        // Simulate calling the PlatAPI_SetPowerState method from the mock plugin
        pmStatus_t status = (pmStatus_t)_mockPluginPowerManagerHal->PLAT_API_SetPowerState(newState);

        if (status != PWRMGR_SUCCESS) {
            TEST_LOG("PlatAPI_SetPowerState failed with status: %u", status);
            return status;
        }

        TEST_LOG("PlatAPI_SetPowerState successful");
        return PWRMGR_SUCCESS;
    }

    TEST_LOG("Mock plugin is uninitialized");
    return PWRMGR_TERM_FAILURE;
}

pmStatus_t PowerManagerHalMockProxy::PlatAPI_GetPowerState(PWRMgr_PowerState_t* curState)
{
    TEST_LOG("Inside PlatAPI_GetPowerState");

    // Check if the mock plugin has been initialized
    if (!_mockPluginPowerManagerHal) {
        uint32_t result = Instance().Initialize();
        if (result != WPEFramework::Core::ERROR_NONE) {
            TEST_LOG("Mock plugin initialization failed with status: %u", result);
            return PWRMGR_INIT_FAILURE;
        }
    }

    if (_mockPluginPowerManagerHal) {
        TEST_LOG("Calling PlatAPI_GetPowerState");
        uint32_t u32_curState;

        // Simulate calling the PlatAPI_GetPowerState method from the mock plugin
        pmStatus_t status = (pmStatus_t)_mockPluginPowerManagerHal->PLAT_API_GetPowerState(u32_curState);
        *curState = static_cast<PWRMgr_PowerState_t>(u32_curState);

        if (status != PWRMGR_SUCCESS) {
            TEST_LOG("PlatAPI_GetPowerState failed with status: %u", status);
            return status;
        }

        TEST_LOG("PlatAPI_GetPowerState successful");
        return PWRMGR_SUCCESS;
    }

    TEST_LOG("Mock plugin is uninitialized");
    return PWRMGR_TERM_FAILURE;
}

pmStatus_t PowerManagerHalMockProxy::PlatAPI_SetWakeupSrc(PWRMGR_WakeupSrcType_t srcType, bool enable)
{
    TEST_LOG("Inside PlatAPI_SetWakeupSrc");

    // Check if the mock plugin has been initialized
    if (!_mockPluginPowerManagerHal) {
        uint32_t result = PowerManagerHalMockProxy::Instance().Initialize();
        if (result != WPEFramework::Core::ERROR_NONE) {
            TEST_LOG("Mock plugin initialization failed with status: %u", result);
            return PWRMGR_INIT_FAILURE;
        }
    }

    if (_mockPluginPowerManagerHal) {
        TEST_LOG("Calling PlatAPI_SetWakeupSrc");

        // Simulate calling the PlatAPI_SetWakeupSrc method from the mock plugin
        pmStatus_t status = (pmStatus_t)_mockPluginPowerManagerHal->PLAT_API_SetWakeupSrc(srcType,enable);

        if (status != PWRMGR_SUCCESS) {
            TEST_LOG("PlatAPI_SetWakeupSrc failed with status: %u", status);
            return status;
        }

        TEST_LOG("PlatAPI_SetWakeupSrc successful");
        return PWRMGR_SUCCESS;
    }

    TEST_LOG("Mock plugin is uninitialized");
    return PWRMGR_TERM_FAILURE;
}

pmStatus_t PowerManagerHalMockProxy::PlatAPI_GetWakeupSrc(PWRMGR_WakeupSrcType_t srcType, bool* enable)
{
    TEST_LOG("Inside PlatAPI_GetWakeupSrc");

    // Check if the mock plugin has been initialized
    if (!_mockPluginPowerManagerHal) {
        uint32_t result = PowerManagerHalMockProxy::Instance().Initialize();
        if (result != WPEFramework::Core::ERROR_NONE) {
            TEST_LOG("Mock plugin initialization failed with status: %u", result);
            return PWRMGR_INIT_FAILURE;
        }
    }

    if (_mockPluginPowerManagerHal) {
        TEST_LOG("Calling PlatAPI_GetWakeupSrc");
        bool isEnable;

        // Simulate calling the PlatAPI_GetWakeupSrc method from the mock plugin
        pmStatus_t status = (pmStatus_t)_mockPluginPowerManagerHal->PLAT_API_GetWakeupSrc((uint32_t)srcType,isEnable);
        *enable= isEnable;

        if (status != PWRMGR_SUCCESS) {
            TEST_LOG("PlatAPI_GetWakeupSrc failed with status: %u", status);
            return status;
        }

        TEST_LOG("PlatAPI_GetWakeupSrc successful");
        return PWRMGR_SUCCESS;
    }

    TEST_LOG("Mock plugin is uninitialized");
    return PWRMGR_TERM_FAILURE;
}

pmStatus_t PowerManagerHalMockProxy::PlatReset(PWRMgr_PowerState_t newState)
{
    TEST_LOG("Inside PlatReset");

    // Check if the mock plugin has been initialized
    if (!_mockPluginPowerManagerHal) {
        uint32_t result = PowerManagerHalMockProxy::Instance().Initialize();
        if (result != WPEFramework::Core::ERROR_NONE) {
            TEST_LOG("Mock plugin initialization failed with status: %u", result);
            return PWRMGR_INIT_FAILURE;
        }
    }

    if (_mockPluginPowerManagerHal) {
        TEST_LOG("Calling PlatReset");

        // Simulate calling the PlatReset method from the mock plugin
        pmStatus_t status = (pmStatus_t)_mockPluginPowerManagerHal->PLAT_Reset(newState);

        if (status != PWRMGR_SUCCESS) {
            TEST_LOG("PlatReset failed with status: %u", status);
            return status;
        }

        TEST_LOG("PlatReset successful");
        return PWRMGR_SUCCESS;
    }

    TEST_LOG("Mock plugin is uninitialized");
    return PWRMGR_TERM_FAILURE;
}

pmStatus_t PowerManagerHalMockProxy::PlatTERM()
{
    TEST_LOG("Inside PlatTERM");

    // Check if the mock plugin has been initialized
    if (!_mockPluginPowerManagerHal) {
        uint32_t result = PowerManagerHalMockProxy::Instance().Initialize();
        if (result != WPEFramework::Core::ERROR_NONE) {
            TEST_LOG("Mock plugin initialization failed with status: %u", result);
            return PWRMGR_INIT_FAILURE;
        }
    }

    if (_mockPluginPowerManagerHal) {
        TEST_LOG("Calling PlatTERM");

        // Simulate calling the PlatTERM method from the mock plugin
        pmStatus_t status = (pmStatus_t)_mockPluginPowerManagerHal->PLAT_TERM();

        if (status != PWRMGR_SUCCESS) {
            TEST_LOG("PlatTERM failed with status: %u", status);
            return status;
        }

        TEST_LOG("PlatTERM successful");
        return PWRMGR_SUCCESS;
    }

    TEST_LOG("Mock plugin is uninitialized");
    return PWRMGR_TERM_FAILURE;
}

DeepSleep_Return_Status_t PowerManagerHalMockProxy::PlatDS_INIT()
{
    TEST_LOG("Inside PlatDS_INIT");

    // Check if the mock plugin has been initialized
    if (!_mockPluginPowerManagerHal) {
        uint32_t result = PowerManagerHalMockProxy::Instance().Initialize();
        if (result != WPEFramework::Core::ERROR_NONE) {
            TEST_LOG("Mock plugin initialization failed with status: %u", result);
            return DEEPSLEEPMGR_INIT_FAILURE;
        }
    }

    if (_mockPluginPowerManagerHal) {
        TEST_LOG("Calling PlatDS_INIT");

        // Simulate calling the PlatDS_INIT method from the mock plugin
        DeepSleep_Return_Status_t status = (DeepSleep_Return_Status_t)_mockPluginPowerManagerHal->PLAT_DS_INIT();

        if (status != DEEPSLEEPMGR_SUCCESS) {
            TEST_LOG("PlatDS_INIT failed with status: %u", status);
            return status;
        }

        TEST_LOG("PlatDS_INIT successful");
        return DEEPSLEEPMGR_SUCCESS;
    }

    TEST_LOG("Mock plugin is uninitialized");
    return DEEPSLEEPMGR_TERM_FAILURE;
}

DeepSleep_Return_Status_t PowerManagerHalMockProxy::PlatDS_SetDeepSleep(uint32_t deep_sleep_timeout, bool* isGPIOWakeup, bool networkStandby)
{
    TEST_LOG("Inside PlatDS_SetDeepSleep");

    // Check if the mock plugin has been initialized
    if (!_mockPluginPowerManagerHal) {
        uint32_t result = PowerManagerHalMockProxy::Instance().Initialize();
        if (result != WPEFramework::Core::ERROR_NONE) {
            TEST_LOG("Mock plugin initialization failed with status: %u", result);
            return DEEPSLEEPMGR_INIT_FAILURE;
        }
    }

    if (_mockPluginPowerManagerHal) {
        TEST_LOG("Calling PlatDS_SetDeepSleep");
        bool bGPIOWakeup;

        // Simulate calling the PlatDS_SetDeepSleep method from the mock plugin
        DeepSleep_Return_Status_t status = (DeepSleep_Return_Status_t)_mockPluginPowerManagerHal->PLAT_DS_SetDeepSleep(deep_sleep_timeout, bGPIOWakeup, networkStandby);
        *isGPIOWakeup = bGPIOWakeup;

        if (status != DEEPSLEEPMGR_SUCCESS) {
            TEST_LOG("PlatDS_SetDeepSleep failed with status: %u", status);
            return status;
        }

        TEST_LOG("PlatDS_SetDeepSleep successful");
        return DEEPSLEEPMGR_SUCCESS;
    }

    TEST_LOG("Mock plugin is uninitialized");
    return DEEPSLEEPMGR_TERM_FAILURE;
}

DeepSleep_Return_Status_t PowerManagerHalMockProxy::PlatDS_DeepSleepWakeup()
{
    TEST_LOG("Inside PlatDS_DeepSleepWakeup");

    // Check if the mock plugin has been initialized
    if (!_mockPluginPowerManagerHal) {
        uint32_t result = PowerManagerHalMockProxy::Instance().Initialize();
        if (result != WPEFramework::Core::ERROR_NONE) {
            TEST_LOG("Mock plugin initialization failed with status: %u", result);
            return DEEPSLEEPMGR_INIT_FAILURE;
        }
    }

    if (_mockPluginPowerManagerHal) {
        TEST_LOG("Calling PlatDS_DeepSleepWakeup");

        // Simulate calling the PlatDS_DeepSleepWakeup method from the mock plugin
        DeepSleep_Return_Status_t status = (DeepSleep_Return_Status_t)_mockPluginPowerManagerHal->PLAT_DS_DeepSleepWakeup();

        if (status != DEEPSLEEPMGR_SUCCESS) {
            TEST_LOG("PlatDS_DeepSleepWakeup failed with status: %u", status);
            return status;
        }

        TEST_LOG("PlatDS_DeepSleepWakeup successful");
        return DEEPSLEEPMGR_SUCCESS;
    }

    TEST_LOG("Mock plugin is uninitialized");
    return DEEPSLEEPMGR_TERM_FAILURE;
}

DeepSleep_Return_Status_t PowerManagerHalMockProxy::PlatDS_GetLastWakeupReason(DeepSleep_WakeupReason_t* wakeupReason)
{
    TEST_LOG("Inside PlatDS_GetLastWakeupReason");

    // Check if the mock plugin has been initialized
    if (!_mockPluginPowerManagerHal) {
        uint32_t result = PowerManagerHalMockProxy::Instance().Initialize();
        if (result != WPEFramework::Core::ERROR_NONE) {
            TEST_LOG("Mock plugin initialization failed with status: %u", result);
            return DEEPSLEEPMGR_INIT_FAILURE;
        }
    }

    if (_mockPluginPowerManagerHal) {
        TEST_LOG("Calling PlatDS_GetLastWakeupReason");
        uint32_t u32_wakeupReason;

        // Simulate calling the PlatDS_GetLastWakeupReason method from the mock plugin
        DeepSleep_Return_Status_t status = (DeepSleep_Return_Status_t)_mockPluginPowerManagerHal->PLAT_DS_GetLastWakeupReason(u32_wakeupReason);
        *wakeupReason = static_cast<DeepSleep_WakeupReason_t>(u32_wakeupReason);

        if (status != DEEPSLEEPMGR_SUCCESS) {
            TEST_LOG("PlatDS_GetLastWakeupReason failed with status: %u", status);
            return status;
        }

        TEST_LOG("PlatDS_GetLastWakeupReason successful");
        return DEEPSLEEPMGR_SUCCESS;
    }

    TEST_LOG("Mock plugin is uninitialized");
    return DEEPSLEEPMGR_TERM_FAILURE;
}

DeepSleep_Return_Status_t PowerManagerHalMockProxy::PlatDS_GetLastWakeupKeyCode(DeepSleepMgr_WakeupKeyCode_Param_t* wakeupKeyCode)
{
    TEST_LOG("Inside PlatDS_GetLastWakeupKeyCode");

    // Check if the mock plugin has been initialized
    if (!_mockPluginPowerManagerHal) {
        uint32_t result = PowerManagerHalMockProxy::Instance().Initialize();
        if (result != WPEFramework::Core::ERROR_NONE) {
            TEST_LOG("Mock plugin initialization failed with status: %u", result);
            return DEEPSLEEPMGR_INIT_FAILURE;
        }
    }

    if (_mockPluginPowerManagerHal) {
        TEST_LOG("Calling PlatDS_GetLastWakeupKeyCode");
        uint32_t u32_wakeupKeyCode;

        // Simulate calling the PlatDS_GetLastWakeupKeyCode method from the mock plugin
        DeepSleep_Return_Status_t status = (DeepSleep_Return_Status_t)_mockPluginPowerManagerHal->PLAT_DS_GetLastWakeupKeyCode(u32_wakeupKeyCode);
        wakeupKeyCode->keyCode = u32_wakeupKeyCode;

        if (status != DEEPSLEEPMGR_SUCCESS) {
            TEST_LOG("PlatDS_GetLastWakeupKeyCode failed with status: %u", status);
            return status;
        }

        TEST_LOG("PlatDS_GetLastWakeupKeyCode successful");
        return DEEPSLEEPMGR_SUCCESS;
    }

    TEST_LOG("Mock plugin is uninitialized");
    return DEEPSLEEPMGR_TERM_FAILURE;
}

DeepSleep_Return_Status_t PowerManagerHalMockProxy::PlatDS_TERM()
{
    TEST_LOG("Inside PlatDS_TERM");

    // Check if the mock plugin has been initialized
    if (!_mockPluginPowerManagerHal) {
        uint32_t result = PowerManagerHalMockProxy::Instance().Initialize();
        if (result != WPEFramework::Core::ERROR_NONE) {
            TEST_LOG("Mock plugin initialization failed with status: %u", result);
            return DEEPSLEEPMGR_INIT_FAILURE;
        }
    }

    if (_mockPluginPowerManagerHal) {
        TEST_LOG("Calling PlatDS_TERM");

        // Simulate calling the PlatDS_TERM method from the mock plugin
        DeepSleep_Return_Status_t status = (DeepSleep_Return_Status_t)_mockPluginPowerManagerHal->PLAT_DS_TERM();

        if (status != DEEPSLEEPMGR_SUCCESS) {
            TEST_LOG("PlatDS_TERM failed with status: %u", status);
            return status;
        }

        TEST_LOG("PlatDS_TERM successful");
        return DEEPSLEEPMGR_SUCCESS;
    }

    TEST_LOG("Mock plugin is uninitialized");
    return DEEPSLEEPMGR_TERM_FAILURE;
}

pmStatus_t (*PLAT_INIT)() = &PowerManagerHalMockProxy::PlatINIT;
pmStatus_t (*PLAT_API_SetPowerState)(PWRMgr_PowerState_t) = &PowerManagerHalMockProxy::PlatAPI_SetPowerState;
pmStatus_t (*PLAT_API_GetPowerState)(PWRMgr_PowerState_t*) = &PowerManagerHalMockProxy::PlatAPI_GetPowerState;
pmStatus_t (*PLAT_API_SetWakeupSrc)(PWRMGR_WakeupSrcType_t, bool) = &PowerManagerHalMockProxy::PlatAPI_SetWakeupSrc;
pmStatus_t (*PLAT_API_GetWakeupSrc)(PWRMGR_WakeupSrcType_t, bool*) = &PowerManagerHalMockProxy::PlatAPI_GetWakeupSrc;
pmStatus_t (*PLAT_Reset)(PWRMgr_PowerState_t) = &PowerManagerHalMockProxy::PlatReset;
pmStatus_t (*PLAT_TERM)() = &PowerManagerHalMockProxy::PlatTERM;

DeepSleep_Return_Status_t (*PLAT_DS_INIT)() = &PowerManagerHalMockProxy::PlatDS_INIT;
DeepSleep_Return_Status_t (*PLAT_DS_SetDeepSleep)(uint32_t, bool*, bool) = &PowerManagerHalMockProxy::PlatDS_SetDeepSleep;
DeepSleep_Return_Status_t (*PLAT_DS_DeepSleepWakeup)() = &PowerManagerHalMockProxy::PlatDS_DeepSleepWakeup;
DeepSleep_Return_Status_t (*PLAT_DS_GetLastWakeupReason)(DeepSleep_WakeupReason_t*) = &PowerManagerHalMockProxy::PlatDS_GetLastWakeupReason;
DeepSleep_Return_Status_t (*PLAT_DS_GetLastWakeupKeyCode)(DeepSleepMgr_WakeupKeyCode_Param_t*) = &PowerManagerHalMockProxy::PlatDS_GetLastWakeupKeyCode;
DeepSleep_Return_Status_t (*PLAT_DS_TERM)() = &PowerManagerHalMockProxy::PlatDS_TERM;

