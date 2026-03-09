/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 RDK Management
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

#include <com/com.h>

// ID_L2TEST_CONTROLLER is reserved in entservices-apis apis/Ids.h:
//   ID_L2TEST_CONTROLLER = ID_ENTOS_OFFSET + 0x270
// IL2Test.h has NOT been added to entservices-apis; it lives here locally.
#ifndef ID_L2TEST_CONTROLLER
#define ID_L2TEST_CONTROLLER (RPC::IDS::ID_EXTERNAL_CC_INTERFACE_OFFSET + 0x270)
#endif

namespace WPEFramework {
namespace Exchange {

/* @json 1.0.0 @text:keep */
struct EXTERNAL IL2Test : virtual public Core::IUnknown {

    enum { ID = ID_L2TEST_CONTROLLER };

    // @brief Run L2 Google Tests
    // @param parameters JSON string (optional), e.g. {"test_suite_list":"MySuite.*"}
    // @param response   JSON response string, e.g. {"status":0}
    virtual Core::hresult PerformL2Tests(const string& parameters, string& response /* @out */) = 0;
};

} // namespace Exchange
} // namespace WPEFramework
