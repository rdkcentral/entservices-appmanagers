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

namespace
{
    const char* TEMP_RALF_NWCFG    = "_temp_ralf_nwcfg";
    const char* NETWORK            = "network";
    const char* PUBLIC             = "public";
    const char* EXPORTED           = "exported";
    const char* IMPORTED           = "imported";
    const char* REQUIRED           = "required";
    const char* DIRECTION          = "direction";
    const char* IN_DIRECTION       = "in";
    const char* OUT_DIRECTION      = "out";

    bool ensureMountTargetFileInRootfs(const std::string& configFilePath, const std::string& containerPath)
    {
        if (containerPath.empty() || ('/' != containerPath[0]))
        {
            LOGERR("Invalid container path '%s' for mount target preparation", containerPath.c_str());
            return false;
        }

        const size_t configDirPos = configFilePath.find_last_of('/');
        if (std::string::npos == configDirPos)
        {
            LOGERR("Invalid config file path '%s'; unable to resolve rootfs directory", configFilePath.c_str());
            return false;
        }

        const std::string appBundleDir = configFilePath.substr(0, configDirPos);
        const std::string rootfsPath = appBundleDir + "/rootfs";
        const std::string targetPath = rootfsPath + containerPath;

        const size_t parentDirPos = targetPath.find_last_of('/');
        if (std::string::npos == parentDirPos)
        {
            LOGERR("Unable to resolve parent directory for mount target '%s'", targetPath.c_str());
            return false;
        }

        const std::string parentDir = targetPath.substr(0, parentDirPos);
        if (false == create_directories(parentDir))
        {
            LOGERR("Failed to create parent directory '%s' for mount target", parentDir.c_str());
            return false;
        }

        if (true == checkIfPathExists(targetPath))
        {
            return true;
        }

        std::ofstream targetFile(targetPath.c_str(), std::ios::out | std::ios::app);
        if (false == targetFile.is_open())
        {
            LOGERR("Failed to create mount target file '%s': %s", targetPath.c_str(), strerror(errno));
            return false;
        }

        targetFile.close();
        return true;
    }

    void addNetworkSystemMountsToOCIConfig(Json::Value& ociConfigRootNode, const std::string& configFilePath)
    {
        auto mountIfAvailable = [&ociConfigRootNode, &configFilePath](const std::string& path)
        {
            if (true == checkIfPathExists(path))
            {
                if (true == ensureMountTargetFileInRootfs(configFilePath, path))
                {
                    addBindMountToOCIConfig(ociConfigRootNode, path, path);
                }
                else
                {
                    LOGERR("Failed to prepare rootfs target for %s; skipping mount entry", path.c_str());
                }
            }
            else
            {
                LOGWARN("Host path %s is missing; skipping mount", path.c_str());
            }
        };

        const bool dnsmasqEnabled = ociConfigRootNode[RDKPLUGINS][NETWORKING][DATA][DNSMASQ].asBool();
        if (false == dnsmasqEnabled)
        {
            const std::string resolverSourcePath = getResolverSourcePathForContainer();
            const std::string resolverDestinationPath = RALF_DEFAULT_RESOLV_CONF_FILE;

            LOGDBG("Resolver mount selection: host '%s' -> container '%s'", resolverSourcePath.c_str(), resolverDestinationPath.c_str());

            if (true == checkIfPathExists(resolverSourcePath))
            {
                if (true == ensureMountTargetFileInRootfs(configFilePath, resolverDestinationPath))
                {
                    addBindMountToOCIConfig(ociConfigRootNode, resolverSourcePath, resolverDestinationPath);
                }
                else
                {
                    LOGERR("Failed to prepare rootfs target for %s; skipping mount entry", resolverDestinationPath.c_str());
                }
            }
            else
            {
                LOGWARN("Host path %s is missing; skipping mount", resolverSourcePath.c_str());
            }
        }
        else
        {
            LOGDBG("dnsmasq enabled for networking plugin; skipping host /etc/resolv.conf mount");
        }

        mountIfAvailable("/etc/hosts");
    }
}

