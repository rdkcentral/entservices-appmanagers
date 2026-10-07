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

#include "RuntimeConfiguration.h"

#include "RuntimeConfigPayload.h"

#include <limits>
#include <utility>

namespace WPEFramework {
namespace Plugin {

RuntimeConfiguration::RuntimeConfiguration()
    : dial(false)
    , wanLanAccess(false)
    , thunder(false)
    , systemMemoryLimit(0)
    , gpuMemoryLimit(0)
    , envVariables()
    , userId(0)
    , groupId(0)
    , dataImageSize(0)
    , resourceManagerClientEnabled(false)
    , dialId()
    , command()
    , appType()
    , appPath()
    , runtimePath()
    , logFilePath()
    , logFileMaxSize(0)
    , logLevels()
    , mapi(false)
    , fkpsFiles()
    , capabilities()
    , ralfPkgPath()
    , fireboltVersion()
    , enableDebugger(false)
    , unpackedPath()
{
}

namespace {

bool GetInt32(const Utils::RuntimeConfigPayload& payload, const std::string& key, int32_t& destination, std::string& error)
{
    int64_t value = 0;
    bool present = false;
    if (!payload.GetInteger(key, value, present, error)) {
        return false;
    }
    if (present) {
        if (value < std::numeric_limits<int32_t>::min() || value > std::numeric_limits<int32_t>::max()) {
            error = key + " is outside the int32 range";
            return false;
        }
        destination = static_cast<int32_t>(value);
    }
    return true;
}

bool GetUint32(const Utils::RuntimeConfigPayload& payload, const std::string& key, uint32_t& destination, std::string& error)
{
    uint64_t value = 0;
    bool present = false;
    if (!payload.GetUnsigned(key, value, present, error)) {
        return false;
    }
    if (present) {
        if (value > std::numeric_limits<uint32_t>::max()) {
            error = key + " is outside the uint32 range";
            return false;
        }
        destination = static_cast<uint32_t>(value);
    }
    return true;
}

bool GetString(const Utils::RuntimeConfigPayload& payload, const std::string& key, std::string& destination, std::string& error)
{
    bool present = false;
    return payload.GetString(key, destination, present, error);
}

bool GetBoolean(const Utils::RuntimeConfigPayload& payload, const std::string& key, bool& destination, std::string& error)
{
    bool present = false;
    return payload.GetBoolean(key, destination, present, error);
}

bool GetStringArray(const Utils::RuntimeConfigPayload& payload, const std::string& key, std::vector<std::string>& destination, std::string& error)
{
    bool present = false;
    return payload.GetStringArray(key, destination, present, error);
}

} // namespace

bool RuntimeConfigurationDecoder::Decode(const std::string& serialized, RuntimeConfiguration& configuration, std::string& error, uint32_t defaultUserId, uint32_t defaultGroupId, bool requireCommand)
{
    Utils::RuntimeConfigPayload payload;
    if (!payload.Parse(serialized, error)) {
        return false;
    }

    RuntimeConfiguration decoded;
    decoded.userId = defaultUserId;
    decoded.groupId = defaultGroupId;
    if (!GetBoolean(payload, "dial", decoded.dial, error)
        || !GetBoolean(payload, "wanLanAccess", decoded.wanLanAccess, error)
        || !GetBoolean(payload, "thunder", decoded.thunder, error)
        || !GetInt32(payload, "systemMemoryLimit", decoded.systemMemoryLimit, error)
        || !GetInt32(payload, "gpuMemoryLimit", decoded.gpuMemoryLimit, error)
        || !GetStringArray(payload, "envVariables", decoded.envVariables, error)
        || !GetUint32(payload, "userId", decoded.userId, error)
        || !GetUint32(payload, "groupId", decoded.groupId, error)
        || !GetUint32(payload, "dataImageSize", decoded.dataImageSize, error)
        || !GetBoolean(payload, "resourceManagerClientEnabled", decoded.resourceManagerClientEnabled, error)
        || !GetString(payload, "dialId", decoded.dialId, error)
        || !GetString(payload, "command", decoded.command, error)
        || !GetString(payload, "appType", decoded.appType, error)
        || !GetString(payload, "appPath", decoded.appPath, error)
        || !GetString(payload, "runtimePath", decoded.runtimePath, error)
        || !GetString(payload, "logFilePath", decoded.logFilePath, error)
        || !GetUint32(payload, "logFileMaxSize", decoded.logFileMaxSize, error)
        || !GetStringArray(payload, "logLevels", decoded.logLevels, error)
        || !GetBoolean(payload, "mapi", decoded.mapi, error)
        || !GetStringArray(payload, "fkpsFiles", decoded.fkpsFiles, error)
        || !GetString(payload, "capabilities", decoded.capabilities, error)
        || !GetString(payload, "ralfPkgPath", decoded.ralfPkgPath, error)
        || !GetString(payload, "fireboltVersion", decoded.fireboltVersion, error)
        || !GetBoolean(payload, "enableDebugger", decoded.enableDebugger, error)
        || !GetString(payload, "unpackedPath", decoded.unpackedPath, error)) {
        return false;
    }

    if (requireCommand && decoded.command.empty()) {
        error = "command is mandatory and must not be empty";
        return false;
    }
    if (decoded.userId == 0) {
        error = "userId is mandatory and must not be zero";
        return false;
    }

    configuration = std::move(decoded);
    error.clear();
    return true;
}

} // namespace Plugin
} // namespace WPEFramework
