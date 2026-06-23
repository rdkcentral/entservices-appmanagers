/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
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
 *
 *
 */
#pragma once

// @stubgen:skip

#include <string>
#include <memory>
#include <map>
#include <vector>
#include <set>
#include <mutex>

namespace packagemanager
{

    enum Result : uint8_t
    {
        SUCCESS,
        FAILED,
        VERSION_MISMATCH,
        PERSISTENCE_FAILURE,
        VERIFICATION_FAILURE
    };

    typedef enum : uint8_t
    {
        UNKNOWN = 0,
        INTERACTIVE,
        SYSTEM
    } ApplicationType;

    struct ConfigMetaData
    {
        bool dial;
        bool wanLanAccess;
        bool thunder;
        int32_t systemMemoryLimit;
        int32_t gpuMemoryLimit;
        std::vector<std::string> envVars;
        uint32_t userId;
        uint32_t groupId;
        uint32_t dataImageSize;

        bool resourceManagerClientEnabled;
        std::string dialId;
        std::string command;
        ApplicationType appType;
        std::string appPath;
        std::string runtimePath;

        std::string logFilePath;
        uint32_t logFileMaxSize;
        std::string logLevels; // json array of strings
        bool mapi;
        std::set<std::string> fkpsFiles;
        std::string capabilities;
        std::string runtimeType;
        std::string mimeType;

        std::string fireboltVersion;
        bool enableDebugger;
        std::string ralfPkgPath;
    };

    typedef std::pair<std::string, std::string> ConfigMetadataKey;
    typedef std::map<ConfigMetadataKey, ConfigMetaData> ConfigMetadataArray;

    typedef std::pair<std::string, std::string> NameValue;
    typedef std::vector<NameValue> NameValues;
    class IPackageImplDummy
    {
    public:
        virtual ~IPackageImplDummy() = default;

        virtual Result Initialize(const std::string &configStr, ConfigMetadataArray &aConfigMetadata) { 
            ConfigMetaData configMetadata;
            configMetadata.dial = true; 
            configMetadata.wanLanAccess = true;
            configMetadata.thunder = true;
            configMetadata.systemMemoryLimit = 128888000;
            configMetadata.gpuMemoryLimit = -1;
            configMetadata.userId = 1000;
            configMetadata.groupId = 1001;
            configMetadata.dataImageSize = 31457280;
            configMetadata.appPath = "/opt/YouTube";
            configMetadata.appType = packagemanager::ApplicationType::INTERACTIVE;
            configMetadata.fkpsFiles = {"file1","file2","file3"};
            configMetadata.capabilities = "dial-app,wan-lan,thunder,fkps";
            configMetadata.runtimeType = "";
            configMetadata.mimeType = "application/vnd.rdk-app.dac.native";
            
            ConfigMetadataKey key {"YouTube", "100.1.24"};

            aConfigMetadata.insert({key, configMetadata});
            
            return SUCCESS; 
        }

        virtual Result Install(const std::string &packageId, const std::string &version, const NameValues &additionalMetadata, const std::string &fileLocator, ConfigMetaData &configMetadata) {
            (void)version;
            (void)additionalMetadata;
            (void)fileLocator;

            if (packageId == "MismatchApp") {
                return VERSION_MISMATCH;
            }
            if (packageId == "PersistFailApp") {
                return PERSISTENCE_FAILURE;
            }
            if (packageId == "VerifyFailApp") {
                return VERIFICATION_FAILURE;
            }

            configMetadata.dial = true;
            configMetadata.wanLanAccess = true;
            configMetadata.thunder = true;
            configMetadata.systemMemoryLimit = 128888000;
            configMetadata.gpuMemoryLimit = -1;
            configMetadata.userId = 1000;
            configMetadata.groupId = 1001;
            configMetadata.dataImageSize = 31457280;
            configMetadata.appPath = "/opt/" + packageId;
            configMetadata.appType = packagemanager::ApplicationType::INTERACTIVE;
            configMetadata.fkpsFiles = {"file1", "file2", "file3"};
            configMetadata.capabilities = "dial-app,wan-lan,thunder,fkps";
            configMetadata.runtimeType = "";
            configMetadata.mimeType = "application/vnd.rdk-app.dac.native";
            return SUCCESS;
        }
        virtual Result Uninstall(const std::string &packageId) { return SUCCESS; }

        virtual Result Lock(const std::string &packageId, const std::string &version, std::string &unpackedPath, ConfigMetaData &configMetadata, NameValues &additionalLocks) { return SUCCESS; }
        virtual Result Unlock(const std::string &packageId, const std::string &version) { return SUCCESS; }

        // XXX: Below THREE functions will be removed after RDK-M is updated, so don't need to time the changes in RDK-M
        virtual Result GetLockInfo(const std::string &packageId, const std::string &version, std::string &unpackedPath, bool &locked) { return SUCCESS; }
        virtual Result GetList(std::string &packageList) { return SUCCESS; }
        virtual Result Lock(const std::string &packageId, const std::string &version, std::string &unpackedPath, ConfigMetaData &configMetadata) { return SUCCESS; }
        
        virtual Result GetFileMetadata(const std::string &fileLocator, std::string &packageId, std::string &version, ConfigMetaData &configMetadata) { return SUCCESS; }

        /**
         * Returns the active IPackageImpl-like instance used by PackageManagerImplementation
         * when built with `-DUNIT_TEST`. By default this is a fresh IPackageImplDummy whose
         * methods return canned values. Tests can override this with a gmock mock (see
         * Tests/mocks/IPackageImplMock.h) by calling setInstance(...).
         */
        static std::shared_ptr<packagemanager::IPackageImplDummy> instance() {
            std::lock_guard<std::mutex> lock(overrideMutex());
            if (overrideSlot()) {
                return overrideSlot();
            }
            return std::make_shared<packagemanager::IPackageImplDummy>();
        }

        /**
         * Install a custom IPackageImplDummy (typically an IPackageImplMock derived
         * via gmock) that subsequent calls to instance() will return. Call
         * clearInstance() in the test's TearDown to restore the default behavior.
         */
        static void setInstance(const std::shared_ptr<packagemanager::IPackageImplDummy>& override) {
            std::lock_guard<std::mutex> lock(overrideMutex());
            overrideSlot() = override;
        }

        /** Remove any previously installed override. Safe to call when none is set. */
        static void clearInstance() {
            std::lock_guard<std::mutex> lock(overrideMutex());
            overrideSlot().reset();
        }

    private:
        // Function-local statics avoid needing a separate .cpp for the storage and are
        // safe under C++17's guaranteed initialization-order rules for static locals.
        static std::shared_ptr<packagemanager::IPackageImplDummy>& overrideSlot() {
            static std::shared_ptr<packagemanager::IPackageImplDummy> s_override;
            return s_override;
        }
        static std::mutex& overrideMutex() {
            static std::mutex s_mutex;
            return s_mutex;
        }
    };

}

