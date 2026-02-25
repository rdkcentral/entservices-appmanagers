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

#ifndef L2TEST_CONTROLLER_H
#define L2TEST_CONTROLLER_H
#include <interfaces/IL2Test.h>

using namespace WPEFramework;

class L2testController
{
    private:
        static L2testController* instancePtr;
        L2testController() = default;
        ~L2testController() = default;

        /** @brief Pipe file to send commands to the WPEFramework process. */
        Core::ProxyType<RPC::CommunicatorClient> m_client;
        /** @brief Plugin shell interface for accessing L2Tests plugin. */
        PluginHost::IShell* m_pluginShell = nullptr;
        /** @brief COM-RPC interface for executing L2 test operations. */
        Exchange::IL2Test* m_l2TestInterface = nullptr;

        FILE* m_fp = nullptr; // Thunder process

   public:
       // deleting copy constructor
         L2testController(const L2testController& obj) = delete;
         L2testController& operator=(const L2testController&) = delete;

        /**
         * @brief Getting the class instance
         *
         */
        static L2testController* getInstance();

        /**
         * @brief Spawn Thunder process in isolation
         *
         */
        bool StartThunder();

        /**
         * @brief Stops thunder
         *
         */
        void StopThunder();

        /**
         * @brief Perform L2 test, calls first method of L2Tests plugin
         *
         * @param[in] method Method name
         * @param[in] params Method parameters
         * @param[out] results Method results
         * @return Zero (Core::ERROR_NONE) on success or another value on error
         */
       Core::hresult PerformL2Tests(const std::string& params, std::string& response);

       /**
         * @brief Initialize COM-RPC client
         */
       void initClient();

       /**
         * @brief Release COM-RPC client
         */
       void releaseClient();

};

#endif /* L2TEST_CONTROLLER_H */