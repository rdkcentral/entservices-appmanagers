
#pragma once

#include "Module.h"
#include <interfaces/IApplicationService.h>
#include <interfaces/IConfiguration.h>
#include "tracing/Logging.h"
#include "UtilsLogging.h"
#include "RDKAppManagersService.h"
#include <mutex>
#include <vector>
#include <algorithm>
#include <thread>
#include <memory>

// Forward declarations
//class NetworkService;
class EventLoop;
/*
namespace ASThunder {
namespace Thunder {
    class NetworkServiceThunderHandlers;
}
}
*/
namespace WPEFramework {
namespace Plugin {

// One class implements all ApplicationService interfaces
class RDKAppManagersImplementation
    : public Exchange::IApplicationServiceRequest
    , public Exchange::IApplicationServiceConfig
    , public Exchange::IApplicationServiceListener
    , public Exchange::IApplicationServiceSystemInfo
    , public Exchange::IApplicationServiceSystemSettings
    , public Exchange::IApplicationServiceTestPreferences
    , public Exchange::IApplicationServiceDiagnostics
    , public Exchange::IConfiguration {
public:
    RDKAppManagersImplementation(const RDKAppManagersImplementation&) = delete;
    RDKAppManagersImplementation& operator=(const RDKAppManagersImplementation&) = delete;

    RDKAppManagersImplementation();
    ~RDKAppManagersImplementation() override;

    BEGIN_INTERFACE_MAP(RDKAppManagersImplementation)
        INTERFACE_ENTRY(Exchange::IApplicationServiceRequest)
        INTERFACE_ENTRY(Exchange::IApplicationServiceConfig)
        INTERFACE_ENTRY(Exchange::IApplicationServiceListener)
        INTERFACE_ENTRY(Exchange::IApplicationServiceSystemInfo)
        INTERFACE_ENTRY(Exchange::IApplicationServiceSystemSettings)
        INTERFACE_ENTRY(Exchange::IApplicationServiceTestPreferences)
        INTERFACE_ENTRY(Exchange::IApplicationServiceDiagnostics)
        INTERFACE_ENTRY(Exchange::IConfiguration)
    END_INTERFACE_MAP

    // IApplicationServiceRequest
    Core::hresult Request(const uint32_t flags, const string& url, const string& headers,
                          const string& queryParams, const string& body,
                          uint32_t& code, string& responseHeaders, string& responseBody) override;

    // IApplicationServiceConfig
    Core::hresult Config(string& config) override;

    // IApplicationServiceListener
    Core::hresult RegisterWebSocketListener(const string& url, const string& clientId, string& listenerId) override;
    Core::hresult UnregisterWebSocketListener(const string& listenerId) override;
    Core::hresult RegisterUpdatesListener(const string& url, const string& clientId, string& listenerId) override;
    Core::hresult UnregisterUpdatesListener(const string& listenerId) override;
    Core::hresult RegisterSysStatusListener(const string& clientId, string& listenerId) override;
    Core::hresult UnregisterSysStatusListener(const string& listenerId) override;

    Core::hresult Register(IApplicationServiceListener::INotification* notification) override;
    Core::hresult Unregister(IApplicationServiceListener::INotification* notification) override;

    // Called by NetworkController via callback when events occur
    void NotifyWebSocketUpdate(const std::string& url, const std::string& message);
    void NotifyHttpUpdate(const std::string& url, uint32_t code);
    void NotifySysStatusUpdate(const std::string& status);

    // IApplicationServiceSystemInfo
    Core::hresult GetSystemInfo(string& info) override;

    // IApplicationServiceSystemSettings
    Core::hresult GetSystemSetting(const string& name, string& value) override;
    Core::hresult SetSystemSetting(const string& name, const string& value) override;

    // IApplicationServiceTestPreferences
    Core::hresult GetTestPreference(const string& name, string& value) override;
    Core::hresult SetTestPreference(const string& name, const string& value, const uint32_t pin) override;

    // IApplicationServiceDiagnostics
    Core::hresult GetDiagContexts(string& contexts) override;
    Core::hresult SetDiagContexts(const string& contexts, uint32_t& updated) override;

    // Test/Debug methods - manually trigger events for testing
    Core::hresult TestTriggerWebSocketEvent(const string& url, const string& message);
    Core::hresult TestTriggerHttpEvent(const string& url, uint32_t code);
    Core::hresult TestTriggerSysStatusEvent(const string& status);

    // IConfiguration
    uint32_t Configure(PluginHost::IShell* shell) override;

private:
    // Legacy D-Bus thread for service-to-service communication
    void LegacyMain();
    
    std::mutex mNotificationMutex;
    std::list<Exchange::IApplicationServiceListener::INotification*> mNotifications;
    
    // Listener registrations (in-memory, no D-Bus)
    struct ListenerInfo {
        std::string listenerId;
        std::string url;
        std::string clientId;
    };
    
    std::vector<ListenerInfo> m_wsListeners;      // WebSocket listeners
    std::vector<ListenerInfo> m_httpListeners;    // HTTP update listeners
    std::vector<ListenerInfo> m_sysListeners;     // System status listeners
    
    uint32_t m_listenerIdCounter = 0;
    mutable std::mutex m_listenerMutex;
    
    // Cache for system status (mirrors D-Bus implementation behavior)
    std::string m_cachedSysStatus;
/*
    std::unique_ptr<NetworkService> m_networkService;
    std::unique_ptr<ASThunder::Thunder::NetworkServiceThunderHandlers> m_thunderHandlers;
*/
    // Thunder plugin shell
    PluginHost::IShell* m_shell;
    std::unique_ptr<RDKAppManagersService> m_service;

    // Legacy D-Bus thread management
    std::thread m_legacyThread;
    EventLoop* m_legacyEventLoop;
    Core::CriticalSection m_legacyLock;
    bool m_running;
};

} // namespace Plugin
} // namespace WPEFramework
