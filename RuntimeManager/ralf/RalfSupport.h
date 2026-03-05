
/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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
#pragma once

#include <string>
#include <vector>
#include <json/json.h>

#include "RalfConstants.h"
namespace ralf
{
    /**
     * Function to create directories recursively, with default permissions 0777
     * @param path The directory path to create
     * @return true on success, false on failure
     */
    bool create_directories(const std::string &path, int uid = 0, int gid = 0);
    /**
     * Given the Ralf package configuration data, parse and extract package metadata paths and mount paths
     * @param configData The path to the Ralf package configuration file
     * @param packages [out parameter] A vector of pairs containing package metadata paths and mount paths
     * @return true on success, false on failure
     */
    bool parseRalPkgInfo(const std::string &configData, std::vector<RalfPkgInfoPair> &packages);
    /**
     * Function to read JSON data from a file
     * @param filePath The path to the JSON file
     * @param jsonData [out parameter] The parsed JSON data
     * @return true on success, false on failure
     */
    bool JsonFromFile(const std::string &filePath, Json::Value &rootNode);
    /**
     *
     * Function to mount overlay filesystem. This function will create the directories if they do not exist.
     * The directory structure is as follows
     * RALF_APP_ROOTFS_DIR/appInstanceId/rootfs  (this is the mount point)
     * The overlay filesystem will use the pkgmountPaths as lowerdir and RALF_APP_ROOTFS_DIR/appInstanceId as workdir
     * @param appInstanceId The application instance ID
     * @param pkgmountPaths The colon separated list of package mount paths to be used as lowerdir
     * @param uid The user ID to set as owner of the created directories
     * @param gid The group ID to set as owner of the created directories
     * @param ociRootfsPath [out parameter] The path to the mounted OCI root filesystem
     * @return true on success, false on failure
     */
    bool generateOCIRootfs(const std::string appInstanceId, const std::string &pkgmountPaths, const int uid, const int gid, std::string &ociRootfsPath);
    /**
     * Given a device node path, this function will return the major and minor numbers.
     * @param devNodePath : Input device node path.
     * @param majorNum : out parameter - The major number of the device node.
     * @param minorNum : out parameter - The minor number of the device node.
     * @param devType : out parameter - The device type ('c' for character, 'b' for block).
     * @return true if successful, false otherwise.
     */

    bool getDevNodeMajorMinor(const std::string &devNodePath, unsigned int &majorNum, unsigned int &minorNum, char &devType);

    /**
     * Function to check if a given path exists
     * @param path The path to check
     * @return true if the path exists, false otherwise
     */
    bool checkIfPathExists(const std::string &path);

    /**
     * Function to unmount the overlay filesystem for a given app instance ID
     * @param overlayfsMountPath The mount path of the overlay filesystem to unmount
     * @return true on success, false on failure
     */
    bool unmountOverlayfs(const std::string &overlayfsMountPath);

    /**
     * Given a group name, this function will return the corresponding group ID.
     * @param grpName : Input group name.
     * @param gid : out parameter - The group ID.
     * @return true if successful, false otherwise.
     */
    bool getGroupId(const std::string &grpName, uint32_t &gid);

    /**
     * This function will return user id and group id for the Ralf user.
     * @param userId : out parameter - The user ID.
     * @param groupId : out parameter - The group ID.
     * @return true if successful, false otherwise.
     */
    bool getRalfUserInfo(uid_t &userId, gid_t &groupId);

    /**
     * Function to parse memory size strings (e.g., "512M", "2G") into bytes.
     * @param str The memory size string to parse.
     * @return The memory size in bytes, or 0 on failure.
     */
    uint64_t parseMemorySize(const std::string &str);
} // namespace ralf
