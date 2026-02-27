#include "RDKAppManagersLiteImplementation.h"

#include <fnmatch.h>
#include <cctype>
#include <iomanip>
#include <regex>
#include <sstream>

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

// Register the implementation class with Thunder's object factory so that the
// plugin host can instantiate it out-of-process (or in-process when mode=Local).
SERVICE_REGISTRATION(RDKAppManagersLiteImplementation, 1, 0, 0);

namespace WPEFramework {
namespace Plugin {

namespace {

std::string EscapeJsonString(const std::string& input) {
    std::ostringstream escaped;
    escaped << std::hex;

    for (const unsigned char ch : input) {
        switch (ch) {
        case '"': escaped << "\\\""; break;
        case '\\': escaped << "\\\\"; break;
        case '\b': escaped << "\\b"; break;
        case '\f': escaped << "\\f"; break;
        case '\n': escaped << "\\n"; break;
        case '\r': escaped << "\\r"; break;
        case '\t': escaped << "\\t"; break;
        default:
            if (ch < 0x20) {
                escaped << "\\u"
                        << std::setw(4)
                        << std::setfill('0')
                        << static_cast<int>(ch);
            } else {
                escaped << static_cast<char>(ch);
            }
            break;
        }
    }

    return escaped.str();
}

std::string TrimCopy(const std::string& value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        ++start;
    }

    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }

    return value.substr(start, end - start);
}

bool HasMeaningfulInput(const std::string& input) {
    const std::string trimmed = TrimCopy(input);
    return !trimmed.empty() && trimmed != "{}";
}

std::string BuildStubResponse(const std::string& fields,
                              const std::string* queryParams = nullptr,
                              const std::string* body = nullptr) {
    std::ostringstream json;
    json << "{" << fields;

    bool hasEcho = false;
    if ((queryParams != nullptr) && HasMeaningfulInput(*queryParams)) {
        json << ",\"echo\":{\"queryParams\":\"" << EscapeJsonString(*queryParams) << "\"";
        hasEcho = true;
    }

    if ((body != nullptr) && HasMeaningfulInput(*body)) {
        if (!hasEcho) {
            json << ",\"echo\":{";
            hasEcho = true;
        } else {
            json << ",";
        }
        json << "\"body\":\"" << EscapeJsonString(*body) << "\"";
    }

    if (hasEcho) {
        json << "}";
    }

    json << "}";
    return json.str();
}

} // namespace

RDKAppManagersLiteImplementation::RDKAppManagersLiteImplementation()
    : _listenerIdCounter(0) {
    LOGINFO("RDKAppManagersLiteImplementation created");
}

RDKAppManagersLiteImplementation::~RDKAppManagersLiteImplementation() {
    LOGINFO("RDKAppManagersLiteImplementation destroyed");
    std::lock_guard<std::mutex> guard(_lock);
    for (auto* n : _notifications) {
        n->Release();
    }
    _notifications.clear();
}

