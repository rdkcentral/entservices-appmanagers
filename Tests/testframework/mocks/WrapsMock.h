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

#include <gmock/gmock.h>
#include <mntent.h>

#include "Wraps.h"

extern "C" FILE* __real_setmntent(const char* command, const char* type);
extern "C" struct mntent* __real_getmntent(FILE* pipe);
extern "C" int __real_access(const char* pathname, int mode);
extern "C" int __real_rmdir(const char* pathname);
extern "C" DIR* __real_opendir(const char* pathname);
extern "C" int __real_closedir(DIR* dirp);
extern "C" int __real_mkdir(const char* path, mode_t mode);
extern "C" struct dirent* __real_readdir(DIR* dirp);
extern "C" int __real_chown(const char *path, uid_t owner, gid_t group);
extern "C" int __real_nftw(const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags);
extern "C" int __real_open(const char* pathname, int flags, mode_t mode);
extern "C" int __real_stat(const char* path, struct stat* info);
extern "C" int __real_statvfs(const char* path, struct statvfs* buf);

#undef curl_easy_setopt
#undef curl_easy_getinfo
#undef curl_share_setopt
#undef curl_multi_setopt

class WrapsImplMock : public WrapsImpl {
public:
    WrapsImplMock():WrapsImpl()
    {
	   /*Setting up Default behavior for setmntent: 
        * We are mocking setmntent in this file below with __wrap_setmntent,
        * and the actual setmntent will be called via this interface */
        ON_CALL(*this, setmntent(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* command, const char* type) -> FILE* {
                return __real_setmntent(command, type);
            }));
        
        ON_CALL(*this, getmntent(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](FILE* pipe) -> struct mntent* {
                return __real_getmntent(pipe);
            }));
        
        ON_CALL(*this, access(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* pathname, int mode) -> int {
                return __real_access(pathname, mode);
            }));
        
        ON_CALL(*this, rmdir(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* pathname) -> int {
                return __real_rmdir(pathname);
            }));
        
        ON_CALL(*this, opendir(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* pathname) -> DIR* {
                return __real_opendir(pathname);
            }));
        
        ON_CALL(*this, closedir(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](DIR* dirp) -> int {
                return __real_closedir(dirp);
            }));
        
        ON_CALL(*this, mkdir(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* path, mode_t mode) -> int {
                return __real_mkdir(path, mode);
            }));
        
        ON_CALL(*this, readdir(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](DIR* dirp) -> struct dirent* {
                return __real_readdir(dirp);
            }));
        
        ON_CALL(*this, chown(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char *path, uid_t owner, gid_t group) -> int {
                return __real_chown(path, owner, group);
            }));
        
        ON_CALL(*this, nftw(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags) -> int {
                return __real_nftw(dirpath, fn, nopenfd, flags);
            }));
        
        ON_CALL(*this, open(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* pathname, int flags, mode_t mode) -> int {
                return __real_open(pathname, flags, mode);
            }));
        
        ON_CALL(*this, stat(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* path, struct stat* info) -> int {
                return __real_stat(path, info);
            }));
        
        ON_CALL(*this, statvfs(::testing::_, ::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const char* path, struct statvfs* buf) -> int {
                return __real_statvfs(path, buf);
            }));
    }
    virtual ~WrapsImplMock() = default;

    MOCK_METHOD(int, system, (const char* command), (override));
    MOCK_METHOD(FILE*, popen, (const char* command, const char* type), (override));
    MOCK_METHOD(int, pclose, (FILE* pipe), (override));
    MOCK_METHOD(void, syslog, (int pri, const char* fmt, va_list args), (override));
    MOCK_METHOD(FILE*, setmntent, (const char* command, const char* type), (override));
    MOCK_METHOD(struct mntent*, getmntent, (FILE* pipe), (override));
    MOCK_METHOD(struct wpa_ctrl*, wpa_ctrl_open, (const char *ctrl_path), (override));
    MOCK_METHOD(int, wpa_ctrl_request, (struct wpa_ctrl *ctrl, const char *cmd, size_t cmd_len,char *reply, size_t *reply_len,void (*msg_cb)(char *msg, size_t len)), (override));
    MOCK_METHOD(void, wpa_ctrl_close, (struct wpa_ctrl *ctrl), (override));
    MOCK_METHOD(int, wpa_ctrl_pending, (struct wpa_ctrl *ctrl), (override));
    MOCK_METHOD(int, wpa_ctrl_recv , (struct wpa_ctrl *ctrl, char *reply, size_t *reply_len), (override));
    MOCK_METHOD(int, wpa_ctrl_attach, (struct wpa_ctrl *ctrl), (override));
    MOCK_METHOD(FILE*, v_secure_popen, (const char *direction, const char *command, va_list args), (override));
    MOCK_METHOD(int, v_secure_pclose, (FILE *file), (override));
    MOCK_METHOD(int, unlink, (const char* filePath), (override));
    MOCK_METHOD(int, v_secure_system, (const char *command, va_list args), (override));
    MOCK_METHOD(ssize_t, readlink, (const char *pathname, char *buf, size_t bufsiz), (override));
    MOCK_METHOD(int, ioctl, (int fd,unsigned long request,void* arg), (override));
    MOCK_METHOD(int, statvfs, (const char* path,struct statvfs* buf), (override));
    MOCK_METHOD(int, statfs, (const char* path, struct statfs* buf), (override));
    MOCK_METHOD(std::istream&, getline, (std::istream& is, std::string& line), (override));
    MOCK_METHOD(int, mkdir, (const char* path, mode_t mode), (override));
    MOCK_METHOD(int, mount, (const char* source, const char* target, const char* filesystemtype, unsigned long mountflags, const void* data), ());
    MOCK_METHOD(int, stat, (const char* path, struct stat* info), (override));
    MOCK_METHOD(int, open, (const char* pathname, int flags, mode_t mode),(override));
    MOCK_METHOD(FILE*, fopen, (const char *pathname, const char *mode),(override));    
    MOCK_METHOD(int, umount, (const char* path), (override));
    MOCK_METHOD(int, rmdir, (const char* pathname), (override));
    MOCK_METHOD(time_t, time, (time_t* arg), (override));
    MOCK_METHOD(int, access, (const char* pathname, int mode), (override));
    MOCK_METHOD(int, chown, (const char *path, uid_t owner, gid_t group), (override));
    MOCK_METHOD(int, nftw, (const char* dirpath, int (*fn)(const char*, const struct stat*, int, struct FTW*), int nopenfd, int flags), (override));
    MOCK_METHOD(DIR* , opendir, (const char* pathname), (override));
    MOCK_METHOD(struct dirent*, readdir, (DIR*), (override));
    MOCK_METHOD(int, closedir, (DIR* dirp), (override));
    MOCK_METHOD(CURLcode, curl_easy_setopt, (CURL* curl, CURLoption option, void* param), (override));
    MOCK_METHOD(CURLcode, curl_easy_perform, (CURL* curl), (override));
    MOCK_METHOD(CURLcode, curl_easy_getinfo, (CURL* curl, CURLINFO info, long* value), (override));
    MOCK_METHOD(const char*, curl_easy_strerror, (CURLcode errornum), (override));
    MOCK_METHOD(CURL*, curl_easy_init, (), (override));
    MOCK_METHOD(void, curl_easy_cleanup, (CURL* handle), (override));


};
