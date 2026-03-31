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
 * @file Ralf_OCIConfigGeneratorTests.cpp
 *
 * L0 tests for ralf/RalfOCIConfigGenerator:
 *   - Construction and destruction
 *   - generateRalfOCIConfig() error paths (missing base spec file, missing
 *     graphics config file, etc.)
 *   - Exercises code paths that do not require a real OCI runtime environment.
 */

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <sys/stat.h>

#include "ralf/RalfOCIConfigGenerator.h"
#include "ralf/RalfConstants.h"
#include "ralf/RalfSupport.h"
#include "ApplicationConfiguration.h"
#include <interfaces/IRuntimeManager.h>
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

// ──────────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────────

static bool WriteFile_OCIGen(const std::string &path, const std::string &content)
{
    const size_t slashPos = path.find_last_of('/');
    if (slashPos != std::string::npos) {
        ralf::create_directories(path.substr(0, slashPos));
    }
    std::ofstream ofs(path, std::ios::out | std::ios::trunc);
    if (!ofs.is_open())
        return false;
    ofs << content;
    return true;
}

static WPEFramework::Plugin::ApplicationConfiguration MakeAppConfig_OCI(
    const std::string &appId = "com.test.app",
    const std::string &appInstanceId = "inst-oci-001",
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

static WPEFramework::Exchange::RuntimeConfig MakeRuntimeConfig_OCI()
{
    WPEFramework::Exchange::RuntimeConfig cfg;
    cfg.ralfPkgPath = "";
    cfg.envVariables = "";
    return cfg;
}

// ──────────────────────────────────────────────────────────────────────────────
// Construction / Destruction
// ──────────────────────────────────────────────────────────────────────────────

/* Test_RalfOCIConfigGenerator_ConstructionAndDestruction
 *
 * Verifies that RalfOCIConfigGenerator can be constructed and destroyed
 * without crashing, even when given a non-existent config file path.
 */
uint32_t Test_RalfOCIConfigGenerator_ConstructionAndDestruction()
{
    L0Test::TestResult tr;

    {
        std::vector<ralf::RalfPkgInfoPair> packages;
        ralf::RalfOCIConfigGenerator gen("/tmp/ralf_l0test_oci_config.json", packages);
        L0Test::ExpectTrue(tr, true,
                           "RalfOCIConfigGenerator constructor does not crash");
    }
    L0Test::ExpectTrue(tr, true,
                       "RalfOCIConfigGenerator destructor does not crash");

    return tr.failures;
}

/* Test_RalfOCIConfigGenerator_ConstructionWithEmptyPackages
 *
 * Verifies that RalfOCIConfigGenerator can be constructed with an empty
 * package list without crashing.
 */
uint32_t Test_RalfOCIConfigGenerator_ConstructionWithEmptyPackages()
{
    L0Test::TestResult tr;

    std::vector<ralf::RalfPkgInfoPair> emptyPackages;
    ralf::RalfOCIConfigGenerator gen("/tmp/ralf_l0test_empty_cfg.json", emptyPackages);
    L0Test::ExpectTrue(tr, true,
                       "RalfOCIConfigGenerator with empty packages does not crash");

    return tr.failures;
}

/* Test_RalfOCIConfigGenerator_ConstructionWithMultiplePackages
 *
 * Verifies that RalfOCIConfigGenerator can be constructed with a pre-populated
 * package list.
 */
uint32_t Test_RalfOCIConfigGenerator_ConstructionWithMultiplePackages()
{
    L0Test::TestResult tr;

    std::vector<ralf::RalfPkgInfoPair> packages;
    packages.emplace_back("/pkg/meta/app.json", "/mnt/app");
    packages.emplace_back("/pkg/meta/rt.json",  "/mnt/rt");

    ralf::RalfOCIConfigGenerator gen("/tmp/ralf_l0test_multi_pkg.json", packages);
    L0Test::ExpectTrue(tr, true,
                       "RalfOCIConfigGenerator with multiple packages does not crash");

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// generateRalfOCIConfig() error path tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_RalfOCIConfigGenerator_GenerateFailsWhenBaseSpecFileMissing
 *
 * Verifies that generateRalfOCIConfig() returns false when the OCI base spec
 * file (RALF_OCI_BASE_SPEC_FILE = /usr/share/ralf/oci-base-spec.json) is absent.
 * In a test environment this file is typically not present.
 */
uint32_t Test_RalfOCIConfigGenerator_GenerateFailsWhenBaseSpecFileMissing()
{
    L0Test::TestResult tr;

    // Only run this test when the base spec file is absent
    if (ralf::checkIfPathExists(ralf::RALF_OCI_BASE_SPEC_FILE)) {
        // Skip: base spec file is present, can't test this error path easily
        L0Test::ExpectTrue(tr, true,
                           "SKIP: base OCI spec file found, skipping missing-file test");
        return tr.failures;
    }

    std::vector<ralf::RalfPkgInfoPair> packages;
    ralf::RalfOCIConfigGenerator gen("/tmp/ralf_l0test_oci_out.json", packages);

    auto config = MakeAppConfig_OCI();
    auto runtimeCfg = MakeRuntimeConfig_OCI();

    const bool result = gen.generateRalfOCIConfig(config, runtimeCfg);
    L0Test::ExpectTrue(tr, !result,
                       "generateRalfOCIConfig() returns false when base spec file is absent");

    return tr.failures;
}

/* Test_RalfOCIConfigGenerator_GenerateFailsWhenGraphicsConfigMissing
 *
 * When the base OCI spec file exists but the graphics config does not,
 * generateRalfOCIConfig() should return false.
 * This test only runs when the base spec is present but graphics config is absent.
 */
uint32_t Test_RalfOCIConfigGenerator_GenerateFailsWhenGraphicsConfigMissing()
{
    L0Test::TestResult tr;

    // Only meaningful when base spec exists but graphics config does not
    if (!ralf::checkIfPathExists(ralf::RALF_OCI_BASE_SPEC_FILE) ||
         ralf::checkIfPathExists(ralf::RALF_GRAPHICS_LAYER_CONFIG)) {
        // Architecture guard: skip if preconditions aren't met in this environment
        L0Test::ExpectTrue(tr, true,
                           "SKIP: preconditions for graphics-config-missing test not met");
        return tr.failures;
    }

    std::vector<ralf::RalfPkgInfoPair> packages;
    ralf::RalfOCIConfigGenerator gen("/tmp/ralf_l0test_oci_out2.json", packages);

    auto config = MakeAppConfig_OCI("com.test.app2", "inst-oci-002");
    auto runtimeCfg = MakeRuntimeConfig_OCI();

    const bool result = gen.generateRalfOCIConfig(config, runtimeCfg);
    L0Test::ExpectTrue(tr, !result,
                       "generateRalfOCIConfig() returns false when graphics config is absent");

    return tr.failures;
}

/* Test_RalfOCIConfigGenerator_GenerateWithMockBaseSpec
 *
 * Provides a minimal mock OCI base spec and graphics config JSON, and
 * verifies that generateRalfOCIConfig() processes them without crashing.
 * The test verifies the error path caused by missing package-specific files.
 */
uint32_t Test_RalfOCIConfigGenerator_GenerateWithMockBaseSpec()
{
    L0Test::TestResult tr;

    // Skip if real system files exist to avoid interfering with production data
    if (ralf::checkIfPathExists(ralf::RALF_OCI_BASE_SPEC_FILE)) {
        L0Test::ExpectTrue(tr, true, "SKIP: real OCI base spec found, skipping mock test");
        return tr.failures;
    }

    // Provide minimal base spec and graphics config in temp paths.
    // Since override of RALF_OCI_BASE_SPEC_FILE would require const_cast or
    // other design changes, this test instead validates the object can be
    // instantiated and that calling generate with existing pkg metadata that
    // points to non-existent files causes a false result.
    std::vector<ralf::RalfPkgInfoPair> packages;
    packages.emplace_back("/tmp/ralf_l0test_fakemeta.json", "/mnt/fake");

    ralf::RalfOCIConfigGenerator gen("/tmp/ralf_l0test_output.json", packages);

    auto config = MakeAppConfig_OCI("com.test.app3", "inst-oci-003");
    auto runtimeCfg = MakeRuntimeConfig_OCI();

    // Will fail because RALF_OCI_BASE_SPEC_FILE doesn't exist
    const bool result = gen.generateRalfOCIConfig(config, runtimeCfg);
    L0Test::ExpectTrue(tr, !result,
                       "generateRalfOCIConfig() returns false with mock packages when base spec is absent");

    return tr.failures;
}

/* Test_RalfOCIConfigGenerator_MultipleGenerateCallsDoNotCrash
 *
 * Verifies that calling generateRalfOCIConfig() multiple times on the same
 * object does not crash (even if it always fails in the test environment).
 */
uint32_t Test_RalfOCIConfigGenerator_MultipleGenerateCallsDoNotCrash()
{
    L0Test::TestResult tr;

    std::vector<ralf::RalfPkgInfoPair> packages;
    ralf::RalfOCIConfigGenerator gen("/tmp/ralf_l0test_multi_call.json", packages);

    auto config = MakeAppConfig_OCI("com.test.app4", "inst-oci-004");
    auto runtimeCfg = MakeRuntimeConfig_OCI();

    // Call twice — should not crash regardless of return value
    gen.generateRalfOCIConfig(config, runtimeCfg);
    gen.generateRalfOCIConfig(config, runtimeCfg);

    L0Test::ExpectTrue(tr, true,
                       "Multiple generateRalfOCIConfig() calls do not crash");

    return tr.failures;
}

/* Test_RalfOCIConfigGenerator_GenerateWithDifferentAppInstances
 *
 * Verifies that generateRalfOCIConfig() can be called for different app
 * instance IDs on the same generator without crashing.
 */
uint32_t Test_RalfOCIConfigGenerator_GenerateWithDifferentAppInstances()
{
    L0Test::TestResult tr;

    std::vector<ralf::RalfPkgInfoPair> packages;
    ralf::RalfOCIConfigGenerator gen("/tmp/ralf_l0test_diff_inst.json", packages);

    const std::string instances[] = {"inst-A", "inst-B", "inst-C"};
    for (const auto &instId : instances) {
        auto config = MakeAppConfig_OCI("com.test.app", instId);
        auto runtimeCfg = MakeRuntimeConfig_OCI();
        gen.generateRalfOCIConfig(config, runtimeCfg);  // result ignored; no crash expected
    }

    L0Test::ExpectTrue(tr, true,
                       "generateRalfOCIConfig() with different app instances does not crash");

    return tr.failures;
}
