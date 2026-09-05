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
#include <string>
#include <utility>

namespace ralf
{
    const std::string RALF_OCI_BASE_SPEC_FILE = "/usr/share/ralf/oci-base-spec.json";
    const std::string RALF_GRAPHICS_LAYER_PATH = "/usr/share/gpu-layer/";
    const std::string RALF_GRAPHICS_LAYER_ROOTFS = RALF_GRAPHICS_LAYER_PATH + "rootfs";
    const std::string RALF_GRAPHICS_LAYER_CONFIG = RALF_GRAPHICS_LAYER_PATH + "config.json";
    const std::string RALF_OVERLAYFS_TYPE = "overlay";
    const std::string RALF_APP_ROOTFS_DIR = "/tmp/ralf/";
    const std::string RALF_USER_NAME = "ralf";
    const std::string RALF_ZONE_INFO_PATH = "/usr/share/zoneinfo";
    const std::string RALF_HOST_LOCALTIME_PATH = "/opt/persistent/localtime";
    const std::string RALF_HOST_TIMEZONE_DST_PATH = "/opt/persistent/timeZoneDST";
    const std::string RALF_TIMEZONE_PATH = "/etc/timezone";
    const std::string RALF_LOCALTIME_PATH = "/etc/localtime";
    const std::string RALF_DEFAULT_RESOLV_CONF_FILE = "/etc/resolv.conf";
    const std::string RALF_HOST_DEFAULT_RESOLV_CONF_FILE = "/etc/resolv.conf";
    const std::string RALF_HOST_NOSTUB_NWMGR_RESOLV_CONF_FILE = "/run/NetworkManager/no-stub-resolv.conf";
    const std::string RALF_HOST_NOSTUB_SYSTEMD_RESOLV_CONF_FILE = "/run/systemd/resolve/resolv.conf";

    typedef std::pair<std::string, std::string> RalfPkgInfoPair; // <packageMetadataJsonPath, mountPoint>
} // namespace ralf
