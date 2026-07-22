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


#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <fstream>
#include <vector>
#include <cstdio>
#include <cerrno>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <json/json.h>

#define TEST_LOG(x, ...) fprintf(stderr, "\033[1;32m[%s:%d](%s)<PID:%d><TID:%d>" x "\n\033[0m", \
    __FILE__, __LINE__, __FUNCTION__, getpid(), (pid_t)syscall(SYS_gettid), ##__VA_ARGS__); fflush(stderr);

// Keep RuntimeConfig ABI consistent in this TU to prevent weak-symbol
#ifndef RUNTIME_CONFIG
#define RUNTIME_CONFIG
namespace WPEFramework {
namespace Exchange {
struct RuntimeConfig {
    bool dial;
    bool wanLanAccess;
    bool thunder;
    int32_t systemMemoryLimit;
    int32_t gpuMemoryLimit;
    std::string envVariables;
    uint32_t userId;
    uint32_t groupId;
    uint32_t dataImageSize;

    bool resourceManagerClientEnabled;
    std::string dialId;
    std::string command;
    std::string appType;
    std::string appPath;
    std::string runtimePath;

    std::string logFilePath;
    uint32_t logFileMaxSize;
    std::string logLevels;
    bool mapi;
    std::string fkpsFiles;
    std::string capabilities;
    std::string ralfPkgPath;

    std::string fireboltVersion;
    bool enableDebugger;
    std::string unpackedPath;

    ~RuntimeConfig() = default;
};
} // namespace Exchange
} // namespace WPEFramework
#endif


// Include ralf support header
#include "ralf/RalfSupport.h"
#include "ralf/RalfPackageBuilder.h"
// #define private public lets the test accessor reach private methods of
// RalfOCIConfigGenerator without any change to the production header.
#define private public
#include "ralf/RalfOCIConfigGenerator.h"
#undef private
#include "ralf/RalfConstants.h"
#include "ralf/OCISpecConstants.h"
#include "WrapsMock.h"

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

// Forward-declare real syscalls used in mock forwarding
extern "C" int __real_open(const char* pathname, int flags, int mode=0);

