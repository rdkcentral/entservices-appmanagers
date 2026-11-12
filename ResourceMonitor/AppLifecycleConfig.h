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

#pragma once

#include <cstdint>
#include <string>

namespace WPEFramework {
namespace Plugin {

    // Lifecycle states an application package can support. ACTIVE is always supported.
    enum class AppLifecycleState : uint8_t {
        ACTIVE = 1 << 0,
        PAUSED = 1 << 1,
        SUSPENDED = 1 << 2,
        HIBERNATED = 1 << 3
    };

    enum class LifecycleConfigStatus : uint8_t {
        CONFIGURED,     // RALF package defines urn:rdk:config:application-lifecycle
        NOT_CONFIGURED, // RALF package without the lifecycle block: only ACTIVE is supported
        UNAVAILABLE     // No data to consult (legacy package, Package Manager unavailable, ambiguous or malformed)
    };

    struct SuspendedStateConfig {
        bool hasMaxSystemMemory = false;
        uint64_t maxSystemMemoryBytes = 0;   // from maxSuspendedSystemMemory
        bool hasMaxTimeToSuspend = false;
        uint32_t maxTimeToSuspendSec = 0;    // from maxTimeToSuspendMemoryState
    };

    struct AppLifecycleCapabilities {
        LifecycleConfigStatus status = LifecycleConfigStatus::UNAVAILABLE;
        uint8_t supportedStates = static_cast<uint8_t>(AppLifecycleState::ACTIVE);
        SuspendedStateConfig suspended;

        bool IsSupported(const AppLifecycleState state) const
        {
            return (supportedStates & static_cast<uint8_t>(state)) != 0;
        }
    };

    namespace AppLifecycleConfig {
        // Interprets the applicationLifecycle member of a Package Manager runtime configuration payload.
        // Never throws; degrades to UNAVAILABLE / NOT_CONFIGURED and logs as described in the
        // app-lifecycle-capabilities spec. Returns out.status.
        LifecycleConfigStatus Parse(const std::string& runtimeConfigPayload, AppLifecycleCapabilities& out);
    }

} // namespace Plugin
} // namespace WPEFramework
