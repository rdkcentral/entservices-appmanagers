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
    constexpr const char *PUBLIC             = "public";
    constexpr const char *EXPORTED           = "exported";
    constexpr const char *IMPORTED           = "imported";
    constexpr const char *REQUIRED           = "required";
    constexpr const char *DIRECTION          = "direction";
    constexpr const char *IP                 = "ip";
    constexpr const char *DIRECTION_IN       = "in";
    constexpr const char *DIRECTION_OUT      = "out";
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
    // Internal translation hint (non-RALF key): imported endpoint is on host, not another container.
    // Dobby interContainer is for container-to-container routing over the bridge.
    // Host loopback access must use portForwarding.containerToHost (with localhostMasquerade).
    constexpr const char *HOST_ENDPOINT_MARKER = "_hostEndpoint";

    /**
     * @brief Navigates, allocates, or migrates the base connection settings of the Dobby module profile.
     * @param[in,out] ociConfigRootNode  The target root pointer of your global OCI map file.
     * @param[in]     createIfMissing   Determines if missing keys should be structurally instantiated.
     * @param[in]     enforceInternetDefaults  If true, the networking data node will be initialized to NAT/dnsmasq defaults.
     * @return Json::Value* Raw pointer addressing the parameters 'data' sub-node interface.
     */
    Json::Value* getNetworkingDataNode(Json::Value& ociConfigRootNode, const bool createIfMissing,
                                       const bool enforceInternetDefaults)
    {
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

        Json::Value* networking = nullptr;
        if (rdkPlugins->isMember(ralf::NETWORKING) && (*rdkPlugins)[ralf::NETWORKING].isObject())
        {
            networking = &(*rdkPlugins)[ralf::NETWORKING];
        }
        else
        {
            if (!createIfMissing)
            {
                LOGERR("%s: %s.%s node is missing/invalid type and could not create it.",
                       MODULE_LOGTAG, ralf::RDKPLUGINS, ralf::NETWORKING);
                return nullptr;
            }
            Json::Value& netRef = (*rdkPlugins)[ralf::NETWORKING] = Json::Value(Json::objectValue);
            netRef[REQUIRED] = true;
            networking = &netRef;
        }

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
                       MODULE_LOGTAG, ralf::RDKPLUGINS, ralf::NETWORKING, ralf::DATA);
                return nullptr;
            }
            (*networking)[ralf::DATA] = Json::Value(Json::objectValue);
            dataNode = &(*networking)[ralf::DATA];
        }

        if (!dataNode->isMember(ralf::TYPE))
        {
            if (!createIfMissing) {
                LOGERR("%s: %s.%s.%s.%s node is missing and could not create in OCI config.",
                        MODULE_LOGTAG, ralf::RDKPLUGINS, ralf::NETWORKING, ralf::DATA, ralf::TYPE);
                return nullptr;
            }
            (*dataNode)[ralf::TYPE] = NETWORK_TYPE_NONE;
            (*dataNode)[NETWORK_IPV4] = true;
            (*dataNode)[NETWORK_IPV6] = true;
            (*dataNode)[ralf::DNSMASQ] = false;
            if (enforceInternetDefaults)
            {
                (*dataNode)[ralf::TYPE] = NETWORK_TYPE_NAT;
                (*dataNode)[ralf::DNSMASQ] = true;
            }
        }
        else if (createIfMissing)
        {
            if (!(*dataNode)[ralf::TYPE].isString() ||
                NETWORK_TYPE_NAT != (*dataNode)[ralf::TYPE].asString())
            {
                (*dataNode)[ralf::TYPE] = NETWORK_TYPE_NONE;
            }
            (*dataNode)[NETWORK_IPV4] = true;
            (*dataNode)[NETWORK_IPV6] = true;
            (*dataNode)[ralf::DNSMASQ] = false;
            if (enforceInternetDefaults)
            {
                (*dataNode)[ralf::TYPE] = NETWORK_TYPE_NAT;
                (*dataNode)[ralf::DNSMASQ] = true;
            }
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
        // Port 0 is a special reserved port in networking (wildcard/ephemeral assignment) and is completely invalid
        // for a persistent Dobby container port-forwarding rule or an application loopback connection endpoint.
        if ((endPtr == startPtr) || (0 >= port) || (65535 < port) ||
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

        // Evaluation group for TCP-based protocols
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
                       (host_len == 17 && endpoint.compare(start_pos, 17, "[0:0:0:0:0:0:0:1]") == 0);
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
     * @brief Scans a Dobby rule array for duplicate configurations based on port and protocol.
     * @param[in] containerToHost The source rule layout array node to inspect.
     * @param[in] port            The integer port number we are searching for.
     * @param[in] protocol        The transport layer protocol string ("tcp" or "udp").
     * @return True if a matching duplicate rule exists, false otherwise.
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
            if (rule.isMember(ralf::PORT) && (rule[ralf::PORT].isUInt() ||
                (rule[ralf::PORT].isInt() && 0 <= rule[ralf::PORT].asInt())) &&
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
     * @brief Safely appends a unique port and protocol rule to the target containerToHost array if it is missing.
     * @param[in,out] containerToHost The source rule layout array node to modify.
     * @param[in]     port            The integer port number to append.
     * @param[in]     protocol        The transport layer protocol string ("tcp" or "udp").
     * @return void
     */
    void addContainerToHostRuleIfMissing(Json::Value& containerToHost, const uint32_t port, const std::string& protocol)
    {
        if (true == hasContainerToHostRule(containerToHost, port, protocol))
        {
            return;
        }

        Json::Value rule(Json::objectValue);
        rule[ralf::PORT] = port;
        rule[ralf::PROTOCOL] = protocol;
        containerToHost.append(rule);
    }

    /**
     * @brief Checks if a rule array contains duplicate constraints and appends elements safely.
     * @param[in,out] arrayContainer The target Json::Value vector layout container.
     * @param[in]     rule           The reference configuration array object.
     * @return void
     */
    void appendIfUnique(Json::Value& arrayContainer, const Json::Value& rule)
    {
        if (!arrayContainer.isArray())
        {
            return;
        }

        uint32_t port = rule[ralf::PORT].asUInt();
        std::string protocol = rule[ralf::PROTOCOL].asString();

        for (const auto& existingRule : arrayContainer)
        {
            if (existingRule.isMember(ralf::PORT) && existingRule[ralf::PORT].isUInt() &&
                existingRule.isMember(ralf::PROTOCOL) && existingRule[ralf::PROTOCOL].isString())
            {
                if (port == existingRule[ralf::PORT].asUInt() && protocol == existingRule[ralf::PROTOCOL].asString())
                {
                    return;
                }
            }
        }

        arrayContainer.append(rule);
    }

    /**
     * @brief Checks if a specific port & protocol rule exists in the Dobby portForwarding.containerToHost array.
     * @param[in] netData The root networking JSON object data node.
     * @param[in] targetPort The integer port number we are searching for.
     * @param[in] targetProto The normalized protocol string literal ("tcp" or "udp").
     * @return true if a duplicate rule is found, false otherwise.
     */
    static bool hasPortFwdContainerToHostRule(const Json::Value& netData, unsigned int targetPort,
                                              const std::string& targetProto)
    {
        if (netData.isObject() && netData.isMember(PORT_FORWARDING))
        {
            const Json::Value& portFwd = netData[PORT_FORWARDING];
            if (portFwd.isObject() && portFwd.isMember(CONTAINER_TO_HOST))
            {
                const Json::Value& cToHArray = portFwd[CONTAINER_TO_HOST];
                if (cToHArray.isArray())
                {
                    for (const auto& rule : cToHArray)
                    {
                        if (rule.isObject() && rule.isMember(ralf::PORT) &&
                            (rule[ralf::PORT].isInt() || rule[ralf::PORT].isUInt()) &&
                            rule.isMember(ralf::PROTOCOL) && rule[ralf::PROTOCOL].isString())
                        {
                            int existingPort = rule[ralf::PORT].asInt();
                            if (existingPort > 0 && existingPort <= 65535 &&
                                static_cast<unsigned int>(existingPort) == targetPort &&
                                targetProto == rule[ralf::PROTOCOL].asCString())
                            {
                                return true;
                            }
                        }
                    }
                }
            }
        }
        return false;
    }
}

using namespace ralf;
namespace NetworkConfigurationHelper
{

/**
 * @brief Merges intermediate structural parameters onto the validated rdkPlugins.networking.data sub-node.
 * @param[in,out] ociConfigNWDataNode The reference mapping straight onto rdkPlugins.networking.data.
 * @param[in] dobbyNWCfgObject The compiled input object containing rules data arrays.
 * @return True on successful injection tracking loops, false otherwise.
 */
bool updategetNetworkingDataNode(Json::Value& ociConfigNWDataNode, const Json::Value& dobbyNWCfgObject)
{
    if (false == dobbyNWCfgObject.isObject() || false == ociConfigNWDataNode.isObject())
    {
        LOGERR("%s: Invalid input parameters for updating networking data node.", MODULE_LOGTAG);
        return false;
    }

    // 1. Process portForwarding rules defensively
    if (dobbyNWCfgObject.isMember(PORT_FORWARDING))
    {
        const Json::Value& dobbyPortForwarding = dobbyNWCfgObject[PORT_FORWARDING];
        if (dobbyPortForwarding.isObject())
        {
            Json::Value& ociPortForwarding = ociConfigNWDataNode[PORT_FORWARDING];
            if (false == ociPortForwarding.isObject())
            {
                ociPortForwarding = Json::Value(Json::objectValue);
            }

            // Only create array if it doesn't exist; never reset it.
            if (dobbyPortForwarding.isMember(HOST_TO_CONTAINER))
            {
                const Json::Value& hostToContRules = dobbyPortForwarding[HOST_TO_CONTAINER];
                if (hostToContRules.isArray() && false == hostToContRules.empty())
                {
                    Json::Value& ociHostToContainer = ociPortForwarding[HOST_TO_CONTAINER];
                    if (false == ociHostToContainer.isArray())
                    {
                        ociHostToContainer = Json::Value(Json::arrayValue);
                    }

                    for (const auto& rule : hostToContRules)
                    {
                        if (rule.isObject() && rule.isMember(ralf::PORT) && rule.isMember(ralf::PROTOCOL))
                        {
                            uint32_t rPort = rule[ralf::PORT].asUInt();
                            std::string rProto = rule[ralf::PROTOCOL].asString();
                            bool duplicate = false;

                            for (const auto& existing : ociHostToContainer)
                            {
                                if (existing.isObject() && existing.isMember(ralf::PORT) && existing.isMember(ralf::PROTOCOL) &&
                                    rPort == existing[ralf::PORT].asUInt() && rProto == existing[ralf::PROTOCOL].asString())
                                {
                                    duplicate = true;
                                    break;
                                }
                            }
                            if (false == duplicate)
                            {
                                Json::Value newRule(Json::objectValue);
                                newRule[ralf::PORT] = rPort;
                                newRule[ralf::PROTOCOL] = rProto;
                                ociHostToContainer.append(newRule);
                            }
                        }
                    }
                }
            }

            // Merge instead of overwrite for containerToHost
            if (dobbyPortForwarding.isMember(CONTAINER_TO_HOST))
            {
                const Json::Value& contToHostRules = dobbyPortForwarding[CONTAINER_TO_HOST];
                if (contToHostRules.isArray() && false == contToHostRules.empty())
                {
                    Json::Value& ociContainerToHost = ociPortForwarding[CONTAINER_TO_HOST];
                    if (false == ociContainerToHost.isArray())
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
                    // Enable localhostMasquerade if any containerToHost rules exist
                    if (false == ociPortForwarding.get(LOCALHOST_MASQUERADE, false).asBool())
                    {
                        ociPortForwarding[LOCALHOST_MASQUERADE] = true;
                    }
                    else
                    {
                        // Nothing to do here.
                    }
                }
            }
        }
    }

    // 2. Process multicastForwarding rules defensively
    // Note: RALF spec does not define multicastForwarding, but we will process it if present in the input.
    if (dobbyNWCfgObject.isMember(MULTICAST_FORWARDING))
    {
        const Json::Value& multicastRules = dobbyNWCfgObject[MULTICAST_FORWARDING];
        if (multicastRules.isArray() && false == multicastRules.empty())
        {
            LOGWARN("%s: SPEC CHANGED?, Processing multicastForwarding rules from RALF NW cfg.", MODULE_LOGTAG);
            Json::Value& ociMulticastForwarding = ociConfigNWDataNode[MULTICAST_FORWARDING];
            if (false == ociMulticastForwarding.isArray())
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
                            LOGDBG("%s: Duplicate multicastForwarding rule found for IP %s and port %u; skipping.",
                                     MODULE_LOGTAG, targetIp.c_str(), targetPort);
                            duplicateFound = true;
                            break;
                        }
                    }
                }

                if (false == duplicateFound)
                {
                    ociMulticastForwarding.append(rule);
                }
            }
        }
    }

    // 3. Process interContainer rules defensively
    if (dobbyNWCfgObject.isMember(INTER_CONTAINER))
    {
        const Json::Value& interContRules = dobbyNWCfgObject[INTER_CONTAINER];
        if (interContRules.isArray() && false == interContRules.empty())
        {
            Json::Value& ociInterContainer = ociConfigNWDataNode[INTER_CONTAINER];
            if (false == ociInterContainer.isArray())
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

                    if (existingRule[ralf::PORT].asUInt() != targetPort)
                        continue;

                    if (existingRule.isMember(DIRECTION) && existingRule.isMember(ralf::PROTOCOL))
                    {
                        if (existingRule[DIRECTION].asString() == targetDir &&
                            existingRule[ralf::PROTOCOL].asString() == targetProto &&
                            existingRule.get(LOCALHOST_MASQUERADE, false).asBool() == targetHasMasq)
                        {
                            LOGDBG("%s: Duplicate interContainer rule found for direction %s, port %u, protocol %s; skipping.",
                                     MODULE_LOGTAG, targetDir.c_str(), targetPort, targetProto.c_str());
                            duplicateFound = true;
                            break;
                        }
                    }
                }

                if (false == duplicateFound)
                {
                    ociInterContainer.append(rule);
                }
            }
        }
    }

    return true;
}

