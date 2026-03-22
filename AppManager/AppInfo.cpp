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

#include "AppInfo.h"

namespace WPEFramework {
namespace Plugin {

AppInfo::AppInfo()
    : mAppInstanceId("")
    , mActiveSessionId("")
    , mPackageInfo()
    , mAppNewState(Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED)
    , mTargetAppState(Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED)
    , mAppOldState(Exchange::IAppManager::AppLifecycleState::APP_STATE_UNLOADED)
    , mAppLifecycleState(Exchange::ILifecycleManager::LifecycleState::UNLOADED)
    , mLastActiveStateChangeTime({0, 0})
    , mLastActiveIndex(0)
    , mAppIntent("")
    , mCurrentAction(AppManagerTypes::APP_ACTION_NONE)
#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
    , mCurrentActionTime(0)
#endif
{}

/* ----- Getters ----- */

const std::string& AppInfo::getAppInstanceId()   const { return mAppInstanceId; }
const std::string& AppInfo::getActiveSessionId() const { return mActiveSessionId; }

const AppManagerTypes::PackageInfo& AppInfo::getPackageInfo()        const { return mPackageInfo; }
AppManagerTypes::PackageInfo&       AppInfo::getPackageInfoMutable()       { return mPackageInfo; }

Exchange::IAppManager::AppLifecycleState    AppInfo::getAppNewState()       const { return mAppNewState; }
Exchange::IAppManager::AppLifecycleState    AppInfo::getTargetAppState()    const { return mTargetAppState; }
Exchange::IAppManager::AppLifecycleState    AppInfo::getAppOldState()       const { return mAppOldState; }
Exchange::ILifecycleManager::LifecycleState AppInfo::getAppLifecycleState() const { return mAppLifecycleState; }

const timespec& AppInfo::getLastActiveStateChangeTime() const { return mLastActiveStateChangeTime; }
uint32_t        AppInfo::getLastActiveIndex()            const { return mLastActiveIndex; }
const std::string& AppInfo::getAppIntent()               const { return mAppIntent; }

AppManagerTypes::CurrentAction AppInfo::getCurrentAction() const { return mCurrentAction; }

#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
time_t AppInfo::getCurrentActionTime() const { return mCurrentActionTime; }
#endif

/* ----- Setters ----- */

void AppInfo::setAppInstanceId(const std::string& id)   { mAppInstanceId   = id; }
void AppInfo::setActiveSessionId(const std::string& id) { mActiveSessionId = id; }

void AppInfo::setPackageInfo(const AppManagerTypes::PackageInfo& info) { mPackageInfo = info; }

void AppInfo::setAppNewState(Exchange::IAppManager::AppLifecycleState state)    { mAppNewState    = state; }
void AppInfo::setTargetAppState(Exchange::IAppManager::AppLifecycleState state) { mTargetAppState = state; }
void AppInfo::setAppOldState(Exchange::IAppManager::AppLifecycleState state)    { mAppOldState    = state; }
void AppInfo::setAppLifecycleState(Exchange::ILifecycleManager::LifecycleState state) { mAppLifecycleState = state; }

void AppInfo::setLastActiveStateChangeTime(const timespec& ts) { mLastActiveStateChangeTime = ts; }
void AppInfo::setLastActiveIndex(uint32_t index)               { mLastActiveIndex            = index; }
void AppInfo::setAppIntent(const std::string& intent)          { mAppIntent                 = intent; }

void AppInfo::setCurrentAction(AppManagerTypes::CurrentAction action) { mCurrentAction = action; }

#ifdef ENABLE_AIMANAGERS_TELEMETRY_METRICS
void AppInfo::setCurrentActionTime(time_t t) { mCurrentActionTime = t; }
#endif

} // namespace Plugin
} // namespace WPEFramework
