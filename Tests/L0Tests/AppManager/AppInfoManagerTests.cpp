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

#include "AppInfoManager.h"
#include "common/L0Expect.hpp"

uint32_t Test_AM_AppInfoManagerCrudAndConvenienceAccessors()
{
    L0Test::TestResult tr;

    auto& mgr = WPEFramework::Plugin::AppInfoManager::getInstance();
    mgr.clear();

    L0Test::ExpectTrue(tr, !mgr.exists("missing"), "Missing entry is not present");
    WPEFramework::Plugin::AppInfo missingSnapshot;
    L0Test::ExpectTrue(tr, !mgr.get("missing", missingSnapshot), "get() on a missing entry returns false");

    mgr.setAppInstanceId("app1", "instance-1");
    mgr.setActiveSessionId("app1", "session-1");
    mgr.setAppIntent("app1", "intent-1");
    mgr.setCurrentAction("app1", WPEFramework::Plugin::AppManagerTypes::APP_ACTION_CLOSE);
    mgr.setCurrentActionTime("app1", 77);
    mgr.setLastActiveIndex("app1", 12);
    timespec ts { 123, 456 };
    mgr.setLastActiveStateChangeTime("app1", ts);
    mgr.setPackageInfoVersion("app1", "9.8.7");
    mgr.setPackageInfoType("app1", WPEFramework::Plugin::AppManagerTypes::APPLICATION_TYPE_SYSTEM);

    L0Test::ExpectTrue(tr, mgr.exists("app1"), "Entry exists after setter calls");
    L0Test::ExpectEqStr(tr, mgr.getAppInstanceId("app1"), std::string("instance-1"), "Instance id getter returns stored value");
    L0Test::ExpectEqStr(tr, mgr.getActiveSessionId("app1"), std::string("session-1"), "Session id getter returns stored value");
    L0Test::ExpectEqStr(tr, mgr.getAppIntent("app1"), std::string("intent-1"), "Intent getter returns stored value");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(mgr.getCurrentAction("app1")), static_cast<uint32_t>(WPEFramework::Plugin::AppManagerTypes::APP_ACTION_CLOSE), "Current action getter returns stored value");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(mgr.getCurrentActionTime("app1")), 77U, "Current action time getter returns stored value");
    L0Test::ExpectEqU32(tr, mgr.getLastActiveIndex("app1"), 12U, "Last active index getter returns stored value");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(mgr.getLastActiveStateChangeTime("app1").tv_sec), 123U, "Timestamp seconds getter returns stored value");
    L0Test::ExpectEqStr(tr, mgr.getPackageInfoVersion("app1"), std::string("9.8.7"), "Package version getter returns stored value");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(mgr.getPackageInfoType("app1")), static_cast<uint32_t>(WPEFramework::Plugin::AppManagerTypes::APPLICATION_TYPE_SYSTEM), "Package type getter returns stored value");

    WPEFramework::Plugin::AppInfo snapshot;
    L0Test::ExpectTrue(tr, mgr.get("app1", snapshot), "get() returns a snapshot for an existing entry");
    L0Test::ExpectEqStr(tr, snapshot.getAppInstanceId(), std::string("instance-1"), "Snapshot contains the stored instance id");

    const bool updated = mgr.update("app1", [&](WPEFramework::Plugin::AppInfo& a) {
        a.setAppInstanceId("instance-2");
        a.setCurrentAction(WPEFramework::Plugin::AppManagerTypes::APP_ACTION_RESUME);
    });
    L0Test::ExpectTrue(tr, updated, "update() succeeds for an existing entry");
    L0Test::ExpectEqStr(tr, mgr.getAppInstanceId("app1"), std::string("instance-2"), "update() changes the stored instance id");

    mgr.remove("app1");
    L0Test::ExpectTrue(tr, !mgr.exists("app1"), "remove() deletes the entry");

    mgr.clear();
    return tr.failures;
}

uint32_t Test_AM_AppInfoManagerUpsertUpdateBranch()
{
    L0Test::TestResult tr;

    auto& mgr = WPEFramework::Plugin::AppInfoManager::getInstance();
    mgr.clear();

    // First upsert — creates a new entry.
    mgr.upsert("app.upsert", [](WPEFramework::Plugin::AppInfo& a) {
        a.setAppInstanceId("instance-first");
    });
    L0Test::ExpectTrue(tr, mgr.exists("app.upsert"), "upsert() creates entry on first call");
    L0Test::ExpectEqStr(tr, mgr.getAppInstanceId("app.upsert"), std::string("instance-first"),
        "upsert() stores the initial instance id");

    // Second upsert on same key — hits the update-existing branch (copy = it->second).
    mgr.upsert("app.upsert", [](WPEFramework::Plugin::AppInfo& a) {
        a.setAppInstanceId("instance-updated");
    });
    L0Test::ExpectEqStr(tr, mgr.getAppInstanceId("app.upsert"), std::string("instance-updated"),
        "upsert() updates the existing entry on second call");

    mgr.clear();
    return tr.failures;
}
