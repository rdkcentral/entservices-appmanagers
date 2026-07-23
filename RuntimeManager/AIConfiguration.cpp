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

#include "AIConfiguration.h"
#include "UtilsLogging.h"
#include "tracing/Logging.h"
#include <json/json.h>
#include <fstream>
#include <sstream>
#include <string>
#include <set>
#include <climits>
#include <cinttypes>
#include <cctype>
#include <sys/stat.h>
#include <yaml-cpp/yaml.h>

#define AICONFIGURATION_JSON_PATH "/opt/rdkappmanagers.config"

extern char **environ;

namespace WPEFramework
{
namespace Plugin
{
    // Fix for Coverity issue 1078 - UNINIT_CTOR: Initialize all member variables in constructor
    AIConfiguration::AIConfiguration()
        : mConsoleLogCap(0)
        , mAppsCpuSet()
        , mNonHomeAppMemoryLimit(0)
        , mNonHomeAppGpuLimit(0)
        , mVpuAccessBlacklist()
        , mAppsRequiringDBus()
        , mMapiPorts()
        , mResourceManagerClientEnabled(false)
        , mGstreamerRegistryEnabled(false)
        , mSvpEnabled(false)
        , mEnableUsbMassStorage(false)
        , mIPv6Enabled(false)
        , mIonHeapDefaultQuota(0)
        , mDialServerPort(0)
        , mDialServerPathPrefix()
        , mDialUsn()
        , mIonHeapQuotas()
        , mPreloads()
        , mEnvVariables()
        , mDefaultAllowedLogLevels({"fatal", "error", "warning", "milestone", "info", "debug"})
    {
        // All members initialized in initialization list above
    }


    AIConfiguration::~AIConfiguration()
    {
    }

    void AIConfiguration::initialize(const std::string& runtimeConfigFile)
    {
        readFromConfigFile();
        readFromYamlConfigFile(runtimeConfigFile);
    }

    size_t AIConfiguration::getContainerConsoleLogCap()
    {
        return mConsoleLogCap;	
    }

    std::bitset<32> AIConfiguration::getAppsCpuSet() const
    {
        return mAppsCpuSet;
    }

    ssize_t AIConfiguration::getNonHomeAppMemoryLimit()
    {
        return mNonHomeAppMemoryLimit;
    }

    ssize_t AIConfiguration::getNonHomeAppGpuLimit()
    {
        return mNonHomeAppGpuLimit;	
    }

    std::list<std::string> AIConfiguration::getVpuAccessBlacklist() const
    {
        return mVpuAccessBlacklist;	    
    }

    std::list<std::string> AIConfiguration::getAppsRequiringDBus() const
    {
        return mAppsRequiringDBus;
    }

    std::list<int> AIConfiguration::getMapiPorts() const
    {
        return mMapiPorts;
    }

    bool AIConfiguration::getResourceManagerClientEnabled() const
    {
        return mResourceManagerClientEnabled;
    }

    bool AIConfiguration::getGstreamerRegistryEnabled()
    {
        return mGstreamerRegistryEnabled;
    }

    bool AIConfiguration::getSvpEnabled()
    {
        return mSvpEnabled;
    }

    bool AIConfiguration::getEnableUsbMassStorage()
    {
        return mEnableUsbMassStorage;
    }

    bool AIConfiguration::getIPv6Enabled()
    {
        return mIPv6Enabled;
    }

    size_t AIConfiguration::getIonHeapDefaultQuota() const
    {
        return mIonHeapDefaultQuota;
    }

    in_port_t AIConfiguration::getDialServerPort() const
    {
        return mDialServerPort;
    }
    
    std::string AIConfiguration::getDialServerPathPrefix() const
    { 
        return mDialServerPathPrefix;
    }
    
    std::string AIConfiguration::getDialUsn() const
    {
        return mDialUsn;
    }

    std::list<std::string> AIConfiguration::getPreloads() const
    {
        return mPreloads;
    }

    std::list<std::string> AIConfiguration::getDefaultAllowedLogLevels() const
    {
        return mDefaultAllowedLogLevels;
    }
    std::list<std::string> AIConfiguration::readGlobalEnv() const
    {
       std::list<std::string> environmentVariables;
       char **envList = environ;

       for (;*envList;envList++)
       {
           environmentVariables.emplace_back(*envList);
       }
       return environmentVariables;
    }

