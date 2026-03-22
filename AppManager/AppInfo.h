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

#pragma once

#include "AppManagerTypes.h"
#include <interfaces/IAppManager.h>
#include <interfaces/ILifecycleManager.h>
#include <string>
#include <time.h>

namespace WPEFramework {
namespace Plugin {

/**
 * @brief Encapsulates all runtime state for a single loaded application.
 *
 * All members are private; access is through typed getters and setters so
 * that the AppInfoManager can enforce synchronised access to the map from
 * multiple threads without exposing raw fields.
 */
class AppInfo
{
public:
    AppInfo();
    ~AppInfo() = default;

    /* ----- Getters ----- */
    const std::string& getAppInstanceId()   const;
    const std::string& getActiveSessionId() const;

    const AppManagerTypes::PackageInfo& getPackageInfo() const;
    AppManagerTypes::PackageInfo&       getPackageInfoMutable();   ///< Used for bulk in-place update inside AppInfoManager lambdas.

    Exchange::IAppManager::AppLifecycleState    getAppNewState()       const;
    Exchange::IAppManager::AppLifecycleState    getTargetAppState()    const;
    Exchange::IAppManager::AppLifecycleState    getAppOldState()       const;
    Exchange::ILifecycleManager::LifecycleState getAppLifecycleState() const;

    const timespec& getLastActiveStateChangeTime() const;
    uint32_t        getLastActiveIndex()            const;
    const std::string& getAppIntent()               const;

    AppManagerTypes::CurrentAction getCurrentAction() const;

#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    time_t getCurrentActionTime() const;
#endif

    /* ----- Setters ----- */
    void setAppInstanceId(const std::string& id);
    void setActiveSessionId(const std::string& id);

    void setPackageInfo(const AppManagerTypes::PackageInfo& info);

    void setAppNewState(Exchange::IAppManager::AppLifecycleState state);
    void setTargetAppState(Exchange::IAppManager::AppLifecycleState state);
    void setAppOldState(Exchange::IAppManager::AppLifecycleState state);
    void setAppLifecycleState(Exchange::ILifecycleManager::LifecycleState state);

    void setLastActiveStateChangeTime(const timespec& ts);
    void setLastActiveIndex(uint32_t index);
    void setAppIntent(const std::string& intent);

    void setCurrentAction(AppManagerTypes::CurrentAction action);

#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    void setCurrentActionTime(time_t t);
#endif

private:
    std::string mAppInstanceId;
    std::string mActiveSessionId;
    AppManagerTypes::PackageInfo mPackageInfo;
    Exchange::IAppManager::AppLifecycleState    mAppNewState;
    Exchange::IAppManager::AppLifecycleState    mTargetAppState;
    Exchange::IAppManager::AppLifecycleState    mAppOldState;
    Exchange::ILifecycleManager::LifecycleState mAppLifecycleState;
    timespec   mLastActiveStateChangeTime;
    uint32_t   mLastActiveIndex;
    std::string mAppIntent;
    AppManagerTypes::CurrentAction mCurrentAction;
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    time_t mCurrentActionTime;
#endif
};

} // namespace Plugin
} // namespace WPEFramework
