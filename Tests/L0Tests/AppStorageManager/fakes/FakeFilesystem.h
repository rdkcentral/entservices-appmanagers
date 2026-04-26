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

#pragma once

#include <sys/stat.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <map>
#include <string>
#include <vector>
#include <cstdint>

namespace L0Test {

/**
 * @brief Fake filesystem operations for testing without real file I/O
 */
class FilesystemShim {
public:
    static FilesystemShim& getInstance();

    // Control interface
    void Reset();
    void SetMkdirResult(bool success, int errorCode = 0);
    void SetRmdirResult(bool success, int errorCode = 0);
    void SetChownResult(bool success, int errorCode = 0);
    void SetStatResult(bool success, const struct stat& st);
    void SetStatvfsResult(bool success, const struct statvfs& st);
    void SetDirectoryExists(const std::string& path, bool exists);
    void SetDirectorySize(const std::string& path, uint64_t bytes);
    void SetDirectoryContents(const std::string& path, const std::vector<std::string>& entries);

    // Mock filesystem operations
    int mkdir(const char* path, mode_t mode);
    int rmdir(const char* path);
    int chown(const char* path, uid_t owner, gid_t group);
    int stat(const char* path, struct stat* buf);
    int statvfs(const char* path, struct statvfs* buf);

    // Statistics
    uint32_t GetMkdirCallCount() const { return _mkdirCalls; }
    uint32_t GetRmdirCallCount() const { return _rmdirCalls; }
    uint32_t GetChownCallCount() const { return _chownCalls; }
    uint32_t GetStatCallCount() const { return _statCalls; }
    uint32_t GetStatvfsCallCount() const { return _statvfsCalls; }
    std::vector<std::string> GetCreatedDirectories() const { return _createdDirs; }
    std::string GetLastPath() const { return _lastPath; }

private:
    FilesystemShim();
    ~FilesystemShim() = default;
    FilesystemShim(const FilesystemShim&) = delete;
    FilesystemShim& operator=(const FilesystemShim&) = delete;

    std::map<std::string, bool> _dirExists;
    std::map<std::string, uint64_t> _dirSizes;
    std::map<std::string, std::vector<std::string>> _dirContents;
    std::map<std::string, struct stat> _statResults;
    
    bool _mkdirSuccess;
    bool _rmdirSuccess;
    bool _chownSuccess;
    bool _statSuccess;
    bool _statvfsSuccess;
    
    int _errorCode;
    struct statvfs _statvfsResult;
    
    std::vector<std::string> _createdDirs;
    std::string _lastPath;
    
    uint32_t _mkdirCalls;
    uint32_t _rmdirCalls;
    uint32_t _chownCalls;
    uint32_t _statCalls;
    uint32_t _statvfsCalls;
};

} // namespace L0Test

// Conditional compilation macros for RequestHandler.cpp
#ifdef UNIT_TEST
#define FS_MKDIR(path, mode) L0Test::FilesystemShim::getInstance().mkdir(path, mode)
#define FS_RMDIR(path) L0Test::FilesystemShim::getInstance().rmdir(path)
#define FS_CHOWN(path, uid, gid) L0Test::FilesystemShim::getInstance().chown(path, uid, gid)
#define FS_STAT(path, buf) L0Test::FilesystemShim::getInstance().stat(path, buf)
#define FS_STATVFS(path, buf) L0Test::FilesystemShim::getInstance().statvfs(path, buf)
#else
#define FS_MKDIR(path, mode) ::mkdir(path, mode)
#define FS_RMDIR(path) ::rmdir(path)
#define FS_CHOWN(path, uid, gid) ::chown(path, uid, gid)
#define FS_STAT(path, buf) ::stat(path, buf)
#define FS_STATVFS(path, buf) ::statvfs(path, buf)
#endif
