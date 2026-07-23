/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2025 RDK Management
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

#include <filesystem>

namespace WPEFramework {
namespace Plugin {

/**
 * Generates a GStreamer plugin-registry binary and exposes its path.
 *
 * Mirrors appinfrastructure GStreamerRegistry.  Spawns gst-launch-1.0 --version
 * with GST_REGISTRY set to the target path so that GStreamer's registry-write
 * logic fires without starting a real pipeline.
 *
 * Intended to be used once at start-up; the generated registry is then
 * bind-mounted read-only into every container via setGstreamerRegistryPath().
 */
class GStreamerRegistry
{
public:
    explicit GStreamerRegistry(
        std::filesystem::path gstLaunchPath   = "gst-launch-1.0",
        std::filesystem::path gstRegistryPath = "/tmp/rdkappmanagers-gstreamer-registry.bin");

    ~GStreamerRegistry() = default;

    /** Generates the registry file.  Blocks up to ~2 s. Returns true on success. */
    bool generate();

    /** Returns the registry path if the file exists, otherwise an empty path. */
    std::filesystem::path path() const;

private:
    const std::filesystem::path mGstLaunchPath;
    const std::filesystem::path mRegistryPath;
};

} /* namespace Plugin */
} /* namespace WPEFramework */
