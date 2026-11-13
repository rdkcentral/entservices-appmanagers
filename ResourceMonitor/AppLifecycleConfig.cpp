/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2026 RDK Management
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

#include "AppLifecycleConfig.h"

#include "Module.h"
#include "RuntimeConfigPayload.h"
#include "UtilsLogging.h"
#include "UtilsMemorySize.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <limits>

namespace WPEFramework {
namespace Plugin {
namespace AppLifecycleConfig {

    namespace {
        const char* const kLifecycleMember = "applicationLifecycle";

        std::string ToLower(std::string text)
        {
            std::transform(text.begin(), text.end(), text.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return text;
        }

        uint8_t Bit(const AppLifecycleState state)
        {
            return static_cast<uint8_t>(state);
        }

        void ParseStates(const JsonObject& lifecycle, AppLifecycleCapabilities& out)
        {
            if (!lifecycle.HasLabel("supportedNonActiveStates")) {
                LOGWARN("applicationLifecycle has no supportedNonActiveStates; only ACTIVE is supported");
                return;
            }
            if (lifecycle["supportedNonActiveStates"].Content() != JsonValue::type::ARRAY) {
                LOGWARN("applicationLifecycle.supportedNonActiveStates is not an array; only ACTIVE is supported");
                return;
            }
            const JsonArray states = lifecycle["supportedNonActiveStates"].Array();
            for (uint16_t index = 0; index < states.Length(); ++index) {
                if (states[index].Content() != JsonValue::type::STRING) {
                    LOGWARN("Ignoring non-string entry in supportedNonActiveStates");
                    continue;
                }
                const std::string name = ToLower(states[index].String());
                if (name == "paused") {
                    out.supportedStates |= Bit(AppLifecycleState::PAUSED);
                } else if (name == "suspended") {
                    out.supportedStates |= Bit(AppLifecycleState::SUSPENDED);
                } else if (name == "hibernated") {
                    out.supportedStates |= Bit(AppLifecycleState::HIBERNATED);
                } else {
                    LOGWARN("Ignoring unknown lifecycle state '%s'", states[index].String().c_str());
                }
            }

            const uint8_t optional = out.supportedStates & static_cast<uint8_t>(~Bit(AppLifecycleState::ACTIVE));
            const uint8_t paused = Bit(AppLifecycleState::PAUSED);
            const uint8_t pausedSuspended = paused | Bit(AppLifecycleState::SUSPENDED);
            const uint8_t all = pausedSuspended | Bit(AppLifecycleState::HIBERNATED);
            if ((optional != 0) && (optional != paused) && (optional != pausedSuspended) && (optional != all)) {
                LOGWARN("supportedNonActiveStates is not one of the combinations defined by the metadata specification");
            }
        }

        void ParseSuspendedConfig(const JsonObject& lifecycle, SuspendedStateConfig& out)
        {
            if (!lifecycle.HasLabel("maxSuspendedSystemMemory")) {
                LOGWARN("applicationLifecycle has no maxSuspendedSystemMemory");
            } else if ((lifecycle["maxSuspendedSystemMemory"].Content() != JsonValue::type::STRING) ||
                !::Utils::ParseMemorySize(lifecycle["maxSuspendedSystemMemory"].String(), out.maxSystemMemoryBytes)) {
                LOGWARN("applicationLifecycle.maxSuspendedSystemMemory is invalid");
                out.maxSystemMemoryBytes = 0;
            } else {
                out.hasMaxSystemMemory = true;
            }

            if (!lifecycle.HasLabel("maxTimeToSuspendMemoryState")) {
                LOGWARN("applicationLifecycle has no maxTimeToSuspendMemoryState");
                return;
            }
            const JsonValue& time = lifecycle["maxTimeToSuspendMemoryState"];
            const std::string encoded = (time.Content() == JsonValue::type::NUMBER) ? time.Value() : std::string();
            char* end = nullptr;
            errno = 0;
            const unsigned long long seconds = encoded.empty() ? 0 : std::strtoull(encoded.c_str(), &end, 10);
            if (encoded.empty() || (encoded[0] == '-') || (errno == ERANGE) || (end == encoded.c_str()) || (*end != '\0') ||
                (seconds > std::numeric_limits<uint32_t>::max())) {
                LOGWARN("applicationLifecycle.maxTimeToSuspendMemoryState is invalid");
                return;
            }
            out.maxTimeToSuspendSec = static_cast<uint32_t>(seconds);
            out.hasMaxTimeToSuspend = true;
        }
    }

    LifecycleConfigStatus Parse(const std::string& runtimeConfigPayload, AppLifecycleCapabilities& out)
    {
        out = AppLifecycleCapabilities();

        Utils::RuntimeConfigPayload payload;
        std::string error;
        if (!payload.Parse(runtimeConfigPayload, error)) {
            LOGWARN("Runtime configuration payload is not valid: %s", error.c_str());
            return out.status;
        }

        std::string ralfPkgPath;
        bool present = false;
        if (!payload.GetString("ralfPkgPath", ralfPkgPath, present, error) || !present || ralfPkgPath.empty()) {
            LOGINFO("No RALF package metadata available; lifecycle configuration unavailable");
            return out.status;
        }

        std::string lifecycleText;
        if (!payload.GetObject(kLifecycleMember, lifecycleText, present, error)) {
            LOGWARN("Invalid applicationLifecycle: %s", error.c_str());
            return out.status;
        }
        if (!present) {
            out.status = LifecycleConfigStatus::NOT_CONFIGURED;
            return out.status;
        }

        JsonObject lifecycle;
        if (!lifecycle.FromString(lifecycleText)) {
            LOGWARN("applicationLifecycle could not be decoded");
            return out.status;
        }

        out.status = LifecycleConfigStatus::CONFIGURED;
        ParseStates(lifecycle, out);
        ParseSuspendedConfig(lifecycle, out.suspended);
        return out.status;
    }

} // namespace AppLifecycleConfig
} // namespace Plugin
} // namespace WPEFramework
