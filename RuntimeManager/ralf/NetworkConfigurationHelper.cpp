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

#include <cerrno>
#include <cstring>
#include <fstream>
#include <sstream>
#include <set>

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
        if (containerPath.empty() || ('/' != containerPath[0]))
        {
            LOGERR("%s: Invalid container path '%s' for mount target preparation", MODULE_LOGTAG, containerPath.c_str());
            return false;
        }

        const size_t configDirPos = configFilePath.find_last_of('/');
        if (std::string::npos == configDirPos)
        {
            LOGERR("%s: Invalid config file path '%s'; unable to resolve rootfs directory", MODULE_LOGTAG, configFilePath.c_str());
            return false;
        }

        const std::string appBundleDir = configFilePath.substr(0, configDirPos);
        const std::string rootfsPath = appBundleDir + "/rootfs";
        const std::string targetPath = rootfsPath + containerPath;

        const size_t parentDirPos = targetPath.find_last_of('/');
        if (std::string::npos == parentDirPos)
        {
            LOGERR("%s: Unable to resolve parent directory for mount target '%s'", MODULE_LOGTAG, targetPath.c_str());
            return false;
        }

        const std::string parentDir = targetPath.substr(0, parentDirPos);
        if (false == ralf::create_directories(parentDir))
        {
            LOGERR("%s: Failed to create parent directory '%s' for mount target", MODULE_LOGTAG, parentDir.c_str());
            return false;
        }

        if (true == ralf::checkIfPathExists(targetPath))
        {
            return true;
        }

        std::ofstream targetFile(targetPath.c_str(), std::ios::out | std::ios::app);
        if (false == targetFile.is_open())
        {
            LOGERR("%s: Failed to create mount target file '%s': %s", MODULE_LOGTAG, targetPath.c_str(), strerror(errno));
            return false;
        }

        targetFile.close();
        return true;
    }

    void addNetworkSystemMountsToOCIConfig(Json::Value& ociConfigRootNode, const std::string& configFilePath)
    {
        const bool dnsmasqEnabled = ociConfigRootNode[ralf::RDKPLUGINS][NETWORKING][ralf::DATA][DNSMASQ].asBool();
        if (false == dnsmasqEnabled)
        {
            const std::string resolverSourcePath = ralf::getResolverSourcePathForContainer();
            const std::string resolverDestinationPath = ralf::RALF_DEFAULT_RESOLV_CONF_FILE;

            LOGDBG("%s: Resolver mount selection: host '%s' -> container '%s'", MODULE_LOGTAG, resolverSourcePath.c_str(), resolverDestinationPath.c_str());

            if (true == ralf::checkIfPathExists(resolverSourcePath))
            {
                if (true == ensureMountTargetFileInRootfs(configFilePath, resolverDestinationPath))
                {
                    ralf::addBindMountToOCIConfig(ociConfigRootNode, resolverSourcePath, resolverDestinationPath);
                }
                else
                {
                    LOGERR("%s: Failed to prepare rootfs target for %s; skipping mount entry", MODULE_LOGTAG, resolverDestinationPath.c_str());
                }
            }
            else
            {
                LOGWARN("%s: Host path %s is missing; skipping mount", MODULE_LOGTAG, resolverSourcePath.c_str());
            }
        }
        else
        {
            LOGDBG("%s: dnsmasq enabled for networking plugin; skipping host /etc/resolv.conf mount", MODULE_LOGTAG);
        }
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

    if (!ociConfigRootNode[TEMP_RALF_NWCFG][ralf::NETWORK].isObject())
    {
        ociConfigRootNode[TEMP_RALF_NWCFG][ralf::NETWORK] = Json::Value(Json::objectValue);
    }

    Json::Value& networkStore = ociConfigRootNode[TEMP_RALF_NWCFG][ralf::NETWORK];

    for (Json::ArrayIndex index = 0; index < networkConfiguration.size(); ++index)
    {
        const Json::Value& entry = networkConfiguration[index];

        if (!entry.isObject())
        {
            LOGWARN("%s: Network entry is not an object", MODULE_LOGTAG);
            continue;
        }

        std::string entryName;

        if (entry.isMember(ralf::NAME) && entry[ralf::NAME].isString())
        {
            entryName = entry[ralf::NAME].asString();
        }
        else
        {
            std::ostringstream generatedName;
            generatedName << "unnamed-" << networkStore.size();
            LOGWARN("%s: Network entry is missing a name; generating unique name: %s", MODULE_LOGTAG, generatedName.str().c_str());
            entryName = generatedName.str();
        }

        networkStore[entryName] = entry;

        LOGDBG("%s: Merged network entry: %s", MODULE_LOGTAG, entryName.c_str());
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
        if (!permissions[index].isString())
        {
            continue;
        }

        const std::string permission = permissions[index].asString();
        if (ralf::PERMISSION_INTERNET == permission)
        {
            hasPermissionInternet = true;
        }
        else if (ralf::PERMISSION_FIREBOLT == permission)
        {
            hasPermissionFirebolt = true;
        }
        else if (ralf::PERMISSION_THUNDER == permission)
        {
            hasPermissionThunder = true;
        }
    }
    LOGDBG("%s: Permissions found - internet=%d firebolt=%d thunder=%d", MODULE_LOGTAG, hasPermissionInternet, hasPermissionFirebolt, hasPermissionThunder);
    Json::Value& permissionFlags = ociConfigRootNode[TEMP_RALF_NWCFG][PERMISSION_FLAGS];
    if (!permissionFlags.isObject())
    {
        permissionFlags = Json::Value(Json::objectValue);
    }

    permissionFlags[PERMISSION_INTERNET_ENABLED] =
        permissionFlags[PERMISSION_INTERNET_ENABLED].asBool() || hasPermissionInternet;
    permissionFlags[PERMISSION_FIREBOLT_ENABLED] =
        permissionFlags[PERMISSION_FIREBOLT_ENABLED].asBool() || hasPermissionFirebolt;
    permissionFlags[PERMISSION_THUNDER_ENABLED] =
        permissionFlags[PERMISSION_THUNDER_ENABLED].asBool() || hasPermissionThunder;

    LOGDBG("%s: Permission networking flags updated: internet=%d firebolt=%d thunder=%d", MODULE_LOGTAG,
           permissionFlags[PERMISSION_INTERNET_ENABLED].asBool(),
           permissionFlags[PERMISSION_FIREBOLT_ENABLED].asBool(),
           permissionFlags[PERMISSION_THUNDER_ENABLED].asBool());

    return true;
}

