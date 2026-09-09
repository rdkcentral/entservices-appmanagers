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
                       MODULE_LOGTAG, ralf::RDKPLUGINS,NETWORKING);
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
     * @brief Normalizes the given protocol to either "tcp" or "udp".
     * @param protocol The protocol string to normalize.
     * @return "tcp" or "udp" if the protocol is recognized, otherwise DEFAULT_PROTOCOL.
     */
    const char* normalizeProtocol(const std::string& protocol)
    {
        // Quick length filter to instantly bypass long string comparisons
        const size_t len = protocol.size();
        if (len < 2 || len > 5) {
            LOGWARN("%s: Unknown protocol '%s'; defaulting to '%s'", MODULE_LOGTAG, protocol.c_str(), DEFAULT_PROTOCOL);
            return DEFAULT_PROTOCOL;
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

        LOGWARN("%s: Unknown protocol '%s'; defaulting to '%s'", MODULE_LOGTAG, protocol.c_str(), DEFAULT_PROTOCOL);
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

    /**
     * @brief Adds a container-to-host port forwarding rule to the provided JSON array.
     * @param containerToHost The JSON array representing container-to-host port forwarding rules.
     * @param port The port number for the rule.
     * @param protocol The protocol for the rule (default is "tcp").
     */
    void addContainerToHostRuleIfMissing(Json::Value& containerToHost, const uint32_t port, const std::string& protocol = DEFAULT_PROTOCOL)
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
 */
bool updategetNetworkingDataNode(Json::Value& ociConfigNWDataNode, const Json::Value& dobbyNWCfgObject)
{
    if (!dobbyNWCfgObject.isObject() || !ociConfigNWDataNode.isObject())
    {
        LOGERR("%s: Invalid input parameters for updating networking data node.", MODULE_LOGTAG);
        return false;
    }

    // Process portForwarding rules
    if (const Json::Value* dobbyPortForwarding = dobbyNWCfgObject.find(PORT_FORWARDING))
    {
        if (dobbyPortForwarding->isObject())
        {
            Json::Value& ociPortForwarding = ociConfigNWDataNode[PORT_FORWARDING];
            if (!ociPortForwarding.isObject())
            {
                ociPortForwarding = Json::Value(Json::objectValue);
            }

            // LocalhostMasquerade check
            if (const Json::Value* masqVal = dobbyPortForwarding->find(LOCALHOST_MASQUERADE))
            {
                if (masqVal->isBool() && masqVal->asBool())
                {
                    ociPortForwarding[LOCALHOST_MASQUERADE] = true;
                }
            }

            // HostToContainer rules
            if (const Json::Value* hostToContRules = dobbyPortForwarding->find(HOST_TO_CONTAINER))
            {
                if (hostToContRules->isArray())
                {
                    Json::Value& ociHostToContainer = ociPortForwarding[HOST_TO_CONTAINER];
                    if (!ociHostToContainer.isArray())
                    {
                        ociHostToContainer = Json::Value(Json::arrayValue);
                    }

                    for (const auto& rule : *hostToContRules)
                    {
                        if (rule.isObject())
                        {
                            const Json::Value* portVal = rule.find(PORT);
                            const Json::Value* protoVal = rule.find(PROTOCOL);
                            if (portVal && protoVal)
                            {
                                addContainerToHostRuleIfMissing(ociHostToContainer, portVal->asUInt(), protoVal->asString());
                            }
                        }
                    }
                }
            }

            // ContainerToHost rules
            if (const Json::Value* contToHostRules = dobbyPortForwarding->find(CONTAINER_TO_HOST))
            {
                if (contToHostRules->isArray())
                {
                    Json::Value& ociContainerToHost = ociPortForwarding[CONTAINER_TO_HOST];
                    if (!ociContainerToHost.isArray())
                    {
                        ociContainerToHost = Json::Value(Json::arrayValue);
                    }

                    for (const auto& rule : *contToHostRules)
                    {
                        if (rule.isObject())
                        {
                            const Json::Value* portVal = rule.find(PORT);
                            const Json::Value* protoVal = rule.find(PROTOCOL);
                            if (portVal && protoVal)
                            {
                                addContainerToHostRuleIfMissing(ociContainerToHost, portVal->asUInt(), protoVal->asString());
                            }
                        }
                    }
                }
            }
        }
    }

    // Process multicastForwarding rules
    // RALF Spec does not expose any multicast options, but process them if they exist in the Dobby config.
    if (const Json::Value* multicastRules = dobbyNWCfgObject.find(MULTICAST_FORWARDING))
    {
        if (multicastRules->isArray())
        {
            LOGWARN("%s: SPEC CHANGED?, Processing multicastForwarding rules from RALF NW cfg.", MODULE_LOGTAG);
            Json::Value& ociMulticastForwarding = ociConfigNWDataNode[MULTICAST_FORWARDING];
            if (!ociMulticastForwarding.isArray())
            {
                ociMulticastForwarding = Json::Value(Json::arrayValue);
            }

            for (const auto& rule : *multicastRules)
            {
                if (rule.isObject())
                {
                    const Json::Value* ipVal = rule.find(IP);
                    const Json::Value* portVal = rule.find(PORT);
                    if (ipVal && portVal)
                    {
                        // Cache values locally once to avoid string conversions inside the loop
                        std::string targetIp = ipVal->asString();
                        unsigned int targetPort = portVal->asUInt();
                        bool duplicateFound = false;

                        for (const auto& existingRule : ociMulticastForwarding)
                        {
                            if (existingRule.isObject())
                            {
                                const Json::Value* eIp = existingRule.find(IP);
                                const Json::Value* ePort = existingRule.find(PORT);
                                // Direct scalar comparisons are highly efficient
                                if (eIp && ePort && ePort->asUInt() == targetPort && eIp->asString() == targetIp)
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
        }
    }

    // Process interContainer rules
    if (const Json::Value* interContRules = dobbyNWCfgObject.find(INTER_CONTAINER))
    {
        if (interContRules->isArray())
        {
            Json::Value& ociInterContainer = ociConfigNWDataNode[INTER_CONTAINER];
            if (!ociInterContainer.isArray())
            {
                ociInterContainer = Json::Value(Json::arrayValue);
            }

            for (const auto& rule : *interContRules)
            {
                if (rule.isObject())
                {
                    const Json::Value* dirVal = rule.find(DIRECTION);
                    const Json::Value* portVal = rule.find(PORT);
                    const Json::Value* protoVal = rule.find(PROTOCOL);

                    if (dirVal && portVal && protoVal)
                    {
                        // Cache input string/scalar values locally
                        std::string targetDir = dirVal->asString();
                        unsigned int targetPort = portVal->asUInt();
                        std::string targetProto = protoVal->asString();

                        const Json::Value* masqVal = rule.find(LOCALHOST_MASQUERADE);
                        bool targetHasMasq = (masqVal && masqVal->isBool()) ? masqVal->asBool() : false;

                        bool duplicateFound = false;
                        for (const auto& existingRule : ociInterContainer)
                        {
                            if (existingRule.isObject())
                            {
                                const Json::Value* ePort = existingRule.find(PORT);
                                // Quickest integer-first exit check
                                if (ePort && ePort->asUInt() == targetPort)
                                {
                                    const Json::Value* eDir = existingRule.find(DIRECTION);
                                    const Json::Value* eProto = existingRule.find(PROTOCOL);

                                    if (eDir && eProto && eDir->asString() == targetDir && eProto->asString() == targetProto)
                                    {
                                        const Json::Value* eMasq = existingRule.find(LOCALHOST_MASQUERADE);
                                        bool existingHasMasq = (eMasq && eMasq->isBool()) ? eMasq->asBool() : false;

                                        if (targetHasMasq == existingHasMasq)
                                        {
                                            duplicateFound = true;
                                            break;
                                        }
                                    }
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
        }
    }

    return true;
}

/**
 * @brief @brief Converts a RALF network configuration object to a Dobby network configuration object.
 * @param ralfNWCfgObject The RALF network configuration array or single object node.
 * Input RALF Node::
      {
        "name": "netflix-mdx",
        "port": 8009,
        "protocol": "tcp",
        "type": "public"
      },
      {
        "name": "com.example.myapp.service",
        "port": 1234,
        "protocol": "tcp",
        "type": "exported"
      },
      {
        "name": "com.example.someotherapp.service",
        "port": 4567,
        "protocol": "tcp",
        "type": "imported"
      }
public: Network services that an app or service exposes outside the device.
exported: Service that an app or service exposes to other apps or services on the device.
imported: Network services supplied by another app or service that the current app requires access to.

 * Output Dobby Node:
    "portForwarding": {
        "hostToContainer": [
            {
                "port": 1234,
                "protocol": "tcp"
            },
            {
                "port": 5678,
                "protocol": "udp"
            }
        ],
        "containerToHost": [
            {
                "port": 1234,
                "protocol": "tcp"
            },
            {
                "port": 5678,
                "protocol": "udp"
            }
        ],
        "localhostMasquerade": true
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
            "localhostMasquerade": true
        },
        {
            "direction": "out",
            "port": 2468,
            "protocol": "tcp"
        }
    ]

 portForwarding:
    protocol: The protocol for the port forwarding rule (e.g., "tcp" or "udp"); optional and defaults to "tcp" if not specified.
    hostToContainer: forwards incoming packets to specified port on the host to the container. (allow containered processes to run servers)
    containerToHost: allow containers access to the host over certain ports. Adds firewall rules to allow containers to access the specified port(s) on the host via the bridge device.
    localhostMasquerade: If enabled, redirect packets sent to localhost in the container to the host's localhost (via the dobby bridge device) for the forwarded ports. Allows containers to access services on the host without needing to change existing code to point to the bridge IP address. This can obviously only work for ports specified in the containerToHost section.

 Multicast Forwarding: allows containered processes to receive multicast traffic from specified address/port combinations.
    multicastForwarding.ip and multicastForwarding.port fields are both required for each forwarded multicast address.

 interContainer: allows containers to communicate.
    direction: "in" or "out" to specify the direction of the inter-container communication.
    port: The port number for the inter-container communication.
    protocol: The protocol for the inter-container communication (e.g., "tcp" or "udp").
    localhostMasquerade: allows the server container to bind to localhost. For the client container, it allows connecting to localhost, forwarding the connection to the server container.
 * @return A Dobby network configuration object (empty object {} if validation fails).
 */
Json::Value translateRALFNWCfgObjToDobbyNWCfgObj(const Json::Value& ralfNWCfgObject)
{
    // Allocated on the stack per function call - safe for looping and avoids cross-contamination
    Json::Value dobbyNWCfgObject(Json::objectValue);

    bool isArray = ralfNWCfgObject.isArray();
    if (!isArray && !ralfNWCfgObject.isObject())
    {
        LOGERR("%s: Invalid RALF network configuration object type.", MODULE_LOGTAG);
        return dobbyNWCfgObject;
    }

    // Pre-initialize basic structural containers to avoid nested lookups later
    Json::Value& portForwarding = dobbyNWCfgObject[PORT_FORWARDING] = Json::Value(Json::objectValue);
    Json::Value& hostToContainer = portForwarding[HOST_TO_CONTAINER] = Json::Value(Json::arrayValue);
    Json::Value& interContainer = dobbyNWCfgObject[INTER_CONTAINER] = Json::Value(Json::arrayValue);

    // Set global localhostMasquerade helper
    portForwarding[LOCALHOST_MASQUERADE] = true;

    // Direct loop handling depending on input layout structure
    auto processItem = [&](const Json::Value& ralfItem) {
        if (!ralfItem.isObject()) return;

        const Json::Value* portVal = ralfItem.find(PORT);
        const Json::Value* typeVal = ralfItem.find(TYPE);
        if (!portVal || !typeVal) return;

        unsigned int port = portVal->asUInt();
        std::string type = typeVal->asString();

        // Protocol is optional and defaults to "tcp"
        const Json::Value* protoVal = ralfItem.find(PROTOCOL);
        std::string protocol = (protoVal && protoVal->isString()) ? protoVal->asString() : DEFAULT_PROTOCOL;

        if (type == PUBLIC)
        {
            Json::Value rule(Json::objectValue);
            rule[PORT] = port;
            rule[PROTOCOL] = protocol;
            hostToContainer.append(rule);
        }
        else if (type == EXPORTED)
        {
            Json::Value rule(Json::objectValue);
            rule[DIRECTION] = DIRECTION_IN;
            rule[PORT] = port;
            rule[PROTOCOL] = protocol;
            rule[LOCALHOST_MASQUERADE] = true;
            interContainer.append(rule);
        }
        else if (type == IMPORTED)
        {
            Json::Value rule(Json::objectValue);
            rule[DIRECTION] = DIRECTION_OUT;
            rule[PORT] = port;
            rule[PROTOCOL] = protocol;
            interContainer.append(rule);
        }
    };

    if (isArray)
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

    // Clean up empty tracking members to ensure clean, valid Dobby JSON output
    if (hostToContainer.empty())
    {
        portForwarding.removeMember(HOST_TO_CONTAINER);
    }
    if (portForwarding.empty())
    {
        dobbyNWCfgObject.removeMember(PORT_FORWARDING);
    }
    if (interContainer.empty())
    {
        dobbyNWCfgObject.removeMember(INTER_CONTAINER);
    }

    return dobbyNWCfgObject; // RVO optimization ensures this is highly performant
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
    if (!manifestRootNode.isMember(CONFIGURATION))
    {
        LOGWARN("%s: No configuration node found in manifest; skipping network configuration update", MODULE_LOGTAG);
        return true; // Not an error; just no configuration to process
    }

    const Json::Value& configurationNode = manifestRootNode[CONFIGURATION];
    if (!configurationNode.isMember(NETWORK_CONFIG_URN))
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

    const Json::Value& networkConfiguration = configurationNode[NETWORK_CONFIG_URN];
    if (!networkConfiguration.isArray())
    {
        LOGWARN("%s: Network configuration is not an array, no need to process it.", MODULE_LOGTAG);
        return false;
    }

    const Json::ArrayIndex configSize = networkConfiguration.size();
    if (0 == configSize)
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

    // Process each network configuration entry and update the OCI config accordingly.
    bool status = false;
    for (Json::ArrayIndex index = 0; index < configSize; ++index)
    {
        const Json::Value& ralfNWCfgObject = networkConfiguration[index];
        if (!ralfNWCfgObject.isObject())
        {
            LOGWARN("%s: Network entry is not an object, skipping it.", MODULE_LOGTAG);
            continue;
        }

        // Spec mandates that all entries must have "name", "port", "protocol", and "type" fields.
        // check if all "required" fields are present and matching the expected types.
        if (!ralfNWCfgObject.isMember(ralf::NAME) || !ralfNWCfgObject[ralf::NAME].isString() ||
            !ralfNWCfgObject.isMember(ralf::PORT) || !ralfNWCfgObject[ralf::PORT].isUInt() ||
            !ralfNWCfgObject.isMember(ralf::PROTOCOL) || !ralfNWCfgObject[ralf::PROTOCOL].isString() ||
            !ralfNWCfgObject.isMember(ralf::TYPE) || !ralfNWCfgObject[ralf::TYPE].isString())
        {
            LOGWARN("%s: Network entry is missing required fields or has incorrect types, skipping it.", MODULE_LOGTAG);
            continue;
        }

        // Convert RALFNWCfgObject to DobbyNWCfgObject
        Json::Value dobbyNWCfgObject = translateRALFNWCfgObjToDobbyNWCfgObj(ralfNWCfgObject);
        if (dobbyNWCfgObject.empty())
        {
            LOGWARN("%s: Failed to convert RALF network entry to Dobby format, skipping it.", MODULE_LOGTAG);
            continue;
        }
        status = updategetNetworkingDataNode(*netData, dobbyNWCfgObject);
        if (!status)
        {
            LOGWARN("%s: Failed to update networking data node with RALF network entry.", MODULE_LOGTAG);
        }
    }

    return status;
}

/**
 * @brief Updates the OCI configuration with network settings based on permissions specified in the manifest.
 * @param ociConfigRootNode The root node of the OCI configuration JSON.
 * @param manifestRootNode The root node of the manifest JSON.
 * @return true if the update was successful or if there were no permissions to process; false on error.
 */
bool updatePermissionBasedNetworkConfiguration(Json::Value& ociConfigRootNode, const Json::Value& manifestRootNode)
{
    if (!manifestRootNode.isMember(PERMISSIONS))
    {
        LOGWARN("%s: No permissions found in manifest; skipping permission-based networking update", MODULE_LOGTAG);
        return true; // Not an error; just no permissions to process
    }

    const Json::Value& permissions = manifestRootNode[PERMISSIONS];
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
        if (!netData->isMember(TYPE) || !(*netData)[TYPE].isString() || (*netData)[TYPE].asString() != NETWORK_TYPE_NAT)
        {
            (*netData)[TYPE] = NETWORK_TYPE_NAT;
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

    if (hasPermissionFirebolt)
    {
        /*
           We need to extract FIREBOLT_ENDPOINT configs from manifestRootNode.envVariables if present.
           The string is a serialized form of json value .. An example is
           ["FIREBOLT_ENDPOINT=http:\/\/127.0.0.1:3473?session=810b474c-5f68-4cdf-82f2-86dc4d6d1f97","TARGET_STATE=4"]
        */
        if (manifestRootNode.isMember(ENV_VARIABLES) && manifestRootNode[ENV_VARIABLES].isArray())
        {
            const Json::Value& envVariables = manifestRootNode[ENV_VARIABLES];
            const Json::ArrayIndex envSize = envVariables.size();
            for (Json::ArrayIndex index = 0; index < envSize; ++index)
            {
                const Json::Value& envEntry = envVariables[index];
                if (!envEntry.isString())
                {
                    continue;
                }

                const char* envPair = envEntry.asCString();
                const char* equalsSign = std::strchr(envPair, '=');
                if (equalsSign == nullptr)
                {
                    continue;
                }

                const size_t nameLen = equalsSign - envPair;
                if (nameLen == std::strlen(FIREBOLT_ENDPOINT_ENV_KEY) &&
                    std::strncmp(envPair, FIREBOLT_ENDPOINT_ENV_KEY, nameLen) == 0)
                {
                    const char* endpointValue = equalsSign + 1;
                    if (*endpointValue != '\0')
                    {
                        // Extract port from FIREBOLT_ENDPOINT environment variable and
                        // add to containerToHost rules
                        const int port = extractPortFromEndpoint(endpointValue);
                        if (port != -1) // Valid port extracted
                        {
                            Json::Value fireboltNWCfgObject(Json::objectValue);
                            fireboltNWCfgObject[ralf::NAME] = "firebolt";
                            fireboltNWCfgObject[ralf::PORT] = port;
                            fireboltNWCfgObject[ralf::PROTOCOL] = "tcp";
                            fireboltNWCfgObject[ralf::TYPE] = ralf::IMPORTED;
                            Json::Value dobbyNWCfgObject = translateRALFNWCfgObjToDobbyNWCfgObj(fireboltNWCfgObject);
                            if (dobbyNWCfgObject.empty()) {
                                LOGWARN("%s: translateRALFNWCfgObjToDobbyNWCfgObj error skipping it.", MODULE_LOGTAG);
                            }
                            updatedFireboltNwCfg = updategetNetworkingDataNode(*netData, dobbyNWCfgObject);
                            if (!updatedFireboltNwCfg)
                            {
                                LOGWARN("%s: Failed to update networking data node with Firebolt network entry.", MODULE_LOGTAG);
                            }
                        }
                        break;
                    }
                }
            }
        }
    }

    if (hasPermissionThunder)
    {
        const char* thunderaccess = getenv(THUNDER_ACCESS_ENV_KEY);
        if (nullptr != thunderaccess) {
            // extract port from THUNDER_ACCESS environment variable and
            // add to containerToHost rules
            const int port = extractPortFromEndpoint(thunderaccess);
            if (port != -1)  // Valid port extracted
            {
                // Thunder is running on host and container needs to
                // access it. Construct a RALF network configuration
                // object for Thunder and add it to the networking
                // data node.
                Json::Value thunderNWCfgObject(Json::objectValue);
                thunderNWCfgObject[ralf::NAME] = "thunder";
                thunderNWCfgObject[ralf::PORT] = port;
                thunderNWCfgObject[ralf::PROTOCOL] = "tcp";
                thunderNWCfgObject[ralf::TYPE] = ralf::IMPORTED;
                Json::Value dobbyNWCfgObject = translateRALFNWCfgObjToDobbyNWCfgObj(thunderNWCfgObject);
                if (dobbyNWCfgObject.empty()) {
                    LOGWARN("%s: translateRALFNWCfgObjToDobbyNWCfgObj error skipping it.", MODULE_LOGTAG);
                }
                updatedThunderNwCfg = updategetNetworkingDataNode(*netData, dobbyNWCfgObject);
                if (!updatedThunderNwCfg)
                {
                    LOGWARN("%s: Failed to update networking data node with Thunder network entry.", MODULE_LOGTAG);
                }
            }
        }
    }

     return (!hasPermissionThunder || updatedThunderNwCfg) &&
         (!hasPermissionFirebolt || updatedFireboltNwCfg) &&
         (!hasPermissionInternet || updatedInternetNwCfg);
}
} // namespace NetworkConfigurationHelper
