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

#include "RalfOCIConfigGenerator.h"
#include "RalfSupport.h"
#include "OCISpecConstants.h"
#include <fstream>
#include <string>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <json/json.h>

#define PERSIST_STORAGE_PATH "/data"
#ifndef THUNDER_CONFIG_FILE
#define THUNDER_CONFIG_FILE  "/etc/WPEFramework/config.json"
#endif
#ifndef THUNDER_COM_SOCKET
#define THUNDER_COM_SOCKET   "/tmp/communicator"
#endif

namespace ralf
{
    /**
     * @brief: Extracts and returns an explicit port from a URI context. Supports IPv4, IPv6, and standard hostnames
     *         without allocating heap memory.
     * @param uri: The URI string to parse for an explicit port number.
     * @return: The parsed port number (1-65535) or -1 if no explicit port is found.
     */
    int RalfOCIConfigGenerator::getExplicitPortFromUri(const std::string &uri)
    {
        size_t startPos = 0;

        if (uri.empty())
        {
            return -1;
        }

        // Skip scheme prefix if present (e.g., "ws://", "http://")
        size_t schemePos = uri.find("://");
        if (schemePos != std::string::npos)
        {
            startPos = schemePos + strlen("://");
        }

        // Isolate where the authority/host area ends (path, query, or fragment boundaries)
        size_t hostEndPos = uri.find_first_of("/?#", startPos);
        size_t searchEnd = (hostEndPos == std::string::npos) ? uri.size() : hostEndPos;

        size_t portColonPos = std::string::npos;

        // Check for IPv6 closing bracket ']' within the isolated host window
        size_t closingBracketPos = uri.rfind(']', searchEnd - 1);

        if (closingBracketPos != std::string::npos && closingBracketPos >= startPos)
        {
            // IPv6 scenario: The port delimiter ':' must appear precisely after the closing bracket
            size_t nextColon = uri.find(':', closingBracketPos);
            if (nextColon != std::string::npos && nextColon < searchEnd)
            {
                portColonPos = nextColon;
            }
        }
        else
        {
            // IPv4 / Hostname scenario: Locate the last colon in the host window area
            portColonPos = uri.rfind(':', searchEnd - 1);
            if (portColonPos != std::string::npos && portColonPos < startPos)
            {
                portColonPos = std::string::npos; // Colon belongs to a malformed scheme prefix region
            }
        }

        // Validate that a valid port integer section follows the delimiter colon
        if (portColonPos == std::string::npos || portColonPos + 1 >= searchEnd)
        {
            return -1; // No explicit port specified in the URI
        }

        // Parse the port integer directly out of the continuous memory string buffer
        int port = std::atoi(&uri[portColonPos + 1]);

        return (port > 0 && port <= 65535) ? port : -1;
    }

    /**
     * Helper: Caches the JsonCpp builder configuration statically to avoid heavy repetitive
     * setup allocation churn across multiple execution cycles.
     */
    static const Json::CharReaderBuilder& RalfOCIConfigGenerator::getCachedJsonReaderBuilder()
    {
        static const Json::CharReaderBuilder builder = []() {
            Json::CharReaderBuilder b;
            b["allowComments"] = false; // Internal tweak optimization
            return b;
        }();
        return builder;
    }

    /**
     * @brief: Extracts the Firebolt port from a serialized JSON environment variable string.
     * @param envVar: The serialized JSON string containing environment variable entries.
     * @return: The parsed port number (1-65535) or -1 if no valid Firebolt endpoint is found.
     */
    int RalfOCIConfigGenerator::getFireboltPortFromEnvVars(const std::string &envVar)
    {
        if (envVar.empty())
        {
            return -1;
        }

        std::unique_ptr<Json::CharReader> reader(getCachedJsonReaderBuilder().newCharReader());
        Json::Value envVarsNode;

        // Passing nullptr avoids allocations inside the error messaging pipeline
        if (!reader->parse(envVar.data(), envVar.data() + envVar.size(), &envVarsNode, nullptr))
        {
            return -1;
        }

        if (!envVarsNode.isArray())
        {
            return -1;
        }

        // Calculate prefix length constraints once before entering the loop
        const char* keyPtr = FIREBOLT_ENDPOINT_ENV_KEY;
        size_t keyLen = std::strlen(keyPtr);

        for (const auto &envEntry : envVarsNode)
        {
            if (envEntry.isString())
            {
                // Extract the underlying string pointer, bypassing .asString() allocation overhead
                const char* entryStr = envEntry.asCString();
                size_t entryLen = envEntry.size();

                // Perform an optimized prefix match on continuous memory buffers for "KEY="
                if (entryLen > keyLen + 1 &&
                    std::strncmp(entryStr, keyPtr, keyLen) == 0 &&
                    entryStr[keyLen] == '=')
                {
                    // Construct a targeted substring copy containing only the trailing extracted URI
                    size_t prefixOffset = keyLen + 1;
                    std::string endpoint(entryStr + prefixOffset, entryLen - prefixOffset);

                    return getExplicitPortFromUri(endpoint);
                }
            }
        }

        return -1;
    }

    /**
     * @brief: Extracts a valid port specification from a localized configuration file mapping.
     * @return: The parsed port number (1-65535) or -1 if no valid port is found.
     */
    int RalfOCIConfigGenerator::getThunderPortFromConfigFile()
    {
        Json::Value configRoot;
        if (JsonFromFile(THUNDER_CONFIF_FILE, configRoot))
        {
            // Optimization: Query the member once using find().
            // This cuts map lookup overhead in half by avoiding 'isMember' followed by '[]'.
            const Json::Value* portNode = configRoot.find("port");

            if (portNode != nullptr && portNode->isInt())
            {
                int port = portNode->asInt();
                if (port > 0 && port <= 65535)
                {
                    return port;
                }
            }
        }

        return -1;
    }

    /**
     * @brief: Extracts Thunder port from the environment variable "THUNDER_ACCESS" or from a local configuration file.
     * falling back gracefully to the file system configuration structure if unavailable.
     * @return: The parsed port number (1-65535) or -1 if no valid port is found.
     */
    int RalfOCIConfigGenerator::getThunderPortFromEnvironmentOrConfig()
    {
        const char *thunderAccess = std::getenv("THUNDER_ACCESS");
        if (thunderAccess != nullptr && thunderAccess[0] != '\0')
        {
            const int port = getExplicitPortFromUri(thunderAccess);
            if (port > 0)
            {
                return port;
            }

            LOGWARN("THUNDER_ACCESS is present but not a TCP endpoint (value: %s); falling back to %s",
                    thunderAccess, THUNDER_CONFIF_FILE);
        }

        return getThunderPortFromConfigFile();
    }

