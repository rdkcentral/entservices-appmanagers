#include "RDKAMServiceLiteImplementation.h"
#include "RDKAMServiceLiteUtils.h"

#include <fnmatch.h>
#include <cctype>
#include <iomanip>
#include <regex>
#include <sstream>

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

// Register the implementation class with Thunder's object factory so that the
// plugin host can instantiate it out-of-process (or in-process when mode=Local).
SERVICE_REGISTRATION(RDKAMServiceLiteImplementation, 1, 0, 0);

namespace WPEFramework {
namespace Plugin {

using namespace RDKAMServiceLiteUtils;

RDKAMServiceLiteImplementation::RDKAMServiceLiteImplementation()
    : _listenerIdCounter(0) {
    LOGINFO("RDKAMServiceLiteImplementation created");
}

RDKAMServiceLiteImplementation::~RDKAMServiceLiteImplementation() {
    LOGINFO("RDKAMServiceLiteImplementation destroyed");
    std::lock_guard<std::mutex> guard(_lock);
    for (auto* n : _notifications) {
        n->Release();
    }
    _notifications.clear();
}

Core::hresult RDKAMServiceLiteImplementation::Request(
    const uint32_t flags,
    const string&  url,
    const string&  headers,
    const string&  queryParams,
    const string&  body,
    uint32_t&      code,
    string&        responseHeaders,
    string&        responseBody) {

    LOGINFO("RDKAMServiceLite::Request flags=0x%x url=%s", flags, url.c_str());

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
    LOGERR("RDKAMServiceLite::Request – no route for url=%s flags=0x%x", url.c_str(), flags);
    return Core::ERROR_NONE;
}

// ===========================================================================
// IApplicationServiceConfig
// ===========================================================================

Core::hresult RDKAMServiceLiteImplementation::Config(string& config) {
    LOGINFO("RDKAMServiceLite::Config");

    config = R"({
  "service": "RDKAMServiceLite",
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

Core::hresult RDKAMServiceLiteImplementation::HandleAppsRefresh(
    uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleAppsRefresh");
    code = 200;
    responseBody = R"({"status":"success","message":"[stubbed] refresh completed"})";
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleAppsSubscriptionLaunch(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleAppsSubscriptionLaunch");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] subscription launch completed\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleAppsSubscriptionReturn(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleAppsSubscriptionReturn");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] subscription return completed\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleAppsLaunchRequest(
    const string& queryParams, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleAppsLaunchRequest");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] launch request completed\"",
        &queryParams,
        nullptr);
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleAppsInactivePriority(
    bool isPost, const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleAppsInactivePriority isPost=%d", (int)isPost);
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

Core::hresult RDKAMServiceLiteImplementation::HandleAppsTokens(
    uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleAppsTokens");
    code = 200;
    responseBody = R"({"tokens":[],"message":"[stubbed]"})";
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleSystemTakeFocus(
    uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleSystemTakeFocus");
    code = 200;
    responseBody = R"({"status":"success","message":"[stubbed] focus taken"})";
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleAppsAnalyticsSubmit(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleAppsAnalyticsSubmit");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] analytics submitted\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleAppsHeartbeat(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleAppsHeartbeat");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] heartbeat received\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleAppsEnable(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleAppsEnable");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] app enabled\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleAppsDisable(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleAppsDisable");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] app disabled\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleAppsRefreshCert(
    uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleAppsRefreshCert");
    code = 200;
    responseBody = R"({"status":"success","message":"[stubbed] certificate refreshed"})";
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleAppsIntent(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleAppsIntent");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] app intent received\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleAppsLock(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleAppsLock");
    code = 200;
    responseBody = BuildStubResponse(
        "\"success\":false,\"message\":\"[stubbed] PIN control lock\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleAppsUnlock(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleAppsUnlock");
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

Core::hresult RDKAMServiceLiteImplementation::HandleAppsNetflix(
    uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleAppsNetflix");
    code = 200;
    responseBody = R"({"status":"success","message":"[stubbed] Netflix app info"})";
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleNetflixUpdateCookie(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleNetflixUpdateCookie");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] Netflix cookie updated\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleNetflixUpdateToken(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleNetflixUpdateToken");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] Netflix token updated\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleNetflixRestore(
    uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleNetflixRestore");
    code = 200;
    responseBody = R"({"status":"success","message":"[stubbed] Netflix restored"})";
    return Core::ERROR_NONE;
}

// ===========================================================================
// Secure storage stubs (token-scoped)
// ===========================================================================

Core::hresult RDKAMServiceLiteImplementation::HandleSecStorageGet(
    const string& queryParams, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleSecStorageGet params=%s", queryParams.c_str());
    // TODO: forward to Thunder SecureStorage plugin using scope/key/token from queryParams
    code = 200;
    responseBody = BuildStubResponse(
        "\"success\":true,\"value\":\"\",\"message\":\"[stubbed] secure storage get\"",
        &queryParams,
        nullptr);
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleSecStorageSet(
    const string& queryParams, const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleSecStorageSet params=%s", queryParams.c_str());
    // TODO: forward to Thunder SecureStorage plugin using scope/key/token/ttl from queryParams, value from body
    code = 200;
    responseBody = BuildStubResponse(
        "\"success\":true,\"message\":\"[stubbed] secure storage set\"",
        &queryParams,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleSecStorageClear(
    const string& queryParams, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleSecStorageClear params=%s", queryParams.c_str());
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

Core::hresult RDKAMServiceLiteImplementation::HandleSecStorageAppGet(
    const string& queryParams, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleSecStorageAppGet params=%s", queryParams.c_str());
    // TODO: forward to Thunder SecureStorage using scope/key/appId from queryParams
    code = 200;
    responseBody = BuildStubResponse(
        "\"success\":true,\"value\":\"\",\"message\":\"[stubbed] secure storage app get\"",
        &queryParams,
        nullptr);
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleSecStorageAppSet(
    const string& queryParams, const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleSecStorageAppSet params=%s", queryParams.c_str());
    // TODO: forward to Thunder SecureStorage using scope/key/appId/ttl from queryParams, value from body
    code = 200;
    responseBody = BuildStubResponse(
        "\"success\":true,\"message\":\"[stubbed] secure storage app set\"",
        &queryParams,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleSecStorageAppClear(
    const string& queryParams, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleSecStorageAppClear params=%s", queryParams.c_str());
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

Core::hresult RDKAMServiceLiteImplementation::HandleTestGetEpgToken(
    uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleTestGetEpgToken");
    // TODO: get home app ID, check it is running, return its token
    code = 200;
    responseBody = R"({"status":"success","token":"test-epg-token-12345","message":"[stubbed]"})";
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleTestInactiveApps(
    uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleTestInactiveApps");
    // TODO: query app manager for suspended/hibernated GUI apps
    code = 200;
    responseBody = R"({"status":"success","inactiveApps":[{"id":"netflix","status":"suspended"},{"id":"youtube","status":"hibernated"}],"message":"[stubbed]"})";
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleTestAppLaunchDetails(
    const string& url, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleTestAppLaunchDetails url=%s", url.c_str());
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

Core::hresult RDKAMServiceLiteImplementation::HandleTestNetflixSetEsn(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleTestNetflixSetEsn");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] Netflix ESN set\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleTestNetflixSetUserLoggedIn(
    const string& body, uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleTestNetflixSetUserLoggedIn");
    code = 200;
    responseBody = BuildStubResponse(
        "\"status\":\"success\",\"message\":\"[stubbed] Netflix user logged-in status set\"",
        nullptr,
        &body);
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::HandleTestTriggerSampleEvents(
    uint32_t& code, string& responseBody) {
    LOGINFO("RDKAMServiceLite::HandleTestTriggerSampleEvents");

    const string wsUrl     = "/as/test/events/ws";
    const string wsMessage = R"({"event":"sample-ws-update","source":"RDKAMServiceLite"})";
    const string httpUrl   = "/as/test/events/http";
    const uint32_t httpCode = 200;
    const string sysStatus = R"({"status":"ok","source":"RDKAMServiceLite"})";

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

void RDKAMServiceLiteImplementation::SendIntentEvent(
    const string& token, const string& appId) {
    LOGINFO("RDKAMServiceLite::SendIntentEvent token=%s appId=%s",
        token.c_str(), appId.c_str());
    const string wsUrl = "/as/apps/" + token + "/intents";
    const string payload = R"({"appIntent":{"action":"launch","uri":"netflix://watch/80057281","extras":{"contentId":"80057281","source":"voice"}}})";
    // TODO: deliver via WebSocket/asproxy channel
    NotifyWebSocketUpdate(wsUrl, payload);
}

void RDKAMServiceLiteImplementation::SendAppUpdateEvent(const string& uuid) {
    LOGINFO("RDKAMServiceLite::SendAppUpdateEvent uuid=%s", uuid.c_str());
    const string wsUrl = "/as/test/apps/" + uuid + "/updates";
    std::ostringstream payload;
    payload << R"({"appupdates":{"launchdetails":)" << std::time(nullptr) << "}}";
    // TODO: deliver via WebSocket/asproxy channel
    NotifyWebSocketUpdate(wsUrl, payload.str());
}

// ===========================================================================
// IApplicationServiceListener – registration
// ===========================================================================

Core::hresult RDKAMServiceLiteImplementation::Register(
    Exchange::IApplicationServiceListener::INotification* notification) {

    ASSERT(notification != nullptr);
    std::lock_guard<std::mutex> guard(_lock);

    auto it = std::find(_notifications.begin(), _notifications.end(), notification);
    if (it == _notifications.end()) {
        notification->AddRef();
        _notifications.push_back(notification);
        LOGINFO("RDKAMServiceLite - notification registered (total=%zu)",
            _notifications.size());
    }
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::Unregister(
    Exchange::IApplicationServiceListener::INotification* notification) {

    ASSERT(notification != nullptr);
    std::lock_guard<std::mutex> guard(_lock);

    auto it = std::find(_notifications.begin(), _notifications.end(), notification);
    if (it != _notifications.end()) {
        (*it)->Release();
        _notifications.erase(it);
        LOGINFO("RDKAMServiceLite - notification unregistered (total=%zu)",
            _notifications.size());
    }
    return Core::ERROR_NONE;
}

// ---------------------------------------------------------------------------
// Listener slot registration helpers
// ---------------------------------------------------------------------------

Core::hresult RDKAMServiceLiteImplementation::RegisterWebSocketListener(
    const string& url, const string& clientId, string& listenerId) {

    std::lock_guard<std::mutex> guard(_lock);
    std::ostringstream oss;
    oss << "ws-" << (++_listenerIdCounter);
    listenerId = oss.str();
    LOGINFO("RegisterWebSocketListener url=%s clientId=%s -> id=%s",
        url.c_str(), clientId.c_str(), listenerId.c_str());
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::UnregisterWebSocketListener(
    const string& listenerId) {

    LOGINFO("UnregisterWebSocketListener id=%s", listenerId.c_str());
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::RegisterUpdatesListener(
    const string& url, const string& clientId, string& listenerId) {

    std::lock_guard<std::mutex> guard(_lock);
    std::ostringstream oss;
    oss << "upd-" << (++_listenerIdCounter);
    listenerId = oss.str();
    LOGINFO("RegisterUpdatesListener url=%s clientId=%s -> id=%s",
        url.c_str(), clientId.c_str(), listenerId.c_str());
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::UnregisterUpdatesListener(
    const string& listenerId) {

    LOGINFO("UnregisterUpdatesListener id=%s", listenerId.c_str());
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::RegisterSysStatusListener(
    const string& clientId, string& listenerId) {

    std::lock_guard<std::mutex> guard(_lock);
    std::ostringstream oss;
    oss << "sys-" << (++_listenerIdCounter);
    listenerId = oss.str();
    LOGINFO("RegisterSysStatusListener clientId=%s -> id=%s",
        clientId.c_str(), listenerId.c_str());
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceLiteImplementation::UnregisterSysStatusListener(
    const string& listenerId) {

    LOGINFO("UnregisterSysStatusListener id=%s", listenerId.c_str());
    return Core::ERROR_NONE;
}

// ---------------------------------------------------------------------------
// Internal event fan-out helpers
// ---------------------------------------------------------------------------

void RDKAMServiceLiteImplementation::NotifyWebSocketUpdate(
    const string& url, const string& message) {

    std::lock_guard<std::mutex> guard(_lock);
    for (auto* n : _notifications) {
        n->OnNotifyWebSocketUpdate(url, message);
    }
}

void RDKAMServiceLiteImplementation::NotifyHttpUpdate(
    const string& url, uint32_t code) {

    std::lock_guard<std::mutex> guard(_lock);
    for (auto* n : _notifications) {
        n->OnNotifyHttpUpdate(url, code);
    }
}

void RDKAMServiceLiteImplementation::NotifySysStatusUpdate(const string& status) {
    std::lock_guard<std::mutex> guard(_lock);
    for (auto* n : _notifications) {
        n->OnNotifySysStatusUpdate(status);
    }
}

} // namespace Plugin
} // namespace WPEFramework
