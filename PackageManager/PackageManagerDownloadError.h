/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
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
*/

#pragma once

#include "HttpClient.h"

namespace WPEFramework {
namespace Plugin {
namespace PackageManagerTelemetry {

inline const char* downloadErrorTelemetry(const std::string& url,
    HttpClient::Status status,
    CURLcode curlCode,
    long httpCode)
{
    if (url.empty() || (CURLE_URL_MALFORMAT == curlCode) || (CURLE_UNSUPPORTED_PROTOCOL == curlCode)) {
        return "URL_INVALID";
    }
    if (HttpClient::Status::DiskError == status) {
        return "DISK_WRITE_ERROR";
    }

    switch (httpCode) {
        case 401:
        case 403:
            return "NETWORK_AUTH_ERROR";
        case 404:
            return "NETWORK_NOT_FOUND";
        case 429:
            return "NETWORK_RATE_LIMITED";
        default:
            if ((httpCode >= 500) && (httpCode <= 599)) {
                return "NETWORK_SERVER_ERROR";
            }
            break;
    }

    if (CURLE_LOGIN_DENIED == curlCode) {
        return "NETWORK_AUTH_ERROR";
    }

    switch (curlCode) {
        case CURLE_COULDNT_RESOLVE_HOST:
            return "NETWORK_DNS_FAILURE";
        case CURLE_OPERATION_TIMEDOUT:
            return "NETWORK_TIMEOUT";
        case CURLE_COULDNT_CONNECT:
            return "NETWORK_CONNECTION_REFUSED";
        case CURLE_SSL_CONNECT_ERROR:
        case CURLE_PEER_FAILED_VERIFICATION:
        case CURLE_SSL_CERTPROBLEM:
            return "NETWORK_SSL_ERROR";
        case CURLE_PARTIAL_FILE:
        case CURLE_BAD_CONTENT_ENCODING:
            return "NETWORK_CONTENT_ERROR";
        default:
            return "NETWORK_ERROR";
    }
}

} // namespace PackageManagerTelemetry
} // namespace Plugin
} // namespace WPEFramework