/**
 * @brief Translates RALF network rules into Dobby networking plugin data.
 *
 * Mapping rules:
 * - public   -> portForwarding.hostToContainer
 * - exported -> interContainer direction "in"
 * - imported -> interContainer direction "out" (default RALF semantics)
 * - imported with "_hostEndpoint": true -> portForwarding.containerToHost
 *
 * The internal "_hostEndpoint" marker is only a translation hint used for host-backed
 * endpoints (for example, loopback Thunder/Firebolt) and is not emitted in the final Dobby
 * output. Duplicate rules are deduplicated and empty wrapper nodes are removed.
 *
 * @param[in] ralfNWCfgObject Source RALF network JSON payload (object or array).
 * @return Json::Value Translated Dobby networking JSON fragment.
 */
Json::Value translateRALFNWCfgObjToDobbyNWCfgObj(const Json::Value& ralfNWCfgObject)
{
    Json::Value dobbyNWCfgObject(Json::objectValue);

    if (false == ralfNWCfgObject.isArray() && false == ralfNWCfgObject.isObject())
    {
        LOGERR("%s: Invalid RALF network configuration object type.", MODULE_LOGTAG);
        return dobbyNWCfgObject;
    }

    Json::Value& portForwarding = dobbyNWCfgObject[PORT_FORWARDING] = Json::Value(Json::objectValue);
    Json::Value& hostToContainer = portForwarding[HOST_TO_CONTAINER] = Json::Value(Json::arrayValue);
    Json::Value& containerToHost = portForwarding[CONTAINER_TO_HOST] = Json::Value(Json::arrayValue);
    Json::Value& interContainer = dobbyNWCfgObject[INTER_CONTAINER] = Json::Value(Json::arrayValue);

    auto processItem = [&](const Json::Value& ralfItem) {
        if (false == ralfItem.isObject() || false == ralfItem.isMember(ralf::PORT) ||
            false == ralfItem.isMember(ralf::TYPE) ||
            (false == ralfItem[ralf::PORT].isUInt() && false == ralfItem[ralf::PORT].isInt()) ||
            false == ralfItem[ralf::TYPE].isString())
        {
            LOGWARN("%s: Invalid RALF network configuration item.", MODULE_LOGTAG);
            return;
        }

        uint32_t port = ralfItem[ralf::PORT].asUInt();
        std::string type = ralfItem.get(ralf::TYPE, "NoType").asString();

        if (0 == port || port > 65535 || (PUBLIC != type && EXPORTED != type && IMPORTED != type))
        {
            LOGWARN("%s: Invalid port %u or type '%s'; skipping item.", MODULE_LOGTAG, port, type.c_str());
            return;
        }

        std::string protocol = DEFAULT_PROTOCOL;
        if (ralfItem.isMember(ralf::PROTOCOL) && ralfItem[ralf::PROTOCOL].isString())
        {
            protocol = normalizeProtocol(ralfItem[ralf::PROTOCOL].asString());
        }

        if (PUBLIC == type) // Network services that container exposes outside the device.
        {
            Json::Value rule(Json::objectValue);
            rule[ralf::PORT] = port;
            rule[ralf::PROTOCOL] = protocol;
            appendIfUnique(hostToContainer, rule); // Dobby portForwarding.hostToContainer
        }
        else if (EXPORTED == type) // Container exposes to other apps or services on the device.
        {
            Json::Value rule(Json::objectValue);
            rule[DIRECTION] = DIRECTION_IN;
            rule[ralf::PORT] = port;
            rule[ralf::PROTOCOL] = protocol;
            rule[LOCALHOST_MASQUERADE] = true;
            appendIfUnique(interContainer, rule); // Dobby interContainer
        }
        else if (IMPORTED == type) // Container consumes from other apps or services on the device.
        {
            if (ralfItem.get(HOST_ENDPOINT_MARKER, false).asBool()) // Special case: Imported rule is a host endpoint.
            {
                Json::Value rule(Json::objectValue);
                rule[ralf::PORT] = port;
                rule[ralf::PROTOCOL] = protocol;
                appendIfUnique(containerToHost, rule);
            }
            else // Container consumes from other containers.
            {
                Json::Value rule(Json::objectValue);
                rule[DIRECTION] = DIRECTION_OUT;
                rule[ralf::PORT] = port;
                rule[ralf::PROTOCOL] = protocol;
                rule[LOCALHOST_MASQUERADE] = true;
                appendIfUnique(interContainer, rule); // Dobby interContainer
            }
        }
    };

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

    if (hostToContainer.empty()) { portForwarding.removeMember(HOST_TO_CONTAINER); }
    if (containerToHost.empty()) { portForwarding.removeMember(CONTAINER_TO_HOST); }
    if (portForwarding.empty()) { dobbyNWCfgObject.removeMember(PORT_FORWARDING); }
    if (interContainer.empty()) { dobbyNWCfgObject.removeMember(INTER_CONTAINER); }

    return dobbyNWCfgObject;
}

