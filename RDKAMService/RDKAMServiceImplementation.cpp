#include "RDKAMServiceImplementation.h"
#include "RDKAMServiceUtils.h"
#include "SystemSettings.h"
#include "TestPreferences.h"
#include <json/json.h>
#include <vector>
#include <map>
#include <cctype>

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

SERVICE_REGISTRATION(RDKAMServiceImplementation, 1, 0, 0);

namespace WPEFramework {
namespace Plugin {

RDKAMServiceImplementation::RDKAMServiceImplementation()
    : m_listenerIdCounter(0)
    , m_shell(nullptr)
    , m_service(std::make_unique<RDKAMServiceHandler>(nullptr))
{
    LOGINFO("RDKAMService Constructor - START");
    if (m_service) {
        m_service->SetEventHandler(this);
        m_service->StartStatusListener();
    }
    LOGINFO("RDKAMService Constructor - END");
}
  
RDKAMServiceImplementation::~RDKAMServiceImplementation() {
    LOGINFO("RDKAMService Destructor");
  
    if (m_service) {
        m_service->StopAppManagerListener();
        m_service->StopStatusListener();
        m_service.reset();
    }
   
    // Cleanup resources
    if (m_shell != nullptr) {
        m_shell->Release();
        m_shell = nullptr;
    }
    
    LOGINFO("RDKAMService Destroyed");
}

// Request
Core::hresult RDKAMServiceImplementation::Request(const uint32_t flags, const string& url, const string& headers,
                                               const string& queryParams, const string& body,
                                               uint32_t& code, string& responseHeaders, string& responseBody) {
    LOGINFO("RDKAMService::Request - flags=%u, url=%s", flags, url.c_str());
    LOGINFO("  headers='%s'", headers.c_str());
    LOGINFO("  queryParams='%s' (length=%zu)", queryParams.c_str(), queryParams.length());
    LOGINFO("  body='%s'", body.c_str());

    const std::pair<std::string, std::string> urlInfo = RDKAMServiceUtils::NormalizeUrlAndExtractQuery(url, queryParams);
    const std::string& normalizedUrl = urlInfo.first;
    const std::string& effectiveQueryParams = urlInfo.second;

    const bool isGet = ((flags & 0x01U) != 0U);
    const bool isPost = ((flags & 0x02U) != 0U);

    auto isMethodAllowedForUrl = [&](const std::string& path) -> bool {
        if (path == "/as/apps" || path == "/as/system/stats") {
            return isGet;
        }
        if (path == "/as/apps/action/launch" ||
            path == "/as/apps/action/close" ||
            path == "/as/apps/action/kill" ||
            path == "/as/apps/action/focus" ||
            path == "/as/apps/action/reset") {
            return isPost;
        }
        return true;
    };

    responseHeaders = "Content-Type: application/json";

    if (m_shell == nullptr || m_service == nullptr) {
        code = 503;
        responseBody = RDKAMServiceUtils::BuildErrorResponse("Service shell is not configured");
        LOGERR("Request failed: shell/service is not configured");
        return Core::ERROR_NONE;
    }

    const RDKAMServiceUtils::RequestContext context = RDKAMServiceUtils::BuildRequestContext(normalizedUrl, effectiveQueryParams, body);

    Core::hresult status = Core::ERROR_NONE;
    code = 500;

    static const std::map<std::string, RDKAMServiceUtils::RouteEntry> requestMap = RDKAMServiceUtils::LoadRequestMap();

    if (!isMethodAllowedForUrl(normalizedUrl)) {
        code = 405;
        responseBody = RDKAMServiceUtils::BuildErrorResponse("Method not allowed");
        LOGERR("Method not allowed for url=%s normalized=%s flags=%u", url.c_str(), normalizedUrl.c_str(), flags);
        return Core::ERROR_NONE;
    }

    const auto routeIt = requestMap.find(normalizedUrl);
    if (routeIt != requestMap.end()) {
        status = m_service->DispatchMappedRequest(routeIt->second.methods, routeIt->second.params, context.runtimeParams, code, responseBody);
        RDKAMServiceUtils::EnsureErrorResponse(status, normalizedUrl, responseBody);

        LOGINFO("Request executed url=%s normalized=%s methodCount=%zu status=%u code=%u",
            url.c_str(), normalizedUrl.c_str(), routeIt->second.methods.size(), status, code);

        return Core::ERROR_NONE;
    }

    if (normalizedUrl == "/as/apps/action/launch") {
        status = m_service->LaunchAppRequest(context.appId, context.token, context.mode, context.intent, context.launchArgs, code, responseBody);
    } else if (normalizedUrl == "/as/apps/status") {
        responseBody = m_service->GetAppsStatus();
        code = 200;
        status = Core::ERROR_NONE;
    } else if (normalizedUrl == "/as/apps/action/reset") {
        status = m_service->ResetAppDataRequest(context.appId, code, responseBody);
    } else if (normalizedUrl == "/as/system/stats") {
        status = m_service->GetSystemStatsRequest(code, responseBody);
    } else {
        code = 404;
        Json::Value errorResponse;
        errorResponse["success"] = false;
        errorResponse["error"] = "Unsupported URL";
        errorResponse["url"] = url;
        errorResponse["normalizedUrl"] = normalizedUrl;

        Json::StreamWriterBuilder writerBuilder;
        writerBuilder["indentation"] = "";
        responseBody = Json::writeString(writerBuilder, errorResponse);
        LOGERR("Request mapping not found for url=%s normalized=%s", url.c_str(), normalizedUrl.c_str());
        return Core::ERROR_NONE;
    }

    RDKAMServiceUtils::EnsureErrorResponse(status, normalizedUrl, responseBody);

    LOGINFO("Request executed by explicit handler url=%s normalized=%s status=%u code=%u",
        url.c_str(), normalizedUrl.c_str(), status, code);

    return Core::ERROR_NONE;
}

// Config
Core::hresult RDKAMServiceImplementation::Config(string& config) {
    LOGINFO("RDKAMService::Config request");
    config = R"JSON(
{
    "providesSystemStatus" : true,
    "domain": "/apps",
    "uris": [
        {
            "path": "/as/apps/status",
            "method": "ws",
            "thread": "AS_WS_APPS"
        },
        {
            "path": "/as/apps",
            "method": "get"
        },
        {
            "path": "/as/apps/action/launch",
            "method": "post",
            "expectsBody": true
        },
        {
            "path": "/as/apps/action/close",
            "method": "post"
        },
        {
            "path": "/as/apps/action/kill",
            "method": "post"
        },
        {
            "path": "/as/system/stats",
            "method": "get"
        },
        {
            "path": "/as/apps/action/focus",
            "method": "post"
        },
        {
            "path": "/as/apps/action/reset",
            "method": "post"
        }
    ],
    "systemSettings": [
        "appCatalogueURI",
        "apps.datamigration.enable",
        "apps.enforcepin",
        "apps.softcat.enable",
        "system.devicelocation",
        "system.acceptcasting"
    ]
}
)JSON";

