#include "RDKAppManagersLite.h"

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

namespace WPEFramework {
namespace {
    static Plugin::Metadata<Plugin::RDKAppManagersLite> metadata(
        API_VERSION_NUMBER_MAJOR,
        API_VERSION_NUMBER_MINOR,
        API_VERSION_NUMBER_PATCH,
        {}, {}, {}
    );
}

namespace Plugin {

SERVICE_REGISTRATION(RDKAppManagersLite,
    API_VERSION_NUMBER_MAJOR,
    API_VERSION_NUMBER_MINOR,
    API_VERSION_NUMBER_PATCH);

RDKAppManagersLite::RDKAppManagersLite()
    : _service(nullptr)
    , _connectionId(0)
    , _request(nullptr)
    , _config(nullptr)
    , _listener(nullptr)
    , _notificationSink(this) {
    LOGINFO("RDKAppManagersLite constructor");
}

RDKAppManagersLite::~RDKAppManagersLite() {
    LOGINFO("RDKAppManagersLite destructor");
}

const string RDKAppManagersLite::Initialize(PluginHost::IShell* service) {
    string message;

    ASSERT(service != nullptr);
    ASSERT(_service == nullptr);

    _service = service;
    _service->AddRef();
    _service->Register(&_notificationSink);

    LOGINFO("RDKAppManagersLite::Initialize – PID=%u", getpid());

    // Instantiate the out-of-process (or in-process) implementation
    _request = _service->Root<Exchange::IApplicationServiceRequest>(
        _connectionId,
        RPC::CommunicationTimeOut,
        _T("RDKAppManagersLiteImplementation"));

    if (_request != nullptr) {
        // Query the other two interfaces from the single root object
        _config   = _request->QueryInterface<Exchange::IApplicationServiceConfig>();
        _listener = _request->QueryInterface<Exchange::IApplicationServiceListener>();

        // Register the listener notification sink so events propagate to JSONRPC clients
        if (_listener != nullptr) {
            _listener->Register(&_notificationSink);
        }

        // Invoking Plugin API register to wpeframework (same pattern as AppManager)
        Exchange::JApplicationServiceRequest::Register(*this, _request);
        if (_config   != nullptr) Exchange::JApplicationServiceConfig::Register(*this, _config);
        if (_listener != nullptr) Exchange::JApplicationServiceListener::Register(*this, _listener);

        LOGINFO("RDKAppManagersLite interfaces acquired - config=%s listener=%s",
            _config   != nullptr ? "OK" : "NULL",
            _listener != nullptr ? "OK" : "NULL");
    } else {
        message = _T("RDKAppManagersLiteImplementation could not be instantiated.");
        LOGERR("RDKAppManagersLite::Initialize – %s", message.c_str());
    }

    if (!message.empty()) {
        Deinitialize(service);
    }

    return message;
}

void RDKAppManagersLite::Deinitialize(PluginHost::IShell* service) {
    ASSERT(_service == service);

    LOGINFO("RDKAppManagersLite::Deinitialize");

    Exchange::JApplicationServiceRequest::Unregister(*this);
    Exchange::JApplicationServiceConfig::Unregister(*this);
    Exchange::JApplicationServiceListener::Unregister(*this);

    if (_listener != nullptr) {
        _listener->Unregister(&_notificationSink);
        _listener->Release();
        _listener = nullptr;
    }
    if (_config != nullptr) {
        _config->Release();
        _config = nullptr;
    }
    if (_request != nullptr) {
        _request->Release();
        _request = nullptr;
    }

    _service->Unregister(&_notificationSink);
    _service->Release();
    _service = nullptr;
    _connectionId = 0;
}

string RDKAppManagersLite::Information() const {
    return string();
}

void RDKAppManagersLite::Deactivated(RPC::IRemoteConnection* connection) {
    if (_connectionId == connection->Id()) {
        ASSERT(_service != nullptr);
        Core::IWorkerPool::Instance().Submit(
            PluginHost::IShell::Job::Create(_service,
                PluginHost::IShell::DEACTIVATED,
                PluginHost::IShell::FAILURE));
    }
}

// Notification methods – forward events to JSONRPC subscribers
void RDKAppManagersLite::Notification::OnNotifyWebSocketUpdate(
    const string& url, const string& message) {
    LOGINFO("OnNotifyWebSocketUpdate url=%s", url.c_str());
    Exchange::JApplicationServiceListener::Event::OnNotifyWebSocketUpdate(_parent, url, message);
}

void RDKAppManagersLite::Notification::OnNotifyHttpUpdate(
    const string& url, const uint32_t code) {
    LOGINFO("OnNotifyHttpUpdate url=%s code=%u", url.c_str(), code);
    Exchange::JApplicationServiceListener::Event::OnNotifyHttpUpdate(_parent, url, code);
}

void RDKAppManagersLite::Notification::OnNotifySysStatusUpdate(const string& status) {
    LOGINFO("OnNotifySysStatusUpdate status=%s", status.c_str());
    Exchange::JApplicationServiceListener::Event::OnNotifySysStatusUpdate(_parent, status);
}

} // namespace Plugin
} // namespace WPEFramework