    /**
     * @brief: Checks if the resolver file contains only loopback nameservers.
     * @param resolvPath: The path to the resolver file.
     * @return: True if only loopback nameservers are found, false otherwise.
     */
    bool RalfOCIConfigGenerator::hasOnlyLoopbackNameServers(const std::string &resolvPath)
    {
        if (checkIfPathExists(resolvPath) == false)
        {
            LOGWARN("Resolver file %s does not exist", resolvPath.c_str());
            return false;
        }

        std::ifstream in(resolvPath.c_str());
        if (!in)
        {
            return false;
        }

        bool foundNameServer = false;
        std::string line;

        while (std::getline(in, line))
        {
            // Quick prefix skip: A valid line must be at least long enough for "nameserver "
            if (line.size() < 12)
            {
                continue;
            }

            // Performance Win: Match "nameserver" prefix directly in-place without std::istringstream
            if (std::strncmp(line.data(), "nameserver", 10) != 0)
            {
                continue;
            }

            // Find the first non-whitespace character after "nameserver"
            size_t valStart = line.find_first_not_of(" \t", 10);
            if (valStart == std::string::npos)
            {
                continue; // Missing IP value
            }

            // Find where the IP token ends (whitespace or end of line)
            size_t valEnd = line.find_first_of(" \t\r\n", valStart);
            size_t valLen = (valEnd == std::string::npos) ? (line.size() - valStart) : (valEnd - valStart);

            if (valLen == 0)
            {
                continue;
            }

            foundNameServer = true;

            // Evaluate the loopback IP completely in-place
            const char* valPtr = line.data() + valStart;

            // Check for IPv6 Loopback "::1"
            if (valLen == 3 && std::strncmp(valPtr, "::1", 3) == 0)
            {
                continue;
            }

            // Check for IPv4 Loopback range "127.*"
            if (valLen >= 4 && std::strncmp(valPtr, "127.", 4) == 0)
            {
                continue;
            }

            // Found a non-loopback nameserver! Exit immediately.
            return false;
        }

        return foundNameServer;
    }

    /**
     * @brief: Gets the resolver source path for the container.
     * @return: The path to the resolver file to be used inside the container.
     * Defaults to /etc/resolv.conf, but may return a fallback path if the default resolver only contains loopback nameservers.
     */
    std::string RalfOCIConfigGenerator::getResolverSourcePathForContainer()
    {
        static const std::string defaultResolver = "/etc/resolv.conf";
        static const std::string noStubNetworkManager = "/run/NetworkManager/no-stub-resolv.conf";
        static const std::string noStubSystemdResolved = "/run/systemd/resolve/resolv.conf";

        // Fast-path: If the default resolver has actual public nameservers, return it instantly
        if (!hasOnlyLoopbackNameServers(defaultResolver))
        {
            return defaultResolver;
        }

        // Fallback lookups: Check for local container stub overrides
        if (checkIfPathExists(noStubNetworkManager))
        {
            return noStubNetworkManager;
        }

        if (checkIfPathExists(noStubSystemdResolved))
        {
            return noStubSystemdResolved;
        }

        LOGWARN("Host resolver file %s only has loopback nameservers and no fallback resolver file found", defaultResolver.c_str());
        return defaultResolver;
    }

    bool RalfOCIConfigGenerator::ensureMountTargetFileInRootfs(const std::string &containerPath)
    {
        if (containerPath.empty() || containerPath[0] != '/')
        {
            LOGERR("Invalid container path for mount target creation: %s", containerPath.c_str());
            return false;
        }

        static const std::string rootfsToken = "rootfs";
        // Optimization 2: Reserve memory upfront for the final path to prevent re-allocating during concatenation
        const std::string bundleDir = WPEFramework::Core::File::PathName(mConfigFilePath);

        std::string targetPath;
        targetPath.reserve(bundleDir.size() + rootfsToken.size() + containerPath.size());
        targetPath.append(bundleDir).append(rootfsToken).append(containerPath);

        const std::string targetParentDir = WPEFramework::Core::File::PathName(targetPath);

        WPEFramework::Core::Directory parentDir(targetParentDir.c_str());
        if (!parentDir.CreatePath())
        {
            LOGERR("Failed to create mount target parent dir %s", targetParentDir.c_str());
            return false;
        }

        WPEFramework::Core::File mountTarget(targetPath);

        // Create() internally opens/creates the file safely. If it fails, we know it couldn't be prepared.
        if (!mountTarget.Create())
        {
            LOGERR("Failed to create mount target file in rootfs: %s", targetPath.c_str());
            return false;
        }
        mountTarget.Close();

        return true;
    }

    void RalfOCIConfigGenerator::addNetworkSystemMountsToOCIConfig(Json::Value &ociConfigRootNode,
                                                                  bool networkEnabled,
                                                                  bool thunderAccessEnabled)
    {
        // A single lambda that takes both host & container paths, eliminating code duplication
        auto mountIfAvailable = [this, &ociConfigRootNode](const std::string& srcPath, const std::string& destPath)
        {
            if (checkIfPathExists(srcPath))
            {
                if (ensureMountTargetFileInRootfs(destPath))
                {
                    addMountEntry(ociConfigRootNode, srcPath, destPath);
                }
                else
                {
                    LOGERR("Failed to prepare rootfs target for %s; skipping mount entry", destPath.c_str());
                }
            }
            else
            {
                LOGWARN("Host path %s is missing; skipping mount", srcPath.c_str());
            }
        };

        if (networkEnabled)
        {
            // Single safe traversal path check for deeply nested JSON objects.
            // Prevents building dynamic null-nodes in JsonCpp map buckets.
            bool dnsmasqEnabled = false;

            const Json::Value* plugins = ociConfigRootNode.find(RDKPLUGINS);
            if (plugins) {
                const Json::Value* networking = plugins->find(NETWORKING);
                if (networking) {
                    const Json::Value* data = networking->find(DATA);
                    if (data) {
                        const Json::Value* dnsmasq = data->find(DNSMASQ);
                        if (dnsmasq && dnsmasq->isBool()) {
                            dnsmasqEnabled = dnsmasq->asBool();
                        }
                    }
                }
            }

            if (!dnsmasqEnabled)
            {
                static const std::string resolverDestinationPath = "/etc/resolv.conf";
                const std::string resolverSourcePath = getResolverSourcePathForContainer();

                LOGDBG("Resolver mount selection: host '%s' -> container '%s'",
                       resolverSourcePath.c_str(), resolverDestinationPath.c_str());

                mountIfAvailable(resolverSourcePath, resolverDestinationPath);
            }
            else
            {
                LOGDBG("dnsmasq enabled for networking plugin; skipping host /etc/resolv.conf mount");
            }

            static const std::string etcHostsPath = "/etc/hosts";
            mountIfAvailable(etcHostsPath, etcHostsPath);
        }

        if (thunderAccessEnabled)
        {
            std::string communicatorPath;
            std::string communicatorEnv;

            // Direct environment resolution check
            if (WPEFramework::Core::SystemInfo::GetEnvironment("COMMUNICATOR_CONNECTOR", communicatorEnv) && !communicatorEnv.empty())
            {
                communicatorPath = std::move(communicatorEnv);
            }
            else
            {
                communicatorPath = THUNDER_COM_SOCKET;
            }

            mountIfAvailable(communicatorPath, communicatorPath);
        }
    }

