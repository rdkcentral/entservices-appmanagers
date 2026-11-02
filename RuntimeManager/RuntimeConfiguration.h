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

#include <cstdint>
#include <string>
#include <vector>

namespace WPEFramework {
namespace Plugin {

struct RuntimeConfiguration {
    RuntimeConfiguration();

    bool dial;
    bool wanLanAccess;
    bool thunder;
    int32_t systemMemoryLimit;
    int32_t gpuMemoryLimit;
    std::vector<std::string> envVariables;
    uint32_t userId;
    uint32_t groupId;
    uint32_t dataImageSize;

    bool resourceManagerClientEnabled;
    std::string dialId;
    std::string command;
    std::string appType;
    std::string appPath;
    std::string runtimePath;

    std::string logFilePath;
    uint32_t logFileMaxSize;
    std::vector<std::string> logLevels;
    bool mapi;
    std::vector<std::string> fkpsFiles;
    std::string capabilities;
    std::string ralfPkgPath;

    std::string fireboltVersion;
    bool enableDebugger;
    std::string unpackedPath;
};

class RuntimeConfigurationDecoder {
public:
    static bool Decode(const std::string& payload, RuntimeConfiguration& configuration, std::string& error, uint32_t defaultUserId = 0, uint32_t defaultGroupId = 0, bool requireCommand = true);
};

} // namespace Plugin
} // namespace WPEFramework
