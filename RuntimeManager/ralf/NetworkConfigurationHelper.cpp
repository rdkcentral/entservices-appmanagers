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
#include "../Module.h" //Otherwise logging won't work
#include <UtilsLogging.h>
#include "OCISpecConstants.h"
#include "NetworkConfigurationHelper.h"
#include "RalfSupport.h"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#define MODULE_LOGTAG "RALF-NC"

namespace
{
    constexpr const char *TEMP_RALF_NWCFG    = "_temp_ralf_nwcfg";
    constexpr const char *PERMISSION_FLAGS   = "permissionFlags";
    constexpr const char *PERMISSION_INTERNET_ENABLED = "internetEnabled";
    constexpr const char *PERMISSION_FIREBOLT_ENABLED = "fireboltEnabled";
    constexpr const char *PERMISSION_THUNDER_ENABLED = "thunderEnabled";
    constexpr const char *NETWORKING         = "networking";
    constexpr const char *PUBLIC             = "public";
    constexpr const char *EXPORTED           = "exported";
    constexpr const char *IMPORTED           = "imported";
    constexpr const char *REQUIRED           = "required";
    constexpr const char *DIRECTION          = "direction";
    constexpr const char *IN_DIRECTION       = "in";
    constexpr const char *OUT_DIRECTION      = "out";
    constexpr const char *DNSMASQ            = "dnsmasq";
    constexpr const char *HOST_TO_CONTAINER  = "hostToContainer";
    constexpr const char *CONTAINER_TO_HOST  = "containerToHost";
    constexpr const char *PORT_FORWARDING    = "portForwarding";
    constexpr const char *LOCALHOST_MASQUERADE = "localhostMasquerade";
    constexpr const char *MULTICAST_FORWARDING = "multicastForwarding";
    constexpr const char *INTER_CONTAINER    = "interContainer";
    constexpr const char *NETWORK_TYPE_OPEN  = "open";
    constexpr const char *NETWORK_TYPE_NAT   = "nat";
    constexpr const char *NETWORK_TYPE_NONE  = "none";
    constexpr const char *NETWORK_IPV4       = "ipv4";
    constexpr const char *NETWORK_IPV6       = "ipv6";

    int extractPortFromEndpoint(const std::string& url)
    {
        // Isolate the protocol scheme and the start of the host
        size_t schemePos = url.find("://");
        size_t hostStart = (schemePos == std::string::npos) ? 0 : schemePos + 3;
        size_t colonPos = std::string::npos;

        // Safely find the port colon (handles IPv6 bracket notation vs IPv4/Hostname)
        if (hostStart < url.size() && url[hostStart] == '[') {
            // IPv6 Path
            size_t bracketEnd = url.find(']', hostStart);
            if (bracketEnd != std::string::npos && bracketEnd + 1 < url.size() && url[bracketEnd + 1] == ':') {
                colonPos = bracketEnd + 1;
            }
        } else {
            // IPv4 / Hostname Path
            size_t firstSlash = url.find_first_of("/?", hostStart);
            // Only search for the colon WITHIN the host boundary to prevent path colons from interfering
            if (firstSlash == std::string::npos) {
                colonPos = url.find(':', hostStart);
            } else {
                colonPos = url.find(':', hostStart);
                if (colonPos >= firstSlash) {
                    colonPos = std::string::npos;
                }
            }
        }

        // Effective fallback: If no explicit port colon was discovered, deduce the port via the protocol scheme
        if (colonPos == std::string::npos) {
            LOGWARN("%s: No explicit port found in endpoint '%s'; deducing from protocol.", MODULE_LOGTAG, url.c_str());
            if (schemePos == std::string::npos || schemePos == 0) {
                return -1; // No scheme and no port provided
            }

            // Exact comparison matching for standard protocols using zero-allocation string compares
            if (url.compare(0, schemePos, "http") == 0)  return 80;
            if (url.compare(0, schemePos, "https") == 0) return 443;
            if (url.compare(0, schemePos, "ssh") == 0)   return 22;
            if (url.compare(0, schemePos, "ftp") == 0)   return 21;
            if (url.compare(0, schemePos, "dns") == 0)   return 53;
            if (url.compare(0, schemePos, "dhcp") == 0)  return 67;
            if (url.compare(0, schemePos, "snmp") == 0)  return 161;
            if (url.compare(0, schemePos, "tftp") == 0)  return 69;

            return -1; // Unknown scheme with no explicit port
        }

        // Extract explicit port boundaries
        size_t portStart = colonPos + 1;
        size_t portEnd = url.find_first_of("/?", portStart);
        size_t portLen = (portEnd == std::string::npos) ? (url.size() - portStart) : (portEnd - portStart);

        if (portLen == 0 || portLen > 5) {
            return -1;
        }

        // Allocation-free parsing
        const char* startPtr = url.data() + portStart;
        char* endPtr = nullptr;
        long port = std::strtol(startPtr, &endPtr, 10);

        // Validate parsed range and characters
        if ((endPtr == startPtr) || (port < 0) || (port > 65535) ||
            ((endPtr - startPtr) != static_cast<std::ptrdiff_t>(portLen))) {
            return -1;
        }

        return static_cast<int>(port);
    }

