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

#include "WindowManagerCapabilities.h"
#include <sstream>

namespace WPEFramework {
namespace Plugin {

std::string buildWindowManagerCapabilities(const std::string& runtimeCapabilities)
{
    std::string result;
    std::istringstream stream(runtimeCapabilities);
    std::string token;

    while (std::getline(stream, token, ','))
    {
        // capability name is the part before '=' (handles "name=value" entries)
        const std::string capName = token.substr(0, token.find('='));

        // Mirrors IPackage::Capability checks from GuiAppManagerV2::doLaunchApp:

        if (capName == "home-app")              // IPackage::Capability::HomeApp / CompositorApp
        {
            if (!result.empty()) result += ',';
            result += "homeapp";
        }
        else if (capName == "issue-notifications")  // IPackage::Capability::DisplayOverlays
        {
            if (!result.empty()) result += ',';
            result += "overlayenabled";
        }
        else if (capName == "suspend-mode")     // IPackage::Capability::Suspendable
        {
            if (!result.empty()) result += ',';
            result += "suspendable";
        }
        else if (capName == "refresh-rate-60hz")    // IPackage::Capability::SixtyHertz
        {
            if (!result.empty()) result += ',';
            result += "60hz";
        }
        else if (capName == "mediarite-underlay")   // IPackage::Capability::RequiresMediarite
        {
            if (!result.empty()) result += ',';
            result += "mediarite";
        }
        else if (capName == "game-controller")  // IPackage::Capability::WantsGameController
        {
            if (!result.empty()) result += ',';
            result += "gamecontrollersupport";
        }
    }

    return result;
}

} // namespace Plugin
} // namespace WPEFramework
