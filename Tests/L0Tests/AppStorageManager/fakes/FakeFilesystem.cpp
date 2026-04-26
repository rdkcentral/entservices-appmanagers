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

#include "FakeFilesystem.h"
#include <cstring>
#include <cerrno>

namespace L0Test {

FilesystemShim::FilesystemShim()
    : _mkdirSuccess(true)
    , _rmdirSuccess(true)
    , _chownSuccess(true)
    , _statSuccess(true)
    , _statvfsSuccess(true)
    , _errorCode(0)
    , _mkdirCalls(0)
    , _rmdirCalls(0)
    , _chownCalls(0)
    , _statCalls(0)
    , _statvfsCalls(0)
{
    std::memset(&_statvfsResult, 0, sizeof(_statvfsResult));
    // Default: 1 GB free space
    _statvfsResult.f_bsize = 4096;
    _statvfsResult.f_bavail = 262144; // 1 GB / 4096
}

FilesystemShim& FilesystemShim::getInstance()
{
    static FilesystemShim instance;
    return instance;
}

void FilesystemShim::Reset()
{
    _dirExists.clear();
    _dirSizes.clear();
    _dirContents.clear();
    _statResults.clear();
    _createdDirs.clear();
    _lastPath.clear();
    
    _mkdirSuccess = true;
    _rmdirSuccess = true;
    _chownSuccess = true;
    _statSuccess = true;
    _statvfsSuccess = true;
    _errorCode = 0;
    
    _mkdirCalls = 0;
    _rmdirCalls = 0;
    _chownCalls = 0;
    _statCalls = 0;
    _statvfsCalls = 0;

    std::memset(&_statvfsResult, 0, sizeof(_statvfsResult));
    _statvfsResult.f_bsize = 4096;
    _statvfsResult.f_bavail = 262144;
}

void FilesystemShim::SetMkdirResult(bool success, int errorCode)
{
    _mkdirSuccess = success;
    _errorCode = errorCode;
}

void FilesystemShim::SetRmdirResult(bool success, int errorCode)
{
    _rmdirSuccess = success;
    _errorCode = errorCode;
}

void FilesystemShim::SetChownResult(bool success, int errorCode)
{
    _chownSuccess = success;
    _errorCode = errorCode;
}

void FilesystemShim::SetStatResult(bool success, const struct stat& st)
{
    _statSuccess = success;
    if (success) {
        _statResults[_lastPath] = st;
    }
}

void FilesystemShim::SetStatvfsResult(bool success, const struct statvfs& st)
{
    _statvfsSuccess = success;
    _statvfsResult = st;
}

void FilesystemShim::SetDirectoryExists(const std::string& path, bool exists)
{
    _dirExists[path] = exists;
}

void FilesystemShim::SetDirectorySize(const std::string& path, uint64_t bytes)
{
    _dirSizes[path] = bytes;
}

void FilesystemShim::SetDirectoryContents(const std::string& path, const std::vector<std::string>& entries)
{
    _dirContents[path] = entries;
}

int FilesystemShim::mkdir(const char* path, mode_t mode)
{
    (void)mode; // Unused in mock
    _mkdirCalls++;
    _lastPath = path;

    if (!_mkdirSuccess) {
        errno = _errorCode ? _errorCode : EACCES;
        return -1;
    }

    _createdDirs.push_back(path);
    _dirExists[path] = true;
    return 0;
}

int FilesystemShim::rmdir(const char* path)
{
    _rmdirCalls++;
    _lastPath = path;

    if (!_rmdirSuccess) {
        errno = _errorCode ? _errorCode : EACCES;
        return -1;
    }

    _dirExists[path] = false;
    return 0;
}

int FilesystemShim::chown(const char* path, uid_t owner, gid_t group)
{
    (void)owner;  // Unused in mock
    (void)group;  // Unused in mock
    _chownCalls++;
    _lastPath = path;

    if (!_chownSuccess) {
        errno = _errorCode ? _errorCode : EPERM;
        return -1;
    }

    return 0;
}

int FilesystemShim::stat(const char* path, struct stat* buf)
{
    _statCalls++;
    _lastPath = path;

    if (!_statSuccess) {
        errno = _errorCode ? _errorCode : ENOENT;
        return -1;
    }

    // Check if we have a pre-configured result
    auto it = _statResults.find(path);
    if (it != _statResults.end()) {
        *buf = it->second;
        return 0;
    }

    // Default behavior
    std::memset(buf, 0, sizeof(struct stat));
    
    auto existsIt = _dirExists.find(path);
    if (existsIt != _dirExists.end() && existsIt->second) {
        buf->st_mode = S_IFDIR | 0755;
        buf->st_nlink = 2;
        
        auto sizeIt = _dirSizes.find(path);
        if (sizeIt != _dirSizes.end()) {
            buf->st_size = sizeIt->second;
        }
        return 0;
    }

    errno = ENOENT;
    return -1;
}

int FilesystemShim::statvfs(const char* path, struct statvfs* buf)
{
    _statvfsCalls++;
    _lastPath = path;

    if (!_statvfsSuccess) {
        errno = _errorCode ? _errorCode : ENOENT;
        return -1;
    }

    *buf = _statvfsResult;
    return 0;
}

} // namespace L0Test
