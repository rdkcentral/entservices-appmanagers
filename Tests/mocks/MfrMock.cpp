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
#include "MfrMock.h"

mfrImpl* mfr::impl = nullptr;

mfr::mfr(){}

void mfr::setImpl(mfrImpl* newImpl) {
    EXPECT_TRUE ((nullptr == impl) || (nullptr == newImpl));
    impl = newImpl;
}

mfrError_t mfr::mfrGetTempThresholds(int*high, int*critical)
{
    return impl->mfrGetTempThresholds(high, critical);
}

mfrError_t mfr::mfrSetTempThresholds(int high, int critical)
{
    return impl->mfrSetTempThresholds(high, critical);
}

mfrError_t mfr::mfrGetTemperature(mfrTemperatureState_t *curState, int *curTemperature, int *wifiTemperature)
{
    return impl->mfrGetTemperature(curState, curTemperature, wifiTemperature);
}

mfrError_t (*mfrGetTempThresholds)(int *tempHigh, int *tempCritical) = &mfr::mfrGetTempThresholds;
mfrError_t (*mfrSetTempThresholds)(int tempHigh, int tempCritical) = &mfr::mfrSetTempThresholds;
mfrError_t (*mfrGetTemperature)(mfrTemperatureState_t *state, int *, int *) = &mfr::mfrGetTemperature;
