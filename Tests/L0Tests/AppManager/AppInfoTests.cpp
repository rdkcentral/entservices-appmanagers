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

#include <ctime>
#include <string>

#include "AppInfo.h"
#include "common/L0Expect.hpp"

uint32_t Test_AM_AppInfoDefaultsAndSetters()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AppInfo info;
    L0Test::ExpectEqStr(tr, info.getAppInstanceId(), std::string(), "Default app instance id is empty");
    L0Test::ExpectEqStr(tr, info.getActiveSessionId(), std::string(), "Default active session id is empty");
    L0Test::ExpectEqStr(tr, info.getAppIntent(), std::string(), "Default intent is empty");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(info.getCurrentAction()), static_cast<uint32_t>(WPEFramework::Plugin::AppManagerTypes::APP_ACTION_NONE), "Default action is APP_ACTION_NONE");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(info.getLastActiveIndex()), 0U, "Default last active index is zero");

    WPEFramework::Plugin::AppManagerTypes::PackageInfo pkg;
    pkg.version = "1.2.3";
    pkg.lockId = 42;
    pkg.unpackedPath = "/tmp/app";
    pkg.type = WPEFramework::Plugin::AppManagerTypes::APPLICATION_TYPE_INTERACTIVE;
    info.setAppInstanceId("instance-1");
    info.setActiveSessionId("session-1");
    info.setAppIntent("intent-1");
    info.setPackageInfo(pkg);
    info.setCurrentAction(WPEFramework::Plugin::AppManagerTypes::APP_ACTION_LAUNCH);
    info.setCurrentActionTime(1234);

    timespec ts { 10, 20 };
    info.setLastActiveStateChangeTime(ts);
    info.setLastActiveIndex(9);

    L0Test::ExpectEqStr(tr, info.getAppInstanceId(), std::string("instance-1"), "App instance id round-trips");
    L0Test::ExpectEqStr(tr, info.getActiveSessionId(), std::string("session-1"), "Active session id round-trips");
    L0Test::ExpectEqStr(tr, info.getAppIntent(), std::string("intent-1"), "Intent round-trips");
    L0Test::ExpectEqStr(tr, info.getPackageInfo().version, std::string("1.2.3"), "Package version round-trips");
    L0Test::ExpectEqU32(tr, info.getPackageInfo().lockId, 42U, "Package lock id round-trips");
    L0Test::ExpectEqStr(tr, info.getPackageInfo().unpackedPath, std::string("/tmp/app"), "Package unpacked path round-trips");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(info.getCurrentAction()), static_cast<uint32_t>(WPEFramework::Plugin::AppManagerTypes::APP_ACTION_LAUNCH), "Current action round-trips");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(info.getCurrentActionTime()), 1234U, "Current action time round-trips");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(info.getLastActiveIndex()), 9U, "Last active index round-trips");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(info.getLastActiveStateChangeTime().tv_sec), 10U, "Last active state change seconds round-trip");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(info.getLastActiveStateChangeTime().tv_nsec), 20U, "Last active state change nanoseconds round-trip");

    return tr.failures;
}
