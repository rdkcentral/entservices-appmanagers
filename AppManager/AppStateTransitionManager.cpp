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

#include "AppStateTransitionManager.h"
#include "AppManagerImplementation.h"
#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

AppStateTransitionManager::AppStateTransitionManager(AppManagerImplementation& parent)
    : mParent(parent)
    , mRunning(false)
    , mPausedToSuspendedTimeout(60)
    , mSuspendedToHibernatedTimeout(300)
    , mHibernationEnabled(true)
{
}

AppStateTransitionManager::~AppStateTransitionManager()
{
    Stop();
}

void AppStateTransitionManager::Configure(uint32_t pausedToSuspendedSeconds, uint32_t suspendedToHibernatedSeconds, bool hibernationEnabled)
{
    std::lock_guard<std::mutex> lock(mLock);
    mPausedToSuspendedTimeout = std::chrono::seconds(pausedToSuspendedSeconds);
    mSuspendedToHibernatedTimeout = std::chrono::seconds(suspendedToHibernatedSeconds);
    mHibernationEnabled = hibernationEnabled;
}

void AppStateTransitionManager::Start()
{
    if (!mRunning.exchange(true)) {
        mWorker = std::thread(&AppStateTransitionManager::Worker, this);
    }
}

void AppStateTransitionManager::Stop()
{
    if (mRunning.exchange(false)) {
        mCondition.notify_all();
        if (mWorker.joinable()) {
            mWorker.join();
        }
    }
}

void AppStateTransitionManager::ExpirePausedAppLocked(const std::string& exceptAppId)
{
    if (!mPausedAppId.empty() && mPausedAppId != exceptAppId) {
        std::map<std::string, Entry>::iterator entry = mEntries.find(mPausedAppId);
        if (entry != mEntries.end()) {
            entry->second.deadline = std::chrono::steady_clock::now();
        }
    }
}

void AppStateTransitionManager::OnStateChanged(const std::string& appId, Exchange::IAppManager::AppLifecycleState state)
{
    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(mLock);
        if (state == Exchange::IAppManager::APP_STATE_PAUSED) {
            ExpirePausedAppLocked(appId);
            mPausedAppId = appId;
            mEntries[appId] = Entry{state, now + mPausedToSuspendedTimeout};
        } else if (state == Exchange::IAppManager::APP_STATE_SUSPENDED) {
            if (mPausedAppId == appId) {
                mPausedAppId.clear();
            }
            mEntries[appId] = Entry{state, now + mSuspendedToHibernatedTimeout};
        } else if (state == Exchange::IAppManager::APP_STATE_ACTIVE) {
            mEntries.erase(appId);
            ExpirePausedAppLocked(appId);
        } else {
            mEntries.erase(appId);
            if (mPausedAppId == appId) {
                mPausedAppId.clear();
            }
        }
    }
    mCondition.notify_all();
}

void AppStateTransitionManager::Remove(const std::string& appId)
{
    std::lock_guard<std::mutex> lock(mLock);
    mEntries.erase(appId);
    if (mPausedAppId == appId) {
        mPausedAppId.clear();
    }
}

void AppStateTransitionManager::Worker()
{
    while (mRunning.load()) {
        std::string appId;
        Exchange::IAppManager::AppLifecycleState state = Exchange::IAppManager::APP_STATE_UNKNOWN;
        {
            std::unique_lock<std::mutex> lock(mLock);
            if (mEntries.empty()) {
                mCondition.wait(lock, [this] { return !mRunning.load() || !mEntries.empty(); });
            } else {
                std::map<std::string, Entry>::iterator next = mEntries.begin();
                for (std::map<std::string, Entry>::iterator it = mEntries.begin(); it != mEntries.end(); ++it) {
                    if (it->second.deadline < next->second.deadline) {
                        next = it;
                    }
                }
                mCondition.wait_until(lock, next->second.deadline);
            }

            if (!mRunning.load()) {
                break;
            }

            const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
            for (std::map<std::string, Entry>::iterator it = mEntries.begin(); it != mEntries.end(); ++it) {
                if (it->second.deadline <= now) {
                    appId = it->first;
                    state = it->second.state;
                    if (mPausedAppId == appId) {
                        mPausedAppId.clear();
                    }
                    mEntries.erase(it);
                    break;
                }
            }
        }

        if (!appId.empty()) {
            Process(appId, state);
        }
    }
}

void AppStateTransitionManager::Process(const std::string& appId, Exchange::IAppManager::AppLifecycleState state)
{
    if (state == Exchange::IAppManager::APP_STATE_PAUSED) {
        if (mParent.SetInactiveTargetState(appId, Exchange::ILifecycleManager::SUSPENDED) != Core::ERROR_NONE) {
            LOGWARN("Unable to suspend appId %s; terminating it", appId.c_str());
            mParent.TerminateApp(appId);
        }
    } else if (state == Exchange::IAppManager::APP_STATE_SUSPENDED) {
        bool targetRamAchieved = false;
        if (!mParent.ReconcileAndWait(appId, mParent.GetAppRamTargetMB(appId), false, targetRamAchieved)) {
            LOGWARN("Resource check unavailable for suspended appId %s; leaving it suspended", appId.c_str());
            OnStateChanged(appId, Exchange::IAppManager::APP_STATE_SUSPENDED);
        } else if (!targetRamAchieved) {
            LOGWARN("Resource target not achieved for suspended appId %s; terminating it", appId.c_str());
            mParent.TerminateApp(appId);
        } else if (!mHibernationEnabled || !mParent.SupportsHibernation(appId) || !mParent.HasHibernationFlashSpace(appId)) {
            LOGINFO("AppId %s remains suspended because hibernation is unsupported or flash space is unavailable", appId.c_str());
            OnStateChanged(appId, Exchange::IAppManager::APP_STATE_SUSPENDED);
        } else if (mParent.SetInactiveTargetState(appId, Exchange::ILifecycleManager::HIBERNATED) != Core::ERROR_NONE) {
            LOGINFO("AppId %s remains suspended because hibernation request failed", appId.c_str());
            OnStateChanged(appId, Exchange::IAppManager::APP_STATE_SUSPENDED);
        }
    }
}

} // namespace Plugin
} // namespace WPEFramework
