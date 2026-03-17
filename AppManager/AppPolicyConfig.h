/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2024 RDK Management
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

#include <set>
#include <string>

namespace WPEFramework {
namespace Plugin {

    /**
     * @brief Temporary hard-coded app policy helpers.
     *
     * TODO: Remove this class once proper per-app configuration
     *       is provided by the system (e.g. app manifest / config store).
     */
    class AppPolicyConfig {
    public:
        static bool IsAppSuspendable(const std::string& appId)
        {
            return true; //TODO: For now return true for all apps
            // static const std::set<std::string> suspendableApps = {
            //     "Netflix", "YouTube", "AppleTV", "Xumo", "PrimeVideo"
            // };
            // return suspendableApps.find(appId) != suspendableApps.end();
        }

        static bool IsAppHibernatable(const std::string& appId)
        {
            static const std::set<std::string> hibernatableApps = {
                "Netflix", "YouTube", "xumo", "apple.tv"
            };
            return hibernatableApps.find(appId) != hibernatableApps.end();
        }

        static int GetHibernateDelayTimeSeconds(const std::string& appId)
        {
            // For demonstration, return 30 seconds for all apps - sky AS default
            // In a real implementation, this could be based on app-specific policies.
            return 30;
        }

    private:
        AppPolicyConfig() = delete;
    };

} /* namespace Plugin */
} /* namespace WPEFramework */
