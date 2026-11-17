/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2026 RDK Management
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

#include <json/json.h>
#include <string>

namespace Utils {
namespace LaunchArgs {
namespace detail {

inline int hexToInt(char c)
{
    if ((c >= '0') && (c <= '9')) {
        return c - '0';
    }
    if ((c >= 'a') && (c <= 'f')) {
        return c - 'a' + 10;
    }
    if ((c >= 'A') && (c <= 'F')) {
        return c - 'A' + 10;
    }
    return -1;
}

} // namespace detail

inline bool decodePercentEncoded(const std::string& in, std::string& out)
{
    out.clear();
    out.reserve(in.size());

    bool replaced = false;
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%' && (i + 2) < in.size()) {
            const int hi = detail::hexToInt(in[i + 1]);
            const int lo = detail::hexToInt(in[i + 2]);
            if ((hi >= 0) && (lo >= 0)) {
                out.push_back(static_cast<char>((hi << 4) | lo));
                i += 2;
                replaced = true;
                continue;
            }
        }

        if ('+' == in[i]) {
            out.push_back(' ');
            replaced = true;
        } else {
            out.push_back(in[i]);
        }
    }

    return replaced;
}

inline bool isJsonObject(const std::string& payload)
{
    Json::Reader reader;
    Json::Value value;
    return (reader.parse(payload, value) && value.isObject());
}

inline std::string normalizeLaunchArgs(const std::string& launchArgs)
{
    if (launchArgs.empty() || isJsonObject(launchArgs)) {
        return launchArgs;
    }

    std::string decoded;
    if (!decodePercentEncoded(launchArgs, decoded)) {
        return launchArgs;
    }

    if (isJsonObject(decoded)) {
        return decoded;
    }

    return launchArgs;
}

} // namespace LaunchArgs
} // namespace Utils
