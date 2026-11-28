#include "../Module.h"
#include <UtilsLogging.h>

#include "RalfVendorLayer.h"
#include "RalfSupport.h"

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <mutex>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace ralf
{
    RalfVendorLayer::RalfVendorLayer(std::string layerPath)
        : mLayerPath(std::move(layerPath))
        , mLayerRootFd(-1)
    {
    }

    bool RalfVendorLayer::createShared(const std::string &layerPath, const Json::Value &config)
    {
        static std::mutex creationLock;
        static bool created = false;

        std::lock_guard<std::mutex> guard(creationLock);
        if (!created)
        {
            RalfVendorLayer layer(layerPath);
            created = layer.create(config);
        }
        return created;
    }

    bool RalfVendorLayer::create(const Json::Value &config)
    {
        if (!config.isMember("vendorGpuSupport") ||
            !config["vendorGpuSupport"].isObject())
        {
            LOGERR("Invalid vendor GPU config node");
            return false;
        }

        const Json::Value &files = config["vendorGpuSupport"]["files"];
        if (!files.isArray())
        {
            LOGERR("Vendor GPU config files is not an array");
            return false;
        }

        if (!openDirectoryPath(mLayerPath, true, mLayerRootFd))
        {
            LOGERR("Failed to securely open vendor GPU layer %s: %s", mLayerPath.c_str(), strerror(errno));
            return false;
        }

        bool status = clearDirectory(mLayerRootFd);
        if (status)
        {
            status = createEntries(mLayerRootFd, files);
        }

        if (!status)
        {
            LOGERR("Failed to create vendor GPU layer entries in %s", mLayerPath.c_str());
            clearDirectory(mLayerRootFd);
        }
        ::close(mLayerRootFd);
        mLayerRootFd = -1;
        return status;
    }

    bool RalfVendorLayer::openDirectoryPath(const std::string &path, bool create, int &directoryFd) const
    {
        if (path.empty() || std::string::npos != path.find('\0'))
        {
            return false;
        }

        int currentFd = ::open('/' == path[0] ? "/" : ".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (currentFd < 0)
        {
            return false;
        }

        size_t componentStart = 0;
        size_t componentCount = 0;
        while (componentStart < path.size())
        {
            while (componentStart < path.size() && '/' == path[componentStart])
            {
                ++componentStart;
            }
            if (componentStart == path.size())
            {
                break;
            }

            const size_t componentEnd = path.find('/', componentStart);
            const std::string component = path.substr(componentStart, componentEnd - componentStart);
            if ("." == component)
            {
                componentStart = (std::string::npos == componentEnd) ? path.size() : componentEnd + 1;
                continue;
            }
            if (".." == component)
            {
                ::close(currentFd);
                return false;
            }

            int nextFd = ::openat(currentFd, component.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
            if (nextFd < 0 && create && ENOENT == errno)
            {
                if (0 != ::mkdirat(currentFd, component.c_str(), 0755) && EEXIST != errno)
                {
                    ::close(currentFd);
                    return false;
                }
                nextFd = ::openat(currentFd, component.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
            }
            if (nextFd < 0)
            {
                ::close(currentFd);
                return false;
            }

            ::close(currentFd);
            currentFd = nextFd;
            ++componentCount;
            componentStart = (std::string::npos == componentEnd) ? path.size() : componentEnd + 1;
        }

        if (0 == componentCount)
        {
            ::close(currentFd);
            return false;
        }
        directoryFd = currentFd;
        return true;
    }

    bool RalfVendorLayer::clearDirectory(int directoryFd) const
    {
        const int scanFd = ::openat(directoryFd, ".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        DIR *directory = (0 <= scanFd) ? ::fdopendir(scanFd) : nullptr;
        if (nullptr == directory)
        {
            if (0 <= scanFd)
            {
                ::close(scanFd);
            }
            return false;
        }

        bool status = true;
        struct dirent *entry = nullptr;
        while (nullptr != (entry = ::readdir(directory)))
        {
            if (0 == strcmp(entry->d_name, ".") || 0 == strcmp(entry->d_name, ".."))
            {
                continue;
            }

            struct stat entryStat;
            if (0 != ::fstatat(directoryFd, entry->d_name, &entryStat, AT_SYMLINK_NOFOLLOW))
            {
                status = false;
                continue;
            }

            if (S_ISDIR(entryStat.st_mode))
            {
                const int childFd = ::openat(directoryFd, entry->d_name,
                                             O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
                if (childFd < 0)
                {
                    status = false;
                    continue;
                }
                if (!clearDirectory(childFd))
                {
                    status = false;
                }
                ::close(childFd);
                if (0 != ::unlinkat(directoryFd, entry->d_name, AT_REMOVEDIR))
                {
                    status = false;
                }
            }
            else if (0 != ::unlinkat(directoryFd, entry->d_name, 0))
            {
                status = false;
            }
        }

        ::closedir(directory);
        return status;
    }

    bool RalfVendorLayer::createEntries(int layerFd, const Json::Value &files)
    {
        if (layerFd < 0 || layerFd != mLayerRootFd)
        {
            return false;
        }
        for (const Json::Value &entry : files)
        {
            if (!entry.isObject() || !entry["type"].isString())
            {
                LOGERR("Invalid entry in vendor GPU config files");
                return false;
            }

            const std::string type = entry["type"].asString();
            bool status = false;
            if ("file" == type && entry["source"].isString() && entry["destination"].isString())
            {
                status = createFile(entry["source"].asString(), entry["destination"].asString());
            }
            else if ("symlink" == type && entry["target"].isString() && entry["linkPath"].isString())
            {
                status = createSymlink(entry["target"].asString(), entry["linkPath"].asString());
            }
            else if (("bind" == type || "socket" == type) && entry["source"].isString() &&
                     entry["destination"].isString())
            {
                status = createMountPoint(entry["source"].asString(), entry["destination"].asString());
            }
            else
            {
                LOGERR("Unsupported or incomplete vendor GPU config entry type: %s", type.c_str());
            }

            if (!status)
            {
                return false;
            }
        }
        return true;
    }

    bool RalfVendorLayer::prepareDestination(const std::string &destination, int &parentFd, std::string &name) const
    {
        std::string relativePath;
        if (!resolveDestination(destination, relativePath))
        {
            LOGERR("Invalid vendor GPU destination path: %s", destination.c_str());
            return false;
        }

        int currentFd = ::dup(mLayerRootFd);
        if (currentFd < 0)
        {
            return false;
        }

        size_t componentStart = 0;
        while (true)
        {
            const size_t componentEnd = relativePath.find('/', componentStart);
            const std::string component = relativePath.substr(componentStart, componentEnd - componentStart);
            if (std::string::npos == componentEnd)
            {
                name = component;
                parentFd = currentFd;
                return true;
            }

            int nextFd = ::openat(currentFd, component.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
            if (nextFd < 0 && ENOENT == errno)
            {
                if (0 != ::mkdirat(currentFd, component.c_str(), 0755) && EEXIST != errno)
                {
                    ::close(currentFd);
                    return false;
                }
                nextFd = ::openat(currentFd, component.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
            }
            if (nextFd < 0)
            {
                ::close(currentFd);
                return false;
            }
            ::close(currentFd);
            currentFd = nextFd;
            componentStart = componentEnd + 1;
        }
    }

    bool RalfVendorLayer::resolveDestination(const std::string &destination, std::string &relativePath) const
    {
        if (destination.empty() || std::string::npos != destination.find('\0'))
        {
            return false;
        }

        relativePath.clear();
        size_t componentStart = 0;
        while (componentStart < destination.size())
        {
            while (componentStart < destination.size() && '/' == destination[componentStart])
            {
                ++componentStart;
            }
            if (componentStart == destination.size())
            {
                break;
            }

            const size_t componentEnd = destination.find('/', componentStart);
            const std::string component = destination.substr(componentStart, componentEnd - componentStart);
            if ("." == component || ".." == component)
            {
                return false;
            }
            if (!relativePath.empty())
            {
                relativePath += '/';
            }
            relativePath += component;
            componentStart = (std::string::npos == componentEnd) ? destination.size() : componentEnd + 1;
        }
        return !relativePath.empty();
    }

    bool RalfVendorLayer::createFile(const std::string &source, const std::string &destination)
    {
        const int sourceFd = ::open(source.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
        struct stat sourceStat;
        if (sourceFd < 0 || 0 != ::fstat(sourceFd, &sourceStat) || !S_ISREG(sourceStat.st_mode))
        {
            if (0 <= sourceFd)
            {
                ::close(sourceFd);
            }
            LOGERR("Vendor GPU file source is missing, symlinked, or not regular: %s", source.c_str());
            return false;
        }

        int parentFd = -1;
        std::string name;
        if (!prepareDestination(destination, parentFd, name))
        {
            ::close(sourceFd);
            return false;
        }

        const mode_t mode = sourceStat.st_mode & 07777;
        const int targetFd = ::openat(parentFd, name.c_str(),
                                      O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK, mode);
        if (targetFd < 0)
        {
            LOGERR("Failed to open vendor GPU destination %s: %s", destination.c_str(), strerror(errno));
            ::close(parentFd);
            ::close(sourceFd);
            return false;
        }
        struct stat targetStat;
        if (0 != ::fstat(targetFd, &targetStat) || !S_ISREG(targetStat.st_mode))
        {
            LOGERR("Vendor GPU destination is not a regular file: %s", destination.c_str());
            ::close(targetFd);
            ::close(parentFd);
            ::close(sourceFd);
            return false;
        }

        bool status = 0 == ::fchmod(targetFd, mode);
        char buffer[8192];
        while (status)
        {
            const ssize_t count = ::read(sourceFd, buffer, sizeof(buffer));
            if (0 == count)
            {
                break;
            }
            if (count < 0)
            {
                if (EINTR == errno)
                {
                    continue;
                }
                status = false;
                break;
            }

            ssize_t written = 0;
            while (written < count)
            {
                const ssize_t bytes = ::write(targetFd, buffer + written, static_cast<size_t>(count - written));
                if (bytes < 0 && EINTR == errno)
                {
                    continue;
                }
                if (bytes <= 0)
                {
                    status = false;
                    break;
                }
                written += bytes;
            }
        }

        if (0 != ::close(targetFd))
        {
            status = false;
        }
        if (0 != ::close(sourceFd))
        {
            status = false;
        }
        ::close(parentFd);
        if (!status)
        {
            LOGERR("Failed while copying vendor GPU file: %s -> %s", source.c_str(), destination.c_str());
        }
        return status;
    }

    bool RalfVendorLayer::createSymlink(const std::string &target, const std::string &linkPath)
    {
        int parentFd = -1;
        std::string name;
        if (!prepareDestination(linkPath, parentFd, name))
        {
            return false;
        }

        const int result = ::symlinkat(target.c_str(), parentFd, name.c_str());
        const int savedErrno = errno;
        ::close(parentFd);
        if (0 != result)
        {
            LOGERR("Failed to create vendor GPU symlink %s -> %s: %s",
                   linkPath.c_str(), target.c_str(), strerror(savedErrno));
            return false;
        }
        return true;
    }

    bool RalfVendorLayer::createMountPoint(const std::string &source, const std::string &destination)
    {
        int parentFd = -1;
        std::string name;
        if (!prepareDestination(destination, parentFd, name))
        {
            return false;
        }

        struct stat sourceStat;
        if (0 == stat(source.c_str(), &sourceStat) && S_ISDIR(sourceStat.st_mode))
        {
            if (0 != ::mkdirat(parentFd, name.c_str(), 0755) && EEXIST != errno)
            {
                ::close(parentFd);
                return false;
            }
            const int targetFd = ::openat(parentFd, name.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
            if (targetFd < 0)
            {
                ::close(parentFd);
                return false;
            }
            ::close(targetFd);
        }
        else
        {
            const int fd = ::openat(parentFd, name.c_str(),
                                    O_CLOEXEC | O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_NONBLOCK, 0644);
            if (fd < 0)
            {
                LOGERR("Failed to create vendor GPU bind target %s: %s", destination.c_str(), strerror(errno));
                ::close(parentFd);
                return false;
            }
            if (::close(fd) != 0)
            {
                LOGERR("Failed to close vendor GPU bind target %s: %s", destination.c_str(), strerror(errno));
                ::close(parentFd);
                return false;
            }
        }

        ::close(parentFd);
        return true;
    }
} // namespace ralf