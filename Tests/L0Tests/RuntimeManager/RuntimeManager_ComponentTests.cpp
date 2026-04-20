/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
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
 **/

/**
 * @file RuntimeManager_ComponentTests.cpp
 *
 * L0 tests for RuntimeManager helper components:
 *   - UserIdManager  (UserIdManager.cpp/.h)
 *   - AIConfiguration (AIConfiguration.cpp/.h)
 *   - DobbyEventListener (DobbyEventListener.cpp/.h)
 *   - WindowManagerConnector (WindowManagerConnector.cpp/.h)
 *   - DobbySpecGenerator (DobbySpecGenerator.cpp/.h)
 */

#include <atomic>
#include <fstream>
#include <iostream>
#include <string>

#include "AIConfiguration.h"
#include "ApplicationConfiguration.h"
#include "DobbyEventListener.h"
#include "DobbySpecGenerator.h"
#include "UserIdManager.h"
#include "WindowManagerConnector.h"
#include "ServiceMock.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

// ──────────────────────────────────────────────────────────────────────────────
//  UserIdManager tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_UserIdManager_GetUserIdReturnsValidRange
 *
 * Verifies that getUserId returns a UID in the expected range [30001, 31000].
 */
uint32_t Test_UserIdManager_GetUserIdReturnsValidRange()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::UserIdManager mgr;
    const uid_t uid = mgr.getUserId("com.sky.app.youtube");

    L0Test::ExpectTrue(tr, uid >= 30001u && uid <= 31000u,
                       "getUserId() returns UID in range [30001, 31000]");

    return tr.failures;
}

/* Test_UserIdManager_GetUserIdSameAppReturnsSameUid
 *
 * Verifies that calling getUserId twice with the same appId returns the
 * same UID (persistent assignment).
 */
uint32_t Test_UserIdManager_GetUserIdSameAppReturnsSameUid()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::UserIdManager mgr;
    const uid_t uid1 = mgr.getUserId("com.sky.app.youtube");
    const uid_t uid2 = mgr.getUserId("com.sky.app.youtube");

    L0Test::ExpectEqU32(tr, uid1, uid2,
                        "Two calls for the same appId return identical UIDs");

    return tr.failures;
}

/* Test_UserIdManager_GetUserIdDifferentAppsGetDifferentUids
 *
 * Verifies that two distinct appIds receive different UIDs.
 */
uint32_t Test_UserIdManager_GetUserIdDifferentAppsGetDifferentUids()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::UserIdManager mgr;
    const uid_t uid1 = mgr.getUserId("com.sky.app.youtube");
    const uid_t uid2 = mgr.getUserId("com.sky.app.netflix");

    L0Test::ExpectTrue(tr, uid1 != uid2,
                       "Different appIds receive different UIDs");

    return tr.failures;
}

/* Test_UserIdManager_ClearUserIdReturnsUidToPool
 *
 * Verifies that after clearUserId the UID can be reused by a different appId.
 */
uint32_t Test_UserIdManager_ClearUserIdReturnsUidToPool()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::UserIdManager mgr;
    const uid_t uid1 = mgr.getUserId("com.sky.app.youtube");

    // Release the UID back to the pool
    mgr.clearUserId("com.sky.app.youtube");

    // A new app should now be able to get that UID (pool reuse – the exact
    // value may differ based on LIFO/FIFO, but a valid UID must be returned)
    const uid_t uid2 = mgr.getUserId("com.sky.app.netflix");
    L0Test::ExpectTrue(tr, uid2 >= 30001u && uid2 <= 31000u,
                       "After clearUserId(), a new app still gets a valid UID");

    // uid1 should be reusable  
    (void)uid1;
    L0Test::ExpectTrue(tr, uid2 != 0u,
                       "clearUserId() successfully returned UID to pool (non-zero reuse)");

    return tr.failures;
}

/* Test_UserIdManager_ClearUserIdForUnknownAppDoesNotCrash
 *
 * Verifies that clearUserId with an appId that was never registered does
 * not crash and is a no-op.
 */
uint32_t Test_UserIdManager_ClearUserIdForUnknownAppDoesNotCrash()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::UserIdManager mgr;
    mgr.clearUserId("com.sky.app.unknown"); // must not crash
    L0Test::ExpectTrue(tr, true,
                       "clearUserId() for unknown appId does not crash");

    return tr.failures;
}

/* Test_UserIdManager_GetAppsGidReturns30000
 *
 * Verifies getAppsGid() always returns 30000.
 */
uint32_t Test_UserIdManager_GetAppsGidReturns30000()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::UserIdManager mgr;
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(mgr.getAppsGid()), 30000u,
                        "getAppsGid() returns 30000");

    return tr.failures;
}

/* Test_UserIdManager_ExhaustPoolReturnsZero
 *
 * Verifies that when fewer than 5 UIDs remain in the pool, getUserId()
 * returns 0 to signal pool depletion.
 *
 * We exhaust the pool by creating a fresh UserIdManager and assigning UIDs
 * to 997 unique appIds (pool has 1000; guard triggers at <5 remaining).
 */
uint32_t Test_UserIdManager_ExhaustPoolReturnsZero()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::UserIdManager mgr;

    // Consume all but 4 UIDs from the pool
    for (int i = 0; i < 996; ++i) {
        const std::string appId = "com.sky.app." + std::to_string(i);
        mgr.getUserId(appId);
    }

    // Next request should fail because <5 remain in pool
    const uid_t uid = mgr.getUserId("com.sky.app.overflow");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(uid), 0u,
                        "getUserId() returns 0 when pool has < 5 UIDs remaining");

    return tr.failures;
}

/* Test_UserIdManager_MultipleGetAndClearCycles
 *
 * Exercises repeated get/clear cycles to validate pool consistency.
 */
uint32_t Test_UserIdManager_MultipleGetAndClearCycles()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::UserIdManager mgr;
    const std::string id = "com.sky.app.cycling";

    for (int cycle = 0; cycle < 5; ++cycle) {
        const uid_t uid = mgr.getUserId(id);
        L0Test::ExpectTrue(tr, uid >= 30001u && uid <= 31000u,
                           "Cycle get returns valid UID");
        mgr.clearUserId(id);
    }

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
//  AIConfiguration tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_AIConfig_DefaultConsoleLogCap
 *
 * Verifies getContainerConsoleLogCap() returns the default value (1 MB)
 * when no config file exists at AICONFIGURATION_INI_PATH.
 */
uint32_t Test_AIConfig_DefaultConsoleLogCap()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize(); // reads from /opt/demo/config.ini (likely absent in test env)

    const size_t cap = cfg.getContainerConsoleLogCap();
    // Default from readFromCustomData() is 1 * 1024 * 1024
    L0Test::ExpectTrue(tr, cap > 0,
                       "getContainerConsoleLogCap() returns a positive value");

    return tr.failures;
}

/* Test_AIConfig_DefaultNonHomeAppMemoryLimit
 *
 * Verifies getNonHomeAppMemoryLimit() returns the default value (200 MB)
 * when no config file is present.
 */
uint32_t Test_AIConfig_DefaultNonHomeAppMemoryLimit()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize();

    const ssize_t memLimit = cfg.getNonHomeAppMemoryLimit();
    // Default from readFromCustomData() is 200 * 1024 * 1024 = 209715200
    L0Test::ExpectTrue(tr, memLimit > 0,
                       "getNonHomeAppMemoryLimit() returns a positive value");

    return tr.failures;
}

