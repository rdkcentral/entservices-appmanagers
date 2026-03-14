#pragma once

#include "Module.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace WPEFramework {
namespace Plugin {

class RDKAppManagersServiceUtils {
public:
    struct RouteEntry {
        std::vector<std::string> methods;
        std::map<std::string, std::string> params;
    };

    struct RequestContext {
        std::string normalizedUrl;
        std::string appId;
        std::string client;
        std::string token;
        std::string mode;
        std::string intent;
        std::string launchArgs;
        std::map<std::string, std::string> runtimeParams;
    };

    static std::string UrlDecode(const std::string& value);
    static std::map<std::string, std::string> ParseQueryParams(const std::string& query);
    static bool getParamValue(const std::map<std::string, std::string>& params, const std::string& key, std::string& value);
    static bool getParamValue(const std::string& query, const std::string& key, std::string& value);
    static std::string GetJsonStringField(const std::string& json, const std::string& key);
    static std::string BuildErrorResponse(const std::string& message);
    static std::pair<std::string, std::string> NormalizeUrlAndExtractQuery(const std::string& url, const std::string& queryParams);
    static std::map<std::string, RouteEntry> LoadRequestMap();
    static RequestContext BuildRequestContext(const std::string& normalizedUrl, const std::string& effectiveQueryParams, const std::string& body);
    static void EnsureErrorResponse(const Core::hresult status, const std::string& normalizedUrl, std::string& responseBody);
};

} // namespace Plugin
} // namespace WPEFramework
