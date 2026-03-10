/**
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Comcast Cable Communications Management, LLC
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
 */

#pragma once

#include <chrono>
#include <string>
#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

#ifdef ENABLE_RDKAPPMANAGERS_DEBUG

/**
 * @class ScopeLogger
 * @brief RAII-based scope logger for timing API execution
 * 
 * This class logs entry and exit timestamps for instrumented APIs,
 * allowing downstream analysis of API execution times, cross-plugin
 * invocation latencies, and COM-RPC overhead.
 */
class ScopeLogger {
public:
    /**
     * @brief Construct and log entry timestamp
     * @param pluginName Name of the plugin (e.g., "AppManager")
     * @param apiName Name of the API being called (e.g., "Launch")
     */
    ScopeLogger(const std::string& pluginName, const std::string& apiName)
        : m_pluginName(pluginName)
        , m_apiName(apiName)
        , m_startTime(std::chrono::system_clock::now())
    {
        auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            m_startTime.time_since_epoch()).count();
        
        LOGINFO("[RDKAPPMANAGERS_TIMING] ENTRY plugin=%s api=%s timestamp=%lld thread=0x%lx",
                m_pluginName.c_str(),
                m_apiName.c_str(),
                static_cast<long long>(timestamp),
                static_cast<unsigned long>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
    }

    /**
     * @brief Destruct and log exit timestamp with duration
     */
    ~ScopeLogger()
    {
        auto endTime = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime.time_since_epoch()).count();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - m_startTime).count();
        
        LOGINFO("[RDKAPPMANAGERS_TIMING] EXIT plugin=%s api=%s timestamp=%lld duration_us=%lld thread=0x%lx",
                m_pluginName.c_str(),
                m_apiName.c_str(),
                static_cast<long long>(timestamp),
                static_cast<long long>(duration),
                static_cast<unsigned long>(std::hash<std::thread::id>{}(std::this_thread::get_id())));
    }

private:
    std::string m_pluginName;
    std::string m_apiName;
    std::chrono::system_clock::time_point m_startTime;
};

/**
 * @def RDKAPPMANAGERS_DEBUG_TIME_SCOPE
 * @brief Macro to instrument API scope with timing logger
 * 
 * Usage: Place at the beginning of any API method:
 *   RDKAPPMANAGERS_DEBUG_TIME_SCOPE("AppManager", "Launch");
 * 
 * This will automatically log entry/exit with timestamps.
 */
#define RDKAPPMANAGERS_DEBUG_TIME_SCOPE(pluginName, apiName) \
    ScopeLogger __scopeLogger##__LINE__(pluginName, apiName)

#else

// When debug timing is disabled, macro expands to nothing
#define RDKAPPMANAGERS_DEBUG_TIME_SCOPE(pluginName, apiName) do {} while(0)

#endif // ENABLE_RDKAPPMANAGERS_DEBUG

} // namespace Plugin
} // namespace WPEFramework
