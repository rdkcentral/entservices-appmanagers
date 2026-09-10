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
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#define MODULE_LOGTAG "RALF-NC"

namespace
{
    constexpr const char *PERMISSION_INTERNET_ENABLED = "internetEnabled";
    constexpr const char *PERMISSION_FIREBOLT_ENABLED = "fireboltEnabled";
    constexpr const char *PERMISSION_THUNDER_ENABLED = "thunderEnabled";
    constexpr const char *NETWORKING         = "networking";
    constexpr const char *PUBLIC             = "public";
    constexpr const char *EXPORTED           = "exported";
    constexpr const char *IMPORTED           = "imported";
    constexpr const char *REQUIRED           = "required";
    constexpr const char *DIRECTION          = "direction";
    constexpr const char *IP                 = "ip";
    constexpr const char *DIRECTION_IN       = "in";
    constexpr const char *DIRECTION_OUT      = "out";
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
    constexpr const char *DEFAULT_PROTOCOL   = "tcp";

    /**
     * @brief Creates/Retrieves the networking data node from the OCI config root node.
     * @param ociConfigRootNode The root node of the OCI config JSON.
     * @param createIfMissing If true, will create the necessary nodes if they are missing.
     *   It will create this:
     *   {
     *     "rdkPlugins": {
     *       "networking": {
     *         "required": true,
     *         "data": {
     *           "type": "nat",
     *           "ipv4": true,
     *           "ipv6": true,
     *           "dnsmasq": true
     *         }
     *       }
     *     }
     *   }
     * @return Pointer to the networking data node, or nullptr if it could not be found/created.
     */
    Json::Value* getNetworkingDataNode(Json::Value& ociConfigRootNode, const bool createIfMissing)
    {
        // 1. Resolve RDKPLUGINS
        if (!ociConfigRootNode.isObject())
        {
            LOGERR("%s: Root OCI configuration node is not an object.", MODULE_LOGTAG);
            return nullptr;
        }

        Json::Value* rdkPlugins = nullptr;
        if (ociConfigRootNode.isMember(ralf::RDKPLUGINS) && ociConfigRootNode[ralf::RDKPLUGINS].isObject())
        {
            rdkPlugins = &ociConfigRootNode[ralf::RDKPLUGINS];
        }
        else
        {
            if (!createIfMissing)
            {
                LOGERR("%s: %s node is missing/invalid type and could not create it.", MODULE_LOGTAG, ralf::RDKPLUGINS);
                return nullptr;
            }
            ociConfigRootNode[ralf::RDKPLUGINS] = Json::Value(Json::objectValue);
            rdkPlugins = &ociConfigRootNode[ralf::RDKPLUGINS];
        }

        // 2. Resolve NETWORKING
        Json::Value* networking = nullptr;
        if (rdkPlugins->isMember(NETWORKING) && (*rdkPlugins)[NETWORKING].isObject())
        {
            networking = &(*rdkPlugins)[NETWORKING];
        }
        else
        {
            if (!createIfMissing)
            {
                LOGERR("%s: %s.%s node is missing/invalid type and could not create it.",
                       MODULE_LOGTAG, ralf::RDKPLUGINS, NETWORKING);
                return nullptr;
            }
            // Explicitly re-initialize to objectValue to clear any prior malformed type
            Json::Value& netRef = (*rdkPlugins)[NETWORKING] = Json::Value(Json::objectValue);
            netRef[REQUIRED] = true;
            networking = &netRef;
        }

        // 3. Resolve DATA
        Json::Value* dataNode = nullptr;
        if (networking->isMember(ralf::DATA) && (*networking)[ralf::DATA].isObject())
        {
            dataNode = &(*networking)[ralf::DATA];
        }
        else
        {
            if (!createIfMissing)
            {
                LOGERR("%s: %s.%s.%s node is missing/invalid type and could not create it.",
                       MODULE_LOGTAG, ralf::RDKPLUGINS, NETWORKING, ralf::DATA);
                return nullptr;
            }
            (*networking)[ralf::DATA] = Json::Value(Json::objectValue);
            dataNode = &(*networking)[ralf::DATA];
        }

        // 4. Resolve TYPE
        if (!dataNode->isMember(ralf::TYPE)) {
            if (!createIfMissing) {
                LOGERR("%s: %s.%s.%s.%s node is missing and could not create in OCI config.",
                        MODULE_LOGTAG, ralf::RDKPLUGINS, NETWORKING, ralf::DATA, ralf::TYPE);
                return nullptr;
            }
            (*dataNode)[ralf::TYPE] = NETWORK_TYPE_NAT;
            (*dataNode)[NETWORK_IPV4] = true;
            (*dataNode)[NETWORK_IPV6] = true;
            (*dataNode)[DNSMASQ] = true;
        }

        return dataNode;
    }

    /**
     * @brief Extracts the port number from a given URL endpoint.
     * @param url The URL endpoint string.
     * @return The port number if found, otherwise -1.
     */
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

