#pragma once

#include <string>

namespace ralf
{
    class RalfVendorLayer
    {
    public:
        explicit RalfVendorLayer(std::string layerPath);
        bool create(const std::string &configPath);

    private:
        bool createFile(const std::string &source, const std::string &destination);
        bool createSymlink(const std::string &target, const std::string &linkPath);
        bool createMountPoint(const std::string &source, const std::string &destination);
        bool resolveDestination(const std::string &destination, std::string &resolvedPath) const;
        bool prepareDestination(const std::string &destination, std::string &resolvedPath) const;

        std::string mLayerPath;
    };
} // namespace ralf