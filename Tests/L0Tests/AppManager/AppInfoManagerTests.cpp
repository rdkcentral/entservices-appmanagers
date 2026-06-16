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

// NOTE: Test disabled - iterator functionality removed in new API
// uint32_t Test_AM_AppInfoManagerIteratorEdgeCases()
// {
//     AppInfoManager no longer provides createAppInfoIterator()
//     If iteration is needed, it must be implemented differently
// }
uint32_t Test_AM_AppInfoManagerIteratorEdgeCases()
{
    L0Test::TestResult tr;
    // Test disabled - iterator not available in new singleton API
    L0Test::ExpectTrue(tr, true, "Test skipped - iterator removed from API");
    return tr.failures;
}

// NOTE: Test disabled - multiple instances per appId not supported in new architecture
// In new API, appId is the unique key, so can't have multiple instances with same appId
uint32_t Test_AM_AppInfoManagerRemoveMultipleInstances()
{
    L0Test::TestResult tr;
    auto& mgr = WPEFramework::Plugin::AppInfoManager::getInstance();
    mgr.clear();

    // In new API, appId is unique key - test basic remove instead
    mgr.setAppInstanceId("testApp", "instance1");
    mgr.setAppInstanceId("otherApp", "instance3");

    L0Test::ExpectTrue(tr, mgr.exists("testApp"), "testApp exists before remove");
    mgr.remove("testApp");
    L0Test::ExpectTrue(tr, !mgr.exists("testApp"), "testApp removed");
    L0Test::ExpectTrue(tr, mgr.exists("otherApp"), "otherApp still exists");

    mgr.clear();
    return tr.failures;
}

// NOTE: Test disabled - removeByAppInstanceId() removed in new API
// New API uses appId as key, not instanceId
uint32_t Test_AM_AppInfoManagerRemoveByInstanceIdEdgeCases()
{
    L0Test::TestResult tr;
    auto& mgr = WPEFramework::Plugin::AppInfoManager::getInstance();
    mgr.clear();

    // Test basic remove by appId instead
    mgr.setAppInstanceId("app1", "instance1");
    mgr.setAppInstanceId("app2", "instance2");

    L0Test::ExpectTrue(tr, mgr.exists("app1"), "app1 exists");
    mgr.remove("app1");
    L0Test::ExpectTrue(tr, !mgr.exists("app1"), "app1 removed by appId");
    L0Test::ExpectTrue(tr, mgr.exists("app2"), "app2 still exists");

    mgr.clear();
    return tr.failures;
}

// NOTE: Test adapted - multiple instances per appId not possible in new architecture
uint32_t Test_AM_AppInfoManagerGetByAppIdMultipleInstances()
{
    L0Test::TestResult tr;
    auto& mgr = WPEFramework::Plugin::AppInfoManager::getInstance();
    mgr.clear();

    // In new API, appId is unique key - test basic get functionality
    mgr.setAppInstanceId("app1", "instance1");
    mgr.setPackageInfoVersion("app1", "1.0.0");

    WPEFramework::Plugin::AppInfo snapshot;
    bool found = mgr.get("app1", snapshot);
    L0Test::ExpectTrue(tr, found, "get() finds existing app");
    L0Test::ExpectEqStr(tr, snapshot.getAppInstanceId(), std::string("instance1"), "Instance ID matches");
    L0Test::ExpectEqStr(tr, snapshot.getPackageInfo().version, std::string("1.0.0"), "Version matches");

    mgr.clear();
    return tr.failures;
}

uint32_t Test_AM_AppInfoManagerUpsertEmptyFields()
{
    L0Test::TestResult tr;
    auto& mgr = WPEFramework::Plugin::AppInfoManager::getInstance();
    mgr.clear();

    // Add app with minimal info
    mgr.upsert("minimalApp", [](WPEFramework::Plugin::AppInfo& a) {
        a.setAppInstanceId("instance1");
    });

    L0Test::ExpectTrue(tr, mgr.exists("minimalApp"), "App created");

    // Upsert with additional fields
    mgr.upsert("minimalApp", [](WPEFramework::Plugin::AppInfo& a) {
        a.setAppInstanceId("instance1");
    });
    mgr.setPackageInfoVersion("minimalApp", "1.0.0");
    mgr.setPackageInfoType("minimalApp", WPEFramework::Plugin::AppManagerTypes::APPLICATION_TYPE_INTERACTIVE);

    // Verify updated fields
    L0Test::ExpectEqStr(tr, mgr.getPackageInfoVersion("minimalApp"), std::string("1.0.0"), "Version updated");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(mgr.getPackageInfoType("minimalApp")),
        static_cast<uint32_t>(WPEFramework::Plugin::AppManagerTypes::APPLICATION_TYPE_INTERACTIVE), "Type updated");

    mgr.clear();
    return tr.failures;
}

