#pragma once

#include <json/json.h>
#include <string>

namespace ralf
{
    class RalfVendorLayer
    {
    public:
        explicit RalfVendorLayer(std::string layerPath);
        bool create(const Json::Value &config);

        /// Builds the layer once per process and reuses it for every bundle (AI1 OCIVendorLayer semantics).
        static bool createShared(const std::string &layerPath, const Json::Value &config);

    private:
        bool createFile(const std::string &source, const std::string &destination);
        bool createSymlink(const std::string &target, const std::string &linkPath);
        bool createMountPoint(const std::string &source, const std::string &destination);
        bool resolveDestination(const std::string &destination, std::string &resolvedPath) const;
        bool prepareDestination(const std::string &destination, std::string &resolvedPath) const;

        std::string mLayerPath;
    };
} // namespace ralf