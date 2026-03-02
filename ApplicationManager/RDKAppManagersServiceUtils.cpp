#include "RDKAppManagersServiceUtils.h"

#include <json/json.h>
#include <sstream>
#include <cstdlib>
#include <memory>
#include <fstream>

namespace WPEFramework {
namespace Plugin {

std::string RDKAppManagersServiceUtils::UrlDecode(const std::string& value)
{
    std::string output;
    output.reserve(value.size());

    for (size_t index = 0; index < value.size(); ++index) {
        if (value[index] == '+') {
            output += ' ';
        } else if (value[index] == '%' && (index + 2) < value.size()) {
            const std::string hex = value.substr(index + 1, 2);
            char* end = nullptr;
            const long decoded = std::strtol(hex.c_str(), &end, 16);
            if (end != nullptr && *end == '\0') {
                output += static_cast<char>(decoded);
                index += 2;
            } else {
                output += value[index];
            }
        } else {
            output += value[index];
        }
    }

    return output;
}

std::map<std::string, std::string> RDKAppManagersServiceUtils::ParseQueryParams(const std::string& query)
{
    std::map<std::string, std::string> parsed;

    if (query.empty()) {
        return parsed;
    }

    const size_t firstNonSpace = query.find_first_not_of(" \t\r\n");
    if (firstNonSpace != std::string::npos && query[firstNonSpace] == '{') {
        Json::CharReaderBuilder builder;
        builder["collectComments"] = false;

        Json::Value root;
        std::string parseErrors;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        const bool parsedJson = reader->parse(query.data(), query.data() + query.size(), &root, &parseErrors);
        if (parsedJson && root.isObject()) {
            Json::ValueConstIterator it = root.begin();
            while (it != root.end()) {
                const Json::Value& value = *it;
                if (value.isString()) {
                    parsed[it.name()] = value.asString();
                } else if (value.isBool()) {
                    parsed[it.name()] = value.asBool() ? "true" : "false";
                } else if (value.isInt64()) {
                    parsed[it.name()] = std::to_string(value.asInt64());
                } else if (value.isUInt64()) {
                    parsed[it.name()] = std::to_string(value.asUInt64());
                } else if (value.isDouble()) {
                    parsed[it.name()] = std::to_string(value.asDouble());
                }
                ++it;
            }
            return parsed;
        }
    }

    std::stringstream stream(query);
    std::string token;

    while (std::getline(stream, token, '&')) {
        if (token.empty()) {
            continue;
        }

        const size_t equals = token.find('=');
        if (equals == std::string::npos) {
            parsed[UrlDecode(token)] = "";
            continue;
        }

        const std::string key = UrlDecode(token.substr(0, equals));
        const std::string decodedValue = UrlDecode(token.substr(equals + 1));
        parsed[key] = decodedValue;
    }

    return parsed;
}

bool RDKAppManagersServiceUtils::getParamValue(const std::map<std::string, std::string>& params, const std::string& key, std::string& value)
{
    const auto it = params.find(key);
    if (it != params.end()) {
        value = it->second;
        return true;
    }

    return false;
}

bool RDKAppManagersServiceUtils::getParamValue(const std::string& query, const std::string& key, std::string& value)
{
    const std::map<std::string, std::string> params = ParseQueryParams(query);
    return getParamValue(params, key, value);
}

std::string RDKAppManagersServiceUtils::GetJsonStringField(const std::string& json, const std::string& key)
{
    if (json.empty()) {
        return "";
    }

    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;

    Json::Value root;
    std::string parseErrors;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    const bool parsed = reader->parse(json.data(), json.data() + json.size(), &root, &parseErrors);
    if (!parsed || !root.isObject() || !root.isMember(key)) {
        return "";
    }

    const Json::Value& value = root[key];
    if (value.isString()) {
        return value.asString();
    }

    return "";
}

std::string RDKAppManagersServiceUtils::BuildErrorResponse(const std::string& message)
{
    Json::Value errorResponse;
    errorResponse["success"] = false;
    errorResponse["error"] = message;

    Json::StreamWriterBuilder writerBuilder;
    writerBuilder["indentation"] = "";
    return Json::writeString(writerBuilder, errorResponse);
}

std::pair<std::string, std::string> RDKAppManagersServiceUtils::NormalizeUrlAndExtractQuery(const std::string& url, const std::string& queryParams)
{
    std::string normalizedUrl = url;
    std::string queryFromUrl;

    const size_t schemePos = normalizedUrl.find("://");
    if (schemePos != std::string::npos) {
        const size_t pathPos = normalizedUrl.find('/', schemePos + 3);
        normalizedUrl = (pathPos == std::string::npos) ? "/" : normalizedUrl.substr(pathPos);
    }

    const size_t queryPos = normalizedUrl.find('?');
    if (queryPos != std::string::npos) {
        queryFromUrl = normalizedUrl.substr(queryPos + 1);
        normalizedUrl = normalizedUrl.substr(0, queryPos);
    }

    if ((normalizedUrl.length() > 1) && (normalizedUrl.back() == '/')) {
        normalizedUrl.pop_back();
    }

    return { normalizedUrl, queryParams.empty() ? queryFromUrl : queryParams };
}

std::map<std::string, RDKAppManagersServiceUtils::RouteEntry> RDKAppManagersServiceUtils::LoadRequestMap()
{
    std::map<std::string, RouteEntry> routes;

    std::string configPath = "/etc/rdkappmanagers_plugin.json";
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        configPath = "rdkappmanagers_plugin.json";
        configFile.open(configPath);
    }
    if (!configFile.is_open()) {
        configPath = "/etc/rdkappmanagers.json";
        configFile.open(configPath);
    }
    if (!configFile.is_open()) {
        configPath = "rdkappmanagers.json";
        configFile.open(configPath);
    }

