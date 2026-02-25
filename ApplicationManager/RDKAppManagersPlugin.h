
#pragma once


#include "Module.h"
#include <interfaces/IApplicationService.h>
#include <interfaces/json/JApplicationServiceRequest.h> // For JSON-RPC registration helpers (replace with generated if available)
#include <interfaces/json/JApplicationServiceConfig.h>
#include <interfaces/json/JApplicationServiceListener.h>
#include <interfaces/json/JApplicationServiceSystemInfo.h>
#include <interfaces/json/JApplicationServiceSystemSettings.h>
#include <interfaces/json/JApplicationServiceTestPreferences.h>
#include <interfaces/json/JApplicationServiceDiagnostics.h>
#include <interfaces/json/JsonData_ApplicationServiceListener.h>
#include <tracing/Logging.h>

namespace WPEFramework {
namespace Plugin {

class RDKAppManagers : public PluginHost::IPlugin, public PluginHost::JSONRPC {
public:
    RDKAppManagers(const RDKAppManagers&) = delete;
    RDKAppManagers& operator=(const RDKAppManagers&) = delete;

    RDKAppManagers();
    ~RDKAppManagers() override;

    // IPlugin
    const string Initialize(PluginHost::IShell* service) override;
    void Deinitialize(PluginHost::IShell* service) override;
    string Information() const override;

    BEGIN_INTERFACE_MAP(RDKAppManagers)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_AGGREGATE(Exchange::IApplicationServiceRequest, _request)
        INTERFACE_AGGREGATE(Exchange::IApplicationServiceConfig, _config)
        INTERFACE_AGGREGATE(Exchange::IApplicationServiceListener, _listener)
        INTERFACE_AGGREGATE(Exchange::IApplicationServiceSystemInfo, _systemInfo)
        INTERFACE_AGGREGATE(Exchange::IApplicationServiceSystemSettings, _systemSettings)
        INTERFACE_AGGREGATE(Exchange::IApplicationServiceTestPreferences, _testPrefs)
        INTERFACE_AGGREGATE(Exchange::IApplicationServiceDiagnostics, _diagnostics)
    END_INTERFACE_MAP

private:
    class Notification : public RPC::IRemoteConnection::INotification,
                         public Exchange::IApplicationServiceListener::INotification {
    public:
        explicit Notification(RDKAppManagers* parent)
            : _parent(*parent) {
            ASSERT(parent != nullptr);
        }
        ~Notification() override = default;

        BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(Exchange::IApplicationServiceListener::INotification)
            INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
        END_INTERFACE_MAP

        void Activated(RPC::IRemoteConnection*) override {
            SYSLOG(Logging::Startup, (string(_T("RDKAppManagers Notification Activated"))));
        }
        void Deactivated(RPC::IRemoteConnection* connection) override {
            SYSLOG(Logging::Shutdown, (string(_T("RDKAppManagers Notification Deactivated"))));
            _parent.Deactivated(connection);
        }

        // Listener events
        void OnNotifyWebSocketUpdate(const string& url, const string& message) override {
            SYSLOG(Logging::Startup, (_T("OnNotifyWebSocketUpdate url=%s"), url.c_str()));
            Exchange::JApplicationServiceListener::Event::OnNotifyWebSocketUpdate(_parent, url, message);
        }
        void OnNotifyHttpUpdate(const string& url, const uint32_t code) override {
            SYSLOG(Logging::Startup, (_T("OnNotifyHttpUpdate url=%s code=%u"), url.c_str(), code));
            Exchange::JApplicationServiceListener::Event::OnNotifyHttpUpdate(_parent, url, code);
        }
        void OnNotifySysStatusUpdate(const string& status) override {
            SYSLOG(Logging::Startup, (_T("OnNotifySysStatusUpdate status=%s"), status.c_str()));
            Exchange::JApplicationServiceListener::Event::OnNotifySysStatusUpdate(_parent, status);
        }

    private:
        RDKAppManagers& _parent;
    };

    void Deactivated(RPC::IRemoteConnection* connection);

private:
    PluginHost::IShell* _service { nullptr };
    uint32_t _connectionId { 0 };

    // Root a single implementation then query for the rest
    Exchange::IApplicationServiceRequest* _request { nullptr };
    Exchange::IApplicationServiceConfig* _config { nullptr };
    Exchange::IApplicationServiceListener* _listener { nullptr };
    Exchange::IApplicationServiceSystemInfo* _systemInfo { nullptr };
    Exchange::IApplicationServiceSystemSettings* _systemSettings { nullptr };
    Exchange::IApplicationServiceTestPreferences* _testPrefs { nullptr };
    Exchange::IApplicationServiceDiagnostics* _diagnostics { nullptr };

    Core::Sink<Notification> _notificationSink;
};

} // namespace Plugin
} // namespace WPEFramework
