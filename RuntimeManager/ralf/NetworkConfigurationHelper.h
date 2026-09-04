/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

#pragma once

#include <json/json.h>
#include <string>

/**
 * @file NetworkConfigurationHelper.h
 * @brief Helpers for merging OCI network metadata and generating Dobby networking plugin configuration.
 */
namespace NetworkConfigurationHelper
{

/**
 * @brief Merges urn:rdk:config:network metadata into the temporary _temp_ralf_nwcfg.network store within the OCI configuration.
 * Entries are keyed by the network service name. Existing entries are replaced and new entries are added.
 *
 * @param[in,out] ociConfigRootNode Root OCI configuration.
 * @param[in] manifestRootNode Package metadata manifest.
 * @return true on success.
 */
bool updateNetworkConfiguration(Json::Value& ociConfigRootNode, const Json::Value& manifestRootNode);

/**
 * @brief Generates Dobby networking plugin configuration from the temporary _temp_ralf_nwcfg.network store.
 * Generated plugin is written to: rdkPlugins.networking.
 * After successful generation temporary _temp_ralf_nwcfg node is removed.
 *
 * @param[in,out] ociConfigRootNode Root OCI configuration.
 * @return true on success.
 */
bool generateNetworkingPlugin(Json::Value& ociConfigRootNode);

/**
 * @brief Applies runtime-driven network policy to rdkPlugins.networking.data and process capabilities.
 *
 * This handles default networking mode (nat/none), dnsmasq flag, required capabilities,
 * and optional host-system mounts for networking support.
 *
 * @param[in,out] ociConfigRootNode Root OCI configuration.
 * @param[in] networkEnabled Whether networking should be enabled for this app.
 * @param[in] configFilePath Output OCI config file path; used to resolve the generated rootfs location.
 * @return true on success.
 */
bool applyRuntimeNetworkingConfiguration(Json::Value& ociConfigRootNode, bool networkEnabled, const std::string& configFilePath);

/**
 * @brief Checks whether a specific permission URN is present in a comma-separated capabilities string.
 * @param[in] capabilities Comma-separated capabilities string.
 * @param[in] permission Exact permission URN to search for.
 * @return true if permission is present, false otherwise.
 */
bool hasCapabilityPermission(const std::string& capabilities, const std::string& permission);

} // namespace NetworkConfigurationHelper