    bool RalfOCIConfigGenerator::generateRalfOCIConfig(const WPEFramework::Plugin::ApplicationConfiguration &config, const WPEFramework::Exchange::RuntimeConfig &runtimeConfigObject)
    {
        Json::Value ociConfigRootNode;

        if (!JsonFromFile(RALF_OCI_BASE_SPEC_FILE, ociConfigRootNode))
        {
            LOGERR("Failed to load base OCI config template");
            return false;
        }
        // Load graphics config and integrate into OCI config
        Json::Value graphicsConfigNode;
        if (!JsonFromFile(RALF_GRAPHICS_LAYER_CONFIG, graphicsConfigNode))
        {
            LOGERR("Failed to load Ralf graphics config JSON from file: %s", RALF_GRAPHICS_LAYER_CONFIG.c_str());
            return false;
        }

        // Apply graphics configuration
        if (!applyGraphicsConfigToOCIConfig(ociConfigRootNode, graphicsConfigNode))
        {
            LOGERR("Failed to apply graphics config to OCI config");
            return false;
        }
        // Now apply each Ralf package configuration
        for (const auto &ralfPkgInfo : mRalfPackages)
        {
            Json::Value ralfPackageConfigNode;
            if (!JsonFromFile(ralfPkgInfo.first, ralfPackageConfigNode))
            {
                LOGERR("Failed to load Ralf package config JSON from file: %s", ralfPkgInfo.first.c_str());
                return false;
            }
            if (!applyConfigurationToOCIConfig(ociConfigRootNode, ralfPackageConfigNode))
            {
                LOGERR("Failed to apply Ralf package config to OCI config for file: %s", ralfPkgInfo.first.c_str());
                return false;
            }
            // Apply permissions if exists
            // TODO tracked under RDKEMW-13995
        }
        if (generateHooksForOCIConfig(ociConfigRootNode) == false)
        {
            LOGERR("Failed to generate hooks for OCI config");
            return false;
        }

        // // Let us apply data from runtimeConfigObject and applicationConfiguration
        if (applyRuntimeAndAppConfigToOCIConfig(ociConfigRootNode, runtimeConfigObject, config) == false)
        {
            LOGERR("Failed to apply runtime and application config to OCI config");
            return false;
        }
        // Add FIREBOLT_ENDPOINT environment variable from runtime config to OCI config if it exists
        addFireboltEndPointToConfig(ociConfigRootNode, runtimeConfigObject.envVariables);
        // /rootdir is a 10MB tmpfs, so we need to ensure that the application has enough space for its working directory.
        addToEnvironment(ociConfigRootNode, "TEMP_STORAGE_PATH", "/rootdir");
        // Log name update.
        addLogNameToOCIConfig(ociConfigRootNode, config.mAppStorageInfo.path, config.mAppId);
        // Add Timezone info
        addTimezoneInfo(ociConfigRootNode);
        // Finally save the modified OCI config to file
        addThunderAccessToPrivilegedApps(ociConfigRootNode);
        return saveOCIConfigToFile(ociConfigRootNode, config.mUserId, config.mGroupId);
    }

    void RalfOCIConfigGenerator::addThunderAccessToPrivilegedApps(Json::Value &ociConfigRootNode)
    {
        const char* thunderaccess = getenv("THUNDER_ACCESS");
        if (nullptr != thunderaccess)
        {
            //TODO  this should be checked against urn:rdk:permission:thunder capability before adding to environment
            addToEnvironment(ociConfigRootNode, "THUNDER_ACCESS", thunderaccess);
            LOGINFO("THUNDER_ACCESS environment variable is set to: %s", thunderaccess);
        }
    }

    void RalfOCIConfigGenerator::addLogNameToOCIConfig(Json::Value &ociConfigRootNode, const std::string &appStoragePath, const std::string &appId)
    {
        // Override the default log file path in the generated OCI config with an app-specific path.
        // The updated entry is rdkPlugins->logging->data->fileOptions->path.
        std::string logFilePath = appStoragePath + "/" + appId + ".log";
        ociConfigRootNode[RDKPLUGINS][LOGGING][DATA][LOG_FILE_OPTIONS][PATH] = logFilePath;
    }

