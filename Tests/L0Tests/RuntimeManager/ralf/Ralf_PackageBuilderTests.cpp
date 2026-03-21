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
 * @file Ralf_PackageBuilderTests.cpp
 *
 * L0 tests for ralf/RalfPackageBuilder:
 *   - Construction and destruction
 *   - unmountOverlayfsIfExists() with non-existent paths
 *   - generateRalfDobbySpec() error paths (missing/invalid pkg files)
 */

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <cstdlib>

#include "ralf/RalfPackageBuilder.h"
#include "ralf/RalfSupport.h"
#include "ApplicationConfiguration.h"
#include <interfaces/IRuntimeManager.h>
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

// ──────────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────────

static bool WriteFile_PB(const std::string &path, const std::string &content)
{
    const size_t slashPos = path.find_last_of('/');
    if (slashPos != std::string::npos) {
        const std::string dir = path.substr(0, slashPos);
        ralf::create_directories(dir);
    }
    std::ofstream ofs(path, std::ios::out | std::ios::trunc);
    if (!ofs.is_open())
        return false;
    ofs << content;
    return true;
}

static WPEFramework::Plugin::ApplicationConfiguration MakeAppConfig(
    const std::string &appId,
    const std::string &appInstanceId,
    uint32_t uid = 1000,
    uint32_t gid = 1000)
{
    WPEFramework::Plugin::ApplicationConfiguration config;
    config.mAppId = appId;
    config.mAppInstanceId = appInstanceId;
    config.mUserId = uid;
    config.mGroupId = gid;
    return config;
}

static WPEFramework::Exchange::RuntimeConfig MakeRuntimeConfig(const std::string &ralfPkgPath = "")
{
    WPEFramework::Exchange::RuntimeConfig cfg;
    cfg.ralfPkgPath = ralfPkgPath;
    return cfg;
}

// ──────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ──────────────────────────────────────────────────────────────────────────────

/* Test_RalfPackageBuilder_ConstructionAndDestruction
 *
 * Verifies that RalfPackageBuilder can be constructed and destroyed without crash.
 */