    const char* normalizeProtocol(const std::string& protocol)
    {
        // Quick length filter to instantly bypass long string comparisons
        const size_t len = protocol.size();
        if (len < 2 || len > 5) {
            LOGWARN("%s: Unknown protocol '%s'; defaulting to 'tcp'", MODULE_LOGTAG, protocol.c_str());
            return "tcp";
        }

        // Optimized evaluation group for UDP-based protocols
        if (protocol == "udp"  || protocol == "dns"  || protocol == "dhcp" ||
            protocol == "snmp" || protocol == "coap" || protocol == "coaps" ||
            protocol == "tftp" || protocol == "rtp"  || protocol == "rtsp")
        {
            return "udp";
        }

        // Known TCP-based protocols evaluation group
        if (protocol == "http" || protocol == "https" || protocol == "ftp" ||
            protocol == "ssh"  || protocol == "git"   || protocol == "ws"  ||
            protocol == "wss"  || protocol == "tcp")
        {
            return "tcp";
        }

        LOGWARN("%s: Unknown protocol '%s'; defaulting to 'tcp'", MODULE_LOGTAG, protocol.c_str());
        return "tcp";
    }

    bool isLoopbackEndpoint(const std::string& endpoint)
    {
        // Isolate the start of the host (skip "://")
        size_t start_pos = endpoint.find("://");
        start_pos = (start_pos == std::string::npos) ? 0 : start_pos + 3;

        // Handle IPv6 bracket notation (e.g., "[::1]")
        if (start_pos < endpoint.size() && endpoint[start_pos] == '[') {
            size_t bracket_end = endpoint.find(']', start_pos);
            if (bracket_end != std::string::npos) {
                size_t host_len = bracket_end - start_pos + 1;
                // Exact match for "[::1]" or "[0:0:0:0:0:0:0:1]"
                return (host_len == 5 && endpoint.compare(start_pos, 5, "[::1]") == 0) ||
                       (host_len == 23 && endpoint.compare(start_pos, 23, "[0:0:0:0:0:0:0:1]") == 0);
            }
            return false; // Malformed IPv6 bracket
        }

        // Handle IPv4 and Hostnames (stop at ':' or '/')
        size_t end_pos = endpoint.find_first_of(":/", start_pos);
        size_t host_len = (end_pos == std::string::npos) ? (endpoint.size() - start_pos) : (end_pos - start_pos);

        return ((host_len == 9) &&
                (endpoint.compare(start_pos, 9, "localhost") == 0 ||
                 endpoint.compare(start_pos, 9, "127.0.0.1") == 0));
    }

