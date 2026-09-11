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
#include <vector>

/**
 * @file NetworkConfigurationHelper.h
 * @brief Helpers for merging OCI network metadata and generating Dobby networking plugin configuration.
 */
namespace NetworkConfigurationHelper
{

/**
 * @brief Merges urn:rdk:config:network metadata into OCI root networking data.
 * @param[in,out] ociConfigRootNode The root node of the OCI configuration JSON.
 * @param[in] manifestRootNode The root node of the manifest JSON.
 * @return true if the update was successful, false otherwise.
 */
bool updateNetworkConfigurationNode(Json::Value& ociConfigRootNode, const Json::Value& manifestRootNode);

/**
 * @brief Updates the OCI configuration with network settings based on permissions specified in the manifest.
 *
 * @param[in,out] ociConfigRootNode The root node of the OCI configuration JSON.
 * @param[in] manifestRootNode The root node of the manifest JSON.
 * @param[in] envVariables The serialized JSON array string of environment variables as provided by RuntimeConfig.envVariables.
 * @return true if the update was successful or if there were no permissions to process; false on error.
 */
bool updatePermissionBasedNetworkConfiguration(Json::Value& ociConfigRootNode, const Json::Value& manifestRootNode,
                                               const std::string& envVariables);

} // namespace NetworkConfigurationHelper
