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
#include "PowerManagerHalMock.h"

PowerManagerImpl* PowerManagerAPI::impl = nullptr;
PowerManagerAPI::PowerManagerAPI(){}

void PowerManagerAPI::setImpl(PowerManagerImpl* newImpl) {
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

pmStatus_t PowerManagerAPI::PLAT_INIT(void)
{
    return impl->PLAT_INIT();
}

pmStatus_t PowerManagerAPI::PLAT_API_SetPowerState(PWRMgr_PowerState_t newState)
{
    return impl->PLAT_API_SetPowerState(newState);
}

pmStatus_t PowerManagerAPI::PLAT_API_GetPowerState(PWRMgr_PowerState_t* curState)
{
    return impl->PLAT_API_GetPowerState(curState);
}

pmStatus_t PowerManagerAPI::PLAT_API_SetWakeupSrc(PWRMGR_WakeupSrcType_t srcType, bool enable)
{
    return impl->PLAT_API_SetWakeupSrc(srcType, enable);
}

pmStatus_t PowerManagerAPI::PLAT_API_GetWakeupSrc(PWRMGR_WakeupSrcType_t srcType, bool* enable)
{
    return impl->PLAT_API_GetWakeupSrc(srcType, enable);
}

pmStatus_t PowerManagerAPI::PLAT_Reset(PWRMgr_PowerState_t newState)
{
    return impl->PLAT_Reset(newState);
}

pmStatus_t PowerManagerAPI::PLAT_TERM(void)
{
    return impl->PLAT_TERM();
}

DeepSleep_Return_Status_t PowerManagerAPI::PLAT_DS_INIT(void)
{
    return impl->PLAT_DS_INIT();
}

DeepSleep_Return_Status_t PowerManagerAPI::PLAT_DS_SetDeepSleep(uint32_t deep_sleep_timeout, bool* isGPIOWakeup, bool networkStandby)
{
    return impl->PLAT_DS_SetDeepSleep(deep_sleep_timeout, isGPIOWakeup, networkStandby);
}

DeepSleep_Return_Status_t PowerManagerAPI::PLAT_DS_DeepSleepWakeup(void)
{
    return impl->PLAT_DS_DeepSleepWakeup();
}

DeepSleep_Return_Status_t PowerManagerAPI::PLAT_DS_GetLastWakeupReason(DeepSleep_WakeupReason_t* wakeupReason)
{
    return impl->PLAT_DS_GetLastWakeupReason(wakeupReason);
}

DeepSleep_Return_Status_t PowerManagerAPI::PLAT_DS_GetLastWakeupKeyCode(DeepSleepMgr_WakeupKeyCode_Param_t* wakeupKeyCode)
{
    return impl->PLAT_DS_GetLastWakeupKeyCode(wakeupKeyCode);
}

DeepSleep_Return_Status_t PowerManagerAPI::PLAT_DS_TERM(void)
{
    return impl->PLAT_DS_TERM();
}

pmStatus_t (*PLAT_INIT)() = &PowerManagerAPI::PLAT_INIT;
pmStatus_t (*PLAT_API_SetPowerState)(PWRMgr_PowerState_t) = &PowerManagerAPI::PLAT_API_SetPowerState;
pmStatus_t (*PLAT_API_GetPowerState)(PWRMgr_PowerState_t*) = &PowerManagerAPI::PLAT_API_GetPowerState;
pmStatus_t (*PLAT_API_SetWakeupSrc)(PWRMGR_WakeupSrcType_t, bool) = &PowerManagerAPI::PLAT_API_SetWakeupSrc;
pmStatus_t (*PLAT_API_GetWakeupSrc)(PWRMGR_WakeupSrcType_t, bool*) = &PowerManagerAPI::PLAT_API_GetWakeupSrc;
pmStatus_t (*PLAT_Reset)(PWRMgr_PowerState_t) = &PowerManagerAPI::PLAT_Reset;
pmStatus_t (*PLAT_TERM)() = &PowerManagerAPI::PLAT_TERM;

DeepSleep_Return_Status_t (*PLAT_DS_INIT)() = &PowerManagerAPI::PLAT_DS_INIT;
DeepSleep_Return_Status_t (*PLAT_DS_SetDeepSleep)(uint32_t, bool*, bool) = &PowerManagerAPI::PLAT_DS_SetDeepSleep;
DeepSleep_Return_Status_t (*PLAT_DS_DeepSleepWakeup)() = &PowerManagerAPI::PLAT_DS_DeepSleepWakeup;
DeepSleep_Return_Status_t (*PLAT_DS_GetLastWakeupReason)(DeepSleep_WakeupReason_t*) = &PowerManagerAPI::PLAT_DS_GetLastWakeupReason;
DeepSleep_Return_Status_t (*PLAT_DS_GetLastWakeupKeyCode)(DeepSleepMgr_WakeupKeyCode_Param_t*) = &PowerManagerAPI::PLAT_DS_GetLastWakeupKeyCode;
DeepSleep_Return_Status_t (*PLAT_DS_TERM)() = &PowerManagerAPI::PLAT_DS_TERM;

