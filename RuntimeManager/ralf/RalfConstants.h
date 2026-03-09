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

    typedef std::pair<std::string, std::string> RalfPkgInfoPair; // <packageMetadataJsonPath, mountPoint>
} // namespace ralf
