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

#include <interfaces/IAppManager.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>
#include <thread>

namespace WPEFramework {
namespace Plugin {

class AppManagerImplementation;

class AppStateTransitionManager {
public:
    explicit AppStateTransitionManager(AppManagerImplementation& parent);
    ~AppStateTransitionManager();

    AppStateTransitionManager(const AppStateTransitionManager&) = delete;
    AppStateTransitionManager& operator=(const AppStateTransitionManager&) = delete;

    void Configure(uint32_t pausedToSuspendedSeconds, uint32_t suspendedToHibernatedSeconds);
    void Start();
    void Stop();
    void OnStateChanged(const std::string& appId, Exchange::IAppManager::AppLifecycleState state);
    void Remove(const std::string& appId);

private:
    struct Entry {
        Exchange::IAppManager::AppLifecycleState state;
        std::chrono::steady_clock::time_point deadline;
    };

    void Worker();
    void Process(const std::string& appId, Exchange::IAppManager::AppLifecycleState state);
    void ExpirePausedAppLocked(const std::string& exceptAppId);

    AppManagerImplementation& mParent;
    std::atomic<bool> mRunning;
    std::thread mWorker;
    std::mutex mLock;
    std::condition_variable mCondition;
    std::map<std::string, Entry> mEntries;
    std::string mPausedAppId;
    std::chrono::seconds mPausedToSuspendedTimeout;
    std::chrono::seconds mSuspendedToHibernatedTimeout;
};

} // namespace Plugin
} // namespace WPEFramework