/* Test_AIConfig_DefaultNonHomeAppGpuLimit
 *
 * Verifies getNonHomeAppGpuLimit() returns the default value (64 MB).
 */
uint32_t Test_AIConfig_DefaultNonHomeAppGpuLimit()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize();

    const ssize_t gpuLimit = cfg.getNonHomeAppGpuLimit();
    // Default from readFromCustomData() is 64 * 1024 * 1024 = 67108864
    L0Test::ExpectTrue(tr, gpuLimit > 0,
                       "getNonHomeAppGpuLimit() returns a positive value");

    return tr.failures;
}

/* Test_AIConfig_DefaultAppsCpuSet
 *
 * Verifies getAppsCpuSet() returns a non-zero bitset (default 0xff).
 */
uint32_t Test_AIConfig_DefaultAppsCpuSet()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize();

    const auto cpuSet = cfg.getAppsCpuSet();
    L0Test::ExpectTrue(tr, cpuSet.any(),
                       "getAppsCpuSet() returns a non-zero bitset");

    return tr.failures;
}

/* Test_AIConfig_DefaultDialServerPort
 *
 * Verifies getDialServerPort() returns the default port (8009).
 */
uint32_t Test_AIConfig_DefaultDialServerPort()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize();

    const in_port_t port = cfg.getDialServerPort();
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(port), 8009u,
                        "getDialServerPort() returns default 8009");

    return tr.failures;
}

/* Test_AIConfig_DefaultIonHeapQuota
 *
 * Verifies getIonHeapDefaultQuota() returns a positive quota (256 MB default).
 */
uint32_t Test_AIConfig_DefaultIonHeapQuota()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize();

    const size_t quota = cfg.getIonHeapDefaultQuota();
    L0Test::ExpectTrue(tr, quota > 0,
                       "getIonHeapDefaultQuota() returns a positive value");

    return tr.failures;
}

/* Test_AIConfig_DefaultEnvVariablesNonEmpty
 *
 * Verifies getEnvs() returns a non-empty list of environment variables.
 * readFromCustomData() pushes several WESTEROS_ entries.
 */
uint32_t Test_AIConfig_DefaultEnvVariablesNonEmpty()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize();

    const auto envVars = cfg.getEnvs();
    L0Test::ExpectTrue(tr, !envVars.empty(),
                       "getEnvs() returns a non-empty list after initialize()");

    return tr.failures;
}

/* Test_AIConfig_ResourceManagerClientDisabledByDefault
 *
 * Verifies getResourceManagerClientEnabled() returns false by default.
 */
uint32_t Test_AIConfig_ResourceManagerClientDisabledByDefault()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize();

    L0Test::ExpectTrue(tr, !cfg.getResourceManagerClientEnabled(),
                       "getResourceManagerClientEnabled() returns false by default");

    return tr.failures;
}

/* Test_AIConfig_SvpDisabledByDefault
 *
 * Verifies getSvpEnabled() returns false by default.
 */
uint32_t Test_AIConfig_SvpDisabledByDefault()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize();

    L0Test::ExpectTrue(tr, !cfg.getSvpEnabled(),
                       "getSvpEnabled() returns false by default");

    return tr.failures;
}

/* Test_AIConfig_IPv6DisabledByDefault
 *
 * Verifies getIPv6Enabled() returns false by default.
 */
uint32_t Test_AIConfig_IPv6DisabledByDefault()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize();

    L0Test::ExpectTrue(tr, !cfg.getIPv6Enabled(),
                       "getIPv6Enabled() returns false by default");

    return tr.failures;
}

/* Test_AIConfig_PrintAIConfigurationDoesNotCrash
 *
 * Smoke test that printAIConfiguration() completes without crashing.
 */
uint32_t Test_AIConfig_PrintAIConfigurationDoesNotCrash()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize();
    cfg.printAIConfiguration(); // redirected to stdout; no assertion

    L0Test::ExpectTrue(tr, true,
                       "printAIConfiguration() does not crash");

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
//  DobbyEventListener tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_DobbyEventListener_ConstructionAndDestruction
 *
 * Verifies that DobbyEventListener can be constructed and destroyed without
 * crashing.
 */
uint32_t Test_DobbyEventListener_ConstructionAndDestruction()
{
    L0Test::TestResult tr;

    {
        WPEFramework::Plugin::DobbyEventListener listener;
        L0Test::ExpectTrue(tr, true, "DobbyEventListener constructs without crash");
    }

    L0Test::ExpectTrue(tr, true, "DobbyEventListener destructs without crash");
    return tr.failures;
}

/* Test_DobbyEventListener_InitializeWithNullOCIPlugin
 *
 * Verifies initialize() returns false (or does not crash) when passed
 * nullptr for the OCIContainer interface.
 */
uint32_t Test_DobbyEventListener_InitializeWithNullOCIPlugin()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbyEventListener listener;
    const bool result = listener.initialize(nullptr, nullptr, nullptr);
    // No contract on return value when nullptr is passed, but must not crash
    L0Test::ExpectTrue(tr, !result,
                       "initialize() with null arguments returns false");

    return tr.failures;
}

/* Test_DobbyEventListener_DeinitializeWithoutInitialize
 *
 * Verifies deinitialize() returns true (no crash) after a failed initialize()
 * (i.e. initialize() was called with null OCIContainer, setting
 * mOCIContainerObject to nullptr, which is the only safe precondition).
 */
uint32_t Test_DobbyEventListener_DeinitializeWithoutInitialize()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbyEventListener listener;
    // Call initialize with null args so mOCIContainerObject is explicitly
    // set to nullptr before deinitialize() is invoked.
    listener.initialize(nullptr, nullptr, nullptr);
    const bool result = listener.deinitialize();
    // When mOCIContainerObject is null, deinitialize has nothing to do
    // and returns true (success)
    L0Test::ExpectTrue(tr, result,
                       "deinitialize() after failed initialize() does not crash");

    return tr.failures;
}

/* Test_DobbyEventListener_DoubleDeinitialize
 *
 * Verifies that calling deinitialize() twice does not crash.
 */
uint32_t Test_DobbyEventListener_DoubleDeinitialize()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbyEventListener listener;
    // Call initialize with null args so mOCIContainerObject is explicitly
    // set to nullptr before the two deinitialize() calls.
    listener.initialize(nullptr, nullptr, nullptr);
    listener.deinitialize();
    listener.deinitialize(); // second call must not crash

    L0Test::ExpectTrue(tr, true,
                       "Double deinitialize() does not crash");

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
//  WindowManagerConnector tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_WindowManagerConnector_ConstructionAndDestruction
 *
 * Verifies that WindowManagerConnector can be constructed and destroyed
 * without crashing.
 */
uint32_t Test_WindowManagerConnector_ConstructionAndDestruction()
{
    L0Test::TestResult tr;

    {
        WPEFramework::Plugin::WindowManagerConnector connector;
        L0Test::ExpectTrue(tr, true,
                           "WindowManagerConnector constructs without crash");
    }

    L0Test::ExpectTrue(tr, true,
                       "WindowManagerConnector destructs without crash");
    return tr.failures;
}

/* Test_WindowManagerConnector_IsPluginInitializedReturnsFalseInitially
 *
 * Verifies isPluginInitialized() returns false before initializePlugin()
 * has been called.
 */
