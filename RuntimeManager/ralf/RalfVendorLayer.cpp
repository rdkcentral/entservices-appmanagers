#include "../Module.h"
#include <UtilsLogging.h>

#include "RalfVendorLayer.h"
#include "RalfSupport.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace ralf
{
    RalfVendorLayer::RalfVendorLayer(std::string layerPath)
        : mLayerPath(std::move(layerPath))
    {
    }

    bool RalfVendorLayer::create(const std::string &configPath)
    {
        Json::Value config;
        if (!JsonFromFile(configPath, config) || !config.isMember("vendorGpuSupport") ||
            !config["vendorGpuSupport"].isObject())
        {
            LOGERR("Invalid vendor GPU config: %s", configPath.c_str());
            return false;
        }

        const Json::Value &files = config["vendorGpuSupport"]["files"];
        if (!files.isArray())
        {
            LOGERR("Vendor GPU config files is not an array: %s", configPath.c_str());
            return false;
        }

        if (checkIfPathExists(mLayerPath) && !removeDirectoryRecursively(mLayerPath))
        {
            LOGERR("Failed to clear vendor GPU layer %s", mLayerPath.c_str());
            return false;
        }
        if (!create_directories(mLayerPath))
        {
            LOGERR("Failed to create vendor GPU layer %s", mLayerPath.c_str());
            return false;
        }

        for (const Json::Value &entry : files)
        {
            if (!entry.isObject() || !entry["type"].isString())
            {
                LOGERR("Invalid entry in vendor GPU config files: %s", configPath.c_str());
                removeDirectoryRecursively(mLayerPath);
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
                removeDirectoryRecursively(mLayerPath);
                return false;
            }
        }

        return true;
    }

    bool RalfVendorLayer::prepareDestination(const std::string &destination, std::string &resolvedPath) const
    {
        if (!resolveDestination(destination, resolvedPath))
        {
            LOGERR("Invalid vendor GPU destination path: %s", destination.c_str());
            return false;
        }

        const size_t separator = resolvedPath.find_last_of('/');
        const std::string parentPath = (std::string::npos == separator) ? mLayerPath : resolvedPath.substr(0, separator);
        if (!create_directories(parentPath))
        {
            LOGERR("Failed to create vendor GPU destination parent %s", parentPath.c_str());
            return false;
        }
        return true;
    }

    bool RalfVendorLayer::resolveDestination(const std::string &destination, std::string &resolvedPath) const
    {
        if (destination.empty())
        {
            return false;
        }

        size_t componentStart = 0;
        while (componentStart < destination.size())
        {
            while (componentStart < destination.size() && '/' == destination[componentStart])
            {
                ++componentStart;
            }
            const size_t componentEnd = destination.find('/', componentStart);
            const std::string component = destination.substr(componentStart, componentEnd - componentStart);
            if (".." == component)
            {
                return false;
            }
            if (std::string::npos == componentEnd)
            {
                break;
            }
            componentStart = componentEnd + 1;
        }

        const std::string relativePath = ('/' == destination[0]) ? destination.substr(1) : destination;
        resolvedPath = mLayerPath + "/" + relativePath;
        return true;
    }

    bool RalfVendorLayer::createFile(const std::string &source, const std::string &destination)
    {
        std::string target;
        if (!prepareDestination(destination, target))
        {
            return false;
        }

        struct stat sourceStat;
        if (0 != stat(source.c_str(), &sourceStat) || !S_ISREG(sourceStat.st_mode))
        {
            LOGERR("Vendor GPU file source is missing or not regular: %s", source.c_str());
            return false;
        }

        std::ifstream input(source.c_str(), std::ios::binary);
        std::ofstream output(target.c_str(), std::ios::binary | std::ios::trunc);
        if (!input.is_open() || !output.is_open())
        {
            LOGERR("Failed to open vendor GPU file source or destination: %s -> %s",
                   source.c_str(), target.c_str());
            return false;
        }
        output << input.rdbuf();
        if (!input.good() && !input.eof())
        {
            LOGERR("Failed while copying vendor GPU file: %s", source.c_str());
            return false;
        }
        output.close();
        if (!output.good())
        {
            LOGERR("Failed while writing vendor GPU file: %s", target.c_str());
            return false;
        }
        if (0 != chmod(target.c_str(), sourceStat.st_mode & 07777))
        {
            LOGERR("Failed to preserve vendor GPU file permissions: %s", target.c_str());
            return false;
        }
        return true;
    }

    bool RalfVendorLayer::createSymlink(const std::string &target, const std::string &linkPath)
    {
        std::string link;
        if (!prepareDestination(linkPath, link))
        {
            return false;
        }

        if (0 != symlink(target.c_str(), link.c_str()))
        {
            LOGERR("Failed to create vendor GPU symlink %s -> %s: %s",
                   link.c_str(), target.c_str(), strerror(errno));
            return false;
        }
        return true;
    }

    bool RalfVendorLayer::createMountPoint(const std::string &source, const std::string &destination)
    {
        std::string target;
        if (!prepareDestination(destination, target))
        {
            return false;
        }

        struct stat sourceStat;
        if (0 == stat(source.c_str(), &sourceStat) && S_ISDIR(sourceStat.st_mode))
        {
            if (!create_directories(target))
            {
                return false;
            }
        }
        else
        {
            const int fd = open(target.c_str(), O_CLOEXEC | O_WRONLY | O_CREAT, 0644);
            if (fd < 0)
            {
                LOGERR("Failed to create vendor GPU bind target %s: %s", target.c_str(), strerror(errno));
                return false;
            }
            if (close(fd) != 0)
            {
                LOGERR("Failed to close vendor GPU bind target %s: %s", target.c_str(), strerror(errno));
                return false;
            }
        }

        return true;
    }
} // namespace ralf