using namespace ralf;
namespace NetworkConfigurationHelper
{

bool updateNetworkConfiguration(Json::Value& ociConfigRootNode, const Json::Value& manifestRootNode)
{
    if (!manifestRootNode.isMember(CONFIGURATION))
    {
        return true;
    }

    const Json::Value& configurationNode = manifestRootNode[CONFIGURATION];
    if (!configurationNode.isMember(NETWORK_CONFIG_URN))
    {
        return true;
    }

    const Json::Value& networkConfiguration =
        configurationNode[NETWORK_CONFIG_URN];

    if (!networkConfiguration.isArray())
    {
        LOGWARN("Network configuration is not an array");
        return true;
    }

    if (!ociConfigRootNode[TEMP_RALF_NWCFG][NETWORK].isObject())
    {
        ociConfigRootNode[TEMP_RALF_NWCFG][NETWORK] = Json::Value(Json::objectValue);
    }

    Json::Value& networkStore = ociConfigRootNode[TEMP_RALF_NWCFG][NETWORK];

    for (Json::ArrayIndex index = 0; index < networkConfiguration.size(); ++index)
    {
        const Json::Value& entry = networkConfiguration[index];

        if (!entry.isObject())
        {
            LOGWARN("Network entry is not an object");
            continue;
        }

        std::string entryName;

        if (entry.isMember(NAME) && entry[NAME].isString())
        {
            entryName = entry[NAME].asString();
        }
        else
        {
            std::ostringstream generatedName;
            generatedName << "unnamed-" << networkStore.size();
            entryName = generatedName.str();
        }

        networkStore[entryName] = entry;

        LOGDBG("Merged network entry: %s", entryName.c_str());
    }

    return true;
}

bool generateNetworkingPlugin(Json::Value& ociConfigRootNode)
{
    if (!ociConfigRootNode.isMember(TEMP_RALF_NWCFG) ||
        !ociConfigRootNode[TEMP_RALF_NWCFG].isMember(NETWORK))
    {
        return true;
    }

    const Json::Value& networkStore = ociConfigRootNode[TEMP_RALF_NWCFG][NETWORK];

    Json::Value networkingPlugin(Json::objectValue);
    Json::Value pluginData(Json::objectValue);
    Json::Value portForwarding(Json::objectValue);
    Json::Value hostToContainer(Json::arrayValue);
    Json::Value interContainer(Json::arrayValue);

    networkingPlugin[REQUIRED] = true;

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

bool applyRuntimeNetworkingConfiguration(Json::Value& ociConfigRootNode, bool networkEnabled, const std::string& configFilePath)
{
    Json::Value& netData = ociConfigRootNode[RDKPLUGINS][NETWORKING][DATA];

    if (true == networkEnabled)
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

        netData[TYPE] = NETWORK_TYPE_NAT;
        netData[DNSMASQ] = true;

        if (true == checkIfPathExists("/opt/arun-mount-files"))
        {
            LOGWARN("/opt/arun-mount-files exists; triggering network system mounts");
            addNetworkSystemMountsToOCIConfig(ociConfigRootNode, configFilePath);
        }
        else
        {
            LOGWARN("/opt/arun-mount-files does not exist; skipping network system mounts");
        }
    }
    else
    {
        netData[TYPE] = NETWORK_TYPE_NONE;
        netData[DNSMASQ] = false;
    }

    LOGDBG("Network mode set to '%s' (wanLanAccess/hasPermissionInternet=%d)",
           (true == networkEnabled ? NETWORK_TYPE_NAT : NETWORK_TYPE_NONE), networkEnabled);

    return true;
}

bool hasCapabilityPermission(const std::string& capabilities, const std::string& permission)
{
    if (capabilities.empty() || permission.empty())
    {
        LOGDBG("Capabilities or permission string is empty; cannot check for permission");
        return false;
    }

    size_t pos = 0;
    const std::string delimiter = ",";

    while (pos < capabilities.length())
    {
        size_t end = capabilities.find(delimiter, pos);
        if (std::string::npos == end)
        {
            end = capabilities.length();
        }

        std::string token = capabilities.substr(pos, end - pos);

        token.erase(0, token.find_first_not_of(" \t\n\r\f\v"));
        token.erase(token.find_last_not_of(" \t\n\r\f\v") + 1);

        if (permission == token)
        {
            LOGDBG("Found capability permission '%s'", permission.c_str());
            return true;
        }

        pos = end + delimiter.length();
    }

    return false;
}

} // namespace NetworkConfigurationHelper