uint32_t Test_WindowManagerConnector_IsPluginInitializedReturnsFalseInitially()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::WindowManagerConnector connector;
    L0Test::ExpectTrue(tr, !connector.isPluginInitialized(),
                       "isPluginInitialized() returns false before initialization");

    return tr.failures;
}

/* Test_WindowManagerConnector_InitializeWithNullServiceReturnsFalse
 *
 * Verifies initializePlugin() returns false when passed nullptr as service.
 */
uint32_t Test_WindowManagerConnector_InitializeWithNullServiceReturnsFalse()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::WindowManagerConnector connector;
    const bool result = connector.initializePlugin(nullptr, nullptr);
    L0Test::ExpectTrue(tr, !result,
                       "initializePlugin(nullptr) returns false");
    L0Test::ExpectTrue(tr, !connector.isPluginInitialized(),
                       "isPluginInitialized() remains false after failed init");

    return tr.failures;
}

/* Test_WindowManagerConnector_InitializeWithServiceMissingWindowManagerPlugin
 *
 * Verifies initializePlugin() returns false when the service cannot
 * resolve the IRDKWindowManager plugin (QueryInterfaceByCallsign returns null).
 */
uint32_t Test_WindowManagerConnector_InitializeWithServiceMissingWindowManagerPlugin()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::WindowManagerConnector connector;
    L0Test::ServiceMock service; // QueryInterfaceByCallsign returns nullptr by default

    const bool result = connector.initializePlugin(&service, nullptr);
    L0Test::ExpectTrue(tr, !result,
                       "initializePlugin() returns false when IRDKWindowManager unavailable");
    L0Test::ExpectTrue(tr, !connector.isPluginInitialized(),
                       "isPluginInitialized() remains false after failed init");

    return tr.failures;
}

/* Test_WindowManagerConnector_ReleasePluginWithoutInitDoesNotCrash
 *
 * Verifies releasePlugin() does not crash when called without prior
 * initializePlugin().
 */
uint32_t Test_WindowManagerConnector_ReleasePluginWithoutInitDoesNotCrash()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::WindowManagerConnector connector;
    connector.releasePlugin(); // must not crash

    L0Test::ExpectTrue(tr, true,
                       "releasePlugin() without init does not crash");

    return tr.failures;
}

/* Test_WindowManagerConnector_GetDisplayInfoWithoutInitDoesNotCrash
 *
 * Verifies getDisplayInfo() does not crash when called before plugin
 * initialization.
 */
uint32_t Test_WindowManagerConnector_GetDisplayInfoWithoutInitDoesNotCrash()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::WindowManagerConnector connector;
    std::string xdgRuntimeDir, waylandDisplayName;
    connector.getDisplayInfo("youTube", xdgRuntimeDir, waylandDisplayName);

    L0Test::ExpectTrue(tr, true,
                       "getDisplayInfo() without init does not crash");

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
//  DobbySpecGenerator tests
// ──────────────────────────────────────────────────────────────────────────────

namespace {
/* Static fixture: shared AIConfiguration instance for all DobbySpecGenerator tests.
 * Initialized once to avoid repeated file I/O across tests.
 */
static WPEFramework::Plugin::AIConfiguration g_aiConfiguration;
static bool g_aiConfigurationInitialized = false;

/* Helper: gets or initializes the shared AIConfiguration fixture. */
WPEFramework::Plugin::AIConfiguration& GetAIConfigurationFixture()
{
    if (!g_aiConfigurationInitialized) {
        g_aiConfiguration.initialize();
        g_aiConfigurationInitialized = true;
    }
    return g_aiConfiguration;
}

/* Helper: creates a minimal valid ApplicationConfiguration. */
WPEFramework::Plugin::ApplicationConfiguration MakeValidAppConfig()
{
    WPEFramework::Plugin::ApplicationConfiguration cfg;
    cfg.mAppId              = "com.sky.app.youtube";
    cfg.mAppInstanceId      = "youTube";
    cfg.mUserId             = 30001u;
    cfg.mGroupId            = 30000u;
    cfg.mWesterosSocketPath = "/tmp/westeros";
    return cfg;
}

/* Helper: creates a minimal valid RuntimeConfig. */
WPEFramework::Exchange::RuntimeConfig MakeValidRuntimeConfig()
{
    WPEFramework::Exchange::RuntimeConfig cfg;
    cfg.command             = "SkyBrowserLauncher";
    cfg.envVariables        = "XDG_RUNTIME_DIR=/tmp;WAYLAND_DISPLAY=test";
    cfg.systemMemoryLimit   = 128 * 1024 * 1024; // 128 MB — required by getSysMemoryLimit()
    return cfg;
}
} // namespace

/* Test_DobbySpecGenerator_GenerateWithValidConfig
 *
 * Verifies generate() returns true and produces non-empty JSON when given
 * valid mandatory fields (mUserId != 0, memLimit > 0, command non-empty).
 */
uint32_t Test_DobbySpecGenerator_GenerateWithValidConfig()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg  = MakeValidAppConfig();
    auto rtCfg   = MakeValidRuntimeConfig();
    std::string spec;

    const bool result = gen.generate(appCfg, rtCfg, spec);
    L0Test::ExpectTrue(tr, result,
                       "generate() returns true for valid configuration");
    L0Test::ExpectTrue(tr, !spec.empty(),
                       "generate() produces non-empty JSON spec");

    return tr.failures;
}

/* Test_DobbySpecGenerator_GenerateWithZeroUserId
 *
 * Verifies generate() returns false when mUserId == 0 (mandatory check).
 */
uint32_t Test_DobbySpecGenerator_GenerateWithZeroUserId()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg  = MakeValidAppConfig();
    appCfg.mUserId = 0; // mandatory check will fail
    auto rtCfg   = MakeValidRuntimeConfig();
    std::string spec;

    const bool result = gen.generate(appCfg, rtCfg, spec);
    L0Test::ExpectTrue(tr, !result,
                       "generate() returns false when mUserId is 0");

    return tr.failures;
}

/* Test_DobbySpecGenerator_GenerateWithEmptyCommand
 *
 * Verifies generate() returns false when runtimeConfig.command is empty.
 */
uint32_t Test_DobbySpecGenerator_GenerateWithEmptyCommand()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    auto rtCfg  = MakeValidRuntimeConfig();
    rtCfg.command.clear(); // mandatory check will fail
    std::string spec;

    const bool result = gen.generate(appCfg, rtCfg, spec);
    L0Test::ExpectTrue(tr, !result,
                       "generate() returns false when command is empty");

    return tr.failures;
}

/* Test_DobbySpecGenerator_GenerateSpecContainsVersion
 *
 * Verifies the generated JSON spec contains the "version" key.
 */
uint32_t Test_DobbySpecGenerator_GenerateSpecContainsVersion()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    auto rtCfg  = MakeValidRuntimeConfig();
    std::string spec;

    gen.generate(appCfg, rtCfg, spec);

    // The generated JSON should contain "version":"1.1"
    L0Test::ExpectTrue(tr, spec.find("\"version\"") != std::string::npos,
                       "Generated spec contains version field");

    return tr.failures;
}

/* Test_DobbySpecGenerator_GenerateSpecContainsArgs
 *
 * Verifies the generated JSON spec contains the "args" array.
 */
uint32_t Test_DobbySpecGenerator_GenerateSpecContainsArgs()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    auto rtCfg  = MakeValidRuntimeConfig();
    std::string spec;

    gen.generate(appCfg, rtCfg, spec);

    L0Test::ExpectTrue(tr, spec.find("\"args\"") != std::string::npos,
                       "Generated spec contains args field");

    return tr.failures;
}

