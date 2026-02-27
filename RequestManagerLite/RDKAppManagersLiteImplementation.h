#pragma once

#include "Module.h"
#include <interfaces/Ids.h>
#include <interfaces/IApplicationService.h>
#include "UtilsLogging.h"

#include <algorithm>
#include <mutex>
#include <string>
#include <vector>

namespace WPEFramework {
namespace Plugin {

// RDKAppManagersLiteImplementation
//
// Single concrete class that satisfies all three RDKAppManagersLite interfaces:
//   • IApplicationServiceRequest  – HTTP-style request dispatch
//   • IApplicationServiceConfig   – static service configuration
//   • IApplicationServiceListener – listener registration + event fan-out
class RDKAppManagersLiteImplementation
    : public Exchange::IApplicationServiceRequest
    , public Exchange::IApplicationServiceConfig
    , public Exchange::IApplicationServiceListener {

public:
    RDKAppManagersLiteImplementation(const RDKAppManagersLiteImplementation&)            = delete;
    RDKAppManagersLiteImplementation& operator=(const RDKAppManagersLiteImplementation&) = delete;

    RDKAppManagersLiteImplementation();
    ~RDKAppManagersLiteImplementation() override;

    // Thunder COM interface map
    BEGIN_INTERFACE_MAP(RDKAppManagersLiteImplementation)
        INTERFACE_ENTRY(Exchange::IApplicationServiceRequest)
        INTERFACE_ENTRY(Exchange::IApplicationServiceConfig)
        INTERFACE_ENTRY(Exchange::IApplicationServiceListener)
    END_INTERFACE_MAP

    // IApplicationServiceRequest
    Core::hresult Request(
        const uint32_t flags,
        const string&  url,
        const string&  headers,
        const string&  queryParams,
        const string&  body,
        uint32_t&      code            /* @out */,
        string&        responseHeaders  /* @out */,
        string&        responseBody     /* @out */) override;

    // IApplicationServiceConfig
    Core::hresult Config(string& config /* @out */) override;

    // IApplicationServiceListener
    Core::hresult RegisterWebSocketListener(
        const string& url,
        const string& clientId,
        string&       listenerId /* @out */) override;

    Core::hresult UnregisterWebSocketListener(
        const string& listenerId) override;

    Core::hresult RegisterUpdatesListener(
        const string& url,
        const string& clientId,
        string&       listenerId /* @out */) override;

    Core::hresult UnregisterUpdatesListener(
        const string& listenerId) override;

    Core::hresult RegisterSysStatusListener(
        const string& clientId,
        string&       listenerId /* @out */) override;

    Core::hresult UnregisterSysStatusListener(
        const string& listenerId) override;

    Core::hresult Register(
        Exchange::IApplicationServiceListener::INotification* notification) override;

    Core::hresult Unregister(
        Exchange::IApplicationServiceListener::INotification* notification) override;

    // Internal helpers – call these to fire events to all registered listeners
    void NotifyWebSocketUpdate(const string& url, const string& message);
    void NotifyHttpUpdate(const string& url, uint32_t code);
    void NotifySysStatusUpdate(const string& status);

private:
    // HTTP request handlers

    // App lifecycle
    Core::hresult HandleAppsRefresh(uint32_t& code, string& responseBody);
    Core::hresult HandleAppsSubscriptionLaunch(const string& body, uint32_t& code, string& responseBody);
    Core::hresult HandleAppsSubscriptionReturn(const string& body, uint32_t& code, string& responseBody);
    Core::hresult HandleAppsLaunchRequest(const string& queryParams, uint32_t& code, string& responseBody);
    Core::hresult HandleAppsInactivePriority(bool isPost, const string& body, uint32_t& code, string& responseBody);
    Core::hresult HandleAppsTokens(uint32_t& code, string& responseBody);
    Core::hresult HandleSystemTakeFocus(uint32_t& code, string& responseBody);
    Core::hresult HandleAppsAnalyticsSubmit(const string& body, uint32_t& code, string& responseBody);
    Core::hresult HandleAppsHeartbeat(const string& body, uint32_t& code, string& responseBody);
    Core::hresult HandleAppsEnable(const string& body, uint32_t& code, string& responseBody);
    Core::hresult HandleAppsDisable(const string& body, uint32_t& code, string& responseBody);
    Core::hresult HandleAppsRefreshCert(uint32_t& code, string& responseBody);
    Core::hresult HandleAppsIntent(const string& body, uint32_t& code, string& responseBody);
    Core::hresult HandleAppsLock(const string& body, uint32_t& code, string& responseBody);
    Core::hresult HandleAppsUnlock(const string& body, uint32_t& code, string& responseBody);

    // Netflix
    Core::hresult HandleAppsNetflix(uint32_t& code, string& responseBody);
    Core::hresult HandleNetflixUpdateCookie(const string& body, uint32_t& code, string& responseBody);
    Core::hresult HandleNetflixUpdateToken(const string& body, uint32_t& code, string& responseBody);
    Core::hresult HandleNetflixRestore(uint32_t& code, string& responseBody);

    // Secure storage (token-scoped)
    Core::hresult HandleSecStorageGet(const string& queryParams, uint32_t& code, string& responseBody);
    Core::hresult HandleSecStorageSet(const string& queryParams, const string& body, uint32_t& code, string& responseBody);
    Core::hresult HandleSecStorageClear(const string& queryParams, uint32_t& code, string& responseBody);

    // Secure storage (appId-scoped)
    Core::hresult HandleSecStorageAppGet(const string& queryParams, uint32_t& code, string& responseBody);
    Core::hresult HandleSecStorageAppSet(const string& queryParams, const string& body, uint32_t& code, string& responseBody);
    Core::hresult HandleSecStorageAppClear(const string& queryParams, uint32_t& code, string& responseBody);

    // Test / debug
    Core::hresult HandleTestGetEpgToken(uint32_t& code, string& responseBody);
    Core::hresult HandleTestInactiveApps(uint32_t& code, string& responseBody);
    Core::hresult HandleTestAppLaunchDetails(const string& url, uint32_t& code, string& responseBody);
    Core::hresult HandleTestNetflixSetEsn(const string& body, uint32_t& code, string& responseBody);
    Core::hresult HandleTestNetflixSetUserLoggedIn(const string& body, uint32_t& code, string& responseBody);
    Core::hresult HandleTestTriggerSampleEvents(uint32_t& code, string& responseBody);

    // WebSocket event helpers
    void SendIntentEvent(const string& token, const string& appId);
    void SendAppUpdateEvent(const string& uuid);

    // Members
    mutable std::mutex _lock;
    uint32_t           _listenerIdCounter { 0 };

    std::vector<Exchange::IApplicationServiceListener::INotification*> _notifications;
};

} // namespace Plugin
} // namespace WPEFramework
