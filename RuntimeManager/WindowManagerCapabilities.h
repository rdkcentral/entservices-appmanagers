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

#include <string>

namespace WPEFramework {
namespace Plugin {

/**
 * @brief Derives window-manager capability tokens from the full runtime capabilities string.
 *
 * Mirrors the IPackage::Capability checks from GuiAppManagerV2::doLaunchApp:
 *   "home-app"           (IPackage::Capability::HomeApp / CompositorApp) -> "homeapp"
 *   "issue-notifications"(IPackage::Capability::DisplayOverlays)         -> "overlayenabled"
 *   "suspend-mode"       (IPackage::Capability::Suspendable)             -> "suspendable"
 *   "refresh-rate-60hz"  (IPackage::Capability::SixtyHertz)              -> "60hz"
 *   "mediarite-underlay" (IPackage::Capability::RequiresMediarite)       -> "mediarite"
 *   "game-controller"    (IPackage::Capability::WantsGameController)     -> "gamecontrollersupport"
 *
 * @param runtimeCapabilities  Comma-separated runtime capability tokens from RuntimeConfig.
 * @return Comma-separated window-manager capability tokens (may be empty).
 */
std::string buildWindowManagerCapabilities(const std::string& runtimeCapabilities);

} // namespace Plugin
} // namespace WPEFramework