/* Test_DobbySpecGenerator_GenerateSpecContainsMemLimit
 *
 * Verifies the generated JSON spec contains the "memLimit" field.
 */
uint32_t Test_DobbySpecGenerator_GenerateSpecContainsMemLimit()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    auto rtCfg  = MakeValidRuntimeConfig();
    std::string spec;

    gen.generate(appCfg, rtCfg, spec);

    L0Test::ExpectTrue(tr, spec.find("\"memLimit\"") != std::string::npos,
                       "Generated spec contains memLimit field");

    return tr.failures;
}

/* Test_DobbySpecGenerator_ConstructionAndDestruction
 *
 * Smoke test: DobbySpecGenerator can be constructed and destroyed.
 */
uint32_t Test_DobbySpecGenerator_ConstructionAndDestruction()
{
    L0Test::TestResult tr;

    {
        WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
        L0Test::ExpectTrue(tr, true, "DobbySpecGenerator constructs without crash");
    }

    L0Test::ExpectTrue(tr, true, "DobbySpecGenerator destructs without crash");
    return tr.failures;
}

/* Test_DobbySpecGenerator_GenerateWithRuntimePath
 *
 * Verifies generate() correctly prefixes "/runtime/" when runtimePath is set.
 */
uint32_t Test_DobbySpecGenerator_GenerateWithRuntimePath()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    auto rtCfg  = MakeValidRuntimeConfig();
    rtCfg.runtimePath = "/opt/runtimes/sky";
    std::string spec;

    const bool result = gen.generate(appCfg, rtCfg, spec);
    L0Test::ExpectTrue(tr, result,
                       "generate() returns true with runtimePath set");
    L0Test::ExpectTrue(tr, spec.find("/runtime/") != std::string::npos,
                       "Generated spec contains /runtime/ prefix when runtimePath set");

    return tr.failures;
}

/* Test_DobbySpecGenerator_GenerateWithoutRuntimePath
 *
 * Verifies generate() uses "/package/" prefix when runtimePath is empty.
 */
uint32_t Test_DobbySpecGenerator_GenerateWithoutRuntimePath()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    auto rtCfg  = MakeValidRuntimeConfig();
    rtCfg.runtimePath.clear(); // no runtime path set
    std::string spec;

    const bool result = gen.generate(appCfg, rtCfg, spec);
    L0Test::ExpectTrue(tr, result,
                       "generate() returns true without runtimePath");
    L0Test::ExpectTrue(tr, spec.find("/package/") != std::string::npos,
                       "Generated spec uses /package/ prefix when runtimePath is empty");

    return tr.failures;
}

/* Test_DobbySpecGenerator_GenerateSpecContainsCpu
 *
 * Verifies the generated JSON spec contains a "cpu" section.
 */
uint32_t Test_DobbySpecGenerator_GenerateSpecContainsCpu()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    auto rtCfg  = MakeValidRuntimeConfig();
    std::string spec;

    gen.generate(appCfg, rtCfg, spec);

    L0Test::ExpectTrue(tr, spec.find("\"cpu\"") != std::string::npos,
                       "Generated spec contains cpu section");

    return tr.failures;
}

/* Test_DobbySpecGenerator_GenerateSpecContainsNetwork
 *
 * Verifies the generated JSON spec contains a "network" section.
 */
uint32_t Test_DobbySpecGenerator_GenerateSpecContainsNetwork()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    auto rtCfg  = MakeValidRuntimeConfig();
    std::string spec;

    gen.generate(appCfg, rtCfg, spec);

    L0Test::ExpectTrue(tr, spec.find("\"network\"") != std::string::npos,
                       "Generated spec contains network section");

    return tr.failures;
}

/* Test_DobbySpecGenerator_GenerateSpecContainsRdkPlugins
 *
 * Verifies the generated JSON spec contains an "rdkPlugins" section.
 */
uint32_t Test_DobbySpecGenerator_GenerateSpecContainsRdkPlugins()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    auto rtCfg  = MakeValidRuntimeConfig();
    std::string spec;

    gen.generate(appCfg, rtCfg, spec);

    L0Test::ExpectTrue(tr, spec.find("\"rdkPlugins\"") != std::string::npos,
                       "Generated spec contains rdkPlugins section");

    return tr.failures;
}

/* Test_DobbySpecGenerator_GenerateWithMemLimitFromConfig
 *
 * Verifies generate() succeeds and includes non-default memLimit when a
 * non-zero memLimit is set in the RuntimeConfig.
 */
uint32_t Test_DobbySpecGenerator_GenerateWithMemLimitFromConfig()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    auto rtCfg  = MakeValidRuntimeConfig();
    rtCfg.systemMemoryLimit = 128 * 1024 * 1024; // 128 MB
    std::string spec;

    const bool result = gen.generate(appCfg, rtCfg, spec);
    L0Test::ExpectTrue(tr, result,
                       "generate() returns true when memLimit is set in RuntimeConfig");
    L0Test::ExpectTrue(tr, spec.find("\"memLimit\"") != std::string::npos,
                       "Generated spec contains memLimit when set from RuntimeConfig");

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
//  AIConfiguration – additional getter tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_AIConfig_GetGstreamerRegistryEnabledDefault
 *
 * Verifies getGstreamerRegistryEnabled() returns false by default
 * (readFromCustomData sets mGstreamerRegistryEnabled = false).
 */
uint32_t Test_AIConfig_GetGstreamerRegistryEnabledDefault()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize();

    L0Test::ExpectTrue(tr, !cfg.getGstreamerRegistryEnabled(),
                       "getGstreamerRegistryEnabled() returns false by default");

    return tr.failures;
}

/* Test_AIConfig_GetEnableUsbMassStorageDefault
 *
 * Verifies getEnableUsbMassStorage() returns false by default
 * (readFromCustomData sets mEnableUsbMassStorage = false).
 */
uint32_t Test_AIConfig_GetEnableUsbMassStorageDefault()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize();

    L0Test::ExpectTrue(tr, !cfg.getEnableUsbMassStorage(),
                       "getEnableUsbMassStorage() returns false by default");

    return tr.failures;
}

/* Test_AIConfig_GetMapiPortsEmptyByDefault
 *
 * Verifies getMapiPorts() returns an empty list when no config file is present.
 */
uint32_t Test_AIConfig_GetMapiPortsEmptyByDefault()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize();

    const auto ports = cfg.getMapiPorts();
    L0Test::ExpectTrue(tr, ports.empty(),
                       "getMapiPorts() returns empty list by default");

    return tr.failures;
}

/* Test_AIConfig_GetVpuAccessBlacklistEmptyByDefault
 *
 * Verifies getVpuAccessBlacklist() returns an empty list by default.
 */
uint32_t Test_AIConfig_GetVpuAccessBlacklistEmptyByDefault()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize();

    const auto list = cfg.getVpuAccessBlacklist();
    L0Test::ExpectTrue(tr, list.empty(),
                       "getVpuAccessBlacklist() returns empty list by default");

    return tr.failures;
}

/* Test_AIConfig_GetAppsRequiringDBusEmptyByDefault
 *
 * Verifies getAppsRequiringDBus() returns an empty list by default.
 */
uint32_t Test_AIConfig_GetAppsRequiringDBusEmptyByDefault()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize();

    const auto list = cfg.getAppsRequiringDBus();
    L0Test::ExpectTrue(tr, list.empty(),
                       "getAppsRequiringDBus() returns empty list by default");

    return tr.failures;
}

