/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
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
 */

#pragma once

#ifdef ENABLE_RDKAPPMANAGERS_PERFMETRICS

#include "UtilsLogging.h"
#include <atomic>
#include <chrono>
#include <cinttypes>

namespace WPEFramework {
namespace Plugin {
namespace PerfMetrics {

class Scope {
public:
    Scope(const char* pluginApi, const char* calledPluginApi)
        : mPluginApi((pluginApi != nullptr) ? pluginApi : "UNKNOWN:UNKNOWN")
        , mCalledPluginApi((calledPluginApi != nullptr) ? calledPluginApi : mPluginApi)
        , mId(NextId())
    {
        LOGINFO("RDKAppManagers Time Details: %s: %" PRIu64 ": %s: %" PRIu64,
            mPluginApi,
            mId,
            mCalledPluginApi,
            EpochMs());
    }

    ~Scope()
    {
        LOGINFO("RDKAppManagers Time Details: %s: %" PRIu64 ": %s: %" PRIu64,
            mPluginApi,
            mId,
            mCalledPluginApi,
            EpochMs());
    }

private:
    static uint64_t NextId()
    {
        static std::atomic<uint64_t> nextId{1};
        return nextId.fetch_add(1, std::memory_order_relaxed);
    }

    static uint64_t EpochMs()
    {
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    }

private:
    const char* mPluginApi;
    const char* mCalledPluginApi;
    const uint64_t mId;
};

} // namespace PerfMetrics
} // namespace Plugin
} // namespace WPEFramework

#define RDKAPPMANAGERS_PERF_SCOPE(pluginApiName) \
    WPEFramework::Plugin::PerfMetrics::Scope _rdkPerfScope_##__LINE__(pluginApiName, pluginApiName)

#define RDKAPPMANAGERS_PERF_CALL(callerPluginApi, calledPluginApi) \
    WPEFramework::Plugin::PerfMetrics::Scope _rdkPerfCall_##__LINE__(callerPluginApi, calledPluginApi)

#else

#define RDKAPPMANAGERS_PERF_SCOPE(pluginApiName)
#define RDKAPPMANAGERS_PERF_CALL(callerPluginApi, calledPluginApi)

#endif
