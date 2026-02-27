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
#pragma once
/**
 * Aim is to give constant name instead of using strings in the codebase for better readability and maintainability. These constants are specific to the OCI config generation for Ralf and are not expected to be used outside of this context.
 * If there are constants that are more generic and can be used across different components, those should ideally be moved to a more common location.
 **/
#include <string>
namespace ralf
{
    constexpr const char *PROCESS = "process";
    constexpr const char *ENV = "env";
    constexpr const char *ARGS = "args";
    constexpr const char *USER = "user";
    constexpr const char *UID = "uid";
    constexpr const char *GID = "gid";
    constexpr const char *ADDITIONAL_GIDS = "additionalGids";
    constexpr const char *HOSTNAME = "hostname";
    constexpr const char *LINUX = "linux";
    constexpr const char *UID_MAPPINGS = "uidMappings";
    constexpr const char *GID_MAPPINGS = "gidMappings";
    constexpr const char *DEVICES = "devices";
    constexpr const char *RESOURCES = "resources";
    constexpr const char *DEV_TYPE = "type";
    constexpr const char *DEV_MAJOR = "major";
    constexpr const char *DEV_MINOR = "minor";
    constexpr const char *DEV_PATH = "path";
    constexpr const char *ACCESS = "access";
    constexpr const char *ALLOW = "allow";
    constexpr const char *PATH = "path";
    constexpr const char *MOUNT = "mounts";
    constexpr const char *SOURCE = "source";
    constexpr const char *DESTINATION = "destination";
    constexpr const char *TYPE = "type";
    constexpr const char *CONTAINER_ID = "containerID";
    constexpr const char *HOST_ID = "hostID";
    constexpr const char *SIZE = "size";
    constexpr const char *HOOKS = "hooks";
    constexpr const char *OPTIONS = "options";
    constexpr const char *VENDOR_GPU_SUPPORT = "vendorGpuSupport";
    constexpr const char *DEV_NODES = "devNodes";
    constexpr const char *GROUP_IDS = "groupIds";
    constexpr const char *FILES = "files";

    constexpr const char *ENTRY_POINT = "entryPoint";
    constexpr const char *CONFIGURATION = "configuration";
    constexpr const char *KEY = "key";
    constexpr const char *VALUE = "value";

    constexpr const char *PACKAGE_TYPE = "packageType";
    constexpr const char *PKG_TYPE_APPLICATION = "application";
    constexpr const char *PKG_TYPE_RUNTIME = "runtime";
    constexpr const char *FIREBOLT_ENDPOINT_ENV_KEY = "FIREBOLT_ENDPOINT";

    constexpr const char *CONFIG_OVERRIDES_URN = "urn:rdk:config:overrides";
    constexpr const char *MEMORY_CONFIG_URN = "urn:rdk:config:memory";
    constexpr const char *SYSTEM_MEMORY = "system";
    constexpr const char *MEMORY_LIMIT = "limit";
    constexpr const char *MEMORY = "memory";

    constexpr const char *VERSION = "version";
    constexpr const char *VERSION_NAME = "versionName";

    constexpr const char *STORAGE_CONFIG_URN = "urn:rdk:config:storage";
    constexpr const char *MAX_LOCAL_STORAGE = "maxLocalStorage";

    constexpr const char *RUNTIME_CONFIG_OVERRIDES_ENV_KEY = "RUNTIME_CONFIG_OVERRIDES_JSON";
    constexpr const char *APP_CONFIG_OVERRIDES_ENV_KEY = "APP_CONFIG_OVERRIDES_JSON";

    // 500 MB is the default RAM given to an application. Tied to resources/oci-base-spec.json
    // Any change to this should match that file.
    constexpr const char *DEFAULT_RAM_LIMIT = "524288000";
    // 100 MB is the default storage given to an application.
    constexpr const char *DEFAULT_STORAGE_LIMIT = "104857600";
} // namespace ralf