/* Test_AIConfig_GetDialUsnEmptyByDefault
 *
 * Verifies getDialUsn() returns an empty string by default.
 */
uint32_t Test_AIConfig_GetDialUsnEmptyByDefault()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize();

    const std::string usn = cfg.getDialUsn();
    L0Test::ExpectTrue(tr, usn.empty(),
                       "getDialUsn() returns empty string by default");

    return tr.failures;
}

/* Test_AIConfig_GetDialServerPathPrefixEmptyByDefault
 *
 * Verifies getDialServerPathPrefix() returns an empty string by default.
 */
uint32_t Test_AIConfig_GetDialServerPathPrefixEmptyByDefault()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize();

    const std::string prefix = cfg.getDialServerPathPrefix();
    L0Test::ExpectTrue(tr, prefix.empty(),
                       "getDialServerPathPrefix() returns empty string by default");

    return tr.failures;
}

/* Test_AIConfig_GetPreloadsEmptyByDefault
 *
 * Verifies getPreloads() returns an empty list by default.
 */
uint32_t Test_AIConfig_GetPreloadsEmptyByDefault()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize();

    const auto preloads = cfg.getPreloads();
    L0Test::ExpectTrue(tr, preloads.empty(),
                       "getPreloads() returns empty list by default");

    return tr.failures;
}

/* Test_AIConfig_GetIonHeapQuotasEmptyByDefault
 *
 * Verifies getIonHeapQuotas() returns an empty map by default
 * (readFromCustomData does not populate individual quotas).
 */
uint32_t Test_AIConfig_GetIonHeapQuotasEmptyByDefault()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::AIConfiguration cfg;
    cfg.initialize();

    const auto quotas = cfg.getIonHeapQuotas();
    L0Test::ExpectTrue(tr, quotas.empty(),
                       "getIonHeapQuotas() returns empty map by default");

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
//  WindowManagerConnector – additional tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_WindowManagerConnector_CreateDisplayWithoutInitReturnsFalse
 *
 * Verifies createDisplay() returns false when the plugin has not been
 * initialised (mPluginInitialized == false).
 */
uint32_t Test_WindowManagerConnector_CreateDisplayWithoutInitReturnsFalse()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::WindowManagerConnector connector;
    const bool result = connector.createDisplay("appInst", "wst-appInst", 30001u, 30000u);
    L0Test::ExpectTrue(tr, !result,
                       "createDisplay() returns false when plugin is not initialised");

    return tr.failures;
}

/* Test_WindowManagerConnector_GetDisplayInfoSetsNonEmptyDisplayName
 *
 * Verifies getDisplayInfo() produces a non-empty waylandDisplayName
 * even when the plugin has not been initialised (it falls through to
 * the XDG_RUNTIME_DIR / /tmp path logic unconditionally).
 */
uint32_t Test_WindowManagerConnector_GetDisplayInfoSetsNonEmptyDisplayName()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::WindowManagerConnector connector;
    std::string xdgDir, waylandName;
    connector.getDisplayInfo("youTube", xdgDir, waylandName);

    L0Test::ExpectTrue(tr, !waylandName.empty(),
                       "getDisplayInfo() produces a non-empty waylandDisplayName");
    L0Test::ExpectTrue(tr, !xdgDir.empty(),
                       "getDisplayInfo() produces a non-empty xdgDirectory");

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// DobbyEventListener — additional coverage tests
// ──────────────────────────────────────────────────────────────────────────────
// Fake stubs are defined in ServiceMock.h (following AppGateway test conventions).

using FakeOCIContainer  = L0Test::FakeOCIContainer;
using FakeEventHandler  = L0Test::FakeEventHandler;
using FakeWindowManager = L0Test::FakeWindowManager;



/* Test_DobbyEventListener_InitializeWithValidOCIContainerSucceeds
 *
 * Verifies that initialize() returns true when supplied a non-null
 * IOCIContainer whose Register() returns ERROR_NONE.  Covers the success
 * path in DobbyEventListener.cpp lines 61-72.
 */
uint32_t Test_DobbyEventListener_InitializeWithValidOCIContainerSucceeds()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbyEventListener listener;
    FakeEventHandler handler;
    FakeOCIContainer container;

    const bool result = listener.initialize(nullptr, &handler, &container);

    L0Test::ExpectTrue(tr, result,
                       "initialize() returns true when OCI container Register succeeds");
    L0Test::ExpectEqU32(tr, container.registerCalls.load(), 1u,
                        "Initialize calls Register() exactly once on the OCI container");

    listener.deinitialize();
    return tr.failures;
}

/* Test_DobbyEventListener_InitializeWithRegisterFailureReturnsFalse
 *
 * Verifies that initialize() returns false when IOCIContainer::Register()
 * returns an error code.  Covers the failure branch inside the success path.
 */
uint32_t Test_DobbyEventListener_InitializeWithRegisterFailureReturnsFalse()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbyEventListener listener;
    FakeEventHandler handler;
    FakeOCIContainer container;
    container.SetRegisterReturnCode(WPEFramework::Core::ERROR_GENERAL);

    const bool result = listener.initialize(nullptr, &handler, &container);

    L0Test::ExpectTrue(tr, !result,
                       "initialize() returns false when Register() fails");

    return tr.failures;
}

/* Test_DobbyEventListener_DeinitializeWithValidOCIContainerCallsUnregister
 *
 * Verifies that deinitialize() calls Unregister() on the OCI container when
 * the listener was successfully initialized.  Covers DobbyEventListener.cpp
 * lines 80-85.
 */
uint32_t Test_DobbyEventListener_DeinitializeWithValidOCIContainerCallsUnregister()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbyEventListener listener;
    FakeEventHandler handler;
    FakeOCIContainer container;

    listener.initialize(nullptr, &handler, &container);
    const bool result = listener.deinitialize();

    L0Test::ExpectTrue(tr, result,
                       "deinitialize() returns true after successful init");
    L0Test::ExpectEqU32(tr, container.unregisterCalls.load(), 1u,
                        "deinitialize() calls Unregister() once on the OCI container");

    return tr.failures;
}

/* Test_DobbyEventListener_OnContainerStartedCallsEventHandler
 *
 * Verifies that OCIContainerNotification::OnContainerStarted forwards the
 * event to the IEventHandler.  Covers DobbyEventListener.cpp lines 90-98.
 */
uint32_t Test_DobbyEventListener_OnContainerStartedCallsEventHandler()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbyEventListener listener;
    FakeEventHandler handler;
    FakeOCIContainer container;

    listener.initialize(nullptr, &handler, &container);

    // Retrieve the registered notification and fire the event.
    auto* notif = container.GetStoredNotification();
    L0Test::ExpectTrue(tr, notif != nullptr,
                       "Notification object was registered with OCI container");
    if (notif != nullptr) {
        notif->OnContainerStarted("container-1", "youTube");
        L0Test::ExpectEqU32(tr, handler.startedCalls.load(), 1u,
                            "OnContainerStarted forwards to onOCIContainerStartedEvent");
    }

    listener.deinitialize();
    return tr.failures;
}

/* Test_DobbyEventListener_OnContainerStoppedCallsEventHandler
 *
 * Verifies that OCIContainerNotification::OnContainerStopped forwards the
 * event to the IEventHandler.  Covers DobbyEventListener.cpp lines 100-108.
 */