    bool RalfOCIConfigGenerator::applyRuntimeAndAppConfigToOCIConfig(Json::Value &ociConfigRootNode, const WPEFramework::Exchange::RuntimeConfig &runtimeConfigObject, const WPEFramework::Plugin::ApplicationConfiguration &appConfig)
    {
        bool status = true;
        // Set user and group ID
        ociConfigRootNode[PROCESS][USER][UID] = 0; // Run as root inside container
        ociConfigRootNode[PROCESS][USER][GID] = 0; // Run as root group inside container
        ociConfigRootNode[PROCESS][USER][ADDITIONAL_GIDS] = Json::Value(Json::arrayValue);

        // Add video group
        uint32_t videoGid = 44;
        if (!getGroupId("video", videoGid))
        {
            LOGWARN("Failed to get GID for video group, using default GID 44");
        }
        ociConfigRootNode[PROCESS][USER][ADDITIONAL_GIDS].append(videoGid); // video group

        // set hostname to appid
        ociConfigRootNode[HOSTNAME] = appConfig.mAppId;

        // Set uidMappings
        Json::Value uidMapping;
        uidMapping[CONTAINER_ID] = 0;
        uidMapping[HOST_ID] = appConfig.mUserId;
        uidMapping[SIZE] = 1;
        ociConfigRootNode[LINUX][UID_MAPPINGS].append(uidMapping);

        // set gidMappings also
        Json::Value gidMapping;
        gidMapping[CONTAINER_ID] = 0;
        gidMapping[HOST_ID] = appConfig.mGroupId;
        gidMapping[SIZE] = 1;
        ociConfigRootNode[LINUX][GID_MAPPINGS].append(gidMapping);


        // Network configuration policy:
        // - wanLanAccess=true: internet access (nat)
        // - thunder=true or dial=true: local network access required (nat)
        // - urn:rdk:permission:internet: internet access enabled (nat)
        // - none enabled: keep container isolated (none)
        const std::string &capabilities = runtimeConfigObject.capabilities;
        bool hasInternet  = hasCapabilityPermission(capabilities, PERMISSION_INTERNET);
        bool hasThunder   = runtimeConfigObject.thunder || hasCapabilityPermission(capabilities, PERMISSION_THUNDER);
        bool hasFirebolt  = hasCapabilityPermission(capabilities, PERMISSION_FIREBOLT);
        bool networkEnabled = runtimeConfigObject.wanLanAccess || runtimeConfigObject.dial ||
                    hasInternet || hasThunder || hasFirebolt;
        if (networkEnabled)
        {
            static const std::string netRawCap = "CAP_NET_RAW";
            static const char *capabilitySets[] = {"ambient", "bounding", "effective", "inheritable", "permitted"};

            for (const char *setName : capabilitySets)
            {
                Json::Value &capSet = ociConfigRootNode[PROCESS]["capabilities"][setName];
                if (!capSet.isArray())
                {
                    capSet = Json::Value(Json::arrayValue);
                }

                bool alreadyPresent = false;
                for (const auto &cap : capSet)
                {
                    if (cap.asString() == netRawCap)
                    {
                        alreadyPresent = true;
                        break;
                    }
                }

                if (!alreadyPresent)
                {
                    capSet.append(netRawCap);
                }
            }
        }

        ociConfigRootNode[RDKPLUGINS][NETWORKING][DATA][TYPE] = networkEnabled ? "nat" : "closed";
        ociConfigRootNode[RDKPLUGINS][NETWORKING][DATA][DNSMASQ] = networkEnabled;
        LOGDBG("Network mode set to '%s' (wanLanAccess=%d dial=%d thunder=%d hasInternet=%d hasFirebolt=%d)",
             networkEnabled ? "nat" : "closed",
             runtimeConfigObject.wanLanAccess,
             runtimeConfigObject.dial,
             hasThunder,
             hasInternet,
             hasFirebolt);

        // Granular containerToHost access with localhostMasquerade:
        // - Thunder JSONRPC/WebSocket endpoint port comes from THUNDER_ACCESS env when it is TCP,
        //   otherwise falls back to /etc/WPEFramework/config.json "port"
        // - Firebolt endpoint port comes from FIREBOLT_ENDPOINT in runtime env variables
        // Add both when required and deduplicate if they resolve to the same port.
        if (hasThunder || hasFirebolt)
        {
            std::vector<int> portsToExpose = {};

            if (hasThunder)
            {
                int thunderPort = getThunderPortFromEnvironmentOrConfig();
                if (thunderPort != -1)
                {
                    portsToExpose.push_back(thunderPort);
                }
                else
                {
                    LOGWARN("Failed to resolve Thunder port(%d) from THUNDER_ACCESS or config file; skipping localhostMasquerade entry.", thunderPort);
                }
            }

            if (hasFirebolt)
            {
                int fireboltPort = getFireboltPortFromEnvVars(runtimeConfigObject.envVariables);
                if (fireboltPort != -1)
                {
                    portsToExpose.push_back(fireboltPort);
                }
                else
                {
                    LOGWARN("Failed to resolve Firebolt endpoint port(%d) from runtime env; skipping Firebolt-specific localhostMasquerade entry");
                }
            }

            // Dobby Networking plugin consumes localhostMasquerade at the
            // portForwarding object level, not per individual port entries.
            if (!portsToExpose.empty())
            {
                ociConfigRootNode[RDKPLUGINS][NETWORKING][DATA][PORT_FORWARDING][LOCALHOST_MASQUERADE] = true;
            }

            for (size_t i = 0; i < portsToExpose.size(); ++i)
            {
                int port = portsToExpose[i];
                bool alreadyAdded = false;
                for (size_t j = 0; j < i; ++j)
                {
                    if (portsToExpose[j] == port)
                    {
                        alreadyAdded = true;
                        break;
                    }
                }

                if (alreadyAdded)
                {
                    continue;
                }

                Json::Value portEntry(Json::objectValue);
                portEntry[PORT] = port;
                portEntry[PROTOCOL] = "tcp";
                ociConfigRootNode[RDKPLUGINS][NETWORKING][DATA][PORT_FORWARDING][CONTAINER_TO_HOST].append(portEntry);
                LOGDBG("Added containerToHost localhostMasquerade on port %d (hasThunder=%d hasFirebolt=%d)",
                       port, hasThunder, hasFirebolt);
            }
        }

        addNetworkSystemMountsToOCIConfig(ociConfigRootNode, networkEnabled, (hasThunder || hasFirebolt));

        // Set westeros environment variable
        // mWesterosSocketPath has XDG_RUNTIME_DIR/WAYLAND_DISPLAY, we need to set WAYLAND_DISPLAY env variable to just the display
        // name (i.e. the last part of the path) because that is what westeros expects.
        std::string waylandDisplay = appConfig.mWesterosSocketPath.substr(appConfig.mWesterosSocketPath.find_last_of("/") + 1);
        addToEnvironment(ociConfigRootNode, "WAYLAND_DISPLAY", waylandDisplay);
        // Keep it consistent with what is set in DobbyPluginLauncher for Ralf package.
        // DobbyPluginLauncher will bind mount /tmp from host to container and westeros socket will be created inside /tmp in the container.
        addToEnvironment(ociConfigRootNode, "XDG_RUNTIME_DIR", "/tmp");

        // Need to mount bind  XDG_RUNTIME_DIR/WAYLAND_DISPLAY from host to container
        addBindMountToOCIConfig(ociConfigRootNode, appConfig.mWesterosSocketPath, appConfig.mWesterosSocketPath);

        // Home by default will be set to PERSIST_STORAGE_PATH in the OCI config.
        std::string homePath = PERSIST_STORAGE_PATH; // Default HOME path
        addToEnvironment(ociConfigRootNode, "HOME", homePath);

        // Mount persistent storage path
        std::string appStoragePath = appConfig.mAppStorageInfo.path;
        addAppStorageToOCIConfig(ociConfigRootNode, appStoragePath);

        // Finally add rialto path to the environment variables
        std::string rialtoSocketPath = "/tmp/rlto-" + appConfig.mAppInstanceId;
        addToEnvironment(ociConfigRootNode, "RIALTO_SOCKET_PATH", rialtoSocketPath);
        LOGDBG("Added RIALTO_SOCKET environment variable with value %s\n", rialtoSocketPath.c_str());
        addBindMountToOCIConfig(ociConfigRootNode, rialtoSocketPath, rialtoSocketPath);
        LOGDBG("Mounted rialto socket path %s to container path %s\n", rialtoSocketPath.c_str(), rialtoSocketPath.c_str());
        return status;
    }
    bool RalfOCIConfigGenerator::addAppStorageToOCIConfig(Json::Value &ociConfigRootNode, const std::string &appStoragePath)
    {
        bool status = false;
        // We will mount application storage to /home/root/appstorage and
        // set PERSIST_STORAGE_PATH environment variable to it.
        std::string containerStoragePath = PERSIST_STORAGE_PATH;
        addBindMountToOCIConfig(ociConfigRootNode, appStoragePath, containerStoragePath);
        addToEnvironment(ociConfigRootNode, "PERSIST_STORAGE_PATH", containerStoragePath);
        LOGDBG("Added application storage mount from %s to %s and set PERSIST_STORAGE_PATH environment variable\n", appStoragePath.c_str(), containerStoragePath.c_str());
        status = true;
        return status;
    }
    bool RalfOCIConfigGenerator::generateHooksForOCIConfig(Json::Value &ociConfigRootNode)
    {
        // We need to add four hooks: createRuntime, createContainer, poststart, poststop
        return generateHooksForOCIConfig(ociConfigRootNode, "createRuntime") &&
               generateHooksForOCIConfig(ociConfigRootNode, "createContainer") &&
               generateHooksForOCIConfig(ociConfigRootNode, "poststart") &&
               generateHooksForOCIConfig(ociConfigRootNode, "poststop");
    }
    bool RalfOCIConfigGenerator::generateHooksForOCIConfig(Json::Value &ociConfigRootNode, const std::string &operation)
    {
        /*
        The hooks structure is as follows:
        "hooks": {
            "operation": [
                {
                    "path": "/usr/bin/DobbyPluginLauncher",
                    "args": [
                        "DobbyPluginLauncher",
                        "-h",
                        "operation",
                        "-c",
                        "path to config file",
                        "-vv"
                    ],
                }
            ]
        }
        */

        Json::Value hookEntry;
        hookEntry[PATH] = "/usr/bin/DobbyPluginLauncher";
        hookEntry[ARGS] = Json::Value(Json::arrayValue);
        hookEntry[ARGS].append("DobbyPluginLauncher");
        hookEntry[ARGS].append("-h");
        hookEntry[ARGS].append(operation);
        hookEntry[ARGS].append("-c");
        hookEntry[ARGS].append(mConfigFilePath);
        hookEntry[ARGS].append("-v");

        ociConfigRootNode[HOOKS][operation] = Json::Value(Json::arrayValue);
        ociConfigRootNode[HOOKS][operation].append(hookEntry);

        return true;
    }