    std::list<std::string> AIConfiguration::getEnvs() const
    {
        return mEnvVariables;
    }

    std::map<std::string, size_t> AIConfiguration::getIonHeapQuotas() const
    {
        return mIonHeapQuotas;
    }

    void AIConfiguration::readFromCustomData()
    {
        mConsoleLogCap = 1 * 1024 * 1024; //.apps.limits.consoleLogBytes
        mAppsCpuSet = 0xff; //.apps.limits.cpuset
        mNonHomeAppMemoryLimit = 200 * 1024 * 1024; //.apps.limits.memory.defaultLimitBytes
        mNonHomeAppGpuLimit = 64 * 1024 * 1024; //.apps.limits.memory.defaultLimitBytes
        /* mVpuAccessBlacklist - apps.vpuAccessBlacklist */
        /* mAppsRequiringDBusList - apps.requireDBus */
        /* mMapiPorts = apps.mapi.ports */
        mResourceManagerClientEnabled = false; // .apps.essosResourceManager.enableClient
        mGstreamerRegistryEnabled = false; // apps.gstreamer.mapCachedRegistry
        mSvpEnabled = false; // apps.svp.enable
        mEnableUsbMassStorage = false; // apps.usbMassStorage.enable
        mIPv6Enabled = false; // apps.enableIPv6
        mIonHeapDefaultQuota = 256 * 1024 * 1024; // apps.limits.ion.defaultLimitBytes
        mDialServerPort = 8009; //.dial.server.port;
        mDialServerPathPrefix = ""; // dial.server.prefix
        mDialUsn = ""; //.dial.usn
        //mPreloads

        //TODO: SUPPORT Dial
	/*
        if (mDialServerPathPrefix.empty())
        {
            auto dialUuid = AICommon::Uuid::createUuid();
            mDialServerPathPrefix = dialUuid.toString();
        }
        if (mDialUsn.empty())
        {
            AICommon::DthMacAddressProvider macAddressProvider;
            mDialUsn = AICommon::getDeviceDialUsn(macAddressProvider.getMac());
        }
	*/
        mEnvVariables.push_back("WESTEROS_SINK_AMLOGIC_USE_DMABUF=1");
        mEnvVariables.push_back("WESTEROS_GL_USE_AMLOGIC_AVSYNC=1");
        mEnvVariables.push_back("WESTEROS_SINK_USE_FREERUN=1");
        mEnvVariables.push_back("WESTEROS_GL_MODE=3840x2160x60");
        mEnvVariables.push_back("WESTEROS_GL_GRAPHICS_MAX_SIZE=1920x1080");
        mEnvVariables.push_back("WESTEROS_GL_USE_REFRESH_LOCK=1");
        mDefaultAllowedLogLevels = {"fatal", "error", "warning", "milestone", "info", "debug"};
    }

    // Parses a cpuset string like "0-3,8-11" into a bitset<32>.
    // Mirrors ConfigFile::readBitsetValue from appinfrastructure.
    // Accepts comma-delimited bit indices and inclusive dash-separated ranges.
    std::bitset<32> AIConfiguration::parseCpuSetBitset(const std::string& bits,
                                             const std::bitset<32>& defaultValue)
    {
        for (char ch : bits)
        {
            if (!isdigit(static_cast<unsigned char>(ch)) && ch != '-' && ch != ',')
            {
                LOGWARN("parseCpuSetBitset: invalid character '%c' in cpuset string '%s'",
                        ch, bits.c_str());
                return defaultValue;
            }
        }

        std::bitset<32> value;
        std::istringstream stream(bits);
        std::string item;
        while (std::getline(stream, item, ','))
        {
            if (item.empty())
                continue;

            if (item.find('-') == std::string::npos)
            {
                unsigned long bit = strtoul(item.c_str(), nullptr, 10);
                if (bit < value.size())
                    value.set(bit);
                else
                    LOGWARN("parseCpuSetBitset: bit index %lu out of range, ignoring", bit);
            }
            else
            {
                bool foundDash = false;
                unsigned long lower = 0, upper = 0;
                for (char rch : item)
                {
                    if (rch == '-')
                    {
                        if (foundDash) break;
                        foundDash = true;
                    }
                    else
                    {
                        if (!foundDash) { lower = lower * 10u + static_cast<unsigned long>(rch - '0'); }
                        else            { upper = upper * 10u + static_cast<unsigned long>(rch - '0'); }
                    }
                }
                if ((lower > upper) || (upper >= value.size()))
                    LOGWARN("parseCpuSetBitset: range '%s' invalid or out of range, ignoring", item.c_str());
                else
                    for (unsigned long n = lower; n <= upper; n++)
                        value.set(n);
            }
        }
        return value;
    }

