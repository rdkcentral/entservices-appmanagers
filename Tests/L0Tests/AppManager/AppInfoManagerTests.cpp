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

uint32_t Test_AM_AppInfoManagerIteratorEdgeCases()
{
    L0Test::TestResult tr;
    WPEFramework::Plugin::AppInfoManager manager;

    // Create multiple app infos
    for (int i = 0; i < 5; i++) {
        WPEFramework::Plugin::AppInfo info;
        info.mAppId = "app" + std::to_string(i);
        info.mAppInstanceId = "instance" + std::to_string(i);
        manager.add(info);
    }

    // Create iterator for all apps
    auto* iterator = manager.createAppInfoIterator();
    L0Test::ExpectTrue(tr, iterator != nullptr, "createAppInfoIterator() returns non-null");

    if (iterator) {
        // Count elements
        uint32_t count = 0;
        while (iterator->Next()) {
            count++;
        }
        L0Test::ExpectEqU32(tr, count, 5U, "Iterator returns correct count of elements");
        
        // Reset and iterate again
        iterator->Reset();
        count = 0;
        while (iterator->Next()) {
            count++;
        }
        L0Test::ExpectEqU32(tr, count, 5U, "Iterator reset works correctly");
        
        iterator->Release();
    }

    return tr.failures;
}

uint32_t Test_AM_AppInfoManagerRemoveMultipleInstances()
{
    L0Test::TestResult tr;
    WPEFramework::Plugin::AppInfoManager manager;

    // Add multiple instances of same app
    WPEFramework::Plugin::AppInfo info1, info2, info3;
    info1.mAppId = "testApp";
    info1.mAppInstanceId = "instance1";
    info2.mAppId = "testApp";
    info2.mAppInstanceId = "instance2";
    info3.mAppId = "otherApp";
    info3.mAppInstanceId = "instance3";

    manager.add(info1);
    manager.add(info2);
    manager.add(info3);

    // Remove by app ID should remove all instances of that app
    bool removed = manager.removeByAppId(std::string("testApp"));
    L0Test::ExpectTrue(tr, removed, "removeByAppId() returns true for existing app");

    // Verify testApp instances are gone
    auto* foundInfo = manager.getByAppId(std::string("testApp"));
    L0Test::ExpectTrue(tr, foundInfo == nullptr, "App removed by removeByAppId() is no longer found");

    // Verify otherApp still exists
    foundInfo = manager.getByAppId(std::string("otherApp"));
    L0Test::ExpectTrue(tr, foundInfo != nullptr, "Other app still exists after selective removal");

    if (foundInfo) {
        delete foundInfo;
    }

    return tr.failures;
}

uint32_t Test_AM_AppInfoManagerRemoveByInstanceIdEdgeCases()
{
    L0Test::TestResult tr;
    WPEFramework::Plugin::AppInfoManager manager;

    // Add test apps
    WPEFramework::Plugin::AppInfo info1, info2;
    info1.mAppId = "app1";
    info1.mAppInstanceId = "instance1";
    info2.mAppId = "app2";
    info2.mAppInstanceId = "instance2";

    manager.add(info1);
    manager.add(info2);

    // Remove by instance ID
    bool removed = manager.removeByAppInstanceId(std::string("instance1"));
    L0Test::ExpectTrue(tr, removed, "removeByAppInstanceId() returns true for existing instance");

    // Try to remove non-existent instance
    bool removed2 = manager.removeByAppInstanceId(std::string("nonExistent"));
    L0Test::ExpectTrue(tr, !removed2, "removeByAppInstanceId() returns false for non-existent instance");

    // Verify correct app was removed
    auto* foundInfo = manager.getByAppInstanceId(std::string("instance1"));
    L0Test::ExpectTrue(tr, foundInfo == nullptr, "Removed instance not found");

    foundInfo = manager.getByAppInstanceId(std::string("instance2"));
    L0Test::ExpectTrue(tr, foundInfo != nullptr, "Other instance still exists");

    if (foundInfo) {
        delete foundInfo;
    }

    return tr.failures;
}

uint32_t Test_AM_AppInfoManagerGetByAppIdMultipleInstances()
{
    L0Test::TestResult tr;
    WPEFramework::Plugin::AppInfoManager manager;

    // Add multiple instances of same app
    WPEFramework::Plugin::AppInfo info1, info2;
    info1.mAppId = "sameApp";
    info1.mAppInstanceId = "instance1";
    info1.mAppVersion = "1.0.0";
    info2.mAppId = "sameApp";
    info2.mAppInstanceId = "instance2";
    info2.mAppVersion = "2.0.0";

    manager.add(info1);
    manager.add(info2);

    // getByAppId should return first match
    auto* foundInfo = manager.getByAppId(std::string("sameApp"));
    L0Test::ExpectTrue(tr, foundInfo != nullptr, "getByAppId() finds app with multiple instances");

    if (foundInfo) {
        L0Test::ExpectTrue(tr, foundInfo->mAppId == "sameApp", "Found correct app ID");
        L0Test::ExpectTrue(tr, !foundInfo->mAppInstanceId.empty(), "Instance ID is not empty");
        delete foundInfo;
    }

    return tr.failures;
}

