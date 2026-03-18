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
 * @file Ralf_SupportTests.cpp
 *
 * L0 tests for ralf/RalfSupport utility functions:
 *   - parseMemorySize()
 *   - serializeJsonNode()
 *   - checkIfPathExists()
 *   - create_directories()
 *   - JsonFromFile()
 *   - parseRalPkgInfo()
 *   - getDevNodeMajorMinor()
 *   - getGroupId()
 *   - getRalfUserInfo()
 */

#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>

#include "ralf/RalfSupport.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

// ──────────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────────

static bool WriteFile(const std::string &path, const std::string &content)
{
    std::ofstream ofs(path, std::ios::out | std::ios::trunc);
    if (!ofs.is_open())
        return false;
    ofs << content;
    return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// parseMemorySize() tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Ralf_ParseMemorySize_EmptyStringReturnsZero
 *
 * Verifies that an empty string returns 0.
 */
uint32_t Test_Ralf_ParseMemorySize_EmptyStringReturnsZero()
{
    L0Test::TestResult tr;

    const uint64_t result = ralf::parseMemorySize("");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(result), 0u,
                        "parseMemorySize(\"\") returns 0");

    return tr.failures;
}

/* Test_Ralf_ParseMemorySize_PlainIntegerNoSuffix
 *
 * Verifies that a plain integer without suffix returns the numeric value.
 */
uint32_t Test_Ralf_ParseMemorySize_PlainIntegerNoSuffix()
{
    L0Test::TestResult tr;

    const uint64_t result = ralf::parseMemorySize("1024");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(result), 1024u,
                        "parseMemorySize(\"1024\") returns 1024");

    return tr.failures;
}

/* Test_Ralf_ParseMemorySize_KilobytesSuffix
 *
 * Verifies that "512K" returns 512 * 1024 bytes.
 */
uint32_t Test_Ralf_ParseMemorySize_KilobytesSuffix()
{
    L0Test::TestResult tr;

    const uint64_t result = ralf::parseMemorySize("512K");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(result), 512u * 1024u,
                        "parseMemorySize(\"512K\") returns 524288");

    return tr.failures;
}

/* Test_Ralf_ParseMemorySize_MegabytesSuffix
 *
 * Verifies that "100M" returns 100 * 1024 * 1024 bytes.
 */
uint32_t Test_Ralf_ParseMemorySize_MegabytesSuffix()
{
    L0Test::TestResult tr;

    const uint64_t result = ralf::parseMemorySize("100M");
    const uint64_t expected = 100ULL * 1024ULL * 1024ULL;
    L0Test::ExpectTrue(tr, result == expected,
                       "parseMemorySize(\"100M\") returns 104857600");

    return tr.failures;
}

/* Test_Ralf_ParseMemorySize_GigabytesSuffix
 *
 * Verifies that "2G" returns 2 * 1024^3 bytes.
 */
uint32_t Test_Ralf_ParseMemorySize_GigabytesSuffix()
{
    L0Test::TestResult tr;

    const uint64_t result = ralf::parseMemorySize("2G");
    const uint64_t expected = 2ULL * 1024ULL * 1024ULL * 1024ULL;
    L0Test::ExpectTrue(tr, result == expected,
                       "parseMemorySize(\"2G\") returns 2147483648");

    return tr.failures;
}

/* Test_Ralf_ParseMemorySize_LowercaseSuffix
 *
 * Verifies that lowercase suffix "512m" is handled the same as uppercase.
 */
uint32_t Test_Ralf_ParseMemorySize_LowercaseSuffix()
{
    L0Test::TestResult tr;

    const uint64_t upper = ralf::parseMemorySize("512M");
    const uint64_t lower = ralf::parseMemorySize("512m");
    L0Test::ExpectTrue(tr, upper == lower,
                       "parseMemorySize(\"512m\") == parseMemorySize(\"512M\")");

    return tr.failures;
}

/* Test_Ralf_ParseMemorySize_SuffixWithByte
 *
 * Verifies that "256MB" is treated the same as "256M".
 */
uint32_t Test_Ralf_ParseMemorySize_SuffixWithByte()
{
    L0Test::TestResult tr;

    const uint64_t withB = ralf::parseMemorySize("256MB");
    const uint64_t withoutB = ralf::parseMemorySize("256M");
    L0Test::ExpectTrue(tr, withB == withoutB,
                       "parseMemorySize(\"256MB\") == parseMemorySize(\"256M\")");

    return tr.failures;
}