    /**
     * @brief Normalizes application-level protocol aliases into Dobby-compatible L4 layer types ("tcp" or "udp").
     * Maps known industry layer-7 primitives to their underlying transport protocols. Unknown inputs
     * safely fall back to the globally defined default protocol macro.
     * @param[in] protocol The raw string input from the configuration payload.
     * @return const char* A static string literal pointing to "tcp" or "udp".
     */
    const char* normalizeProtocol(const std::string& protocol)
    {
        // Quick length filter to instantly bypass out-of-bounds string tokens
        const size_t len = protocol.size();
        if (len < 2 || len > 5) {
            LOGWARN("%s: Unknown protocol layout '%s'; defaulting to '%s'", MODULE_LOGTAG, protocol.c_str(), DEFAULT_PROTOCOL);
            return DEFAULT_PROTOCOL;
        }

        // Evaluation group for UDP-based protocols
        if (protocol == "udp"  || protocol == "dns"  || protocol == "dhcp" ||
            protocol == "snmp" || protocol == "coap" || protocol == "coaps" ||
            protocol == "tftp" || protocol == "rtp")
        {
            return "udp";
        }

        // Evaluation group for TCP-based protocols (Fixed: Moved 'rtsp' to TCP control layer)
        if (protocol == "tcp"  || protocol == "http" || protocol == "https" ||
            protocol == "ftp"  || protocol == "ssh"  || protocol == "git"   ||
            protocol == "ws"   || protocol == "wss"  || protocol == "rtsp")
        {
            return "tcp";
        }

        LOGWARN("%s: Unrecognized mapping alias '%s'; defaulting to '%s'", MODULE_LOGTAG, protocol.c_str(), DEFAULT_PROTOCOL);
        return DEFAULT_PROTOCOL;
    }

    /**
     * @brief Checks if the given endpoint is a loopback address.
     * @param endpoint The endpoint string to check.
     * @return true if the endpoint is a loopback address, false otherwise.
     */
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

    /**
     * @brief Checks if a given port is already present in the container-to-host port forwarding rules.
     * @param containerToHost The JSON array representing container-to-host port forwarding rules.
     * @param port The port number to check for.
     * @return true if the port is found, false otherwise.
     */
    bool hasContainerToHostRule(const Json::Value& containerToHost, const uint32_t port, const std::string& protocol)
    {
        if (!containerToHost.isArray())
        {
            return false;
        }

        const Json::ArrayIndex size = containerToHost.size();
        for (Json::ArrayIndex index = 0; index < size; ++index)
        {
            const Json::Value& rule = containerToHost[index];

            if (rule.isMember(ralf::PORT) && rule[ralf::PORT].isUInt() &&
                rule.isMember(ralf::PROTOCOL) && rule[ralf::PROTOCOL].isString())
            {
                if (port == rule[ralf::PORT].asUInt() && protocol == rule[ralf::PROTOCOL].asString())
                {
                    return true;
                }
            }
        }

        return false;
    }

    /**
     * @brief Adds a container-to-host port forwarding rule to the provided JSON array.
     * @param containerToHost The JSON array representing container-to-host port forwarding rules.
     * @param port The port number for the rule.
     * @param protocol The protocol for the rule (default is "tcp").
     */
    void addContainerToHostRuleIfMissing(Json::Value& containerToHost, const uint32_t port, const std::string& protocol)
    {
        if (hasContainerToHostRule(containerToHost, port, protocol))
        {
            return;
        }

        Json::Value rule(Json::objectValue);
        rule[ralf::PORT] = port;
        rule[ralf::PROTOCOL] = protocol;
        containerToHost.append(rule);
    }

