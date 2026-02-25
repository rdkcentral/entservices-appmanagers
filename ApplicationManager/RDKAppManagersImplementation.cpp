#include "RDKAppManagersImplementation.h"
#include "RDKAppManagersServiceUtils.h"
#include "SystemSettings.h"
#include "TestPreferences.h"
//#include "NetworkService.h"
//#include "Controllers/inc/NetworkController.h"
//#include "sky/log.h"
//#include "sky/eventloop.h"
//#include "dbusconnection.h"
#include <vector>
#include <map>
#include <cctype>

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

SERVICE_REGISTRATION(RDKAppManagersImplementation, 1, 0, 0);

namespace WPEFramework {
namespace Plugin {

RDKAppManagersImplementation::RDKAppManagersImplementation() 
    : m_listenerIdCounter(0)
    , m_shell(nullptr)
    , m_service(std::make_unique<RDKAppManagersService>(nullptr))
    , m_legacyEventLoop(nullptr)
    , m_running(false)
{
    SYSLOG(Logging::Startup, (string(_T("RDKAppManagersImplementation Constructor - START"))));
  /*  
    // Create NetworkService in Thunder mode (nullptr for DBusConnection)
    m_networkService = std::make_unique<NetworkService>(nullptr);
    
    // Create Thunder handlers
    m_thunderHandlers = std::make_unique<ASThunder::Thunder::NetworkServiceThunderHandlers>(
        m_networkService.get()
    );
    */
    SYSLOG(Logging::Startup, (string(_T("RDKAppManagersImplementation Constructor - END"))));
}
RDKAppManagersImplementation::~RDKAppManagersImplementation() {
    SYSLOG(Logging::Shutdown, (string(_T("RDKAppManagersImplementation Destructor"))));
   /* 
    // Stop legacy thread
    m_legacyLock.Lock();
    if (m_running && m_legacyEventLoop) {
        m_legacyEventLoop->quit(1);
    }
    m_legacyLock.Unlock();
    
    if (m_legacyThread.joinable()) {
        m_legacyThread.detach();
      //  LOG_MIL("RDKAppmanagersImplementation: Destructor thread.detach() returned");

    }
    */
    // Cleanup resources
    if (m_shell != nullptr) {
        m_shell->Release();
        m_shell = nullptr;
    }
    
    SYSLOG(Logging::Shutdown, (string(_T("RDKAppManagersImplementation Destroyed"))));
}

// Request
Core::hresult RDKAppManagersImplementation::Request(const uint32_t flags, const string& url, const string& headers,
                                               const string& queryParams, const string& body,
                                               uint32_t& code, string& responseHeaders, string& responseBody) {
    SYSLOG(Logging::Startup, (_T("RDKAppManagersImplementation::Request - flags=%u, url=%s"), 
        flags, url.c_str()));
    SYSLOG(Logging::Startup, (_T("  headers='%s'"), headers.c_str()));
    SYSLOG(Logging::Startup, (_T("  queryParams='%s' (length=%zu)"), queryParams.c_str(), queryParams.length()));
    SYSLOG(Logging::Startup, (_T("  body='%s'"), body.c_str()));

    const std::pair<std::string, std::string> urlInfo = RDKAppManagersServiceUtils::NormalizeUrlAndExtractQuery(url, queryParams);
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
        responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("Service shell is not configured");
        SYSLOG(Logging::Error, (_T("Request failed: shell/service is not configured")));
        return Core::ERROR_NONE;
    }

    const RDKAppManagersServiceUtils::RequestContext context = RDKAppManagersServiceUtils::BuildRequestContext(normalizedUrl, effectiveQueryParams, body);

    Core::hresult status = Core::ERROR_NONE;
    code = 500;

    static const std::map<std::string, RDKAppManagersServiceUtils::RouteEntry> requestMap = RDKAppManagersServiceUtils::LoadRequestMap();

    if (!isMethodAllowedForUrl(normalizedUrl)) {
        code = 405;
        responseBody = RDKAppManagersServiceUtils::BuildErrorResponse("Method not allowed");
        SYSLOG(Logging::Error, (_T("Method not allowed for url=%s normalized=%s flags=%u"), url.c_str(), normalizedUrl.c_str(), flags));
        return Core::ERROR_NONE;
    }