    bool RalfOCIConfigGenerator::saveOCIConfigToFile(const Json::Value &ociConfigRootNode, int uid, int gid)
    {
        bool status = false;
        std::string ociConfigJson;
        Json::StreamWriterBuilder writer;
        ociConfigJson = Json::writeString(writer, ociConfigRootNode);

        LOGDBG("Generated OCI config JSON: Writing to file  %s\n", mConfigFilePath.c_str());
        // Write to file
        std::ofstream outFile(mConfigFilePath.c_str());
        if (outFile)
        {

            outFile << ociConfigJson;
            outFile.close();
            // Change ownership to uid:gid
            if (chown(mConfigFilePath.c_str(), uid, gid) != 0)
            {
                LOGERR("Failed to change ownership of OCI config file %s to %d:%d\n", mConfigFilePath.c_str(), uid, gid);
            }
            status = true;
        }
        else
        {
            LOGERR("Failed to open OCI config output file: %s", mConfigFilePath.c_str());
        }
        return status;
    }

    bool RalfOCIConfigGenerator::applyGraphicsConfigToOCIConfig(Json::Value &ociConfigRootNode, const Json::Value &graphicsConfigNode)
    {
        bool status = true;

        // We need to get the devNodes and groupIds and apply them to the OCI config json structure.

        // Check if vendorGpuSupport/devNodes exists
        if (graphicsConfigNode.isMember(VENDOR_GPU_SUPPORT))
        {
            if (graphicsConfigNode[VENDOR_GPU_SUPPORT].isMember(DEV_NODES))
            {
                const Json::Value &devNodes = graphicsConfigNode[VENDOR_GPU_SUPPORT][DEV_NODES];
                status = addDeviceNodeEntriesToOCIConfig(ociConfigRootNode, devNodes);
            }
            else
            {
                LOGWARN("No vendorGpuSupport/devNodes found in graphics config\n");
            }
            // Let us map the groups in the graphics config to the OCI config as well. This is needed for cases where GPU access requires specific group permissions.
            if (graphicsConfigNode[VENDOR_GPU_SUPPORT].isMember(GROUP_IDS))
            {
                const Json::Value &groupIds = graphicsConfigNode[VENDOR_GPU_SUPPORT][GROUP_IDS];
                for (Json::Value::ArrayIndex i = 0; i < groupIds.size(); ++i)
                {
                    // get the group name from the config and then get the gid for it. Then add a gid mapping for that gid.
                    std::string groupName = groupIds[i].asString();
                    uint32_t gid = 0;
                    if (getGroupId(groupName, gid))
                    {
                        Json::Value gidMapping;
                        gidMapping[CONTAINER_ID] = gid;
                        gidMapping[HOST_ID] = gid;
                        gidMapping[SIZE] = 1;
                        ociConfigRootNode[LINUX][GID_MAPPINGS].append(gidMapping);
                        LOGDBG("Added additional GID mapping for GPU access: host GID %u to container GID %u\n", gid, gid);
                    }
                    else
                    {
                        LOGWARN("Failed to get GID for group %s specified in graphics config\n", groupName.c_str());
                    }
                }
            }
            else
            {
                LOGWARN("No vendorGpuSupport/groupIds found in graphics config\n");
            }

            // Now get the file that needs to be mapped, this is optional.
            if (graphicsConfigNode[VENDOR_GPU_SUPPORT].isMember(FILES))
            {
                const Json::Value &files = graphicsConfigNode[VENDOR_GPU_SUPPORT][FILES];
                for (Json::Value::ArrayIndex i = 0; i < files.size(); ++i)
                {
                    const Json::Value &fileEntry = files[i];
                    if (fileEntry.isMember(SOURCE) && fileEntry.isMember(DESTINATION))
                    {
                        std::string sourcePath = fileEntry[SOURCE].asString();
                        std::string destPath = fileEntry[DESTINATION].asString();
                        addBindMountToOCIConfig(ociConfigRootNode, sourcePath, destPath);
                        LOGDBG("Added graphics file mount from %s to %s\n", sourcePath.c_str(), destPath.c_str());
                    }
                    else
                    {
                        LOGWARN("Invalid file entry in graphics config, missing source or destination\n");
                    }
                }
            }
            else
            {
                LOGWARN("No vendorGpuSupport/files found in graphics config\n");
            }
        }
        else
        {
            LOGWARN("No vendorGpuSupport found in graphics config.\n");
        }
        return status;
    }

    bool RalfOCIConfigGenerator::addAdditionalEnvVariablesToOCIConfig(Json::Value &ociConfigRootNode, const WPEFramework::Exchange::RuntimeConfig &runtimeConfigObject, const WPEFramework::Plugin::ApplicationConfiguration &appConfig)
    {
        bool status = true;
        /* The following environmental variables expected
        APP_PACKAGE_VERSION, XDG_RUNTIME_DIR, and TEMP_STORAGE_PATH, READ_PATH,
        RUNTIME_CONFIG_OVERRIDES_JSON,APP_CONFIG_OVERRIDES_JSON,APP_PROVIDER_ID,
        CLIENT_CERT_KEY, CLIENT_CERT,LOG_LEVEL,STORAGE_LIMIT,GPU_MEMORY_LIMIT,
        CPU_MEMORY_LIMIT, DIAL_FRIENDLY_NAME, DIAL_ENABLED,

        TODO: We will need to define how to get the values for these environment variables.
        RDKEMW-13998
        */

        return status;
    }