Core::hresult RDKAppManagersLiteImplementation::Request(
    const uint32_t flags,
    const string&  url,
    const string&  headers,
    const string&  queryParams,
    const string&  body,
    uint32_t&      code,
    string&        responseHeaders,
    string&        responseBody) {

    LOGINFO("RDKAppManagersLite::Request flags=0x%x url=%s", flags, url.c_str());

    responseHeaders = "Content-Type: application/json";

    const bool isGet    = ((flags & 0x01U) != 0U);
    const bool isPost   = ((flags & 0x02U) != 0U);
    const bool isDelete = ((flags & 0x04U) != 0U);
    (void)isDelete;


    if (url == "/as/apps/action/refresh" && isPost)
        return HandleAppsRefresh(code, responseBody);
    if (url == "/as/apps/subscription/action/launch" && isPost)
        return HandleAppsSubscriptionLaunch(body, code, responseBody);
    if (url == "/as/apps/subscription/action/return" && isPost)
        return HandleAppsSubscriptionReturn(body, code, responseBody);
    if (url == "/as/apps/launchrequest" && isGet)
        return HandleAppsLaunchRequest(queryParams, code, responseBody);
    if (url == "/as/apps/inactive/priority" && (isGet || isPost))
        return HandleAppsInactivePriority(isPost, body, code, responseBody);
    if (url == "/as/apps/tokens" && isGet)
        return HandleAppsTokens(code, responseBody);
    if (url == "/as/system/action/takefocus" && isPost)
        return HandleSystemTakeFocus(code, responseBody);
    if (url == "/as/apps/analytics/action/submit" && isPost)
        return HandleAppsAnalyticsSubmit(body, code, responseBody);
    if (url == "/as/apps/action/heartbeat" && isPost)
        return HandleAppsHeartbeat(body, code, responseBody);
    if (url == "/as/apps/action/enable" && isPost)
        return HandleAppsEnable(body, code, responseBody);
    if (url == "/as/apps/action/disable" && isPost)
        return HandleAppsDisable(body, code, responseBody);
    if (url == "/as/apps/action/refreshcert" && isPost)
        return HandleAppsRefreshCert(code, responseBody);
    if (url == "/as/apps/action/intent" && isPost)
        return HandleAppsIntent(body, code, responseBody);
    if (url == "/as/apps/action/lock" && isPost)
        return HandleAppsLock(body, code, responseBody);
    if (url == "/as/apps/action/unlock" && isPost)
        return HandleAppsUnlock(body, code, responseBody);

    // ------------------------------------------------------------------
    // Netflix
    // ------------------------------------------------------------------
    if (url == "/as/apps/netflix" && isGet)
        return HandleAppsNetflix(code, responseBody);
    if (url == "/as/apps/netflix/action/updatecookie" && isPost)
        return HandleNetflixUpdateCookie(body, code, responseBody);
    if (url == "/as/apps/netflix/action/updatetoken" && isPost)
        return HandleNetflixUpdateToken(body, code, responseBody);
    if (url == "/as/apps/netflix/action/restore" && isPost)
        return HandleNetflixRestore(code, responseBody);

    // ------------------------------------------------------------------
    // Secure storage (token-scoped)
    // ------------------------------------------------------------------
    if (url == "/as/apps/securestorage/action/get" && isGet)
        return HandleSecStorageGet(queryParams, code, responseBody);
    if (url == "/as/apps/securestorage/action/set" && isPost)
        return HandleSecStorageSet(queryParams, body, code, responseBody);
    if (url == "/as/apps/securestorage/action/clear" && isPost)
        return HandleSecStorageClear(queryParams, code, responseBody);

    // ------------------------------------------------------------------
    // Secure storage (appId-scoped)
    // ------------------------------------------------------------------
    if (url == "/as/apps/securestorage/app/action/get" && isGet)
        return HandleSecStorageAppGet(queryParams, code, responseBody);
    if (url == "/as/apps/securestorage/app/action/set" && isPost)
        return HandleSecStorageAppSet(queryParams, body, code, responseBody);
    if (url == "/as/apps/securestorage/app/action/clear" && isPost)
        return HandleSecStorageAppClear(queryParams, code, responseBody);

    // ------------------------------------------------------------------
    // Test / debug
    // ------------------------------------------------------------------
    if (url == "/as/test/getepgtoken" && isGet)
        return HandleTestGetEpgToken(code, responseBody);
    if (url == "/as/test/inactiveapps" && isGet)
        return HandleTestInactiveApps(code, responseBody);
    if (url == "/as/test/apps/netflix/action/setesn" && isPost)
        return HandleTestNetflixSetEsn(body, code, responseBody);
    if (url == "/as/test/apps/netflix/action/setuserloggedin" && isPost)
        return HandleTestNetflixSetUserLoggedIn(body, code, responseBody);
    if (url == "/as/test/action/triggersampleevents" && isPost)
        return HandleTestTriggerSampleEvents(code, responseBody);
    // wildcard: /as/test/apps/<uuid>/launchdetails
    if (isGet && fnmatch("/as/test/apps/*/launchdetails", url.c_str(), FNM_PATHNAME) == 0)
        return HandleTestAppLaunchDetails(url, code, responseBody);

    // Catch-all – 404
    code         = 404;
    responseBody = std::string(R"({"success":false,"error":"Not Found","url":")") + url + "\"}";    
    LOGERR("RDKAppManagersLite::Request – no route for url=%s flags=0x%x", url.c_str(), flags);
    return Core::ERROR_NONE;
}