    // helper functions for json parsing
    std::list<std::string> parseStringArray(std::string key, std::string value)
    {
        std::list<std::string> result;

        Json::Value root;
        Json::Reader reader;

        if (!reader.parse(value, root))
        {
            LOGERR("Failed to parse JSON for - %s", key.c_str());
            return {};
        }

        for (const auto &val : root)
        {
            result.push_back(val.asString());
        }

        return result;
    }

    std::list<int> parseIntArray(std::string value)
    {
        std::list<int> result;

        Json::Value root;
        Json::Reader reader;

        if (!reader.parse(value, root))
        {
            LOGERR("Failed to parse JSON for int array");
            return {};
        }

        for (const auto &val : root)
        {
            result.push_back(val.asInt());
        }

        return result;
    }

    std::map<std::string, size_t> parseIonLimits(std::string value)
    {
        std::map<std::string, size_t> result;

        Json::Value root;
        Json::Reader reader;

        if (!reader.parse(value, root))
        {
            LOGERR("Failed to parse JSON for ionLimits");
            return {};
        }

        for (const auto &key : root.getMemberNames())
        {
            result[key] = root[key].asUInt();
        }

        return result;
    }

    void AIConfiguration::printAIConfiguration()
    {
         // Output parsed values for verification using member variables

        std::string vpuBlacklistStr = "";
        for (const auto &item : mVpuAccessBlacklist)
        {
            vpuBlacklistStr += item + " ";
        }

        std::string dbusAppsStr = "";
        for (const auto &item : mAppsRequiringDBus)
        {
            dbusAppsStr += item + " ";
        }

        std::string mapiPortsStr = "";
        for (const auto &port : mMapiPorts)
        {
            mapiPortsStr += std::to_string(port) + " ";
        }

        std::string preloadsStr = "";
        for (const auto &lib : mPreloads)
        {
            preloadsStr += lib + " ";
        }

        std::string envsStr = "";
        for (const auto &env : mEnvVariables)
        {
            envsStr += env + " ";
        }

        LOGINFO("consoleLogCap: %zu", mConsoleLogCap);
        LOGINFO("appsCpuSet: 0x%08lx", mAppsCpuSet.to_ulong());
        LOGINFO("nonHomeAppMemoryLimit: %zd", mNonHomeAppMemoryLimit);
        LOGINFO("nonHomeAppGpuLimit: %zd", mNonHomeAppGpuLimit);
        LOGINFO("vpuAccessBlacklist: %s", vpuBlacklistStr.c_str());
        LOGINFO("appsRequiringDBus: %s", dbusAppsStr.c_str());
        LOGINFO("mapiPorts: %s", mapiPortsStr.c_str());
        LOGINFO("resourceManagerClientEnabled: %s", mResourceManagerClientEnabled ? "true" : "false");
        LOGINFO("gstreamerRegistryEnabled: %s", mGstreamerRegistryEnabled ? "true" : "false");
        LOGINFO("svpEnabled: %s", mSvpEnabled ? "true" : "false");
        LOGINFO("enableUsbMassStorage: %s", mEnableUsbMassStorage ? "true" : "false");
        LOGINFO("ipv6Enabled: %s", mIPv6Enabled ? "true" : "false");
        LOGINFO("ionHeapDefaultQuota: %zu", mIonHeapDefaultQuota);
        LOGINFO("dialServerPort: %u", static_cast<unsigned int>(mDialServerPort));
        LOGINFO("dialServerPathPrefix: %s", mDialServerPathPrefix.c_str());
        LOGINFO("dialUsn: %s", mDialUsn.c_str());
        LOGINFO("ionHeapQuotas:");
        for (const auto &pair : mIonHeapQuotas)
        {
            LOGINFO("  Name: %s, Limit: %zu", pair.first.c_str(), pair.second);
        }
        LOGINFO("preloads: %s", preloadsStr.c_str());
        LOGINFO("envVariables: %s", envsStr.c_str());
    }

