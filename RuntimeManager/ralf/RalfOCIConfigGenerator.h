/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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

#pragma once
#include <string>
#include <json/json.h>
#include "RalfConstants.h"
#include "../ApplicationConfiguration.h"
#include <interfaces/IRuntimeManager.h>
namespace ralf
{
    class RalfOCIConfigGenerator
    {
    public:
        RalfOCIConfigGenerator(const std::string &configFilePath, const std::vector<RalfPkgInfoPair> &ralfPackages) : mRalfPackages(ralfPackages), mConfigFilePath(configFilePath) {}
        virtual ~RalfOCIConfigGenerator() {}
        /**
         * Generates the OCI config JSON for the RALF application instance.
         * @param config The application configuration.
         * @param runtimeConfigObject The runtime configuration.
         */
        bool generateRalfOCIConfig(const WPEFramework::Plugin::ApplicationConfiguration &config, const WPEFramework::Exchange::RuntimeConfig &runtimeConfigObject);

    private:
        /**
         * Applies the graphics configuration to the OCI config JSON.
         * @param ociConfigRootNode The root node of the OCI config JSON.
         * @param graphicsConfigNode The graphics configuration JSON node.
         */
        bool applyGraphicsConfigToOCIConfig(Json::Value &ociConfigRootNode, const Json::Value &graphicsConfigNode);

        bool generateHooksForOCIConfig(Json::Value &ociConfigRootNode);
        bool generateHooksForOCIConfig(Json::Value &ociConfigRootNode, const std::string &operation);

        /**
         * Applies the entrypoint options from the Ralf package to the OCI config JSON.
         * @param ociConfigRootNode The root node of the OCI config JSON.
         * @param ralfPackageConfigNode The Ralf package configuration JSON node.
         * @return true if the configuration options were applied successfully, false otherwise.
         */
        bool addEntryPointToOCIConfig(Json::Value &ociConfigRootNode, const Json::Value &ralfPackageConfigNode);

        /**
         * Saves the OCI config JSON to mConfigFilePath.
         * @param ociConfigRootNode The root node of the OCI config JSON.
         * @param uid The user ID to set as the owner of the file.
         * @param gid The group ID to set as the owner of the file.
         * @return true if the OCI config was saved successfully, false otherwise.
         */
        bool saveOCIConfigToFile(const Json::Value &ociConfigRootNode, int uid, int gid);

        /**
         * Applies runtime and application configuration to the OCI config JSON.
         * @param ociConfigRootNode The root node of the OCI config JSON.
         * @param runtimeConfigObject The runtime configuration.
         * @param config The application configuration.
         * @return true if the configuration options were applied successfully, false otherwise.
         */
        bool applyRuntimeAndAppConfigToOCIConfig(Json::Value &ociConfigRootNode, const WPEFramework::Exchange::RuntimeConfig &runtimeConfigObject, const WPEFramework::Plugin::ApplicationConfiguration &config);

        /**
         * Applies the configuration from the package config to the OCI config JSON.
         * @param ociConfigRootNode The root node of the OCI config JSON.
         * @param manifestRootNode The manifest root node.
         * @return true if the configuration was applied successfully, false otherwise.
         */
        bool applyConfigurationToOCIConfig(Json::Value &ociConfigRootNode, Json::Value &manifestRootNode);
        /**
         * Adds a mount entry to the OCI config JSON.
         * @param ociConfigRootNode The root node of the OCI config JSON.
         * @param source The source path of the mount.
         * @param destination The destination path of the mount.
         */
        void addMountEntry(Json::Value &ociConfigRootNode, const std::string &source, const std::string &destination);
        /**
         * Add device node entries from graphics config to OCI config.
         * @param ociConfigRootNode The root node of the OCI config JSON.
         * @param graphicsDevNode The devNode configuration JSON node.
         * @return true if device nodes were added successfully, false otherwise.
         */
        bool addDeviceNodeEntriesToOCIConfig(Json::Value &ociConfigRootNode, const Json::Value &graphicsDevNode);

        /**
         * Mount storage path for the application in the OCI config and export the location as an environment variable.
         * @param ociConfigRootNode The root node of the OCI config JSON.
         * @param appStoragePath The storage path for the application in the host.
         * @return true if storage path was added successfully, false otherwise.
         */