// ===========================================================================
// IApplicationServiceConfig
// ===========================================================================

Core::hresult RDKAppManagersLiteImplementation::Config(string& config) {
    LOGINFO("RDKAppManagersLite::Config");

    config = R"({
  "service": "RDKAppManagersLite",
  "version": "1.0.0",
  "endpoints": [
    "POST /as/apps/action/refresh",
    "POST /as/apps/subscription/action/launch",
    "POST /as/apps/subscription/action/return",
    "GET  /as/apps/launchrequest",
    "GET  /as/apps/inactive/priority",
    "POST /as/apps/inactive/priority",
    "GET  /as/apps/tokens",
    "POST /as/system/action/takefocus",
    "POST /as/apps/analytics/action/submit",
    "POST /as/apps/action/heartbeat",
    "POST /as/apps/action/enable",
    "POST /as/apps/action/disable",
    "POST /as/apps/action/refreshcert",
    "POST /as/apps/action/intent",
    "POST /as/apps/action/lock",
    "POST /as/apps/action/unlock",
    "GET  /as/apps/netflix",
    "POST /as/apps/netflix/action/updatecookie",
    "POST /as/apps/netflix/action/updatetoken",
    "POST /as/apps/netflix/action/restore",
    "GET  /as/apps/securestorage/action/get",
    "POST /as/apps/securestorage/action/set",
    "POST /as/apps/securestorage/action/clear",
    "GET  /as/apps/securestorage/app/action/get",
    "POST /as/apps/securestorage/app/action/set",
    "POST /as/apps/securestorage/app/action/clear",
    "GET  /as/test/getepgtoken",
    "GET  /as/test/inactiveapps",
    "GET  /as/test/apps/<uuid>/launchdetails",
    "POST /as/test/apps/netflix/action/setesn",
        "POST /as/test/apps/netflix/action/setuserloggedin",
        "POST /as/test/action/triggersampleevents"
  ],
  "features": {
    "webSocket": true,
    "httpUpdates": true,
    "sysStatus": true
  }
})";

    return Core::ERROR_NONE;
}

// ===========================================================================
// App lifecycle stubs
// ===========================================================================

