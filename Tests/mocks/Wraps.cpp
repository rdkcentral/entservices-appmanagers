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

#include "Wraps.h"

#include <stdarg.h>
#include <sys/vfs.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <ftw.h>
#include <dirent.h>

WrapsImpl* Wraps::impl = nullptr;

void Wraps::setImpl(WrapsImpl* newImpl)
{
    impl = newImpl;
}

Wraps& Wraps::getInstance()
{
    static Wraps instance;
    return instance;
}

// ---------------------------------------------------------------------------
// Static method implementations (delegate to impl)
// ---------------------------------------------------------------------------

int Wraps::system(const char* command)
{
    return impl->system(command);
}

FILE* Wraps::popen(const char* command, const char* type)
{
    return impl->popen(command, type);
}

int Wraps::pclose(FILE* pipe)
{
    return impl->pclose(pipe);
}

void Wraps::syslog(int pri, const char* fmt, va_list args)
{
    impl->syslog(pri, fmt, args);
}

FILE* Wraps::setmntent(const char* command, const char* type)
{
    return impl->setmntent(command, type);
}

struct mntent* Wraps::getmntent(FILE* pipe)
{
    return impl->getmntent(pipe);
}

int Wraps::unlink(const char* filePath)
{
    return impl->unlink(filePath);
}

int Wraps::statvfs(const char* path, struct statvfs* buf)
{
    return impl->statvfs(path, buf);
}

int Wraps::statfs(const char* path, struct statfs* buf)
{
    return impl->statfs(path, buf);
}

int Wraps::mkdir(const char* path, mode_t mode)
{
    return impl->mkdir(path, mode);
}

int Wraps::stat(const char* path, struct stat* info)
{
    return impl->stat(path, info);
}

int Wraps::open(const char* pathname, int flags, mode_t mode)
{
    return impl->open(pathname, flags, mode);
}

int Wraps::umount(const char* path)
{
    return impl->umount(path);
}

int Wraps::rmdir(const char* pathname)
{
    return impl->rmdir(pathname);
}

int Wraps::access(const char* pathname, int mode)
{
    return impl->access(pathname, mode);
}

int Wraps::chown(const char* path, uid_t owner, gid_t group)
{
    return impl->chown(path, owner, group);
}

int Wraps::nftw(const char* dirpath,
                int (*fn)(const char*, const struct stat*, int, struct FTW*),
                int nopenfd, int flags)
{
    return impl->nftw(dirpath, fn, nopenfd, flags);
}

DIR* Wraps::opendir(const char* pathname)
{
    return impl->opendir(pathname);
}

struct dirent* Wraps::readdir(DIR* dirp)
{
    return impl->readdir(dirp);
}

int Wraps::closedir(DIR* dirp)
{
    return impl->closedir(dirp);
}

// ---------------------------------------------------------------------------
// __wrap_* C functions – called by plugin shared libs built with --wrap
// ---------------------------------------------------------------------------

extern "C" {

extern int   __real_system(const char* command);
extern FILE* __real_popen(const char* command, const char* type);
extern int   __real_pclose(FILE* pipe);
extern FILE* __real_setmntent(const char* command, const char* type);
extern int   __real_unlink(const char* filePath);
extern int   __real_statvfs(const char* path, struct statvfs* buf);
extern int   __real_mkdir(const char* path, mode_t mode);
extern int   __real_stat(const char* path, struct stat* info);
extern int   __real_open(const char* pathname, int flags, mode_t mode);
extern int   __real_rmdir(const char* pathname);
extern int   __real_access(const char* pathname, int mode);
extern int   __real_chown(const char* path, uid_t owner, gid_t group);
extern int   __real_nftw(const char* dirpath,
                         int (*fn)(const char*, const struct stat*, int, struct FTW*),
                         int nopenfd, int flags);
extern DIR*          __real_opendir(const char* pathname);
extern struct dirent* __real_readdir(DIR* dirp);
extern int            __real_closedir(DIR* dirp);

int __wrap_system(const char* command)
{
    if (Wraps::impl)
        return Wraps::system(command);
    return __real_system(command);
}

FILE* __wrap_popen(const char* command, const char* type)
{
    if (Wraps::impl)
        return Wraps::popen(command, type);
    return __real_popen(command, type);
}

int __wrap_pclose(FILE* pipe)
{
    if (Wraps::impl)
        return Wraps::pclose(pipe);
    return __real_pclose(pipe);
}

FILE* __wrap_setmntent(const char* command, const char* type)
{
    if (Wraps::impl)
        return Wraps::setmntent(command, type);
    return __real_setmntent(command, type);
}

int __wrap_unlink(const char* filePath)
{
    if (Wraps::impl)
        return Wraps::unlink(filePath);
    return __real_unlink(filePath);
}

int __wrap_statvfs(const char* path, struct statvfs* buf)
{
    if (Wraps::impl)
        return Wraps::statvfs(path, buf);
    return __real_statvfs(path, buf);
}

int __wrap_mkdir(const char* path, mode_t mode)
{
    if (Wraps::impl)
        return Wraps::mkdir(path, mode);
    return __real_mkdir(path, mode);
}

int __wrap_stat(const char* path, struct stat* info)
{
    if (Wraps::impl)
        return Wraps::stat(path, info);
    return __real_stat(path, info);
}

int __wrap_open(const char* pathname, int flags, ...)
{
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    if (Wraps::impl)
        return Wraps::open(pathname, flags, mode);
    return __real_open(pathname, flags, mode);
}

int __wrap_rmdir(const char* pathname)
{
    if (Wraps::impl)
        return Wraps::rmdir(pathname);
    return __real_rmdir(pathname);
}

int __wrap_access(const char* pathname, int mode)
{
    if (Wraps::impl)
        return Wraps::access(pathname, mode);
    return __real_access(pathname, mode);
}

int __wrap_chown(const char* path, uid_t owner, gid_t group)
{
    if (Wraps::impl)
        return Wraps::chown(path, owner, group);
    return __real_chown(path, owner, group);
}

int __wrap_nftw(const char* dirpath,
                int (*fn)(const char*, const struct stat*, int, struct FTW*),
                int nopenfd, int flags)
{
    if (Wraps::impl)
        return Wraps::nftw(dirpath, fn, nopenfd, flags);
    return __real_nftw(dirpath, fn, nopenfd, flags);
}

DIR* __wrap_opendir(const char* pathname)
{
    if (Wraps::impl)
        return Wraps::opendir(pathname);
    return __real_opendir(pathname);
}

struct dirent* __wrap_readdir(DIR* dirp)
{
    if (Wraps::impl)
        return Wraps::readdir(dirp);
    return __real_readdir(dirp);
}

int __wrap_closedir(DIR* dirp)
{
    if (Wraps::impl)
        return Wraps::closedir(dirp);
    return __real_closedir(dirp);
}

} // extern "C"
