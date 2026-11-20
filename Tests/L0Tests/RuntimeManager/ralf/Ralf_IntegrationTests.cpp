/*
 * Copyright 2026 RDK Management
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <sys/mount.h>
#include <unistd.h>
#include <vector>

#include "ralf/RalfPackageBuilder.h"
#include "ralf/RalfSupport.h"
#include "ralf/OCISpecConstants.h"
#include "ralf/NetworkConfigurationHelper.h"
#define private public
#include "ralf/RalfOCIConfigGenerator.h"
#undef private
#include "common/L0Expect.hpp"

namespace {

bool mountSucceeds = false;
int unmountError = EPERM;
int chownError = 0;
std::string mountOptions;
std::string mountTarget;
unsigned int unmountCalls = 0;

class Fixture {
public:
    Fixture()
        : instance("coverage-" + std::to_string(getpid())),
          appDirectory(ralf::RALF_APP_ROOTFS_DIR + instance),
          metadata(appDirectory + "/metadata.json"),
          packageList(appDirectory + "/packages.json")
    {
        mountSucceeds = true;
        unmountError = 0;
        unmountCalls = 0;
    }

    ~Fixture()
    {
        mountSucceeds = false;
        unmountError = EPERM;
        chownError = 0;
        for (const auto& path : files) {
            ::remove(path.c_str());
        }
        if (ralf::checkIfPathExists(appDirectory)) {
            ralf::removeDirectoryRecursively(appDirectory);
        }
    }

    bool write(const std::string& path, const Json::Value& node)
    {
        if (!ralf::create_directories(path.substr(0, path.find_last_of('/')))) {
            return false;
        }
        // Never overwrite an existing fixture or a host configuration.
        if (ralf::checkIfPathExists(path)) {
            return false;
        }
        files.push_back(path);
        std::ofstream output(path);
        output << ralf::serializeJsonNode(node);
        return output.good();
    }

    bool configure()
    {
        Json::Value base(Json::objectValue);
        base[ralf::PROCESS][ralf::ENV].append("HOME=/old");
        base[ralf::PROCESS][ralf::ENV].append("KEEP=yes");
        base[ralf::PROCESS][ralf::ENV].append("HOME=/duplicate");
        base[ralf::PROCESS][ralf::ENV].append(42);
        base[ralf::LINUX][ralf::RESOURCES][ralf::MEMORY][ralf::MEMORY_LIMIT] = 123;
        Json::Value graphics(Json::objectValue);
        graphics[ralf::VENDOR_GPU_SUPPORT][ralf::DEV_NODES].append("/dev/null");
        graphics[ralf::VENDOR_GPU_SUPPORT][ralf::DEV_NODES].append(appDirectory + "/absent-device");
        graphics[ralf::VENDOR_GPU_SUPPORT][ralf::GROUP_IDS].append("root");
        graphics[ralf::VENDOR_GPU_SUPPORT][ralf::GROUP_IDS].append("ralf-absent-group");
        Json::Value file(Json::objectValue);
        file[ralf::SOURCE] = "/dev/null";
        file[ralf::DESTINATION] = "/etc/gpu.conf";
        graphics[ralf::VENDOR_GPU_SUPPORT][ralf::FILES].append(file);
        graphics[ralf::VENDOR_GPU_SUPPORT][ralf::FILES].append(Json::Value(Json::objectValue));

        Json::Value manifest(Json::objectValue);
        manifest[ralf::PACKAGE_TYPE] = ralf::PKG_TYPE_APPLICATION;
        manifest[ralf::ENTRY_POINT] = "/app/start";
        manifest[ralf::ENTRY_ARGS].append("--verbose");
        manifest[ralf::VERSION] = "12";
        manifest[ralf::VERSION_NAME] = "release";
        auto& configuration = manifest[ralf::CONFIGURATION];
        configuration[ralf::MEMORY_CONFIG_URN][ralf::SYSTEM_MEMORY] = "64M";
        configuration[ralf::STORAGE_CONFIG_URN][ralf::MAX_LOCAL_STORAGE] = "32M";
        configuration[ralf::CONFIG_OVERRIDES_URN][ralf::PKG_TYPE_APPLICATION]["enabled"] = true;
        configuration[ralf::CONFIG_OVERRIDES_URN][ralf::PKG_TYPE_RUNTIME]["mode"] = "test";
        configuration[ralf::ENV_CONFIG_URN]["EXTRA"] = "value";
        manifest[ralf::PERMISSIONS].append(ralf::PERMISSION_INTERNET);

        Json::Value packages(Json::objectValue);
        Json::Value package(Json::objectValue);
        package["pkgMetaDataPath"] = metadata;
        package["pkgMountPath"] = appDirectory + "/application-layer";
        packages["packages"].append(package);
        package["pkgMountPath"] = appDirectory + "/base-layer";
        packages["packages"].append(package);
        return write(ralf::RALF_OCI_BASE_SPEC_FILE, base) &&
               write(ralf::RALF_GRAPHICS_LAYER_CONFIG, graphics) &&
               write(metadata, manifest) && write(packageList, packages);
    }

    WPEFramework::Plugin::ApplicationConfiguration app() const
    {
        WPEFramework::Plugin::ApplicationConfiguration result;
        result.mAppId = "com.test.coverage";
        result.mAppInstanceId = instance;
        result.mUserId = getuid();
        result.mGroupId = getgid();
        result.mAppStorageInfo.path = appDirectory + "/storage";
        result.mWesterosSocketPath = "/tmp/wayland-coverage";
        return result;
    }

    WPEFramework::Plugin::RuntimeConfiguration runtime() const
    {
        WPEFramework::Plugin::RuntimeConfiguration result;
        result.ralfPkgPath = packageList;
        return result;
    }

    const std::string instance;
    const std::string appDirectory;
    const std::string metadata;
    const std::string packageList;
private:
    std::vector<std::string> files;
};

std::string environment(const Json::Value& config, const std::string& key)
{
    const std::string prefix = key + "=";
    for (const auto& entry : config[ralf::PROCESS][ralf::ENV]) {
        if (entry.isString() && entry.asString().find(prefix) == 0) {
            return entry.asString().substr(prefix.size());
        }
    }
    return "";
}

}

// These tests must never perform real mounts, even when run as root.
extern "C" int __wrap_mount(const char*, const char* target, const char*,
                            unsigned long, const void* data)
{
    mountTarget = target;
    mountOptions = data ? static_cast<const char*>(data) : "";
    if (!mountSucceeds) {
        errno = EPERM;
        return -1;
    }
    return 0;
}

extern "C" int __wrap_umount(const char*)
{
    ++unmountCalls;
    if (unmountError != 0) {
        errno = unmountError;
        return -1;
    }
    return 0;
}

extern "C" int __wrap_chown(const char*, uid_t, gid_t)
{
    if (chownError != 0) {
        errno = chownError;
        return -1;
    }
    return 0;
}

uint32_t Test_RalfPackageBuilder_GeneratesCompletePackageAndCleansUp()
{
    L0Test::TestResult tr;
    Fixture fixture;
    if (!fixture.configure()) {
        L0Test::ExpectTrue(tr, false, "Create isolated Ralf configuration fixtures");
        return tr.failures;
    }
    ralf::RalfPackageBuilder builder;
    const auto app = fixture.app();
    std::string spec;
    const bool generated = builder.generateRalfDobbySpec(app, fixture.runtime(), spec);
    L0Test::ExpectTrue(tr, generated,
                       "Generate package with a mocked overlay mount");
    if (!generated) {
        return tr.failures;
    }
    L0Test::ExpectEqStr(tr, spec, fixture.appDirectory, "Return the OCI bundle directory");
    L0Test::ExpectEqStr(tr, mountTarget, fixture.appDirectory + "/rootfs", "Mount target");
    const std::string layers = "lowerdir=" + ralf::RALF_GRAPHICS_LAYER_ROOTFS + ":" +
        fixture.appDirectory + "/base-layer:" + fixture.appDirectory + "/application-layer";
    L0Test::ExpectTrue(tr, mountOptions.find(layers + ",upperdir=") == 0,
                       "Graphics layer precedes reversed package layers");
    Json::Value config;
    if (!ralf::JsonFromFile(spec + "/config.json", config)) {
        L0Test::ExpectTrue(tr, false, "Read generated OCI config");
        return tr.failures;
    }
    L0Test::ExpectEqStr(tr, config[ralf::HOSTNAME].asString(), app.mAppId, "Application hostname");
    L0Test::ExpectEqStr(tr, environment(config, "HOME"), "/data", "Override duplicate HOME entries");
    unsigned int homes = 0;
    for (const auto& entry : config[ralf::PROCESS][ralf::ENV]) {
        if (entry.isString() && entry.asString().find("HOME=") == 0) {
            ++homes;
        }
    }
    L0Test::ExpectEqU32(tr, homes, 1, "Only one HOME remains");
    L0Test::ExpectEqStr(tr, environment(config, "KEEP"), "yes", "Preserve unrelated environment");
    L0Test::ExpectEqStr(tr, environment(config, "WAYLAND_DISPLAY"), "wayland-coverage", "Wayland display basename");
    L0Test::ExpectEqStr(tr, environment(config, "APP_PACKAGE_VERSION"), "release_12", "Package version");
    L0Test::ExpectEqStr(tr, environment(config, "CPU_MEMORY_LIMIT"), "67108864", "Memory limit in bytes");
    L0Test::ExpectEqStr(tr, environment(config, "STORAGE_LIMIT"), "33554432", "Storage limit in bytes");
    L0Test::ExpectEqStr(tr, environment(config, "EXTRA"), "value", "Manifest environment");
    L0Test::ExpectEqStr(tr, environment(config, "APP_CONFIG_OVERRIDES_JSON"), "{\"enabled\":true}", "Application overrides");
    L0Test::ExpectEqStr(tr, environment(config, "RUNTIME_CONFIG_OVERRIDES_JSON"), "{\"mode\":\"test\"}", "Runtime overrides");
    L0Test::ExpectEqU32(tr, config[ralf::LINUX][ralf::UID_MAPPINGS][0][ralf::HOST_ID].asUInt(),
                        getuid(), "Map container root to the application user");
    L0Test::ExpectEqStr(tr, config[ralf::LINUX][ralf::DEVICES][0][ralf::DEV_PATH].asString(),
                        "/dev/null", "Graphics device");
    L0Test::ExpectTrue(tr, config[ralf::LINUX][ralf::RESOURCES][ralf::DEVICES][0][ralf::ALLOW].asBool(),
                       "Allow graphics device access");
    for (const auto* operation : {"createRuntime", "createContainer", "poststart", "poststop"}) {
        L0Test::ExpectEqStr(tr, config[ralf::HOOKS][operation][0][ralf::ARGS][2].asString(),
                            operation, "Hook operation");
        L0Test::ExpectEqStr(tr, config[ralf::HOOKS][operation][0][ralf::ARGS][4].asString(),
                            spec + "/config.json", "Hook config path");
    }
    L0Test::ExpectTrue(tr, ralf::checkIfPathExists(spec + "/rootfs/etc/gpu.conf"),
                       "Prepare graphics file bind destination");
    L0Test::ExpectTrue(tr, builder.unmountOverlayfsIfExists(fixture.instance), "Unmount and clean bundle");
    L0Test::ExpectEqU32(tr, unmountCalls, 1, "One unmount");
    L0Test::ExpectTrue(tr, !ralf::checkIfPathExists(spec), "Bundle directory removed");
    return tr.failures;
}

uint32_t Test_RalfPackageBuilder_ConfigFailureClearsSpecAndUnmounts()
{
    L0Test::TestResult tr;
    Fixture fixture;
    if (!fixture.configure()) {
        L0Test::ExpectTrue(tr, false, "Create isolated Ralf configuration fixtures");
        return tr.failures;
    }
    ::remove(fixture.metadata.c_str());
    ralf::RalfPackageBuilder builder;
    std::string spec = "previous";
    L0Test::ExpectTrue(tr, !builder.generateRalfDobbySpec(fixture.app(), fixture.runtime(), spec),
                       "Reject missing package metadata after mounting");
    L0Test::ExpectTrue(tr, spec.empty(), "Clear output on config failure");
    L0Test::ExpectEqU32(tr, unmountCalls, 1, "Unmount after config failure");
    L0Test::ExpectTrue(tr, !ralf::checkIfPathExists(fixture.appDirectory), "Clean failed bundle");
    return tr.failures;
}

uint32_t Test_RalfPackageBuilder_UnmountedDirectoryCleanupIsNonFatal()
{
    L0Test::TestResult tr;
    Fixture fixture;
    L0Test::ExpectTrue(tr, ralf::create_directories(fixture.appDirectory + "/rootfs"),
                       "Create partial bundle");
    unmountError = EINVAL;
    ralf::RalfPackageBuilder builder;
    L0Test::ExpectTrue(tr, builder.unmountOverlayfsIfExists(fixture.instance),
                       "Already-unmounted directory is non-fatal");
    L0Test::ExpectTrue(tr, !ralf::checkIfPathExists(fixture.appDirectory), "Remove partial bundle");
    return tr.failures;
}

uint32_t Test_RalfOCIConfigGenerator_RuntimeAndServiceConfiguration()
{
    L0Test::TestResult tr;
    const std::string outputPath = "/unused/config.json";
    const std::vector<ralf::RalfPkgInfoPair> packages;
    std::unique_ptr<ralf::RalfOCIConfigGenerator> owner(
        new ralf::RalfOCIConfigGenerator(outputPath, packages));
    auto& generator = *owner;
    Json::Value runtimeManifest(Json::objectValue);
    runtimeManifest[ralf::PACKAGE_TYPE] = ralf::PKG_TYPE_RUNTIME;
    runtimeManifest[ralf::ENTRY_POINT] = "/runtime/start";
    runtimeManifest[ralf::ENTRY_ARGS].append("--runtime");
    runtimeManifest[ralf::CONFIGURATION][ralf::ENV_CONFIG_URN]["IGNORED"] = "value";
    WPEFramework::Plugin::RuntimeConfiguration runtime;
    runtime.command = "/app/start";
    Json::Value config(Json::objectValue);
    L0Test::ExpectTrue(tr, generator.applyConfigurationToOCIConfig(config, runtimeManifest, runtime),
                       "Apply runtime package configuration");
    L0Test::ExpectEqStr(tr, config[ralf::PROCESS][ralf::ARGS][2].asString(),
                        "/app/start", "Append runtime command after entry arguments");
    L0Test::ExpectTrue(tr, environment(config, "IGNORED").empty(), "Runtime cannot set manifest env");

    Json::Value service(Json::objectValue);
    service[ralf::PACKAGE_TYPE] = ralf::PKG_TYPE_SERVICE;
    service[ralf::ENTRY_ARGS] = "invalid";
    service[ralf::CONFIGURATION][ralf::ENV_CONFIG_URN]["SERVICE"] = "enabled";
    service[ralf::CONFIGURATION][ralf::ENV_CONFIG_URN]["NUMBER"] = 42;
    L0Test::ExpectTrue(tr, generator.applyConfigurationToOCIConfig(config, service, runtime),
                       "Apply service package configuration");
    L0Test::ExpectEqStr(tr, environment(config, "SERVICE"), "enabled", "Service manifest env");
    L0Test::ExpectTrue(tr, environment(config, "NUMBER").empty(), "Skip non-string environment values");

    Json::Value invalidConfig(Json::objectValue);
    invalidConfig[ralf::ENV_CONFIG_URN] = "not an object";
    L0Test::ExpectTrue(tr, !generator.addConfigEnvToOCIConfig(config, invalidConfig), "Reject non-object env");
    invalidConfig[ralf::ENV_CONFIG_URN] = Json::Value(Json::objectValue);
    L0Test::ExpectTrue(tr, !generator.addConfigEnvToOCIConfig(config, invalidConfig), "Reject empty env");
    invalidConfig[ralf::CONFIG_OVERRIDES_URN] = Json::Value(Json::objectValue);
    L0Test::ExpectTrue(tr, !generator.addConfigOverridesToOCIConfig(config, invalidConfig),
                       "Reject overrides without application or runtime objects");
    invalidConfig[ralf::MEMORY_CONFIG_URN][ralf::SYSTEM_MEMORY] = "invalid";
    config[ralf::LINUX][ralf::RESOURCES][ralf::MEMORY][ralf::MEMORY_LIMIT] = 123;
    L0Test::ExpectTrue(tr, !generator.addMemoryConfigToOCIConfig(config, invalidConfig, ralf::PKG_TYPE_APPLICATION),
                       "Reject invalid memory size");
    L0Test::ExpectEqU32(tr, config[ralf::LINUX][ralf::RESOURCES][ralf::MEMORY][ralf::MEMORY_LIMIT].asUInt(),
                        123, "Retain base memory limit on invalid override");
    L0Test::ExpectEqStr(tr, environment(config, "CPU_MEMORY_LIMIT"), ralf::DEFAULT_RAM_LIMIT, "Default memory env");
    invalidConfig[ralf::STORAGE_CONFIG_URN][ralf::MAX_LOCAL_STORAGE] = "invalid";
    L0Test::ExpectTrue(tr, !generator.addStorageConfigToOCIConfig(config, invalidConfig), "Reject invalid storage size");
    L0Test::ExpectEqStr(tr, environment(config, "STORAGE_LIMIT"), ralf::DEFAULT_STORAGE_LIMIT, "Default storage env");
    return tr.failures;
}

uint32_t Test_RalfOCIConfigGenerator_GraphicsAndSaveFailures()
{
    L0Test::TestResult tr;
    Fixture fixture;
    const std::vector<ralf::RalfPkgInfoPair> packages;
    const std::string outputPath = fixture.appDirectory + "/config.json";
    ralf::RalfOCIConfigGenerator generator(outputPath, packages);
    Json::Value config(Json::objectValue);
    Json::Value graphics(Json::objectValue);
    L0Test::ExpectTrue(tr, generator.applyGraphicsConfigToOCIConfig(config, graphics), "Optional graphics config");
    graphics[ralf::VENDOR_GPU_SUPPORT] = Json::Value(Json::objectValue);
    L0Test::ExpectTrue(tr, generator.applyGraphicsConfigToOCIConfig(config, graphics), "Optional graphics fields");
    graphics[ralf::VENDOR_GPU_SUPPORT][ralf::DEV_NODES] = Json::Value(Json::arrayValue);
    L0Test::ExpectTrue(tr, !generator.applyGraphicsConfigToOCIConfig(config, graphics), "Empty device list fails");
    L0Test::ExpectTrue(tr, !generator.saveOCIConfigToFile(config, getuid(), getgid()),
                       "Report missing output directory");
    L0Test::ExpectTrue(tr, ralf::create_directories(fixture.appDirectory), "Create output directory");
    chownError = EPERM;
    L0Test::ExpectTrue(tr, !generator.saveOCIConfigToFile(config, getuid(), getgid()),
                       "Report ownership change failure");
    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_LoopbackEndpointVariants()
{
    L0Test::TestResult tr;
    struct EndpointCase {
        const char* endpoint;
        unsigned int port;
        const char* protocol;
    };
    const EndpointCase cases[] = {
        { "http://localhost/path:ignored", 80, "tcp" },
        { "https://127.0.0.1?query=value", 443, "tcp" },
        { "ssh://localhost", 22, "tcp" },
        { "ftp://localhost", 21, "tcp" },
        { "dns://localhost", 53, "udp" },
        { "dhcp://localhost", 67, "udp" },
        { "snmp://localhost", 161, "udp" },
        { "tftp://localhost", 69, "udp" },
        { "wss://[::1]:3473/path", 3473, "tcp" },
        { "http://[0:0:0:0:0:0:0:1]", 80, "tcp" },
        { "ws://localhost:65535?x=y", 65535, "tcp" },
        { "http://localhost:0", 0, "" },
        { "http://localhost:65536", 0, "" },
        { "http://localhost:123456", 0, "" },
        { "http://localhost:", 0, "" },
        { "http://localhost:12x", 0, "" },
        { "unknown://localhost", 0, "" },
        { "localhost", 0, "" },
        { "http://[::1", 0, "" },
        { "http://[::1]suffix:80", 0, "" },
        { "http://[::2]:80", 0, "" },
        { "http://localhost.example:80", 0, "" },
    };
    for (const auto& test : cases) {
        Json::Value config(Json::objectValue);
        Json::Value manifest(Json::objectValue);
        manifest[ralf::PERMISSIONS].append(ralf::PERMISSION_FIREBOLT);
        const bool status = NetworkConfigurationHelper::updatePermissionBasedNetworkConfiguration(
            config, manifest, {std::string("FIREBOLT_ENDPOINT=") + test.endpoint});
        L0Test::ExpectTrue(tr, status == (test.port != 0), test.endpoint);
        const auto& rules = config[ralf::RDKPLUGINS][ralf::NETWORKING][ralf::DATA]
                                  ["portForwarding"]["containerToHost"];
        if (test.port != 0) {
            L0Test::ExpectEqU32(tr, rules.size(), 1, "Exactly one loopback forwarding rule");
            if (rules.size() == 1) {
                L0Test::ExpectEqU32(tr, rules[0][ralf::PORT].asUInt(), test.port, test.endpoint);
                L0Test::ExpectEqStr(tr, rules[0][ralf::PROTOCOL].asString(), test.protocol, test.endpoint);
            }
        } else {
            L0Test::ExpectTrue(tr, rules.isNull(), "Invalid endpoint must not create forwarding rules");
        }
    }
    return tr.failures;
}

uint32_t Test_NetworkConfigurationHelper_ProtocolAliasesAndRepeatedMerge()
{
    L0Test::TestResult tr;
    Json::Value config(Json::objectValue);
    Json::Value manifest(Json::objectValue);
    const char* protocols[] = {"http", "dns", "coap", "wss", "", "unsupported", "other"};
    for (unsigned int index = 0; index < 7; ++index) {
        Json::Value entry(Json::objectValue);
        entry[ralf::TYPE] = "public";
        entry[ralf::PORT] = 8000 + index;
        entry[ralf::PROTOCOL] = protocols[index];
        manifest[ralf::CONFIGURATION][ralf::NETWORK_CONFIG_URN].append(entry);
    }
    for (unsigned int iteration = 0; iteration < 2; ++iteration) {
        L0Test::ExpectTrue(tr, NetworkConfigurationHelper::updateNetworkConfigurationNode(config, manifest),
                           "Merge normalized network rules");
    }
    const auto& rules = config[ralf::RDKPLUGINS][ralf::NETWORKING][ralf::DATA]
                              ["portForwarding"]["hostToContainer"];
    L0Test::ExpectEqU32(tr, rules.size(), 7, "Repeated merge must not duplicate forwarding rules");
    if (rules.size() == 7) {
        for (unsigned int index = 0; index < 7; ++index) {
            L0Test::ExpectEqStr(tr, rules[index][ralf::PROTOCOL].asString(),
                                index == 1 || index == 2 ? "udp" : "tcp", protocols[index]);
        }
    }
    return tr.failures;
}