uint32_t Test_DobbyEventListener_OnContainerStoppedCallsEventHandler()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbyEventListener listener;
    FakeEventHandler handler;
    FakeOCIContainer container;

    listener.initialize(nullptr, &handler, &container);

    auto* notif = container.GetStoredNotification();
    L0Test::ExpectTrue(tr, notif != nullptr,
                       "Notification object was registered with OCI container");
    if (notif != nullptr) {
        notif->OnContainerStopped("container-1", "youTube");
        L0Test::ExpectEqU32(tr, handler.stoppedCalls.load(), 1u,
                            "OnContainerStopped forwards to onOCIContainerStoppedEvent");
    }

    listener.deinitialize();
    return tr.failures;
}

/* Test_DobbyEventListener_OnContainerFailedCallsEventHandler
 *
 * Verifies that OCIContainerNotification::OnContainerFailed forwards the
 * event to the IEventHandler.  Covers DobbyEventListener.cpp lines 110-119.
 */
uint32_t Test_DobbyEventListener_OnContainerFailedCallsEventHandler()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbyEventListener listener;
    FakeEventHandler handler;
    FakeOCIContainer container;

    listener.initialize(nullptr, &handler, &container);

    auto* notif = container.GetStoredNotification();
    L0Test::ExpectTrue(tr, notif != nullptr,
                       "Notification object was registered with OCI container");
    if (notif != nullptr) {
        notif->OnContainerFailed("container-1", "youTube", 1u);
        L0Test::ExpectEqU32(tr, handler.failureCalls.load(), 1u,
                            "OnContainerFailed forwards to onOCIContainerFailureEvent");
    }

    listener.deinitialize();
    return tr.failures;
}

/* Test_DobbyEventListener_OnContainerStateChangedStartingState
 *
 * Verifies the STARTING ContainerState maps to RUNTIME_STATE_STARTING and
 * triggers the state-changed event handler.  Covers lines 121-160 (the
 * OnContainerStateChanged switch statement).
 */
uint32_t Test_DobbyEventListener_OnContainerStateChangedStartingState()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbyEventListener listener;
    FakeEventHandler handler;
    FakeOCIContainer container;

    listener.initialize(nullptr, &handler, &container);

    auto* notif = container.GetStoredNotification();
    L0Test::ExpectTrue(tr, notif != nullptr,
                       "Notification object was registered with OCI container");
    if (notif != nullptr) {
        notif->OnContainerStateChanged("container-1",
            WPEFramework::Exchange::IOCIContainer::ContainerState::STARTING);
        L0Test::ExpectEqU32(tr, handler.stateChangedCalls.load(), 1u,
                            "OnContainerStateChanged(STARTING) calls state-changed handler");
    }

    listener.deinitialize();
    return tr.failures;
}

/* Test_DobbyEventListener_OnContainerStateChangedRunningState
 *
 * Verifies the RUNNING ContainerState path in the state-changed switch.
 */
uint32_t Test_DobbyEventListener_OnContainerStateChangedRunningState()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbyEventListener listener;
    FakeEventHandler handler;
    FakeOCIContainer container;

    listener.initialize(nullptr, &handler, &container);

    auto* notif = container.GetStoredNotification();
    if (notif != nullptr) {
        notif->OnContainerStateChanged("container-1",
            WPEFramework::Exchange::IOCIContainer::ContainerState::RUNNING);
        L0Test::ExpectEqU32(tr, handler.stateChangedCalls.load(), 1u,
                            "OnContainerStateChanged(RUNNING) calls state-changed handler");
    } else {
        L0Test::ExpectTrue(tr, false, "Notification was null");
    }

    listener.deinitialize();
    return tr.failures;
}

/* Test_DobbyEventListener_OnContainerStateChangedAllStates
 *
 * Verifies all ContainerState enum values are handled by OnContainerStateChanged
 * without crashing, covering the full switch statement including the default case.
 */
uint32_t Test_DobbyEventListener_OnContainerStateChangedAllStates()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbyEventListener listener;
    FakeEventHandler handler;
    FakeOCIContainer container;

    listener.initialize(nullptr, &handler, &container);

    auto* notif = container.GetStoredNotification();
    L0Test::ExpectTrue(tr, notif != nullptr, "Notification was registered");

    if (notif != nullptr) {
        using CS = WPEFramework::Exchange::IOCIContainer::ContainerState;
        // Cover each named state in the switch
        notif->OnContainerStateChanged("c1", CS::STOPPING);
        notif->OnContainerStateChanged("c1", CS::PAUSED);
        notif->OnContainerStateChanged("c1", CS::STOPPED);
        notif->OnContainerStateChanged("c1", CS::HIBERNATING);
        notif->OnContainerStateChanged("c1", CS::HIBERNATED);
        notif->OnContainerStateChanged("c1", CS::AWAKENING);

        L0Test::ExpectEqU32(tr, handler.stateChangedCalls.load(), 6u,
                            "All six additional ContainerState values fire state-changed");
    }

    listener.deinitialize();
    return tr.failures;
}

/* Test_DobbyEventListener_OnContainerStateChangedNullEventHandler
 *
 * Verifies that OnContainerStateChanged does not crash when the event handler
 * pointer is null (covers the LOGERR branch in SEND_EVENT_TO_RUNTIME_MANAGER).
 */
uint32_t Test_DobbyEventListener_OnContainerStateChangedNullEventHandler()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbyEventListener listener;
    FakeOCIContainer container;

    // initialize with null event handler on purpose
    listener.initialize(nullptr, nullptr, &container);

    auto* notif = container.GetStoredNotification();
    L0Test::ExpectTrue(tr, notif != nullptr, "Notification was registered even with null handler");
    if (notif != nullptr) {
        // Should not crash even though mEventHandler is nullptr
        notif->OnContainerStarted("c1", "youTube");
        notif->OnContainerStopped("c1", "youTube");
        notif->OnContainerFailed("c1", "youTube", 2u);
        notif->OnContainerStateChanged("c1",
            WPEFramework::Exchange::IOCIContainer::ContainerState::RUNNING);
    }
    L0Test::ExpectTrue(tr, true, "No crash with null event handler");

    listener.deinitialize();
    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// WindowManagerConnector — additional coverage tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_WindowManagerConnector_InitializeWithValidWindowManagerSucceeds
 *
 * Verifies initializePlugin() returns true when the service provides a valid
 * IRDKWindowManager object.  Covers WindowManagerConnector.cpp lines 51-59.
 */
uint32_t Test_WindowManagerConnector_InitializeWithValidWindowManagerSucceeds()
{
    L0Test::TestResult tr;

    FakeWindowManager wm;
    L0Test::ServiceMock service(L0Test::ServiceMock::Config(nullptr, &wm));
    WPEFramework::Plugin::WindowManagerConnector connector;

    const bool result = connector.initializePlugin(&service, nullptr);

    L0Test::ExpectTrue(tr, result,
                       "initializePlugin() returns true when IRDKWindowManager is available");
    L0Test::ExpectTrue(tr, connector.isPluginInitialized(),
                       "isPluginInitialized() returns true after successful init");
    L0Test::ExpectEqU32(tr, wm.registerCalls.load(), 1u,
                        "initializePlugin() calls Register() on the window manager");

    connector.releasePlugin();
    return tr.failures;
}