        bool addAppStorageToOCIConfig(Json::Value &ociConfigRootNode, const std::string &appStoragePath);
        /**
         * Add additional environmental variables as defined in specification.
         * @param ociConfigRootNode The root node of the OCI config JSON.
         * @param runtimeConfigObject The runtime configuration object containing the environment variables to be added.
         * @param appConfig The application configuration containing the environment variables to be added.
         * @return true if environment variables were added successfully, false otherwise.
         */
        bool addAdditionalEnvVariablesToOCIConfig(Json::Value &ociConfigRootNode, const WPEFramework::Exchange::RuntimeConfig &runtimeConfigObject, const WPEFramework::Plugin::ApplicationConfiguration &appConfig);
        /**
         * Reads the environment variables JSON array string from WPEFramework::Exchange::RuntimeConfig.envVariables and pushes them to OCI config.
         * We are specifically looking to update the FIREBOLT_ENDPOINT environment variable here.
         * @param ociConfigRootNode The root node of the OCI config JSON.
         * @param envVar The serialized JSON array string of environment variables (e.g. "[\"KEY=VALUE\", ...]") as provided by RuntimeConfig.envVariables.
         * @return true if the environment variable was added successfully, false otherwise.
         */
        bool addFireboltEndPointToConfig(Json::Value &ociConfigRootNode, const std::string &envVar);

        /**
         * Adds configuration overrides from the Ralf package config to the OCI config JSON.
         * As per the current specification, these are expected to be added as environment variables in the OCI config.
         * The environment variable key will be APP_CONFIG_OVERRIDES_JSON or RUNTIME_CONFIG_OVERRIDES_JSON depending
         * on the source of the config overrides. The value will be the serialized JSON string of the config overrides.
         * @param ociConfigRootNode The root node of the OCI config JSON.
         * @param configNode  The config node containing the overrides.
         * @param packageType The type of the Ralf package (e.g., "application" or "runtime").
         * @return true if the configuration overrides were added successfully, false otherwise.
         */
        bool addConfigOverridesToOCIConfig(Json::Value &ociConfigRootNode, const Json::Value &configNode, const std::string &packageType);

        /**
         * Adds memory configuration from the Ralf package config to the OCI config JSON.
         * if packageType is "application", it will override any configuration that runtime written.
         * @param ociConfigRootNode The root node of the OCI config JSON.
         * @param configNode The config node containing the memory configuration.
         * @param packageType The type of the Ralf package (e.g., "application" or "runtime").
         * @return true if the memory configuration was added from package, false if default value is added.
         */

        bool addMemoryConfigToOCIConfig(Json::Value &ociConfigRootNode, const Json::Value &configNode, const std::string &packageType);
        /**
         * Adds a key-value pair to the environment section of the OCI config JSON.
         * @param ociConfigRootNode The root node of the OCI config JSON.
         * @param key The environment variable key.
         * @param value The environment variable value.
         */
        void addToEnvironment(Json::Value &ociConfigRootNode, const std::string &key, const std::string &value);

        /**
         * Adds the application package version to the OCI config as an environment variable named APP_PACKAGE_VERSION.
         * Identifies the application package version. This value is used for logging, debugging, and version-specific behavior.
         * It must be derived from the version and versionName fields retrieved from the package metadata and formatted as 'versionName_version'.
         * @param ociConfigRootNode The root node of the OCI config JSON.
         * @param manifestRootNode The root node of the manifest JSON containing the version information.
         * @return true if the version was added successfully, false otherwise.
         */
        bool addAppPackageVersionToConfig(Json::Value &ociConfigRootNode, Json::Value &manifestRootNode);

        /**
         * Add the storage configuration from the Ralf package config to the OCI config JSON.
         * Specifies the maximum local storage (in MB) allocated to the container. The runtime manager MUST
         * retrieve urn:rdk:config:storage from the package metadata and set the value of the maxLocalStorage
         * field as this environment variable.
         * @param ociConfigRootNode The root node of the OCI config JSON.
         * @param manifestRootNode The root node of the manifest JSON containing the storage information.
         * @return true if the storage configuration was added from package, false if default value is addded.
         */
        bool addStorageConfigToOCIConfig(Json::Value &ociConfigRootNode, Json::Value &manifestRootNode);

        /**
         * The vector of Ralf package details as pairs of mount point and metadata path.
         */
        const std::vector<RalfPkgInfoPair> &mRalfPackages;
        /**
         * The OCI config file path to be generated.
         */
        const std::string &mConfigFilePath;
    };
} // namespace ralf
