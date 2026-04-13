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
// ID_L2TEST_CONTROLLER is defined in entservices-apis apis/Ids.h (topic/rdkemw-13235+):
//   ID_L2TEST_CONTROLLER = ID_ENTOS_OFFSET + 0x270
// Include interfaces/Ids.h to get the enum member directly.
// DO NOT #define it as a macro — that would corrupt the enum in Ids.h
// when Ids.h is included later in the same translation unit.
#include <interfaces/Ids.h>

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
