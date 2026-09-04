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
#include <algorithm>
#include <unordered_set>

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
        size_t hostStart = 0;
        const size_t localSchemePos = url.find("://");
        if (std::string::npos != localSchemePos)
        {
            hostStart = localSchemePos + 3;
        }

        const size_t colonPos = url.find(':', hostStart);
        if (std::string::npos == colonPos)
        {
            return -1;
        }

        const size_t portStart = colonPos + 1;
        const size_t portEnd = url.find_first_of("/?", portStart);
        const std::string portStr = url.substr(portStart, portEnd - portStart);

        try
        {
            return std::stoi(portStr);
        }
        catch (const std::exception&)
        {
            return -1;
        }
    }

    std::string normalizeProtocol(const std::string& protocol)
    {
        if (("udp" == protocol) ||
            ("dns" == protocol) ||
            ("dhcp" == protocol) ||
            ("snmp" == protocol) ||
            ("coap" == protocol) ||
            ("coaps" == protocol) ||
            ("tftp" == protocol) ||
            ("rtp" == protocol) ||
            ("rtsp" == protocol))
        {
            return "udp";
        }

        if (("http" != protocol) &&
            ("https" != protocol) &&
            ("ftp" != protocol) &&
            ("ssh" != protocol) &&
            ("git" != protocol) &&
            ("ws" != protocol) &&
            ("wss" != protocol) &&
            ("tcp" != protocol))
        {
            LOGWARN("%s: Unknown protocol '%s'; defaulting to 'tcp'", MODULE_LOGTAG, protocol.c_str());
        }

        return "tcp";
    }

    bool isLoopbackEndpoint(const std::string& endpoint)
    {
        return ((std::string::npos != endpoint.find("://localhost")) ||
                (std::string::npos != endpoint.find("://127.0.0.1")) ||
                (0 == endpoint.rfind("localhost:", 0)) ||
                (0 == endpoint.rfind("127.0.0.1:", 0)));
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

    bool addNetworkSystemMountsToOCIConfig(Json::Value& ociConfigRootNode, const std::string& configFilePath)
    {
        bool status = false;
        const bool dnsmasqEnabled = ociConfigRootNode[ralf::RDKPLUGINS][NETWORKING][ralf::DATA][DNSMASQ].asBool();
        if (false == dnsmasqEnabled)
        {
            const std::string resolverSourcePath = ralf::getResolverSourcePathForContainer();
            const std::string resolverDestinationPath = ralf::RALF_DEFAULT_RESOLV_CONF_FILE;

            if (true == ralf::checkIfPathExists(resolverSourcePath))
            {
                if (true == ensureMountTargetFileInRootfs(configFilePath, resolverDestinationPath))
                {
                    status = ralf::addBindMountToOCIConfig(ociConfigRootNode, resolverSourcePath, resolverDestinationPath);
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
            status = true;
            LOGWARN("%s: dnsmasq is enabled; skipping network system mounts", MODULE_LOGTAG);
        }
        return status;
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

    return true;
}

bool updateTempRalfNWCfgFromEnv(Json::Value& ociConfigRootNode, const std::vector<std::string>& envVarNames)
{
    if (envVarNames.empty())
    {
        LOGWARN("%s: Environment variable list is empty; skipping update", MODULE_LOGTAG);
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

    const std::unordered_set<std::string> requestedEnvNames(envVarNames.begin(), envVarNames.end());
    std::unordered_set<std::string> foundEnvNames;
    bool updated = false;

    // Single pass over process.env; update all requested names in one scan.
    for (const auto& envEntry : processNode[ralf::ENV])
    {
        if (!envEntry.isString())
        {
            continue;
        }

        const std::string envPair = envEntry.asString();
        const size_t separatorPos = envPair.find('=');
        if (std::string::npos == separatorPos)
        {
            continue;
        }

        const std::string envVarName = envPair.substr(0, separatorPos);
        if (requestedEnvNames.end() == requestedEnvNames.find(envVarName))
        {
            continue;
        }

        if (foundEnvNames.end() != foundEnvNames.find(envVarName))
        {
            continue;
        }

        const std::string envVarValue = envPair.substr(separatorPos + 1);
        if (envVarValue.empty())
        {
            LOGWARN("%s: Environment variable %s is empty; skipping update", MODULE_LOGTAG, envVarName.c_str());
            continue;
        }

        const size_t schemePos = envVarValue.find("://");
        const std::string protocolRaw = (std::string::npos == schemePos) ? "tcp" : envVarValue.substr(0, schemePos);
        const std::string protocol = normalizeProtocol(protocolRaw);

        const int port = extractPortFromEndpoint(envVarValue);
        if (0 >= port)
        {
            LOGWARN("%s: Invalid port extracted from %s: %d; skipping update", MODULE_LOGTAG, envVarName.c_str(), port);
            continue;
        }

        addContainerToHostRuleIfMissing(containerToHost, static_cast<uint32_t>(port), protocol);
        foundEnvNames.insert(envVarName);
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

    for (const auto& envVarName : requestedEnvNames)
    {
        if (foundEnvNames.end() == foundEnvNames.find(envVarName))
        {
            LOGWARN("%s: Environment variable %s not found in process.env; skipping update", MODULE_LOGTAG, envVarName.c_str());
        }
    }

    return updated;
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

    std::vector<std::string> envVarNames;
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

    const bool hasNetworkStore = tempConfig.isMember(NETWORK) && tempConfig[NETWORK].isObject() && !tempConfig[NETWORK].empty();
    const bool hasContainerToHostStore = tempConfig.isMember(CONTAINER_TO_HOST) && tempConfig[CONTAINER_TO_HOST].isArray() && !tempConfig[CONTAINER_TO_HOST].empty();

    if (!hasNetworkStore && !hasContainerToHostStore && !permissionInternetEnabled && !permissionFireboltEnabled && !permissionThunderEnabled)
    {
        LOGWARN("%s: Temporary network configuration is empty; skipping networking plugin generation", MODULE_LOGTAG);
        return true;
    }

    const Json::Value& networkStore = tempConfig[NETWORK];

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

    // Dobby will not create the network namespace if dnsmasq is disabled, so we need to ensure that
    // resolv.conf is mounted into the container when dnsmasq is disabled.
    if ((false == netData[DNSMASQ].asBool()) &&
        (false == addNetworkSystemMountsToOCIConfig(ociConfigRootNode, configFilePath)))
    {
            LOGWARN("%s: addNetworkSystemMountsToOCIConfig failed", MODULE_LOGTAG);
    }

    LOGDBG("%s: Network mode set to '%s' (runtimeEnabled=%d permissionInternet=%d permissionContainerToHost=%d)",
            MODULE_LOGTAG, netData[TYPE].asCString(), runtimeNetworkRequested, permissionInternetEnabled, permissionContainerToHostEnabled);

    return true;
}

} // namespace NetworkConfigurationHelper