    const auto routeIt = requestMap.find(normalizedUrl);
    if (routeIt != requestMap.end()) {
        status = m_service->DispatchMappedRequest(routeIt->second.methods, routeIt->second.params, context.runtimeParams, code, responseBody);
        RDKAppManagersServiceUtils::EnsureErrorResponse(status, normalizedUrl, responseBody);

        SYSLOG(Logging::Startup, (_T("Request executed url=%s normalized=%s methodCount=%zu status=%u code=%u"),
            url.c_str(), normalizedUrl.c_str(), routeIt->second.methods.size(), status, code));

        return Core::ERROR_NONE;
    }

    if (normalizedUrl == "/as/apps/action/launch") {
        status = m_service->LaunchAppRequest(context.appId, context.token, context.mode, context.intent, context.launchArgs, code, responseBody);
    } else if (normalizedUrl == "/as/apps/action/reset") {
        status = m_service->ResetAppDataRequest(context.appId, code, responseBody);
    } else if (normalizedUrl == "/as/system/stats") {
        status = m_service->GetSystemStatsRequest(code, responseBody);
    } else {
        code = 404;
        responseBody = std::string("{\"success\":false,\"error\":\"Unsupported URL\",\"url\":\"") + RDKAppManagersServiceUtils::EscapeJson(url) +
                       "\",\"normalizedUrl\":\"" + RDKAppManagersServiceUtils::EscapeJson(normalizedUrl) + "\"}";
        SYSLOG(Logging::Error, (_T("Request mapping not found for url=%s normalized=%s"), url.c_str(), normalizedUrl.c_str()));
        return Core::ERROR_NONE;
    }

    RDKAppManagersServiceUtils::EnsureErrorResponse(status, normalizedUrl, responseBody);

    SYSLOG(Logging::Startup, (_T("Request executed by explicit handler url=%s normalized=%s status=%u code=%u"),
        url.c_str(), normalizedUrl.c_str(), status, code));