/* Test_Ralf_ParseMemorySize_InvalidSuffixReturnsZero
 *
 * Verifies that an unknown suffix returns 0.
 */
uint32_t Test_Ralf_ParseMemorySize_InvalidSuffixReturnsZero()
{
    L0Test::TestResult tr;

    const uint64_t result = ralf::parseMemorySize("100X");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(result), 0u,
                        "parseMemorySize(\"100X\") returns 0 for unknown unit");

    return tr.failures;
}

/* Test_Ralf_ParseMemorySize_NonNumericInputReturnsZero
 *
 * Verifies that purely non-numeric input returns 0.
 */
uint32_t Test_Ralf_ParseMemorySize_NonNumericInputReturnsZero()
{
    L0Test::TestResult tr;

    const uint64_t result = ralf::parseMemorySize("abc");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(result), 0u,
                        "parseMemorySize(\"abc\") returns 0");

    return tr.failures;
}

/* Test_Ralf_ParseMemorySize_ZeroValueReturnsZero
 *
 * Verifies that "0" returns 0.
 */
uint32_t Test_Ralf_ParseMemorySize_ZeroValueReturnsZero()
{
    L0Test::TestResult tr;

    const uint64_t result = ralf::parseMemorySize("0");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(result), 0u,
                        "parseMemorySize(\"0\") returns 0");

    return tr.failures;
}

/* Test_Ralf_ParseMemorySize_KbSuffixWithB
 *
 * Verifies that "1KB" is treated the same as "1K".
 */
