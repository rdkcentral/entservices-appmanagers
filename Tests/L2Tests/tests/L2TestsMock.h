/* If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2023 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
*/

#pragma once

#include <websocket/JSONRPCLink.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "UtilsJsonRpc.h"

// Only include mocks available in entservices-appmanagers
#include "WrapsMock.h"
#include "TelemetryMock.h"



using ::testing::NiceMock;
using namespace WPEFramework;

class L2TestMocks : public ::testing::Test {
protected:
        // Mocks available in entservices-appmanagers
        WrapsImplMock        *p_wrapsImplMock        = nullptr;
        TelemetryApiImplMock *p_telemetryApiImplMock = nullptr;

        std::string thunder_address;

        L2TestMocks();
        virtual ~L2TestMocks();

       /**
         * @brief Invoke a service method
         *
         * @param[in] callsign Service callsign
         * @param[in] method Method name
         * @param[in] params Method parameters
         * @param[out] results Method results
         * @return Zero (Core::ERROR_NONE) on succes or another value on error
         */
        uint32_t InvokeServiceMethod(const char *callsign, const char *method, JsonObject &params, JsonObject &results);

        /**
         * @brief Invoke a service method (jsonObject Return Type)
         *
         * @param[in] callsign Service callsign
         * @param[in] method Method name
         * @param[out] results Method results (JsonObject)
         * @return Zero (Core::ERROR_NONE) on success or another value on error
         */
        uint32_t InvokeServiceMethod(const char *callsign, const char *method, JsonObject &results);

       /**
        * @brief Invoke a service method
        *
        * @param[in] callsign Service callsign
        * @param[in] method Method name
        * @param[in] params Method parameters
        * @param[out] results Method results with string format
        * @return Zero (Core::ERROR_NONE) on succes or another value on error
        */
        uint32_t InvokeServiceMethod(const char *callsign, const char *method, JsonObject &params, Core::JSON::String &results);

        /**
        * @brief Invoke a service method
        *
        * @param[in] callsign Service callsign
        * @param[in] method Method name
        * @param[in] params Method parameters
        * @param[out] results Method results with string format
        * @return Zero (Core::ERROR_NONE) on succes or another value on error
        */
        uint32_t InvokeServiceMethod(const char *callsign, const char *method, JsonObject &params, Core::JSON::Boolean &results);

       /**
         * @brief Invoke a service method
         *
         * @param[in] callsign Service callsign
         * @param[in] method Method name
         * @param[out] results Method results
         * @return Zero (Core::ERROR_NONE) on succes or another value on error
         */
        uint32_t InvokeServiceMethod(const char *callsign, const char *method, Core::JSON::Boolean &results);

       /**
         * @brief Invoke a service method
         *
         * @param[in] callsign Service callsign
         * @param[in] method Method name
         * @param[out] results Method results
         * @return Zero (Core::ERROR_NONE) on succes or another value on error
         */
        uint32_t InvokeServiceMethod(const char *callsign, const char *method, Core::JSON::String &results);

        /**
          * @brief Invoke a service method
          *
          * @param[in] callsign Service callsign
          * @param[in] method Method name
          * @param[out] results Method results
          * @return Zero (Core::ERROR_NONE) on succes or another value on error
          */
          uint32_t InvokeServiceMethod(const char *callsign, const char *method, Core::JSON::Double &results);

        /**
         * @brief Activate a service plugin
         *
         * @param[in] callsign Service callsign
         * @return Zero (Core::ERROR_NONE) on succes or another value on error
         */
        uint32_t ActivateService(const char *callsign);

        /**
         * @brief Deactivate a service plugin
         *
         * @param[in] callsign Service callsign
         * @return Zero (Core::ERROR_NONE) on succes or another value on error
         */
        uint32_t DeactivateService(const char *callsign);

};

