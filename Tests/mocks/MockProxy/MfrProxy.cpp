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

#include "../mfr_temperature.h"
#include "../thunder/Module.h"
#ifndef MODULE_NAME
#define MODULE_NAME RdkServicesL2Test
#endif
#include <core/core.h>
#include <interfaces/IMfr.h>
#include <cassert> 
#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", __FILE__, __LINE__, __FUNCTION__, getpid(), gettid(), ##__VA_ARGS__); fflush(stderr);

class MfrProxy {
public:
MfrProxy();  // Constructor
~MfrProxy();

static MfrProxy& Instance() {
    static MfrProxy instance;
    return instance;
}

    // Interface methods
    static mfrError_t mfrGetTempThresholds(int*high, int*critical);
    static mfrError_t mfrSetTempThresholds(int tempHigh, int tempCritical);
    static mfrError_t mfrGetTemperature(mfrTemperatureState_t* curState, int* curTemperature, int* wifiTemperature);

    uint32_t Initialize();
    // Static member to hold the implementation
    static WPEFramework::Exchange::IMfr* _mockPluginMfr;

private:
    MfrProxy(const MfrProxy&) = delete;
    MfrProxy& operator=(const MfrProxy&) = delete;

private:
    WPEFramework::Core::ProxyType<WPEFramework::RPC::InvokeServerType<1, 0, 4>> _engine;
    WPEFramework::Core::ProxyType<WPEFramework::RPC::CommunicatorClient> _communicatorClient;
};

WPEFramework::Exchange::IMfr* MfrProxy::_mockPluginMfr = nullptr;

uint32_t MfrProxy::Initialize() {
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
        _mockPluginMfr = _communicatorClient->Open<WPEFramework::Exchange::IMfr>(_T("org.rdk.MockPlugin"), ~0, 3000);
        if (_mockPluginMfr) {
            TEST_LOG("Connected to MockPlugin successfully");
        } else {
            TEST_LOG("Failed to create MockPlugin Controller");
        }

    return WPEFramework::Core::ERROR_NONE;
}

MfrProxy::MfrProxy()
{
    TEST_LOG("Inside MfrProxy constructor");
}

MfrProxy::~MfrProxy() {
    TEST_LOG("Inside MfrProxy destructor");
    if (_mockPluginMfr) {
        _mockPluginMfr->Release();
        _mockPluginMfr = nullptr;
    }

    // Close the communicator client if it's valid
    if (_communicatorClient.IsValid()) {
        _communicatorClient->Close(WPEFramework::Core::infinite);
    }

    // Release the communicator client and engine
    _communicatorClient.Release();
    _engine.Release();
}

mfrError_t MfrProxy::mfrGetTempThresholds(int*high, int*critical)
{
    TEST_LOG("Inside mfrGetTempThresholds");

    // Check if the mock plugin has been initialized
    if (!_mockPluginMfr) {
        uint32_t result = Instance().Initialize();
        if (result != WPEFramework::Core::ERROR_NONE) {
            TEST_LOG("Mock plugin initialization failed with status: %u", result);
            return mfrERR_NOT_INITIALIZED;
        }
    }

    if (_mockPluginMfr) {
        TEST_LOG("Calling mfrGetTempThresholds");
        int tempHigh, tempCritical;

        // Simulate calling the mfrGetTempThresholds method from the mock plugin
        mfrError_t status = (mfrError_t)_mockPluginMfr->mfrGetTempThresholds(tempHigh, tempCritical);
        *high = tempHigh;
        *critical = tempCritical;

        if (status != mfrERR_NONE) {
            TEST_LOG("mfrGetTempThresholds failed with status: %u", status);
            return status;
        }

        TEST_LOG("mfrGetTempThresholds successful");
        return mfrERR_NONE;
    }

    TEST_LOG("Mock plugin is uninitialized");
    return mfrERR_GENERAL;
}

mfrError_t MfrProxy::mfrSetTempThresholds(int tempHigh, int tempCritical)
{
    TEST_LOG("Inside mfrSetTempThresholds");

    // Check if the mock plugin has been initialized
    if (!_mockPluginMfr) {
        uint32_t result = Instance().Initialize();
        if (result != WPEFramework::Core::ERROR_NONE) {
            TEST_LOG("Mock plugin initialization failed with status: %u", result);
            return mfrERR_NOT_INITIALIZED;
        }
    }

    if (_mockPluginMfr) {
        TEST_LOG("Calling mfrSetTempThresholds");

        // Simulate calling the mfrSetTempThresholds method from the mock plugin
        mfrError_t status = (mfrError_t)_mockPluginMfr->mfrSetTempThresholds(tempHigh, tempCritical);

        if (status != mfrERR_NONE) {
            TEST_LOG("mfrSetTempThresholds failed with status: %u", status);
            return status;
        }

        TEST_LOG("mfrSetTempThresholds successful");
        return mfrERR_NONE;
    }

    TEST_LOG("Mock plugin is uninitialized");
    return mfrERR_GENERAL;
}

mfrError_t MfrProxy::mfrGetTemperature(mfrTemperatureState_t* curState, int* curTemperature, int* wifiTemperature)
{
    TEST_LOG("Inside mfrGetTemperature");

    // Check if the mock plugin has been initialized
    if (!_mockPluginMfr) {
        uint32_t result = Instance().Initialize();
        if (result != WPEFramework::Core::ERROR_NONE) {
            TEST_LOG("Mock plugin initialization failed with status: %u", result);
            return mfrERR_NOT_INITIALIZED;
        }
    }

    if (_mockPluginMfr) {
        TEST_LOG("Calling mfrGetTemperature");
        uint32_t _curState;
        int tempCurTemperature, tempWifiTemperature;

        // Simulate calling the mfrGetTemperature method from the mock plugin
        mfrError_t status = (mfrError_t)_mockPluginMfr->mfrGetTemperature( _curState, tempCurTemperature, tempWifiTemperature);
        *curState = static_cast<mfrTemperatureState_t>(_curState);
        *curTemperature = tempCurTemperature;
        *wifiTemperature = tempWifiTemperature;

        if (status != mfrERR_NONE) {
            TEST_LOG("mfrGetTemperature failed with status: %u", status);
            return status;
        }

        TEST_LOG("mfrGetTemperature successful");
        return mfrERR_NONE;
    }

    TEST_LOG("Mock plugin is uninitialized");
    return mfrERR_GENERAL;
}

mfrError_t (*mfrGetTempThresholds)(int *tempHigh, int *tempCritical) = &MfrProxy::mfrGetTempThresholds;
mfrError_t (*mfrSetTempThresholds)(int tempHigh, int tempCritical) = &MfrProxy::mfrSetTempThresholds;
mfrError_t (*mfrGetTemperature)(mfrTemperatureState_t *state, int *, int *) = &MfrProxy::mfrGetTemperature;