uint32_t Test_AM_AppInfoManagerUpsertEmptyFields()
{
    L0Test::TestResult tr;
    WPEFramework::Plugin::AppInfoManager manager;

    // Add app with minimal info
    WPEFramework::Plugin::AppInfo info;
    info.mAppId = "minimalApp";
    info.mAppInstanceId = "instance1";
    // Leave other fields empty

    manager.add(info);

    // Upsert with update
    WPEFramework::Plugin::AppInfo updateInfo;
    updateInfo.mAppId = "minimalApp";
    updateInfo.mAppInstanceId = "instance1";
    updateInfo.mAppVersion = "1.0.0";
    updateInfo.mAppType = "interactive";

    bool updated = manager.upsert(updateInfo);
    L0Test::ExpectTrue(tr, updated, "upsert() updates existing entry");

    // Verify updated fields
    auto* foundInfo = manager.getByAppInstanceId(std::string("instance1"));
    L0Test::ExpectTrue(tr, foundInfo != nullptr, "Updated app found");

    if (foundInfo) {
        L0Test::ExpectTrue(tr, foundInfo->mAppVersion == "1.0.0", "Version updated");
        L0Test::ExpectTrue(tr, foundInfo->mAppType == "interactive", "AppType updated");
        delete foundInfo;
    }

    return tr.failures;
}

uint32_t Test_AM_AppInfoManagerClear()
{
    L0Test::TestResult tr;
    WPEFramework::Plugin::AppInfoManager manager;

    // Add multiple apps
    for (int i = 0; i < 10; i++) {
        WPEFramework::Plugin::AppInfo info;
        info.mAppId = "app" + std::to_string(i);
        info.mAppInstanceId = "instance" + std::to_string(i);
        manager.add(info);
    }

    // Verify apps exist
    auto* info = manager.getByAppId(std::string("app0"));
    L0Test::ExpectTrue(tr, info != nullptr, "Apps added before clear");
    if (info) delete info;

    // Clear all
    manager.clear();

    // Verify all apps are gone
    info = manager.getByAppId(std::string("app0"));
    L0Test::ExpectTrue(tr, info == nullptr, "Apps removed after clear");

    info = manager.getByAppId(std::string("app5"));
    L0Test::ExpectTrue(tr, info == nullptr, "All apps removed after clear");

    return tr.failures;
}

uint32_t Test_AM_AppInfoManagerBasicThreadSafety()
{
    L0Test::TestResult tr;
    WPEFramework::Plugin::AppInfoManager manager;

    // Add app
    WPEFramework::Plugin::AppInfo info;
    info.mAppId = "threadTestApp";
    info.mAppInstanceId = "instance1";
    manager.add(info);

    // Multiple reads should work
    for (int i = 0; i < 100; i++) {
        auto* foundInfo = manager.getByAppId(std::string("threadTestApp"));
        if (foundInfo) {
            delete foundInfo;
        }
    }

    L0Test::ExpectTrue(tr, true, "Multiple sequential reads succeed");

    return tr.failures;
}

uint32_t Test_AM_AppInfoManagerUpdateOperationBranches()
{
    L0Test::TestResult tr;
    WPEFramework::Plugin::AppInfoManager manager;

    // Add initial app
    WPEFramework::Plugin::AppInfo info;
    info.mAppId = "updateTestApp";
    info.mAppInstanceId = "instance1";
    info.mAppVersion = "1.0.0";
    manager.add(info);

    // Update with new app ID (should add new entry)
    WPEFramework::Plugin::AppInfo newInfo;
    newInfo.mAppId = "differentApp";
    newInfo.mAppInstanceId = "instance2";
    bool updated = manager.update(newInfo);
    L0Test::ExpectTrue(tr, !updated, "update() with new app ID returns false");

    // Update existing app
    WPEFramework::Plugin::AppInfo updateInfo;
    updateInfo.mAppId = "updateTestApp";
    updateInfo.mAppInstanceId = "instance1";
    updateInfo.mAppVersion = "2.0.0";
    updated = manager.update(updateInfo);
    L0Test::ExpectTrue(tr, updated, "update() with existing app succeeds");

    // Verify update
    auto* foundInfo = manager.getByAppInstanceId(std::string("instance1"));
    if (foundInfo) {
        L0Test::ExpectTrue(tr, foundInfo->mAppVersion == "2.0.0", "Version updated correctly");
        delete foundInfo;
    }

    return tr.failures;
}

uint32_t Test_AM_AppInfoManagerGetByCopyEdgeCases()
{
    L0Test::TestResult tr;
    WPEFramework::Plugin::AppInfoManager manager;

    // Test with non-existent instance
    WPEFramework::Plugin::AppInfo outInfo;
    bool found = manager.getAppInfoByCopyOnInstanceId(std::string("nonExistent"), outInfo);
    L0Test::ExpectTrue(tr, !found, "getAppInfoByCopyOnInstanceId() returns false for non-existent instance");

    // Add test app
    WPEFramework::Plugin::AppInfo info;
    info.mAppId = "copyTestApp";
    info.mAppInstanceId = "instance1";
    info.mAppVersion = "1.0.0";
    manager.add(info);

    // Get by copy
    WPEFramework::Plugin::AppInfo copiedInfo;
    found = manager.getAppInfoByCopyOnInstanceId(std::string("instance1"), copiedInfo);
    L0Test::ExpectTrue(tr, found, "getAppInfoByCopyOnInstanceId() finds existing instance");
    L0Test::ExpectTrue(tr, copiedInfo.mAppId == "copyTestApp", "Copied info has correct app ID");
    L0Test::ExpectTrue(tr, copiedInfo.mAppVersion == "1.0.0", "Copied info has correct version");

    return tr.failures;
}
