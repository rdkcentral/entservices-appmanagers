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

#include "dsFPD.h"
#include <gmock/gmock.h>

class dsFPDMock : public dsFPDImpl {
public:
    dsFPDMock()
        : dsFPDImpl() {}
    virtual ~dsFPDMock() = default;

    MOCK_METHOD(dsError_t, dsFPInit, (), (override));
    MOCK_METHOD(dsError_t, dsSetFPBlink, (dsFPDIndicator_t eIndicator, unsigned int uBlinkDuration, unsigned int uBlinkIterations), (override));
    MOCK_METHOD(dsError_t, dsSetFPBrightness, (dsFPDIndicator_t eIndicator, dsFPDBrightness_t eBrightness), (override));
    MOCK_METHOD(dsError_t, dsGetFPBrightness, (dsFPDIndicator_t eIndicator, dsFPDBrightness_t* pBrightness), (override));
    MOCK_METHOD(dsError_t, dsSetFPState, (dsFPDIndicator_t eIndicator, dsFPDState_t state), (override));
    MOCK_METHOD(dsError_t, dsGetFPState, (dsFPDIndicator_t eIndicator, dsFPDState_t* state), (override));
    MOCK_METHOD(dsError_t, dsSetFPColor, (dsFPDIndicator_t eIndicator, dsFPDColor_t eColor), (override));
    MOCK_METHOD(dsError_t, dsGetFPColor, (dsFPDIndicator_t eIndicator, dsFPDColor_t* pColor), (override));
    MOCK_METHOD(dsError_t, dsFPTerm, (), (override));
    MOCK_METHOD(dsError_t, dsFPGetLEDState, (dsFPDLedState_t* state), (override));
    MOCK_METHOD(dsError_t, dsFPSetLEDState, (dsFPDLedState_t state), (override));
    MOCK_METHOD(dsError_t, dsFPGetSupportedLEDStates, (unsigned int* states), (override));

};