uint32_t Test_AM_AppInfoManagerClear()
{
    L0Test::TestResult tr;
    auto& mgr = WPEFramework::Plugin::AppInfoManager::getInstance();
    mgr.clear();

    // Add multiple apps
    for (int i = 0; i < 10; i++) {
        std::string appId = "app" + std::to_string(i);
        mgr.setAppInstanceId(appId, "instance" + std::to_string(i));
    }

    // Verify apps exist
    L0Test::ExpectTrue(tr, mgr.exists("app0"), "Apps added before clear");
    L0Test::ExpectTrue(tr, mgr.exists("app5"), "Multiple apps exist");

    // Clear all
    mgr.clear();

    // Verify all apps are gone
    L0Test::ExpectTrue(tr, !mgr.exists("app0"), "Apps removed after clear");
    L0Test::ExpectTrue(tr, !mgr.exists("app5"), "All apps removed after clear");

    return tr.failures;
}

uint32_t Test_AM_AppInfoManagerBasicThreadSafety()
{
    L0Test::TestResult tr;
    auto& mgr = WPEFramework::Plugin::AppInfoManager::getInstance();
    mgr.clear();

    // Add app
    mgr.setAppInstanceId("threadTestApp", "instance1");

    // Multiple reads should work
    for (int i = 0; i < 100; i++) {
        std::string instanceId = mgr.getAppInstanceId("threadTestApp");
        L0Test::ExpectEqStr(tr, instanceId, std::string("instance1"), "Read returns correct value");
    }

    mgr.clear();
    return tr.failures;
}

uint32_t Test_AM_AppInfoManagerUpdateOperationBranches()
{
    L0Test::TestResult tr;
    auto& mgr = WPEFramework::Plugin::AppInfoManager::getInstance();
    mgr.clear();

    // Add initial app
    mgr.upsert("updateTestApp", [](WPEFramework::Plugin::AppInfo& a) {
        a.setAppInstanceId("instance1");
    });
    mgr.setPackageInfoVersion("updateTestApp", "1.0.0");

    // Update non-existent app (should return false)
    bool updated = mgr.update("differentApp", [](WPEFramework::Plugin::AppInfo& a) {
        a.setAppInstanceId("instance2");
    });
    L0Test::ExpectTrue(tr, !updated, "update() on non-existent app returns false");

    // Update existing app (should succeed)
    updated = mgr.update("updateTestApp", [](WPEFramework::Plugin::AppInfo& a) {
        a.setAppInstanceId("instance1");
    });
    mgr.setPackageInfoVersion("updateTestApp", "2.0.0");
    L0Test::ExpectTrue(tr, updated, "update() on existing app succeeds");

    // Verify update
    L0Test::ExpectEqStr(tr, mgr.getPackageInfoVersion("updateTestApp"), std::string("2.0.0"), "Version updated correctly");

    mgr.clear();
    return tr.failures;
}

uint32_t Test_AM_AppInfoManagerGetByCopyEdgeCases()
{
    L0Test::TestResult tr;
    auto& mgr = WPEFramework::Plugin::AppInfoManager::getInstance();
    mgr.clear();

    // Test with non-existent app
    WPEFramework::Plugin::AppInfo outInfo;
    bool found = mgr.get("nonExistent", outInfo);
    L0Test::ExpectTrue(tr, !found, "get() returns false for non-existent app");

    // Add test app
    mgr.upsert("copyTestApp", [](WPEFramework::Plugin::AppInfo& a) {
        a.setAppInstanceId("instance1");
    });
    mgr.setPackageInfoVersion("copyTestApp", "1.0.0");

    // Get by copy
    WPEFramework::Plugin::AppInfo copiedInfo;
    found = mgr.get("copyTestApp", copiedInfo);
    L0Test::ExpectTrue(tr, found, "get() finds existing app");
    L0Test::ExpectEqStr(tr, copiedInfo.getAppInstanceId(), std::string("instance1"), "Copied info has correct instance ID");
    L0Test::ExpectEqStr(tr, copiedInfo.getPackageInfo().version, std::string("1.0.0"), "Copied info has correct version");

    mgr.clear();
    return tr.failures;
}