uint32_t Test_RalfPackageBuilder_ConstructionAndDestruction()
{
    L0Test::TestResult tr;

    {
        ralf::RalfPackageBuilder builder;
        L0Test::ExpectTrue(tr, true,
                           "RalfPackageBuilder constructor does not crash");
    }
    L0Test::ExpectTrue(tr, true,
                       "RalfPackageBuilder destructor does not crash");

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// unmountOverlayfsIfExists() tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_RalfPackageBuilder_UnmountOverlayfsIfExists_NonExistentPathReturnsFalse
 *
 * Verifies that unmountOverlayfsIfExists() returns false when the overlayfs
 * mount path (RALF_APP_ROOTFS_DIR/<appInstanceId>/rootfs) does not exist.
 */
uint32_t Test_RalfPackageBuilder_UnmountOverlayfsIfExists_NonExistentPathReturnsFalse()
{
    L0Test::TestResult tr;

    ralf::RalfPackageBuilder builder;
    // Use an appInstanceId whose rootfs mount will not exist
    const bool result = builder.unmountOverlayfsIfExists("ralf_l0test_never_exists_xyz");
    L0Test::ExpectTrue(tr, !result,
                       "unmountOverlayfsIfExists() returns false when path does not exist");

    return tr.failures;
}

/* Test_RalfPackageBuilder_UnmountOverlayfsIfExists_EmptyAppInstanceId
 *
 * Verifies that unmountOverlayfsIfExists() returns false for an empty instance ID.
 */
uint32_t Test_RalfPackageBuilder_UnmountOverlayfsIfExists_EmptyAppInstanceId()
{
    L0Test::TestResult tr;

    ralf::RalfPackageBuilder builder;
    const bool result = builder.unmountOverlayfsIfExists("");
    // Empty ID produces path "/tmp/ralf//rootfs" which should not exist
    L0Test::ExpectTrue(tr, !result,
                       "unmountOverlayfsIfExists() returns false for empty appInstanceId");

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// generateRalfDobbySpec() error path tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_RalfPackageBuilder_GenerateRalfDobbySpec_EmptyRalfPkgPathReturnsFalse
 *
 * Verifies that generateRalfDobbySpec() returns false when ralfPkgPath is empty
 * (parseRalPkgInfo will fail to open the file).
 */
uint32_t Test_RalfPackageBuilder_GenerateRalfDobbySpec_EmptyRalfPkgPathReturnsFalse()
{
    L0Test::TestResult tr;

    ralf::RalfPackageBuilder builder;
    auto config = MakeAppConfig("com.test.app", "inst-001");
    auto runtimeCfg = MakeRuntimeConfig(""); // empty path

    std::string dobbySpec;
    const bool result = builder.generateRalfDobbySpec(config, runtimeCfg, dobbySpec);
    L0Test::ExpectTrue(tr, !result,
                       "generateRalfDobbySpec() returns false for empty ralfPkgPath");

    return tr.failures;
}

/* Test_RalfPackageBuilder_GenerateRalfDobbySpec_NonExistentPkgFileReturnsFalse
 *
 * Verifies that generateRalfDobbySpec() returns false when the package file
 * does not exist on disk.
 */
uint32_t Test_RalfPackageBuilder_GenerateRalfDobbySpec_NonExistentPkgFileReturnsFalse()
{
    L0Test::TestResult tr;

    ralf::RalfPackageBuilder builder;
    auto config = MakeAppConfig("com.test.app", "inst-002");
    auto runtimeCfg = MakeRuntimeConfig("/tmp/ralf_l0test_nonexistent_pkg_cfg.json");

    std::string dobbySpec;
    const bool result = builder.generateRalfDobbySpec(config, runtimeCfg, dobbySpec);
    L0Test::ExpectTrue(tr, !result,
                       "generateRalfDobbySpec() returns false when pkg file is missing");

    return tr.failures;
}

/* Test_RalfPackageBuilder_GenerateRalfDobbySpec_MalformedPkgFileReturnsFalse
 *
 * Verifies that generateRalfDobbySpec() returns false when the pkg config
 * file is malformed JSON.
 */
uint32_t Test_RalfPackageBuilder_GenerateRalfDobbySpec_MalformedPkgFileReturnsFalse()
{
    L0Test::TestResult tr;

    const std::string pkgFile = "/tmp/ralf_l0test_malformed_pkg.json";
    WriteFile_PB(pkgFile, "{ not valid json !!");

    ralf::RalfPackageBuilder builder;
    auto config = MakeAppConfig("com.test.app", "inst-003");
    auto runtimeCfg = MakeRuntimeConfig(pkgFile);

    std::string dobbySpec;
    const bool result = builder.generateRalfDobbySpec(config, runtimeCfg, dobbySpec);
    L0Test::ExpectTrue(tr, !result,
                       "generateRalfDobbySpec() returns false for malformed pkg JSON");

    ::remove(pkgFile.c_str());

    return tr.failures;
}

/* Test_RalfPackageBuilder_GenerateRalfDobbySpec_PkgFileMissingPackagesFieldReturnsFalse
 *
 * Verifies that generateRalfDobbySpec() returns false when the pkg config
 * JSON does not contain the expected 'packages' field.
 */
uint32_t Test_RalfPackageBuilder_GenerateRalfDobbySpec_PkgFileMissingPackagesFieldReturnsFalse()
{
    L0Test::TestResult tr;

    const std::string pkgFile = "/tmp/ralf_l0test_nopkg_field.json";
    WriteFile_PB(pkgFile, "{\"other\": \"data\"}");

    ralf::RalfPackageBuilder builder;
    auto config = MakeAppConfig("com.test.app", "inst-004");
    auto runtimeCfg = MakeRuntimeConfig(pkgFile);

    std::string dobbySpec;
    const bool result = builder.generateRalfDobbySpec(config, runtimeCfg, dobbySpec);
    L0Test::ExpectTrue(tr, !result,
                       "generateRalfDobbySpec() returns false when 'packages' field missing");

    ::remove(pkgFile.c_str());

    return tr.failures;
}

/* Test_RalfPackageBuilder_GenerateRalfDobbySpec_OutputDobbySpecEmptyOnFailure
 *
 * Verifies that on failure the dobbySpec output string is not modified to a
 * non-empty value (remains empty when generation fails).
 */
uint32_t Test_RalfPackageBuilder_GenerateRalfDobbySpec_OutputDobbySpecEmptyOnFailure()
{
    L0Test::TestResult tr;

    ralf::RalfPackageBuilder builder;
    auto config = MakeAppConfig("com.test.app", "inst-005");
    auto runtimeCfg = MakeRuntimeConfig("/tmp/ralf_l0test_none.json");

    std::string dobbySpec;
    builder.generateRalfDobbySpec(config, runtimeCfg, dobbySpec);

    // On failure, dobbySpec should remain empty
    L0Test::ExpectTrue(tr, dobbySpec.empty(),
                       "dobbySpec output remains empty when generateRalfDobbySpec() fails");

    return tr.failures;
}
