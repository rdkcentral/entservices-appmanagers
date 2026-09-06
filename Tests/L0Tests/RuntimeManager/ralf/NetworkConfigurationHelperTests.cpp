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
 *   - updatePermissionConfigurationNode()
 *   - updateTempRalfNWCfgFromEnv()
 *   - generateNetworkingPluginNode()
 *   - applyRuntimeNetworkingConfiguration()
 */

#include <cstdint>
#include <string>
#include <vector>

#include "ralf/NetworkConfigurationHelper.h"
#include "ralf/OCISpecConstants.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

uint32_t Test_NetworkConfigurationHelper_UpdateNetworkConfigurationNode_MergesEntriesByName()
{
    L0Test::TestResult tr;

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    Json::Value networkArray(Json::arrayValue);

    // --- POSITIVE SCENARIO: Duplicate name override/merge ---
    Json::Value firstEntry(Json::objectValue);
    firstEntry[ralf::NAME] = "svc";
    firstEntry[ralf::PORT] = 80;
    firstEntry[ralf::PROTOCOL] = "tcp";
    firstEntry[ralf::TYPE] = "public";

    Json::Value secondEntry(Json::objectValue);
    secondEntry[ralf::NAME] = "svc";
    secondEntry[ralf::PORT] = 443;
    secondEntry[ralf::PROTOCOL] = "tcp";
    secondEntry[ralf::TYPE] = "public";

    // --- POSITIVE BOUNDARY SCENARIO: Missing name fallback naming ---
    Json::Value entryNoName(Json::objectValue);
    entryNoName[ralf::PORT] = 8080;

    Json::Value entryBadNameType(Json::objectValue);
    entryBadNameType[ralf::NAME] = 12345; // Integer type instead of String
    entryBadNameType[ralf::PORT] = 9090;

    networkArray.append(firstEntry);
    networkArray.append(secondEntry);
    networkArray.append(entryNoName);
    networkArray.append(entryBadNameType);

    manifestRootNode[ralf::CONFIGURATION][ralf::NETWORK_CONFIG_URN] = networkArray;

    const bool status = NetworkConfigurationHelper::updateNetworkConfigurationNode(ociConfigRootNode, manifestRootNode);
    L0Test::ExpectTrue(tr, status,
                       "updateNetworkConfigurationNode() returns true for valid manifest network array");

    const Json::Value& networkStore = ociConfigRootNode["_temp_ralf_nwcfg"][ralf::NETWORK];
    L0Test::ExpectTrue(tr, networkStore.isObject(), "Temporary network store node exists");

    // Verify Override Logic
    const Json::Value& mergedNode = networkStore["svc"];
    L0Test::ExpectTrue(tr, mergedNode.isObject(), "Merged network entry exists under network.svc");
    L0Test::ExpectEqU32(tr, mergedNode[ralf::PORT].asUInt(), 443u,
                        "Later entry replaces earlier entry with same service name");

    // Verify Optimized snprintf Unnamed Fallbacks
    L0Test::ExpectTrue(tr, networkStore.isMember("unnamed-1"), "Generates 'unnamed-1' for missing name key node");
    L0Test::ExpectTrue(tr, networkStore.isMember("unnamed-2"), "Generates 'unnamed-2' for bad name type node");

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

    const Json::Value& networkStore = ociConfigRootNode["_temp_ralf_nwcfg"][ralf::NETWORK];
    L0Test::ExpectTrue(tr, networkStore.isObject(), "temporary network store exists");

    L0Test::ExpectTrue(tr, networkStore.isMember("svc-valid"),
                       "valid network entry is retained when malformed entries are present");
    if (networkStore.isMember("svc-valid")) {
        L0Test::ExpectEqU32(tr, networkStore["svc-valid"][ralf::PORT].asUInt(), 7777u,
                            "valid entry port is preserved");
    }

    // Verify malformed items were safely skipped and did not pollute the object keys
    L0Test::ExpectTrue(tr, !networkStore.isMember("invalid-entry-string"), "Malformed strings are safely ignored");

    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_UpdatePermissionConfigurationNode_SetsFlags()
{
    L0Test::TestResult tr;

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    Json::Value permissions(Json::arrayValue);

    // --- POSITIVE SCENARIO: Set target permissions ---
    permissions.append(ralf::PERMISSION_FIREBOLT);
    permissions.append(ralf::PERMISSION_THUNDER);
    permissions.append("urn:rdk:permission:other");
    // NOTE: ralf::PERMISSION_INTERNET is intentionally OMITTED here (Negative Isolation scenario)

    manifestRootNode[ralf::PERMISSIONS] = permissions;

    const bool status = NetworkConfigurationHelper::updatePermissionConfigurationNode(ociConfigRootNode, manifestRootNode);
    L0Test::ExpectTrue(tr, status, "updatePermissionConfigurationNode() returns true for permissions array");

    const Json::Value& flags = ociConfigRootNode["_temp_ralf_nwcfg"]["permissionFlags"];
    L0Test::ExpectTrue(tr, flags.isObject(), "permissionFlags object is initialized");

    // Verify Active Permission Flags (Positive)
    L0Test::ExpectTrue(tr, flags["fireboltEnabled"].asBool(), "fireboltEnabled flag is explicitly set");
    L0Test::ExpectTrue(tr, flags["thunderEnabled"].asBool(), "thunderEnabled flag is explicitly set");

    // Verify Absent Permission Isolation (Negative)
    L0Test::ExpectTrue(tr, !flags.isMember("internetEnabled"), "Absent permission keys are isolated and left unset");

    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_UpdateTempRalfNWCfgFromEnv_ParsesPortAndLocalhostMasquerade()
{
    L0Test::TestResult tr;

    // --- MIXED BOUNDARY TESTING ---
    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value envArray(Json::arrayValue);

    // Entry A: Valid loopback endpoint (Triggers port extract + masquerade)
    envArray.append(std::string(ralf::FIREBOLT_ENDPOINT_ENV_KEY) + "=https://localhost:9998");
    // Entry B: Valid non-loopback remote endpoint (Negative masquerade verification)
    envArray.append(std::string(ralf::THUNDER_ACCESS_ENV_KEY) + "=ws://8.8.8.8:8080");
    // Entry C: Malformed endpoint format containing alphabetic text port (Negative parsing validation)
    envArray.append("MALFORMED_ENDPOINT_VAR=ws://127.0.0.1:abc");
    // Entry D: Completely empty value field (Negative truncation validation)
    envArray.append("EMPTY_ENDPOINT_VAR=");

    ociConfigRootNode[ralf::PROCESS][ralf::ENV] = envArray;

    std::vector<std::string> requestedVars;
    requestedVars.push_back(ralf::FIREBOLT_ENDPOINT_ENV_KEY);
    requestedVars.push_back(ralf::THUNDER_ACCESS_ENV_KEY);
    requestedVars.push_back("MALFORMED_ENDPOINT_VAR");
    requestedVars.push_back("EMPTY_ENDPOINT_VAR");

    const bool updated = NetworkConfigurationHelper::updateTempRalfNWCfgFromEnv(ociConfigRootNode, requestedVars);
    L0Test::ExpectTrue(tr, updated, "updateTempRalfNWCfgFromEnv() returns true when valid vars match");

    const Json::Value& rules = ociConfigRootNode["_temp_ralf_nwcfg"]["containerToHost"];
    L0Test::ExpectTrue(tr, rules.isArray(), "containerToHost rules array generated");

    // Verify Port Extraction Matrix
    if (rules.isArray() && rules.size() >= 2u) {
        L0Test::ExpectEqU32(tr, rules[0][ralf::PORT].asUInt(), 9998u, "Parsed loopback port matches 9998");
        L0Test::ExpectEqU32(tr, rules[1][ralf::PORT].asUInt(), 8080u, "Parsed remote server port matches 8080");
    }

    // Verify Masquerade Isolation Flags
    const Json::Value& flags = ociConfigRootNode["_temp_ralf_nwcfg"]["permissionFlags"];
    L0Test::ExpectTrue(tr, flags["localhostMasquerade"].asBool(), "Localhost endpoint toggles localhostMasquerade");

    // Verify that the total parsed rule count reflects skipped malformed entries
    L0Test::ExpectEqU32(tr, rules.size(), 2u, "Malformed text port and empty variable rows are safely ignored");

    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_UpdateTempRalfNWCfgFromEnv_EmptyEnvVarListReturnsFalse()
{
    L0Test::TestResult tr;

    // --- NEGATIVE SCENARIO: Empty request vectors ---
    Json::Value ociConfigRootNode(Json::objectValue);
    ociConfigRootNode[ralf::PROCESS][ralf::ENV] = Json::Value(Json::arrayValue);

    std::vector<std::string> requestedVars; // Empty vector list
    const bool updated = NetworkConfigurationHelper::updateTempRalfNWCfgFromEnv(ociConfigRootNode, requestedVars);

    L0Test::ExpectTrue(tr, !updated, "updateTempRalfNWCfgFromEnv() returns false when envVarNames is empty");
    L0Test::ExpectTrue(tr, !ociConfigRootNode.isMember("_temp_ralf_nwcfg"),
                       "_temp_ralf_nwcfg is not created when envVarNames is empty");

    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_GenerateNetworkingPluginNode_ConsumesTempNode()
{
    L0Test::TestResult tr;
    Json::Value ociConfigRootNode(Json::objectValue);

    // --- SCENARIO 1: Valid Public (hostToContainer) ---
    Json::Value entryPublic(Json::objectValue);
    entryPublic[ralf::PORT] = 8080;
    entryPublic[ralf::PROTOCOL] = "tcp";
    entryPublic[ralf::TYPE] = "public";

    // --- SCENARIO 2: Valid Exported (interContainer IN) ---
    Json::Value entryExported(Json::objectValue);
    entryExported[ralf::PORT] = 5001;
    entryExported[ralf::PROTOCOL] = "udp";
    entryExported[ralf::TYPE] = "exported";

    // --- SCENARIO 3: Valid Imported (interContainer OUT) ---
    Json::Value entryImported(Json::objectValue);
    entryImported[ralf::PORT] = 5002;
    entryImported[ralf::PROTOCOL] = "tcp";
    entryImported[ralf::TYPE] = "imported";

    // --- SCENARIO 4: NEGATIVE BOUNDARY - Missing/Malformed Port ---
    Json::Value entryMalformed(Json::objectValue);
    entryMalformed[ralf::PROTOCOL] = "tcp";
    entryMalformed[ralf::TYPE] = "public"; // Missing PORT key completely

    ociConfigRootNode["_temp_ralf_nwcfg"][ralf::NETWORK]["svc-public"] = entryPublic;
    ociConfigRootNode["_temp_ralf_nwcfg"][ralf::NETWORK]["svc-exported"] = entryExported;
    ociConfigRootNode["_temp_ralf_nwcfg"][ralf::NETWORK]["svc-imported"] = entryImported;
    ociConfigRootNode["_temp_ralf_nwcfg"][ralf::NETWORK]["svc-broken"] = entryMalformed;

    const bool status = NetworkConfigurationHelper::generateNetworkingPluginNode(ociConfigRootNode);
    L0Test::ExpectTrue(tr, status,
                       "generateNetworkingPluginNode() returns true with valid temp network config matrix");

    const Json::Value& pluginNode = ociConfigRootNode[ralf::RDKPLUGINS]["networking"];
    L0Test::ExpectTrue(tr, pluginNode.isObject(),
                       "networking plugin node is generated under rdkPlugins.networking");
    L0Test::ExpectTrue(tr, pluginNode["required"].asBool(),
                       "generated networking plugin is marked as required");

    // Verify hostToContainer Mappings
    const Json::Value& hostToContainer = pluginNode[ralf::DATA]["portForwarding"]["hostToContainer"];
    L0Test::ExpectTrue(tr, hostToContainer.isArray() && hostToContainer.size() == 1u,
                       "Valid public service generates exactly 1 hostToContainer rule; malformed node is safely skipped");
    if (hostToContainer.isArray() && !hostToContainer.empty()) {
        L0Test::ExpectEqU32(tr, hostToContainer[0][ralf::PORT].asUInt(), 8080u, "Public service port is verified");
    }

    // Verify interContainer Directional Matrix Mappings (Exported and Imported)
    const Json::Value& interContainer = pluginNode[ralf::DATA]["interContainer"];
    L0Test::ExpectTrue(tr, interContainer.isArray() && interContainer.size() == 2u,
                       "Generates both directional rules cleanly under interContainer array store");

    if (interContainer.isArray() && interContainer.size() == 2u) {
        L0Test::ExpectEqStr(tr, interContainer[0]["direction"].asString(), "in", "Exported types cleanly map to directional rule 'in'");
        L0Test::ExpectEqStr(tr, interContainer[1]["direction"].asString(), "out", "Imported types cleanly map to directional rule 'out'");
    }

    // Verify In-place Cleanup
    L0Test::ExpectTrue(tr, !ociConfigRootNode.isMember("_temp_ralf_nwcfg"),
                       "temporary _temp_ralf_nwcfg node is scrubbed from the root layout after plugin generation");

    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_ApplyRuntimeNetworkingConfiguration_AddsCapabilityForNetworkMode()
{
    L0Test::TestResult tr;

    // ========================================================================
    // TEST PASS 1: POSITIVE SCENARIO - NAT mode enables capabilities cleanly
    // ========================================================================
    Json::Value ociConfigRootNode(Json::objectValue);

    // Inject existing capability to verify duplicate shield logic works safely
    ociConfigRootNode[ralf::PROCESS]["capabilities"]["ambient"].append("CAP_NET_BIND_SERVICE");
    ociConfigRootNode[ralf::PROCESS]["capabilities"]["ambient"].append("CAP_SYS_ADMIN");

    ociConfigRootNode[ralf::RDKPLUGINS]["networking"][ralf::DATA][ralf::TYPE] = "nat";
    ociConfigRootNode[ralf::RDKPLUGINS]["networking"][ralf::DATA]["dnsmasq"] = true;

    bool status = NetworkConfigurationHelper::applyRuntimeNetworkingConfiguration(
        ociConfigRootNode, "/tmp/ralf_l0test_cfg/config.json");

    L0Test::ExpectTrue(tr, status, "applyRuntimeNetworkingConfiguration() returns true for nat networking mode");

    const Json::Value& netData = ociConfigRootNode[ralf::RDKPLUGINS]["networking"][ralf::DATA];
    L0Test::ExpectEqStr(tr, netData[ralf::TYPE].asString(), "nat", "network type remains nat");
    L0Test::ExpectTrue(tr, netData["dnsmasq"].asBool(), "dnsmasq remains enabled");

    // Verify Capability injection and uniqueness check
    const Json::Value& ambient = ociConfigRootNode[ralf::PROCESS]["capabilities"]["ambient"];
    uint32_t matchCount = 0;
    const Json::ArrayIndex capCount = ambient.size();
    for (Json::ArrayIndex i = 0; i < capCount; ++i) {
        if (ambient[i].isString() && ambient[i].asString() == "CAP_NET_BIND_SERVICE") {
            matchCount++;
        }
    }
    L0Test::ExpectEqU32(tr, matchCount, 1u, "CAP_NET_BIND_SERVICE is present exactly once (duplicate injection prevented)");

    // ========================================================================
    // TEST PASS 2: NEGATIVE SCENARIO - Disabled features fall back to NONE/false
    // ========================================================================
    Json::Value ociConfigRootNodeDisabled(Json::objectValue);
    ociConfigRootNodeDisabled[ralf::RDKPLUGINS]["networking"][ralf::DATA][ralf::TYPE] = "none";
    ociConfigRootNodeDisabled[ralf::RDKPLUGINS]["networking"][ralf::DATA]["dnsmasq"] = false;

    status = NetworkConfigurationHelper::applyRuntimeNetworkingConfiguration(
        ociConfigRootNodeDisabled, "/tmp/ralf_l0test_cfg/config.json");

    L0Test::ExpectTrue(tr, status, "applyRuntimeNetworkingConfiguration() returns true when routing fields are disabled");

    const Json::Value& disabledNetData = ociConfigRootNodeDisabled[ralf::RDKPLUGINS]["networking"][ralf::DATA];
    L0Test::ExpectEqStr(tr, disabledNetData[ralf::TYPE].asString(), "none", "Type defaults back cleanly to 'none'");
    L0Test::ExpectTrue(tr, !disabledNetData["dnsmasq"].asBool(), "dnsmasq falls back cleanly to false");

    return tr.failures;
}
