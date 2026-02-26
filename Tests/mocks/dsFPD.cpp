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

#include "dsFPD.h"
#include <gtest/gtest.h>

dsFPDImpl* dsFPDApi::impl = nullptr;

dsFPDApi::dsFPDApi() {}

void dsFPDApi::setImpl(dsFPDImpl* newImpl) {
  ASSERT_TRUE((nullptr == impl) || (nullptr == newImpl));
  impl = newImpl;
}

dsError_t dsFPDApi::dsFPInit() {
    EXPECT_NE(impl, nullptr);
    return impl->dsFPInit();
}

dsError_t dsFPDApi::dsSetFPBlink(dsFPDIndicator_t eIndicator, unsigned int uBlinkDuration, unsigned int uBlinkIterations) {
    EXPECT_NE(impl, nullptr);
    return impl->dsSetFPBlink(eIndicator, uBlinkDuration, uBlinkIterations);
}

dsError_t dsFPDApi::dsSetFPBrightness(dsFPDIndicator_t eIndicator, dsFPDBrightness_t eBrightness) {
    EXPECT_NE(impl, nullptr);
    return impl->dsSetFPBrightness(eIndicator, eBrightness);
}

dsError_t dsFPDApi::dsGetFPBrightness(dsFPDIndicator_t eIndicator, dsFPDBrightness_t* pBrightness) {
    EXPECT_NE(impl, nullptr);
    return impl->dsGetFPBrightness(eIndicator, pBrightness);
}

dsError_t dsFPDApi::dsSetFPState(dsFPDIndicator_t eIndicator, dsFPDState_t state) {
    EXPECT_NE(impl, nullptr);
    return impl->dsSetFPState(eIndicator, state);
}

dsError_t dsFPDApi::dsGetFPState(dsFPDIndicator_t eIndicator, dsFPDState_t* state) {
    EXPECT_NE(impl, nullptr);
    return impl->dsGetFPState(eIndicator, state);
}

dsError_t dsFPDApi::dsSetFPColor(dsFPDIndicator_t eIndicator, dsFPDColor_t eColor) {
    EXPECT_NE(impl, nullptr);
    return impl->dsSetFPColor(eIndicator, eColor);
}
dsError_t dsFPDApi::dsGetFPColor(dsFPDIndicator_t eIndicator, dsFPDColor_t* pColor) {
    EXPECT_NE(impl, nullptr);
    return impl->dsGetFPColor(eIndicator, pColor);
}

dsError_t dsFPDApi::dsFPTerm() {
    EXPECT_NE(impl, nullptr);
    return impl->dsFPTerm();
}

dsError_t dsFPDApi::dsFPGetLEDState(dsFPDLedState_t* state) {
    EXPECT_NE(impl, nullptr);
    return impl->dsFPGetLEDState(state);
}

dsError_t dsFPDApi::dsFPSetLEDState(dsFPDLedState_t state) {
    EXPECT_NE(impl, nullptr);
    return impl->dsFPSetLEDState(state);
}

dsError_t dsFPDApi::dsFPGetSupportedLEDStates(unsigned int* states) {
    EXPECT_NE(impl, nullptr);
    return impl->dsFPGetSupportedLEDStates(states);
}

dsError_t (*dsFPInit) () = &dsFPDApi::dsFPInit;
dsError_t (*dsSetFPBlink) (dsFPDIndicator_t, unsigned int, unsigned int) = &dsFPDApi::dsSetFPBlink;
dsError_t (*dsSetFPBrightness) (dsFPDIndicator_t, dsFPDBrightness_t) = &dsFPDApi::dsSetFPBrightness;
dsError_t (*dsGetFPBrightness) (dsFPDIndicator_t, dsFPDBrightness_t*) = &dsFPDApi::dsGetFPBrightness;
dsError_t (*dsSetFPState) (dsFPDIndicator_t, dsFPDState_t) = &dsFPDApi::dsSetFPState;
dsError_t (*dsGetFPState) (dsFPDIndicator_t, dsFPDState_t*) = &dsFPDApi::dsGetFPState;
dsError_t (*dsSetFPColor) (dsFPDIndicator_t, dsFPDColor_t) = &dsFPDApi::dsSetFPColor;
dsError_t (*dsGetFPColor) (dsFPDIndicator_t, dsFPDColor_t*) = &dsFPDApi::dsGetFPColor;
dsError_t (*dsFPTerm) () = &dsFPDApi::dsFPTerm;
dsError_t (*dsFPGetLEDState) (dsFPDLedState_t*) = &dsFPDApi::dsFPGetLEDState;
dsError_t (*dsFPSetLEDState) (dsFPDLedState_t) = &dsFPDApi::dsFPSetLEDState;
dsError_t (*dsFPGetSupportedLEDStates) (unsigned int*) = &dsFPDApi::dsFPGetSupportedLEDStates;