bool updateTempRalfNWCfgFromEnv(Json::Value& ociConfigRootNode, const std::string& envVarName, const std::string& name)
{
    if (envVarName.empty())
    {
        LOGWARN("%s: Environment variable name is empty; skipping update", MODULE_LOGTAG);
        return false;
    }

    Json::Value &processNode = ociConfigRootNode[PROCESS];
    if (!processNode.isObject())
    {
        processNode = Json::Value(Json::objectValue);
    }

    if (!processNode[ENV].isArray())
    {
        processNode[ENV] = Json::Value(Json::arrayValue);
    }

    /**
                "env" :
                [
                        "PATH=/usr/sbin:/usr/bin:/sbin:/bin",
                        "CPU_MEMORY_LIMIT=524288000",
                        "STORAGE_LIMIT=104857600",
                        "APP_PACKAGE_VERSION=6.0.32_6.0.32",
                        "WAYLAND_DISPLAY=wst-a606c5dc-7c85-4c7c-a0ad-54ccc00b6444",
                        "XDG_RUNTIME_DIR=/tmp",
                        "HOME=/data",
                        "PERSIST_STORAGE_PATH=/data",
                        "RIALTO_SOCKET_PATH=/tmp/rlto-a606c5dc-7c85-4c7c-a0ad-54ccc00b6444",
                        "FIREBOLT_ENDPOINT=ws://127.0.0.1:3473/?session=a606c5dc-7c85-4c7c-a0ad-54ccc00b6444",
                        "TEMP_STORAGE_PATH=/rootdir",
                        "THUNDER_ACCESS=127.0.0.1:9998"
                ]
     */

    // get envVarName from process.env and parse it to extract the protocol and port, then add it to TEMP_RALF_NWCFG
    std::string envVarValue = "";
    for (const auto &envEntry : processNode[ralf::ENV])
    {
        if (envEntry.isString())
        {
            std::string envPair = envEntry.asString();
            std::string prefix = envVarName + "=";
            if (envPair.rfind(prefix, 0) == 0)
            {
                envVarValue = envPair.substr(prefix.size());
                break;
            }
        }
    }
    if (envVarValue.empty())
    {
        LOGWARN("%s: Environment variable %s not found in process.env; skipping update", MODULE_LOGTAG, envVarName.c_str());
        return false;
    }
    const size_t schemePos = envVarValue.find("://");
    std::string protocol = (std::string::npos == schemePos) ? "tcp" : envVarValue.substr(0, schemePos);

    // lamda to extract port
	auto extractPort = [](const std::string& url) -> int {
        size_t hostStart = 0;
        const size_t localSchemePos = url.find("://");
        if (std::string::npos != localSchemePos)
        {
            hostStart = localSchemePos + 3;
        }

        size_t colonPos = url.find(':', hostStart);
        if (colonPos == std::string::npos)
        {
            return -1; // No port found
        }
        size_t portStart = colonPos + 1;
        size_t portEnd = url.find_first_of("/?", portStart);
        std::string portStr = url.substr(portStart, portEnd - portStart);
        try
        {
            return std::stoi(portStr);
        }
        catch (const std::exception&)
        {
            return -1; // Invalid port
        }
    };
    int port = extractPort(envVarValue);
    if (port <= 0)
    {
        LOGWARN("%s: Invalid port extracted from %s: %d; skipping update", MODULE_LOGTAG, envVarName.c_str(), port);
        return false;
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

    // protocol need to be mapped to tcp or udp, default to tcp
    std::set<std::string> tcpSchemes = { "http", "https", "ftp", "ssh", "git", "ws", "wss", "tcp" };
    std::set<std::string> udpSchemes = {"udp", "dns", "dhcp", "snmp", "coap", "coaps", "tftp", "rtp", "rtsp"};
    if (tcpSchemes.find(protocol) != tcpSchemes.end())
    {
        protocol = "tcp";
    }
    else if (udpSchemes.find(protocol) != udpSchemes.end())
    {
        protocol = "udp";
    }
    else
    {
        LOGWARN("%s: Unknown protocol '%s' in %s; defaulting to 'tcp'", MODULE_LOGTAG, protocol.c_str(), envVarName.c_str());
        protocol = "tcp";
    }

    addContainerToHostRuleIfMissing(containerToHost, static_cast<uint32_t>(port), protocol);

    // if host in protocol is localhost, enables localhost-masqueraded container egress to the resolved port.
    // localhostMasquerade is required when the container must keep using localhost endpoints
    // that are actually provided by host services.
    if ((envVarValue.find("://localhost") != std::string::npos) ||
        (envVarValue.find("://127.0.0.1") != std::string::npos) ||
        (0 == envVarValue.rfind("localhost:", 0)) ||
        (0 == envVarValue.rfind("127.0.0.1:", 0)))
    {
        Json::Value& permissionFlags = tempRalfNWCfgNode[PERMISSION_FLAGS];
        if (!permissionFlags.isObject())
        {
            permissionFlags = Json::Value(Json::objectValue);
        }
        permissionFlags[LOCALHOST_MASQUERADE] = true;
    }

    LOGDBG("%s: Updated TEMP_RALF_NWCFG with %s: protocol=%s port=%d", MODULE_LOGTAG, name.c_str(), protocol.c_str(), port);

    return true;
}

bool generateNetworkingPluginNode(Json::Value& ociConfigRootNode)
{
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

    if (permissionFireboltEnabled && !updateTempRalfNWCfgFromEnv(ociConfigRootNode, FIREBOLT_ENDPOINT_ENV_KEY, "firebolt"))
    {
        LOGWARN("%s: Failed to update TEMP_RALF_NWCFG with %s from environment", MODULE_LOGTAG, FIREBOLT_ENDPOINT_ENV_KEY);
    }
    if (permissionThunderEnabled && !updateTempRalfNWCfgFromEnv(ociConfigRootNode, THUNDER_ACCESS_ENV_KEY, "thunder"))
    {
        LOGWARN("%s: Failed to update TEMP_RALF_NWCFG with %s from environment", MODULE_LOGTAG, THUNDER_ACCESS_ENV_KEY);
    }

    const bool hasNetworkStore = tempConfig.isMember(NETWORK) && tempConfig[NETWORK].isObject() && !tempConfig[NETWORK].empty();
    const bool hasContainerToHostStore = tempConfig.isMember(CONTAINER_TO_HOST) && tempConfig[CONTAINER_TO_HOST].isArray() && !tempConfig[CONTAINER_TO_HOST].empty();

    if (!hasNetworkStore && !hasContainerToHostStore && !permissionInternetEnabled && !permissionFireboltEnabled && !permissionThunderEnabled)
    {
        LOGWARN("%s: Temporary network configuration is empty; skipping networking plugin generation", MODULE_LOGTAG);
        return true;
    }

    const Json::Value& networkStore = tempConfig[NETWORK];

    if (checkIfPathExists("/opt/apply-ralf-nwcfg"))
    {
        // dump json to a file for debugging.
        std::string debugFilePath = "/opt/temp_ralf_nwcfg.json";
        std::ofstream debugOutFile(debugFilePath.c_str());
        if (debugOutFile)
        {
            debugOutFile << networkStore;
            debugOutFile.close();
            LOGDBG("%s: Copied temporary network configuration to debug file %s", MODULE_LOGTAG, debugFilePath.c_str());
        }
        else
        {
            LOGERR("%s: Failed to open debug output file: %s", MODULE_LOGTAG, debugFilePath.c_str());
        }
    }

    Json::Value networkingPlugin(Json::objectValue);
    Json::Value pluginData(Json::objectValue);
    Json::Value portForwarding(Json::objectValue);
    Json::Value hostToContainer(Json::arrayValue);
    Json::Value interContainer(Json::arrayValue);

    networkingPlugin[REQUIRED] = true;

    pluginData[TYPE] = NETWORK_TYPE_NONE;
    // By default, enable both IPv4 and IPv6 support in the networking plugin.
    // TODO: In the future, we may want to make this configurable based on package manifest or runtime configuration.
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

            if (!entry.isMember(PORT) || !entry[PORT].isUInt())
            {
                continue;
            }

            const std::string protocol =
                (entry.isMember(PROTOCOL) && entry[PROTOCOL].isString()) ?
                entry[PROTOCOL].asString() : "tcp";

            const std::string type =
                (entry.isMember(TYPE) && entry[TYPE].isString()) ?
                entry[TYPE].asString() : PUBLIC;

            if (PUBLIC == type)
            {
                Json::Value rule(Json::objectValue);
                rule[PORT] = entry[PORT];
                rule[PROTOCOL] = protocol;
                hostToContainer.append(rule);
            }
            else if (EXPORTED == type)
            {
                Json::Value rule(Json::objectValue);
                rule[DIRECTION] = IN_DIRECTION;
                rule[PORT] = entry[PORT];
                rule[PROTOCOL] = protocol;
                interContainer.append(rule);
            }
            else if (IMPORTED == type)
            {
                Json::Value rule(Json::objectValue);
                rule[DIRECTION] = OUT_DIRECTION;
                rule[PORT] = entry[PORT];
                rule[PROTOCOL] = protocol;
                interContainer.append(rule);
            }
        }
    }

    Json::Value containerToHost(Json::arrayValue);
    bool localhostMasqueradeRequired = false;

    if (tempConfig.isMember(CONTAINER_TO_HOST) && tempConfig[CONTAINER_TO_HOST].isArray())
    {
        const Json::Value& envContainerToHost = tempConfig[CONTAINER_TO_HOST];
        const Json::ArrayIndex size = envContainerToHost.size();
        for (Json::ArrayIndex index = 0; index < size; ++index)
        {
            const Json::Value& rule = envContainerToHost[index];
            if (!rule.isObject() || !rule.isMember(PORT) || !rule[PORT].isUInt())
            {
                continue;
            }

            const std::string protocol =
                (rule.isMember(PROTOCOL) && rule[PROTOCOL].isString()) ?
                rule[PROTOCOL].asString() : "tcp";

            addContainerToHostRuleIfMissing(containerToHost, rule[PORT].asUInt(), protocol);
        }
    }

    localhostMasqueradeRequired = tempConfig[PERMISSION_FLAGS][LOCALHOST_MASQUERADE].asBool();

    if (permissionFireboltEnabled && !hasContainerToHostRule(containerToHost, FIREBOLT_CONTAINER_TO_HOST_PORT))
    {
        addContainerToHostRuleIfMissing(containerToHost, FIREBOLT_CONTAINER_TO_HOST_PORT);
    }
    if (permissionThunderEnabled && !hasContainerToHostRule(containerToHost, THUNDER_CONTAINER_TO_HOST_PORT))
    {
        addContainerToHostRuleIfMissing(containerToHost, THUNDER_CONTAINER_TO_HOST_PORT);
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
        portForwarding[LOCALHOST_MASQUERADE] = localhostMasqueradeRequired;
    }

    portForwarding[HOST_TO_CONTAINER] = hostToContainer;
    pluginData[PORT_FORWARDING] = portForwarding;
    pluginData[INTER_CONTAINER] = interContainer;

    networkingPlugin[DATA] = pluginData;

    ociConfigRootNode[RDKPLUGINS][NETWORKING] = networkingPlugin;

    if (checkIfPathExists("/opt/apply-ralf-nwcfg"))
    {
        // dump json to a file for debugging.
        std::string debugFilePath = "/opt/temp_ralf_nwcfg.json";
        std::ofstream debugOutFile(debugFilePath.c_str());
        if (debugOutFile)
        {
            debugOutFile << networkingPlugin;
            debugOutFile.close();
            LOGDBG("%s: Copied translated network configuration to debug file %s", MODULE_LOGTAG, debugFilePath.c_str());
        }
        else
        {
            LOGERR("%s: Failed to open debug output file: %s", MODULE_LOGTAG, debugFilePath.c_str());
        }
    }

    if (ociConfigRootNode.isMember(TEMP_RALF_NWCFG))
    {
        ociConfigRootNode.removeMember(TEMP_RALF_NWCFG);
    }

    return true;
}