    bool RalfOCIConfigGenerator::addDeviceNodeEntriesToOCIConfig(Json::Value &ociConfigRootNode, const Json::Value &graphicsDevNode)
    {
        bool status = graphicsDevNode.size() > 0 ? true : false; // If no entries, return true.
        for (Json::Value::ArrayIndex i = 0; i < graphicsDevNode.size(); ++i)
        {
            std::string devNodePath = graphicsDevNode[i].asString();
            unsigned int majorNum = 0, minorNum = 0;
            char devType = '\0';
            if (getDevNodeMajorMinor(devNodePath, majorNum, minorNum, devType))
            {
                Json::Value deviceNode;
                deviceNode[DEV_PATH] = devNodePath;
                deviceNode[DEV_TYPE] = std::string(1, devType);
                deviceNode[DEV_MAJOR] = majorNum;
                deviceNode[DEV_MINOR] = minorNum;

                ociConfigRootNode[LINUX][DEVICES].append(deviceNode);

                // Add in the resources devices section as well
                Json::Value resourceDevice;
                resourceDevice[DEV_TYPE] = std::string(1, devType);
                resourceDevice[DEV_MAJOR] = majorNum;
                resourceDevice[DEV_MINOR] = minorNum;
                resourceDevice[ACCESS] = "rwm";
                resourceDevice[ALLOW] = true;
                ociConfigRootNode[LINUX][RESOURCES][DEVICES].append(resourceDevice);
                LOGDBG("Added device node to OCI config: %s (type=%c, major=%u, minor=%u)\n", devNodePath.c_str(), devType, majorNum, minorNum);
            }
            else
            {
                LOGWARN("Failed to get major/minor for device node: %s\n", devNodePath.c_str());
            }
        }
        return status;
    }

    bool RalfOCIConfigGenerator::addEntryPointToOCIConfig(Json::Value &ociConfigRootNode, const Json::Value &ralfPackageConfigNode)
    {
        bool status = true;

        // Apply entryPoint if exists
        if (ralfPackageConfigNode.isMember(ENTRY_POINT))
        {
            // args is a array of strings. So append each string to args array
            ociConfigRootNode[PROCESS][ARGS].append(ralfPackageConfigNode[ENTRY_POINT]);
            LOGDBG("Applied entryPoint to OCI config\n");
        }
        else
        {
            LOGWARN("No entryPoint found in Ralf package config\n");
        }
        return status;
    }