    return Core::ERROR_NONE;
    // return m_thunderHandlers->request(flags,url,headers,queryParams,body,code,responseHeaders,responseBody);
}

// Config
Core::hresult RDKAppManagersImplementation::Config(string& config) {
    SYSLOG(Logging::Startup, (string(_T("RDKAppManagersImplementation::Config requestt"))));

    /*
    config = R"JSON(
{
  "providesSystemStatus" : true,
  "domain": "/network",
  "uris": [
    {
      "path": "/as/network/wireless/scan/status",
      "method": "ws",
      "thread": "AS_WS_NTW_SCAN"
    },
    {
      "path": "/as/network/status",
      "method": "ws",
      "thread": "AS_WS_NTW"
    },
    {
      "path": "/as/network/action/enable",
      "method": "post",
      "expectsBody": false
    },
    {
      "path": "/as/network/action/disable",
      "method": "post",
      "expectsBody": false
    },
    {
      "path": "/as/network/action/startautoconnect",
      "method": "post",
      "expectsBody": false
    },
    {
      "path": "/as/network/action/cancelautoconnect",
      "method": "post",
      "expectsBody": false
    },
    {
      "path": "/as/network/action/connect",
      "method": "post",
      "expectsBody": false
    },
    {
      "path": "/as/network/wireless/action/scanstart",
      "method": "post",
      "expectsBody": false
    },
    {
      "path": "/as/network/wireless/action/scanstop",
      "method": "post",
      "expectsBody": false
    },
    {
      "path": "/as/network/stats",
      "method": "get",
      "expectsBody": false
    },
    {
      "path": "/as/network/action/reset",
      "method": "post",
      "expectsBody": false
    },
    {
      "path": "/as/network/action/startlnfconnect",
      "method": "post",
      "expectsBody": false
    },
    {
      "path": "/as/network/action/cancellnfconnect",
      "method": "post",
      "expectsBody": false
    },
    {
      "path": "/as/network/ipconfig/settings",
      "method": "get",
      "expectsBody": false
    },
    {
      "path": "/as/network/ipconfig/settings",
      "method": "post",
      "expectsBody": true
    },
    {
      "path": "PingUrl",
      "method": "dbusmethod",
      "expectedInputArgs": "sus",
      "expectedOutputArgs": "us"
    },
    {
      "path": "PingRouter",
      "method": "dbusmethod",
      "expectedInputArgs": "s",
      "expectedOutputArgs": "us"
    },
    {
      "path": "GetSTBIpAddress",
      "method": "dbusmethod",
      "expectedOutputArgs": "us"
    },
    {
      "path": "GetActiveConnectedInterface",
      "method": "dbusmethod",
      "expectedOutputArgs": "us"
    },
    {
      "path": "MoveToLowPowerMode",
      "method": "dbusmethod",
      "expectedOutputArgs": "us"
    },
    {
      "path": "RestoreFromLowPowerMode",
      "method": "dbusmethod",
      "expectedOutputArgs": "us"
    },
    {
      "path": "MoveToActiveStandbyMode",
      "method": "dbusmethod",
      "expectedOutputArgs": "us"
    },
    {
      "path": "RestoreFromActiveStandbyMode",
      "method": "dbusmethod",
      "expectedOutputArgs": "us"
    }
  ]
}
)JSON";
    */
    return Core::ERROR_NONE;
}

// Listener registrations
Core::hresult RDKAppManagersImplementation::RegisterWebSocketListener(const string& url, const string& clientId, string& listenerId) {
    SYSLOG(Logging::Startup, (_T("RegisterWebSocketListener url=%s clientId=%s"), url.c_str(), clientId.c_str()));
    /*
    std::lock_guard<std::mutex> lock(m_listenerMutex);
    
    // Generate unique listener ID
    listenerId = "ws-" + std::to_string(++m_listenerIdCounter);
    
    // Store listener registration in-memory
    ListenerInfo info;
    info.listenerId = listenerId;
    info.url = url;
    info.clientId = clientId;
    
    m_wsListeners.push_back(info);
    
    SYSLOG(Logging::Startup, (_T("RegisterWebSocketListener success: listenerId=%s, total=%zu"), 
           listenerId.c_str(), m_wsListeners.size()));
    */
    return Core::ERROR_NONE;
}
Core::hresult RDKAppManagersImplementation::UnregisterWebSocketListener(const string& listenerId) {
    SYSLOG(Logging::Startup, (_T("UnregisterWebSocketListener id=%s"), listenerId.c_str()));
    /*
    std::lock_guard<std::mutex> lock(m_listenerMutex);
    
    auto it = std::find_if(m_wsListeners.begin(), m_wsListeners.end(),
        [&listenerId](const ListenerInfo& info) {
            return info.listenerId == listenerId;
        });
    
    if (it == m_wsListeners.end()) {
        return Core::ERROR_UNKNOWN_KEY;
    }
    
    m_wsListeners.erase(it);
    
    SYSLOG(Logging::Startup, (_T("UnregisterWebSocketListener success, remaining=%zu"), 
           m_wsListeners.size()));
    */
    return Core::ERROR_NONE;
}
Core::hresult RDKAppManagersImplementation::RegisterUpdatesListener(const string& url, const string& clientId, string& listenerId) {
    SYSLOG(Logging::Startup, (_T("RegisterUpdatesListener url=%s clientId=%s"), url.c_str(), clientId.c_str()));
    /*
    std::lock_guard<std::mutex> lock(m_listenerMutex);
    
    // Generate unique listener ID
    listenerId = "http-" + std::to_string(++m_listenerIdCounter);
    
    // Store listener registration in-memory
    ListenerInfo info;
    info.listenerId = listenerId;
    info.url = url;
    info.clientId = clientId;
    
    m_httpListeners.push_back(info);
    
    SYSLOG(Logging::Startup, (_T("RegisterUpdatesListener success: listenerId=%s, total=%zu"), 
           listenerId.c_str(), m_httpListeners.size()));
    */
    return Core::ERROR_NONE;
}
Core::hresult RDKAppManagersImplementation::UnregisterUpdatesListener(const string& listenerId) {
    SYSLOG(Logging::Startup, (_T("UnregisterUpdatesListener id=%s"), listenerId.c_str()));
    /*
    std::lock_guard<std::mutex> lock(m_listenerMutex);
    
    auto it = std::find_if(m_httpListeners.begin(), m_httpListeners.end(),
        [&listenerId](const ListenerInfo& info) {
            return info.listenerId == listenerId;
        });
    
    if (it == m_httpListeners.end()) {
        return Core::ERROR_UNKNOWN_KEY;
    }
    
    m_httpListeners.erase(it);
    
    SYSLOG(Logging::Startup, (_T("UnregisterUpdatesListener success, remaining=%zu"), 
           m_httpListeners.size()));
    */
    return Core::ERROR_NONE;
}
Core::hresult RDKAppManagersImplementation::RegisterSysStatusListener(const string& clientId, string& listenerId) {
    SYSLOG(Logging::Startup, (_T("RegisterSysStatusListener clientId=%s"), clientId.c_str()));
    
    /*{
        std::lock_guard<std::mutex> lock(m_listenerMutex);
        
        // Generate unique listener ID
        listenerId = "sys-" + std::to_string(++m_listenerIdCounter);
        
        // Store listener registration in-memory
        ListenerInfo info;
        info.listenerId = listenerId;
        info.url = "";  // SysStatus doesn't use URL
        info.clientId = clientId;
        
        m_sysListeners.push_back(info);
        
        SYSLOG(Logging::Startup, (_T("RegisterSysStatusListener success: listenerId=%s, total=%zu"), 
               listenerId.c_str(), m_sysListeners.size()));
        
        // Send cached status to new listener immediately (mirrors D-Bus behavior)
        if (!m_cachedSysStatus.empty()) {
            SYSLOG(Logging::Startup, (_T("RegisterSysStatusListener - Sending cached status to new listener: %s"), 
                   m_cachedSysStatus.c_str()));
        }
    }
    
    // Send cached status immediately if available (outside lock to avoid deadlock)
    if (!m_cachedSysStatus.empty()) {
        NotifySysStatusUpdate(m_cachedSysStatus);
    }
    */
    return Core::ERROR_NONE;
}
Core::hresult RDKAppManagersImplementation::UnregisterSysStatusListener(const string& listenerId)
{
    SYSLOG(Logging::Startup, (_T("UnregisterSysStatusListener id=%s"), listenerId.c_str()));
    /*
    std::lock_guard<std::mutex> lock(m_listenerMutex);
    
    auto it = std::find_if(m_sysListeners.begin(), m_sysListeners.end(),
        [&listenerId](const ListenerInfo& info) {
            return info.listenerId == listenerId;
        });
    
    if (it == m_sysListeners.end()) {
        return Core::ERROR_UNKNOWN_KEY;
    }
    
    m_sysListeners.erase(it);
    
    SYSLOG(Logging::Startup, (_T("UnregisterSysStatusListener success, remaining=%zu"), 
           m_sysListeners.size()));
   */ 
    return Core::ERROR_NONE;
}
// Listener observer management
Core::hresult RDKAppManagersImplementation::Register(IApplicationServiceListener::INotification* notification) {
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
Core::hresult RDKAppManagersImplementation::Unregister(IApplicationServiceListener::INotification* notification) {
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
Core::hresult RDKAppManagersImplementation::GetSystemInfo(string& info) {
    SYSLOG(Logging::Startup, (string(_T("GetSystemInfo"))));
    info = "{}";
    return Core::ERROR_NONE;
}

// SystemSettings
Core::hresult RDKAppManagersImplementation::GetSystemSetting(const string& name, string& value) {
    SYSLOG(Logging::Startup, (_T("GetSystemSetting name=%s"), name.c_str()));

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
Core::hresult RDKAppManagersImplementation::SetSystemSetting(const string& name, const string& value) {
    SYSLOG(Logging::Startup, (_T("SetSystemSetting name=%s"), name.c_str()));

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
Core::hresult RDKAppManagersImplementation::GetTestPreference(const string& name, string& value) {
    SYSLOG(Logging::Startup, (_T("GetTestPreference name=%s"), name.c_str()));

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
Core::hresult RDKAppManagersImplementation::SetTestPreference(const string& name, const string& value, const uint32_t pin) {
    SYSLOG(Logging::Startup, (_T("SetTestPreference name=%s"), name.c_str()));

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

Core::hresult RDKAppManagersImplementation::GetDiagContexts(string& contexts) {
    SYSLOG(Logging::Startup, (string(_T("GetDiagContexts"))));
    return Core::ERROR_NONE;
//    return m_thunderHandlers->getDiagContexts(contexts);
}
Core::hresult RDKAppManagersImplementation::SetDiagContexts(const string& contexts, uint32_t& updated) {
    SYSLOG(Logging::Startup, (_T("SetDiagContexts ctx.len=%zu"), contexts.size()));
    return Core::ERROR_NONE;
    //return m_thunderHandlers->setDiagContexts(contexts, updated);
}


// Notification methods called by NetworkController via callback
void RDKAppManagersImplementation::NotifyWebSocketUpdate(const std::string& url, const std::string& message) {
    SYSLOG(Logging::Startup, (_T("NotifyWebSocketUpdate url=%s"), url.c_str()));
    
    // Check if any Thunder clients are listening to this URL
    {
        std::lock_guard<std::mutex> lock(m_listenerMutex);
        
        bool hasListeners = std::any_of(m_wsListeners.begin(), m_wsListeners.end(),
            [&url](const ListenerInfo& info) {
                return info.url == url;
            });
        
        if (!hasListeners) {
            SYSLOG(Logging::Startup, (_T("[EVENT] NotifyWebSocketUpdate- NO listeners for URL :%s - SKIPPING"), url.c_str()));
            // No one is listening, skip notification
            return;
        }
    }
    
    // Notify all Thunder observers (ASNetwork plugin notification sinks)
    for (auto* observer : mNotifications) {
        if (observer) {
            SYSLOG(Logging::Startup, (_T("[EVENT] NotifyWebSocketUpdate- listeners for URL :%s "), url.c_str()));
            observer->OnNotifyWebSocketUpdate(url, message);
        }
    }
}

void RDKAppManagersImplementation::NotifyHttpUpdate(const std::string& url, uint32_t code) {
    SYSLOG(Logging::Startup, (_T("NotifyHttpUpdate url=%s code=%u"), url.c_str(), code));
    
    {
        std::lock_guard<std::mutex> lock(m_listenerMutex);
        
        bool hasListeners = std::any_of(m_httpListeners.begin(), m_httpListeners.end(),
            [&url](const ListenerInfo& info) {
                return info.url == url;
            });
        
        if (!hasListeners) {
            SYSLOG(Logging::Startup, (_T("[EVENT] NotifyHttpUpdate- NO listeners for URL :%s - SKIPPING"), url.c_str()));
            return;
        }
    }

    // FIXME: Dispatch job instead
    for (auto* observer : mNotifications) {
        if (observer) {
            SYSLOG(Logging::Startup, (_T("[EVENT] NotifyHttpUpdate- listeners for URL :%s "), url.c_str()));
            observer->OnNotifyHttpUpdate(url, code);
        }
    }
}

void RDKAppManagersImplementation::NotifySysStatusUpdate(const std::string& status) {
    SYSLOG(Logging::Startup, (_T("NotifySysStatusUpdate")));
    
    {
        std::lock_guard<std::mutex> lock(m_listenerMutex);
        
        // Cache the status for new listeners (mirrors D-Bus implementation)
        m_cachedSysStatus = status;
        SYSLOG(Logging::Startup, (_T("NotifySysStatusUpdate - Cached status: %s"), status.c_str()));
        
        if (m_sysListeners.empty()) {
            SYSLOG(Logging::Startup, (_T("[EVENT] NotifySysStatusUpdate- NO listeners for URL :%s - SKIPPING"), status.c_str()));
            return;
        }
    }
    
    // FIXME: Dispatch job instead
    for (auto* observer : mNotifications) {
        if (observer) {
            SYSLOG(Logging::Startup, (_T("[EVENT] NotifySysStatusUpdate- listeners for URL :%s "), status.c_str()));
            observer->OnNotifySysStatusUpdate(status);
        }
    }
}


uint32_t RDKAppManagersImplementation::Configure(PluginHost::IShell* shell) {
        ASSERT(shell != nullptr);
        if (m_shell != nullptr) {
                m_shell->Release();
        }
        m_shell = shell;
        m_shell->AddRef();

        if (m_service != nullptr) {
                m_service->SetShell(m_shell);
        }

    /*

    // Start legacy D-Bus thread for service-to-service communication
    m_running = true;
    m_legacyThread = std::thread(&ASNetworkImplementation::LegacyMain, this);

    // Register callbacks with NetworkController
    auto controller = m_networkService->getNetworkController();
    if (controller) {
        controller->SetThunderNotificationCallback(
            [this](const std::string& url, const std::string& message) {
                this->NotifyWebSocketUpdate(url, message);
            }
        );

        controller->SetThunderHttpCallback(
            [this](const std::string& url, uint32_t code) {
                this->NotifyHttpUpdate(url, code);
            }
        );
        
        controller->SetThunderSysStatusCallback(
            [this](const std::string& status) {
                this->NotifySysStatusUpdate(status);
            }
        );
    }
*/
    return Core::ERROR_NONE;
}

// Test/Debug methods - manually trigger events for testing
Core::hresult RDKAppManagersImplementation::TestTriggerWebSocketEvent(const string& url, const string& message) {
    SYSLOG(Logging::Startup, (_T("[TEST] TestTriggerWebSocketEvent url=%s message=%s"), 
           url.c_str(), message.c_str()));
    
    // Directly call the notification method to simulate NetworkController callback
    NotifyWebSocketUpdate(url, message);
    
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersImplementation::TestTriggerHttpEvent(const string& url, uint32_t code) {
    SYSLOG(Logging::Startup, (_T("[TEST] TestTriggerHttpEvent url=%s code=%u"), 
           url.c_str(), code));
    
    NotifyHttpUpdate(url, code);
    
    return Core::ERROR_NONE;
}

Core::hresult RDKAppManagersImplementation::TestTriggerSysStatusEvent(const string& status) {
    SYSLOG(Logging::Startup, (_T("[TEST] TestTriggerSysStatusEvent status=%s"), 
           status.c_str()));
    
    NotifySysStatusUpdate(status);
    
    return Core::ERROR_NONE;
}
/*
// LegacyMain - Runs D-Bus event loop in separate thread
void RDKAppManagersImplementation::LegacyMain() {
    LOG_MIL("ASNetworkImplementation: Starting Legacy D-Bus Main Loop");

    // 1. Create Event Loop
    EventLoop eventLoop;

    // Store pointer so we can quit it from destructor
    m_legacyLock.Lock();
    m_legacyEventLoop = &eventLoop;
    m_legacyLock.Unlock();

    // 2. Connect to System Bus
    DBusConnection dbusConn = DBusConnection::systemBus(eventLoop);

    if (!dbusConn.isConnected()) {
        LOG_FATAL("ASNetworkImplementation: failed to connect to system bus");
        return;
    }

    // 3. Register Service Name
    if (!dbusConn.registerName("com.sky.as.network")) {
        LOG_FATAL("ASNetworkImplementation: failed to acquire service name 'com.sky.as.network'");
        return;
    }

    // 4. Instantiate the D-Bus NetworkService
    //NetworkService dbusService(&dbusConn);

    SYSLOG("ASNetworkImplementation: D-Bus NetworkService instance created, entering exec()");

    // 5. Run Event Loop
    //eventLoop.exec();

    LOG_MIL("ASNetworkImplementation: Legacy D-Bus Main Loop exited");

    // Cleanup pointer
    m_legacyLock.Lock();
    m_legacyEventLoop = nullptr;
    m_legacyLock.Unlock();
}
*/
} // namespace Plugin
} // namespace WPEFramework