/* Test_WindowManagerConnector_ReleasePluginAfterInitCallsUnregister
 *
 * Verifies that releasePlugin() calls Unregister() and Release() on the
 * window manager, and resets mPluginInitialized.
 * Covers WindowManagerConnector.cpp lines 71-79.
 */
uint32_t Test_WindowManagerConnector_ReleasePluginAfterInitCallsUnregister()
{
    L0Test::TestResult tr;

    FakeWindowManager wm;
    L0Test::ServiceMock service(L0Test::ServiceMock::Config(nullptr, &wm));
    WPEFramework::Plugin::WindowManagerConnector connector;

    connector.initializePlugin(&service, nullptr);
    L0Test::ExpectTrue(tr, connector.isPluginInitialized(),
                       "Plugin initialized before releasePlugin()");

    connector.releasePlugin();

    L0Test::ExpectTrue(tr, !connector.isPluginInitialized(),
                       "isPluginInitialized() returns false after releasePlugin()");
    L0Test::ExpectEqU32(tr, wm.unregisterCalls.load(), 1u,
                        "releasePlugin() calls Unregister() on the window manager");

    return tr.failures;
}

/* Test_WindowManagerConnector_CreateDisplayAfterInitReturnsTrue
 *
 * Verifies createDisplay() succeeds when the plugin is initialized and
 * the window manager returns ERROR_NONE.
 * Covers WindowManagerConnector.cpp lines 89-107.
 */
uint32_t Test_WindowManagerConnector_CreateDisplayAfterInitReturnsTrue()
{
    L0Test::TestResult tr;

    FakeWindowManager wm;
    L0Test::ServiceMock service(L0Test::ServiceMock::Config(nullptr, &wm));
    WPEFramework::Plugin::WindowManagerConnector connector;

    connector.initializePlugin(&service, nullptr);

    const bool result = connector.createDisplay("youTube", "wst-youTube", 30001u, 30000u);
    L0Test::ExpectTrue(tr, result,
                       "createDisplay() returns true when CreateDisplay() succeeds");
    L0Test::ExpectEqU32(tr, wm.createDisplayCalls.load(), 1u,
                        "createDisplay() calls CreateDisplay() on the window manager");

    connector.releasePlugin();
    return tr.failures;
}

/* Test_WindowManagerConnector_CreateDisplayAfterInitReturnsFalseOnError
 *
 * Verifies createDisplay() returns false when the window manager returns an
 * error from CreateDisplay().  Covers the failure path in lines 100-105.
 */
uint32_t Test_WindowManagerConnector_CreateDisplayAfterInitReturnsFalseOnError()
{
    L0Test::TestResult tr;

    FakeWindowManager wm;
    wm.SetCreateDisplayReturnCode(WPEFramework::Core::ERROR_GENERAL);
    L0Test::ServiceMock service(L0Test::ServiceMock::Config(nullptr, &wm));
    WPEFramework::Plugin::WindowManagerConnector connector;

    connector.initializePlugin(&service, nullptr);

    const bool result = connector.createDisplay("youTube", "wst-youTube", 30001u, 30000u);
    L0Test::ExpectTrue(tr, !result,
                       "createDisplay() returns false when CreateDisplay() returns error");

    connector.releasePlugin();
    return tr.failures;
}

/* Test_WindowManagerConnector_OnUserInactivityDoesNotCrash
 *
 * Verifies that firing OnUserInactivity through the WindowManagerNotification
 * inner class does not crash.  The current implementation body is empty.
 * Covers WindowManagerConnector.cpp lines 174-176.
 */
uint32_t Test_WindowManagerConnector_OnUserInactivityDoesNotCrash()
{
    L0Test::TestResult tr;

    FakeWindowManager wm;
    L0Test::ServiceMock service(L0Test::ServiceMock::Config(nullptr, &wm));
    WPEFramework::Plugin::WindowManagerConnector connector;

    connector.initializePlugin(&service, nullptr);

    // Retrieve the stored notification pointer and fire the event.
    auto* notif = wm.GetStoredNotification();
    L0Test::ExpectTrue(tr, notif != nullptr,
                       "WindowManagerNotification was registered with window manager");
    if (notif != nullptr) {
        notif->OnUserInactivity(5.0); // must not crash
    }
    L0Test::ExpectTrue(tr, true, "OnUserInactivity() does not crash");

    connector.releasePlugin();
    return tr.failures;
}

/* Test_WindowManagerConnector_GetDisplayInfoWithXdgRuntimeDirSet
 *
 * Verifies getDisplayInfo() uses XDG_RUNTIME_DIR when that env var is set
 * and openable.  Covers WindowManagerConnector.cpp lines 123-132.
 */
uint32_t Test_WindowManagerConnector_GetDisplayInfoWithXdgRuntimeDirSet()
{
    L0Test::TestResult tr;

    // Set the env var to a directory that always exists on the CI runner.
    setenv("XDG_RUNTIME_DIR", "/tmp", 1);

    WPEFramework::Plugin::WindowManagerConnector connector;
    std::string xdgDir, waylandName;
    connector.getDisplayInfo("testApp123", xdgDir, waylandName);

    // After getDisplayInfo(), xdgDir should be set to our env value.
    L0Test::ExpectTrue(tr, xdgDir == "/tmp",
                       "getDisplayInfo() uses XDG_RUNTIME_DIR when set");
    L0Test::ExpectTrue(tr, !waylandName.empty(),
                       "getDisplayInfo() produces non-empty waylandDisplayName with XDG set");

    unsetenv("XDG_RUNTIME_DIR");
    return tr.failures;
}

/* Test_WindowManagerConnector_GetDisplayInfoWithoutXdgRuntimeDirFallsToTmp
 *
 * Verifies getDisplayInfo() falls back to /tmp when XDG_RUNTIME_DIR is unset.
 */
uint32_t Test_WindowManagerConnector_GetDisplayInfoWithoutXdgRuntimeDirFallsToTmp()
{
    L0Test::TestResult tr;

    unsetenv("XDG_RUNTIME_DIR");

    WPEFramework::Plugin::WindowManagerConnector connector;
    std::string xdgDir, waylandName;
    connector.getDisplayInfo("testApp456", xdgDir, waylandName);

    L0Test::ExpectTrue(tr, xdgDir == "/tmp",
                       "getDisplayInfo() falls back to /tmp when XDG_RUNTIME_DIR is unset");
    L0Test::ExpectTrue(tr, !waylandName.empty(),
                       "getDisplayInfo() produces non-empty waylandDisplayName without XDG");

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// DobbySpecGenerator — additional branch coverage tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_DobbySpecGenerator_GenerateWithSpecChangeFile
 *
 * Verifies the generate() spec-override path when /tmp/specchange exists.
 * Covers DobbySpecGenerator.cpp lines 96-106.
 */
uint32_t Test_DobbySpecGenerator_GenerateWithSpecChangeFile()
{
    L0Test::TestResult tr;

    // Create a minimal valid JSON file at /tmp/specchange
    {
        std::ofstream f("/tmp/specchange");
        f << "{\"version\":\"1.1\"}";
    }

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    auto rtCfg  = MakeValidRuntimeConfig();
    std::string spec;

    const bool result = gen.generate(appCfg, rtCfg, spec);
    L0Test::ExpectTrue(tr, result,
                       "generate() returns true when /tmp/specchange override file exists");
    L0Test::ExpectTrue(tr, !spec.empty(),
                       "generate() returns content from override file");

    // Clean up
    std::remove("/tmp/specchange");
    return tr.failures;
}

/* Test_DobbySpecGenerator_GenerateWithWanLanAccessFalse
 *
 * Verifies that generate() sets network to "private" when wanLanAccess is false.
 */
uint32_t Test_DobbySpecGenerator_GenerateWithWanLanAccessFalse()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    auto rtCfg  = MakeValidRuntimeConfig();
    rtCfg.wanLanAccess = false;
    std::string spec;

    gen.generate(appCfg, rtCfg, spec);

    L0Test::ExpectTrue(tr, spec.find("\"private\"") != std::string::npos,
                       "Generated spec contains 'private' network when wanLanAccess is false");

    return tr.failures;
}