    bool hasContainerToHostRule(const Json::Value& containerToHost, const uint32_t port)
    {
        const Json::ArrayIndex size = containerToHost.size();
        for (Json::ArrayIndex index = 0; index < size; ++index)
        {
            if (containerToHost[index].isMember(ralf::PORT) &&
                containerToHost[index][ralf::PORT].isUInt() &&
                (port == containerToHost[index][ralf::PORT].asUInt()))
            {
                return true;
            }
        }

        return false;
    }

    void addContainerToHostRuleIfMissing(Json::Value& containerToHost, const uint32_t port, const std::string& protocol = "tcp")
    {
        if (true == hasContainerToHostRule(containerToHost, port))
        {
            return;
        }

        Json::Value rule(Json::objectValue);
        rule[ralf::PORT] = port;
        rule[ralf::PROTOCOL] = protocol;
        containerToHost.append(rule);
    }

    bool ensureMountTargetFileInRootfs(const std::string& configFilePath, const std::string& containerPath)
    {
        // Restored exact original logic check: must not be empty and must start with '/'
        if (containerPath.empty() || '/' != containerPath[0])
        {
            LOGERR("%s: Invalid container path '%s' for mount target preparation", MODULE_LOGTAG, containerPath.c_str());
            return false;
        }

        const size_t configDirPos = configFilePath.find_last_of('/');
        if (configDirPos == std::string::npos)
        {
            LOGERR("%s: Invalid config file path '%s'; unable to resolve rootfs dir.", MODULE_LOGTAG, configFilePath.c_str());
            return false;
        }

        // One Single Allocation for the Target Path
        std::string targetPath;
        targetPath.reserve(configDirPos + 7 + containerPath.size());
        targetPath.append(configFilePath, 0, configDirPos);
        targetPath.append("/rootfs");
        targetPath.append(containerPath);

        const size_t parentDirPos = targetPath.find_last_of('/');
        if (parentDirPos == std::string::npos)
        {
            LOGERR("%s: Unable to resolve parent directory for mount target '%s'", MODULE_LOGTAG, targetPath.c_str());
            return false;
        }

        // Zero-allocation In-Place Truncation
        targetPath[parentDirPos] = '\0';
        bool dirCreated = ralf::create_directories(targetPath.c_str());
        targetPath[parentDirPos] = '/'; // Restore original path separator immediately

        if (!dirCreated)
        {
            // Allocation only happens on the cold failure path
            std::string parentDir = targetPath.substr(0, parentDirPos);
            LOGERR("%s: Failed to create parent directory '%s' for mount target", MODULE_LOGTAG, parentDir.c_str());
            return false;
        }

        if (ralf::checkIfPathExists(targetPath))
        {
            return true;
        }

        std::ofstream targetFile(targetPath.c_str(), std::ios::out | std::ios::app);
        if (!targetFile.is_open())
        {
            LOGERR("%s: Failed to create mount target file '%s': %s", MODULE_LOGTAG, targetPath.c_str(), strerror(errno));
            return false;
        }

        return true;
    }

    bool addNetworkSystemMountsToOCIConfig(Json::Value& ociConfigRootNode, const std::string& configFilePath)
    {
        const bool dnsmasqEnabled = ociConfigRootNode[ralf::RDKPLUGINS][NETWORKING][ralf::DATA][DNSMASQ].asBool();
        if (dnsmasqEnabled)
        {
            LOGWARN("%s: dnsmasq is enabled; skipping network system mounts", MODULE_LOGTAG);
            return true;
        }

        const std::string resolverSourcePath = ralf::getResolverSourcePathForContainer();
        const std::string resolverDestinationPath = ralf::RALF_DEFAULT_RESOLV_CONF_FILE;

        if (!ralf::checkIfPathExists(resolverSourcePath))
        {
            LOGWARN("%s: Host path %s is missing; skipping mount", MODULE_LOGTAG, resolverSourcePath.c_str());
            return false;
        }

        if (!ensureMountTargetFileInRootfs(configFilePath, resolverDestinationPath))
        {
            LOGERR("%s: Failed to prepare rootfs target for %s; skipping mount entry", MODULE_LOGTAG, resolverDestinationPath.c_str());
            return false;
        }

        return ralf::addBindMountToOCIConfig(ociConfigRootNode, resolverSourcePath, resolverDestinationPath);
    }
}