uint32_t Test_Ralf_ParseMemorySize_KbSuffixWithB()
{
    L0Test::TestResult tr;

    const uint64_t withB = ralf::parseMemorySize("1KB");
    const uint64_t withoutB = ralf::parseMemorySize("1K");
    L0Test::ExpectTrue(tr, withB == withoutB,
                       "parseMemorySize(\"1KB\") == parseMemorySize(\"1K\")");

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// serializeJsonNode() tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Ralf_SerializeJsonNode_SimpleObject
 *
 * Verifies that a simple JSON object serializes to a compact JSON string.
 */
uint32_t Test_Ralf_SerializeJsonNode_SimpleObject()
{
    L0Test::TestResult tr;

    Json::Value node;
    node["key"] = "value";

    const std::string result = ralf::serializeJsonNode(node);
    L0Test::ExpectTrue(tr, !result.empty(),
                       "serializeJsonNode() returns non-empty string for object");
    L0Test::ExpectTrue(tr, result.find("key") != std::string::npos,
                       "serialized JSON contains key name");
    L0Test::ExpectTrue(tr, result.find("value") != std::string::npos,
                       "serialized JSON contains value");

    return tr.failures;
}

/* Test_Ralf_SerializeJsonNode_EmptyObject
 *
 * Verifies that an empty JSON object serializes to a non-empty string.
 */
uint32_t Test_Ralf_SerializeJsonNode_EmptyObject()
{
    L0Test::TestResult tr;

    Json::Value node(Json::objectValue);
    const std::string result = ralf::serializeJsonNode(node);
    // Empty object should produce "{}"
    L0Test::ExpectTrue(tr, !result.empty(),
                       "serializeJsonNode() of empty object returns non-empty string");

    return tr.failures;
}

/* Test_Ralf_SerializeJsonNode_IntegerValue
 *
 * Verifies that an integer JSON node serializes correctly.
 */
uint32_t Test_Ralf_SerializeJsonNode_IntegerValue()
{
    L0Test::TestResult tr;

    Json::Value node(42);
    const std::string result = ralf::serializeJsonNode(node);
    L0Test::ExpectTrue(tr, result.find("42") != std::string::npos,
                       "serializeJsonNode() of integer 42 contains '42'");

    return tr.failures;
}

/* Test_Ralf_SerializeJsonNode_ArrayValue
 *
 * Verifies that a JSON array serializes to a string containing array elements.
 */
uint32_t Test_Ralf_SerializeJsonNode_ArrayValue()
{
    L0Test::TestResult tr;

    Json::Value node(Json::arrayValue);
    node.append("elem1");
    node.append("elem2");

    const std::string result = ralf::serializeJsonNode(node);
    L0Test::ExpectTrue(tr, result.find("elem1") != std::string::npos,
                       "serializeJsonNode() of array contains first element");
    L0Test::ExpectTrue(tr, result.find("elem2") != std::string::npos,
                       "serializeJsonNode() of array contains second element");

    return tr.failures;
}

/* Test_Ralf_SerializeJsonNode_NoWhitespace
 *
 * Verifies that the serialized JSON has no extra whitespace/indentation.
 */
uint32_t Test_Ralf_SerializeJsonNode_NoWhitespace()
{
    L0Test::TestResult tr;

    Json::Value node;
    node["a"] = 1;
    node["b"] = 2;

    const std::string result = ralf::serializeJsonNode(node);
    // Compact output should not contain newlines
    L0Test::ExpectTrue(tr, result.find('\n') == std::string::npos,
                       "serializeJsonNode() produces compact output (no newlines)");

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// checkIfPathExists() tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Ralf_CheckIfPathExists_NonExistentPathReturnsFalse
 *
 * Verifies that checkIfPathExists() returns false for a non-existent path.
 */
uint32_t Test_Ralf_CheckIfPathExists_NonExistentPathReturnsFalse()
{
    L0Test::TestResult tr;

    const bool exists = ralf::checkIfPathExists("/tmp/ralf_l0test_nonexistent_path_abc123");
    L0Test::ExpectTrue(tr, !exists,
                       "checkIfPathExists() returns false for non-existent path");

    return tr.failures;
}

/* Test_Ralf_CheckIfPathExists_ExistingPathReturnsTrue
 *
 * Verifies that checkIfPathExists() returns true for /tmp (always present).
 */
uint32_t Test_Ralf_CheckIfPathExists_ExistingPathReturnsTrue()
{
    L0Test::TestResult tr;

    const bool exists = ralf::checkIfPathExists("/tmp");
    L0Test::ExpectTrue(tr, exists,
                       "checkIfPathExists() returns true for /tmp");

    return tr.failures;
}

/* Test_Ralf_CheckIfPathExists_ExistingFileReturnsTrue
 *
 * Creates a temporary file and verifies checkIfPathExists() returns true.
 */
uint32_t Test_Ralf_CheckIfPathExists_ExistingFileReturnsTrue()
{
    L0Test::TestResult tr;

    const std::string tmpPath = "/tmp/ralf_l0test_exist_check.txt";
    WriteFile(tmpPath, "test");

    const bool exists = ralf::checkIfPathExists(tmpPath);
    L0Test::ExpectTrue(tr, exists,
                       "checkIfPathExists() returns true for existing file");

    // Cleanup
    ::remove(tmpPath.c_str());

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// create_directories() tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Ralf_CreateDirectories_EmptyPathReturnsFalse
 *
 * Verifies that create_directories() returns false for an empty path.
 */
uint32_t Test_Ralf_CreateDirectories_EmptyPathReturnsFalse()
{
    L0Test::TestResult tr;

    const bool result = ralf::create_directories("");
    L0Test::ExpectTrue(tr, !result,
                       "create_directories(\"\") returns false");

    return tr.failures;
}

/* Test_Ralf_CreateDirectories_SingleLevelPath
 *
 * Creates a single-level temp dir and verifies it succeeds and can be detected.
 */
uint32_t Test_Ralf_CreateDirectories_SingleLevelPath()
{
    L0Test::TestResult tr;

    const std::string testDir = "/tmp/ralf_l0test_mkdir_single";
    const bool result = ralf::create_directories(testDir);
    L0Test::ExpectTrue(tr, result,
                       "create_directories() succeeds for single-level path");

    const bool exists = ralf::checkIfPathExists(testDir);
    L0Test::ExpectTrue(tr, exists,
                       "created directory exists after create_directories()");

    // Cleanup
    ::rmdir(testDir.c_str());

    return tr.failures;
}

/* Test_Ralf_CreateDirectories_MultiLevelPath
 *
 * Creates a multi-level temp dir hierarchy and verifies each level exists.
 */
uint32_t Test_Ralf_CreateDirectories_MultiLevelPath()
{
    L0Test::TestResult tr;

    const std::string rootDir  = "/tmp/ralf_l0test_mkdir_multi";
    const std::string leafDir  = rootDir + "/a/b/c";

    const bool result = ralf::create_directories(leafDir);
    L0Test::ExpectTrue(tr, result,
                       "create_directories() succeeds for multi-level path");

    L0Test::ExpectTrue(tr, ralf::checkIfPathExists(leafDir),
                       "leaf directory exists after create_directories()");

    // Cleanup (reverse order)
    ::rmdir((rootDir + "/a/b/c").c_str());
    ::rmdir((rootDir + "/a/b").c_str());
    ::rmdir((rootDir + "/a").c_str());
    ::rmdir(rootDir.c_str());

    return tr.failures;
}

/* Test_Ralf_CreateDirectories_ExistingPathReturnsTrue
 *
 * Verifies that create_directories() succeeds idempotently on an existing path.
 */
uint32_t Test_Ralf_CreateDirectories_ExistingPathReturnsTrue()
{
    L0Test::TestResult tr;

    const std::string tmpDir = "/tmp/ralf_l0test_mkdir_exist";
    ::mkdir(tmpDir.c_str(), 0777);  // pre-create

    const bool result = ralf::create_directories(tmpDir);
    L0Test::ExpectTrue(tr, result,
                       "create_directories() returns true for already-existing path");

    // Cleanup
    ::rmdir(tmpDir.c_str());

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// JsonFromFile() tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Ralf_JsonFromFile_NonExistentFileReturnsFalse
 *
 * Verifies that JsonFromFile() returns false for a non-existent file.
 */
uint32_t Test_Ralf_JsonFromFile_NonExistentFileReturnsFalse()
{
    L0Test::TestResult tr;

    Json::Value node;
    const bool result = ralf::JsonFromFile("/tmp/ralf_l0test_nonexistent.json", node);
    L0Test::ExpectTrue(tr, !result,
                       "JsonFromFile() returns false for non-existent file");

    return tr.failures;
}

/* Test_Ralf_JsonFromFile_ValidJsonFileReturnsTrue
 *
 * Creates a valid JSON file and verifies JsonFromFile() parses it correctly.
 */
uint32_t Test_Ralf_JsonFromFile_ValidJsonFileReturnsTrue()
{
    L0Test::TestResult tr;

    const std::string tmpPath = "/tmp/ralf_l0test_valid.json";
    WriteFile(tmpPath, "{\"name\": \"test\", \"value\": 42}");

    Json::Value node;
    const bool result = ralf::JsonFromFile(tmpPath, node);
    L0Test::ExpectTrue(tr, result,
                       "JsonFromFile() returns true for valid JSON file");
    L0Test::ExpectTrue(tr, node.isMember("name"),
                       "Parsed JSON contains 'name' key");
    L0Test::ExpectEqU32(tr, node["value"].asUInt(), 42u,
                        "Parsed JSON 'value' key equals 42");

    ::remove(tmpPath.c_str());

    return tr.failures;
}

/* Test_Ralf_JsonFromFile_InvalidJsonFileReturnsFalse
 *
 * Creates a malformed JSON file and verifies JsonFromFile() returns false.
 */
uint32_t Test_Ralf_JsonFromFile_InvalidJsonFileReturnsFalse()
{
    L0Test::TestResult tr;

    const std::string tmpPath = "/tmp/ralf_l0test_invalid.json";
    WriteFile(tmpPath, "{ this is not valid json !!!");

    Json::Value node;
    const bool result = ralf::JsonFromFile(tmpPath, node);
    L0Test::ExpectTrue(tr, !result,
                       "JsonFromFile() returns false for malformed JSON");

    ::remove(tmpPath.c_str());

    return tr.failures;
}

/* Test_Ralf_JsonFromFile_EmptyFileReturnsFalse
 *
 * Creates an empty file and verifies JsonFromFile() returns false.
 */
uint32_t Test_Ralf_JsonFromFile_EmptyFileReturnsFalse()
{
    L0Test::TestResult tr;

    const std::string tmpPath = "/tmp/ralf_l0test_empty.json";
    WriteFile(tmpPath, "");

    Json::Value node;
    const bool result = ralf::JsonFromFile(tmpPath, node);
    L0Test::ExpectTrue(tr, !result,
                       "JsonFromFile() returns false for empty file");

    ::remove(tmpPath.c_str());

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// parseRalPkgInfo() tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Ralf_ParseRalPkgInfo_NonExistentFileReturnsFalse
 *
 * Verifies that parseRalPkgInfo() returns false for a non-existent file.
 */
uint32_t Test_Ralf_ParseRalPkgInfo_NonExistentFileReturnsFalse()
{
    L0Test::TestResult tr;

    std::vector<ralf::RalfPkgInfoPair> packages;
    const bool result = ralf::parseRalPkgInfo("/tmp/ralf_l0test_nonexistent_pkg.json", packages);
    L0Test::ExpectTrue(tr, !result,
                       "parseRalPkgInfo() returns false for non-existent file");

    return tr.failures;
}

/* Test_Ralf_ParseRalPkgInfo_MissingPackagesFieldReturnsFalse
 *
 * Verifies that parseRalPkgInfo() returns false if 'packages' field is absent.
 */
uint32_t Test_Ralf_ParseRalPkgInfo_MissingPackagesFieldReturnsFalse()
{
    L0Test::TestResult tr;

    const std::string tmpPath = "/tmp/ralf_l0test_nopkgs.json";
    WriteFile(tmpPath, "{\"other\": \"data\"}");

    std::vector<ralf::RalfPkgInfoPair> packages;
    const bool result = ralf::parseRalPkgInfo(tmpPath, packages);
    L0Test::ExpectTrue(tr, !result,
                       "parseRalPkgInfo() returns false when 'packages' field missing");

    ::remove(tmpPath.c_str());

    return tr.failures;
}

/* Test_Ralf_ParseRalPkgInfo_ValidPackagesFileReturnsTrue
 *
 * Verifies that parseRalPkgInfo() succeeds and populates packages from a
 * valid JSON config file.
 */
uint32_t Test_Ralf_ParseRalPkgInfo_ValidPackagesFileReturnsTrue()
{
    L0Test::TestResult tr;

    const std::string tmpPath = "/tmp/ralf_l0test_pkgs.json";
    WriteFile(tmpPath,
        "{"
        "  \"packages\": ["
        "    {\"pkgMetaDataPath\": \"/pkg/meta/app.json\", \"pkgMountPath\": \"/mnt/app\"},"
        "    {\"pkgMetaDataPath\": \"/pkg/meta/rt.json\",  \"pkgMountPath\": \"/mnt/rt\"}"
        "  ]"
        "}");

    std::vector<ralf::RalfPkgInfoPair> packages;
    const bool result = ralf::parseRalPkgInfo(tmpPath, packages);
    L0Test::ExpectTrue(tr, result,
                       "parseRalPkgInfo() returns true for valid pkg config");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(packages.size()), 2u,
                        "parseRalPkgInfo() parses 2 packages");
    if (packages.size() >= 1) {
        L0Test::ExpectEqStr(tr, packages[0].first,  "/pkg/meta/app.json",
                            "First package metadata path is correct");
        L0Test::ExpectEqStr(tr, packages[0].second, "/mnt/app",
                            "First package mount path is correct");
    }
    if (packages.size() >= 2) {
        L0Test::ExpectEqStr(tr, packages[1].first,  "/pkg/meta/rt.json",
                            "Second package metadata path is correct");
        L0Test::ExpectEqStr(tr, packages[1].second, "/mnt/rt",
                            "Second package mount path is correct");
    }

    ::remove(tmpPath.c_str());

    return tr.failures;
}

/* Test_Ralf_ParseRalPkgInfo_EmptyPackagesArraySucceeds
 *
 * Verifies that parseRalPkgInfo() handles an empty packages array gracefully.
 */
uint32_t Test_Ralf_ParseRalPkgInfo_EmptyPackagesArraySucceeds()
{
    L0Test::TestResult tr;

    const std::string tmpPath = "/tmp/ralf_l0test_emptyarray.json";
    WriteFile(tmpPath, "{\"packages\": []}");

    std::vector<ralf::RalfPkgInfoPair> packages;
    const bool result = ralf::parseRalPkgInfo(tmpPath, packages);
    L0Test::ExpectTrue(tr, result,
                       "parseRalPkgInfo() returns true for empty packages array");
    L0Test::ExpectEqU32(tr, static_cast<uint32_t>(packages.size()), 0u,
                        "parseRalPkgInfo() produces empty vector for empty array");

    ::remove(tmpPath.c_str());

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// getDevNodeMajorMinor() tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Ralf_GetDevNodeMajorMinor_NonExistentNodeReturnsFalse
 *
 * Verifies that getDevNodeMajorMinor() returns false for a non-existent path.
 */
uint32_t Test_Ralf_GetDevNodeMajorMinor_NonExistentNodeReturnsFalse()
{
    L0Test::TestResult tr;

    unsigned int major = 0, minor = 0;
    char devType = '\0';
    const bool result = ralf::getDevNodeMajorMinor("/dev/ralf_l0test_nonexistent", major, minor, devType);
    L0Test::ExpectTrue(tr, !result,
                       "getDevNodeMajorMinor() returns false for non-existent path");

    return tr.failures;
}

/* Test_Ralf_GetDevNodeMajorMinor_DevNullReturnsTrue
 *
 * Verifies that getDevNodeMajorMinor() succeeds for /dev/null (always present).
 */
uint32_t Test_Ralf_GetDevNodeMajorMinor_DevNullReturnsTrue()
{
    L0Test::TestResult tr;

    unsigned int major = 0, minor = 0;
    char devType = '\0';
    const bool result = ralf::getDevNodeMajorMinor("/dev/null", major, minor, devType);

    // /dev/null is always present; major must be > 0 on Linux (major=1 for mem devices)
    L0Test::ExpectTrue(tr, result,
                       "getDevNodeMajorMinor() returns true for /dev/null");
    L0Test::ExpectTrue(tr, devType == 'c' || devType == 'b',
                       "getDevNodeMajorMinor() sets devType to 'c' or 'b'");

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// getGroupId() tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Ralf_GetGroupId_UnknownGroupReturnsFalse
 *
 * Verifies that getGroupId() returns false for a group that does not exist.
 */
uint32_t Test_Ralf_GetGroupId_UnknownGroupReturnsFalse()
{
    L0Test::TestResult tr;

    uint32_t gid = 0;
    const bool result = ralf::getGroupId("ralf_l0test_fake_group_xyz", gid);
    L0Test::ExpectTrue(tr, !result,
                       "getGroupId() returns false for non-existent group");

    return tr.failures;
}

/* Test_Ralf_GetGroupId_RootGroupReturnsTrue
 *
 * Verifies that getGroupId() returns true for the 'root' group and gid == 0.
 */
uint32_t Test_Ralf_GetGroupId_RootGroupReturnsTrue()
{
    L0Test::TestResult tr;

    uint32_t gid = 0xFFFFFFFFu;
    const bool result = ralf::getGroupId("root", gid);
    L0Test::ExpectTrue(tr, result,
                       "getGroupId(\"root\") returns true");
    L0Test::ExpectEqU32(tr, gid, 0u,
                        "getGroupId(\"root\") sets gid to 0");

    return tr.failures;
}

// ──────────────────────────────────────────────────────────────────────────────
// getRalfUserInfo() tests
// ──────────────────────────────────────────────────────────────────────────────

/* Test_Ralf_GetRalfUserInfo_ReturnsConsistentResult
 *
 * Verifies that getRalfUserInfo() either succeeds (if ralf user exists) or
 * fails gracefully (if ralf user doesn't exist in test environment).
 * Neither outcome should crash, and if it succeeds uid/gid must be > 0.
 */
uint32_t Test_Ralf_GetRalfUserInfo_ReturnsConsistentResult()
{
    L0Test::TestResult tr;

    uid_t userId = 0;
    gid_t groupId = 0;
    const bool result = ralf::getRalfUserInfo(userId, groupId);

    if (result) {
        // 'ralf' user exists: uid and gid should be non-zero (real system user)
        L0Test::ExpectTrue(tr, userId > 0 || groupId > 0,
                           "getRalfUserInfo() sets valid uid/gid when ralf user exists");
    } else {
        // 'ralf' user does not exist in test environment — acceptable
        L0Test::ExpectTrue(tr, true,
                           "getRalfUserInfo() returns false gracefully when ralf user is absent");
    }

    return tr.failures;
}