bool applyRuntimeNetworkingConfiguration(Json::Value& ociConfigRootNode, const std::string& configFilePath)
{
    Json::Value& netData = ociConfigRootNode[RDKPLUGINS][NETWORKING][DATA];

    const bool permissionInternetEnabled = (NETWORK_TYPE_NAT == netData[TYPE].asString()) && netData[DNSMASQ].asBool();

    bool runtimeNetworkRequested = false;
    if (NETWORK_TYPE_NAT == netData[TYPE].asString())
    {
        runtimeNetworkRequested = true;
    }

    if (netData.isMember(INTER_CONTAINER) && netData[INTER_CONTAINER].isArray() && (0 < netData[INTER_CONTAINER].size()))
    {
        runtimeNetworkRequested = true;
    }

    if (netData.isMember(PORT_FORWARDING) && netData[PORT_FORWARDING].isObject() &&
        netData[PORT_FORWARDING].isMember(HOST_TO_CONTAINER) &&
        netData[PORT_FORWARDING][HOST_TO_CONTAINER].isArray() &&
        (0 < netData[PORT_FORWARDING][HOST_TO_CONTAINER].size()))
    {
        runtimeNetworkRequested = true;
    }

    bool permissionContainerToHostEnabled = false;
    if (netData.isMember(PORT_FORWARDING) && netData[PORT_FORWARDING].isObject())
    {
        const Json::Value& portForwarding = netData[PORT_FORWARDING];
        if (portForwarding.isMember(CONTAINER_TO_HOST) &&
            portForwarding[CONTAINER_TO_HOST].isArray() &&
            (0 < portForwarding[CONTAINER_TO_HOST].size()))
        {
            permissionContainerToHostEnabled = true;
        }
    }

    const bool effectiveNetworkEnabled = runtimeNetworkRequested || permissionInternetEnabled || permissionContainerToHostEnabled;

    if (true == effectiveNetworkEnabled)
    {
        static const std::string netBindServiceCap = "CAP_NET_BIND_SERVICE";
        static const char* capabilitySets[] = {"ambient", "bounding", "effective", "inheritable", "permitted"};

        Json::Value& capabilitiesNode = ociConfigRootNode[PROCESS]["capabilities"];

        for (const char* setName : capabilitySets)
        {
            Json::Value& capSet = capabilitiesNode[setName];
            if (false == capSet.isArray())
            {
                capSet = Json::Value(Json::arrayValue);
            }

            bool alreadyPresent = false;
            const Json::ArrayIndex size = capSet.size();
            for (Json::ArrayIndex i = 0; i < size; ++i)
            {
                if (netBindServiceCap == capSet[i].asString())
                {
                    alreadyPresent = true;
                    break;
                }
            }

            if (false == alreadyPresent)
            {
                capSet.append(netBindServiceCap);
            }
        }

        if (true == permissionInternetEnabled)
        {
            netData[TYPE] = NETWORK_TYPE_NAT;
            netData[DNSMASQ] = true;
        }
        else if (true == runtimeNetworkRequested)
        {
            netData[TYPE] = NETWORK_TYPE_NAT;
            netData[DNSMASQ] = true;
        }
        else if (true == permissionContainerToHostEnabled)
        {
            netData[TYPE] = NETWORK_TYPE_NAT;
            netData[DNSMASQ] = true;
        }
        else
        {
            netData[TYPE] = NETWORK_TYPE_NONE;
            netData[DNSMASQ] = false;
        }
    }
    else
    {
        netData[TYPE] = NETWORK_TYPE_NONE;
        netData[DNSMASQ] = false;
    }

    if (false == netData[DNSMASQ].asBool())
    {
        if (true == checkIfPathExists("/opt/arun-mount-files"))
        {
            LOGWARN("%s: /opt/arun-mount-files exists; triggering network system mounts", MODULE_LOGTAG);
            addNetworkSystemMountsToOCIConfig(ociConfigRootNode, configFilePath);
        }
        else
        {
            LOGWARN("%s: /opt/arun-mount-files does not exist; skipping network system mounts", MODULE_LOGTAG);
        }
    }
    else
    {
        LOGDBG("%s: dnsmasq enabled; skipping network system mounts", MODULE_LOGTAG);
    }

    LOGDBG("%s: Network mode set to '%s' (runtimeEnabled=%d permissionInternet=%d permissionContainerToHost=%d)",
            MODULE_LOGTAG, netData[TYPE].asCString(), runtimeNetworkRequested, permissionInternetEnabled, permissionContainerToHostEnabled);

    return true;
}

} // namespace NetworkConfigurationHelper
