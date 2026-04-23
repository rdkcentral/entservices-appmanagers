/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2024 RDK Management
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
// Spec: app-manager

#include "AppInfoManager.h"

namespace WPEFramework {
namespace Plugin {

/* ---- Singleton accessor ---- */

AppInfoManager& AppInfoManager::getInstance()
{
    static AppInfoManager sInstance;
    return sInstance;
}

/* ---- Existence ---- */

bool AppInfoManager::exists(const std::string& appId) const
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    return mMap.count(appId) > 0;
}

/* ---- Bulk atomic read ---- */

bool AppInfoManager::get(const std::string& appId, AppInfo& outInfo) const
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    auto it = mMap.find(appId);
    if (it == mMap.end())
        return false;
    outInfo = it->second;   // value copy while lock is held
    return true;
}

/* ---- Multi-field update (existing entries only) ---- */

bool AppInfoManager::update(const std::string& appId, std::function<void(AppInfo&)> updater)
{
    /* Copy-update-swap: copy the entry under the lock, run the callback
     * outside the lock (avoiding potential deadlocks), then write back.
     * Returns false if the entry does not exist at the time of either the
     * initial lookup or the write-back (e.g. concurrently removed). */
    AppInfo copy;
    {
        Core::SafeSyncType<Core::CriticalSection> lock(mLock);
        auto it = mMap.find(appId);
        if (it == mMap.end())
            return false;
        copy = it->second;
    }

    updater(copy);

    {
        Core::SafeSyncType<Core::CriticalSection> lock(mLock);
        auto it = mMap.find(appId);
        if (it == mMap.end())
            return false;   // entry removed between copy and write-back
        it->second = std::move(copy);
    }
    return true;
}

/* ---- Insert-or-update ---- */

void AppInfoManager::upsert(const std::string& appId, std::function<void(AppInfo&)> updater)
{
    /* Copy-update-swap: copy (or default-construct) the entry under the lock,
     * run the callback outside the lock, then write back. */
    AppInfo copy;
    {
        Core::SafeSyncType<Core::CriticalSection> lock(mLock);
        auto it = mMap.find(appId);
        if (it != mMap.end())
            copy = it->second;
        // else: copy remains default-constructed (same as operator[] default)
    }

    updater(copy);

    {
        Core::SafeSyncType<Core::CriticalSection> lock(mLock);
        mMap[appId] = std::move(copy);
    }
}

/* ---- Remove ---- */

void AppInfoManager::remove(const std::string& appId)
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap.erase(appId);
}

/* ---- Clear all ---- */

void AppInfoManager::clear()
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap.clear();
}

/* ---- Convenience field getters ---- */

std::string AppInfoManager::getAppInstanceId(const std::string& appId) const
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    auto it = mMap.find(appId);
    return (it != mMap.end()) ? it->second.getAppInstanceId() : std::string{};
}

std::string AppInfoManager::getActiveSessionId(const std::string& appId) const
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    auto it = mMap.find(appId);
    return (it != mMap.end()) ? it->second.getActiveSessionId() : std::string{};
}

AppManagerTypes::PackageInfo AppInfoManager::getPackageInfo(const std::string& appId) const
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    auto it = mMap.find(appId);
    return (it != mMap.end()) ? it->second.getPackageInfo() : AppManagerTypes::PackageInfo{};
}

Exchange::IAppManager::AppLifecycleState AppInfoManager::getAppNewState(const std::string& appId) const
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    auto it = mMap.find(appId);
    return (it != mMap.end()) ? it->second.getAppNewState()
                               : Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED;
}

Exchange::IAppManager::AppLifecycleState AppInfoManager::getTargetAppState(const std::string& appId) const
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    auto it = mMap.find(appId);
    return (it != mMap.end()) ? it->second.getTargetAppState()
                               : Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED;
}

Exchange::IAppManager::AppLifecycleState AppInfoManager::getAppOldState(const std::string& appId) const
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    auto it = mMap.find(appId);
    return (it != mMap.end()) ? it->second.getAppOldState()
                               : Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED;
}

Exchange::ILifecycleManager::LifecycleState AppInfoManager::getAppLifecycleState(const std::string& appId) const
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    auto it = mMap.find(appId);
    return (it != mMap.end()) ? it->second.getAppLifecycleState()
                               : Exchange::ILifecycleManager::LifecycleState::UNLOADED;
}

timespec AppInfoManager::getLastActiveStateChangeTime(const std::string& appId) const
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    auto it = mMap.find(appId);
    return (it != mMap.end()) ? it->second.getLastActiveStateChangeTime() : timespec{0, 0};
}

