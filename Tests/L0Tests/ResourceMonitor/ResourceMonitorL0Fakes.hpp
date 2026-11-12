#pragma once

#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "PackageLifecycleConfigProvider.h"

namespace L0Test {

// Builds a Package Manager runtime configuration payload. An empty lifecycle omits applicationLifecycle.
inline std::string RuntimePayload(const std::string& ralfPkgPath, const std::string& lifecycle)
{
    std::string payload = "{\"appPath\":\"/apps/test\",\"command\":\"run\"";
    if (!ralfPkgPath.empty()) {
        payload += ",\"ralfPkgPath\":\"" + ralfPkgPath + "\"";
    }
    if (!lifecycle.empty()) {
        payload += ",\"applicationLifecycle\":" + lifecycle;
    }
    return payload + "}";
}

inline std::string RalfPayload(const std::string& lifecycle)
{
    return RuntimePayload("/tmp/app_1.0.0_metadata.json", lifecycle);
}

// Thread-safe fake Package Manager. Configure it before use; calls only read the configuration.
class FakePackageLifecycleSource final : public WPEFramework::Plugin::IPackageLifecycleSource {
public:
    struct Version {
        bool locked = false;
        std::string payload;
        WPEFramework::Core::hresult result = WPEFramework::Core::ERROR_NONE;
        uint32_t delayMs = 0;
    };

    FakePackageLifecycleSource()
        : listResult(WPEFramework::Core::ERROR_NONE)
        , available(true)
        , listCalls(0)
        , lockedInfoCalls(0)
    {
    }

    void Add(const std::string& packageId, const std::string& version, const Version& data)
    {
        order.emplace_back(packageId, version);
        versions[std::make_pair(packageId, version)] = data;
    }

    WPEFramework::Core::hresult ListPackages(std::vector<PackageEntry>& packages) override
    {
        ++listCalls;
        packages.clear();
        if (!available.load()) {
            return WPEFramework::Core::ERROR_UNAVAILABLE;
        }
        if (listResult != WPEFramework::Core::ERROR_NONE) {
            return listResult;
        }
        for (const auto& key : order) {
            packages.push_back(PackageEntry { key.first, key.second });
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult GetLockedInfo(const std::string& packageId, const std::string& version,
        bool& locked, std::string& runtimeConfigPayload) override
    {
        ++lockedInfoCalls;
        locked = false;
        runtimeConfigPayload.clear();
        if (!available.load()) {
            return WPEFramework::Core::ERROR_UNAVAILABLE;
        }
        const auto it = versions.find(std::make_pair(packageId, version));
        if (it == versions.end()) {
            return WPEFramework::Core::ERROR_INVALID_PARAMETER;
        }
        if (it->second.delayMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(it->second.delayMs));
        }
        if (it->second.result != WPEFramework::Core::ERROR_NONE) {
            return it->second.result;
        }
        locked = it->second.locked;
        runtimeConfigPayload = it->second.payload;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult listResult;
    std::atomic<bool> available;
    std::atomic<uint32_t> listCalls;
    std::atomic<uint32_t> lockedInfoCalls;

private:
    std::vector<std::pair<std::string, std::string>> order;
    std::map<std::pair<std::string, std::string>, Version> versions;
};

} // namespace L0Test