    /**
     * @brief Scans a Dobby rule array for duplicate configurations and appends the rule if unique.
     *
     * Verifies that no element matching the same port and protocol exists within the
     * target array before appending. Prevents overlapping rules when processing lists.
     *
     * @param[in,out] arrayContainer The target Json::Value array where the rule should be added.
     * @param[in] rule The Json::Value object containing the network rule fields.
     *
     * @return void
     */
    void appendIfUnique(Json::Value& arrayContainer, const Json::Value& rule)
    {
        if (!arrayContainer.isArray()) return;

        uint32_t port = rule[ralf::PORT].asUInt();
        std::string protocol = rule[ralf::PROTOCOL].asString();

        // Scan for duplicate port + protocol combinations
        for (const auto& existingRule : arrayContainer)
        {
            if (existingRule.isMember(ralf::PORT) && existingRule[ralf::PORT].isUInt() &&
                existingRule.isMember(ralf::PROTOCOL) && existingRule[ralf::PROTOCOL].isString())
            {
                if (port == existingRule[ralf::PORT].asUInt() && protocol == existingRule[ralf::PROTOCOL].asString())
                {
                    return; // Duplicate found, exit early
                }
            }
        }

        // No duplicate found, perform the insert
        arrayContainer.append(rule);
    }

#if 0 // Disabled for now, but will need when DNSMASQ is disabled in Dobby Network Configurations.
    /**
     * @brief Checks if the resolv.conf file contains only loopback nameservers.
     * @param resolvPath The path to the resolv.conf file.
     * @return true if only loopback nameservers are present, false otherwise.
     */
    bool hasOnlyLoopbackNameServers(const std::string &resolvPath)
    {
        std::ifstream in(resolvPath);
        if (!in)
        {
            return false;
        }

        std::string line;
        std::string ipStr;
        const std::string whitespace = " \t";

        while (std::getline(in, line))
        {
            if (line.empty()) continue;

            size_t start = line.find_first_not_of(whitespace);
            if (start == std::string::npos || line[start] == '#' || line[start] == ';')
            {
                continue;
            }

            if (line.compare(start, 10, "nameserver") != 0)
            {
                continue;
            }

            size_t valueStart = line.find_first_not_of(whitespace, start + 10);
            if (valueStart == std::string::npos || line[valueStart] == '#' || line[valueStart] == ';')
            {
                continue;
            }

            size_t valueEnd = line.find_first_of(" \t#;\r\n", valueStart);

            // Extract the IP text token (strip off any trailing zone identifiers like %lo0)
            size_t zoneMarker = line.find_first_of('%', valueStart);
            size_t extractEnd = valueEnd;
            if (zoneMarker != std::string::npos && (valueEnd == std::string::npos || zoneMarker < valueEnd))
            {
                extractEnd = zoneMarker;
            }

            if (extractEnd == std::string::npos)
            {
                ipStr.assign(line, valueStart, std::string::npos);
            }
            else
            {
                ipStr.assign(line, valueStart, extractEnd - valueStart);
            }

            if (ipStr.empty()) continue;

            // If it doesn't even start with '1' or ':', it cannot possibly be a loopback address.
            // This completely skips expensive inet_pton system calls for external IPs like 8.8.8.8.
            const char firstChar = ipStr[0];
            if (firstChar != '1' && firstChar != ':')
            {
                return false; // Found an external nameserver, abort immediately!
            }

            // Try parsing as IPv4
            struct in_addr ipv4Addr;
            if (inet_pton(AF_INET, ipStr.c_str(), &ipv4Addr) == 1)
            {
                // Accessing the internal byte layout directly.
                // The first octet is always at index 0 in network memory layout, making it completely endian-independent.
                const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&ipv4Addr.s_addr);
                if (bytes[0] == 127)
                {
                    continue; // Valid IPv4 loopback range
                }
            }
            else // Try parsing as IPv6
            {
                struct in6_addr ipv6Addr;
                if (inet_pton(AF_INET6, ipStr.c_str(), &ipv6Addr) == 1)
                {
                    // IPv6 loopback is strictly ::1 (15 bytes of 0x00, 1 byte of 0x01)
                    if (IN6_IS_ADDR_LOOPBACK(&ipv6Addr))
                    {
                        continue; // Valid IPv6 loopback
                    }
                }
            }

            // If it passes the '1'/':' check but isn't a valid loopback structure, it's external or invalid
            return false;
        }

        return true;
    }

    /**
     * @brief Determines the appropriate resolver source path for the container.
     * @param defaultResolverPath The default resolver file path.
     * @param nwmgrResolverPath The NetworkManager resolver file path.
     * @param systemdResolverPath The systemd-resolved resolver file path.
     * @return The selected resolver file path.
     */
    std::string getResolverSourcePathForContainer(const std::string& defaultResolverPath,
                                                  const std::string& nwmgrResolverPath,
                                                  const std::string& systemdResolverPath)
    {
        /*
         * Determine the appropriate resolver source path for the container.
         * If the default resolver file has only loopback nameservers, check for alternative resolver files provided by
         * NetworkManager or systemd-resolved. If found, use those; otherwise, fall back to the default resolver file.
         */
        // If the default file contains valid external nameservers, use it immediately.
        // Note: we guard with checkIfPathExists(); missing files won't match this branch.
        if (checkIfPathExists(defaultResolverPath) &&
            !hasOnlyLoopbackNameServers(defaultResolverPath))
        {
            return defaultResolverPath;
        }

        // Check the NetworkManager fallback file.
        // It must exist AND contain at least one external, non-loopback nameserver.
        if (checkIfPathExists(nwmgrResolverPath) &&
            !hasOnlyLoopbackNameServers(nwmgrResolverPath))
        {
            return nwmgrResolverPath;
        }

        // Check the systemd-resolved fallback file.
        // It must exist AND contain at least one external, non-loopback nameserver.
        if (checkIfPathExists(systemdResolverPath) &&
            !hasOnlyLoopbackNameServers(systemdResolverPath))
        {
            return systemdResolverPath;
        }

        // Ultimate Fallback path
        LOGWARN("Host resolver file %s only has loopback nameservers and no valid fallback resolver file found",
                defaultResolverPath.c_str());

        return defaultResolverPath;
    }

    /**
     * @brief Determines the appropriate resolver source path for the container.
     * @return The selected resolver file path.
     */
    std::string getResolverSourcePathForContainer()
    {
        return getResolverSourcePathForContainer(RALF_HOST_DEFAULT_RESOLV_CONF_FILE,
                                                 RALF_HOST_NOSTUB_NWMGR_RESOLV_CONF_FILE,
                                                 RALF_HOST_NOSTUB_SYSTEMD_RESOLV_CONF_FILE);
    }

    /**
     * @brief Ensures that the mount target file exists in the rootfs of the container.
     * @param configFilePath The path to the configuration file.
     * @param containerPath The path inside the container where the mount target should be created.
     * @return true if the mount target file exists or was created successfully, false otherwise.
     */
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

    /**
     * @brief Adds network system mounts to the OCI configuration.
     *     Future reserved - for additional system mounts when dnsmasq is disabled.
     * @param[in,out] ociConfigRootNode The root node of the OCI configuration JSON.
     * @param[in] configFilePath The path to the configuration file.
     * @return true if the mounts were added successfully, false otherwise.
     */
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
#endif // 0 - DNSMASQ Disabled Mount helpers.
}