uint32_t AppInfoManager::getLastActiveIndex(const std::string& appId) const
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    auto it = mMap.find(appId);
    return (it != mMap.end()) ? it->second.getLastActiveIndex() : 0u;
}

std::string AppInfoManager::getAppIntent(const std::string& appId) const
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    auto it = mMap.find(appId);
    return (it != mMap.end()) ? it->second.getAppIntent() : std::string{};
}

AppManagerTypes::CurrentAction AppInfoManager::getCurrentAction(const std::string& appId) const
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    auto it = mMap.find(appId);
    return (it != mMap.end()) ? it->second.getCurrentAction() : AppManagerTypes::APP_ACTION_NONE;
}

std::string AppInfoManager::getPackageInfoVersion(const std::string& appId) const
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    auto it = mMap.find(appId);
    return (it != mMap.end()) ? it->second.getPackageInfo().version : std::string{};
}

AppManagerTypes::ApplicationType AppInfoManager::getPackageInfoType(const std::string& appId) const
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    auto it = mMap.find(appId);
    return (it != mMap.end()) ? it->second.getPackageInfo().type
                               : AppManagerTypes::APPLICATION_TYPE_UNKNOWN;
}

time_t AppInfoManager::getCurrentActionTime(const std::string& appId) const
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    auto it = mMap.find(appId);
    return (it != mMap.end()) ? it->second.getCurrentActionTime() : time_t{0};
}

/* ---- Convenience field setters ---- */

void AppInfoManager::setAppInstanceId(const std::string& appId, const std::string& id)
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap[appId].setAppInstanceId(id);
}

void AppInfoManager::setActiveSessionId(const std::string& appId, const std::string& id)
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap[appId].setActiveSessionId(id);
}

void AppInfoManager::setPackageInfo(const std::string& appId, const AppManagerTypes::PackageInfo& info)
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap[appId].setPackageInfo(info);
}

void AppInfoManager::setAppNewState(const std::string& appId, Exchange::IAppManager::AppLifecycleState state)
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap[appId].setAppNewState(state);
}

void AppInfoManager::setTargetAppState(const std::string& appId, Exchange::IAppManager::AppLifecycleState state)
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap[appId].setTargetAppState(state);
}

void AppInfoManager::setAppOldState(const std::string& appId, Exchange::IAppManager::AppLifecycleState state)
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap[appId].setAppOldState(state);
}

void AppInfoManager::setAppLifecycleState(const std::string& appId, Exchange::ILifecycleManager::LifecycleState state)
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap[appId].setAppLifecycleState(state);
}

void AppInfoManager::setLastActiveStateChangeTime(const std::string& appId, const timespec& ts)
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap[appId].setLastActiveStateChangeTime(ts);
}

void AppInfoManager::setLastActiveIndex(const std::string& appId, uint32_t index)
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap[appId].setLastActiveIndex(index);
}

void AppInfoManager::setAppIntent(const std::string& appId, const std::string& intent)
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap[appId].setAppIntent(intent);
}

void AppInfoManager::setCurrentAction(const std::string& appId, AppManagerTypes::CurrentAction action)
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap[appId].setCurrentAction(action);
}

void AppInfoManager::setPackageInfoVersion(const std::string& appId, const std::string& version)
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap[appId].getPackageInfoMutable().version = version;
}

void AppInfoManager::setPackageInfoLockId(const std::string& appId, uint32_t lockId)
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap[appId].getPackageInfoMutable().lockId = lockId;
}

void AppInfoManager::setPackageInfoUnpackedPath(const std::string& appId, const std::string& path)
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap[appId].getPackageInfoMutable().unpackedPath = path;
}

void AppInfoManager::setPackageInfoConfigMetadata(const std::string& appId, const Exchange::RuntimeConfig& cfg)
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap[appId].getPackageInfoMutable().configMetadata = cfg;
}

void AppInfoManager::setPackageInfoAppMetadata(const std::string& appId, const std::string& metadata)
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap[appId].getPackageInfoMutable().appMetadata = metadata;
}

void AppInfoManager::setPackageInfoType(const std::string& appId, AppManagerTypes::ApplicationType type)
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap[appId].getPackageInfoMutable().type = type;
}

void AppInfoManager::setCurrentActionTime(const std::string& appId, time_t t)
{
    Core::SafeSyncType<Core::CriticalSection> lock(mLock);
    mMap[appId].setCurrentActionTime(t);
}

} // namespace Plugin
} // namespace WPEFramework