// ─────────────────────────────────────────────────────────────────────────────
// Helper: write a text file
// ─────────────────────────────────────────────────────────────────────────────
static bool writeTextFile(const std::string &path, const std::string &content)
{
    std::ofstream f(path);
    if (!f.is_open())
        return false;
    f << content;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Test suite for ralf::parseMemorySize
// ─────────────────────────────────────────────────────────────────────────────

class RalfParseSizeTest : public ::testing::Test {};

/* Test Case: ParseMemorySize_EmptyString
 * Verifies that an empty string returns 0.
 */
TEST_F(RalfParseSizeTest, ParseMemorySize_EmptyString)
{
    TEST_LOG("Testing parseMemorySize with empty string");
    EXPECT_EQ(0u, ralf::parseMemorySize(""));
}

/* Test Case: ParseMemorySize_PlainBytes
 * Verifies that a plain integer (no suffix) is returned as-is.
 */
TEST_F(RalfParseSizeTest, ParseMemorySize_PlainBytes)
{
    TEST_LOG("Testing parseMemorySize with plain integer");
    EXPECT_EQ(1024u, ralf::parseMemorySize("1024"));
}

/* Test Case: ParseMemorySize_Kilobytes
 * Verifies that "K" suffix returns value * 1024.
 */
TEST_F(RalfParseSizeTest, ParseMemorySize_Kilobytes)
{
    TEST_LOG("Testing parseMemorySize with K suffix");
    EXPECT_EQ(512u * 1024u, ralf::parseMemorySize("512K"));
}

/* Test Case: ParseMemorySize_KilobytesLowercase
 * Verifies that lowercase "k" suffix is accepted.
 */
TEST_F(RalfParseSizeTest, ParseMemorySize_KilobytesLowercase)
{
    TEST_LOG("Testing parseMemorySize with lowercase k suffix");
    EXPECT_EQ(1u * 1024u, ralf::parseMemorySize("1k"));
}

/* Test Case: ParseMemorySize_KilobytesWith_B
 * Verifies that "KB" suffix returns value * 1024.
 */
TEST_F(RalfParseSizeTest, ParseMemorySize_KilobytesWith_B)
{
    TEST_LOG("Testing parseMemorySize with KB suffix");
    EXPECT_EQ(4u * 1024u, ralf::parseMemorySize("4KB"));
}

/* Test Case: ParseMemorySize_Megabytes
 * Verifies that "M" suffix returns value * 1024 * 1024.
 */
TEST_F(RalfParseSizeTest, ParseMemorySize_Megabytes)
{
    TEST_LOG("Testing parseMemorySize with M suffix");
    EXPECT_EQ(256u * 1024u * 1024u, ralf::parseMemorySize("256M"));
}

/* Test Case: ParseMemorySize_MegabytesLowercase
 * Verifies that lowercase "m" suffix is accepted.
 */
TEST_F(RalfParseSizeTest, ParseMemorySize_MegabytesLowercase)
{
    TEST_LOG("Testing parseMemorySize with lowercase m suffix");
    EXPECT_EQ(64u * 1024u * 1024u, ralf::parseMemorySize("64m"));
}

/* Test Case: ParseMemorySize_MegabytesWith_B
 * Verifies that "MB" suffix is accepted.
 */
TEST_F(RalfParseSizeTest, ParseMemorySize_MegabytesWith_B)
{
    TEST_LOG("Testing parseMemorySize with MB suffix");
    EXPECT_EQ(128u * 1024u * 1024u, ralf::parseMemorySize("128MB"));
}

/* Test Case: ParseMemorySize_Gigabytes
 * Verifies that "G" suffix returns value * 1024^3.
 */
TEST_F(RalfParseSizeTest, ParseMemorySize_Gigabytes)
{
    TEST_LOG("Testing parseMemorySize with G suffix");
    EXPECT_EQ(2u * 1024u * 1024u * 1024u, ralf::parseMemorySize("2G"));
}

/* Test Case: ParseMemorySize_GigabytesLowercase
 * Verifies that lowercase "g" suffix is accepted.
 */
TEST_F(RalfParseSizeTest, ParseMemorySize_GigabytesLowercase)
{
    TEST_LOG("Testing parseMemorySize with lowercase g suffix");
    EXPECT_EQ(1ULL * 1024u * 1024u * 1024u, ralf::parseMemorySize("1g"));
}

/* Test Case: ParseMemorySize_GigabytesWith_B
 * Verifies that "GB" suffix is accepted.
 */
TEST_F(RalfParseSizeTest, ParseMemorySize_GigabytesWith_B)
{
    TEST_LOG("Testing parseMemorySize with GB suffix");
    EXPECT_EQ(4ULL * 1024u * 1024u * 1024u, ralf::parseMemorySize("4GB"));
}

/* Test Case: ParseMemorySize_InvalidSuffix
 * Verifies that an invalid suffix (e.g. "T") returns 0.
 */
TEST_F(RalfParseSizeTest, ParseMemorySize_InvalidSuffix)
{
    TEST_LOG("Testing parseMemorySize with invalid suffix");
    EXPECT_EQ(0u, ralf::parseMemorySize("1T"));
}

/* Test Case: ParseMemorySize_InvalidString
 * Verifies that a non-numeric string returns 0.
 */
TEST_F(RalfParseSizeTest, ParseMemorySize_InvalidString)
{
    TEST_LOG("Testing parseMemorySize with non-numeric string");
    EXPECT_EQ(0u, ralf::parseMemorySize("abc"));
}

/* Test Case: ParseMemorySize_ZeroValue
 * Verifies that "0M" returns 0.
 */
TEST_F(RalfParseSizeTest, ParseMemorySize_ZeroValue)
{
    TEST_LOG("Testing parseMemorySize with zero value");
    EXPECT_EQ(0u, ralf::parseMemorySize("0M"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Test suite for ralf::serializeJsonNode
// ─────────────────────────────────────────────────────────────────────────────

class RalfSerializeJsonTest : public ::testing::Test {};

/* Test Case: SerializeJsonNode_EmptyObject
 * Verifies that an empty JSON object serializes to "{}".
 */
TEST_F(RalfSerializeJsonTest, SerializeJsonNode_EmptyObject)
{
    TEST_LOG("Testing serializeJsonNode with empty object");
    Json::Value node(Json::objectValue);
    std::string result = ralf::serializeJsonNode(node);
    EXPECT_EQ("{}", result);
}

/* Test Case: SerializeJsonNode_SimpleKeyValue
 * Verifies that a simple key-value pair is serialized correctly.
 */
TEST_F(RalfSerializeJsonTest, SerializeJsonNode_SimpleKeyValue)
{
    TEST_LOG("Testing serializeJsonNode with simple key-value");
    Json::Value node;
    node["key"] = "value";
    std::string result = ralf::serializeJsonNode(node);
    EXPECT_FALSE(result.empty());
    EXPECT_NE(std::string::npos, result.find("\"key\""));
    EXPECT_NE(std::string::npos, result.find("\"value\""));
}

/* Test Case: SerializeJsonNode_IntegerValue
 * Verifies that integer values are serialized correctly.
 */
TEST_F(RalfSerializeJsonTest, SerializeJsonNode_IntegerValue)
{
    TEST_LOG("Testing serializeJsonNode with integer value");
    Json::Value node;
    node["count"] = 42;
    std::string result = ralf::serializeJsonNode(node);
    EXPECT_NE(std::string::npos, result.find("42"));
}

/* Test Case: SerializeJsonNode_EmptyArray
 * Verifies that an empty JSON array serializes to "[]".
 */
TEST_F(RalfSerializeJsonTest, SerializeJsonNode_EmptyArray)
{
    TEST_LOG("Testing serializeJsonNode with empty array");
    Json::Value node(Json::arrayValue);
    std::string result = ralf::serializeJsonNode(node);
    EXPECT_EQ("[]", result);
}

/* Test Case: SerializeJsonNode_NestedObject
 * Verifies that a nested JSON object is serialized without extra whitespace.
 */
TEST_F(RalfSerializeJsonTest, SerializeJsonNode_NestedObject)
{
    TEST_LOG("Testing serializeJsonNode with nested object");
    Json::Value node;
    node["outer"]["inner"] = "data";
    std::string result = ralf::serializeJsonNode(node);
    EXPECT_FALSE(result.empty());
    // The compact serializer should not add newlines
    EXPECT_EQ(std::string::npos, result.find('\n'));
}

// ─────────────────────────────────────────────────────────────────────────────
// Test suite for ralf::JsonFromFile
// ─────────────────────────────────────────────────────────────────────────────

class RalfJsonFromFileTest : public ::testing::Test
{
protected:
    const std::string mTmpDir = "/tmp/ralf_l1_test";
    const std::string mValidJsonFile = mTmpDir + "/valid.json";
    const std::string mInvalidJsonFile = mTmpDir + "/invalid.json";
    const std::string mEmptyJsonFile = mTmpDir + "/empty.json";

    void SetUp() override
    {
        // Create temp directory
        mkdir(mTmpDir.c_str(), 0777);

        // Write a valid JSON file
        writeTextFile(mValidJsonFile, R"({"name":"test","version":1})");

        // Write an invalid JSON file
        writeTextFile(mInvalidJsonFile, "{ not valid json }}}");

        // Write an empty file
        writeTextFile(mEmptyJsonFile, "");
    }

    void TearDown() override
    {
        ::remove(mValidJsonFile.c_str());
        ::remove(mInvalidJsonFile.c_str());
        ::remove(mEmptyJsonFile.c_str());
        ::rmdir(mTmpDir.c_str());
    }
};

/* Test Case: JsonFromFile_ValidFile
 * Verifies that a valid JSON file is parsed correctly.
 */
TEST_F(RalfJsonFromFileTest, JsonFromFile_ValidFile)
{
    TEST_LOG("Testing JsonFromFile with valid JSON file");
    Json::Value root;
    bool result = ralf::JsonFromFile(mValidJsonFile, root);
    EXPECT_TRUE(result);
    EXPECT_EQ("test", root["name"].asString());
    EXPECT_EQ(1, root["version"].asInt());
}

/* Test Case: JsonFromFile_NonExistentFile
 * Verifies that a non-existent file returns false.
 */
TEST_F(RalfJsonFromFileTest, JsonFromFile_NonExistentFile)
{
    TEST_LOG("Testing JsonFromFile with non-existent file");
    Json::Value root;
    bool result = ralf::JsonFromFile("/tmp/does_not_exist_xyz_abc.json", root);
    EXPECT_FALSE(result);
}

/* Test Case: JsonFromFile_InvalidJson
 * Verifies that an invalid JSON file returns false.
 */
TEST_F(RalfJsonFromFileTest, JsonFromFile_InvalidJson)
{
    TEST_LOG("Testing JsonFromFile with invalid JSON");
    Json::Value root;
    bool result = ralf::JsonFromFile(mInvalidJsonFile, root);
    EXPECT_FALSE(result);
}

/* Test Case: JsonFromFile_EmptyFile
 * Verifies that an empty file returns false.
 */
TEST_F(RalfJsonFromFileTest, JsonFromFile_EmptyFile)
{
    TEST_LOG("Testing JsonFromFile with empty file");
    Json::Value root;
    bool result = ralf::JsonFromFile(mEmptyJsonFile, root);
    EXPECT_FALSE(result);
}

/* Test Case: JsonFromFile_EmptyPath
 * Verifies that an empty path returns false.
 */
TEST_F(RalfJsonFromFileTest, JsonFromFile_EmptyPath)
{
    TEST_LOG("Testing JsonFromFile with empty path");
    Json::Value root;
    bool result = ralf::JsonFromFile("", root);
    EXPECT_FALSE(result);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test suite for ralf::checkIfPathExists
// ─────────────────────────────────────────────────────────────────────────────

class RalfCheckPathTest : public ::testing::Test
{
protected:
    const std::string mTmpDir = "/tmp/ralf_check_path_test";
    const std::string mExistingFile = mTmpDir + "/existing.txt";

    void SetUp() override
    {
        mkdir(mTmpDir.c_str(), 0777);
        writeTextFile(mExistingFile, "test");
    }

    void TearDown() override
    {
        ::remove(mExistingFile.c_str());
        ::rmdir(mTmpDir.c_str());
    }
};

/* Test Case: CheckIfPathExists_ExistingDirectory
 * Verifies that an existing directory returns true.
 */
TEST_F(RalfCheckPathTest, CheckIfPathExists_ExistingDirectory)
{
    TEST_LOG("Testing checkIfPathExists with existing directory");
    EXPECT_TRUE(ralf::checkIfPathExists(mTmpDir));
}

/* Test Case: CheckIfPathExists_ExistingFile
 * Verifies that an existing file returns true.
 */
TEST_F(RalfCheckPathTest, CheckIfPathExists_ExistingFile)
{
    TEST_LOG("Testing checkIfPathExists with existing file");
    EXPECT_TRUE(ralf::checkIfPathExists(mExistingFile));
}

/* Test Case: CheckIfPathExists_NonExistentPath
 * Verifies that a non-existent path returns false.
 */
TEST_F(RalfCheckPathTest, CheckIfPathExists_NonExistentPath)
{
    TEST_LOG("Testing checkIfPathExists with non-existent path");
    EXPECT_FALSE(ralf::checkIfPathExists("/tmp/ralf_no_such_path_xyz_12345"));
}

/* Test Case: CheckIfPathExists_EmptyPath
 * Verifies that an empty path string returns false.
 */
TEST_F(RalfCheckPathTest, CheckIfPathExists_EmptyPath)
{
    TEST_LOG("Testing checkIfPathExists with empty path");
    EXPECT_FALSE(ralf::checkIfPathExists(""));
}

// ─────────────────────────────────────────────────────────────────────────────
// Test suite for ralf::create_directories
// ─────────────────────────────────────────────────────────────────────────────

class RalfCreateDirectoriesTest : public ::testing::Test
{
protected:
    const std::string mBaseDir = "/tmp/ralf_mkdir_test";

    void TearDown() override
    {
        // Cleanup created directories
        ralf::removeDirectoryRecursively(mBaseDir);
    }
};

/* Test Case: CreateDirectories_SingleLevel
 * Verifies that a single-level directory is created successfully.
 */
TEST_F(RalfCreateDirectoriesTest, CreateDirectories_SingleLevel)
{
    TEST_LOG("Testing create_directories for single level");
    std::string path = mBaseDir;
    bool result = ralf::create_directories(path);
    EXPECT_TRUE(result);
    EXPECT_TRUE(ralf::checkIfPathExists(path));
}

/* Test Case: CreateDirectories_MultiLevel
 * Verifies that a multi-level nested directory is created successfully.
 */
TEST_F(RalfCreateDirectoriesTest, CreateDirectories_MultiLevel)
{
    TEST_LOG("Testing create_directories for multi-level path");
    std::string path = mBaseDir + "/a/b/c";
    bool result = ralf::create_directories(path);
    EXPECT_TRUE(result);
    EXPECT_TRUE(ralf::checkIfPathExists(path));
}

/* Test Case: CreateDirectories_AlreadyExists
 * Verifies that calling create_directories on an existing directory succeeds.
 */
TEST_F(RalfCreateDirectoriesTest, CreateDirectories_AlreadyExists)
{
    TEST_LOG("Testing create_directories when directory already exists");
    ralf::create_directories(mBaseDir);
    // Calling again should not fail
    bool result = ralf::create_directories(mBaseDir);
    EXPECT_TRUE(result);
}

/* Test Case: CreateDirectories_EmptyPath
 * Verifies that an empty path returns false.
 */
TEST_F(RalfCreateDirectoriesTest, CreateDirectories_EmptyPath)
{
    TEST_LOG("Testing create_directories with empty path");
    bool result = ralf::create_directories("");
    EXPECT_FALSE(result);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test suite for ralf::parseRalPkgInfo
// ─────────────────────────────────────────────────────────────────────────────

class RalfParseRalPkgInfoTest : public ::testing::Test
{
protected:
    const std::string mTmpDir = "/tmp/ralf_pkg_info_test";
    const std::string mValidPkgInfoFile = mTmpDir + "/pkg_info.json";
    const std::string mMissingPackagesFile = mTmpDir + "/no_packages.json";
    const std::string mEmptyPackagesFile = mTmpDir + "/empty_packages.json";

    void SetUp() override
    {
        mkdir(mTmpDir.c_str(), 0777);

        // Valid package info JSON with two packages
        writeTextFile(mValidPkgInfoFile, R"({
            "packages": [
                {
                    "pkgMetaDataPath": "/data/ralf/pkg1/metadata.json",
                    "pkgMountPath": "/mnt/ralf/pkg1"
                },
                {
                    "pkgMetaDataPath": "/data/ralf/pkg2/metadata.json",
                    "pkgMountPath": "/mnt/ralf/pkg2"
                }
            ]
        })");

        // JSON without "packages" field
        writeTextFile(mMissingPackagesFile, R"({"other":"value"})");

        // JSON with empty packages array
        writeTextFile(mEmptyPackagesFile, R"({"packages":[]})");
    }

    void TearDown() override
    {
        ::remove(mValidPkgInfoFile.c_str());
        ::remove(mMissingPackagesFile.c_str());
        ::remove(mEmptyPackagesFile.c_str());
        ::rmdir(mTmpDir.c_str());
    }
};

/* Test Case: ParseRalPkgInfo_ValidFile
 * Verifies that valid package info is parsed and the correct number of packages is returned.
 */
TEST_F(RalfParseRalPkgInfoTest, ParseRalPkgInfo_ValidFile)
{
    TEST_LOG("Testing parseRalPkgInfo with valid file");
    std::vector<ralf::RalfPkgInfoPair> packages;
    bool result = ralf::parseRalPkgInfo(mValidPkgInfoFile, packages);
    EXPECT_TRUE(result);
    EXPECT_EQ(2u, packages.size());
    EXPECT_EQ("/data/ralf/pkg1/metadata.json", packages[0].first);
    EXPECT_EQ("/mnt/ralf/pkg1", packages[0].second);
    EXPECT_EQ("/data/ralf/pkg2/metadata.json", packages[1].first);
    EXPECT_EQ("/mnt/ralf/pkg2", packages[1].second);
}

/* Test Case: ParseRalPkgInfo_MissingPackagesField
 * Verifies that a JSON without a "packages" field returns false.
 */
TEST_F(RalfParseRalPkgInfoTest, ParseRalPkgInfo_MissingPackagesField)
{
    TEST_LOG("Testing parseRalPkgInfo with missing packages field");
    std::vector<ralf::RalfPkgInfoPair> packages;
    bool result = ralf::parseRalPkgInfo(mMissingPackagesFile, packages);
    EXPECT_FALSE(result);
    EXPECT_TRUE(packages.empty());
}

/* Test Case: ParseRalPkgInfo_EmptyPackagesArray
 * Verifies that an empty packages array succeeds with zero packages.
 */
TEST_F(RalfParseRalPkgInfoTest, ParseRalPkgInfo_EmptyPackagesArray)
{
    TEST_LOG("Testing parseRalPkgInfo with empty packages array");
    std::vector<ralf::RalfPkgInfoPair> packages;
    bool result = ralf::parseRalPkgInfo(mEmptyPackagesFile, packages);
    EXPECT_TRUE(result);
    EXPECT_TRUE(packages.empty());
}

/* Test Case: ParseRalPkgInfo_NonExistentFile
 * Verifies that a non-existent file returns false.
 */
TEST_F(RalfParseRalPkgInfoTest, ParseRalPkgInfo_NonExistentFile)
{
    TEST_LOG("Testing parseRalPkgInfo with non-existent file");
    std::vector<ralf::RalfPkgInfoPair> packages;
    bool result = ralf::parseRalPkgInfo("/tmp/no_such_ralf_file_xyz.json", packages);
    EXPECT_FALSE(result);
}

/* Test Case: ParseRalPkgInfo_EmptyFilePath
 * Verifies that an empty file path returns false.
 */
TEST_F(RalfParseRalPkgInfoTest, ParseRalPkgInfo_EmptyFilePath)
{
    TEST_LOG("Testing parseRalPkgInfo with empty file path");
    std::vector<ralf::RalfPkgInfoPair> packages;
    bool result = ralf::parseRalPkgInfo("", packages);
    EXPECT_FALSE(result);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test suite for ralf::RalfPackageBuilder::unmountOverlayfsIfExists
// ─────────────────────────────────────────────────────────────────────────────

class RalfPackageBuilderTest : public ::testing::Test
{
protected:
    ralf::RalfPackageBuilder mBuilder;
};

/* Test Case: UnmountOverlayfsIfExists_NonExistentPath
 * Verifies that unmountOverlayfsIfExists returns false when the overlayfs mount path
 * does not exist (path check fails, nothing to unmount).
 */
TEST_F(RalfPackageBuilderTest, UnmountOverlayfsIfExists_NonExistentPath)
{
    TEST_LOG("Testing unmountOverlayfsIfExists with non-existent path");
    // Path /tmp/ralf/non_existent_app_xyz/rootfs should not exist
    bool result = mBuilder.unmountOverlayfsIfExists("non_existent_app_xyz_12345");
    EXPECT_FALSE(result);
}

/* Test Case: UnmountOverlayfsIfExists_EmptyAppInstanceId
 * Verifies that unmountOverlayfsIfExists returns false for an empty app instance ID.
 * The resulting path (/tmp/ralf//rootfs) will not exist, so the path-check fails.
 */
TEST_F(RalfPackageBuilderTest, UnmountOverlayfsIfExists_EmptyAppInstanceId)
{
    TEST_LOG("Testing unmountOverlayfsIfExists with empty app instance ID");
    bool result = mBuilder.unmountOverlayfsIfExists("");
    EXPECT_FALSE(result);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test suite for ralf::unmountOverlayfs (direct function)
// ─────────────────────────────────────────────────────────────────────────────

class RalfUnmountOverlayfsTest : public ::testing::Test {};

/* Test Case: UnmountOverlayfs_NonExistentPath
 * Verifies that attempting to unmount a non-existent path returns false.
 */
TEST_F(RalfUnmountOverlayfsTest, UnmountOverlayfs_NonExistentPath)
{
    TEST_LOG("Testing unmountOverlayfs with non-existent path");
    bool result = ralf::unmountOverlayfs("/tmp/ralf_no_such_mount_xyz_99999");
    EXPECT_FALSE(result);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test suite for ralf::getGroupId
// ─────────────────────────────────────────────────────────────────────────────

class RalfGetGroupIdTest : public ::testing::Test {};

/* Test Case: GetGroupId_NonExistentGroup
 * Verifies that a non-existent group name returns false.
 */
TEST_F(RalfGetGroupIdTest, GetGroupId_NonExistentGroup)
{
    TEST_LOG("Testing getGroupId with non-existent group");
    uint32_t gid = 0;
    bool result = ralf::getGroupId("no_such_group_xyz_ralf_test_12345", gid);
    EXPECT_FALSE(result);
}

/* Test Case: GetGroupId_EmptyGroupName
 * Verifies that an empty group name returns false.
 */
TEST_F(RalfGetGroupIdTest, GetGroupId_EmptyGroupName)
{
    TEST_LOG("Testing getGroupId with empty group name");
    uint32_t gid = 0;
    bool result = ralf::getGroupId("", gid);
    EXPECT_FALSE(result);
}

/* Test Case: GetGroupId_RootGroup
 * Verifies that the "root" group (which always exists) is resolved successfully.
 */
TEST_F(RalfGetGroupIdTest, GetGroupId_RootGroup)
{
    TEST_LOG("Testing getGroupId with 'root' group");
    uint32_t gid = 99;
    bool result = ralf::getGroupId("root", gid);
    EXPECT_TRUE(result);
    EXPECT_EQ(0u, gid);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test suite for ralf::getRalfUserInfo
// ─────────────────────────────────────────────────────────────────────────────

class RalfGetUserInfoTest : public ::testing::Test {};

/* Test Case: GetRalfUserInfo_MissingUser
 * Verifies that getRalfUserInfo returns false when the "ralf" user does not exist
 * on the test host. This is the expected behaviour in a CI/unit-test environment.
 */
TEST_F(RalfGetUserInfoTest, GetRalfUserInfo_MissingUser)
{
    TEST_LOG("Testing getRalfUserInfo when ralf user is absent");
    uid_t uid = 0;
    gid_t gid = 0;
    // In a typical build/test environment the "ralf" system user will not exist.
    // The function should return false and leave uid/gid unchanged.
    bool result = ralf::getRalfUserInfo(uid, gid);
    // We only assert the negative path; if "ralf" user exists on the host, skip.
    if (!result)
    {
        EXPECT_EQ(0u, uid);
        EXPECT_EQ(0u, gid);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Test suite for ralf constants
// ─────────────────────────────────────────────────────────────────────────────

class RalfConstantsTest : public ::testing::Test {};

/* Test Case: RalfConstants_BaseDir
 * Verifies that RALF_APP_ROOTFS_DIR constant has the expected value.
 */
TEST_F(RalfConstantsTest, RalfConstants_BaseDir)
{
    TEST_LOG("Testing RALF_APP_ROOTFS_DIR constant");
    EXPECT_EQ("/tmp/ralf/", ralf::RALF_APP_ROOTFS_DIR);
}

/* Test Case: RalfConstants_OverlayfsType
 * Verifies that RALF_OVERLAYFS_TYPE constant has the expected value.
 */
TEST_F(RalfConstantsTest, RalfConstants_OverlayfsType)
{
    TEST_LOG("Testing RALF_OVERLAYFS_TYPE constant");
    EXPECT_EQ("overlay", ralf::RALF_OVERLAYFS_TYPE);
}

/* Test Case: RalfConstants_UserName
 * Verifies that RALF_USER_NAME constant has the expected value.
 */
TEST_F(RalfConstantsTest, RalfConstants_UserName)
{
    TEST_LOG("Testing RALF_USER_NAME constant");
    EXPECT_EQ("ralf", ralf::RALF_USER_NAME);
}

/* Test Case: RalfConstants_BaseOCISpecFile
 * Verifies that RALF_OCI_BASE_SPEC_FILE constant has the expected value.
 */
TEST_F(RalfConstantsTest, RalfConstants_BaseOCISpecFile)
{
    TEST_LOG("Testing RALF_OCI_BASE_SPEC_FILE constant");
    EXPECT_EQ("/usr/share/ralf/oci-base-spec.json", ralf::RALF_OCI_BASE_SPEC_FILE);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test suite for ralf::removeDirectoryRecursively
// ─────────────────────────────────────────────────────────────────────────────

class RalfRemoveDirectoryTest : public ::testing::Test {};

/* Test Case: RemoveDirectoryRecursively_NonExistentPath
 * Verifies that removing a non-existent directory returns false.
 */
TEST_F(RalfRemoveDirectoryTest, RemoveDirectoryRecursively_NonExistentPath)
{
    TEST_LOG("Testing removeDirectoryRecursively with non-existent path");
    bool result = ralf::removeDirectoryRecursively("/tmp/ralf_no_such_dir_xyz_99999");
    EXPECT_FALSE(result);
}

/* Test Case: RemoveDirectoryRecursively_ExistingNestedDirectory
 * Verifies that an existing nested directory and its contents are removed successfully.
 */
TEST_F(RalfRemoveDirectoryTest, RemoveDirectoryRecursively_ExistingNestedDirectory)
{
    TEST_LOG("Testing removeDirectoryRecursively with existing nested directory");
    const std::string base = "/tmp/ralf_rm_test_dir";
    const std::string nested = base + "/a/b";
    const std::string file = nested + "/test.txt";

    ralf::create_directories(nested);
    writeTextFile(file, "data");

    ASSERT_TRUE(ralf::checkIfPathExists(file));

    bool result = ralf::removeDirectoryRecursively(base);
    EXPECT_TRUE(result);
    EXPECT_FALSE(ralf::checkIfPathExists(base));
}

/* Test Case: RemoveDirectoryRecursively_EmptyDirectory
 * Verifies that removing an empty directory succeeds.
 */
TEST_F(RalfRemoveDirectoryTest, RemoveDirectoryRecursively_EmptyDirectory)
{
    TEST_LOG("Testing removeDirectoryRecursively with empty directory");
    const std::string dir = "/tmp/ralf_rm_empty_dir_test";
    mkdir(dir.c_str(), 0777);
    ASSERT_TRUE(ralf::checkIfPathExists(dir));

    bool result = ralf::removeDirectoryRecursively(dir);
    EXPECT_TRUE(result);
    EXPECT_FALSE(ralf::checkIfPathExists(dir));
}

// ─────────────────────────────────────────────────────────────────────────────
// Test suite for ralf::getDevNodeMajorMinor
// ─────────────────────────────────────────────────────────────────────────────

class RalfGetDevNodeMajorMinorTest : public ::testing::Test {};

/* Test Case: GetDevNodeMajorMinor_NonExistentPath
 * Verifies that getDevNodeMajorMinor returns false for a non-existent device path.
 */
TEST_F(RalfGetDevNodeMajorMinorTest, GetDevNodeMajorMinor_NonExistentPath)
{
    TEST_LOG("Testing getDevNodeMajorMinor with non-existent device path");
    unsigned int majorNum = 0, minorNum = 0;
    char devType = '\0';
    bool result = ralf::getDevNodeMajorMinor("/dev/ralf_no_such_device_xyz_99999", majorNum, minorNum, devType);
    EXPECT_FALSE(result);
}

// ─────────────────────────────────────────────────────────────────────────────
// Base fixture for tests that require WrapsImplMock
// ─────────────────────────────────────────────────────────────────────────────

class RalfMockedBaseTest : public ::testing::Test
{
protected:
    NiceMock<WrapsImplMock> *mWrapsImplMock = nullptr;

    void InstallMock()
    {
        mWrapsImplMock = new NiceMock<WrapsImplMock>;
        Wraps::setImpl(mWrapsImplMock);
    }

    void RemoveMock()
    {
        Wraps::setImpl(nullptr);
        delete mWrapsImplMock;
        mWrapsImplMock = nullptr;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Test suite for create_directories with non-zero uid/gid (chown path)
// ─────────────────────────────────────────────────────────────────────────────

class RalfCreateDirectoriesOwnershipTest : public RalfMockedBaseTest {};

/* Test Case: CreateDirectories_WithUidGid_ChownFails
 * Verifies that create_directories returns false when chown fails for a new directory.
 */
TEST_F(RalfCreateDirectoriesOwnershipTest, CreateDirectories_WithUidGid_ChownFails)
{
    TEST_LOG("Testing create_directories with non-zero uid/gid when chown fails");
    InstallMock();
    ON_CALL(*mWrapsImplMock, mkdir(_, _)).WillByDefault(Return(0));
    ON_CALL(*mWrapsImplMock, chown(_, _, _)).WillByDefault(Return(-1));

    bool result = ralf::create_directories("/tmp/ralf_ownership_test_fail", 1000, 1000);
    EXPECT_FALSE(result);

    RemoveMock();
}

/* Test Case: CreateDirectories_WithUidGid_ChownSucceeds
 * Verifies that create_directories returns true when mkdir and chown both succeed.
 */
TEST_F(RalfCreateDirectoriesOwnershipTest, CreateDirectories_WithUidGid_ChownSucceeds)
{
    TEST_LOG("Testing create_directories with non-zero uid/gid when chown succeeds");
    InstallMock();
    ON_CALL(*mWrapsImplMock, mkdir(_, _)).WillByDefault(Return(0));
    ON_CALL(*mWrapsImplMock, chown(_, _, _)).WillByDefault(Return(0));

    bool result = ralf::create_directories("/tmp/ralf_chown_ok_test", 1000, 1000);
    EXPECT_TRUE(result);

    RemoveMock();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test suite for ralf::generateOCIRootfs
// ─────────────────────────────────────────────────────────────────────────────

class RalfGenerateOCIRootfsTest : public RalfMockedBaseTest {};

/* Test Case: GenerateOCIRootfs_MountFails
 * Verifies that generateOCIRootfs returns false when the overlay mount call fails.
 */
TEST_F(RalfGenerateOCIRootfsTest, GenerateOCIRootfs_MountFails)
{
    TEST_LOG("Testing generateOCIRootfs when mount fails");
    InstallMock();
    ON_CALL(*mWrapsImplMock, mkdir(_, _)).WillByDefault(Return(0));
    ON_CALL(*mWrapsImplMock, mount(_, _, _, _, _))
        .WillByDefault(Invoke([](const char*, const char*, const char*, unsigned long, const void*) -> int {
            errno = EPERM;
            return -1;
        }));

    std::string ociRootfsPath;
    bool result = ralf::generateOCIRootfs("test_app_mount_fail_l1", "/mnt/layer1", 0, 0, ociRootfsPath);
    EXPECT_FALSE(result);

    RemoveMock();
}

/* Test Case: GenerateOCIRootfs_MountSucceeds
 * Verifies that generateOCIRootfs returns true and populates ociRootfsPath when mount succeeds.
 */
TEST_F(RalfGenerateOCIRootfsTest, GenerateOCIRootfs_MountSucceeds)
{
    TEST_LOG("Testing generateOCIRootfs when mount succeeds");
    InstallMock();
    ON_CALL(*mWrapsImplMock, mkdir(_, _)).WillByDefault(Return(0));
    ON_CALL(*mWrapsImplMock, mount(_, _, _, _, _)).WillByDefault(Return(0));

    std::string ociRootfsPath;
    bool result = ralf::generateOCIRootfs("test_app_mount_ok_l1", "/mnt/layer1", 0, 0, ociRootfsPath);
    EXPECT_TRUE(result);
    EXPECT_FALSE(ociRootfsPath.empty());
    EXPECT_NE(std::string::npos, ociRootfsPath.find("test_app_mount_ok_l1"));

    RemoveMock();
}

// ─────────────────────────────────────────────────────────────────────────────
// Extended tests for RalfPackageBuilder using WrapsImplMock
// ─────────────────────────────────────────────────────────────────────────────

class RalfPackageBuilderMockedTest : public RalfMockedBaseTest
{
protected:
    ralf::RalfPackageBuilder mBuilder;
    const std::string mTmpDir      = "/tmp/ralf_pkg_builder_mocked_test";
    const std::string mPkgInfoFile = mTmpDir + "/pkg_info.json";
    const std::string mPkgMetaFile = mTmpDir + "/pkg1_meta.json";

    void SetUp() override
    {
        // Create test files with real syscalls BEFORE installing mock
        mkdir(mTmpDir.c_str(), 0777);
        writeTextFile(mPkgInfoFile, R"({
            "packages": [
                {
                    "pkgMetaDataPath": ")" + mPkgMetaFile + R"(",
                    "pkgMountPath": "/mnt/ralf/pkg1"
                }
            ]
        })");
        writeTextFile(mPkgMetaFile, R"({"entryPoint":"/bin/app","packageType":"application"})");
        InstallMock();
    }

    void TearDown() override
    {
        // Remove mock first so file cleanup uses real syscalls
        RemoveMock();
        ::remove(mPkgInfoFile.c_str());
        ::remove(mPkgMetaFile.c_str());
        ::rmdir(mTmpDir.c_str());
    }
};

/* Test Case: UnmountOverlayfsIfExists_PathExists_UnmountSucceeds
 * Verifies that unmountOverlayfsIfExists returns true when the mount path exists
 * and the umount call succeeds.
 */
TEST_F(RalfPackageBuilderMockedTest, UnmountOverlayfsIfExists_PathExists_UnmountSucceeds)
{
    TEST_LOG("Testing unmountOverlayfsIfExists: path exists, umount succeeds");
    // stat returns 0 → checkIfPathExists returns true
    ON_CALL(*mWrapsImplMock, stat(_, _)).WillByDefault(Return(0));
    // umount succeeds
    ON_CALL(*mWrapsImplMock, umount(_)).WillByDefault(Return(0));
    // opendir returns nullptr → removeDirectoryRecursively fails gracefully (logged as warning)
    ON_CALL(*mWrapsImplMock, opendir(_)).WillByDefault(Return(nullptr));

    bool result = mBuilder.unmountOverlayfsIfExists("test_app_unmount_ok");
    EXPECT_TRUE(result);
}

/* Test Case: UnmountOverlayfsIfExists_PathExists_UnmountFails_EINVAL
 * Verifies that unmountOverlayfsIfExists returns true when umount fails with EINVAL
 * (path not currently mounted — treated as non-fatal).
 */
TEST_F(RalfPackageBuilderMockedTest, UnmountOverlayfsIfExists_PathExists_UnmountFails_EINVAL)
{
    TEST_LOG("Testing unmountOverlayfsIfExists: path exists, umount fails with EINVAL (non-fatal)");
    ON_CALL(*mWrapsImplMock, stat(_, _)).WillByDefault(Return(0));
    ON_CALL(*mWrapsImplMock, umount(_))
        .WillByDefault(Invoke([](const char*) -> int {
            errno = EINVAL;
            return -1;
        }));
    ON_CALL(*mWrapsImplMock, opendir(_)).WillByDefault(Return(nullptr));

    bool result = mBuilder.unmountOverlayfsIfExists("test_app_einval");
    EXPECT_TRUE(result);
}

/* Test Case: UnmountOverlayfsIfExists_PathExists_UnmountFails_OtherError
 * Verifies that unmountOverlayfsIfExists returns false when umount fails with
 * an error other than EINVAL.
 */
TEST_F(RalfPackageBuilderMockedTest, UnmountOverlayfsIfExists_PathExists_UnmountFails_OtherError)
{
    TEST_LOG("Testing unmountOverlayfsIfExists: path exists, umount fails with EBUSY");
    ON_CALL(*mWrapsImplMock, stat(_, _)).WillByDefault(Return(0));
    ON_CALL(*mWrapsImplMock, umount(_))
        .WillByDefault(Invoke([](const char*) -> int {
            errno = EBUSY;
            return -1;
        }));
    ON_CALL(*mWrapsImplMock, opendir(_)).WillByDefault(Return(nullptr));

    bool result = mBuilder.unmountOverlayfsIfExists("test_app_ebusy");
    EXPECT_FALSE(result);
}

/* Test Case: GenerateRalfDobbySpec_ParseRalPkgInfoFails_ReturnsFalse
 * Verifies the first guard in generateRalfDobbySpec:
 *   bool status = parseRalPkgInfo(ralfPkgPath, mRalfPackages);
 *   if (!status) { LOGERR(...); return false; }
 * parseRalPkgInfo uses std::ifstream internally; supplying a non-existent
 * ralfPkgPath causes it to return false, triggering the first failure branch.
 * The dobbySpec output must remain empty.
 */
TEST_F(RalfPackageBuilderMockedTest, GenerateRalfDobbySpec_ParseRalPkgInfoFails_ReturnsFalse)
{
    TEST_LOG("Testing generateRalfDobbySpec returns false when parseRalPkgInfo fails");

    WPEFramework::Plugin::ApplicationConfiguration config;
    config.mAppId          = "com.example.app";
    config.mAppInstanceId  = "inst_parsefail_001";
    config.mUserId         = 1000;
    config.mGroupId        = 1001;
    config.mWesterosSocketPath = "/tmp/wst-test";
    config.mAppStorageInfo.path = "/data/apps/com.example.app";

    WPEFramework::Exchange::RuntimeConfig rc;
    rc.envVariables = "[]";
    // Point to a path that does not exist — parseRalPkgInfo will fail to open it.
    rc.ralfPkgPath  = "/tmp/ralf_l1_no_such_pkginfo_xyz_12345.json";

    std::string dobbySpec;
    EXPECT_FALSE(mBuilder.generateRalfDobbySpec(config, rc, dobbySpec));
    EXPECT_TRUE(dobbySpec.empty());
}

/* Test Case: GenerateRalfDobbySpec_GenerateOCIRootfsPackageFails_ReturnsFalse
 * Verifies the second guard in generateRalfDobbySpec:
 *   status = generateOCIRootfsPackage(...);
 *   if (!status) { LOGERR(...); return status; }
 * parseRalPkgInfo is made to succeed by pointing ralfPkgPath at the valid
 * mPkgInfoFile created in SetUp().  generateOCIRootfsPackage is made to fail
 * by mocking mount() to return -1; mkdir() and chown() default to 0 via
 * NiceMock so directory creation appears to succeed.
 */
TEST_F(RalfPackageBuilderMockedTest, GenerateRalfDobbySpec_GenerateOCIRootfsPackageFails_ReturnsFalse)
{
    TEST_LOG("Testing generateRalfDobbySpec returns false when generateOCIRootfsPackage fails");

    // mkdir and chown default to 0 (success) through NiceMock.
    // Force mount() to fail so generateOCIRootfs — and therefore
    // generateOCIRootfsPackage — returns false.
    ON_CALL(*mWrapsImplMock, mount(_, _, _, _, _))
        .WillByDefault(Invoke([](const char*, const char*, const char*,
                                 unsigned long, const void*) -> int {
            errno = EPERM;
            return -1;
        }));

    WPEFramework::Plugin::ApplicationConfiguration config;
    config.mAppId          = "com.example.app";
    config.mAppInstanceId  = "inst_mountfail_001";
    config.mUserId         = 0;   // uid=0 skips chown in create_directories
    config.mGroupId        = 0;
    config.mWesterosSocketPath = "/tmp/wst-test";
    config.mAppStorageInfo.path = "/data/apps/com.example.app";

    WPEFramework::Exchange::RuntimeConfig rc;
    rc.envVariables = "[]";
    // Use the valid pkg-info file created in SetUp() so parseRalPkgInfo succeeds.
    rc.ralfPkgPath  = mPkgInfoFile;

    std::string dobbySpec;
    EXPECT_FALSE(mBuilder.generateRalfDobbySpec(config, rc, dobbySpec));
    EXPECT_TRUE(dobbySpec.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Test suite for ralf::RalfOCIConfigGenerator
// ─────────────────────────────────────────────────────────────────────────────

class RalfOCIConfigGeneratorTest : public ::testing::Test
{
protected:
    const std::string mTmpDir          = "/tmp/ralf_oci_gen_test";
    const std::string mConfigOutputFile = mTmpDir + "/config.json";

    void SetUp() override
    {
        mkdir(mTmpDir.c_str(), 0777);
    }

    void TearDown() override
    {
        ::remove(mConfigOutputFile.c_str());
        ::rmdir(mTmpDir.c_str());
    }

    // Helpers to construct lightweight test objects
    WPEFramework::Plugin::ApplicationConfiguration makeConfig(
        const std::string &appId = "com.example.testapp",
        const std::string &instanceId = "test_inst_001",
        uint32_t uid = 1000, uint32_t gid = 1000)
    {
        WPEFramework::Plugin::ApplicationConfiguration config;
        config.mAppId              = appId;
        config.mAppInstanceId      = instanceId;
        config.mUserId             = uid;
        config.mGroupId            = gid;
        config.mWesterosSocketPath = "/tmp/wst-compositor";
        config.mAppStorageInfo.path = "/data/apps/" + appId;
        return config;
    }

    WPEFramework::Exchange::RuntimeConfig makeRuntimeConfig(
        const std::string &envVars = "[]")
    {
        WPEFramework::Exchange::RuntimeConfig rc;
        rc.envVariables = envVars;
        return rc;
    }

};


/* Test Case: GenerateRalfOCIConfig_FailsWhenBaseSpecFileMissing
 * Exercises the first if-guard in generateRalfOCIConfig:
 *   if (!JsonFromFile(RALF_OCI_BASE_SPEC_FILE, ociConfigRootNode)) return false;
 *
 * open() is mocked to return ENOENT for RALF_OCI_BASE_SPEC_FILE.
 * RalfSupportStub.cpp provides a JsonFromFile that calls ::open() directly
 * (instead of std::ifstream), making it interceptable via --wrap,open.
 * The test is therefore self-contained and independent of the host filesystem.
 */
TEST_F(RalfOCIConfigGeneratorTest, GenerateRalfOCIConfig_FailsWhenBaseSpecFileMissing)
{
    TEST_LOG("Testing generateRalfOCIConfig when OCI base spec file is missing (first if-guard)");
    NiceMock<WrapsImplMock> mockWraps;
    Wraps::setImpl(&mockWraps);

    // Forward every open() call to the real syscall, except for the base spec path.
    ON_CALL(mockWraps, open(_, _, _))
        .WillByDefault(Invoke([](const char* path, int flags, mode_t mode) -> int {
            if (std::string(path) == ralf::RALF_OCI_BASE_SPEC_FILE)
            {
                errno = ENOENT;
                return -1;
            }
            return __real_open(path, flags, mode);
        }));

    std::vector<ralf::RalfPkgInfoPair> pkgs;
    ralf::RalfOCIConfigGenerator gen(mConfigOutputFile, pkgs);
    EXPECT_FALSE(gen.generateRalfOCIConfig(makeConfig(), makeRuntimeConfig()));

    Wraps::setImpl(nullptr);
}

/* Test Case: GenerateRalfOCIConfig_FailsWhenGraphicsConfigFileMissing
 * Exercises the second if-guard in generateRalfOCIConfig:
 *   if (!JsonFromFile(RALF_GRAPHICS_LAYER_CONFIG, graphicsConfigNode)) return false;
 *
 * open() is mocked to return ENOENT for RALF_GRAPHICS_LAYER_CONFIG only; all
 * other paths (including the base spec) are forwarded to the real syscall so
 * the first guard passes on CI where the base spec file is present.
 */
TEST_F(RalfOCIConfigGeneratorTest, GenerateRalfOCIConfig_FailsWhenGraphicsConfigFileMissing)
{
    TEST_LOG("Testing generateRalfOCIConfig when graphics config file is missing (second if-guard)");
    NiceMock<WrapsImplMock> mockWraps;
    Wraps::setImpl(&mockWraps);

    // Forward every open() call to the real syscall, except for the graphics config path.
    ON_CALL(mockWraps, open(_, _, _))
        .WillByDefault(Invoke([](const char* path, int flags, mode_t mode) -> int {
            if (std::string(path) == ralf::RALF_GRAPHICS_LAYER_CONFIG)
            {
                errno = ENOENT;
                return -1;
            }
            return __real_open(path, flags, mode);
        }));

    std::vector<ralf::RalfPkgInfoPair> pkgs;
    ralf::RalfOCIConfigGenerator gen(mConfigOutputFile, pkgs);
    EXPECT_FALSE(gen.generateRalfOCIConfig(makeConfig(), makeRuntimeConfig()));

    Wraps::setImpl(nullptr);
}

/* Test Case: GenerateRalfOCIConfig_FailsWhenApplyGraphicsConfigFails
 * Exercises the third if-guard in generateRalfOCIConfig:
 *   if (!applyGraphicsConfigToOCIConfig(...)) return false;
 *
 * Both production file paths are redirected to self-contained stub files so
 * the test does not depend on the host filesystem:
 *   - stubBaseSpec   : minimal valid JSON ({}) so the first guard passes.
 *   - fakeGraphics   : valid JSON with an empty devNodes array, which causes
 *                      addDeviceNodeEntriesToOCIConfig to return false, which
 *                      in turn causes applyGraphicsConfigToOCIConfig to return
 *                      false, triggering the third guard.
 */
TEST_F(RalfOCIConfigGeneratorTest, GenerateRalfOCIConfig_FailsWhenApplyGraphicsConfigFails)
{
    TEST_LOG("Testing generateRalfOCIConfig when applyGraphicsConfigToOCIConfig fails (third if-guard)");

    const std::string stubBaseSpec    = mTmpDir + "/stub-oci-base-spec.json";
    const std::string fakeGraphicsConfig = mTmpDir + "/fake-gpu-config.json";
    writeTextFile(stubBaseSpec, "{}");
    writeTextFile(fakeGraphicsConfig, "{\"vendorGpuSupport\":{\"devNodes\":[]}}");

    NiceMock<WrapsImplMock> mockWraps;
    Wraps::setImpl(&mockWraps);

    ON_CALL(mockWraps, open(_, _, _))
        .WillByDefault(Invoke([&](const char* path, int flags, mode_t mode) -> int {
            if (std::string(path) == ralf::RALF_OCI_BASE_SPEC_FILE)
                return __real_open(stubBaseSpec.c_str(), flags, mode);
            if (std::string(path) == ralf::RALF_GRAPHICS_LAYER_CONFIG)
                return __real_open(fakeGraphicsConfig.c_str(), flags, mode);
            return __real_open(path, flags, mode);
        }));

    std::vector<ralf::RalfPkgInfoPair> pkgs;
    ralf::RalfOCIConfigGenerator gen(mConfigOutputFile, pkgs);
    EXPECT_FALSE(gen.generateRalfOCIConfig(makeConfig(), makeRuntimeConfig()));

    Wraps::setImpl(nullptr);
    ::remove(stubBaseSpec.c_str());
    ::remove(fakeGraphicsConfig.c_str());
}

// ─────────────────────────────────────────────────────────────────────────────
// Test accessor: thin wrappers around RalfOCIConfigGenerator private methods.
// Accessible because this TU includes the header with #define private public.
// ─────────────────────────────────────────────────────────────────────────────

class RalfOCIConfigGeneratorTestAccessor
{
public:
    explicit RalfOCIConfigGeneratorTestAccessor(ralf::RalfOCIConfigGenerator &gen)
        : mGen(gen) {}

    void addToEnvironment(Json::Value &node, const std::string &key, const std::string &value)
    {
        mGen.addToEnvironment(node, key, value);
    }
    void addMountEntry(Json::Value &node, const std::string &src, const std::string &dst)
    {
        mGen.addMountEntry(node, src, dst);
    }
    bool generateHooksForOCIConfig(Json::Value &node, const std::string &op)
    {
        return mGen.generateHooksForOCIConfig(node, op);
    }
    bool addFireboltEndPointToConfig(Json::Value &node, const std::string &envVar)
    {
        return mGen.addFireboltEndPointToConfig(node, envVar);
    }
    bool addConfigOverridesToOCIConfig(Json::Value &node, const Json::Value &configNode)
    {
        return mGen.addConfigOverridesToOCIConfig(node, configNode);
    }
    bool addMemoryConfigToOCIConfig(Json::Value &node, const Json::Value &configNode, const std::string &pkgType)
    {
        return mGen.addMemoryConfigToOCIConfig(node, configNode, pkgType);
    }
    bool addStorageConfigToOCIConfig(Json::Value &node, Json::Value &configNode)
    {
        return mGen.addStorageConfigToOCIConfig(node, configNode);
    }
    bool addEntryPointToOCIConfig(Json::Value &node, const Json::Value &pkgNode)
    {
        return mGen.addEntryPointToOCIConfig(node, pkgNode);
    }
    void addLogNameToOCIConfig(Json::Value &node, const std::string &storagePath, const std::string &appId)
    {
        mGen.addLogNameToOCIConfig(node, storagePath, appId);
    }
    bool addAppPackageVersionToConfig(Json::Value &node, Json::Value &manifestNode)
    {
        return mGen.addAppPackageVersionToConfig(node, manifestNode);
    }
    bool addDeviceNodeEntriesToOCIConfig(Json::Value &node, const Json::Value &devNodes)
    {
        return mGen.addDeviceNodeEntriesToOCIConfig(node, devNodes);
    }
    bool applyConfigurationToOCIConfig(Json::Value &node, Json::Value &manifestNode)
    {
        return mGen.applyConfigurationToOCIConfig(node, manifestNode);
    }
    bool applyRuntimeAndAppConfigToOCIConfig(Json::Value &node,
        const WPEFramework::Exchange::RuntimeConfig &rc,
        const WPEFramework::Plugin::ApplicationConfiguration &ac)
    {
        return mGen.applyRuntimeAndAppConfigToOCIConfig(node, rc, ac);
    }
    bool addAppStorageToOCIConfig(Json::Value &node, const std::string &appStoragePath)
    {
        return mGen.addAppStorageToOCIConfig(node, appStoragePath);
    }
    bool saveOCIConfigToFile(const Json::Value &node, int uid, int gid)
    {
        return mGen.saveOCIConfigToFile(node, uid, gid);
    }

private:
    ralf::RalfOCIConfigGenerator &mGen;
};

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture for RalfOCIConfigGenerator private-method unit tests
// ─────────────────────────────────────────────────────────────────────────────

class RalfOCIConfigGeneratorPrivateTest : public ::testing::Test
{
protected:
    // Both members are stored as static so they outlive the generator's
    // reference members (mConfigFilePath and mRalfPackages are references,
    // not values, in RalfOCIConfigGenerator).
    static std::vector<ralf::RalfPkgInfoPair> sEmptyPackages;
    static const std::string                  sConfigFilePath;

    ralf::RalfOCIConfigGenerator       mGen{sConfigFilePath, sEmptyPackages};
    RalfOCIConfigGeneratorTestAccessor mAcc{mGen};
};

std::vector<ralf::RalfPkgInfoPair> RalfOCIConfigGeneratorPrivateTest::sEmptyPackages;
const std::string RalfOCIConfigGeneratorPrivateTest::sConfigFilePath = "/tmp/ralf_priv_test.json";

// ──────────────────────────────
// addToEnvironment
// ──────────────────────────────

/* Test Case: AddToEnvironment_AddsKeyValuePair
 * Verifies that addToEnvironment appends KEY=VALUE to process.env.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddToEnvironment_AddsKeyValuePair)
{
    TEST_LOG("Testing addToEnvironment with valid key and value");
    Json::Value root;
    mAcc.addToEnvironment(root, "MY_VAR", "hello");
    const Json::Value &envArr = root[ralf::PROCESS][ralf::ENV];
    ASSERT_TRUE(envArr.isArray());
    ASSERT_EQ(1u, envArr.size());
    EXPECT_EQ("MY_VAR=hello", envArr[0].asString());
}

/* Test Case: AddToEnvironment_MultipleVars
 * Verifies that multiple calls append multiple entries to process.env in order.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddToEnvironment_MultipleVars)
{
    TEST_LOG("Testing addToEnvironment with multiple calls");
    Json::Value root;
    mAcc.addToEnvironment(root, "VAR1", "val1");
    mAcc.addToEnvironment(root, "VAR2", "val2");
    const Json::Value &envArr = root[ralf::PROCESS][ralf::ENV];
    ASSERT_EQ(2u, envArr.size());
    EXPECT_EQ("VAR1=val1", envArr[0].asString());
    EXPECT_EQ("VAR2=val2", envArr[1].asString());
}

/* Test Case: AddToEnvironment_EmptyValue
 * Verifies that an empty value produces "KEY=" in process.env.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddToEnvironment_EmptyValue)
{
    TEST_LOG("Testing addToEnvironment with empty value");
    Json::Value root;
    mAcc.addToEnvironment(root, "EMPTY_VAR", "");
    EXPECT_EQ("EMPTY_VAR=", root[ralf::PROCESS][ralf::ENV][0].asString());
}

// ──────────────────────────────
// addMountEntry
// ──────────────────────────────

/* Test Case: AddMountEntry_CorrectFields
 * Verifies that addMountEntry creates a bind mount entry with correct source,
 * destination, type "bind", and options ["rbind", "rw"].
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddMountEntry_CorrectFields)
{
    TEST_LOG("Testing addMountEntry field values");
    Json::Value root;
    mAcc.addMountEntry(root, "/host/path", "/container/path");
    const Json::Value &mounts = root[ralf::MOUNT];
    ASSERT_TRUE(mounts.isArray());
    ASSERT_EQ(1u, mounts.size());
    const Json::Value &entry = mounts[0];
    EXPECT_EQ("/host/path",      entry[ralf::SOURCE].asString());
    EXPECT_EQ("/container/path", entry[ralf::DESTINATION].asString());
    EXPECT_EQ("bind",            entry[ralf::TYPE].asString());
    ASSERT_TRUE(entry[ralf::OPTIONS].isArray());
    EXPECT_EQ("rbind", entry[ralf::OPTIONS][0].asString());
    EXPECT_EQ("rw",    entry[ralf::OPTIONS][1].asString());
}

/* Test Case: AddMountEntry_MultipleMounts
 * Verifies that two addMountEntry calls produce two entries in the mounts array.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddMountEntry_MultipleMounts)
{
    TEST_LOG("Testing addMountEntry called twice");
    Json::Value root;
    mAcc.addMountEntry(root, "/src1", "/dst1");
    mAcc.addMountEntry(root, "/src2", "/dst2");
    ASSERT_EQ(2u, root[ralf::MOUNT].size());
    EXPECT_EQ("/src2", root[ralf::MOUNT][1][ralf::SOURCE].asString());
}

// ──────────────────────────────
// generateHooksForOCIConfig (single operation)
// ──────────────────────────────

/* Test Case: GenerateHooksForOperation_CorrectStructure
 * Verifies the hook entry structure created for the "createRuntime" lifecycle operation.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, GenerateHooksForOperation_CorrectStructure)
{
    TEST_LOG("Testing generateHooksForOCIConfig for createRuntime");
    Json::Value root;
    bool result = mAcc.generateHooksForOCIConfig(root, "createRuntime");
    EXPECT_TRUE(result);
    const Json::Value &hooksArr = root[ralf::HOOKS]["createRuntime"];
    ASSERT_TRUE(hooksArr.isArray());
    ASSERT_EQ(1u, hooksArr.size());
    const Json::Value &hook = hooksArr[0];
    EXPECT_EQ("/usr/bin/DobbyPluginLauncher", hook[ralf::PATH].asString());
    ASSERT_TRUE(hook[ralf::ARGS].isArray());
    EXPECT_EQ("DobbyPluginLauncher", hook[ralf::ARGS][0].asString());
    EXPECT_EQ("-h",                  hook[ralf::ARGS][1].asString());
    EXPECT_EQ("createRuntime",       hook[ralf::ARGS][2].asString());
    EXPECT_EQ("-c",                  hook[ralf::ARGS][3].asString());
    EXPECT_EQ("-v",                  hook[ralf::ARGS][5].asString());
}

/* Test Case: GenerateHooksForOperation_PostStop
 * Verifies that the operation name propagates correctly to the "poststop" hook args.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, GenerateHooksForOperation_PostStop)
{
    TEST_LOG("Testing generateHooksForOCIConfig for poststop");
    Json::Value root;
    bool result = mAcc.generateHooksForOCIConfig(root, "poststop");
    EXPECT_TRUE(result);
    EXPECT_EQ("poststop", root[ralf::HOOKS]["poststop"][0][ralf::ARGS][2].asString());
}

// ──────────────────────────────
// addFireboltEndPointToConfig
// ──────────────────────────────

/* Test Case: AddFireboltEndPoint_Present
 * Verifies that FIREBOLT_ENDPOINT is extracted from the env-vars JSON string and
 * appended to process.env.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddFireboltEndPoint_Present)
{
    TEST_LOG("Testing addFireboltEndPointToConfig when FIREBOLT_ENDPOINT is present");
    Json::Value root;
    bool result = mAcc.addFireboltEndPointToConfig(
        root, R"(["FIREBOLT_ENDPOINT=http://127.0.0.1:9998","OTHER=val"])");
    EXPECT_TRUE(result);
    bool found = false;
    for (const auto &e : root[ralf::PROCESS][ralf::ENV])
    {
        if (e.asString() == "FIREBOLT_ENDPOINT=http://127.0.0.1:9998")
            found = true;
    }
    EXPECT_TRUE(found);
}

/* Test Case: AddFireboltEndPoint_NotPresent
 * Verifies that the function returns false when FIREBOLT_ENDPOINT is absent.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddFireboltEndPoint_NotPresent)
{
    TEST_LOG("Testing addFireboltEndPointToConfig when FIREBOLT_ENDPOINT is absent");
    Json::Value root;
    EXPECT_FALSE(mAcc.addFireboltEndPointToConfig(root, R"(["OTHER=val","ANOTHER=123"])"));
}

/* Test Case: AddFireboltEndPoint_EmptyArray
 * Verifies that an empty env-vars JSON array returns false.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddFireboltEndPoint_EmptyArray)
{
    TEST_LOG("Testing addFireboltEndPointToConfig with empty array");
    Json::Value root;
    EXPECT_FALSE(mAcc.addFireboltEndPointToConfig(root, "[]"));
}

/* Test Case: AddFireboltEndPoint_InvalidJson
 * Verifies that malformed JSON input returns false.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddFireboltEndPoint_InvalidJson)
{
    TEST_LOG("Testing addFireboltEndPointToConfig with invalid JSON");
    Json::Value root;
    EXPECT_FALSE(mAcc.addFireboltEndPointToConfig(root, "not valid json {{"));
}

// ──────────────────────────────
// addConfigOverridesToOCIConfig
// ──────────────────────────────

/* Test Case: AddConfigOverrides_ApplicationOverride
 * Verifies that an "application" sub-object is serialized and stored as
 * APP_CONFIG_OVERRIDES_JSON environment variable.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddConfigOverrides_ApplicationOverride)
{
    TEST_LOG("Testing addConfigOverridesToOCIConfig with application override");
    Json::Value root;
    Json::Value configNode;
    configNode[ralf::CONFIG_OVERRIDES_URN][ralf::PKG_TYPE_APPLICATION]["featureFlag"] = true;
    EXPECT_TRUE(mAcc.addConfigOverridesToOCIConfig(root, configNode));
    bool found = false;
    for (const auto &e : root[ralf::PROCESS][ralf::ENV])
    {
        if (e.asString().rfind(std::string(ralf::APP_CONFIG_OVERRIDES_ENV_KEY) + "=", 0) == 0)
            found = true;
    }
    EXPECT_TRUE(found);
}

/* Test Case: AddConfigOverrides_RuntimeOverride
 * Verifies that a "runtime" sub-object is serialized and stored as
 * RUNTIME_CONFIG_OVERRIDES_JSON environment variable.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddConfigOverrides_RuntimeOverride)
{
    TEST_LOG("Testing addConfigOverridesToOCIConfig with runtime override");
    Json::Value root;
    Json::Value configNode;
    configNode[ralf::CONFIG_OVERRIDES_URN][ralf::PKG_TYPE_RUNTIME]["timeout"] = 30;
    EXPECT_TRUE(mAcc.addConfigOverridesToOCIConfig(root, configNode));
    bool found = false;
    for (const auto &e : root[ralf::PROCESS][ralf::ENV])
    {
        if (e.asString().rfind(std::string(ralf::RUNTIME_CONFIG_OVERRIDES_ENV_KEY) + "=", 0) == 0)
            found = true;
    }
    EXPECT_TRUE(found);
}

/* Test Case: AddConfigOverrides_NoOverridesNode
 * Verifies that addConfigOverridesToOCIConfig returns false when CONFIG_OVERRIDES_URN
 * is absent from the config node.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddConfigOverrides_NoOverridesNode)
{
    TEST_LOG("Testing addConfigOverridesToOCIConfig when no overrides are present");
    Json::Value root;
    Json::Value configNode;  // empty
    EXPECT_FALSE(mAcc.addConfigOverridesToOCIConfig(root, configNode));
}

// ──────────────────────────────
// addMemoryConfigToOCIConfig
// ──────────────────────────────

/* Test Case: AddMemoryConfig_ValidSystemMemory
 * Verifies that a valid "system" memory value is parsed and applied to
 * linux.resources.memory.limit.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddMemoryConfig_ValidSystemMemory)
{
    TEST_LOG("Testing addMemoryConfigToOCIConfig with valid 256M");
    Json::Value root;
    Json::Value configNode;
    configNode[ralf::MEMORY_CONFIG_URN][ralf::SYSTEM_MEMORY] = "256M";
    EXPECT_TRUE(mAcc.addMemoryConfigToOCIConfig(root, configNode, ralf::PKG_TYPE_APPLICATION));
    uint64_t expected = 256ULL * 1024 * 1024;
    EXPECT_EQ(static_cast<Json::UInt64>(expected),
              root[ralf::LINUX][ralf::RESOURCES][ralf::MEMORY][ralf::MEMORY_LIMIT].asUInt64());
}

/* Test Case: AddMemoryConfig_InvalidValue
 * Verifies that an unparseable memory string returns false and sets the default
 * CPU_MEMORY_LIMIT environment variable.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddMemoryConfig_InvalidValue)
{
    TEST_LOG("Testing addMemoryConfigToOCIConfig with invalid memory string");
    Json::Value root;
    Json::Value configNode;
    configNode[ralf::MEMORY_CONFIG_URN][ralf::SYSTEM_MEMORY] = "INVALID";
    EXPECT_FALSE(mAcc.addMemoryConfigToOCIConfig(root, configNode, ralf::PKG_TYPE_APPLICATION));
    bool found = false;
    for (const auto &e : root[ralf::PROCESS][ralf::ENV])
    {
        if (e.asString() == std::string("CPU_MEMORY_LIMIT=") + ralf::DEFAULT_RAM_LIMIT)
            found = true;
    }
    EXPECT_TRUE(found);
}

/* Test Case: AddMemoryConfig_NoMemoryNode
 * Verifies that when MEMORY_CONFIG_URN is absent the function returns false and
 * appends the default CPU_MEMORY_LIMIT.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddMemoryConfig_NoMemoryNode)
{
    TEST_LOG("Testing addMemoryConfigToOCIConfig when memory config is absent");
    Json::Value root;
    Json::Value configNode;
    EXPECT_FALSE(mAcc.addMemoryConfigToOCIConfig(root, configNode, ralf::PKG_TYPE_APPLICATION));
    bool found = false;
    for (const auto &e : root[ralf::PROCESS][ralf::ENV])
    {
        if (e.asString().rfind("CPU_MEMORY_LIMIT=", 0) == 0)
            found = true;
    }
    EXPECT_TRUE(found);
}

// ──────────────────────────────
// addStorageConfigToOCIConfig
// ──────────────────────────────

/* Test Case: AddStorageConfig_ValidMaxLocalStorage
 * Verifies that a valid "maxLocalStorage" value is parsed and STORAGE_LIMIT is set.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddStorageConfig_ValidMaxLocalStorage)
{
    TEST_LOG("Testing addStorageConfigToOCIConfig with valid 100M");
    Json::Value root;
    Json::Value configNode;
    configNode[ralf::STORAGE_CONFIG_URN][ralf::MAX_LOCAL_STORAGE] = "100M";
    EXPECT_TRUE(mAcc.addStorageConfigToOCIConfig(root, configNode));
    uint64_t expected = 100ULL * 1024 * 1024;
    bool found = false;
    for (const auto &e : root[ralf::PROCESS][ralf::ENV])
    {
        if (e.asString() == "STORAGE_LIMIT=" + std::to_string(expected))
            found = true;
    }
    EXPECT_TRUE(found);
}

/* Test Case: AddStorageConfig_InvalidValue
 * Verifies that an unparseable maxLocalStorage value returns false and sets
 * the default STORAGE_LIMIT.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddStorageConfig_InvalidValue)
{
    TEST_LOG("Testing addStorageConfigToOCIConfig with invalid storage value");
    Json::Value root;
    Json::Value configNode;
    configNode[ralf::STORAGE_CONFIG_URN][ralf::MAX_LOCAL_STORAGE] = "BAD_VALUE";
    EXPECT_FALSE(mAcc.addStorageConfigToOCIConfig(root, configNode));
    bool found = false;
    for (const auto &e : root[ralf::PROCESS][ralf::ENV])
    {
        if (e.asString() == std::string("STORAGE_LIMIT=") + ralf::DEFAULT_STORAGE_LIMIT)
            found = true;
    }
    EXPECT_TRUE(found);
}

/* Test Case: AddStorageConfig_NoStorageNode
 * Verifies that when STORAGE_CONFIG_URN is absent the function returns false and
 * appends the default STORAGE_LIMIT.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddStorageConfig_NoStorageNode)
{
    TEST_LOG("Testing addStorageConfigToOCIConfig when storage config is absent");
    Json::Value root;
    Json::Value configNode;
    EXPECT_FALSE(mAcc.addStorageConfigToOCIConfig(root, configNode));
    bool found = false;
    for (const auto &e : root[ralf::PROCESS][ralf::ENV])
    {
        if (e.asString().rfind("STORAGE_LIMIT=", 0) == 0)
            found = true;
    }
    EXPECT_TRUE(found);
}

// ──────────────────────────────
// addEntryPointToOCIConfig
// ──────────────────────────────

/* Test Case: AddEntryPoint_StringValue
 * Verifies that an entryPoint string is appended to process.args.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddEntryPoint_StringValue)
{
    TEST_LOG("Testing addEntryPointToOCIConfig with string entryPoint");
    Json::Value root;
    root[ralf::PROCESS][ralf::ARGS] = Json::Value(Json::arrayValue);
    Json::Value pkgNode;
    pkgNode[ralf::ENTRY_POINT] = "/usr/bin/myapp";
    EXPECT_TRUE(mAcc.addEntryPointToOCIConfig(root, pkgNode));
    const Json::Value &args = root[ralf::PROCESS][ralf::ARGS];
    ASSERT_EQ(1u, args.size());
    EXPECT_EQ("/usr/bin/myapp", args[0].asString());
}

/* Test Case: AddEntryPoint_Missing
 * Verifies that addEntryPointToOCIConfig returns true (non-fatal warning) when
 * entryPoint is absent from the package node.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddEntryPoint_Missing)
{
    TEST_LOG("Testing addEntryPointToOCIConfig when entryPoint is absent");
    Json::Value root;
    Json::Value pkgNode;  // no entryPoint
    EXPECT_TRUE(mAcc.addEntryPointToOCIConfig(root, pkgNode));
}

// ──────────────────────────────
// addLogNameToOCIConfig
// ──────────────────────────────

/* Test Case: AddLogNameToOCIConfig_PathFormattedCorrectly
 * Verifies that the log path is set to <appStoragePath>/<appId>.log.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddLogNameToOCIConfig_PathFormattedCorrectly)
{
    TEST_LOG("Testing addLogNameToOCIConfig formats path correctly");
    Json::Value root;
    mAcc.addLogNameToOCIConfig(root, "/data/apps/myapp", "com.example.myapp");
    const std::string &logPath =
        root[ralf::RDKPLUGINS][ralf::LOGGING][ralf::DATA]
            [ralf::LOG_FILE_OPTIONS][ralf::PATH].asString();
    EXPECT_EQ("/data/apps/myapp/com.example.myapp.log", logPath);
}

/* Test Case: AddLogNameToOCIConfig_EmptyStoragePath
 * Verifies that an empty storage path produces "/<appId>.log".
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddLogNameToOCIConfig_EmptyStoragePath)
{
    TEST_LOG("Testing addLogNameToOCIConfig with empty storage path");
    Json::Value root;
    mAcc.addLogNameToOCIConfig(root, "", "myapp");
    const std::string &logPath =
        root[ralf::RDKPLUGINS][ralf::LOGGING][ralf::DATA]
            [ralf::LOG_FILE_OPTIONS][ralf::PATH].asString();
    EXPECT_EQ("/myapp.log", logPath);
}

// ──────────────────────────────
// addAppPackageVersionToConfig
// ──────────────────────────────

/* Test Case: AddAppPackageVersion_ValidVersionAndName
 * Verifies that APP_PACKAGE_VERSION is formatted as "versionName_version".
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddAppPackageVersion_ValidVersionAndName)
{
    TEST_LOG("Testing addAppPackageVersionToConfig with valid fields");
    Json::Value root;
    Json::Value manifest;
    manifest[ralf::VERSION]      = "42";
    manifest[ralf::VERSION_NAME] = "1.0.0";
    EXPECT_TRUE(mAcc.addAppPackageVersionToConfig(root, manifest));
    bool found = false;
    for (const auto &e : root[ralf::PROCESS][ralf::ENV])
    {
        if (e.asString() == "APP_PACKAGE_VERSION=1.0.0_42")
            found = true;
    }
    EXPECT_TRUE(found);
}

/* Test Case: AddAppPackageVersion_MissingVersion
 * Verifies that addAppPackageVersionToConfig returns false when "version" is absent.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddAppPackageVersion_MissingVersion)
{
    TEST_LOG("Testing addAppPackageVersionToConfig with missing version");
    Json::Value root;
    Json::Value manifest;
    manifest[ralf::VERSION_NAME] = "1.0.0";
    EXPECT_FALSE(mAcc.addAppPackageVersionToConfig(root, manifest));
}

/* Test Case: AddAppPackageVersion_MissingVersionName
 * Verifies that addAppPackageVersionToConfig returns false when "versionName" is absent.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddAppPackageVersion_MissingVersionName)
{
    TEST_LOG("Testing addAppPackageVersionToConfig with missing versionName");
    Json::Value root;
    Json::Value manifest;
    manifest[ralf::VERSION] = "1";
    EXPECT_FALSE(mAcc.addAppPackageVersionToConfig(root, manifest));
}

// ──────────────────────────────
// addDeviceNodeEntriesToOCIConfig
// ──────────────────────────────

/* Test Case: AddDeviceNodeEntries_EmptyList
 * Verifies that addDeviceNodeEntriesToOCIConfig returns false for an empty devNodes
 * array (no entries, size == 0 guard).
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddDeviceNodeEntries_EmptyList)
{
    TEST_LOG("Testing addDeviceNodeEntriesToOCIConfig with empty devNodes");
    Json::Value root;
    Json::Value devNodes(Json::arrayValue);
    EXPECT_FALSE(mAcc.addDeviceNodeEntriesToOCIConfig(root, devNodes));
}

/* Test Case: AddDeviceNodeEntries_NonExistentDevice
 * Verifies that a non-existent device path is handled gracefully: the function
 * still iterates, getDevNodeMajorMinor fails, and linux.devices stays empty.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddDeviceNodeEntries_NonExistentDevice)
{
    TEST_LOG("Testing addDeviceNodeEntriesToOCIConfig with non-existent device path");
    Json::Value root;
    Json::Value devNodes(Json::arrayValue);
    devNodes.append("/dev/ralf_no_such_device_xyz_99999");
    mAcc.addDeviceNodeEntriesToOCIConfig(root, devNodes);
    EXPECT_EQ(0u, root[ralf::LINUX][ralf::DEVICES].size());
}

// ──────────────────────────────
// applyConfigurationToOCIConfig
// ──────────────────────────────

/* Test Case: ApplyConfiguration_NoConfigurationNode_ReturnsTrue
 * Verifies that applyConfigurationToOCIConfig returns true when the manifest has
 * an entryPoint but no "configuration" sub-object (non-fatal).
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, ApplyConfiguration_NoConfigurationNode_ReturnsTrue)
{
    TEST_LOG("Testing applyConfigurationToOCIConfig with no configuration node");
    Json::Value root;
    Json::Value manifest;
    manifest[ralf::ENTRY_POINT]  = "/bin/app";
    manifest[ralf::PACKAGE_TYPE] = ralf::PKG_TYPE_APPLICATION;
    EXPECT_TRUE(mAcc.applyConfigurationToOCIConfig(root, manifest));
}

/* Test Case: ApplyConfiguration_WithFullApplicationConfig
 * Verifies that applyConfigurationToOCIConfig processes an application package with
 * memory and storage config nodes and returns true, setting linux.resources.memory.limit.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, ApplyConfiguration_WithFullApplicationConfig)
{
    TEST_LOG("Testing applyConfigurationToOCIConfig with full application config");
    Json::Value root;
    Json::Value manifest;
    manifest[ralf::ENTRY_POINT]  = "/bin/app";
    manifest[ralf::PACKAGE_TYPE] = ralf::PKG_TYPE_APPLICATION;
    manifest[ralf::VERSION]      = "10";
    manifest[ralf::VERSION_NAME] = "2.0.0";
    manifest[ralf::CONFIGURATION][ralf::MEMORY_CONFIG_URN][ralf::SYSTEM_MEMORY]       = "128M";
    manifest[ralf::CONFIGURATION][ralf::STORAGE_CONFIG_URN][ralf::MAX_LOCAL_STORAGE]  = "50M";
    EXPECT_TRUE(mAcc.applyConfigurationToOCIConfig(root, manifest));
    uint64_t expectedMem = 128ULL * 1024 * 1024;
    EXPECT_EQ(static_cast<Json::UInt64>(expectedMem),
              root[ralf::LINUX][ralf::RESOURCES][ralf::MEMORY][ralf::MEMORY_LIMIT].asUInt64());
}

// ──────────────────────────────────────────────────────────
// applyRuntimeAndAppConfigToOCIConfig
// ──────────────────────────────────────────────────────────

/* Helper: build a minimal ApplicationConfiguration for the tests below. */
static WPEFramework::Plugin::ApplicationConfiguration makeAppConfig(
    const std::string &appId = "com.example.app",
    const std::string &instanceId = "inst001",
    uint32_t uid = 1000, uint32_t gid = 1001,
    const std::string &wstSocket = "/tmp/wst-testapp")
{
    WPEFramework::Plugin::ApplicationConfiguration c;
    c.mAppId              = appId;
    c.mAppInstanceId      = instanceId;
    c.mUserId             = uid;
    c.mGroupId            = gid;
    c.mWesterosSocketPath = wstSocket;
    c.mAppStorageInfo.path = "/data/apps/" + appId;
    return c;
}

/* Helper: build a minimal RuntimeConfig for the tests below. */
static WPEFramework::Exchange::RuntimeConfig makeRtConfig()
{
    WPEFramework::Exchange::RuntimeConfig rc;
    rc.envVariables = "[]";
    return rc;
}

// ──────────────────────────────
// addAppStorageToOCIConfig
// ──────────────────────────────

/* Test Case: AddAppStorage_ReturnsTrueOnValidPath
 * Verifies that addAppStorageToOCIConfig returns true for a non-empty path.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddAppStorage_ReturnsTrueOnValidPath)
{
    TEST_LOG("Testing addAppStorageToOCIConfig returns true on valid path");
    Json::Value root;
    EXPECT_TRUE(mAcc.addAppStorageToOCIConfig(root, "/data/apps/com.example.app"));
}

/* Test Case: AddAppStorage_MountsHostPathToContainerPath
 * Verifies that the host appStoragePath is mounted to the container's
 * PERSIST_STORAGE_PATH ("/data") via mounts[].source/destination.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddAppStorage_MountsHostPathToContainerPath)
{
    TEST_LOG("Testing addAppStorageToOCIConfig adds correct mount entry");
    Json::Value root;
    mAcc.addAppStorageToOCIConfig(root, "/data/apps/com.example.app");

    bool found = false;
    for (const auto &m : root[ralf::MOUNT])
    {
        if (m[ralf::SOURCE].asString() == "/data/apps/com.example.app" &&
            m[ralf::DESTINATION].asString() == "/data")
            found = true;
    }
    EXPECT_TRUE(found);
}

/* Test Case: AddAppStorage_SetsPersistStoragePathEnvVar
 * Verifies that PERSIST_STORAGE_PATH is appended to process.env pointing
 * to the container-side path "/data".
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddAppStorage_SetsPersistStoragePathEnvVar)
{
    TEST_LOG("Testing addAppStorageToOCIConfig sets PERSIST_STORAGE_PATH env var");
    Json::Value root;
    mAcc.addAppStorageToOCIConfig(root, "/data/apps/com.example.app");

    bool found = false;
    for (const auto &e : root[ralf::PROCESS][ralf::ENV])
    {
        if (e.asString() == "PERSIST_STORAGE_PATH=/data")
            found = true;
    }
    EXPECT_TRUE(found);
}

/* Test Case: AddAppStorage_EmptyPathStillReturnsTrueAndSetsEnv
 * Verifies that an empty appStoragePath still returns true and sets the env var.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, AddAppStorage_EmptyPathStillReturnsTrueAndSetsEnv)
{
    TEST_LOG("Testing addAppStorageToOCIConfig with empty storage path");
    Json::Value root;
    EXPECT_TRUE(mAcc.addAppStorageToOCIConfig(root, ""));

    bool found = false;
    for (const auto &e : root[ralf::PROCESS][ralf::ENV])
    {
        if (e.asString() == "PERSIST_STORAGE_PATH=/data")
            found = true;
    }
    EXPECT_TRUE(found);
}


/* Test Case: ApplyRuntimeAndAppConfig_GetGroupIdFails_UsesDefaultVideoGid
 * Verifies the first if-case failure in applyRuntimeAndAppConfigToOCIConfig:
 *   if (!getGroupId("video", videoGid)) { LOGWARN(...); }
 * getgrnam() is mocked to return nullptr so the test runs deterministically on
 * every host regardless of whether a "video" group exists in /etc/group.
 * When getGroupId("video", ...) returns false the function must still return
 * true and append the fallback default GID 44 to process.user.additionalGids.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, ApplyRuntimeAndAppConfig_GetGroupIdFails_UsesDefaultVideoGid)
{
    TEST_LOG("Testing applyRuntimeAndAppConfigToOCIConfig uses default GID 44 when getGroupId fails for video group");

    // Mock getgrnam() to return nullptr so getGroupId("video", ...) always fails,
    // regardless of whether the "video" group is present on the test host.
    NiceMock<WrapsImplMock> mockWraps;
    Wraps::setImpl(&mockWraps);
    ON_CALL(mockWraps, getgrnam(_)).WillByDefault(Return(nullptr));

    auto config = makeAppConfig();
    auto rc     = makeRtConfig();
    Json::Value root;

    // The function must return true even when getGroupId fails (first if-case is
    // a warning-only branch — it does not abort the function).
    EXPECT_TRUE(mAcc.applyRuntimeAndAppConfigToOCIConfig(root, rc, config));

    // When getGroupId fails, videoGid stays at its initialised default of 44.
    // Verify that 44 is the value appended to process.user.additionalGids.
    const Json::Value &additionalGids = root[ralf::PROCESS][ralf::USER][ralf::ADDITIONAL_GIDS];
    ASSERT_GE(additionalGids.size(), 1u);
    EXPECT_EQ(44u, additionalGids[0].asUInt());

    Wraps::setImpl(nullptr);
}

// ──────────────────────────────
// saveOCIConfigToFile
// ──────────────────────────────

/* Test Case: SaveOCIConfigToFile_FailsWhenOutputDirectoryMissing
 * Verifies the first if-case failure in saveOCIConfigToFile:
 *   std::ofstream outFile(mConfigFilePath);
 *   if (outFile) { ... } else { LOGERR(...); return false; }
 * When the parent directory of mConfigFilePath does not exist, std::ofstream
 * fails to open, the else-branch logs an error, and the function returns false.
 */
TEST_F(RalfOCIConfigGeneratorPrivateTest, SaveOCIConfigToFile_FailsWhenOutputDirectoryMissing)
{
    TEST_LOG("Testing saveOCIConfigToFile returns false when output directory does not exist");

    // Construct a generator whose output path lives inside a non-existent directory.
    // std::ofstream will fail to open this path, triggering the first if-case failure.
    const std::string badPath = "/tmp/ralf_no_such_dir_xyz_12345/config.json";
    std::vector<ralf::RalfPkgInfoPair> emptyPkgs;
    ralf::RalfOCIConfigGenerator gen(badPath, emptyPkgs);
    RalfOCIConfigGeneratorTestAccessor acc(gen);

    Json::Value root;
    root["test"] = "value";

    EXPECT_FALSE(acc.saveOCIConfigToFile(root, 0, 0));
}

// ──────────────────────────────
// hasInternetPermission
// ──────────────────────────────

class RalfHasInternetPermissionTest : public ::testing::Test
{
protected:
    std::vector<ralf::RalfPkgInfoPair> emptyPkgs;
    ralf::RalfOCIConfigGenerator gen{"dummy.json", emptyPkgs};
    RalfOCIConfigGeneratorTestAccessor acc{gen};
};

TEST_F(RalfHasInternetPermissionTest, HasInternetPermission_EmptyString)
{
    TEST_LOG("Testing hasInternetPermission with empty string");
    EXPECT_FALSE(acc.hasInternetPermission(""));
}

TEST_F(RalfHasInternetPermissionTest, HasInternetPermission_SingleInternetPermission)
{
    TEST_LOG("Testing hasInternetPermission with internet permission");
    EXPECT_TRUE(acc.hasInternetPermission("urn:rdk:permission:internet"));
}

TEST_F(RalfHasInternetPermissionTest, HasInternetPermission_MultiplePermissionsWithInternet)
{
    TEST_LOG("Testing hasInternetPermission with multiple permissions including internet");
    std::string capabilities = "urn:rdk:permission:home-app,urn:rdk:permission:internet,urn:rdk:permission:firebolt";
    EXPECT_TRUE(acc.hasInternetPermission(capabilities));
}

TEST_F(RalfHasInternetPermissionTest, HasInternetPermission_InternetPermissionFirst)
{
    TEST_LOG("Testing hasInternetPermission with internet permission first in list");
    std::string capabilities = "urn:rdk:permission:internet,urn:rdk:permission:home-app,urn:rdk:permission:firebolt";
    EXPECT_TRUE(acc.hasInternetPermission(capabilities));
}

TEST_F(RalfHasInternetPermissionTest, HasInternetPermission_NoInternetPermission)
{
    TEST_LOG("Testing hasInternetPermission without internet permission");
    std::string capabilities = "urn:rdk:permission:home-app,urn:rdk:permission:firebolt,urn:rdk:permission:thunder";
    EXPECT_FALSE(acc.hasInternetPermission(capabilities));
}

TEST_F(RalfHasInternetPermissionTest, HasInternetPermission_WithWhitespace)
{
    TEST_LOG("Testing hasInternetPermission with whitespace");
    std::string capabilities = "urn:rdk:permission:home-app , urn:rdk:permission:internet , urn:rdk:permission:firebolt";
    EXPECT_TRUE(acc.hasInternetPermission(capabilities));
}

TEST_F(RalfHasInternetPermissionTest, HasInternetPermission_PartialMatch)
{
    TEST_LOG("Testing hasInternetPermission with partial string match (should not match)");
    std::string capabilities = "urn:rdk:permission:internet-disabled";
    EXPECT_FALSE(acc.hasInternetPermission(capabilities));
}

// ──────────────────────────────
// applyNetworkConfigToOCIConfig
// ──────────────────────────────

class RalfNetworkConfigTest : public ::testing::Test
{
protected:
    std::vector<ralf::RalfPkgInfoPair> emptyPkgs;
    ralf::RalfOCIConfigGenerator gen{"dummy.json", emptyPkgs};
    RalfOCIConfigGeneratorTestAccessor acc{gen};
    Json::Value ociConfig;

    void SetUp() override
    {
        // Initialize basic OCI structure
        ociConfig[ralf::RDKPLUGINS][ralf::NETWORKING][ralf::DATA] = Json::Value(Json::objectValue);
    }
};

TEST_F(RalfNetworkConfigTest, ApplyNetworkConfig_NoNetworkConfig)
{
    TEST_LOG("Testing applyNetworkConfigToOCIConfig with no network config");
    Json::Value configNode;
    EXPECT_TRUE(acc.applyNetworkConfigToOCIConfig(ociConfig, configNode));
}

TEST_F(RalfNetworkConfigTest, ApplyNetworkConfig_SinglePortForwarding)
{
    TEST_LOG("Testing applyNetworkConfigToOCIConfig with single port forwarding");
    Json::Value configNode;
    Json::Value networkArray(Json::arrayValue);

    Json::Value portEntry;
    portEntry[ralf::NAME] = "netflix-mdx";
    portEntry[ralf::PORT] = 8009;
    portEntry[ralf::PROTOCOL] = "tcp";
    portEntry[ralf::TYPE] = "public";
    networkArray.append(portEntry);

    configNode[ralf::NETWORK_CONFIG_URN] = networkArray;

    EXPECT_TRUE(acc.applyNetworkConfigToOCIConfig(ociConfig, configNode));

    // "public" type -> hostToContainer array
    const Json::Value &htc = ociConfig[ralf::RDKPLUGINS][ralf::NETWORKING][ralf::DATA][ralf::PORT_FORWARDING][ralf::HOST_TO_CONTAINER];
    EXPECT_TRUE(htc.isArray());
    EXPECT_EQ(htc.size(), 1u);
    EXPECT_EQ(htc[0][ralf::PORT].asInt(), 8009);
    EXPECT_EQ(htc[0][ralf::PROTOCOL].asString(), "tcp");
}

TEST_F(RalfNetworkConfigTest, ApplyNetworkConfig_MultiplePortForwarding)
{
    TEST_LOG("Testing applyNetworkConfigToOCIConfig with multiple port forwarding entries");
    Json::Value configNode;
    Json::Value networkArray(Json::arrayValue);

    // First entry
    Json::Value entry1;
    entry1[ralf::NAME] = "netflix-mdx";
    entry1[ralf::PORT] = 8009;
    entry1[ralf::PROTOCOL] = "tcp";
    entry1[ralf::TYPE] = "public";
    networkArray.append(entry1);

    // Second entry
    Json::Value entry2;
    entry2[ralf::NAME] = "myapp-service";
    entry2[ralf::PORT] = 1234;
    entry2[ralf::PROTOCOL] = "tcp";
    entry2[ralf::TYPE] = "exported";
    networkArray.append(entry2);

    configNode[ralf::NETWORK_CONFIG_URN] = networkArray;

    EXPECT_TRUE(acc.applyNetworkConfigToOCIConfig(ociConfig, configNode));

    // Both "public" and "exported" go to hostToContainer
    const Json::Value &htc = ociConfig[ralf::RDKPLUGINS][ralf::NETWORKING][ralf::DATA][ralf::PORT_FORWARDING][ralf::HOST_TO_CONTAINER];
    EXPECT_TRUE(htc.isArray());
    EXPECT_EQ(htc.size(), 2u);
    EXPECT_EQ(htc[0][ralf::PORT].asInt(), 8009);
    EXPECT_EQ(htc[1][ralf::PORT].asInt(), 1234);
}

TEST_F(RalfNetworkConfigTest, ApplyNetworkConfig_MissingName)
{
    TEST_LOG("Testing applyNetworkConfigToOCIConfig: missing name is allowed, entry still added");
    Json::Value configNode;
    Json::Value networkArray(Json::arrayValue);

    Json::Value portEntry;
    // No NAME field — should use "unnamed" for logging, still route correctly
        portEntry[ralf::TYPE] = "public";
    portEntry[ralf::PORT] = 8009;
    portEntry[ralf::PROTOCOL] = "tcp";
    networkArray.append(portEntry);

    configNode[ralf::NETWORK_CONFIG_URN] = networkArray;


        // Entry should still be routed to hostToContainer
        const Json::Value &htc = ociConfig[ralf::RDKPLUGINS][ralf::NETWORKING][ralf::DATA][ralf::PORT_FORWARDING][ralf::HOST_TO_CONTAINER];
        EXPECT_TRUE(htc.isArray());
        EXPECT_EQ(htc.size(), 1u);
        EXPECT_EQ(htc[0][ralf::PORT].asInt(), 8009);
    EXPECT_TRUE(acc.applyNetworkConfigToOCIConfig(ociConfig, configNode));
}

TEST_F(RalfNetworkConfigTest, ApplyNetworkConfig_NonArrayConfig)
{
    TEST_LOG("Testing applyNetworkConfigToOCIConfig with non-array network config");
    Json::Value configNode;
    // Set network config to an object instead of array
    configNode[ralf::NETWORK_CONFIG_URN] = Json::Value(Json::objectValue);

    EXPECT_TRUE(acc.applyNetworkConfigToOCIConfig(ociConfig, configNode));
}

TEST_F(RalfNetworkConfigTest, ApplyNetworkConfig_PartialFields)
{
    TEST_LOG("Testing applyNetworkConfigToOCIConfig: entry without port is skipped");
    Json::Value configNode;
    Json::Value networkArray(Json::arrayValue);

    Json::Value portEntry;
    portEntry[ralf::NAME] = "test-service";
    // Port is intentionally absent — this entry should be skipped
    networkArray.append(portEntry);

    configNode[ralf::NETWORK_CONFIG_URN] = networkArray;

    EXPECT_TRUE(acc.applyNetworkConfigToOCIConfig(ociConfig, configNode));

    // No portForwarding sub-arrays should have been created
    const Json::Value &pf = ociConfig[ralf::RDKPLUGINS][ralf::NETWORKING][ralf::DATA][ralf::PORT_FORWARDING];
    EXPECT_FALSE(pf.isMember(ralf::HOST_TO_CONTAINER));
    EXPECT_FALSE(pf.isMember(ralf::CONTAINER_TO_HOST));
}

TEST_F(RalfNetworkConfigTest, ApplyNetworkConfig_ImportedType_GoesToContainerToHost)
{
    TEST_LOG("Testing applyNetworkConfigToOCIConfig: imported type goes to containerToHost (no localhostMasquerade)");
    Json::Value configNode;
    Json::Value networkArray(Json::arrayValue);

    Json::Value portEntry;
    portEntry[ralf::NAME]     = "peer-service";
    portEntry[ralf::PORT]     = 4567;
    portEntry[ralf::PROTOCOL] = "tcp";
    portEntry[ralf::TYPE]     = "imported";
    networkArray.append(portEntry);

    configNode[ralf::NETWORK_CONFIG_URN] = networkArray;

    EXPECT_TRUE(acc.applyNetworkConfigToOCIConfig(ociConfig, configNode));

    const Json::Value &ctoh = ociConfig[ralf::RDKPLUGINS][ralf::NETWORKING][ralf::DATA][ralf::PORT_FORWARDING][ralf::CONTAINER_TO_HOST];
    EXPECT_TRUE(ctoh.isArray());
    EXPECT_EQ(ctoh.size(), 1u);
    EXPECT_EQ(ctoh[0][ralf::PORT].asInt(), 4567);
    EXPECT_EQ(ctoh[0][ralf::PROTOCOL].asString(), "tcp");
    EXPECT_FALSE(ctoh[0].isMember(ralf::LOCALHOST_MASQUERADE));
}

TEST_F(RalfNetworkConfigTest, ApplyNetworkConfig_MixedTypes_RoutedCorrectly)
{
    TEST_LOG("Testing applyNetworkConfigToOCIConfig: public -> hostToContainer, imported -> containerToHost");
    Json::Value configNode;
    Json::Value networkArray(Json::arrayValue);

    Json::Value pub;
    pub[ralf::NAME] = "mdx"; pub[ralf::PORT] = 8009; pub[ralf::PROTOCOL] = "tcp"; pub[ralf::TYPE] = "public";
    networkArray.append(pub);

    Json::Value imp;
    imp[ralf::NAME] = "peer"; imp[ralf::PORT] = 4567; imp[ralf::PROTOCOL] = "tcp"; imp[ralf::TYPE] = "imported";
    networkArray.append(imp);

    configNode[ralf::NETWORK_CONFIG_URN] = networkArray;

    EXPECT_TRUE(acc.applyNetworkConfigToOCIConfig(ociConfig, configNode));

    const Json::Value &htc  = ociConfig[ralf::RDKPLUGINS][ralf::NETWORKING][ralf::DATA][ralf::PORT_FORWARDING][ralf::HOST_TO_CONTAINER];
    const Json::Value &ctoh = ociConfig[ralf::RDKPLUGINS][ralf::NETWORKING][ralf::DATA][ralf::PORT_FORWARDING][ralf::CONTAINER_TO_HOST];
    EXPECT_EQ(htc.size(),  1u);
    EXPECT_EQ(htc[0][ralf::PORT].asInt(), 8009);
    EXPECT_EQ(ctoh.size(), 1u);
    EXPECT_EQ(ctoh[0][ralf::PORT].asInt(), 4567);
}

// ──────────────────────────────
// hasCapabilityPermission
// ──────────────────────────────

class RalfHasCapabilityPermissionTest : public ::testing::Test
{
protected:
    std::vector<ralf::RalfPkgInfoPair> emptyPkgs;
    ralf::RalfOCIConfigGenerator gen{"dummy.json", emptyPkgs};
    RalfOCIConfigGeneratorTestAccessor acc{gen};
};

TEST_F(RalfHasCapabilityPermissionTest, HasCapabilityPermission_EmptyCapabilities)
{
    EXPECT_FALSE(acc.hasCapabilityPermission("", "urn:rdk:permission:thunder"));
}

TEST_F(RalfHasCapabilityPermissionTest, HasCapabilityPermission_EmptyPermission)
{
    EXPECT_FALSE(acc.hasCapabilityPermission("urn:rdk:permission:thunder", ""));
}

TEST_F(RalfHasCapabilityPermissionTest, HasCapabilityPermission_ThunderFound)
{
    EXPECT_TRUE(acc.hasCapabilityPermission("urn:rdk:permission:thunder", "urn:rdk:permission:thunder"));
}

TEST_F(RalfHasCapabilityPermissionTest, HasCapabilityPermission_FireboltFound)
{
    std::string caps = "urn:rdk:permission:home-app,urn:rdk:permission:firebolt,urn:rdk:permission:rialto";
    EXPECT_TRUE(acc.hasCapabilityPermission(caps, "urn:rdk:permission:firebolt"));
}

TEST_F(RalfHasCapabilityPermissionTest, HasCapabilityPermission_NotPresent)
{
    std::string caps = "urn:rdk:permission:home-app,urn:rdk:permission:rialto";
    EXPECT_FALSE(acc.hasCapabilityPermission(caps, "urn:rdk:permission:thunder"));
}

TEST_F(RalfHasCapabilityPermissionTest, HasCapabilityPermission_PartialMatchNotAccepted)
{
    EXPECT_FALSE(acc.hasCapabilityPermission("urn:rdk:permission:thunder-extra", "urn:rdk:permission:thunder"));
}

// ──────────────────────────────
// Thunder / Firebolt containerToHost via applyRuntimeAndAppConfigToOCIConfig
// ──────────────────────────────

class RalfThunderFireboltNetworkTest : public ::testing::Test
{
protected:
    std::vector<ralf::RalfPkgInfoPair> emptyPkgs;
    ralf::RalfOCIConfigGenerator gen{"/tmp/dummy_ralf_test.json", emptyPkgs};
    RalfOCIConfigGeneratorTestAccessor acc{gen};

    Json::Value ociConfig;
    WPEFramework::Plugin::ApplicationConfiguration appCfg;
    WPEFramework::Exchange::RuntimeConfig rtCfg;

    void SetUp() override
    {
        ociConfig[ralf::RDKPLUGINS][ralf::NETWORKING][ralf::DATA][ralf::TYPE] = "closed";
        ociConfig[ralf::PROCESS][ralf::USER][ralf::UID] = 0;
        ociConfig[ralf::LINUX][ralf::UID_MAPPINGS] = Json::Value(Json::arrayValue);
        ociConfig[ralf::LINUX][ralf::GID_MAPPINGS] = Json::Value(Json::arrayValue);
        ociConfig[ralf::MOUNT] = Json::Value(Json::arrayValue);
        ociConfig[ralf::PROCESS][ralf::ENV] = Json::Value(Json::arrayValue);

        rtCfg.dial         = false;
        rtCfg.wanLanAccess = false;
        rtCfg.thunder      = false;
        rtCfg.capabilities = "";
        rtCfg.envVariables = "[]";

        appCfg.mAppId              = "com.test.app";
        appCfg.mAppInstanceId      = "inst-001";
        appCfg.mUserId             = 1000;
        appCfg.mGroupId            = 1000;
        appCfg.mWesterosSocketPath = "/tmp/wayland-0";
        appCfg.mAppStorageInfo.path = "/opt/data/com.test.app";
    }
};

TEST_F(RalfThunderFireboltNetworkTest, ThunderFlag_AddsContainerToHostPort9998)
{
    TEST_LOG("thunder=true -> containerToHost port 9998 with localhostMasquerade");
    rtCfg.thunder = true;

    EXPECT_TRUE(acc.applyRuntimeAndAppConfigToOCIConfig(ociConfig, rtCfg, appCfg));

    EXPECT_EQ(ociConfig[ralf::RDKPLUGINS][ralf::NETWORKING][ralf::DATA][ralf::TYPE].asString(), "nat");
    const Json::Value &ctoh = ociConfig[ralf::RDKPLUGINS][ralf::NETWORKING][ralf::DATA][ralf::PORT_FORWARDING][ralf::CONTAINER_TO_HOST];
    EXPECT_EQ(ctoh.size(), 1u);
    EXPECT_EQ(ctoh[0][ralf::PORT].asInt(), ralf::THUNDER_JSONRPC_PORT);
    EXPECT_TRUE(ctoh[0][ralf::LOCALHOST_MASQUERADE].asBool());
}

TEST_F(RalfThunderFireboltNetworkTest, FireboltPermission_AddsContainerToHostPort9998)
{
    TEST_LOG("permission:firebolt only -> containerToHost port 9998 with localhostMasquerade");
    rtCfg.capabilities = "urn:rdk:permission:firebolt";

    EXPECT_TRUE(acc.applyRuntimeAndAppConfigToOCIConfig(ociConfig, rtCfg, appCfg));

    EXPECT_EQ(ociConfig[ralf::RDKPLUGINS][ralf::NETWORKING][ralf::DATA][ralf::TYPE].asString(), "nat");
    const Json::Value &ctoh = ociConfig[ralf::RDKPLUGINS][ralf::NETWORKING][ralf::DATA][ralf::PORT_FORWARDING][ralf::CONTAINER_TO_HOST];
    EXPECT_EQ(ctoh.size(), 1u);
    EXPECT_EQ(ctoh[0][ralf::PORT].asInt(), ralf::THUNDER_JSONRPC_PORT);
    EXPECT_TRUE(ctoh[0][ralf::LOCALHOST_MASQUERADE].asBool());
}

TEST_F(RalfThunderFireboltNetworkTest, ThunderAndFireboltBoth_SingleDedupedEntry)
{
    TEST_LOG("thunder + firebolt -> single deduplicated containerToHost entry on port 9998");
    rtCfg.thunder      = true;
    rtCfg.capabilities = "urn:rdk:permission:firebolt,urn:rdk:permission:thunder";

    EXPECT_TRUE(acc.applyRuntimeAndAppConfigToOCIConfig(ociConfig, rtCfg, appCfg));

    const Json::Value &ctoh = ociConfig[ralf::RDKPLUGINS][ralf::NETWORKING][ralf::DATA][ralf::PORT_FORWARDING][ralf::CONTAINER_TO_HOST];
    EXPECT_EQ(ctoh.size(), 1u);  // deduplicated to one entry
    EXPECT_EQ(ctoh[0][ralf::PORT].asInt(), ralf::THUNDER_JSONRPC_PORT);
    EXPECT_TRUE(ctoh[0][ralf::LOCALHOST_MASQUERADE].asBool());
}

TEST_F(RalfThunderFireboltNetworkTest, NoThunderNoFirebolt_NoContainerToHostEntry)
{
    TEST_LOG("No thunder/firebolt -> network stays closed, no containerToHost entry");
    rtCfg.capabilities = "urn:rdk:permission:home-app,urn:rdk:permission:rialto";

    EXPECT_TRUE(acc.applyRuntimeAndAppConfigToOCIConfig(ociConfig, rtCfg, appCfg));

    EXPECT_EQ(ociConfig[ralf::RDKPLUGINS][ralf::NETWORKING][ralf::DATA][ralf::TYPE].asString(), "closed");
    const Json::Value &pf = ociConfig[ralf::RDKPLUGINS][ralf::NETWORKING][ralf::DATA][ralf::PORT_FORWARDING];
    EXPECT_FALSE(pf.isMember(ralf::CONTAINER_TO_HOST));
}