using namespace ralf;
namespace NetworkConfigurationHelper
{

bool updateNetworkConfigurationNode(Json::Value& ociConfigRootNode, const Json::Value& manifestRootNode)
{
    // Handle "configuration" node in the manifest, which may contain "urn:rdk:config:network" metadata.
    if (!manifestRootNode.isMember(CONFIGURATION))
    {
        LOGWARN("%s: No configuration node found in manifest; skipping network configuration update", MODULE_LOGTAG);
        return true;
    }

    const Json::Value& configurationNode = manifestRootNode[CONFIGURATION];
    if (!configurationNode.isMember(NETWORK_CONFIG_URN))
    {
        LOGWARN("%s: No network configuration node found in manifest; skipping network configuration update", MODULE_LOGTAG);
        return true;
    }

    const Json::Value& networkConfiguration = configurationNode[NETWORK_CONFIG_URN];
    if (!networkConfiguration.isArray())
    {
        LOGWARN("%s: Network configuration is not an array", MODULE_LOGTAG);
        return true;
    }

    // Access the node in-place
    Json::Value& networkStore = ociConfigRootNode[TEMP_RALF_NWCFG][ralf::NETWORK];
    if (!networkStore.isObject())
    {
        networkStore = Json::Value(Json::objectValue);
    }

    const Json::ArrayIndex configSize = networkConfiguration.size();
    for (Json::ArrayIndex index = 0; index < configSize; ++index)
    {
        const Json::Value& entry = networkConfiguration[index];

        if (!entry.isObject())
        {
            LOGWARN("%s: Network entry is not an object", MODULE_LOGTAG);
            continue;
        }

        std::string entryName;
        const Json::Value& nameNode = entry[ralf::NAME];

        if (nameNode.isString())
        {
            entryName = nameNode.asString();
        }
        else
        {
            // Replaces heavy streams with a raw stack buffer.
            // 32 bytes is more than enough for "unnamed-" + an unsigned integer.
            char buf[32] = {0};
            int written = snprintf(buf, sizeof(buf), "unnamed-%u", networkStore.size());

            if (written > 0 && static_cast<size_t>(written) < sizeof(buf)) {
                entryName.assign(buf, static_cast<size_t>(written));
            } else {
                entryName = "unnamed-fallback";
            }

            LOGWARN("%s: Network entry is missing a name; generating unique name: %s", MODULE_LOGTAG, entryName.c_str());
        }

        networkStore[entryName] = entry;
    }

    return true;
}

bool updatePermissionConfigurationNode(Json::Value& ociConfigRootNode, const Json::Value& manifestRootNode)
{
    if (!manifestRootNode.isMember(PERMISSIONS))
    {
        LOGWARN("%s: No permissions found in manifest; skipping permission-based networking update", MODULE_LOGTAG);
        return true;
    }

    const Json::Value& permissions = manifestRootNode[PERMISSIONS];
    if (!permissions.isArray())
    {
        LOGWARN("%s: Permissions node is not an array; skipping permission-based networking update", MODULE_LOGTAG);
        return true;
    }

    bool hasPermissionInternet = false;
    bool hasPermissionFirebolt = false;
    bool hasPermissionThunder = false;

    const Json::ArrayIndex size = permissions.size();
    for (Json::ArrayIndex index = 0; index < size; ++index)
    {
        const Json::Value& permValue = permissions[index];
        if (!permValue.isString())
        {
            continue;
        }

        // std::strcmp performs an extremely fast, zero-allocation memory evaluation.
        const char* permStr = permValue.asCString();

        if (std::strcmp(permStr, ralf::PERMISSION_INTERNET) == 0)
        {
            hasPermissionInternet = true;
        }
        else if (std::strcmp(permStr, ralf::PERMISSION_FIREBOLT) == 0)
        {
            hasPermissionFirebolt = true;
        }
        else if (std::strcmp(permStr, ralf::PERMISSION_THUNDER) == 0)
        {
            hasPermissionThunder = true;
        }
    }

    // Single-pass In-Place Write Shield
    Json::Value& permissionFlags = ociConfigRootNode[TEMP_RALF_NWCFG][PERMISSION_FLAGS];
    if (!permissionFlags.isObject())
    {
        permissionFlags = Json::Value(Json::objectValue);
    }

    // Shield against unnecessary JSON map traversal writes if the flag values haven't changed.
    if (hasPermissionInternet)
    {
        permissionFlags[PERMISSION_INTERNET_ENABLED] = true;
    }
    if (hasPermissionFirebolt)
    {
        permissionFlags[PERMISSION_FIREBOLT_ENABLED] = true;
    }
    if (hasPermissionThunder)
    {
        permissionFlags[PERMISSION_THUNDER_ENABLED] = true;
    }

    return true;
}

bool updateTempRalfNWCfgFromEnv(Json::Value& ociConfigRootNode, const std::vector<std::string>& envVarNames)
{
    if (envVarNames.empty())
    {
        LOGWARN("%s: Environment variable list is empty; skipping update", MODULE_LOGTAG);
        return false;
    }

    Json::Value& processNode = ociConfigRootNode[PROCESS];
    if (!processNode.isObject())
    {
        processNode = Json::Value(Json::objectValue);
    }

    Json::Value& envArray = processNode[ENV];
    if (!envArray.isArray())
    {
        envArray = Json::Value(Json::arrayValue);
    }

    Json::Value& tempRalfNWCfgNode = ociConfigRootNode[TEMP_RALF_NWCFG];
    if (!tempRalfNWCfgNode.isObject())
    {
        tempRalfNWCfgNode = Json::Value(Json::objectValue);
    }

    Json::Value& containerToHost = tempRalfNWCfgNode[CONTAINER_TO_HOST];
    if (!containerToHost.isArray())
    {
        containerToHost = Json::Value(Json::arrayValue);
    }

    // Track found strings using pointer hashes instead of allocating new strings
    // Using an unordered_set of const char* references points directly to existing allocations.
    std::unordered_set<const char*> foundEnvNames;
    bool updated = false;

    const Json::ArrayIndex envSize = envArray.size();
    for (Json::ArrayIndex index = 0; index < envSize; ++index)
    {
        const Json::Value& envEntry = envArray[index];
        if (!envEntry.isString())
        {
            continue;
        }

        // Allocation-Free Buffer Traversal
        const char* envPair = envEntry.asCString();
        const char* equalsSign = std::strchr(envPair, '=');
        if (equalsSign == nullptr)
        {
            continue;
        }

        const size_t nameLen = equalsSign - envPair;

        // Single allocation-free match pass against input vector
        // Loops through envVarNames directly using string layout lengths to skip set creation overhead.
        const std::string* matchedVar = nullptr;
        for (const auto& reqName : envVarNames)
        {
            if (reqName.size() == nameLen && std::strncmp(envPair, reqName.c_str(), nameLen) == 0)
            {
                matchedVar = &reqName;
                break;
            }
        }

        if (matchedVar == nullptr)
        {
            continue; // Not requested
        }

        // Deduplication using the static string block address of the matching vector string
        if (foundEnvNames.count(matchedVar->c_str()) > 0)
        {
            continue;
        }

        const char* envVarValue = equalsSign + 1;
        if (*envVarValue == '\0')
        {
            LOGWARN("%s: Environment variable %s is empty; skipping update", MODULE_LOGTAG, matchedVar->c_str());
            continue;
        }

        // Protocol Extraction without allocating temporary substrings
        const char* schemePos = std::strstr(envVarValue, "://");
        const char* protocolStr = "tcp";
        std::string protocolRawStorage; // Allocated only if fallback protocol is used

        if (schemePos != nullptr)
        {
            protocolRawStorage.assign(envVarValue, schemePos - envVarValue);
            protocolStr = normalizeProtocol(protocolRawStorage);
        }

        const int port = extractPortFromEndpoint(envVarValue);
        if (port <= 0)
        {
            LOGWARN("%s: Invalid port extracted from %s: %d; skipping update", MODULE_LOGTAG, matchedVar->c_str(), port);
            continue;
        }

        addContainerToHostRuleIfMissing(containerToHost, static_cast<uint32_t>(port), protocolStr);
        foundEnvNames.insert(matchedVar->c_str());
        updated = true;

        if (isLoopbackEndpoint(envVarValue))
        {
            Json::Value& permissionFlags = tempRalfNWCfgNode[PERMISSION_FLAGS];
            if (!permissionFlags.isObject())
            {
                permissionFlags = Json::Value(Json::objectValue);
            }
            permissionFlags[LOCALHOST_MASQUERADE] = true;
        }
    }

    // Output missing environment warnings cleanly
    for (const auto& envVarName : envVarNames)
    {
        if (foundEnvNames.count(envVarName.c_str()) == 0)
        {
            LOGWARN("%s: Environment variable %s not found in process.env; skipping update", MODULE_LOGTAG, envVarName.c_str());
        }
    }

    return updated;
}

bool generateNetworkingPluginNode(Json::Value& ociConfigRootNode)
{
    // Structural Presence Guard Check
    if (!ociConfigRootNode.isMember(TEMP_RALF_NWCFG))
    {
        LOGWARN("%s: No temporary network configuration found; skipping networking plugin generation", MODULE_LOGTAG);
        return true;
    }

    Json::Value& tempConfig = ociConfigRootNode[TEMP_RALF_NWCFG];
    const Json::Value& permissionFlags = tempConfig[PERMISSION_FLAGS];
    const bool permissionInternetEnabled = permissionFlags[PERMISSION_INTERNET_ENABLED].asBool();
    const bool permissionFireboltEnabled = permissionFlags[PERMISSION_FIREBOLT_ENABLED].asBool();
    const bool permissionThunderEnabled = permissionFlags[PERMISSION_THUNDER_ENABLED].asBool();

    // Pre-allocate Vector Memory
    std::vector<std::string> envVarNames;
    envVarNames.reserve(2);
    if (permissionFireboltEnabled)
    {
        envVarNames.push_back(FIREBOLT_ENDPOINT_ENV_KEY);
    }
    if (permissionThunderEnabled)
    {
        envVarNames.push_back(THUNDER_ACCESS_ENV_KEY);
    }

    if (!envVarNames.empty() && !updateTempRalfNWCfgFromEnv(ociConfigRootNode, envVarNames))
    {
        LOGWARN("%s: Failed to update TEMP_RALF_NWCFG from permission-enabled environment variables", MODULE_LOGTAG);
    }

    // Single-pass Node Isolation
    // Extract references once to prevent repeated map lookups inside conditions and loops
    const Json::Value& networkStore = tempConfig[NETWORK];
    const Json::Value& envContainerToHost = tempConfig[CONTAINER_TO_HOST];

    const bool hasNetworkStore = networkStore.isObject() && !networkStore.empty();
    const bool hasContainerToHostStore = envContainerToHost.isArray() && !envContainerToHost.empty();

    if (!hasNetworkStore && !hasContainerToHostStore && !permissionInternetEnabled && !permissionFireboltEnabled && !permissionThunderEnabled)
    {
        LOGWARN("%s: Temporary network configuration is empty; skipping networking plugin generation", MODULE_LOGTAG);
        return true;
    }

    // Initialize the OCI target JSON tree structures
    Json::Value networkingPlugin(Json::objectValue);
    Json::Value pluginData(Json::objectValue);
    Json::Value portForwarding(Json::objectValue);
    Json::Value hostToContainer(Json::arrayValue);
    Json::Value interContainer(Json::arrayValue);

    networkingPlugin[REQUIRED] = true;
    pluginData[TYPE] = NETWORK_TYPE_NONE;
    pluginData[NETWORK_IPV4] = true;
    pluginData[NETWORK_IPV6] = true;

    if (hasNetworkStore)
    {
        pluginData[TYPE] = NETWORK_TYPE_NAT;
        pluginData[NETWORK_IPV4] = true;
        pluginData[NETWORK_IPV6] = true;

        const Json::Value::Members serviceNames = networkStore.getMemberNames();
        for (const std::string& serviceName : serviceNames)
        {
            const Json::Value& entry = networkStore[serviceName];
            const Json::Value& portNode = entry[PORT];

            if (!portNode.isUInt())
            {
                continue;
            }

            // Zero-Allocation String Evaluation .asCString() - reads directly from internal buffers avoiding heap allocations
            const Json::Value& protoNode = entry[PROTOCOL];
            const char* protocol = (protoNode.isString()) ? protoNode.asCString() : "tcp";

            const Json::Value& typeNode = entry[TYPE];
            const char* type = (typeNode.isString()) ? typeNode.asCString() : PUBLIC;

            if (std::strcmp(PUBLIC, type) == 0)
            {
                Json::Value rule(Json::objectValue);
                rule[PORT] = portNode;
                rule[PROTOCOL] = protocol;
                hostToContainer.append(rule);
            }
            else if (std::strcmp(EXPORTED, type) == 0)
            {
                Json::Value rule(Json::objectValue);
                rule[DIRECTION] = IN_DIRECTION;
                rule[PORT] = portNode;
                rule[PROTOCOL] = protocol;
                interContainer.append(rule);
            }
            else if (std::strcmp(IMPORTED, type) == 0)
            {
                Json::Value rule(Json::objectValue);
                rule[DIRECTION] = OUT_DIRECTION;
                rule[PORT] = portNode;
                rule[PROTOCOL] = protocol;
                interContainer.append(rule);
            }
        }
    }

    Json::Value containerToHost(Json::arrayValue);

    if (hasContainerToHostStore)
    {
        const Json::ArrayIndex size = envContainerToHost.size();
        for (Json::ArrayIndex index = 0; index < size; ++index)
        {
            const Json::Value& rule = envContainerToHost[index];
            const Json::Value& portNode = rule[PORT];
            if (!rule.isObject() || !portNode.isUInt())
            {
                continue;
            }

            // Allocation-free extraction inside container rules
            const Json::Value& protoNode = rule[PROTOCOL];
            const char* protocol = (protoNode.isString()) ? protoNode.asCString() : "tcp";

            addContainerToHostRuleIfMissing(containerToHost, portNode.asUInt(), protocol);
        }
    }

    if (permissionInternetEnabled)
    {
        pluginData[TYPE] = NETWORK_TYPE_NAT;
        pluginData[DNSMASQ] = true;
        pluginData[NETWORK_IPV4] = true;
        pluginData[NETWORK_IPV6] = true;
    }

    if (!containerToHost.empty())
    {
        portForwarding[CONTAINER_TO_HOST] = containerToHost;
        portForwarding[LOCALHOST_MASQUERADE] = permissionFlags[LOCALHOST_MASQUERADE].asBool();
    }

    portForwarding[HOST_TO_CONTAINER] = hostToContainer;
    pluginData[PORT_FORWARDING] = portForwarding;
    pluginData[INTER_CONTAINER] = interContainer;
    networkingPlugin[DATA] = pluginData;

    // Apply the configured plugin node back to the root JSON object
    ociConfigRootNode[RDKPLUGINS][NETWORKING] = networkingPlugin;

    // Clean up temporary setup storage
    ociConfigRootNode.removeMember(TEMP_RALF_NWCFG);

    return true;
}

bool applyRuntimeNetworkingConfiguration(Json::Value& ociConfigRootNode, const std::string& configFilePath)
{
    Json::Value& netData = ociConfigRootNode[RDKPLUGINS][NETWORKING][DATA];

    // Zero-Allocation Type Evaluation
    // Read the type as a raw C-string pointer directly from the internal JSON buffer
    const Json::Value& typeNode = netData[TYPE];
    const char* typeStr = typeNode.isString() ? typeNode.asCString() : "";
    const bool isNatType = (std::strcmp(NETWORK_TYPE_NAT, typeStr) == 0);

    const bool permissionInternetEnabled = isNatType && netData[DNSMASQ].asBool();
    bool runtimeNetworkRequested = isNatType;

    // Single-Pass Cache References
    // Cache structural branches once to avoid repeated multi-level dictionary lookups
    const Json::Value& interContainer = netData[INTER_CONTAINER];
    if (!runtimeNetworkRequested && interContainer.isArray() && !interContainer.empty())
    {
        runtimeNetworkRequested = true;
    }

    const Json::Value& portForwarding = netData[PORT_FORWARDING];
    const bool hasPortForwarding = portForwarding.isObject();

    if (!runtimeNetworkRequested && hasPortForwarding)
    {
        const Json::Value& hostToContainer = portForwarding[HOST_TO_CONTAINER];
        if (hostToContainer.isArray() && !hostToContainer.empty())
        {
            runtimeNetworkRequested = true;
        }
    }

    bool permissionContainerToHostEnabled = false;
    if (hasPortForwarding)
    {
        const Json::Value& containerToHost = portForwarding[CONTAINER_TO_HOST];
        if (containerToHost.isArray() && !containerToHost.empty())
        {
            permissionContainerToHostEnabled = true;
        }
    }

    // Determine if any networking feature has been activated
    const bool effectiveNetworkEnabled = runtimeNetworkRequested ||
                                          permissionInternetEnabled ||
                                          permissionContainerToHostEnabled;

    if (effectiveNetworkEnabled)
    {
        // Zero-Allocation Capability Matrix Scanning
        static const char* capabilitySets[] = {"ambient", "bounding", "effective", "inheritable", "permitted"};
        Json::Value& capabilitiesNode = ociConfigRootNode[PROCESS]["capabilities"];

        for (const char* setName : capabilitySets)
        {
            Json::Value& capSet = capabilitiesNode[setName];
            if (!capSet.isArray())
            {
                capSet = Json::Value(Json::arrayValue);
            }

            bool alreadyPresent = false;
            const Json::ArrayIndex size = capSet.size();
            for (Json::ArrayIndex i = 0; i < size; ++i)
            {
                const Json::Value& capItem = capSet[i];
                // Using .asCString() + std::strcmp prevents creating temporary heap strings during iteration
                if (capItem.isString() && std::strcmp("CAP_NET_BIND_SERVICE", capItem.asCString()) == 0)
                {
                    alreadyPresent = true;
                    break;
                }
            }

            if (!alreadyPresent)
            {
                capSet.append("CAP_NET_BIND_SERVICE");
            }
        }

        // Consolidated State Mapping: If effectiveNetworkEnabled is true, at least one target flag is true.
        // We can safely apply the NAT configuration.
        netData[TYPE] = NETWORK_TYPE_NAT;
        netData[DNSMASQ] = true;
    }
    else
    {
        netData[TYPE] = NETWORK_TYPE_NONE;
        netData[DNSMASQ] = false;
    }

    // Dobby will not create the network namespace if dnsmasq is disabled, so we need to ensure that
    // resolv.conf is mounted into the container when dnsmasq is disabled.
    if (!netData[DNSMASQ].asBool())
    {
        if (!addNetworkSystemMountsToOCIConfig(ociConfigRootNode, configFilePath))
        {
            LOGWARN("%s: addNetworkSystemMountsToOCIConfig failed", MODULE_LOGTAG);
        }
    }

    LOGDBG("%s: Network mode set to '%s' (runtimeEnabled=%d permissionInternet=%d permissionContainerToHost=%d)",
            MODULE_LOGTAG, netData[TYPE].asCString(), runtimeNetworkRequested,
            permissionInternetEnabled, permissionContainerToHostEnabled);

    return true;
}

} // namespace NetworkConfigurationHelper
