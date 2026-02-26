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
#include <gtest/gtest.h>

#include "PowerManagerAPI.h"

class PowerManagerHalMock : public PowerManagerImpl {
public:
    virtual ~PowerManagerHalMock() = default;
    // power hal
    MOCK_METHOD(pmStatus_t, PLAT_INIT, (), ());
    MOCK_METHOD(pmStatus_t, PLAT_API_SetPowerState, (PWRMgr_PowerState_t), ());
    MOCK_METHOD(pmStatus_t, PLAT_API_GetPowerState, (PWRMgr_PowerState_t*), ());
    MOCK_METHOD(pmStatus_t, PLAT_API_SetWakeupSrc, (PWRMGR_WakeupSrcType_t, bool), ());
    MOCK_METHOD(pmStatus_t, PLAT_API_GetWakeupSrc, (PWRMGR_WakeupSrcType_t, bool*), ());
    MOCK_METHOD(pmStatus_t, PLAT_Reset, (PWRMgr_PowerState_t), ());
    MOCK_METHOD(pmStatus_t, PLAT_TERM, (), ());

    // deepsleep hal
    MOCK_METHOD(DeepSleep_Return_Status_t, PLAT_DS_INIT, (), ());
    MOCK_METHOD(DeepSleep_Return_Status_t, PLAT_DS_SetDeepSleep, (uint32_t, bool*, bool), ());
    MOCK_METHOD(DeepSleep_Return_Status_t, PLAT_DS_DeepSleepWakeup, (), ());
    MOCK_METHOD(DeepSleep_Return_Status_t, PLAT_DS_GetLastWakeupReason, (DeepSleep_WakeupReason_t*), ());
    MOCK_METHOD(DeepSleep_Return_Status_t, PLAT_DS_GetLastWakeupKeyCode, (DeepSleepMgr_WakeupKeyCode_Param_t*), ());
    MOCK_METHOD(DeepSleep_Return_Status_t, PLAT_DS_TERM, (), ());
};