    void AIConfiguration::readFromYamlConfigFile(const std::string& runtimeConfigFile)
    {
        if (runtimeConfigFile.empty()) {
            LOGINFO("runtimeConfigFile is empty; skipping YAML runtime config load");
            return;
        }

        const char* configPath = runtimeConfigFile.c_str();
        struct stat st{};
        if (0 != ::stat(configPath, &st)) {
            LOGWARN("Configured runtime YAML file %s not found; skipping YAML runtime config overrides", configPath);
            return;
        }
        LOGINFO("AIConfiguration reading from YAML at %s", configPath);

        try {
            YAML::Node root = YAML::LoadFile(configPath);

            if (!root || !root.IsMap()) {
                LOGWARN("Invalid YAML format: root must be a mapping");
                return;
            }

            const YAML::Node preloads = root["preloads"];
            if (preloads.IsDefined() && preloads.IsSequence()) {
                LOGINFO("preloads (merging with defaults):");
                std::set<std::string> preloadSet(mPreloads.begin(), mPreloads.end());
                for (const auto& item : preloads) {
                    try {
                        std::string val = item.as<std::string>();
                        if (0 == preloadSet.count(val)) {
                            mPreloads.push_back(val);
                            preloadSet.insert(val);
                            LOGINFO("  %s", val.c_str());
                        }
                    } catch (const YAML::BadConversion& ex) {
                        LOGWARN("Invalid preload entry in YAML, skipping: %s", ex.what());
                    }
                }
            }

            YAML::Node envVariablesNode = root["envVariables"];
            if (envVariablesNode.IsDefined() && envVariablesNode.IsSequence()) {
                LOGINFO("envVariables (merging with defaults):");
                std::set<std::string> envSet(mEnvVariables.begin(), mEnvVariables.end());
                for (const auto& n : envVariablesNode) {
                    try {
                        std::string val = n.as<std::string>();
                        if (0 == envSet.count(val)) {
                            mEnvVariables.push_back(val);
                            envSet.insert(val);
                            LOGINFO("  %s", val.c_str());
                        }
                    } catch (const YAML::BadConversion& ex) {
                        LOGWARN("Invalid envVariables entry in YAML, skipping: %s", ex.what());
                    }
                }
            }
            YAML::Node enableSvpNode = root["enableSvp"];
            if (enableSvpNode.IsDefined() && enableSvpNode.IsScalar()) {
                // Use the existing value of mSvpEnabled as a fallback to avoid exceptions on bad input
                mSvpEnabled = enableSvpNode.as<bool>(mSvpEnabled);
                LOGINFO("enableSvp: %s", mSvpEnabled ? "true" : "false");
            }
            {
                const YAML::Node memoryLimitNode = root["memoryLimit"];
                if (memoryLimitNode.IsDefined() && memoryLimitNode.IsScalar()) {
                    try {
                        const uint64_t memoryLimitValue = memoryLimitNode.as<uint64_t>();
                        if (static_cast<uint64_t>(SSIZE_MAX) < memoryLimitValue) {
                            LOGWARN("memoryLimit value %" PRIu64 " exceeds SSIZE_MAX; ignoring", memoryLimitValue);
                        } else {
                            mNonHomeAppMemoryLimit = static_cast<ssize_t>(memoryLimitValue);
                            LOGINFO("memoryLimit: %zd", mNonHomeAppMemoryLimit);
                        }
                    } catch (const YAML::BadConversion& e) {
                        LOGWARN("Invalid value for memoryLimit in YAML: %s", e.what());
                    }
                }
            }
            {
                YAML::Node gpuNode = root["gpuMemoryLimit"];
                if (gpuNode.IsDefined()) {
                    if (gpuNode.IsScalar()) {
                        try {
                            const uint64_t gpuMemoryLimitValue = gpuNode.as<uint64_t>();
                            if (static_cast<uint64_t>(SSIZE_MAX) < gpuMemoryLimitValue) {
                                LOGWARN("gpuMemoryLimit value %" PRIu64 " exceeds SSIZE_MAX; ignoring", gpuMemoryLimitValue);
                            } else {
                                mNonHomeAppGpuLimit = static_cast<ssize_t>(gpuMemoryLimitValue);
                                LOGINFO("gpuMemoryLimit: %zd", mNonHomeAppGpuLimit);
                            }
                        } catch (const YAML::BadConversion& e) {
                            LOGWARN("Invalid gpuMemoryLimit value in YAML: %s", e.what());
                        }
                    } else {
                        LOGWARN("Invalid YAML type for gpuMemoryLimit: expected scalar");
                    }
                }
            }
            {
                YAML::Node ionDefaultQuotaNode = root["ionDefaultQuota"];
                if (ionDefaultQuotaNode && ionDefaultQuotaNode.IsScalar()) {
                    try {
                        mIonHeapDefaultQuota = ionDefaultQuotaNode.as<size_t>();
                        LOGINFO("ionDefaultQuota: %zu", mIonHeapDefaultQuota);
                    } catch (const YAML::BadConversion &e) {
                        LOGWARN("Invalid value for ionDefaultQuota in YAML configuration: %s", e.what());
                    }
                } else if (ionDefaultQuotaNode) {
                    LOGWARN("ionDefaultQuota is present in YAML configuration but is not a scalar value");
                }
            }
            YAML::Node svpNode = root["svpfiles"];
            if (svpNode.IsDefined() && svpNode.IsSequence()) {
                mSvpFiles.clear();
                LOGINFO("svpfiles:");
                for (const auto& n : svpNode) {
                    try {
                        std::string val = n.as<std::string>();
                        mSvpFiles.push_back(val);
                        LOGINFO("  %s", val.c_str());
                    } catch (const YAML::BadConversion& e) {
                        LOGWARN("Invalid entry in svpfiles sequence in YAML configuration: %s", e.what());
                    }
                }
            }

            //printAIConfiguration();

        } catch (const std::exception& ex) {
            LOGWARN("Error parsing YAML: %s", ex.what());
        }
    }

