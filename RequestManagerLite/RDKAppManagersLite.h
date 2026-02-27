#pragma once

#include "Module.h"
#include <interfaces/Ids.h>
#include <interfaces/IApplicationService.h>
#include <interfaces/json/JApplicationServiceRequest.h>
#include <interfaces/json/JApplicationServiceConfig.h>
#include <interfaces/json/JApplicationServiceListener.h>
#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

class RDKAppManagersLite
    : public PluginHost::IPlugin
    , public PluginHost::JSONRPC {
public:
    RDKAppManagersLite(const RDKAppManagersLite&)            = delete;
    RDKAppManagersLite& operator=(const RDKAppManagersLite&) = delete;

    RDKAppManagersLite();
    ~RDKAppManagersLite() override;


    const string Initialize(PluginHost::IShell* service) override;
    void         Deinitialize(PluginHost::IShell* service) override;
    string       Information() const override;

    BEGIN_INTERFACE_MAP(RDKAppManagersLite)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_AGGREGATE(Exchange::IApplicationServiceRequest,  _request)
        INTERFACE_AGGREGATE(Exchange::IApplicationServiceConfig,   _config)
        INTERFACE_AGGREGATE(Exchange::IApplicationServiceListener, _listener)
    END_INTERFACE_MAP

private:
    class Notification
        : public RPC::IRemoteConnection::INotification
        , public Exchange::IApplicationServiceListener::INotification {
    public:
        explicit Notification(RDKAppManagersLite* parent)
            : _parent(*parent) {
            ASSERT(parent != nullptr);
        }
        ~Notification() override = default;

        BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(Exchange::IApplicationServiceListener::INotification)
            INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
        END_INTERFACE_MAP

        void Activated(RPC::IRemoteConnection*) override {
            LOGINFO("RDKAppManagersLite remote connection Activated");
        }
        void Deactivated(RPC::IRemoteConnection* connection) override {
            LOGINFO("RDKAppManagersLite remote connection Deactivated");
            _parent.Deactivated(connection);
        }

        // IApplicationServiceListener::INotification
        void OnNotifyWebSocketUpdate(const string& url, const string& message) override;
        void OnNotifyHttpUpdate(const string& url, const uint32_t code) override;
        void OnNotifySysStatusUpdate(const string& status) override;

    private:
        RDKAppManagersLite& _parent;
    };

    void Deactivated(RPC::IRemoteConnection* connection);

    // Members
    PluginHost::IShell*                         _service      { nullptr };
    uint32_t                                    _connectionId { 0 };
    Exchange::IApplicationServiceRequest*      _request      { nullptr };
    Exchange::IApplicationServiceConfig*       _config       { nullptr };
    Exchange::IApplicationServiceListener*     _listener     { nullptr };
    Core::Sink<Notification>                    _notificationSink { this };
};

} // namespace Plugin
} // namespace WPEFramework