    bool RalfOCIConfigGenerator::applyConfigurationToOCIConfig(Json::Value &ociConfigRootNode, Json::Value &manifestRootNode)
    {
        if (!addEntryPointToOCIConfig(ociConfigRootNode, manifestRootNode))
        {
            LOGERR("Failed to apply Ralf Entry point for package");
            return false;
        }
        // Everything else is optional. So return value will be true.
        //  Check if configuration node exists.
        if (!manifestRootNode.isMember(CONFIGURATION) || !manifestRootNode[CONFIGURATION].isObject())
        {
            LOGWARN("No configurations found in Ralf package manifest config.\n");
            return true; // Just return true since this is not a fatal error.
        }
        bool status = true;
        Json::Value configNode = manifestRootNode[CONFIGURATION];
        std::string packageType;

        if (manifestRootNode.isMember(PACKAGE_TYPE) && manifestRootNode[PACKAGE_TYPE].isString())
        {
            packageType = manifestRootNode[PACKAGE_TYPE].asString();
        }
        // Apply APP_CONFIG_OVERRIDES_JSON/RUNTIME_CONFIG_OVERRIDES_JSON variables
        if (packageType == PKG_TYPE_APPLICATION || packageType == PKG_TYPE_RUNTIME)
        {
            status = addConfigOverridesToOCIConfig(ociConfigRootNode, configNode);
            LOGDBG("Applied config overrides to OCI config ? %s\n", status ? "true" : "false");
        }
        // Apply "urn:rdk:config:memory", "urn:rdk:config:storage", "urn:rdk:config:network", reserved
        if (packageType == PKG_TYPE_APPLICATION || packageType == PKG_TYPE_RUNTIME)
        {
            status = addMemoryConfigToOCIConfig(ociConfigRootNode, configNode, packageType);
            LOGDBG("Applied memory config to OCI config ? %s\n", status ? "true" : "false");

            status = addStorageConfigToOCIConfig(ociConfigRootNode, configNode);
            LOGDBG("Applied storage config to OCI config ? %s\n", status ? "true" : "false");

            status = applyNetworkConfigToOCIConfig(ociConfigRootNode, configNode);
            LOGDBG("Applied network config to OCI config ? %s\n", status ? "true" : "false");
        }
        // Apply urn:rdk:config:env — spec matrix: Application/Service only (N/A for Runtime and Base)
        if (packageType == PKG_TYPE_APPLICATION || packageType == PKG_TYPE_SERVICE)
        {
            status = addConfigEnvToOCIConfig(ociConfigRootNode, configNode);
            LOGDBG("Applied config env to OCI config ? %s\n", status ? "true" : "false");
        }
        else if (configNode.isMember(ENV_CONFIG_URN))
        {
            LOGWARN("Ignoring %s for packageType '%s'; only valid for application/service packages\n", ENV_CONFIG_URN, packageType.c_str());
        }
        // Add APP_PACKAGE_VERSION environment variable from application config to OCI config
        if (packageType == PKG_TYPE_APPLICATION)
        {
            addAppPackageVersionToConfig(ociConfigRootNode, manifestRootNode);
        }

        return true;
    }
    bool RalfOCIConfigGenerator::addAppPackageVersionToConfig(Json::Value &ociConfigRootNode, Json::Value &manifestRootNode)
    {
        // Identifies the application package version. This value is used for logging, debugging, and version-specific behavior.
        // It must be derived from the version and versionName fields retrieved from the package metadata and formatted as 'versionName_version'.

        std::string version = manifestRootNode[VERSION].asString();
        std::string versionName = manifestRootNode[VERSION_NAME].asString();
        if (version.empty() || versionName.empty())
        {
            LOGERR("Version or versionName is missing in the manifest\n");
            return false;
        }
        std::string appPackageVersion = versionName + "_" + version;

        addToEnvironment(ociConfigRootNode, "APP_PACKAGE_VERSION", appPackageVersion);
        return true;
    }
    bool RalfOCIConfigGenerator::addStorageConfigToOCIConfig(Json::Value &ociConfigRootNode, Json::Value &configNode)
    {
        if (configNode.isMember(STORAGE_CONFIG_URN) && configNode[STORAGE_CONFIG_URN].isObject())
        {
            Json::Value storageConfig = configNode[STORAGE_CONFIG_URN];
            // Check if maxLocalStorage is defined
            if (storageConfig.isMember(MAX_LOCAL_STORAGE) && storageConfig[MAX_LOCAL_STORAGE].isString())
            {
                std::string maxLocalStorage = storageConfig[MAX_LOCAL_STORAGE].asString();
                uint64_t storageLimit = parseMemorySize(maxLocalStorage);
                if (storageLimit > 0)
                {
                    addToEnvironment(ociConfigRootNode, "STORAGE_LIMIT", std::to_string(storageLimit));
                    LOGDBG("Applied max local storage to OCI config: %s (bytes: %llu)\n", maxLocalStorage.c_str(), storageLimit);
                    return true;
                }
                LOGWARN("Invalid maxLocalStorage value '%s'. Using default storage limit\n", maxLocalStorage.c_str());
            }
        }
        LOGWARN("Storage configuration not found in the config node. Setting default values\n");
        addToEnvironment(ociConfigRootNode, "STORAGE_LIMIT", DEFAULT_STORAGE_LIMIT);

        return false;
    }
    bool RalfOCIConfigGenerator::addMemoryConfigToOCIConfig(Json::Value &ociConfigRootNode, const Json::Value &configNode, const std::string &packageType)
    {
        /*
          Expects a configuration of the form
          "configuration": {
            "urn:rdk:config:memory": {
            "system": "400M",
            "gpu": "128M"
            }
        }
            This needs to be mapped into
            linux.memory.limits
        */

        // Implementation for adding memory configuration
        if (configNode.isMember(MEMORY_CONFIG_URN) && configNode[MEMORY_CONFIG_URN].isObject())
        {
            Json::Value memoryConfig = configNode[MEMORY_CONFIG_URN];
            if (memoryConfig.isMember(SYSTEM_MEMORY) && memoryConfig[SYSTEM_MEMORY].isString())
            {
                //[LINUX][RESOURCES][MEMORY][MEMORY_LIMIT] exists in oci-base-spec file.
                // We need to convert the value from memoryConfig[SYSTEM_MEMORY] to bytes.
                const std::string systemMemoryStr = memoryConfig[SYSTEM_MEMORY].asString();
                uint64_t memoryLimit = parseMemorySize(systemMemoryStr);
                if (memoryLimit > 0)
                {
                    ociConfigRootNode[LINUX][RESOURCES][MEMORY][MEMORY_LIMIT] = memoryLimit;
                    LOGDBG("Applied system memory limit to OCI config: %llu\n", memoryLimit);
                    addToEnvironment(ociConfigRootNode, "CPU_MEMORY_LIMIT", std::to_string(memoryLimit));
                    return true;
                }
                else
                {
                    // Treat a 0 parse result as invalid: keep base-spec limit and use default for env.
                    LOGWARN("Invalid system memory configuration '%s'; keeping base-spec limit and using default CPU_MEMORY_LIMIT\n", systemMemoryStr.c_str());
                    addToEnvironment(ociConfigRootNode, "CPU_MEMORY_LIMIT", DEFAULT_RAM_LIMIT);
                    return false;
                }
            }
            // TODO not sure what to do with GPU memory for now
        }

        LOGWARN("Memory configuration not found in the config node. Setting default values\n");
        addToEnvironment(ociConfigRootNode, "CPU_MEMORY_LIMIT", DEFAULT_RAM_LIMIT);

        return false;
    }
    bool RalfOCIConfigGenerator::addFireboltEndPointToConfig(Json::Value &ociConfigRootNode, const std::string &envVar)
    {
        /*This string is a serialized for of json value .. An example is
        ["FIREBOLT_ENDPOINT=http:\/\/127.0.0.1:3473?session=810b474c-5f68-4cdf-82f2-86dc4d6d1f97","TARGET_STATE=4"]
        We need to parse it and get the FIREBOLT_ENDPOINT value and push it to OCI config
        */

        Json::CharReaderBuilder readerBuilder;
        Json::Value envVarsNode;
        std::string errs;
        std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());
        if (!reader->parse(envVar.c_str(), envVar.c_str() + envVar.size(), &envVarsNode, &errs))
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
                    std::string fireboltPrefix = std::string(FIREBOLT_ENDPOINT_ENV_KEY) + "=";
                    if (envPair.rfind(fireboltPrefix, 0) == 0)
                    {
                        addToEnvironment(ociConfigRootNode, FIREBOLT_ENDPOINT_ENV_KEY, envPair.substr(fireboltPrefix.size()));
                        LOGDBG("Added FIREBOLT_ENDPOINT environment variable: %s\n", envPair.c_str());
                        return true; // Found and added
                    }
                }
            }
        }
        LOGWARN("FIREBOLT_ENDPOINT environment variable not found in runtime config\n");
        return false;
    }

    bool RalfOCIConfigGenerator::addConfigOverridesToOCIConfig(Json::Value &ociConfigRootNode, const Json::Value &configNode)
    {
        bool status = false;
        if (configNode.isMember(CONFIG_OVERRIDES_URN) && configNode[CONFIG_OVERRIDES_URN].isObject())
        {
            // Serialize each override sub-object and export it as a separate environment variable in OCI config.
            // If an "application" node is present, store its serialized JSON under APP_CONFIG_OVERRIDES_ENV_KEY.
            Json::Value overrideNode = configNode[CONFIG_OVERRIDES_URN];

            if (overrideNode.isMember(PKG_TYPE_APPLICATION) && overrideNode[PKG_TYPE_APPLICATION].isObject())
            {
                std::string overrideJsonStr = serializeJsonNode(overrideNode[PKG_TYPE_APPLICATION]);
                addToEnvironment(ociConfigRootNode, APP_CONFIG_OVERRIDES_ENV_KEY, overrideJsonStr);
                LOGDBG("Added application config overrides to OCI config as environment variable: %s\n", APP_CONFIG_OVERRIDES_ENV_KEY);
                status = true;
            }
            // If a "runtime" node is present, store its serialized JSON under RUNTIME_CONFIG_OVERRIDES_ENV_KEY.
            if (overrideNode.isMember(PKG_TYPE_RUNTIME) && overrideNode[PKG_TYPE_RUNTIME].isObject())
            {
                std::string overrideJsonStr = serializeJsonNode(overrideNode[PKG_TYPE_RUNTIME]);
                addToEnvironment(ociConfigRootNode, RUNTIME_CONFIG_OVERRIDES_ENV_KEY, overrideJsonStr);
                LOGDBG("Added runtime config overrides to OCI config as environment variable: %s\n", RUNTIME_CONFIG_OVERRIDES_ENV_KEY);
                status = true;
            }
            if (!status)
            {
                LOGWARN("Config overrides node found but contains no 'application' or 'runtime' sub-objects\n");
            }
        }
        else
        {
            LOGWARN("No config overrides found in Ralf package config\n");
        }

        return status;
    }

    bool RalfOCIConfigGenerator::addConfigEnvToOCIConfig(Json::Value &ociConfigRootNode, const Json::Value &configNode)
    {
        if (!configNode.isMember(ENV_CONFIG_URN))
        {
            LOGDBG("No config env found in Ralf package config\n");
            return false;
        }

        const Json::Value &envNode = configNode[ENV_CONFIG_URN];
        if (!envNode.isObject())
        {
            LOGWARN("Config env node exists but is not an object; skipping\n");
            return false;
        }

        bool status = false;
        for (const auto &memberName : envNode.getMemberNames())
        {
            const Json::Value &valueNode = envNode[memberName];
            if (!valueNode.isString())
            {
                LOGWARN("Skipping non-string environment variable value in %s for key: %s\n", ENV_CONFIG_URN, memberName.c_str());
                continue;
            }
            addToEnvironment(ociConfigRootNode, memberName, valueNode.asString());
            status = true;
        }

        if (!status)
        {
            LOGWARN("Config env node found but contains no valid key/value entries\n");
        }
        return status;
    }

    void RalfOCIConfigGenerator::addTimezoneInfo(Json::Value &ociConfigRootNode)
    {
        /* As per HLA , three paths needs to be mounted.

        /usr/share/zoneinfo /usr/share/zoneinfo
        /opt/persistent/localtime /etc/localtime
        /opt/persistent/timeZoneDST /etc/timezone

        First one will be always present. Second and third are optional. Mount only if they are present.
        */
        addBindMountToOCIConfig(ociConfigRootNode, RALF_ZONE_INFO_PATH, RALF_ZONE_INFO_PATH, true);

        if (checkIfPathExists(RALF_HOST_LOCALTIME_PATH))
        {
            addBindMountToOCIConfig(ociConfigRootNode, RALF_HOST_LOCALTIME_PATH, RALF_LOCALTIME_PATH, true);
            LOGDBG("Added localtime mount from %s to %s\n", RALF_HOST_LOCALTIME_PATH.c_str(), RALF_LOCALTIME_PATH.c_str());
        }
        else
        {
            LOGWARN("Localtime file %s does not exist. Skipping mount.\n", RALF_HOST_LOCALTIME_PATH.c_str());
        }

        if (checkIfPathExists(RALF_HOST_TIMEZONE_DST_PATH))
        {
            addBindMountToOCIConfig(ociConfigRootNode, RALF_HOST_TIMEZONE_DST_PATH, RALF_TIMEZONE_PATH, true);
            LOGDBG("Added timezone DST mount from %s to %s\n", RALF_HOST_TIMEZONE_DST_PATH.c_str(), RALF_TIMEZONE_PATH.c_str());
        }
        else
        {
            LOGWARN("Timezone DST file %s does not exist. Skipping mount.\n", RALF_HOST_TIMEZONE_DST_PATH.c_str());
        }
    }

    void RalfOCIConfigGenerator::addToEnvironment(Json::Value &ociConfigRootNode, const std::string &key, const std::string &value)
    {
        // Upsert: remove any existing entry for this key, then append the new value.
        // This enforces the precedence rule (last write wins) and keeps process.env dedup-clean.
        std::string envVar = key + "=" + value;

        Json::Value &processNode = ociConfigRootNode[PROCESS];
        if (!processNode.isObject())
        {
            processNode = Json::Value(Json::objectValue);
        }

        if (!processNode[ENV].isArray())
        {
            processNode[ENV] = Json::Value(Json::arrayValue);
        }
        // Build a deduplicated array, dropping any existing entry for this key.
        const std::string prefix = key + "=";
        Json::Value deduped(Json::arrayValue);
        for (const auto &existing : processNode[ENV])
        {
            if (!existing.isString() || existing.asString().rfind(prefix, 0) != 0)
                deduped.append(existing);
            else
                LOGWARN("Removed duplicate environment variable from OCI config: %s\n", existing.asString().c_str());
        }
        deduped.append(envVar);
        processNode[ENV] = deduped;
        LOGDBG("Added environment variable to OCI config: %s\n", envVar.c_str());
    }

    bool RalfOCIConfigGenerator::applyNetworkConfigToOCIConfig(Json::Value &ociConfigRootNode, const Json::Value &configNode)
    {
        if (!configNode.isMember(NETWORK_CONFIG_URN))
        {
            LOGDBG("No network configuration found in config node\n");
            return true; // Optional configuration, not an error
        }

        const Json::Value &networkConfig = configNode[NETWORK_CONFIG_URN];
        if (!networkConfig.isArray())
        {
            LOGWARN("Network configuration is not an array, skipping\n");
            return true;
        }

        Json::Value &portForwarding = ociConfigRootNode[RDKPLUGINS][NETWORKING][DATA][PORT_FORWARDING];
        if (!portForwarding.isObject())
        {
            portForwarding = Json::Value(Json::objectValue);
        }

        // Route entries to Dobby's hostToContainer or containerToHost arrays based on 'type':
        //   "public" / "exported" -> hostToContainer  (inbound: external traffic reaches this container)
        //   "imported"            -> containerToHost  (outbound: this container reaches a host/peer service)
        //   unspecified           -> hostToContainer  (safe default)
        for (const auto &entry : networkConfig)
        {
            if (!entry.isObject())
            {
                LOGWARN("Network config entry is not an object, skipping\n");
                continue;
            }

            // Port is required for a meaningful forwarding rule
            if (!entry.isMember(PORT) || !entry[PORT].isInt())
            {
                LOGWARN("Network config entry missing or invalid 'port' field, skipping\n");
                continue;
            }

            std::string name     = (entry.isMember(NAME)     && entry[NAME].isString())     ? entry[NAME].asString()     : "unnamed";
            std::string type     = (entry.isMember(TYPE)     && entry[TYPE].isString())     ? entry[TYPE].asString()     : "";
            std::string protocol = (entry.isMember(PROTOCOL) && entry[PROTOCOL].isString()) ? entry[PROTOCOL].asString() : "tcp";

            Json::Value portEntry(Json::objectValue);
            portEntry[PORT]     = entry[PORT].asInt();
            portEntry[PROTOCOL] = protocol;

            if (type == "imported")
            {
                portForwarding[CONTAINER_TO_HOST].append(portEntry);
                LOGDBG("Added containerToHost port %d/%s for '%s'\n", entry[PORT].asInt(), protocol.c_str(), name.c_str());
            }
            else
            {
                portForwarding[HOST_TO_CONTAINER].append(portEntry);
                LOGDBG("Added hostToContainer port %d/%s for '%s' (type='%s')\n", entry[PORT].asInt(), protocol.c_str(), name.c_str(), type.c_str());
            }
        }

        LOGDBG("Network configuration applied to OCI config\n");
        return true;
    }

    bool RalfOCIConfigGenerator::hasCapabilityPermission(const std::string &capabilities, const std::string &permission)
    {
        if (capabilities.empty() || permission.empty())
        {
            return false;
        }

        size_t pos = 0;
        const std::string delimiter = ",";

        while (pos < capabilities.length())
        {
            size_t end = capabilities.find(delimiter, pos);
            if (end == std::string::npos)
            {
                end = capabilities.length();
            }

            std::string token = capabilities.substr(pos, end - pos);

            // Trim whitespace
            token.erase(0, token.find_first_not_of(" \t\n\r\f\v"));
            token.erase(token.find_last_not_of(" \t\n\r\f\v") + 1);

            if (token == permission)
            {
                LOGDBG("Found capability permission '%s'\n", permission.c_str());
                return true;
            }

            pos = end + delimiter.length();
        }

        return false;
    }

    bool RalfOCIConfigGenerator::hasInternetPermission(const std::string &capabilities)
    {
        return hasCapabilityPermission(capabilities, PERMISSION_INTERNET);
    }
} // namespace ralf