    void AIConfiguration::readFromConfigFile()
    {
        LOGINFO("AIConfiguration reading from config file at %s", AICONFIGURATION_JSON_PATH);
        std::ifstream configFile(AICONFIGURATION_JSON_PATH);
        if (!configFile.is_open())
        {
            LOGERR("Failed to open config file at %s", AICONFIGURATION_JSON_PATH);
            LOGINFO("Populating custom values for AIConfiguration from readFromCustomData()");
            readFromCustomData();
            return;
        }

        Json::Value root;
        Json::CharReaderBuilder builder;
        builder["allowComments"] = true;   // permit // and /* */ comments
        builder["strictRoot"]    = false;
        std::string errs;
        if (!Json::parseFromStream(builder, configFile, &root, &errs))
        {
            LOGERR("JSON parse error in '%s': %s", AICONFIGURATION_JSON_PATH, errs.c_str());
            readFromCustomData();
            return;
        }
        configFile.close();

        // Helper: safe nested-object access — returns Json::nullValue on miss.
        auto getObj = [](const Json::Value& v, const char* k) -> const Json::Value& {
            static const Json::Value kNull(Json::nullValue);
            return (v.isObject() && v.isMember(k)) ? v[k] : kNull;
        };

        const Json::Value& apps    = getObj(root,  "apps");
        const Json::Value& limits  = getObj(apps,  "limits");
        const Json::Value& mem     = getObj(limits, "memory");
        const Json::Value& gpu     = getObj(limits, "gpumem");
        const Json::Value& ion     = getObj(limits, "ion");
        const Json::Value& svp     = getObj(apps,  "svp");
        const Json::Value& dial    = getObj(root,  "dial");
        const Json::Value& dialSrv = getObj(dial,  "server");

        // ---- memory limits -----------------------------------------------
        if (mem["defaultLimitBytes"].isInt64() || mem["defaultLimitBytes"].isInt() || mem["defaultLimitBytes"].isUInt())
            mNonHomeAppMemoryLimit = static_cast<ssize_t>(mem["defaultLimitBytes"].asInt64());

        // ---- GPU memory limits -------------------------------------------
        if (gpu["defaultLimitBytes"].isInt64() || gpu["defaultLimitBytes"].isInt() || gpu["defaultLimitBytes"].isUInt())
            mNonHomeAppGpuLimit = static_cast<ssize_t>(gpu["defaultLimitBytes"].asInt64());

        // ---- console log cap ---------------------------------------------
        if (limits["consoleLogBytes"].isInt64() || limits["consoleLogBytes"].isInt() || limits["consoleLogBytes"].isUInt())
            mConsoleLogCap = static_cast<size_t>(limits["consoleLogBytes"].asUInt64());

        // ---- CPU set — integer bitmask or range string ("0-3,8-11") ------
        const Json::Value& cpuset = limits["cpuset"];
        if (cpuset.isUInt() || cpuset.isInt())
            mAppsCpuSet = std::bitset<32>(static_cast<unsigned long>(cpuset.asUInt()));
        else if (cpuset.isString())
            mAppsCpuSet = parseCpuSetBitset(cpuset.asString(), mAppsCpuSet);

        // ---- ION heap quotas ---------------------------------------------
        if (ion["defaultLimitBytes"].isInt64() || ion["defaultLimitBytes"].isInt() || ion["defaultLimitBytes"].isUInt())
            mIonHeapDefaultQuota = static_cast<size_t>(ion["defaultLimitBytes"].asUInt64());
        const Json::Value& heaps = getObj(ion, "heapsLimitBytes");
        if (heaps.isObject())
            for (const auto& name : heaps.getMemberNames())
                if (heaps[name].isUInt64() || heaps[name].isUInt() || heaps[name].isInt())
                    mIonHeapQuotas[name] = static_cast<size_t>(heaps[name].asUInt64());

        // ---- feature flags -----------------------------------------------
        if (apps["enableIPv6"].isBool())
            mIPv6Enabled = apps["enableIPv6"].asBool();
        if (getObj(svp, "enable").isBool())
            mSvpEnabled = getObj(svp, "enable").asBool();
        if (getObj(getObj(apps, "usbMassStorage"), "enable").isBool())
            mEnableUsbMassStorage = getObj(getObj(apps, "usbMassStorage"), "enable").asBool();
        if (getObj(getObj(apps, "resourceManagement"), "enabled").isBool())
            mResourceManagerClientEnabled = getObj(getObj(apps, "resourceManagement"), "enabled").asBool();
        if (getObj(getObj(apps, "gstreamer"), "registryEnabled").isBool())
            mGstreamerRegistryEnabled = getObj(getObj(apps, "gstreamer"), "registryEnabled").asBool();

        // ---- SVP files ---------------------------------------------------
        if (svp["files"].isArray())
            for (const auto& f : svp["files"])
                if (f.isString()) mSvpFiles.push_back(f.asString());

        // ---- default log levels ------------------------------------------
        const Json::Value& logLevels = getObj(apps, "defaultAllowedLogLevels");
        if (logLevels.isArray() && !logLevels.empty())
        {
            mDefaultAllowedLogLevels.clear();
            for (const auto& l : logLevels)
                if (l.isString()) mDefaultAllowedLogLevels.push_back(l.asString());
        }

        // ---- MAPI ports --------------------------------------------------
        const Json::Value& mapiPorts = getObj(getObj(apps, "mapi"), "ports");
        if (mapiPorts.isArray())
            for (const auto& p : mapiPorts)
                if (p.isInt()) mMapiPorts.push_back(p.asInt());

        // ---- app allow-lists --------------------------------------------
        if (apps["requireDBus"].isArray())
            for (const auto& a : apps["requireDBus"])
                if (a.isString()) mAppsRequiringDBus.push_back(a.asString());
        if (apps["vpuAccessBlacklist"].isArray())
            for (const auto& a : apps["vpuAccessBlacklist"])
                if (a.isString()) mVpuAccessBlacklist.push_back(a.asString());

        // ---- preloads / extra env vars -----------------------------------
        if (root["ldPreload"].isArray())
            for (const auto& p : root["ldPreload"])
                if (p.isString()) mPreloads.push_back(p.asString());
        if (apps["extraEnvVars"].isArray())
            for (const auto& e : apps["extraEnvVars"])
                if (e.isString()) mEnvVariables.push_back(e.asString());

        // ---- DIAL --------------------------------------------------------
        if (dialSrv["port"].isInt())
            mDialServerPort = static_cast<in_port_t>(dialSrv["port"].asInt());
        if (dialSrv["prefix"].isString())
            mDialServerPathPrefix = dialSrv["prefix"].asString();
        if (dial["usn"].isString())
            mDialUsn = dial["usn"].asString();

        printAIConfiguration();
    }
} /* namespace Plugin */
} /* namespace WPEFramework */
