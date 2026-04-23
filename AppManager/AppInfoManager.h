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

#pragma once

#include "AppInfo.h"
#include <core/core.h>
#include <map>
#include <string>
#include <functional>

namespace WPEFramework {
namespace Plugin {

/**
 * @brief Thread-safe singleton registry for all loaded AppInfo entries.
 *
 * Ownership of the map and its lock lives here, removing the need for callers
 * to hold an external lock before touching individual entries.
 *
 * Usage patterns
 * --------------
 * Single-field read:
 *   std::string id = AppInfoManager::getInstance().getAppInstanceId(appId);
 *
 * Multi-field atomic read:
 *   AppInfo snap;
 *   if (AppInfoManager::getInstance().get(appId, snap)) { ... }
 *
 * Multi-field update (insert-or-update):
 *   AppInfoManager::getInstance().upsert(appId, [&](AppInfo& a) {
 *       a.setAppInstanceId(instanceId);
 *       a.setTargetAppState(state);
 *   });
 *
 * Multi-field update (existing entry only):
 *   AppInfoManager::getInstance().update(appId, [&](AppInfo& a) {
 *       a.setCurrentAction(action);
 *   });
 *
 * NOTE: update() and upsert() use a copy-update-swap strategy.
 * The updater callback is invoked WITHOUT the internal lock held, so
 * it is safe to call AppInfoManager methods or any other blocking work
 * from inside the callback without risking a deadlock.
 * Because the lock is released between the copy and the write-back,
 * two concurrent calls for the same appId may each overwrite the
 * other's changes (last-writer-wins).  When strict atomicity is needed
 * for a single field, use the dedicated convenience setters instead,
 * which acquire the lock only once and are fully atomic.
 */
class AppInfoManager
{
public:
    static AppInfoManager& getInstance();

    AppInfoManager(const AppInfoManager&)            = delete;
    AppInfoManager& operator=(const AppInfoManager&) = delete;

    /* ---- Existence ---- */
    bool exists(const std::string& appId) const;

    /* ---- Bulk atomic read (returns a by-value copy) ---- */
    bool get(const std::string& appId, AppInfo& outInfo) const;

    /* ---- Multi-field update (returns false if appId absent or concurrently removed).
     *      Callback is invoked outside the lock (copy-update-swap); see class note. ---- */
    bool update(const std::string& appId, std::function<void(AppInfo&)> updater);

    /* ---- Insert-or-update.
     *      Callback is invoked outside the lock (copy-update-swap); see class note. ---- */
    void upsert(const std::string& appId, std::function<void(AppInfo&)> updater);

    /* ---- Remove ---- */
    void remove(const std::string& appId);

    /* ---- Clear all entries ---- */
    void clear();

    /* ---- Convenience field getters (return default if appId absent) ---- */
    std::string                                  getAppInstanceId(const std::string& appId)   const;
    std::string                                  getActiveSessionId(const std::string& appId) const;
    AppManagerTypes::PackageInfo                 getPackageInfo(const std::string& appId)     const;
    Exchange::IAppManager::AppLifecycleState     getAppNewState(const std::string& appId)     const;
    Exchange::IAppManager::AppLifecycleState     getTargetAppState(const std::string& appId)  const;
    Exchange::IAppManager::AppLifecycleState     getAppOldState(const std::string& appId)     const;
    Exchange::ILifecycleManager::LifecycleState  getAppLifecycleState(const std::string& appId) const;
    timespec                                     getLastActiveStateChangeTime(const std::string& appId) const;
    uint32_t                                     getLastActiveIndex(const std::string& appId) const;
    std::string                                  getAppIntent(const std::string& appId)       const;
    AppManagerTypes::CurrentAction               getCurrentAction(const std::string& appId)   const;
    std::string                                  getPackageInfoVersion(const std::string& appId) const;
    AppManagerTypes::ApplicationType             getPackageInfoType(const std::string& appId)    const;
    time_t getCurrentActionTime(const std::string& appId) const;

    /* ---- Convenience field setters (upserts the entry if absent) ---- */
    void setAppInstanceId(const std::string& appId,   const std::string& id);
    void setActiveSessionId(const std::string& appId, const std::string& id);
    void setPackageInfo(const std::string& appId,     const AppManagerTypes::PackageInfo& info);
    void setAppNewState(const std::string& appId,     Exchange::IAppManager::AppLifecycleState state);
    void setTargetAppState(const std::string& appId,  Exchange::IAppManager::AppLifecycleState state);
    void setAppOldState(const std::string& appId,     Exchange::IAppManager::AppLifecycleState state);
    void setAppLifecycleState(const std::string& appId, Exchange::ILifecycleManager::LifecycleState state);
    void setLastActiveStateChangeTime(const std::string& appId, const timespec& ts);
    void setLastActiveIndex(const std::string& appId, uint32_t index);
    void setAppIntent(const std::string& appId,       const std::string& intent);
    void setCurrentAction(const std::string& appId,   AppManagerTypes::CurrentAction action);
    void setPackageInfoVersion(const std::string& appId,      const std::string& version);
    void setPackageInfoLockId(const std::string& appId,       uint32_t lockId);
    void setPackageInfoUnpackedPath(const std::string& appId, const std::string& path);
    void setPackageInfoConfigMetadata(const std::string& appId, const Exchange::RuntimeConfig& cfg);
    void setPackageInfoAppMetadata(const std::string& appId,  const std::string& metadata);
    void setPackageInfoType(const std::string& appId,         AppManagerTypes::ApplicationType type);
    void setCurrentActionTime(const std::string& appId, time_t t);

private:
    AppInfoManager()  = default;
    ~AppInfoManager() = default;

    mutable Core::CriticalSection     mLock;
    std::map<std::string, AppInfo>    mMap;
};

} // namespace Plugin
} // namespace WPEFramework
