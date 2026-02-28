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

#define PERSIST_STORAGE_PATH "/data"
namespace ralf
{

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
        // Finally save the modified OCI config to file
        return saveOCIConfigToFile(ociConfigRootNode, config.mUserId, config.mGroupId);
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

        // Set westeros environment variable
        // mWesterosSocketPath has XDG_RUNTIME_DIR/WAYLAND_DISPLAY, we need to set WAYLAND_DISPLAY env variable to just the display
        // name (i.e. the last part of the path) because that is what westeros expects.
        std::string waylandDisplay = appConfig.mWesterosSocketPath.substr(appConfig.mWesterosSocketPath.find_last_of("/") + 1);
        addToEnvironment(ociConfigRootNode, "WAYLAND_DISPLAY", waylandDisplay);
        // Keep it consistent with what is set in DobbyPluginLauncher for Ralf package.
        // DobbyPluginLauncher will bind mount /tmp from host to container and westeros socket will be created inside /tmp in the container.
        addToEnvironment(ociConfigRootNode, "XDG_RUNTIME_DIR", "/tmp");

        // Need to mount bind  XDG_RUNTIME_DIR/WAYLAND_DISPLAY from host to container
        addMountEntry(ociConfigRootNode, appConfig.mWesterosSocketPath, appConfig.mWesterosSocketPath);

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
        addMountEntry(ociConfigRootNode, rialtoSocketPath, rialtoSocketPath);
        LOGDBG("Mounted rialto socket path %s to container path %s\n", rialtoSocketPath.c_str(), rialtoSocketPath.c_str());
        return status;
    }
    bool RalfOCIConfigGenerator::addAppStorageToOCIConfig(Json::Value &ociConfigRootNode, const std::string &appStoragePath)
    {
        bool status = false;
        // We will mount application storage to /home/root/appstorage and
        // set PERSIST_STORAGE_PATH environment variable to it.
        std::string containerStoragePath = PERSIST_STORAGE_PATH;
        addMountEntry(ociConfigRootNode, appStoragePath, containerStoragePath);
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
    void RalfOCIConfigGenerator::addMountEntry(Json::Value &ociConfigRootNode, const std::string &source, const std::string &destination)
    {
        Json::Value mountEntry;
        mountEntry[SOURCE] = source;
        mountEntry[DESTINATION] = destination;
        mountEntry[TYPE] = "bind";

        Json::Value mountOptions(Json::arrayValue);
        mountOptions.append("rbind");
        mountOptions.append("rw");
        mountEntry[OPTIONS] = mountOptions;

        ociConfigRootNode[MOUNT].append(mountEntry);
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
                        addMountEntry(ociConfigRootNode, sourcePath, destPath);
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
        // Everything else is optional. Sp return value will be true.
        //  Check if configuraton node exists.
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
            status = addConfigOverridesToOCIConfig(ociConfigRootNode, configNode, packageType);
            LOGDBG("Applied config overrides to OCI config ? %s\n", status ? "true" : "false");
        }
        // Apply "urn:rdk:config:memory", reserved
        if (packageType == PKG_TYPE_APPLICATION || packageType == PKG_TYPE_RUNTIME)
        {
            status = addMemoryConfigToOCIConfig(ociConfigRootNode, configNode, packageType);
            LOGDBG("Applied memory config to OCI config ? %s\n", status ? "true" : "false");

            status = addStorageConfigToOCIConfig(ociConfigRootNode, configNode);
            LOGDBG("Applied storage config to OCI config ? %s\n", status ? "true" : "false");
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
                        ociConfigRootNode[PROCESS][ENV].append(envPair);
                        LOGDBG("Added FIREBOLT_ENDPOINT environment variable: %s\n", envPair.c_str());
                        return true; // Found and added
                    }
                }
            }
        }
        LOGWARN("FIREBOLT_ENDPOINT environment variable not found in runtime config\n");
        return false;
    }

    bool RalfOCIConfigGenerator::addConfigOverridesToOCIConfig(Json::Value &ociConfigRootNode, const Json::Value &configNode, const std::string &packageType)
    {
        bool status = false;
        if (configNode.isMember(CONFIG_OVERRIDES_URN) && configNode[CONFIG_OVERRIDES_URN].isObject())
        {
            // We will serialize the override JSON object and add it as an environment variable in OCI config
            Json::Value overrideNode = configNode[CONFIG_OVERRIDES_URN];
            Json::StreamWriterBuilder writer;
            std::string overrideJsonStr = Json::writeString(writer, overrideNode);
            std::string envVarKey = packageType == PKG_TYPE_APPLICATION ? APP_CONFIG_OVERRIDES_ENV_KEY : RUNTIME_CONFIG_OVERRIDES_ENV_KEY;
            std::string envVar = envVarKey + "=" + overrideJsonStr;
            addToEnvironment(ociConfigRootNode, envVarKey, overrideJsonStr);
            LOGDBG("Added config overrides to OCI config as environment variable: %s\n", envVar.c_str());
            status = true;
        }
        else
        {
            LOGWARN("No config overrides found in Ralf package config\n");
        }

        return status;
    }
    void RalfOCIConfigGenerator::addToEnvironment(Json::Value &ociConfigRootNode, const std::string &key, const std::string &value)
    {
        // Ensure process/ENV exists and is an array before appending the new environment variable.
        std::string envVar = key + "=" + value;

        Json::Value &processNode = ociConfigRootNode[PROCESS];
        if (!processNode.isObject())
        {
            processNode = Json::Value(Json::objectValue);
        }

        Json::Value &envNode = processNode[ENV];
        if (!envNode.isArray())
        {
            envNode = Json::Value(Json::arrayValue);
        }

        envNode.append(envVar);
        LOGDBG("Added environment variable to OCI config: %s\n", envVar.c_str());
    }
} // namespace ralf