    if (!configFile.is_open()) {
        SYSLOG(Logging::Error, (_T("Request map config not found")));
        return routes;
    }

    Json::CharReaderBuilder reader;
    Json::Value root;
    std::string parseErrors;
    if (!Json::parseFromStream(reader, configFile, &root, &parseErrors) || !root.isArray()) {
        SYSLOG(Logging::Error, (_T("Failed to parse request map config '%s': %s"), configPath.c_str(), parseErrors.c_str()));
        return routes;
    }

    for (Json::ArrayIndex index = 0; index < root.size(); ++index) {
        const Json::Value& entry = root[index];
        if (!entry.isArray() || entry.size() < 2 || !entry[0].isString()) {
            continue;
        }

        RouteEntry route;
        const Json::Value& methodNode = entry[1];
        if (methodNode.isArray()) {
            for (Json::ArrayIndex methodIndex = 0; methodIndex < methodNode.size(); ++methodIndex) {
                if (methodNode[methodIndex].isString()) {
                    route.methods.push_back(methodNode[methodIndex].asString());
                }
            }
        } else if (methodNode.isString()) {
            route.methods.push_back(methodNode.asString());
        }

        if (entry.size() > 2 && entry[2].isObject()) {
            const Json::Value& paramsNode = entry[2];
            Json::ValueConstIterator it = paramsNode.begin();
            while (it != paramsNode.end()) {
                if ((*it).isString()) {
                    route.params[it.name()] = (*it).asString();
                }
                ++it;
            }
        }

        if (!route.methods.empty()) {
            routes[entry[0].asString()] = route;
        }
    }

    SYSLOG(Logging::Startup, (_T("Loaded request map from %s, total routes=%zu"), configPath.c_str(), routes.size()));
    return routes;
}

RDKAppManagersServiceUtils::RequestContext RDKAppManagersServiceUtils::BuildRequestContext(const std::string& normalizedUrl, const std::string& effectiveQueryParams, const std::string& body)
{
    RequestContext context;
    context.normalizedUrl = normalizedUrl;

    const std::map<std::string, std::string> params = ParseQueryParams(effectiveQueryParams);

    if (!getParamValue(params, "appId", context.appId)) {
        getParamValue(params, "appid", context.appId);
    }
    if (context.appId.empty()) {
        context.appId = GetJsonStringField(body, "appId");
    }
    if (context.appId.empty()) {
        context.appId = GetJsonStringField(body, "appid");
    }

    getParamValue(params, "client", context.client);
    if (context.client.empty()) {
        context.client = context.appId;
    }
    if (context.client.empty()) {
        context.client = GetJsonStringField(body, "client");
    }

    getParamValue(params, "token", context.token);
    if (context.token.empty()) {
        context.token = GetJsonStringField(body, "token");
    }

    getParamValue(params, "mode", context.mode);
    if (context.mode.empty()) {
        context.mode = GetJsonStringField(body, "mode");
    }

    getParamValue(params, "intent", context.intent);
    if (context.intent.empty()) {
        context.intent = GetJsonStringField(body, "intent");
    }

    getParamValue(params, "launchArgs", context.launchArgs);
    if (context.launchArgs.empty()) {
        context.launchArgs = GetJsonStringField(body, "launchArgs");
    }

    context.runtimeParams = {
        { "appId", context.appId },
        { "client", context.client },
        { "token", context.token },
        { "mode", context.mode },
        { "intent", context.intent },
        { "launchArgs", context.launchArgs }
    };

    return context;
}

void RDKAppManagersServiceUtils::EnsureErrorResponse(const Core::hresult status, const std::string& normalizedUrl, std::string& responseBody)
{
    if (status != Core::ERROR_NONE && responseBody.empty()) {
        Json::Value errorResponse;
        errorResponse["success"] = false;
        errorResponse["url"] = normalizedUrl;
        errorResponse["status"] = static_cast<Json::Value::Int>(status);

        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        responseBody = Json::writeString(writerBuilder, errorResponse);
    }
}

} // namespace Plugin
} // namespace WPEFramework