    return Core::ERROR_NONE;
}

// Listener registrations
Core::hresult RDKAMServiceImplementation::RegisterWebSocketListener(const string& url, const string& clientId, string& listenerId) {
    LOGINFO("RegisterWebSocketListener url=%s clientId=%s", url.c_str(), clientId.c_str());
    std::lock_guard<std::mutex> lock(m_listenerMutex);

    // Generate unique listener ID
    listenerId = "ws-" + std::to_string(++m_listenerIdCounter);

    // Store listener registration in-memory
    ListenerInfo info;
    info.listenerId = listenerId;
    info.url = url;
    info.clientId = clientId;

    m_wsListeners.push_back(info);

    LOGINFO("RegisterWebSocketListener success: listenerId=%s, total=%zu",
           listenerId.c_str(), m_wsListeners.size());
    return Core::ERROR_NONE;
}
Core::hresult RDKAMServiceImplementation::UnregisterWebSocketListener(const string& listenerId) {
    LOGINFO("UnregisterWebSocketListener id=%s", listenerId.c_str());
    std::lock_guard<std::mutex> lock(m_listenerMutex);

    auto it = std::find_if(m_wsListeners.begin(), m_wsListeners.end(),
        [&listenerId](const ListenerInfo& info) {
            return info.listenerId == listenerId;
        });

    if (it == m_wsListeners.end()) {
        return Core::ERROR_UNKNOWN_KEY;
    }

    m_wsListeners.erase(it);

    LOGINFO("UnregisterWebSocketListener success, remaining=%zu",
           m_wsListeners.size());
    return Core::ERROR_NONE;
}
Core::hresult RDKAMServiceImplementation::RegisterUpdatesListener(const string& url, const string& clientId, string& listenerId) {
    LOGINFO("RegisterUpdatesListener url=%s clientId=%s", url.c_str(), clientId.c_str());
    std::lock_guard<std::mutex> lock(m_listenerMutex);

    // Generate unique listener ID
    listenerId = "http-" + std::to_string(++m_listenerIdCounter);

    // Store listener registration in-memory
    ListenerInfo info;
    info.listenerId = listenerId;
    info.url = url;
    info.clientId = clientId;

    m_httpListeners.push_back(info);

    LOGINFO("RegisterUpdatesListener success: listenerId=%s, total=%zu",
           listenerId.c_str(), m_httpListeners.size());
    return Core::ERROR_NONE;
}
Core::hresult RDKAMServiceImplementation::UnregisterUpdatesListener(const string& listenerId) {
    LOGINFO("UnregisterUpdatesListener id=%s", listenerId.c_str());
    std::lock_guard<std::mutex> lock(m_listenerMutex);

    auto it = std::find_if(m_httpListeners.begin(), m_httpListeners.end(),
        [&listenerId](const ListenerInfo& info) {
            return info.listenerId == listenerId;
        });

    if (it == m_httpListeners.end()) {
        return Core::ERROR_UNKNOWN_KEY;
    }

    m_httpListeners.erase(it);

    LOGINFO("UnregisterUpdatesListener success, remaining=%zu",
           m_httpListeners.size());
    return Core::ERROR_NONE;
}
Core::hresult RDKAMServiceImplementation::RegisterSysStatusListener(const string& clientId, string& listenerId) {
    LOGINFO("RegisterSysStatusListener clientId=%s", clientId.c_str());
    {
        std::lock_guard<std::mutex> lock(m_listenerMutex);

        // Generate unique listener ID
        listenerId = "sys-" + std::to_string(++m_listenerIdCounter);

        // Store listener registration in-memory
        ListenerInfo info;
        info.listenerId = listenerId;
        info.url = "";  // SysStatus doesn't use URL
        info.clientId = clientId;

        m_sysListeners.push_back(info);

        LOGINFO("RegisterSysStatusListener success: listenerId=%s, total=%zu",
               listenerId.c_str(), m_sysListeners.size());

        // Send cached status to new listener immediately (mirrors D-Bus behavior)
        if (!m_cachedSysStatus.empty()) {
            LOGINFO("RegisterSysStatusListener - Sending cached status to new listener: %s",
                   m_cachedSysStatus.c_str());
        }
    }

    // Send cached status immediately if available (outside lock to avoid deadlock)
    if (!m_cachedSysStatus.empty()) {
        NotifySysStatusUpdate(m_cachedSysStatus);
    }
    return Core::ERROR_NONE;
}
Core::hresult RDKAMServiceImplementation::UnregisterSysStatusListener(const string& listenerId)
{
    LOGINFO("UnregisterSysStatusListener id=%s", listenerId.c_str());
    std::lock_guard<std::mutex> lock(m_listenerMutex);

    auto it = std::find_if(m_sysListeners.begin(), m_sysListeners.end(),
        [&listenerId](const ListenerInfo& info) {
            return info.listenerId == listenerId;
        });

    if (it == m_sysListeners.end()) {
        return Core::ERROR_UNKNOWN_KEY;
    }

    m_sysListeners.erase(it);

    LOGINFO("UnregisterSysStatusListener success, remaining=%zu",
           m_sysListeners.size());
    return Core::ERROR_NONE;
}
// Listener observer management
Core::hresult RDKAMServiceImplementation::Register(IApplicationServiceListener::INotification* notification) {
    ASSERT(nullptr != notification);
    std::unique_lock<std::mutex> lock(mNotificationMutex);
    // Make sure we can't register the same notification callback multiple times
    if (std::find(mNotifications.begin(), mNotifications.end(), notification) == mNotifications.end())
    {
        LOGINFO("Register notification");
        mNotifications.push_back(notification);
        notification->AddRef();
    }
    return Core::ERROR_NONE;
}
Core::hresult RDKAMServiceImplementation::Unregister(IApplicationServiceListener::INotification* notification) {
    Core::hresult status = Core::ERROR_GENERAL;
    ASSERT(nullptr != notification);
    std::unique_lock<std::mutex> lock(mNotificationMutex);
    // Make sure we can't unregister the same notification callback multiple times
    auto itr = std::find(mNotifications.begin(), mNotifications.end(), notification);
    if (itr != mNotifications.end())
    {
        (*itr)->Release();
        LOGINFO("Unregister notification");
        mNotifications.erase(itr);
        status = Core::ERROR_NONE;
    }
    else
    {
        LOGERR("notification not found");
    }
    return status;
}
// SystemInfo
Core::hresult RDKAMServiceImplementation::GetSystemInfo(string& info) {
    LOGINFO("GetSystemInfo");
    info = "{}";
    return Core::ERROR_NONE;
}

// SystemSettings
Core::hresult RDKAMServiceImplementation::GetSystemSetting(const string& name, string& value) {
    LOGINFO("GetSystemSetting name=%s", name.c_str());

    std::string lowerName(name);
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    SystemSettings& settings = SystemSettings::Instance();

    if (lowerName.empty()) {
        value = settings.ToJson();
        return Core::ERROR_NONE;
    }

    if (!settings.Get(lowerName, value)) {
        value.clear();
        return Core::ERROR_UNKNOWN_KEY;
    }

    return Core::ERROR_NONE;
}
Core::hresult RDKAMServiceImplementation::SetSystemSetting(const string& name, const string& value) {
    LOGINFO("SetSystemSetting name=%s", name.c_str());

    std::string lowerName(name);
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    if (lowerName.empty()) {
        return Core::ERROR_INVALID_INPUT_LENGTH;
    }

    SystemSettings& settings = SystemSettings::Instance();
    if (!settings.Set(lowerName, value)) {
        return Core::ERROR_UNKNOWN_KEY;
    }

    return Core::ERROR_NONE;
}

// TestPreferences
Core::hresult RDKAMServiceImplementation::GetTestPreference(const string& name, string& value) {
    LOGINFO("GetTestPreference name=%s", name.c_str());

    std::string lowerName(name);
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    TestPreferences& prefs = TestPreferences::Instance();

    if (lowerName.empty()) {
        value = prefs.ToJson();
        return Core::ERROR_NONE;
    }

    if (!prefs.Get(lowerName, value)) {
        value.clear();
        return Core::ERROR_UNKNOWN_KEY;
    }

    return Core::ERROR_NONE;
}
Core::hresult RDKAMServiceImplementation::SetTestPreference(const string& name, const string& value, const uint32_t pin) {
    LOGINFO("SetTestPreference name=%s", name.c_str());

    std::string lowerName(name);
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    if (lowerName.empty()) {
        return Core::ERROR_INVALID_INPUT_LENGTH;
    }

    TestPreferences& prefs = TestPreferences::Instance();
    if (!prefs.Set(lowerName, value)) {
        return Core::ERROR_UNKNOWN_KEY;
    }

    (void)pin;
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceImplementation::GetDiagContexts(string& contexts) {
    LOGINFO("GetDiagContexts");
    return Core::ERROR_NONE;
}


Core::hresult RDKAMServiceImplementation::SetDiagContexts(const string& contexts, uint32_t& updated) {
    LOGINFO("SetDiagContexts ctx.len=%zu", contexts.size());
    return Core::ERROR_NONE;
}



void RDKAMServiceImplementation::NotifyWebSocketUpdate(const std::string& url, const std::string& message) {
    LOGINFO("NotifyWebSocketUpdate url=%s", url.c_str());
    LOGINFO("NotifyWebSocketUpdate - mNotifications.size()=%zu m_wsListeners.size()=%zu", 
           mNotifications.size(), m_wsListeners.size());
    
    // Always notify all Thunder JSON-RPC observers (mNotifications)
    // These are clients that called the "register" JSON-RPC method
    bool notifiedJsonRpcObservers = false;
    for (auto* observer : mNotifications) {
        if (observer) {
            LOGINFO("[EVENT] NotifyWebSocketUpdate- calling observer->OnNotifyWebSocketUpdate for URL :%s ", url.c_str());
            observer->OnNotifyWebSocketUpdate(url, message);
            notifiedJsonRpcObservers = true;
        }
    }

    // Also check for explicit WebSocket listeners (HTTP-based registration via registerWebSocketListener)
    // This is a separate mechanism from JSON-RPC observers
    {
        std::lock_guard<std::mutex> lock(m_listenerMutex);
        
        if (!m_wsListeners.empty()) {
            bool hasListeners = std::any_of(m_wsListeners.begin(), m_wsListeners.end(),
                [&url](const ListenerInfo& info) {
                    return info.url == url;
                });
            
            if (hasListeners) {
                LOGINFO("[EVENT] NotifyWebSocketUpdate- Found matching WebSocket listener for URL :%s", url.c_str());
                // Could notify explicit listeners here if needed
            } else {
                LOGINFO("[EVENT] NotifyWebSocketUpdate- No WebSocket listeners for URL :%s", url.c_str());
            }
        }
    }
    
    if (notifiedJsonRpcObservers) {
        LOGINFO("[EVENT] NotifyWebSocketUpdate- Successfully notified JSON-RPC observers for URL :%s", url.c_str());
    } else {
        LOGINFO("[EVENT] NotifyWebSocketUpdate- WARNING: No JSON-RPC observers registered for URL :%s", url.c_str());
    }
}

void RDKAMServiceImplementation::NotifyHttpUpdate(const std::string& url, uint32_t code) {
    LOGINFO("NotifyHttpUpdate url=%s code=%u", url.c_str(), code);
    
}

void RDKAMServiceImplementation::NotifySysStatusUpdate(const std::string& status) {
    LOGINFO("NotifySysStatusUpdate");
    
}

uint32_t RDKAMServiceImplementation::Configure(PluginHost::IShell* shell) {
        ASSERT(shell != nullptr);
        if (m_shell != nullptr) {
                m_shell->Release();
        }
        m_shell = shell;
        m_shell->AddRef();

        if (m_service != nullptr) {
                m_service->SetShell(m_shell);
        m_service->StartAppManagerListener(m_shell);
        }

    return Core::ERROR_NONE;
}

// Test/Debug methods - manually trigger events for testing
Core::hresult RDKAMServiceImplementation::TestTriggerWebSocketEvent(const string& url, const string& message) {
    LOGINFO("[TEST] TestTriggerWebSocketEvent url=%s message=%s", 
           url.c_str(), message.c_str());
    
    NotifyWebSocketUpdate(url, message);
    
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceImplementation::TestTriggerHttpEvent(const string& url, uint32_t code) {
    LOGINFO("[TEST] TestTriggerHttpEvent url=%s code=%u", 
           url.c_str(), code);
    
    NotifyHttpUpdate(url, code);
    
    return Core::ERROR_NONE;
}

Core::hresult RDKAMServiceImplementation::TestTriggerSysStatusEvent(const string& status) {
    LOGINFO("[TEST] TestTriggerSysStatusEvent status=%s", 
           status.c_str());
    
    NotifySysStatusUpdate(status);
    
    return Core::ERROR_NONE;
}
} // namespace Plugin
} // namespace WPEFramework