using namespace ralf;
namespace NetworkConfigurationHelper
{

/**
 * @brief Updates the networking data node in the OCI config with the provided Dobby network configuration object.
 * @param ociConfigNWDataNode The networking data node of the OCI config JSON.
   Dobby Node:
            {
                "ipv4": true,
                "ipv6": <optional bool>,
                "portForwarding": {
                    "hostToContainer": [
                        {
                            "port": 1234,
                            "protocol": "tcp"
                        }
                    ],
                    "containerToHost": [
                        {
                            "port": 1234,
                            "protocol": "tcp"
                        }
                    ],
                    "localhostMasquerade": <optional bool>
                },
                "multicastForwarding": [
                    {
                        "ip": "239.255.255.250",
                        "port": 1900
                    }
                ],
                "interContainer": [
                    {
                        "direction": "in",
                        "port": 12345,
                        "protocol": "tcp",
                        "localhostMasquerade": <optional bool>
                    }
                ]
            }
 * @param dobbyNWCfgObject The Dobby network configuration object to be added to the networking data node.
 * @return true if the update was successful, false otherwise.
 */
bool updategetNetworkingDataNode(Json::Value& ociConfigNWDataNode, const Json::Value& dobbyNWCfgObject)
{
    if (!dobbyNWCfgObject.isObject() || !ociConfigNWDataNode.isObject())
    {
        LOGERR("%s: Invalid input parameters for updating networking data node.", MODULE_LOGTAG);
        return false;
    }

    // 1. Process portForwarding rules
    if (dobbyNWCfgObject.isMember(PORT_FORWARDING))
    {
        const Json::Value& dobbyPortForwarding = dobbyNWCfgObject[PORT_FORWARDING];
        if (dobbyPortForwarding.isObject())
        {
            Json::Value& ociPortForwarding = ociConfigNWDataNode[PORT_FORWARDING];
            if (!ociPortForwarding.isObject())
            {
                ociPortForwarding = Json::Value(Json::objectValue);
            }

            // portForwarding.LocalhostMasquerade check
            if (dobbyPortForwarding.get(LOCALHOST_MASQUERADE, false).asBool())
            {
                ociPortForwarding[LOCALHOST_MASQUERADE] = true;
            }

            // portForwarding.HostToContainer rules
            if (dobbyPortForwarding.isMember(HOST_TO_CONTAINER))
            {
                const Json::Value& hostToContRules = dobbyPortForwarding[HOST_TO_CONTAINER];
                if (hostToContRules.isArray())
                {
                    Json::Value& ociHostToContainer = ociPortForwarding[HOST_TO_CONTAINER];
                    if (!ociHostToContainer.isArray())
                    {
                        ociHostToContainer = Json::Value(Json::arrayValue);
                    }

                    for (const auto& rule : hostToContRules)
                    {
                        if (rule.isObject() && rule.isMember(ralf::PORT) && rule.isMember(ralf::PROTOCOL))
                        {
                            addContainerToHostRuleIfMissing(ociHostToContainer, rule[ralf::PORT].asUInt(), rule[ralf::PROTOCOL].asString());
                        }
                    }
                }
            }

            // ContainerToHost rules
            if (dobbyPortForwarding.isMember(CONTAINER_TO_HOST))
            {
                const Json::Value& contToHostRules = dobbyPortForwarding[CONTAINER_TO_HOST];
                if (contToHostRules.isArray())
                {
                    Json::Value& ociContainerToHost = ociPortForwarding[CONTAINER_TO_HOST];
                    if (!ociContainerToHost.isArray())
                    {
                        ociContainerToHost = Json::Value(Json::arrayValue);
                    }

                    for (const auto& rule : contToHostRules)
                    {
                        if (rule.isObject() && rule.isMember(ralf::PORT) && rule.isMember(ralf::PROTOCOL))
                        {
                            addContainerToHostRuleIfMissing(ociContainerToHost, rule[ralf::PORT].asUInt(), rule[ralf::PROTOCOL].asString());
                        }
                    }
                }
            }
        }
    }

    // 2. Process multicastForwarding rules
    // Note: RALF spec does not define multicastForwarding, but we will process it if present in the input.
    if (dobbyNWCfgObject.isMember(MULTICAST_FORWARDING))
    {
        const Json::Value& multicastRules = dobbyNWCfgObject[MULTICAST_FORWARDING];
        if (multicastRules.isArray())
        {
            LOGWARN("%s: SPEC CHANGED?, Processing multicastForwarding rules from RALF NW cfg.", MODULE_LOGTAG);
            Json::Value& ociMulticastForwarding = ociConfigNWDataNode[MULTICAST_FORWARDING];
            if (!ociMulticastForwarding.isArray())
            {
                ociMulticastForwarding = Json::Value(Json::arrayValue);
            }

            for (const auto& rule : multicastRules)
            {
                if (!rule.isObject() || !rule.isMember(IP) || !rule.isMember(ralf::PORT))
                    continue;

                std::string targetIp = rule[IP].asString();
                unsigned int targetPort = rule[ralf::PORT].asUInt();
                bool duplicateFound = false;

                for (const auto& existingRule : ociMulticastForwarding)
                {
                    if (existingRule.isObject() && existingRule.isMember(IP) && existingRule.isMember(ralf::PORT))
                    {
                        if (existingRule[ralf::PORT].asUInt() == targetPort && existingRule[IP].asString() == targetIp)
                        {
                            duplicateFound = true;
                            break;
                        }
                    }
                }

                if (!duplicateFound)
                {
                    ociMulticastForwarding.append(rule);
                }
            }
        }
    }

    // 3. Process interContainer rules
    if (dobbyNWCfgObject.isMember(INTER_CONTAINER))
    {
        const Json::Value& interContRules = dobbyNWCfgObject[INTER_CONTAINER];
        if (interContRules.isArray())
        {
            Json::Value& ociInterContainer = ociConfigNWDataNode[INTER_CONTAINER];
            if (!ociInterContainer.isArray())
            {
                ociInterContainer = Json::Value(Json::arrayValue);
            }

            for (const auto& rule : interContRules)
            {
                if (!rule.isObject() || !rule.isMember(DIRECTION) || !rule.isMember(ralf::PORT) || !rule.isMember(ralf::PROTOCOL))
                    continue;

                std::string targetDir = rule[DIRECTION].asString();
                unsigned int targetPort = rule[ralf::PORT].asUInt();
                std::string targetProto = rule[ralf::PROTOCOL].asString();
                bool targetHasMasq = rule.get(LOCALHOST_MASQUERADE, false).asBool();
                bool duplicateFound = false;

                for (const auto& existingRule : ociInterContainer)
                {
                    if (!existingRule.isObject() || !existingRule.isMember(ralf::PORT))
                        continue;

                    // Fast integer check first
                    if (existingRule[ralf::PORT].asUInt() != targetPort)
                        continue;

                    if (existingRule.isMember(DIRECTION) && existingRule.isMember(ralf::PROTOCOL))
                    {
                        if (existingRule[DIRECTION].asString() == targetDir &&
                            existingRule[ralf::PROTOCOL].asString() == targetProto &&
                            existingRule.get(LOCALHOST_MASQUERADE, false).asBool() == targetHasMasq)
                        {
                            duplicateFound = true;
                            break;
                        }
                    }
                }

                if (!duplicateFound)
                {
                    ociInterContainer.append(rule);
                }
            }
        }
    }

    return true;
}

/**
 * @brief Translates a RALF network configuration model object into a Dobby networking plugin schema format.
 *
 * Iterates through standard RALF items containing public, exported, or imported rules. Maps
 * them into their functional Dobby equivalents (hostToContainer, interContainer in/out) while
 * tracking deduplication and dynamically removing empty JSON structural wrappers.
 *
 * @param[in] ralfNWCfgObject The source network configuration JSON payload (can be an Object or Array).
 *
 * @return Json::Value An object populated with the translated Dobby format networking configuration.
 */
Json::Value translateRALFNWCfgObjToDobbyNWCfgObj(const Json::Value& ralfNWCfgObject)
{
    Json::Value dobbyNWCfgObject(Json::objectValue);

    if (!ralfNWCfgObject.isArray() && !ralfNWCfgObject.isObject())
    {
        LOGERR("%s: Invalid RALF network configuration object type.", MODULE_LOGTAG);
        return dobbyNWCfgObject;
    }

    // Pre-initialize basic structural containers
    Json::Value& portForwarding = dobbyNWCfgObject[PORT_FORWARDING] = Json::Value(Json::objectValue);
    Json::Value& hostToContainer = portForwarding[HOST_TO_CONTAINER] = Json::Value(Json::arrayValue);
    Json::Value& interContainer = dobbyNWCfgObject[INTER_CONTAINER] = Json::Value(Json::arrayValue);

    // Set fallback global localhostMasquerade helper
    portForwarding[LOCALHOST_MASQUERADE] = true;

    auto processItem = [&](const Json::Value& ralfItem) {
        // Enforce basic element verification
        if (!ralfItem.isObject() || !ralfItem.isMember(ralf::PORT) || !ralfItem.isMember(ralf::TYPE) ||
            !ralfItem[ralf::PORT].isUInt() || !ralfItem[ralf::TYPE].isString())
        {
            LOGWARN("%s: Invalid RALF network configuration item.", MODULE_LOGTAG);
            return;
        }

        uint32_t port = ralfItem[ralf::PORT].asUInt();
        std::string type = ralfItem[ralf::TYPE].asString();

        if (port == 0 || port > 65535 || (PUBLIC != type && EXPORTED != type && IMPORTED != type))
        {
            LOGWARN("%s: Invalid port %u or type '%s'; skipping item.", MODULE_LOGTAG, port, type.c_str());
            return;
        }

        // Handle string protocol mappings safely
        std::string protocol = DEFAULT_PROTOCOL;
        if (ralfItem.isMember(ralf::PROTOCOL) && ralfItem[ralf::PROTOCOL].isString())
        {
            protocol = normalizeProtocol(ralfItem[ralf::PROTOCOL].asString());
        }

        if (PUBLIC == type)
        {
            Json::Value rule(Json::objectValue);
            rule[ralf::PORT] = port;
            rule[ralf::PROTOCOL] = protocol;
            appendIfUnique(hostToContainer, rule);
        }
        else if (EXPORTED == type)
        {
            Json::Value rule(Json::objectValue);
            rule[DIRECTION] = DIRECTION_IN;
            rule[ralf::PORT] = port;
            rule[ralf::PROTOCOL] = protocol;
            rule[LOCALHOST_MASQUERADE] = true;
            appendIfUnique(interContainer, rule);
        }
        else if (IMPORTED == type)
        {
            Json::Value rule(Json::objectValue);
            rule[DIRECTION] = DIRECTION_OUT;
            rule[ralf::PORT] = port;
            rule[ralf::PROTOCOL] = protocol;
            rule[LOCALHOST_MASQUERADE] = true;
            appendIfUnique(interContainer, rule);
        }
    };

    // Traverse structural layout arrays cleanly
    if (ralfNWCfgObject.isArray())
    {
        for (const auto& item : ralfNWCfgObject)
        {
            processItem(item);
        }
    }
    else
    {
        processItem(ralfNWCfgObject);
    }

    // Clean up empty tracking members to ensure clean, valid Dobby JSON output structure
    if (hostToContainer.empty()) portForwarding.removeMember(HOST_TO_CONTAINER);

    if (portForwarding.empty() || (portForwarding.size() == 1 && portForwarding.isMember(LOCALHOST_MASQUERADE)))
    {
        dobbyNWCfgObject.removeMember(PORT_FORWARDING);
    }
    if (interContainer.empty()) dobbyNWCfgObject.removeMember(INTER_CONTAINER);

    return dobbyNWCfgObject;
}

/**
 * @brief Updates the OCI configuration with network configuration from the manifest.
 * @param ociConfigRootNode The root node of the OCI configuration JSON.
 * @param manifestRootNode The root node of the manifest JSON.
 * @return true if the update was successful, false otherwise.
 */
bool updateNetworkConfigurationNode(Json::Value& ociConfigRootNode, const Json::Value& manifestRootNode)
{
    // Handle "configuration" node in the manifest, which may contain "urn:rdk:config:network" metadata.
    if (!manifestRootNode.isMember(ralf::CONFIGURATION))
    {
        LOGWARN("%s: No configuration node found in manifest; skipping network configuration update", MODULE_LOGTAG);
        return true; // Not an error; just no configuration to process
    }

    const Json::Value& configurationNode = manifestRootNode[ralf::CONFIGURATION];
    if (!configurationNode.isMember(ralf::NETWORK_CONFIG_URN))
    {
        LOGWARN("%s: No network configuration node found in manifest; skipping network configuration update", MODULE_LOGTAG);
        return true; // Not an error; just no configuration to process
    }

    /**
     * Reference: https://github.com/rdkcentral/oci-package-spec/blob/main/metadata.md#urnrdkconfignetwork (Jun 1, 2026)
     * Schema:
        {
          "$schema": "https://json-schema.org",
          "title": "RDK Network Services Configuration Schema",
          "description": "Network services configuration mapping to firewall rules applied to the app or service container.",
          "type": "object",
          "properties": {
            "urn:rdk:config:network": {
              "description": "Network services configuration.",
              "type": "array",
              "items": {
                "type": "object",
                "properties": {
                  "name": {
                    "type": "string"
                  },
                  "port": {
                    "type": "integer"
                  },
                  "protocol": {
                    "type": "string"
                  },
                  "type": {
                    "type": "string"
                  }
                },
                "required": [ "name", "port", "protocol", "type" ]
              }
            }
          }
        }
     */

    const Json::Value& networkConfiguration = configurationNode[ralf::NETWORK_CONFIG_URN];
    if (!networkConfiguration.isArray())
    {
        LOGWARN("%s: Network configuration is not an array, no need to process it.", MODULE_LOGTAG);
        return false;
    }

    if (networkConfiguration.empty())
    {
        LOGDBG("%s: Network configuration is empty, skipping it.", MODULE_LOGTAG);
        return true; // Not an error; just no configuration to process
    }

    // Retrieve or create the networking data node in the OCI config.
    // All FIREBOLT RALF apps require a networking data node to connect to FIREBOLT endpoint.
    Json::Value* netData = nullptr;
    try {
        netData = getNetworkingDataNode(ociConfigRootNode, true);
    } catch (const Json::LogicError& e) {
        LOGERR("%s: Exception Json::LogicError: %s", MODULE_LOGTAG, e.what());
        netData = nullptr;
        return false;
    } catch (const std::exception& e) {
        LOGERR("%s: Exception std::exception: %s", MODULE_LOGTAG, e.what());
        netData = nullptr;
        return false;
    } catch (...) {
        LOGERR("%s: Unknown exception while retrieving networking data node.", MODULE_LOGTAG);
        netData = nullptr;
        return false;
    }

    if (nullptr == netData)
    {
        LOGERR("%s: Failed to retrieve/create networking data node for network configuration update.", MODULE_LOGTAG);
        return false;
    }

    // Batch-process network configuration array entries
    Json::Value dobbyNWCfgObject = translateRALFNWCfgObjToDobbyNWCfgObj(networkConfiguration);
    if (dobbyNWCfgObject.empty())
    {
        LOGWARN("%s: Translated Dobby network configuration object is empty.", MODULE_LOGTAG);
        return true;
    }

    return updategetNetworkingDataNode(*netData, dobbyNWCfgObject);
}

/**
 * @brief Updates the OCI configuration with network settings based on permissions specified in the manifest.
 * @param ociConfigRootNode The root node of the OCI configuration JSON.
 * @param manifestRootNode The root node of the manifest JSON.
 * @param envVariables The serialized JSON array string of environment variables as provided by RuntimeConfig.envVariables.
 * @return true if the update was successful or if there were no permissions to process; false on error.
 */
bool updatePermissionBasedNetworkConfiguration(Json::Value& ociConfigRootNode, const Json::Value& manifestRootNode,
                                               const std::string& envVariables)
{
    if (!manifestRootNode.isMember(ralf::PERMISSIONS))
    {
        LOGWARN("%s: No permissions found in manifest; skipping permission-based networking update", MODULE_LOGTAG);
        return true; // Not an error; just no permissions to process
    }

    const Json::Value& permissions = manifestRootNode[ralf::PERMISSIONS];
    if (!permissions.isArray())
    {
        LOGWARN("%s: Permissions node is not an array; skipping permission-based networking update", MODULE_LOGTAG);
        return true; // Not an error; just no permissions to process
    }

    bool hasPermissionInternet = false;
    bool hasPermissionFirebolt = false;
    bool hasPermissionThunder = false;
    bool updatedInternetNwCfg = false;
    bool updatedFireboltNwCfg = false;
    bool updatedThunderNwCfg = false;

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

    Json::Value* netData = getNetworkingDataNode(ociConfigRootNode, true);

    if ((hasPermissionInternet || hasPermissionFirebolt || hasPermissionThunder) && (nullptr == netData))
    {
        LOGERR("%s: Failed to retrieve/create networking data node for permission-based update.", MODULE_LOGTAG);
        return false;
    }

    if (hasPermissionInternet || hasPermissionFirebolt || hasPermissionThunder)
    {
        if (hasPermissionInternet)
        {
            updatedInternetNwCfg = true;
        }

        // check and update if not matching.
        if (!netData->isMember(ralf::TYPE) || !(*netData)[ralf::TYPE].isString() || (*netData)[ralf::TYPE].asString() != NETWORK_TYPE_NAT)
        {
            (*netData)[ralf::TYPE] = NETWORK_TYPE_NAT;
        }
        if (!netData->isMember(DNSMASQ) || !(*netData)[DNSMASQ].isBool() || !(*netData)[DNSMASQ].asBool())
        {
            (*netData)[DNSMASQ] = true;
        }
        if (!netData->isMember(NETWORK_IPV4) || !(*netData)[NETWORK_IPV4].isBool() || !(*netData)[NETWORK_IPV4].asBool())
        {
            (*netData)[NETWORK_IPV4] = true;
        }
        if (!netData->isMember(NETWORK_IPV6) || !(*netData)[NETWORK_IPV6].isBool() || !(*netData)[NETWORK_IPV6].asBool())
        {
            (*netData)[NETWORK_IPV6] = true;
        }
    }

    // Array type RALF network configuration object for Thunder & Firebolt.
    Json::Value ralfLocalNWCfgObject(Json::arrayValue);

    if (hasPermissionFirebolt && !envVariables.empty())
    {
        /**
         * The envVariables string is a serialized form of JSON value .. An example is
         * ["FIREBOLT_ENDPOINT=http:\/\/127.0.0.1:3473?session=810b474c-5f68-4cdf-82f2-86dc4d6d1f97","TARGET_STATE=4"]
         * We need to parse it and get the FIREBOLT_ENDPOINT string.
         */

        Json::CharReaderBuilder readerBuilder;
        Json::Value envVarsNode;
        std::string errs;
        std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
        if (!reader->parse(envVariables.c_str(), envVariables.c_str() + envVariables.size(), &envVarsNode, &errs))
        {
            LOGERR("Failed to parse env variables JSON string, error: %s\n", errs.c_str());
            return false;
        }

        if (envVarsNode.isArray())
        {
            for (const auto &envEntry : envVarsNode)
            {
                if (envEntry.isString())
                {
                    std::string envPair = envEntry.asString();
                    std::string fireboltPrefix = std::string(ralf::FIREBOLT_ENDPOINT_ENV_KEY) + "=";
                    if (envPair.rfind(fireboltPrefix, 0) == 0)
                    {
                        std::string fireboltEndpointStr = envPair.substr(fireboltPrefix.size());
                        LOGDBG("Found FIREBOLT_ENDPOINT: %s\n", fireboltEndpointStr.c_str());
                        bool isLoopback = isLoopbackEndpoint(fireboltEndpointStr);
                        std::string normalizedProtocol = normalizeProtocol(fireboltEndpointStr);
                        const int port = extractPortFromEndpoint(fireboltEndpointStr);
                        if ((port != -1) && isLoopback) // Valid port extracted and is loopback
                        {
                            // Firebolt is running on host and container needs to access it.
                            // Construct a RALF network configuration object for Firebolt and insert into ralfLocalNWCfgObject
                            Json::Value fireboltNWCfgObject(Json::objectValue);
                            fireboltNWCfgObject[ralf::NAME] = "Firebolt";
                            fireboltNWCfgObject[ralf::PORT] = port;
                            fireboltNWCfgObject[ralf::PROTOCOL] = normalizedProtocol.c_str();
                            fireboltNWCfgObject[ralf::TYPE] = IMPORTED;
                            ralfLocalNWCfgObject.append(fireboltNWCfgObject);
                        }
                        break;
                    }
                }
            }
        }
        LOGWARN("FIREBOLT_ENDPOINT environment variable not found in runtime config\n");
        return false;
    }

    if (hasPermissionThunder)
    {
        const char* thunderaccess = getenv(ralf::THUNDER_ACCESS_ENV_KEY);
        if (nullptr != thunderaccess) {
            bool isLoopback = isLoopbackEndpoint(thunderaccess);
            std::string normalizedProtocol = normalizeProtocol(thunderaccess);
            const int port = extractPortFromEndpoint(thunderaccess);
            if ((port != -1) && isLoopback)  // Valid port extracted and is loopback
            {
                // Thunder is running on host and container needs to access it.
                // Construct a RALF network configuration object for Thunder and insert into ralfLocalNWCfgObject
                Json::Value thunderNWCfgObject(Json::objectValue);
                thunderNWCfgObject[ralf::NAME] = "Thunder";
                thunderNWCfgObject[ralf::PORT] = port;
                thunderNWCfgObject[ralf::PROTOCOL] = normalizedProtocol.c_str();
                thunderNWCfgObject[ralf::TYPE] = IMPORTED; // Thunder is an imported service for the container
                ralfLocalNWCfgObject.append(thunderNWCfgObject);
            }
            else
            {
                LOGWARN("%s: Invalid Thunder endpoint(port:%d, isLoopback:%d), skipping addition.",
                        MODULE_LOGTAG, port, isLoopback);
            }
        }
    }

    if (ralfLocalNWCfgObject.empty())
    {
        LOGWARN("%s: No valid RALF network configuration objects generated for permission-based update.", MODULE_LOGTAG);
        return true;
    }

    Json::Value dobbyLocalNWCfgObject = translateRALFNWCfgObjToDobbyNWCfgObj(ralfLocalNWCfgObject);
    if (dobbyLocalNWCfgObject.empty())
    {
        LOGWARN("%s: Translated Dobby network configuration object is empty.", MODULE_LOGTAG);
        return false;
    }

    return updategetNetworkingDataNode(*netData, dobbyLocalNWCfgObject);
}
} // namespace NetworkConfigurationHelper