/**
 * @brief Handles parsing manifest nodes and cleanly maps the rules onto the nested rdkPlugins.networking.data node.
 * @param[in,out] ociConfigRootNode  The root structure of the OCI configuration tree file map.
 * @param[in]     manifestRootNode   The parsed deployment manifest data matrix.
 * @return True on smooth assignment execution tracking, false on system translation crashes.
 */
bool updateNetworkConfigurationNode(Json::Value& ociConfigRootNode, const Json::Value& manifestRootNode)
{
    if (!manifestRootNode.isMember(ralf::CONFIGURATION))
    {
        LOGWARN("%s: No configuration node found in manifest; skipping network configuration update", MODULE_LOGTAG);
        return true;
    }

    const Json::Value& configurationNode = manifestRootNode[ralf::CONFIGURATION];
    if (!configurationNode.isMember(ralf::NETWORK_CONFIG_URN))
    {
        LOGWARN("%s: No network configuration node found in manifest; skipping network configuration update", MODULE_LOGTAG);
        return true;
    }

    const Json::Value& networkConfiguration = configurationNode[ralf::NETWORK_CONFIG_URN];
    if (!networkConfiguration.isArray())
    {
        LOGWARN("%s: Network configuration is not an array, no need to process it.", MODULE_LOGTAG);
        return false;
    }

    if (networkConfiguration.empty())
    {
        LOGDBG("%s: Network configuration is empty, skipping it.", MODULE_LOGTAG);
        return true;
    }

    Json::Value* netDataNode = nullptr;
    try {
        netDataNode = getNetworkingDataNode(ociConfigRootNode, true, false);
    } catch (const Json::LogicError& e) {
        LOGERR("%s: Exception Json::LogicError: %s", MODULE_LOGTAG, e.what());
        return false;
    } catch (const std::exception& e) {
        LOGERR("%s: Exception std::exception: %s", MODULE_LOGTAG, e.what());
        return false;
    } catch (...) {
        LOGERR("%s: Unknown exception while retrieving networking data node.", MODULE_LOGTAG);
        return false;
    }

    if (nullptr == netDataNode)
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

    return updategetNetworkingDataNode(*netDataNode, dobbyNWCfgObject);
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
        return true;
    }

    const Json::Value& permissions = manifestRootNode[ralf::PERMISSIONS];
    if (!permissions.isArray())
    {
        LOGWARN("%s: Permissions node is not an array; skipping permission-based networking update", MODULE_LOGTAG);
        return true;
    }

    bool hasPermissionInternet = false;
    bool hasPermissionFirebolt = false;
    bool hasPermissionThunder = false;

    const Json::ArrayIndex permissionSize = permissions.size();
    for (Json::ArrayIndex index = 0; index < permissionSize; ++index)
    {
        const Json::Value& permValue = permissions[index];
        if (!permValue.isString())
        {
            continue;
        }
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

    if (!hasPermissionInternet && !hasPermissionFirebolt && !hasPermissionThunder)
    {
        LOGDBG("%s: No relevant permissions found; skipping permission-based networking update", MODULE_LOGTAG);
        return true;
    }

    // Call with false first. This provides a non-mutating snapshot pointer to verify duplicates
    Json::Value* netDataCheck = getNetworkingDataNode(ociConfigRootNode, false, false);

    Json::Value ralfLocalNWCfgObject(Json::arrayValue);

    bool isFireboltFulfilled = !hasPermissionFirebolt;
    bool isThunderFulfilled = !hasPermissionThunder;
    bool isInternetFulfilled = !hasPermissionInternet;

    // Check if dnsmasq is enabled in the networking data node for internet permission.
    if (hasPermissionInternet && netDataCheck != nullptr)
    {
        if (netDataCheck->isMember(ralf::DNSMASQ) && (*netDataCheck)[ralf::DNSMASQ].isBool() &&
            (*netDataCheck)[ralf::DNSMASQ].asBool() && netDataCheck->isMember(ralf::TYPE) &&
            (*netDataCheck)[ralf::TYPE].isString() && NETWORK_TYPE_NAT == (*netDataCheck)[ralf::TYPE].asString() &&
            netDataCheck->isMember(NETWORK_IPV4) && (*netDataCheck)[NETWORK_IPV4].isBool() &&
            (*netDataCheck)[NETWORK_IPV4].asBool() && netDataCheck->isMember(NETWORK_IPV6) &&
            (*netDataCheck)[NETWORK_IPV6].isBool() && (*netDataCheck)[NETWORK_IPV6].asBool())
        {
            isInternetFulfilled = true;
        }
    }

    if (hasPermissionFirebolt && !envVariables.empty())
    {
        const std::string& src = envVariables;
        const std::string prefix = std::string(ralf::FIREBOLT_ENDPOINT_ENV_KEY);
        size_t pos = 0;

        while ((pos = src.find(prefix, pos)) != std::string::npos)
        {
            if (pos > 0 && src[pos - 1] != '"' && src[pos - 1] != '[')
            {
                pos += prefix.size();
                continue;
            }

            size_t valueStart = pos + prefix.size();
            if (valueStart < src.size() && src[valueStart] == '=')
            {
                valueStart++;
                size_t valueEnd = src.find('"', valueStart);
                if (valueEnd != std::string::npos)
                {
                    size_t len = valueEnd - valueStart;
                    std::string fireboltEndpointStr;
                    fireboltEndpointStr.reserve(len);
                    for (size_t i = 0; i < len; ++i)
                    {
                        size_t currentIdx = valueStart + i;
                        if (src[currentIdx] == '\\' && (i + 1 < len) && src[currentIdx + 1] == '/')
                        {
                            continue;
                        }
                        fireboltEndpointStr.push_back(src[currentIdx]);
                    }

                    if (isLoopbackEndpoint(fireboltEndpointStr))
                    {
                        size_t schemeEnd = fireboltEndpointStr.find("://");
                        std::string protocolScheme = (schemeEnd != std::string::npos)
                            ? fireboltEndpointStr.substr(0, schemeEnd)
                            : fireboltEndpointStr;

                        const int port = extractPortFromEndpoint(fireboltEndpointStr);
                        if (port > 0 && port <= 65535)
                        {
                            const std::string normProto = normalizeProtocol(protocolScheme);
                            const unsigned int targetPort = static_cast<unsigned int>(port);

                            // Safe inspection lookup using the non-mutated snapshot block reference
                            if (netDataCheck != nullptr &&
                                hasPortFwdContainerToHostRule(*netDataCheck, targetPort, normProto))
                            {
                                LOGDBG("%s: Firebolt Port %u & protocol %s already exists; skipping addition.",
                                        MODULE_LOGTAG, targetPort, normProto.c_str());
                                isFireboltFulfilled = true;
                                break;
                            }

                            Json::Value fireboltNWCfgObject(Json::objectValue);
                            fireboltNWCfgObject[ralf::NAME] = "Firebolt";
                            fireboltNWCfgObject[ralf::PORT] = targetPort;
                            fireboltNWCfgObject[ralf::PROTOCOL] = normProto;
                            fireboltNWCfgObject[ralf::TYPE] = IMPORTED;
                            fireboltNWCfgObject[HOST_ENDPOINT_MARKER] = true;

                            ralfLocalNWCfgObject.append(fireboltNWCfgObject);
                            isFireboltFulfilled = true;
                            break;
                        }
                    }
                }
            }
            pos += prefix.size();
        }

        if (!isFireboltFulfilled)
        {
            LOGERR("%s: FIREBOLT_ENDPOINT environment variable not found or invalid in runtime config", MODULE_LOGTAG);
        }
    }

    if (hasPermissionThunder)
    {
        const char* thunderaccess = getenv(ralf::THUNDER_ACCESS_ENV_KEY);
        if (thunderaccess != nullptr)
        {
            bool isLoopback = isLoopbackEndpoint(thunderaccess);
            std::string thunderAccessStr(thunderaccess);
            const int port = extractPortFromEndpoint(thunderAccessStr);

            if (port > 0 && port <= 65535 && isLoopback)
            {
                size_t schemeEnd = thunderAccessStr.find("://");
                std::string protocolScheme = (schemeEnd != std::string::npos)
                    ? thunderAccessStr.substr(0, schemeEnd)
                    : "tcp";

                const std::string normProto = normalizeProtocol(protocolScheme);
                const unsigned int targetPort = static_cast<unsigned int>(port);

                // Safe inspection lookup using the non-mutated snapshot block reference
                if (netDataCheck != nullptr && hasPortFwdContainerToHostRule(*netDataCheck, targetPort, normProto))
                {
                    LOGDBG("%s: Thunder Port %u & protocol %s already exists; skipping addition.",
                            MODULE_LOGTAG, targetPort, normProto.c_str());
                    isThunderFulfilled = true;
                }
                else
                {
                    Json::Value thunderNWCfgObject(Json::objectValue);
                    thunderNWCfgObject[ralf::NAME] = "Thunder";
                    thunderNWCfgObject[ralf::PORT] = targetPort;
                    thunderNWCfgObject[ralf::PROTOCOL] = normProto;
                    thunderNWCfgObject[ralf::TYPE] = IMPORTED;
                    thunderNWCfgObject[HOST_ENDPOINT_MARKER] = true;

                    ralfLocalNWCfgObject.append(thunderNWCfgObject);
                    isThunderFulfilled = true;
                }
            }
            else
            {
                LOGDBG("%s: Invalid Thunder endpoint(port:%d, isLoopback:%d)", MODULE_LOGTAG, port, isLoopback);
            }
        }

        if (!isThunderFulfilled)
        {
            LOGERR("%s: THUNDER_ACCESS environment variable not found or invalid in runtime config.", MODULE_LOGTAG);
        }
    }

    // Direct success escape path if everything needed was already present or unrequested
    if (ralfLocalNWCfgObject.empty() && isInternetFulfilled && isFireboltFulfilled && isThunderFulfilled)
    {
        return true;
    }

    // If rules were generated OR internet needs configuration, mutate the node
    if (!ralfLocalNWCfgObject.empty() || !isInternetFulfilled)
    {
        // This will ensure NAT and DNSMASQ are enabled satisfying the internet permission requirement.
        Json::Value* netData = getNetworkingDataNode(ociConfigRootNode, true, hasPermissionInternet);
        if (nullptr == netData)
        {
            LOGERR("%s: Failed to create or initialize the active networking data node.", MODULE_LOGTAG);
            return false;
        }

        if (!ralfLocalNWCfgObject.empty())
        {
            Json::Value dobbyLocalNWCfgObject(Json::objectValue);
            dobbyLocalNWCfgObject = translateRALFNWCfgObjToDobbyNWCfgObj(ralfLocalNWCfgObject);
            if (dobbyLocalNWCfgObject.empty())
            {
                LOGWARN("%s: Translated Dobby network configuration object is empty.", MODULE_LOGTAG);
                return false;
            }

            if (!updategetNetworkingDataNode(*netData, dobbyLocalNWCfgObject))
            {
                return false;
            }
        }
    }

    // --- FINAL HARD GATE BOUNDARY ---
    if (!isFireboltFulfilled || !isThunderFulfilled)
    {
        LOGERR("%s: Permission-based network translation failed."
               "[Fulfillment Status - Firebolt: %s, Thunder: %s, Internet: %s]",
                    MODULE_LOGTAG,
                    isFireboltFulfilled ? "SUCCESS" : "FAILED",
                    isThunderFulfilled ? "SUCCESS" : "FAILED",
                    isInternetFulfilled ? "SUCCESS" : "FAILED");
        return false;
    }

    return true;
}
} // namespace NetworkConfigurationHelper
