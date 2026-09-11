/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2026 RDK Management
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
 **/

/**
 * @file NetworkConfigurationHelperTests.cpp
 *
 * L0 tests for RuntimeManager/ralf/NetworkConfigurationHelper:
 *   - updateNetworkConfigurationNode()
 *   - updatePermissionBasedNetworkConfiguration()
 */

#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include "ralf/NetworkConfigurationHelper.h"
#include "ralf/OCISpecConstants.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

uint32_t Test_NetworkConfigurationHelper_UpdateNetworkConfigurationNode_MapsAndDeduplicatesRules()
{
    L0Test::TestResult tr;

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    Json::Value networkArray(Json::arrayValue);

    Json::Value publicEntry(Json::objectValue);
    publicEntry[ralf::NAME] = "svc-public";
    publicEntry[ralf::PORT] = 8080;
    publicEntry[ralf::PROTOCOL] = "tcp";
    publicEntry[ralf::TYPE] = "public";

    Json::Value exportedEntry(Json::objectValue);
    exportedEntry[ralf::NAME] = "svc-exported";
    exportedEntry[ralf::PORT] = 5001;
    exportedEntry[ralf::PROTOCOL] = "udp";
    exportedEntry[ralf::TYPE] = "exported";

    Json::Value importedEntry(Json::objectValue);
    importedEntry[ralf::NAME] = "svc-imported";
    importedEntry[ralf::PORT] = 5002;
    importedEntry[ralf::PROTOCOL] = "tcp";
    importedEntry[ralf::TYPE] = "imported";

    // Duplicate of public entry to validate dedupe in output rule array.
    Json::Value duplicatePublic(publicEntry);

    networkArray.append(publicEntry);
    networkArray.append(exportedEntry);
    networkArray.append(importedEntry);
    networkArray.append(duplicatePublic);

    manifestRootNode[ralf::CONFIGURATION][ralf::NETWORK_CONFIG_URN] = networkArray;

    const bool status = NetworkConfigurationHelper::updateNetworkConfigurationNode(ociConfigRootNode, manifestRootNode);
    L0Test::ExpectTrue(tr, status,
                       "updateNetworkConfigurationNode() returns true for valid network rules");

    const Json::Value& networkingData = ociConfigRootNode[ralf::RDKPLUGINS]["networking"][ralf::DATA];
    L0Test::ExpectTrue(tr, networkingData.isObject(), "Networking data node exists");

    const Json::Value& hostToContainer = networkingData["portForwarding"]["hostToContainer"];
    L0Test::ExpectTrue(tr, hostToContainer.isArray() && hostToContainer.size() == 1u,
                       "Duplicate public entries are deduplicated to one hostToContainer rule");
    if (hostToContainer.isArray() && hostToContainer.size() == 1u)
    {
        L0Test::ExpectEqU32(tr, hostToContainer[0][ralf::PORT].asUInt(), 8080u,
                            "Public rule port is preserved");
    }

    const Json::Value& interContainer = networkingData["interContainer"];
    L0Test::ExpectTrue(tr, interContainer.isArray() && interContainer.size() == 2u,
                       "Exported and imported map to two interContainer rules");
    if (interContainer.isArray() && interContainer.size() == 2u)
    {
        L0Test::ExpectEqStr(tr, interContainer[0]["direction"].asString(), "in",
                            "Exported maps to direction in");
        L0Test::ExpectEqStr(tr, interContainer[1]["direction"].asString(), "out",
                            "Imported maps to direction out");
    }

    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_UpdateNetworkConfigurationNode_IgnoresMalformedEntries()
{
    L0Test::TestResult tr;

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    Json::Value networkArray(Json::arrayValue);

    // --- NEGATIVE SCENARIO: Malformed non-object JSON entry array types ---
    networkArray.append("invalid-entry-string");
    networkArray.append(42);

    Json::Value validEntry(Json::objectValue);
    validEntry[ralf::NAME] = "svc-valid";
    validEntry[ralf::PORT] = 7777;
    validEntry[ralf::PROTOCOL] = "tcp";
    validEntry[ralf::TYPE] = "public";
    networkArray.append(validEntry);

    manifestRootNode[ralf::CONFIGURATION][ralf::NETWORK_CONFIG_URN] = networkArray;

    const bool status = NetworkConfigurationHelper::updateNetworkConfigurationNode(ociConfigRootNode, manifestRootNode);
    L0Test::ExpectTrue(tr, status,
                       "updateNetworkConfigurationNode() returns true even when malformed entries are present");

    const Json::Value& hostToContainer =
        ociConfigRootNode[ralf::RDKPLUGINS]["networking"][ralf::DATA]["portForwarding"]["hostToContainer"];
    L0Test::ExpectTrue(tr, hostToContainer.isArray() && hostToContainer.size() == 1u,
                       "Only valid entry is translated to hostToContainer");
    if (hostToContainer.isArray() && hostToContainer.size() == 1u)
    {
        L0Test::ExpectEqU32(tr, hostToContainer[0][ralf::PORT].asUInt(), 7777u,
                            "Valid entry port is preserved");
    }

    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_UpdateNetworkConfigurationNode_MissingConfigurationReturnsTrue()
{
    L0Test::TestResult tr;

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);

    const bool status = NetworkConfigurationHelper::updateNetworkConfigurationNode(ociConfigRootNode, manifestRootNode);
    L0Test::ExpectTrue(tr, status,
                       "updateNetworkConfigurationNode() returns true when configuration node is absent");
    L0Test::ExpectTrue(tr, !ociConfigRootNode.isMember(ralf::RDKPLUGINS),
                       "No networking node is created when configuration node is absent");

    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_UpdateNetworkConfigurationNode_NetworkConfigNotArrayReturnsFalse()
{
    L0Test::TestResult tr;

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    manifestRootNode[ralf::CONFIGURATION][ralf::NETWORK_CONFIG_URN] = Json::Value(Json::objectValue);

    const bool status = NetworkConfigurationHelper::updateNetworkConfigurationNode(ociConfigRootNode, manifestRootNode);
    L0Test::ExpectTrue(tr, !status,
                       "updateNetworkConfigurationNode() returns false when network configuration is not an array");

    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_UpdatePermissionBasedNetworkConfiguration_SetsNatAndIpFlags()
{
    L0Test::TestResult tr;

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    manifestRootNode[ralf::PERMISSIONS] = Json::Value(Json::arrayValue);
    manifestRootNode[ralf::PERMISSIONS].append(ralf::PERMISSION_FIREBOLT);

    const std::string envVariables = "[\"FIREBOLT_ENDPOINT=ws://127.0.0.1:3473\"]";

    const bool status = NetworkConfigurationHelper::updatePermissionBasedNetworkConfiguration(
        ociConfigRootNode, manifestRootNode, envVariables);
    L0Test::ExpectTrue(tr, status,
                       "updatePermissionBasedNetworkConfiguration() returns true for valid Firebolt loopback endpoint");

    const Json::Value& networkingData = ociConfigRootNode[ralf::RDKPLUGINS]["networking"][ralf::DATA];
    L0Test::ExpectEqStr(tr, networkingData[ralf::TYPE].asString(), "nat",
                        "Network type is forced to nat when permission-based networking is applied");
    L0Test::ExpectTrue(tr, networkingData["dnsmasq"].asBool(), "dnsmasq is enabled for permission-based networking");
    L0Test::ExpectTrue(tr, networkingData["ipv4"].asBool(), "ipv4 is enabled for permission-based networking");
    L0Test::ExpectTrue(tr, networkingData["ipv6"].asBool(), "ipv6 is enabled for permission-based networking");

    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_UpdatePermissionBasedNetworkConfiguration_MissingPermissionsReturnsTrue()
{
    L0Test::TestResult tr;

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);

    const bool status = NetworkConfigurationHelper::updatePermissionBasedNetworkConfiguration(
        ociConfigRootNode, manifestRootNode, std::string());
    L0Test::ExpectTrue(tr, status,
                       "updatePermissionBasedNetworkConfiguration() returns true when permissions node is absent");
    L0Test::ExpectTrue(tr, !ociConfigRootNode.isMember(ralf::RDKPLUGINS),
                       "No networking node is created when permissions node is absent");

    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_UpdatePermissionBasedNetworkConfiguration_NonArrayPermissionsReturnsTrue()
{
    L0Test::TestResult tr;

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    manifestRootNode[ralf::PERMISSIONS] = "urn:rdk:permission:firebolt";

    const bool status = NetworkConfigurationHelper::updatePermissionBasedNetworkConfiguration(
        ociConfigRootNode, manifestRootNode, std::string());
    L0Test::ExpectTrue(tr, status,
                       "updatePermissionBasedNetworkConfiguration() returns true when permissions node is not an array");
    L0Test::ExpectTrue(tr, !ociConfigRootNode.isMember(ralf::RDKPLUGINS),
                       "No networking node is created when permissions node is not an array");

    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_UpdatePermissionBasedNetworkConfiguration_ParsesThunderHostPort()
{
    L0Test::TestResult tr;

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    manifestRootNode[ralf::PERMISSIONS] = Json::Value(Json::arrayValue);
    manifestRootNode[ralf::PERMISSIONS].append(ralf::PERMISSION_THUNDER);

    setenv(ralf::THUNDER_ACCESS_ENV_KEY, "127.0.0.1:9998", 1);

    const bool status = NetworkConfigurationHelper::updatePermissionBasedNetworkConfiguration(
        ociConfigRootNode, manifestRootNode, std::string());
    L0Test::ExpectTrue(tr, status,
                       "updatePermissionBasedNetworkConfiguration() returns true for THUNDER_ACCESS host:port form");

    const Json::Value& containerToHost =
        ociConfigRootNode[ralf::RDKPLUGINS]["networking"][ralf::DATA]["portForwarding"]["containerToHost"];
    L0Test::ExpectTrue(tr, containerToHost.isArray() && containerToHost.size() == 1u,
                       "Thunder loopback endpoint generates one containerToHost rule");
    if (containerToHost.isArray() && containerToHost.size() == 1u)
    {
        L0Test::ExpectEqU32(tr, containerToHost[0][ralf::PORT].asUInt(), 9998u,
                            "Thunder host:port endpoint port is parsed correctly");
        L0Test::ExpectEqStr(tr, containerToHost[0][ralf::PROTOCOL].asString(), "tcp",
                            "Thunder host:port endpoint defaults to tcp protocol");
    }

    unsetenv(ralf::THUNDER_ACCESS_ENV_KEY);

    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_UpdatePermissionBasedNetworkConfiguration_ThunderInvalidPortReturnsFalse()
{
    L0Test::TestResult tr;

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    manifestRootNode[ralf::PERMISSIONS] = Json::Value(Json::arrayValue);
    manifestRootNode[ralf::PERMISSIONS].append(ralf::PERMISSION_THUNDER);

    setenv(ralf::THUNDER_ACCESS_ENV_KEY, "127.0.0.1:abc", 1);

    const bool status = NetworkConfigurationHelper::updatePermissionBasedNetworkConfiguration(
        ociConfigRootNode, manifestRootNode, std::string());
    L0Test::ExpectTrue(tr, !status,
                       "updatePermissionBasedNetworkConfiguration() returns false for invalid Thunder endpoint when THUNDER permission is requested");

    const Json::Value& networkingData = ociConfigRootNode[ralf::RDKPLUGINS]["networking"][ralf::DATA];
    L0Test::ExpectTrue(tr, !networkingData.isMember("portForwarding") ||
                           !networkingData["portForwarding"].isMember("containerToHost"),
                       "No containerToHost rule is created for invalid Thunder endpoint");

    unsetenv(ralf::THUNDER_ACCESS_ENV_KEY);

    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_UpdatePermissionBasedNetworkConfiguration_FireboltMissingEnvReturnsFalse()
{
    L0Test::TestResult tr;

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    manifestRootNode[ralf::PERMISSIONS] = Json::Value(Json::arrayValue);
    manifestRootNode[ralf::PERMISSIONS].append(ralf::PERMISSION_FIREBOLT);

    const bool status = NetworkConfigurationHelper::updatePermissionBasedNetworkConfiguration(
        ociConfigRootNode, manifestRootNode, std::string());
    L0Test::ExpectTrue(tr, !status,
                       "updatePermissionBasedNetworkConfiguration() returns false when FIREBOLT permission is requested but endpoint is absent");

    const Json::Value& networkingData = ociConfigRootNode[ralf::RDKPLUGINS]["networking"][ralf::DATA];
    L0Test::ExpectTrue(tr, !networkingData.isMember("portForwarding") ||
                           !networkingData["portForwarding"].isMember("containerToHost"),
                       "No containerToHost rule is created when FIREBOLT endpoint is absent");

    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_UpdatePermissionBasedNetworkConfiguration_SkipsDuplicateFireboltRule()
{
    L0Test::TestResult tr;

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    manifestRootNode[ralf::PERMISSIONS] = Json::Value(Json::arrayValue);
    manifestRootNode[ralf::PERMISSIONS].append(ralf::PERMISSION_FIREBOLT);

    Json::Value existingRule(Json::objectValue);
    existingRule[ralf::PORT] = 3473;
    existingRule[ralf::PROTOCOL] = "ws";
    ociConfigRootNode[ralf::RDKPLUGINS]["networking"][ralf::DATA]["portForwarding"]["containerToHost"].append(existingRule);

    const std::string envVariables = "[\"FIREBOLT_ENDPOINT=ws://127.0.0.1:3473\"]";
    const bool status = NetworkConfigurationHelper::updatePermissionBasedNetworkConfiguration(
        ociConfigRootNode, manifestRootNode, envVariables);
    L0Test::ExpectTrue(tr, status,
                       "updatePermissionBasedNetworkConfiguration() returns true when FIREBOLT rule already exists");

    const Json::Value& containerToHost =
        ociConfigRootNode[ralf::RDKPLUGINS]["networking"][ralf::DATA]["portForwarding"]["containerToHost"];
    L0Test::ExpectTrue(tr, containerToHost.isArray() && containerToHost.size() == 1u,
                       "Duplicate FIREBOLT rule is skipped and existing containerToHost entry is retained");

    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_UpdatePermissionBasedNetworkConfiguration_SkipsAllDuplicateRulesAndReturnsTrue()
{
    L0Test::TestResult tr;

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    manifestRootNode[ralf::PERMISSIONS] = Json::Value(Json::arrayValue);
    manifestRootNode[ralf::PERMISSIONS].append(ralf::PERMISSION_FIREBOLT);
    manifestRootNode[ralf::PERMISSIONS].append(ralf::PERMISSION_THUNDER);

    Json::Value existingFireboltRule(Json::objectValue);
    existingFireboltRule[ralf::PORT] = 3473;
    existingFireboltRule[ralf::PROTOCOL] = "ws";
    ociConfigRootNode[ralf::RDKPLUGINS]["networking"][ralf::DATA]["portForwarding"]["containerToHost"].append(existingFireboltRule);

    Json::Value existingThunderRule(Json::objectValue);
    existingThunderRule[ralf::PORT] = 9998;
    existingThunderRule[ralf::PROTOCOL] = "tcp";
    ociConfigRootNode[ralf::RDKPLUGINS]["networking"][ralf::DATA]["portForwarding"]["containerToHost"].append(existingThunderRule);

    const std::string envVariables = "[\"FIREBOLT_ENDPOINT=ws://127.0.0.1:3473\"]";
    setenv(ralf::THUNDER_ACCESS_ENV_KEY, "127.0.0.1:9998", 1);

    const bool status = NetworkConfigurationHelper::updatePermissionBasedNetworkConfiguration(
        ociConfigRootNode, manifestRootNode, envVariables);
    L0Test::ExpectTrue(tr, status,
                       "updatePermissionBasedNetworkConfiguration() returns true when all permission-derived rules already exist");

    const Json::Value& containerToHost =
        ociConfigRootNode[ralf::RDKPLUGINS]["networking"][ralf::DATA]["portForwarding"]["containerToHost"];
    L0Test::ExpectTrue(tr, containerToHost.isArray() && containerToHost.size() == 2u,
                       "No duplicate entries are appended when FIREBOLT and THUNDER rules already exist");

    unsetenv(ralf::THUNDER_ACCESS_ENV_KEY);
    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_UpdateNetworkConfigurationNode_GeneratesNetworkingPluginNode()
{
    L0Test::TestResult tr;
    Json::Value ociConfigRootNode(Json::objectValue);

    Json::Value manifestRootNode(Json::objectValue);
    Json::Value networkArray(Json::arrayValue);

    Json::Value entryPublic(Json::objectValue);
    entryPublic[ralf::PORT] = 8080;
    entryPublic[ralf::PROTOCOL] = "tcp";
    entryPublic[ralf::TYPE] = "public";

    Json::Value entryExported(Json::objectValue);
    entryExported[ralf::PORT] = 5001;
    entryExported[ralf::PROTOCOL] = "udp";
    entryExported[ralf::TYPE] = "exported";

    Json::Value entryImported(Json::objectValue);
    entryImported[ralf::PORT] = 5002;
    entryImported[ralf::PROTOCOL] = "tcp";
    entryImported[ralf::TYPE] = "imported";

    Json::Value entryMalformed(Json::objectValue);
    entryMalformed[ralf::PROTOCOL] = "tcp";
    entryMalformed[ralf::TYPE] = "public";

    networkArray.append(entryPublic);
    networkArray.append(entryExported);
    networkArray.append(entryImported);
    networkArray.append(entryMalformed);
    manifestRootNode[ralf::CONFIGURATION][ralf::NETWORK_CONFIG_URN] = networkArray;

    const bool status = NetworkConfigurationHelper::updateNetworkConfigurationNode(ociConfigRootNode, manifestRootNode);
    L0Test::ExpectTrue(tr, status,
                       "updateNetworkConfigurationNode() returns true for mixed valid and malformed network rules");

    const Json::Value& pluginNode = ociConfigRootNode[ralf::RDKPLUGINS]["networking"];
    L0Test::ExpectTrue(tr, pluginNode.isObject(),
                       "networking plugin node is generated under rdkPlugins.networking");
    L0Test::ExpectTrue(tr, pluginNode["required"].asBool(),
                       "generated networking plugin is marked as required");

    const Json::Value& hostToContainer = pluginNode[ralf::DATA]["portForwarding"]["hostToContainer"];
    L0Test::ExpectTrue(tr, hostToContainer.isArray() && hostToContainer.size() == 1u,
                       "Valid public service generates one hostToContainer rule and malformed node is skipped");
    if (hostToContainer.isArray() && !hostToContainer.empty()) {
        L0Test::ExpectEqU32(tr, hostToContainer[0][ralf::PORT].asUInt(), 8080u, "Public service port is verified");
    }

    const Json::Value& interContainer = pluginNode[ralf::DATA]["interContainer"];
    L0Test::ExpectTrue(tr, interContainer.isArray() && interContainer.size() == 2u,
                       "Generates exported and imported directional interContainer rules");

    if (interContainer.isArray() && interContainer.size() == 2u) {
        L0Test::ExpectEqStr(tr, interContainer[0]["direction"].asString(), "in", "Exported types cleanly map to directional rule 'in'");
        L0Test::ExpectEqStr(tr, interContainer[1]["direction"].asString(), "out", "Imported types cleanly map to directional rule 'out'");
    }

    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_UpdatePermissionBasedNetworkConfiguration_FireboltAndThunderEnableNetworkDefaults()
{
    L0Test::TestResult tr;
    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    manifestRootNode[ralf::PERMISSIONS] = Json::Value(Json::arrayValue);
    manifestRootNode[ralf::PERMISSIONS].append(ralf::PERMISSION_FIREBOLT);
    manifestRootNode[ralf::PERMISSIONS].append(ralf::PERMISSION_THUNDER);

    const std::string envVariables = "[\"FIREBOLT_ENDPOINT=ws://127.0.0.1:3473\"]";
    setenv(ralf::THUNDER_ACCESS_ENV_KEY, "127.0.0.1:9998", 1);

    const bool status = NetworkConfigurationHelper::updatePermissionBasedNetworkConfiguration(
        ociConfigRootNode, manifestRootNode, envVariables);

    L0Test::ExpectTrue(tr, status,
                       "updatePermissionBasedNetworkConfiguration() returns true when Firebolt and Thunder loopback endpoints are valid");

    const Json::Value& netData = ociConfigRootNode[ralf::RDKPLUGINS]["networking"][ralf::DATA];
    L0Test::ExpectEqStr(tr, netData[ralf::TYPE].asString(), "nat", "network type remains nat");
    L0Test::ExpectTrue(tr, netData["dnsmasq"].asBool(), "dnsmasq remains enabled");
    L0Test::ExpectTrue(tr, netData["ipv4"].asBool(), "ipv4 remains enabled");
    L0Test::ExpectTrue(tr, netData["ipv6"].asBool(), "ipv6 remains enabled");

    unsetenv(ralf::THUNDER_ACCESS_ENV_KEY);

    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_UpdatePermissionBasedNetworkConfiguration_UsesContainerToHostForLoopbackEndpoints()
{
    L0Test::TestResult tr;

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    manifestRootNode[ralf::PERMISSIONS] = Json::Value(Json::arrayValue);
    manifestRootNode[ralf::PERMISSIONS].append(ralf::PERMISSION_FIREBOLT);
    manifestRootNode[ralf::PERMISSIONS].append(ralf::PERMISSION_THUNDER);

    const std::string envVariables = "[\"FIREBOLT_ENDPOINT=ws://127.0.0.1:3473\",\"TARGET_STATE=4\"]";
    setenv(ralf::THUNDER_ACCESS_ENV_KEY, "127.0.0.1:9998", 1);

    const bool status = NetworkConfigurationHelper::updatePermissionBasedNetworkConfiguration(
        ociConfigRootNode, manifestRootNode, envVariables);
    L0Test::ExpectTrue(tr, status, "updatePermissionBasedNetworkConfiguration() returns true for loopback host permissions");

    const Json::Value& networking = ociConfigRootNode[ralf::RDKPLUGINS]["networking"][ralf::DATA];
    L0Test::ExpectTrue(tr, networking["portForwarding"]["containerToHost"].isArray(), "loopback host ports are mapped to containerToHost");
    L0Test::ExpectTrue(tr, networking["portForwarding"]["containerToHost"].size() == 2u, "both Firebolt and Thunder loopback ports are forwarded");

    if (networking["portForwarding"]["containerToHost"].isArray())
    {
        bool found3473 = false;
        bool found9998 = false;
        for (const auto& rule : networking["portForwarding"]["containerToHost"]) {
            if (rule[ralf::PORT].asUInt() == 3473u) {
                found3473 = true;
            }
            if (rule[ralf::PORT].asUInt() == 9998u) {
                found9998 = true;
            }
        }
        L0Test::ExpectTrue(tr, found3473, "Firebolt port 3473 is added to containerToHost");
        L0Test::ExpectTrue(tr, found9998, "Thunder port 9998 is added to containerToHost");
    }

    L0Test::ExpectTrue(tr, !networking["interContainer"].isArray() || networking["interContainer"].size() == 0u,
                       "loopback host access must not be emitted as interContainer rules");

    unsetenv(ralf::THUNDER_ACCESS_ENV_KEY);
    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_UpdateNetworkConfigurationNode_ImportedHostEndpointRoutesToContainerToHost()
{
    L0Test::TestResult tr;

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    Json::Value networkArray(Json::arrayValue);

    Json::Value importedInterContainer(Json::objectValue);
    importedInterContainer[ralf::PORT] = 5002;
    importedInterContainer[ralf::PROTOCOL] = "tcp";
    importedInterContainer[ralf::TYPE] = "imported";

    Json::Value importedHostEndpoint(Json::objectValue);
    importedHostEndpoint[ralf::PORT] = 9998;
    importedHostEndpoint[ralf::PROTOCOL] = "tcp";
    importedHostEndpoint[ralf::TYPE] = "imported";
    importedHostEndpoint["_hostEndpoint"] = true;

    networkArray.append(importedInterContainer);
    networkArray.append(importedHostEndpoint);
    manifestRootNode[ralf::CONFIGURATION][ralf::NETWORK_CONFIG_URN] = networkArray;

    const bool status = NetworkConfigurationHelper::updateNetworkConfigurationNode(ociConfigRootNode, manifestRootNode);
    L0Test::ExpectTrue(tr, status,
                       "updateNetworkConfigurationNode() returns true for imported endpoint translation matrix");

    const Json::Value& networkingData = ociConfigRootNode[ralf::RDKPLUGINS]["networking"][ralf::DATA];

    const Json::Value& containerToHost = networkingData["portForwarding"]["containerToHost"];
    L0Test::ExpectTrue(tr, containerToHost.isArray() && containerToHost.size() == 1u,
                       "Only host-marked imported endpoint is translated to containerToHost");
    if (containerToHost.isArray() && containerToHost.size() == 1u)
    {
        L0Test::ExpectEqU32(tr, containerToHost[0][ralf::PORT].asUInt(), 9998u,
                            "Host-marked imported endpoint port is routed to containerToHost");
        L0Test::ExpectEqStr(tr, containerToHost[0][ralf::PROTOCOL].asString(), "tcp",
                            "Host-marked imported endpoint protocol is preserved");
        L0Test::ExpectTrue(tr, !containerToHost[0].isMember("_hostEndpoint"),
                           "Internal host endpoint marker is not emitted in Dobby output rules");
    }

    const Json::Value& interContainer = networkingData["interContainer"];
    L0Test::ExpectTrue(tr, interContainer.isArray() && interContainer.size() == 1u,
                       "Non-host imported endpoint remains interContainer rule");
    if (interContainer.isArray() && interContainer.size() == 1u)
    {
        L0Test::ExpectEqStr(tr, interContainer[0]["direction"].asString(), "out",
                            "Non-host imported endpoint maps to interContainer direction out");
        L0Test::ExpectEqU32(tr, interContainer[0][ralf::PORT].asUInt(), 5002u,
                            "Non-host imported endpoint port is preserved in interContainer");
    }

    return tr.failures;
}