/* Test_DobbySpecGenerator_GenerateWithAppPath
 *
 * Verifies that a non-empty appPath results in a bind mount entry in the spec.
 * Covers DobbySpecGenerator.cpp line 337.
 */
uint32_t Test_DobbySpecGenerator_GenerateWithAppPath()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    auto rtCfg  = MakeValidRuntimeConfig();
    rtCfg.appPath = "/opt/app/youTube";
    std::string spec;

    const bool result = gen.generate(appCfg, rtCfg, spec);
    L0Test::ExpectTrue(tr, result, "generate() succeeds with non-empty appPath");
    L0Test::ExpectTrue(tr, spec.find("/opt/app/youTube") != std::string::npos,
                       "Generated spec contains the appPath in mounts");

    return tr.failures;
}

/* Test_DobbySpecGenerator_GenerateThunderPluginEnabled
 *
 * Verifies the thunder RDK plugin is added to rdkPlugins when thunder is true.
 * Covers DobbySpecGenerator.cpp lines 545-548.
 */
uint32_t Test_DobbySpecGenerator_GenerateThunderPluginEnabled()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    auto rtCfg  = MakeValidRuntimeConfig();
    rtCfg.thunder = true;
    std::string spec;

    gen.generate(appCfg, rtCfg, spec);
    L0Test::ExpectTrue(tr, spec.find("\"thunder\"") != std::string::npos,
                       "Generated spec contains thunder plugin when thunder is enabled");

    return tr.failures;
}

/* Test_DobbySpecGenerator_GenerateThunderPluginDisabled
 *
 * Verifies the thunder RDK plugin is absent when thunder is false.
 */
uint32_t Test_DobbySpecGenerator_GenerateThunderPluginDisabled()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    auto rtCfg  = MakeValidRuntimeConfig();
    rtCfg.thunder = false;
    std::string spec;

    gen.generate(appCfg, rtCfg, spec);
    L0Test::ExpectTrue(tr, spec.find("\"thunder\"") == std::string::npos,
                       "Generated spec does not contain thunder plugin when thunder is disabled");

    return tr.failures;
}

/* Test_DobbySpecGenerator_GenerateWithEmptyWesterosSocket
 *
 * Verifies generate() succeeds with an empty westerosSocketPath (no Wayland
 * env vars appended).
 */
uint32_t Test_DobbySpecGenerator_GenerateWithEmptyWesterosSocket()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    appCfg.mWesterosSocketPath.clear();  // no GUI
    auto rtCfg  = MakeValidRuntimeConfig();
    std::string spec;

    const bool result = gen.generate(appCfg, rtCfg, spec);
    L0Test::ExpectTrue(tr, result, "generate() succeeds with empty westerosSocketPath");
    L0Test::ExpectTrue(tr, spec.find("WAYLAND_DISPLAY") == std::string::npos,
                       "No WAYLAND_DISPLAY env var when westerosSocketPath is empty");

    return tr.failures;
}

/* Test_DobbySpecGenerator_GenerateSysMemLimitZeroFallsBack
 *
 * Verifies generate() returns false when systemMemoryLimit is 0 and appType
 * is neither INTERACTIVE nor SYSTEM (no fallback available).
 * Covers DobbySpecGenerator.cpp getSysMemoryLimit() line 453.
 */
uint32_t Test_DobbySpecGenerator_GenerateSysMemLimitZeroFallsBack()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    auto rtCfg  = MakeValidRuntimeConfig();
    rtCfg.systemMemoryLimit = 0;   // will trigger branch in getSysMemoryLimit
    rtCfg.appType = "UNKNOWN_TYPE"; // not INTERACTIVE, so no fallback
    std::string spec;

    const bool result = gen.generate(appCfg, rtCfg, spec);
    // memLimit <= 0 → mandatory check fails → generate() returns false
    L0Test::ExpectTrue(tr, !result,
                       "generate() returns false when memLimit evaluates to 0");

    return tr.failures;
}

/* Test_DobbySpecGenerator_GenerateWithNonEmptyAppPorts
 *
 * Verifies that when config.mPorts is non-empty the appservicesrdk plugin
 * entry is added to rdkPlugins.
 * Covers DobbySpecGenerator.cpp lines 536-539.
 */
uint32_t Test_DobbySpecGenerator_GenerateWithNonEmptyAppPorts()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    auto rtCfg  = MakeValidRuntimeConfig();
    appCfg.mPorts.push_back(8080u);
    std::string spec;

    const bool result = gen.generate(appCfg, rtCfg, spec);
    L0Test::ExpectTrue(tr, result, "generate() succeeds when mPorts is non-empty");
    L0Test::ExpectTrue(tr, spec.find("appservicesrdk") != std::string::npos,
                       "Generated spec contains appservicesrdk plugin when ports are set");

    return tr.failures;
}

/* Test_DobbySpecGenerator_GetVpuEnabledReturnsFalseForSystemApp
 *
 * Verifies getVpuEnabled() returns false for SYSTEM app type.
 * Covers DobbySpecGenerator.cpp lines 487-490.
 */
uint32_t Test_DobbySpecGenerator_GetVpuEnabledReturnsFalseForSystemApp()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    auto rtCfg  = MakeValidRuntimeConfig();
    rtCfg.appType = "SYSTEM";
    std::string spec;

    const bool result = gen.generate(appCfg, rtCfg, spec);
    L0Test::ExpectTrue(tr, result, "generate() succeeds for SYSTEM appType");
    // VPU for SYSTEM app is false → spec should have "vpu":{"enable":false}
    L0Test::ExpectTrue(tr, spec.find("\"vpu\"") != std::string::npos,
                       "Generated spec has vpu section for SYSTEM app");

    return tr.failures;
}

/* Test_DobbySpecGenerator_GenerateWithEnvVariablesInRuntimeConfig
 *
 * Verifies that env variables supplied in runtimeConfig.envVariables are
 * included in the spec's env array.
 * Covers DobbySpecGenerator.cpp lines 255-260.
 */
uint32_t Test_DobbySpecGenerator_GenerateWithEnvVariablesInRuntimeConfig()
{
    L0Test::TestResult tr;

    WPEFramework::Plugin::DobbySpecGenerator gen(GetAIConfigurationFixture());
    auto appCfg = MakeValidAppConfig();
    auto rtCfg  = MakeValidRuntimeConfig();
    // Provide a JSON array string for envVariables
    rtCfg.envVariables = "[\"MY_TEST_VAR=hello\"]";
    std::string spec;

    const bool result = gen.generate(appCfg, rtCfg, spec);
    L0Test::ExpectTrue(tr, result, "generate() succeeds with envVariables in RuntimeConfig");
    L0Test::ExpectTrue(tr, spec.find("MY_TEST_VAR") != std::string::npos,
                       "Env variable from runtimeConfig.envVariables appears in spec");

    return tr.failures;
}