Core::hresult RDKAppManagersLiteImplementation::HandleAppsRefresh(
    uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleAppsRefresh");
    code = 200;
    responseBody = R"({"status":"success","message":"[stubbed] refresh completed"})";
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleAppsSubscriptionLaunch(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleAppsSubscriptionLaunch");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] subscription launch completed\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleAppsSubscriptionReturn(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleAppsSubscriptionReturn");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] subscription return completed\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleAppsLaunchRequest(
    const string& queryParams, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleAppsLaunchRequest");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] launch request completed\"",
        &queryParams,
        nullptr);
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleAppsInactivePriority(
    bool isPost, const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleAppsInactivePriority isPost=%d", (int)isPost);
    code = 200;
    if (isPost)
        responseBody = BuildStubResponse(
            "\"status\":\"success\",\"message\":\"[stubbed] priority set\"",
            nullptr,
            &body);
    else
        responseBody = R"({"priority":"normal","message":"[stubbed]"})";
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleAppsTokens(
    uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleAppsTokens");
    code = 200;
    responseBody = R"({"tokens":[],"message":"[stubbed]"})";
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleSystemTakeFocus(
    uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleSystemTakeFocus");
    code = 200;
    responseBody = R"({"status":"success","message":"[stubbed] focus taken"})";
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleAppsAnalyticsSubmit(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleAppsAnalyticsSubmit");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] analytics submitted\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleAppsHeartbeat(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleAppsHeartbeat");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] heartbeat received\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleAppsEnable(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleAppsEnable");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] app enabled\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleAppsDisable(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleAppsDisable");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] app disabled\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleAppsRefreshCert(
    uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleAppsRefreshCert");
    code = 200;
    responseBody = R"({"status":"success","message":"[stubbed] certificate refreshed"})";
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleAppsIntent(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleAppsIntent");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] app intent received\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleAppsLock(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleAppsLock");
    code = 200;
    responseBody = BuildStubResponse(
        "\"success\":false,\"message\":\"[stubbed] PIN control lock\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleAppsUnlock(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleAppsUnlock");
    code = 200;
    responseBody = BuildStubResponse(
        "\"success\":false,\"message\":\"[stubbed] PIN control unlock\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

// ===========================================================================
// Netflix stubs
// ===========================================================================

Core::hresult RDKAppManagersLiteImplementation::HandleAppsNetflix(
    uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleAppsNetflix");
    code = 200;
    responseBody = R"({"status":"success","message":"[stubbed] Netflix app info"})";
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleNetflixUpdateCookie(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleNetflixUpdateCookie");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] Netflix cookie updated\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleNetflixUpdateToken(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleNetflixUpdateToken");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] Netflix token updated\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleNetflixRestore(
    uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleNetflixRestore");
    code = 200;
    responseBody = R"({"status":"success","message":"[stubbed] Netflix restored"})";
    return Core::ERROR_NONE;
}

// ===========================================================================
// Secure storage stubs (token-scoped)
// ===========================================================================

Core::hresult RDKAppManagersLiteImplementation::HandleSecStorageGet(
    const string& queryParams, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleSecStorageGet params=%s", queryParams.c_str());
    // TODO: forward to Thunder SecureStorage plugin using scope/key/token from queryParams
    code = 200;
    responseBody = BuildStubResponse(
        "\"success\":true,\"value\":\"\",\"message\":\"[stubbed] secure storage get\"",
        &queryParams,
        nullptr);
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleSecStorageSet(
    const string& queryParams, const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleSecStorageSet params=%s", queryParams.c_str());
    // TODO: forward to Thunder SecureStorage plugin using scope/key/token/ttl from queryParams, value from body
    code = 200;
    responseBody = BuildStubResponse(
        "\"success\":true,\"message\":\"[stubbed] secure storage set\"",
        &queryParams,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleSecStorageClear(
    const string& queryParams, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleSecStorageClear params=%s", queryParams.c_str());
    // TODO: delete key or entire namespace depending on whether key param is present
    code = 200;
    responseBody = BuildStubResponse(
        "\"success\":true,\"message\":\"[stubbed] secure storage clear\"",
        &queryParams,
        nullptr);
    return Core::ERROR_NONE;
}

// ===========================================================================
// Secure storage stubs (appId-scoped)
// ===========================================================================

Core::hresult RDKAppManagersLiteImplementation::HandleSecStorageAppGet(
    const string& queryParams, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleSecStorageAppGet params=%s", queryParams.c_str());
    // TODO: forward to Thunder SecureStorage using scope/key/appId from queryParams
    code = 200;
    responseBody = BuildStubResponse(
        "\"success\":true,\"value\":\"\",\"message\":\"[stubbed] secure storage app get\"",
        &queryParams,
        nullptr);
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleSecStorageAppSet(
    const string& queryParams, const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleSecStorageAppSet params=%s", queryParams.c_str());
    // TODO: forward to Thunder SecureStorage using scope/key/appId/ttl from queryParams, value from body
    code = 200;
    responseBody = BuildStubResponse(
        "\"success\":true,\"message\":\"[stubbed] secure storage app set\"",
        &queryParams,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleSecStorageAppClear(
    const string& queryParams, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleSecStorageAppClear params=%s", queryParams.c_str());
    // TODO: delete key or entire appId namespace depending on whether key param is present
    code = 200;
    responseBody = BuildStubResponse(
        "\"success\":true,\"message\":\"[stubbed] secure storage app clear\"",
        &queryParams,
        nullptr);
    return Core::ERROR_NONE;
}

// ===========================================================================
// Test / debug stubs
// ===========================================================================

Core::hresult RDKAppManagersLiteImplementation::HandleTestGetEpgToken(
    uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleTestGetEpgToken");
    // TODO: get home app ID, check it is running, return its token
    code = 200;
    responseBody = R"({"status":"success","token":"test-epg-token-12345","message":"[stubbed]"})";
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleTestInactiveApps(
    uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleTestInactiveApps");
    // TODO: query app manager for suspended/hibernated GUI apps
    code = 200;
    responseBody = R"({"status":"success","inactiveApps":[{"id":"netflix","status":"suspended"},{"id":"youtube","status":"hibernated"}],"message":"[stubbed]"})";
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleTestAppLaunchDetails(
    const string& url, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleTestAppLaunchDetails url=%s", url.c_str());
    static const std::regex matcher("^\\/as\\/test\\/apps\\/([a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12})\\/launchdetails$");
    std::smatch match;
    if (!std::regex_match(url, match, matcher) || match.size() != 2) {
        code = 404;
        responseBody = R"({"success":false,"error":"Invalid UUID in URL path"})";
        return Core::ERROR_NONE;
    }
    const string uuid = match[1].str();
    // TODO: validate uuid, find appId, fetch real launch details
    code = 200;
    responseBody = R"({"status":"success","launchDetails":{"uuid":")"
        + uuid + R"(","tag":1234567890,"launchMethod":"direct","launchArguments":"--fullscreen","launched":true},"message":"[stubbed]"})";
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleTestNetflixSetEsn(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleTestNetflixSetEsn");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] Netflix ESN set\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleTestNetflixSetUserLoggedIn(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleTestNetflixSetUserLoggedIn");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] Netflix user logged-in status set\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::HandleTestTriggerSampleEvents(
    uint32_t& code, string& responseBody) {
    LOGINFO("RDKAppManagersLite::HandleTestTriggerSampleEvents");

    const string wsUrl     = "/as/test/events/ws";
    const string wsMessage = R"({"event":"sample-ws-update","source":"RDKAppManagersLite"})";
    const string httpUrl   = "/as/test/events/http";
    const uint32_t httpCode = 200;
    const string sysStatus = R"({"status":"ok","source":"RDKAppManagersLite"})";

    NotifyWebSocketUpdate(wsUrl, wsMessage);
    NotifyHttpUpdate(httpUrl, httpCode);
    NotifySysStatusUpdate(sysStatus);

    code = 200;
    responseBody = R"({"status":"success","message":"[stubbed] sample events triggered","events":["onNotifyWebSocketUpdate","onNotifyHttpUpdate","onNotifySysStatusUpdate"]})";
    return Core::ERROR_NONE;
}

// ===========================================================================
// WebSocket event helpers
// ===========================================================================

void RDKAppManagersLiteImplementation::SendIntentEvent(
    const string& token, const string& appId) {
    LOGINFO("RDKAppManagersLite::SendIntentEvent token=%s appId=%s",
        token.c_str(), appId.c_str());
    const string wsUrl = "/as/apps/" + token + "/intents";
    const string payload = R"({"appIntent":{"action":"launch","uri":"netflix://watch/80057281","extras":{"contentId":"80057281","source":"voice"}}})";
    // TODO: deliver via WebSocket/asproxy channel
    NotifyWebSocketUpdate(wsUrl, payload);
}

void RDKAppManagersLiteImplementation::SendAppUpdateEvent(const string& uuid) {
    LOGINFO("RDKAppManagersLite::SendAppUpdateEvent uuid=%s", uuid.c_str());
    const string wsUrl = "/as/test/apps/" + uuid + "/updates";
    std::ostringstream payload;
    payload << R"({"appupdates":{"launchdetails":)" << std::time(nullptr) << "}}";
    // TODO: deliver via WebSocket/asproxy channel
    NotifyWebSocketUpdate(wsUrl, payload.str());
}

// ===========================================================================
// IApplicationServiceListener – registration
// ===========================================================================

Core::hresult RDKAppManagersLiteImplementation::Register(
    Exchange::IApplicationServiceListener::INotification* notification) {

    ASSERT(notification != nullptr);
    std::lock_guard<std::mutex> guard(_lock);

    auto it = std::find(_notifications.begin(), _notifications.end(), notification);
    if (it == _notifications.end()) {
        notification->AddRef();
        _notifications.push_back(notification);
        LOGINFO("RDKAppManagersLite - notification registered (total=%zu)",
            _notifications.size());
    }
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::Unregister(
    Exchange::IApplicationServiceListener::INotification* notification) {

    ASSERT(notification != nullptr);
    std::lock_guard<std::mutex> guard(_lock);

    auto it = std::find(_notifications.begin(), _notifications.end(), notification);
    if (it != _notifications.end()) {
        (*it)->Release();
        _notifications.erase(it);
        LOGINFO("RDKAppManagersLite - notification unregistered (total=%zu)",
            _notifications.size());
    }
    return Core::ERROR_NONE;
}

// ---------------------------------------------------------------------------
// Listener slot registration helpers
// ---------------------------------------------------------------------------

Core::hresult RDKAppManagersLiteImplementation::RegisterWebSocketListener(
    const string& url, const string& clientId, string& listenerId) {

    std::lock_guard<std::mutex> guard(_lock);
    std::ostringstream oss;
    oss << "ws-" << (++_listenerIdCounter);
    listenerId = oss.str();
    LOGINFO("RegisterWebSocketListener url=%s clientId=%s -> id=%s",
        url.c_str(), clientId.c_str(), listenerId.c_str());
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::UnregisterWebSocketListener(
    const string& listenerId) {

    LOGINFO("UnregisterWebSocketListener id=%s", listenerId.c_str());
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::RegisterUpdatesListener(
    const string& url, const string& clientId, string& listenerId) {

    std::lock_guard<std::mutex> guard(_lock);
    std::ostringstream oss;
    oss << "upd-" << (++_listenerIdCounter);
    listenerId = oss.str();
    LOGINFO("RegisterUpdatesListener url=%s clientId=%s -> id=%s",
        url.c_str(), clientId.c_str(), listenerId.c_str());
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::UnregisterUpdatesListener(
    const string& listenerId) {

    LOGINFO("UnregisterUpdatesListener id=%s", listenerId.c_str());
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::RegisterSysStatusListener(
    const string& clientId, string& listenerId) {

    std::lock_guard<std::mutex> guard(_lock);
    std::ostringstream oss;
    oss << "sys-" << (++_listenerIdCounter);
    listenerId = oss.str();
    LOGINFO("RegisterSysStatusListener clientId=%s -> id=%s",
        clientId.c_str(), listenerId.c_str());
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersLiteImplementation::UnregisterSysStatusListener(
    const string& listenerId) {

    LOGINFO("UnregisterSysStatusListener id=%s", listenerId.c_str());
    return Core::ERROR_NONE;
}

// ---------------------------------------------------------------------------
// Internal event fan-out helpers
// ---------------------------------------------------------------------------

void RDKAppManagersLiteImplementation::NotifyWebSocketUpdate(
    const string& url, const string& message) {

    std::lock_guard<std::mutex> guard(_lock);
    for (auto* n : _notifications) {
        n->OnNotifyWebSocketUpdate(url, message);
    }
}

void RDKAppManagersLiteImplementation::NotifyHttpUpdate(
    const string& url, uint32_t code) {

    std::lock_guard<std::mutex> guard(_lock);
    for (auto* n : _notifications) {
        n->OnNotifyHttpUpdate(url, code);
    }
}

void RDKAppManagersLiteImplementation::NotifySysStatusUpdate(const string& status) {
    std::lock_guard<std::mutex> guard(_lock);
    for (auto* n : _notifications) {
        n->OnNotifySysStatusUpdate(status);
    }
}

} // namespace Plugin
} // namespace WPEFramework
