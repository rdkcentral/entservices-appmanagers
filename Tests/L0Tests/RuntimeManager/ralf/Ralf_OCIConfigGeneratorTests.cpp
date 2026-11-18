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
#include <iterator>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <glob.h>
#include <sys/stat.h>
#include <unistd.h>
#include <json/json.h>

#define private public
#include "ralf/RalfOCIConfigGenerator.h"
#undef private
#include "ralf/RalfConstants.h"
#include "ralf/RalfSupport.h"
#include "ralf/OCISpecConstants.h"
#include "ApplicationConfiguration.h"
#include "DobbySpecGenerator.h"
#include "RuntimeConfiguration.h"
#include "AIConfiguration.h"
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

static WPEFramework::Plugin::RuntimeConfiguration MakeRuntimeConfig_OCI()
{
    WPEFramework::Plugin::RuntimeConfiguration cfg;
    cfg.ralfPkgPath = "";
    cfg.envVariables = {};
    return cfg;
}

static bool HasEnvEntry_OCIGen(const Json::Value& ociConfigRootNode, const std::string& expectedEntry)
{
    const Json::Value& envNode = ociConfigRootNode[ralf::PROCESS][ralf::ENV];
    if (!envNode.isArray()) {
        return false;
    }

    for (const auto& entry : envNode) {
        if (entry.isString() && expectedEntry == entry.asString()) {
            return true;
        }
    }

    return false;
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

/* Test_RalfOCIConfigGenerator_GenerateWithMockBaseSpec
 *
 * Provides a minimal mock OCI base spec and empty graphics config, and
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

/* Test_RalfOCIConfigGenerator_LogPathSetCorrectlyInOCIConfig
 *
 * Verifies that generateRalfOCIConfig() correctly sets
 * rdkPlugins.logging.data.fileOptions.path to <appStoragePath>/<appId>.log
 * in the generated OCI config JSON.
 *
 * This test writes minimal mock JSON files to the expected system paths when
 * those paths do not already exist, then verifies the generated output, and
 * cleans up afterwards.
 */
uint32_t Test_RalfOCIConfigGenerator_LogPathSetCorrectlyInOCIConfig()
{
    L0Test::TestResult tr;

    // Skip if the system base spec is already present so production files are not overwritten.
    if (ralf::checkIfPathExists(ralf::RALF_OCI_BASE_SPEC_FILE)) {
        L0Test::ExpectTrue(tr, true,
                           "SKIP: system OCI base spec present, skipping log path test");
        return tr.failures;
    }

    const std::string outputConfigPath = "/tmp/ralf_l0test_logpath_config.json";
    const std::string appStoragePath   = "/tmp/ralf_l0test_appstorage";
    const std::string appId            = "com.test.logpathapp";
    const std::string expectedLogPath  = appStoragePath + "/" + appId + ".log";

    // Write a minimal valid OCI base spec. GPU configuration is passed in-memory.
    const bool baseSpecWritten = WriteFile_OCIGen(ralf::RALF_OCI_BASE_SPEC_FILE, "{}");
    if (!baseSpecWritten) {
        std::remove(ralf::RALF_OCI_BASE_SPEC_FILE.c_str());
        L0Test::ExpectTrue(tr, true,
                           "SKIP: could not write mock OCI spec files, skipping log path test");
        return tr.failures;
    }

    WPEFramework::Plugin::ApplicationConfiguration config;
    config.mAppId               = appId;
    config.mAppInstanceId       = "inst-logpath-001";
    config.mUserId              = static_cast<uint32_t>(getuid());
    config.mGroupId             = static_cast<uint32_t>(getgid());
    config.mAppStorageInfo.path = appStoragePath;

    auto runtimeCfg = MakeRuntimeConfig_OCI();

    std::vector<ralf::RalfPkgInfoPair> packages;
    ralf::RalfOCIConfigGenerator gen(outputConfigPath, packages);

    const bool result = gen.generateRalfOCIConfig(config, runtimeCfg, Json::Value(Json::objectValue));

    // Remove the mock system file before asserting, so cleanup always happens.
    std::remove(ralf::RALF_OCI_BASE_SPEC_FILE.c_str());

    L0Test::ExpectTrue(tr, result,
                       "generateRalfOCIConfig() should succeed with minimal mock base spec");

    if (result) {
        struct stat configStat;
        L0Test::ExpectTrue(tr, 0 == stat(outputConfigPath.c_str(), &configStat) &&
                           0644 == (configStat.st_mode & 07777) && geteuid() == configStat.st_uid,
                           "Generated OCI config is process-owned with legacy 0644 mode");

        Json::Value outputConfig;
        const bool parsed = ralf::JsonFromFile(outputConfigPath, outputConfig);
        L0Test::ExpectTrue(tr, parsed,
                           "Generated OCI config should be parseable JSON");

        if (parsed) {
            const bool hasLogPath = outputConfig.isMember("rdkPlugins") &&
                                    outputConfig["rdkPlugins"].isMember("logging") &&
                                    outputConfig["rdkPlugins"]["logging"].isMember("data") &&
                                    outputConfig["rdkPlugins"]["logging"]["data"].isMember("fileOptions") &&
                                    outputConfig["rdkPlugins"]["logging"]["data"]["fileOptions"].isMember("path");
            L0Test::ExpectTrue(tr, hasLogPath,
                               "Generated OCI config should contain rdkPlugins.logging.data.fileOptions.path");
            if (hasLogPath) {
                const std::string actualPath =
                    outputConfig["rdkPlugins"]["logging"]["data"]["fileOptions"]["path"].asString();
                L0Test::ExpectTrue(tr, actualPath == expectedLogPath,
                                   "rdkPlugins.logging.data.fileOptions.path should equal <appStoragePath>/<appId>.log");
            }
        }
        std::remove(outputConfigPath.c_str());
    }

    return tr.failures;
}

uint32_t Test_RalfOCIConfigGenerator_UsesConfiguredRialtoSocketPath()
{
    L0Test::TestResult tr;

    std::vector<ralf::RalfPkgInfoPair> packages;
    ralf::RalfOCIConfigGenerator gen("/tmp/ralf_l0test_rialto_config.json", packages);
    auto config = MakeAppConfig_OCI();
    auto runtimeCfg = MakeRuntimeConfig_OCI();
    config.mRialtoSocketPath = "/tmp/ralf_l0test_rialto.sock";

    Json::Value ociConfig(Json::objectValue);
    const bool status = gen.applyRuntimeAndAppConfigToOCIConfig(ociConfig, runtimeCfg, config);
    L0Test::ExpectTrue(tr, status,
                       "applyRuntimeAndAppConfigToOCIConfig() accepts configured Rialto socket path");
    L0Test::ExpectTrue(tr, HasEnvEntry_OCIGen(ociConfig,
                       "RIALTO_SOCKET_PATH=" + config.mRialtoSocketPath),
                       "RIALTO_SOCKET_PATH uses the path allocated by RialtoConnector");

    bool hasSocketMount = false;
    const Json::Value& mounts = ociConfig[ralf::MOUNTS];
    if (mounts.isArray())
    {
        for (const Json::Value& mount : mounts)
        {
            if (mount[ralf::TYPE].asString() == "bind" &&
                mount[ralf::SOURCE].asString() == config.mRialtoSocketPath &&
                mount[ralf::DESTINATION].asString() == config.mRialtoSocketPath)
            {
                hasSocketMount = true;
                break;
            }
        }
    }
    L0Test::ExpectTrue(tr, hasSocketMount,
                       "Rialto bind mount uses the allocated host socket path");

    return tr.failures;
}

uint32_t Test_RalfOCIConfigGenerator_LegacyProcessArgsFollowBaseRuntimeAppOrder()
{
    L0Test::TestResult tr;
    std::vector<ralf::RalfPkgInfoPair> packages;
    ralf::RalfOCIConfigGenerator gen("/tmp/ralf_l0test_process_args.json", packages);
    Json::Value base(Json::objectValue);
    base[ralf::PACKAGE_TYPE] = ralf::PKG_TYPE_BASE;
    base[ralf::ENTRY_POINT] = "/usr/libexec/DobbyInit";
    base[ralf::ENTRY_ARGS].append("ignored-base-arg");
    Json::Value runtime(Json::objectValue);
    runtime[ralf::ENTRY_POINT] = "/usr/bin/browser";
    runtime[ralf::ENTRY_ARGS].append("ignored-runtime-arg");
    Json::Value app(Json::objectValue);
    app[ralf::ENTRY_POINT] = ".";
    app[ralf::ENTRY_ARGS].append("--app");
    Json::Value ociConfig(Json::objectValue);

    L0Test::ExpectTrue(tr, gen.setLegacyProcessArgs(ociConfig, {base, runtime, app}),
                       "base, runtime and app entries form a legacy command");
    const Json::Value &args = ociConfig[ralf::PROCESS][ralf::ARGS];
    L0Test::ExpectTrue(tr, args.isArray() && 4 == args.size() &&
                       "/usr/libexec/DobbyInit" == args[0].asString() &&
                       "/usr/bin/browser" == args[1].asString() &&
                       "." == args[2].asString() && "--app" == args[3].asString(),
                       "only app arguments follow the base init, runtime and app entrypoints");
    return tr.failures;
}

uint32_t Test_RalfOCIConfigGenerator_LegacyProcessArgsRejectMissingBase()
{
    L0Test::TestResult tr;
    std::vector<ralf::RalfPkgInfoPair> packages;
    ralf::RalfOCIConfigGenerator gen("/tmp/ralf_l0test_no_base.json", packages);
    Json::Value runtime(Json::objectValue);
    runtime[ralf::ENTRY_POINT] = "/usr/bin/browser";
    Json::Value app(Json::objectValue);
    app[ralf::ENTRY_POINT] = ".";
    Json::Value ociConfig(Json::objectValue);

    L0Test::ExpectTrue(tr, !gen.setLegacyProcessArgs(ociConfig, {runtime, app}),
                       "two-layer manifest without a base init is rejected before Dobby");
    return tr.failures;
}

uint32_t Test_RalfOCIConfigGenerator_LegacyProcessArgsAllowAppWithoutEntryPoint()
{
    L0Test::TestResult tr;
    std::vector<ralf::RalfPkgInfoPair> packages;
    ralf::RalfOCIConfigGenerator gen("/tmp/ralf_l0test_widget_layer.json", packages);
    Json::Value base(Json::objectValue);
    base[ralf::PACKAGE_TYPE] = ralf::PKG_TYPE_BASE;
    base[ralf::ENTRY_POINT] = "/usr/libexec/DobbyInit";
    Json::Value runtime(Json::objectValue);
    runtime[ralf::ENTRY_POINT] = "bin/cobalt";
    Json::Value widget(Json::objectValue);
    widget[ralf::PACKAGE_TYPE] = ralf::PKG_TYPE_APPLICATION;
    Json::Value ociConfig(Json::objectValue);

    L0Test::ExpectTrue(tr, gen.setLegacyProcessArgs(ociConfig, {base, runtime, widget}),
                       "widget layer without entryPoint launches through the runtime");
    const Json::Value &args = ociConfig[ralf::PROCESS][ralf::ARGS];
    L0Test::ExpectTrue(tr, args.isArray() && 2 == args.size() &&
                       "/usr/libexec/DobbyInit" == args[0].asString() &&
                       "bin/cobalt" == args[1].asString(),
                       "command is base init followed by the runtime executable");

    Json::Value bareRuntime(Json::objectValue);
    Json::Value emptyConfig(Json::objectValue);
    L0Test::ExpectTrue(tr, !gen.setLegacyProcessArgs(emptyConfig, {base, bareRuntime, widget}),
                       "base init alone is rejected");
    return tr.failures;
}

#ifdef ENTOS_RALF_SUPPORT
uint32_t Test_Ralf_GetClassicRalfLaunchInfo()
{
    L0Test::TestResult tr;
    const std::string runtimeRoot = "/tmp/ralf_l0test_widget_runtime";
    const std::string appRoot = "/tmp/ralf_l0test_widget_app";
    const std::string manifestPath = "/tmp/ralf_l0test_widget_manifest.json";
    L0Test::ExpectTrue(tr, WriteFile_OCIGen(runtimeRoot + "/config.json",
                       "{\"packageType\":\"runtime\",\"entryPoint\":\"bin/browser\"}") &&
                       WriteFile_OCIGen(appRoot + "/config.json",
                                         "{\"packageType\":\"application\",\"entryPoint\":\".\",\"entryArgs\":[\"--profile\",\"tv\"],\"runtimeSecrets\":{\"applicationSecret\":\"app-secret\",\"credentialsManager\":{\"certificate\":\"cert\",\"privateKey\":\"key\"}}}"),
                       "runtime and RALF app configs are available");

    Json::Value manifest(Json::objectValue);
    Json::Value runtime(Json::objectValue);
    runtime["pkgMetaDataPath"] = runtimeRoot + "/config.json";
    runtime["pkgMountPath"] = runtimeRoot;
    manifest["packages"].append(runtime);
    Json::Value app(Json::objectValue);
    app["pkgMetaDataPath"] = appRoot + "/config.json";
    app["pkgMountPath"] = appRoot;
    manifest["packages"].append(app);
    Json::StreamWriterBuilder writer;
    L0Test::ExpectTrue(tr, WriteFile_OCIGen(manifestPath, Json::writeString(writer, manifest)),
                       "dependency-first mount manifest is available");

    std::string appPath;
    std::string runtimePath;
    std::string command;
    std::vector<std::string> commandArgs;
    std::string applicationSecret;
    std::string credentialsCertificate;
    std::string credentialsPrivateKey;
    L0Test::ExpectTrue(tr, ralf::getClassicRalfLaunchInfo(manifestPath, appPath, runtimePath, command,
                       commandArgs, applicationSecret, credentialsCertificate, credentialsPrivateKey),
                       "runtime and RALF app select the classic launch path");
    L0Test::ExpectTrue(tr, appPath == appRoot && runtimePath == runtimeRoot &&
                       command == "bin/browser",
                       "classic spec receives app mount, runtime mount and executable");
    L0Test::ExpectTrue(tr, commandArgs.size() == 2 && "--profile" == commandArgs[0] && "tv" == commandArgs[1],
                       "classic launch preserves application entryArgs");
    L0Test::ExpectTrue(tr, applicationSecret == "app-secret" && credentialsCertificate == "cert" &&
                       credentialsPrivateKey == "key",
                       "classic launch preserves validated package secrets");

    L0Test::ExpectTrue(tr, WriteFile_OCIGen(runtimeRoot + "/config.json",
                       "{\"packageType\":\"runtime\",\"entryPoint\":\"\"}"),
                       "runtime without an executable config is available");
    L0Test::ExpectTrue(tr, WriteFile_OCIGen(appRoot + "/config.json",
                       "{\"packageType\":\"application\",\"entryPoint\":\"bin/app\"}"),
                       "application executable config is available");
    L0Test::ExpectTrue(tr, ralf::getClassicRalfLaunchInfo(manifestPath, appPath, runtimePath, command),
                       "base-free runtime without an executable still selects classic launch");
    L0Test::ExpectTrue(tr, appPath == appRoot && runtimePath.empty() && command == "bin/app",
                       "classic spec uses the app executable and omits the runtime mount");

    L0Test::ExpectTrue(tr, WriteFile_OCIGen(runtimeRoot + "/config.json",
                       "{\"packageType\":\"runtime\"}"),
                       "runtime with no entryPoint field is available");
    L0Test::ExpectTrue(tr, ralf::getClassicRalfLaunchInfo(manifestPath, appPath, runtimePath, command),
                       "runtime with no entryPoint uses the app executable");
    L0Test::ExpectTrue(tr, runtimePath.empty() && command == "bin/app",
                       "non-executable runtime is not bind-mounted");

    L0Test::ExpectTrue(tr, WriteFile_OCIGen(runtimeRoot + "/config.json",
                       "{\"packageType\":\"application\",\"entryPoint\":\"bin/widget\"}"),
                       "non-runtime dependency config is available");
    L0Test::ExpectTrue(tr, !ralf::getClassicRalfLaunchInfo(manifestPath, appPath, runtimePath, command),
                       "non-runtime ZIP layer is not accepted as a runtime");

    L0Test::ExpectTrue(tr, WriteFile_OCIGen(appRoot + "/config.json",
                       "{\"packageType\":\"application\",\"entryPoint\":\"bin/app\"}"),
                       "app-only RALF config is available");
    manifest["packages"].clear();
    manifest["packages"].append(app);
    L0Test::ExpectTrue(tr, WriteFile_OCIGen(manifestPath, Json::writeString(writer, manifest)),
                       "app-only manifest is available");
    L0Test::ExpectTrue(tr, ralf::getClassicRalfLaunchInfo(manifestPath, appPath, runtimePath, command),
                       "base-free RALF app selects the classic launch path");
    L0Test::ExpectTrue(tr, appPath == appRoot && runtimePath.empty() && command == "bin/app",
                       "app-only classic launch uses the app root and entrypoint");

    std::remove((runtimeRoot + "/config.json").c_str());
    std::remove((appRoot + "/config.json").c_str());
    std::remove(manifestPath.c_str());
    ::rmdir(runtimeRoot.c_str());
    ::rmdir(appRoot.c_str());
    return tr.failures;
}
#endif

uint32_t Test_DobbySpecGenerator_GenerateClassicRalfArgsAndSecrets()
{
    L0Test::TestResult tr;
    WPEFramework::Plugin::AIConfiguration aiConfiguration;
    WPEFramework::Plugin::DobbySpecGenerator generator(aiConfiguration);
    WPEFramework::Plugin::ApplicationConfiguration appConfig;
    appConfig.mAppId = "com.example.classic";
    appConfig.mAppInstanceId = "classic-001";
    appConfig.mUserId = 1000;
    appConfig.mGroupId = 1000;
    appConfig.mAppStorageInfo.path = "/tmp/classic-ralf-app-storage";
    WPEFramework::Plugin::RuntimeConfiguration runtimeConfig;
    runtimeConfig.ralfPkgPath = "/tmp/classic-ralf-manifest.json";
    runtimeConfig.envVariables = { "PERSIST_STORAGE_PATH=/override" };
    runtimeConfig.command = "bin/app";
    runtimeConfig.commandArgs = {"--profile", "tv"};
    runtimeConfig.systemMemoryLimit = 128 * 1024 * 1024;
    runtimeConfig.applicationSecret = "test-secret";
    runtimeConfig.credentialsCertificate = "test-certificate";
    runtimeConfig.credentialsPrivateKey = "test-private-key";

    std::string serializedSpec;
    L0Test::ExpectTrue(tr, generator.generate(appConfig, runtimeConfig, serializedSpec),
                       "Generate classic Dobby spec with RALF arguments and secrets");
    Json::Value spec;
    Json::Reader reader;
    const bool parsed = reader.parse(serializedSpec, spec);
    L0Test::ExpectTrue(tr, parsed, "Parse generated classic Dobby spec");
    if (parsed)
    {
        const Json::Value &args = spec["args"];
        L0Test::ExpectTrue(tr, args.isArray() && 3 == args.size() &&
                           "/package/bin/app" == args[0].asString() &&
                           "--profile" == args[1].asString() && "tv" == args[2].asString(),
                           "Preserve classic RALF entryArgs after executable");
        bool foundSecret = false;
        for (const Json::Value &env : spec["env"])
        {
            foundSecret = foundSecret || "APPLICATION_SECRET=test-secret" == env.asString();
        }
        L0Test::ExpectTrue(tr, foundSecret, "Preserve classic RALF application secret");
        bool foundStorageMount = false;
        for (const Json::Value &mount : spec["mounts"])
        {
            foundStorageMount = foundStorageMount ||
                ("/tmp/classic-ralf-app-storage" == mount["source"].asString() &&
                 "/data" == mount["destination"].asString() && "bind" == mount["type"].asString());
        }
        L0Test::ExpectTrue(tr, foundStorageMount,
                           "Classic RALF mounts AppStorageManager storage at /data");
        bool foundStorageEnvironment = false;
        bool foundOverride = false;
        for (const Json::Value &env : spec["env"])
        {
            foundStorageEnvironment = foundStorageEnvironment || "PERSIST_STORAGE_PATH=/data" == env.asString();
            foundOverride = foundOverride || "PERSIST_STORAGE_PATH=/override" == env.asString();
        }
        L0Test::ExpectTrue(tr, foundStorageEnvironment && !foundOverride,
                           "Classic RALF exports the mounted storage path instead of a generic override");
        L0Test::ExpectTrue(tr,
                           "test-certificate" == spec["rdkPlugins"]["credentialsmanager"]["data"]["certificate"].asString() &&
                           "test-private-key" == spec["rdkPlugins"]["credentialsmanager"]["data"]["privateKey"].asString(),
                           "Preserve classic RALF credentials-manager data");
    }
    return tr.failures;
}

uint32_t Test_RalfOCIConfigGenerator_PrivateDataUsesResolvedImagePath()
{
    L0Test::TestResult tr;

    const std::string dataDirectory = "/tmp/ralf_l0test_private_data";
    const std::string imagePath = dataDirectory + "/private-image.img";
    if (ralf::checkIfPathExists(dataDirectory))
    {
        ralf::removeDirectoryRecursively(dataDirectory);
    }
    L0Test::ExpectTrue(tr, ralf::create_directories(dataDirectory) && WriteFile_OCIGen(imagePath, "image"),
                       "Create a package directory and private image file");

    std::vector<ralf::RalfPkgInfoPair> packages;
    ralf::RalfOCIConfigGenerator generator("/tmp/ralf_l0test_private_data.json", packages);
    Json::Value directoryConfig(Json::objectValue);
    generator.addPrivateDataToOCIConfig(directoryConfig, dataDirectory);
    const Json::Value &directoryLoopback = directoryConfig["rdkPlugins"]["storage"]["data"]["loopback"];
    L0Test::ExpectTrue(tr, !directoryLoopback.isArray() || 0 == directoryLoopback.size(),
                       "RuntimeManager does not infer an image path from a package directory");

    Json::Value imageConfig(Json::objectValue);
    generator.addPrivateDataToOCIConfig(imageConfig, imagePath);
    L0Test::ExpectTrue(tr, imageConfig["rdkPlugins"]["storage"]["data"]["loopback"].isArray() &&
                           1 == imageConfig["rdkPlugins"]["storage"]["data"]["loopback"].size() &&
                           imagePath == imageConfig["rdkPlugins"]["storage"]["data"]["loopback"][0]["source"].asString(),
                       "RuntimeManager mounts the resolved private-data image path");

    ralf::removeDirectoryRecursively(dataDirectory);
    return tr.failures;
}

uint32_t Test_RalfOCIConfigGenerator_ApplyPermissionsToOCIConfig_MissingPackageTypeReturnsTrue()
{
    L0Test::TestResult tr;

    std::vector<ralf::RalfPkgInfoPair> packages;
    ralf::RalfOCIConfigGenerator gen("/tmp/ralf_l0test_permissions_pkgtype_missing.json", packages);

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    manifestRootNode[ralf::PERMISSIONS] = Json::Value(Json::arrayValue);
    manifestRootNode[ralf::PERMISSIONS].append(ralf::PERMISSION_FIREBOLT);

    const bool status = gen.applyPermissionsToOCIConfig(ociConfigRootNode, manifestRootNode,
                                                        { "FIREBOLT_ENDPOINT=ws://127.0.0.1:3473" });
    L0Test::ExpectTrue(tr, status,
                       "applyPermissionsToOCIConfig() returns true when packageType is absent");

    return tr.failures;
}

uint32_t Test_RalfOCIConfigGenerator_ApplyPermissionsToOCIConfig_ValidFireboltPermissionAddsEndpoint()
{
    L0Test::TestResult tr;

    std::vector<ralf::RalfPkgInfoPair> packages;
    ralf::RalfOCIConfigGenerator gen("/tmp/ralf_l0test_permissions_firebolt.json", packages);

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    manifestRootNode[ralf::PACKAGE_TYPE] = ralf::PKG_TYPE_APPLICATION;
    manifestRootNode[ralf::PERMISSIONS] = Json::Value(Json::arrayValue);
    manifestRootNode[ralf::PERMISSIONS].append(ralf::PERMISSION_FIREBOLT);

    const bool status = gen.applyPermissionsToOCIConfig(ociConfigRootNode, manifestRootNode,
                                                        { "FIREBOLT_ENDPOINT=ws://127.0.0.1:3473" });
    L0Test::ExpectTrue(tr, status,
                       "applyPermissionsToOCIConfig() returns true for valid FIREBOLT permission and endpoint");
    L0Test::ExpectTrue(tr,
                       ociConfigRootNode.isMember(ralf::RDKPLUGINS) &&
                           ociConfigRootNode[ralf::RDKPLUGINS].isObject() &&
                           ociConfigRootNode[ralf::RDKPLUGINS].isMember(ralf::NETWORKING),
                       "applyPermissionsToOCIConfig() applies permission-based networking configuration");
    L0Test::ExpectTrue(tr, HasEnvEntry_OCIGen(ociConfigRootNode, "FIREBOLT_ENDPOINT=ws://127.0.0.1:3473"),
                       "applyPermissionsToOCIConfig() adds FIREBOLT_ENDPOINT to OCI process env");

    return tr.failures;
}

uint32_t Test_RalfOCIConfigGenerator_ApplyConfigurationToOCIConfig_PermissionsOnlySkipsPermissionNetworkUpdates()
{
    L0Test::TestResult tr;

    std::vector<ralf::RalfPkgInfoPair> packages;
    ralf::RalfOCIConfigGenerator gen("/tmp/ralf_l0test_cfg_perm_network_guard.json", packages);

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    manifestRootNode[ralf::ENTRY_POINT] = "bin/app";
    manifestRootNode[ralf::PACKAGE_TYPE] = ralf::PKG_TYPE_APPLICATION;
    manifestRootNode[ralf::PERMISSIONS] = Json::Value(Json::arrayValue);
    manifestRootNode[ralf::PERMISSIONS].append(ralf::PERMISSION_FIREBOLT);
    // Keep configuration object present so function does not return early.
    manifestRootNode[ralf::CONFIGURATION] = Json::Value(Json::objectValue);

    const WPEFramework::Plugin::RuntimeConfiguration runtimeConfig;
    const bool status = gen.applyConfigurationToOCIConfig(
        ociConfigRootNode, manifestRootNode, runtimeConfig);

    L0Test::ExpectTrue(tr, status,
                       "applyConfigurationToOCIConfig() succeeds for minimal application manifest");
    L0Test::ExpectTrue(tr,
                       !ociConfigRootNode.isMember(ralf::RDKPLUGINS) ||
                           !ociConfigRootNode[ralf::RDKPLUGINS].isMember(ralf::NETWORKING),
                       "applyConfigurationToOCIConfig() no longer applies permission-based networking updates");

    return tr.failures;
}

uint32_t Test_RalfOCIConfigGenerator_ApplyPermissionsToOCIConfig_FireboltEmptyEnvReturnsFalse()
{
    L0Test::TestResult tr;

    std::vector<ralf::RalfPkgInfoPair> packages;
    ralf::RalfOCIConfigGenerator gen("/tmp/ralf_l0test_perm_env_empty.json", packages);

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    manifestRootNode[ralf::PACKAGE_TYPE] = ralf::PKG_TYPE_APPLICATION;
    manifestRootNode[ralf::PERMISSIONS] = Json::Value(Json::arrayValue);
    manifestRootNode[ralf::PERMISSIONS].append(ralf::PERMISSION_FIREBOLT);

    const bool status = gen.applyPermissionsToOCIConfig(ociConfigRootNode, manifestRootNode, {});
    L0Test::ExpectTrue(tr, !status,
                       "applyPermissionsToOCIConfig() returns false when FIREBOLT permission has empty envVariables");

    return tr.failures;
}

uint32_t Test_RalfOCIConfigGenerator_ApplyPermissionsToOCIConfig_ThunderAddsThunderAccess()
{
    L0Test::TestResult tr;

    std::vector<ralf::RalfPkgInfoPair> packages;
    ralf::RalfOCIConfigGenerator gen("/tmp/ralf_l0test_perm_env_thunder.json", packages);

    Json::Value ociConfigRootNode(Json::objectValue);
    Json::Value manifestRootNode(Json::objectValue);
    manifestRootNode[ralf::PACKAGE_TYPE] = ralf::PKG_TYPE_APPLICATION;
    manifestRootNode[ralf::PERMISSIONS] = Json::Value(Json::arrayValue);
    manifestRootNode[ralf::PERMISSIONS].append(ralf::PERMISSION_THUNDER);

    setenv(ralf::THUNDER_ACCESS_ENV_KEY, "127.0.0.1:9998", 1);

    const bool status = gen.applyPermissionsToOCIConfig(ociConfigRootNode, manifestRootNode,
                                                        { "TARGET_STATE=4" });
    L0Test::ExpectTrue(tr, status,
                       "applyPermissionsToOCIConfig() returns true for THUNDER permission when THUNDER_ACCESS is set");
    L0Test::ExpectTrue(tr, HasEnvEntry_OCIGen(ociConfigRootNode, "THUNDER_ACCESS=127.0.0.1:9998"),
                       "applyPermissionsToOCIConfig() adds THUNDER_ACCESS to OCI process env");

    unsetenv(ralf::THUNDER_ACCESS_ENV_KEY);
    return tr.failures;
}

uint32_t Test_RalfOCIConfigGenerator_AppliesRuntimeEnvironmentAndSecrets()
{
    L0Test::TestResult tr;

    std::vector<ralf::RalfPkgInfoPair> packages;
    ralf::RalfOCIConfigGenerator gen("/tmp/ralf_l0test_runtime_environment.json", packages);
    Json::Value ociConfigRootNode(Json::objectValue);

    Json::Value packageConfig(Json::objectValue);
    packageConfig["runtimeSecrets"]["applicationSecret"] = "test-secret";
    packageConfig["runtimeSecrets"]["credentialsManager"]["certificate"] = "test-certificate";
    packageConfig["runtimeSecrets"]["credentialsManager"]["privateKey"] = "test-private-key";
    const bool secretsApplied = gen.addPackageSecretsToOCIConfig(ociConfigRootNode, packageConfig);
    L0Test::ExpectTrue(tr, secretsApplied, "package secrets are applied to OCI config");

    const std::vector<std::string> envVariables = {
        "APP_CONFIG_OVERRIDES_JSON=eyJ0ZXN0Ijp0cnVlfQ==",
        "EXTRA_VALUE=first=second",
        "APPLICATION_SECRET=generic-secret"
    };
    const bool status = gen.addRuntimeEnvironmentToOCIConfig(ociConfigRootNode, envVariables);

    L0Test::ExpectTrue(tr, status, "runtime environment and secret metadata are applied");
    L0Test::ExpectTrue(tr, HasEnvEntry_OCIGen(ociConfigRootNode,
                                              "APP_CONFIG_OVERRIDES_JSON=eyJ0ZXN0Ijp0cnVlfQ=="),
                       "RALF config overrides are added to process environment");
    L0Test::ExpectTrue(tr, HasEnvEntry_OCIGen(ociConfigRootNode, "EXTRA_VALUE=first=second"),
                       "environment values preserve equals signs");
    L0Test::ExpectTrue(tr, HasEnvEntry_OCIGen(ociConfigRootNode, "APPLICATION_SECRET=test-secret"),
                       "application secret is added to process environment");
    L0Test::ExpectTrue(tr, !HasEnvEntry_OCIGen(ociConfigRootNode, "APPLICATION_SECRET=generic-secret"),
                       "generic runtime environment cannot replace the package application secret");
    L0Test::ExpectTrue(tr, !HasEnvEntry_OCIGen(ociConfigRootNode,
                                               "RALF_CREDMGR_CERTIFICATE=test-certificate"),
                       "credentials are not exposed as process environment variables");
    L0Test::ExpectTrue(tr,
                       ociConfigRootNode["rdkPlugins"]["credentialsmanager"]["data"]["certificate"] ==
                           "test-certificate" &&
                           ociConfigRootNode["rdkPlugins"]["credentialsmanager"]["data"]["privateKey"] ==
                           "test-private-key",
                       "credentials are routed to the credentials-manager plugin");

    const std::string outputPath = "/tmp/ralf_l0test_runtime_environment.json";
#ifdef RDK_APPMANAGERS_DEBUG
    glob_t staleDebugSpecs = {};
    if (0 == ::glob("/tmp/generatedRalfSpec.*", GLOB_NOSORT, nullptr, &staleDebugSpecs))
    {
        for (size_t index = 0; index < staleDebugSpecs.gl_pathc; ++index)
        {
            ::unlink(staleDebugSpecs.gl_pathv[index]);
        }
    }
    ::globfree(&staleDebugSpecs);
#endif
    struct stat outputStat {};
    const bool saved = gen.saveOCIConfigToFile(ociConfigRootNode, 0, 0);
    L0Test::ExpectTrue(tr, saved, "OCI config with package secrets is written successfully");
    L0Test::ExpectTrue(tr, saved && (::stat(outputPath.c_str(), &outputStat) == 0) &&
                           ((outputStat.st_mode & 0777) == 0600),
                       "OCI config containing package secrets is restricted to its owner");
    ::unlink(outputPath.c_str());
#ifdef RDK_APPMANAGERS_DEBUG
    glob_t debugSpecFiles = {};
    const int globStatus = ::glob("/tmp/generatedRalfSpec.*", GLOB_NOSORT, nullptr, &debugSpecFiles);
    const std::string debugSpecPath = (0 == globStatus && debugSpecFiles.gl_pathc > 0)
        ? debugSpecFiles.gl_pathv[0] : "";
    std::ifstream debugSpecFile(debugSpecPath);
    const std::string debugSpec((std::istreambuf_iterator<char>(debugSpecFile)),
                                std::istreambuf_iterator<char>());
    struct stat debugSpecStat {};
    L0Test::ExpectTrue(tr, !debugSpec.empty(),
                       "debug OCI config dump is available when secrets are present");
    L0Test::ExpectTrue(tr, debugSpec.find("test-secret") == std::string::npos &&
                               debugSpec.find("test-certificate") == std::string::npos &&
                               debugSpec.find("test-private-key") == std::string::npos,
                       "debug OCI config dump redacts application secrets and credentials");
    L0Test::ExpectTrue(tr, debugSpec.find("<redacted>") != std::string::npos,
                       "debug OCI config dump indicates redacted secret fields");
    L0Test::ExpectTrue(tr, !debugSpecPath.empty() && 0 == ::stat(debugSpecPath.c_str(), &debugSpecStat) &&
                           (debugSpecStat.st_mode & 0777) == 0600,
                       "debug OCI config dump is private to its owner");
    if (!debugSpecPath.empty())
    {
        ::unlink(debugSpecPath.c_str());
    }
    ::globfree(&debugSpecFiles);
#endif

    Json::Value incompleteConfig(Json::objectValue);
    Json::Value incompleteSecrets(Json::objectValue);
    incompleteSecrets["runtimeSecrets"]["credentialsManager"]["certificate"] = "test-certificate";
    L0Test::ExpectTrue(tr,
                       !gen.addPackageSecretsToOCIConfig(incompleteConfig, incompleteSecrets),
                       "incomplete credentials metadata fails launch config generation");

    return tr.failures;
}
