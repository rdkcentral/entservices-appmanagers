#include "RDKAppManagersPlugin.h"
#include <interfaces/IConfiguration.h>

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

namespace WPEFramework {
namespace {
    static Plugin::Metadata<Plugin::RDKAppManagers> metadata(
        API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
        {}, {}, {}
    );
}
namespace Plugin {

SERVICE_REGISTRATION(RDKAppManagers, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

RDKAppManagers::RDKAppManagers()
    : _service(nullptr)
    , _connectionId(0)
    , _request(nullptr)
    , _config(nullptr)
    , _listener(nullptr)
    , _systemInfo(nullptr)
    , _systemSettings(nullptr)
    , _testPrefs(nullptr)
    , _diagnostics(nullptr)
    , _notificationSink(this) {
    SYSLOG(Logging::Startup, (string(_T("RDKAppManagers Constructor"))));
}

RDKAppManagers::~RDKAppManagers() {
    SYSLOG(Logging::Shutdown, (string(_T("RDKAppManagers Destructor"))));
}

const string RDKAppManagers::Initialize(PluginHost::IShell* service) {
    string message;

    ASSERT(service != nullptr);
    ASSERT(_service == nullptr);
    ASSERT(_connectionId == 0);

    SYSLOG(Logging::Startup, (_T("RDKAppManagers::Initialize: PID=%u"), getpid()));
    SYSLOG(Logging::Startup, (_T("[DEBUG-TRACE-01] RDKAppManagers::Initialize - START")));

    _service = service;
    _service->AddRef();
    _service->Register(&_notificationSink);

    SYSLOG(Logging::Startup, (_T("[DEBUG-TRACE-02] RDKAppManagers - Before Root<IApplicationServiceRequest>")));

    // Root a single implementation and query other interfaces from it
    _request = _service->Root<Exchange::IApplicationServiceRequest>(_connectionId, RPC::CommunicationTimeOut, _T("RDKAppManagersImplementation"));
    SYSLOG(Logging::Startup, (_T("[DEBUG-TRACE-03] RDKAppManagers - After Root<IApplicationServiceRequest>")));
    if (_request != nullptr) {

        // Configure the implementation with the shell (starts DBus thread)
        SYSLOG(Logging::Startup, (_T("[DEBUG-TRACE-03a] RDKAppManagers - Querying IConfiguration interface")));
        auto configConnection = _request->QueryInterface<Exchange::IConfiguration>();
        if (configConnection != nullptr) {
            SYSLOG(Logging::Startup, (_T("[DEBUG-TRACE-03b] RDKAppManagers - Calling Configure() on IConfiguration")));
            configConnection->Configure(service);
            SYSLOG(Logging::Startup, (_T("[DEBUG-TRACE-03c] RDKAppManagers - Configure() returned")));
            configConnection->Release();
        } else {
            SYSLOG(Logging::Error, (_T("[ERROR] RDKAppManagers - IConfiguration interface is NULL!")));
        }
        // Optionally allow implementation to initialize with the shell via a custom method if needed
        // Query other interfaces
        _config = _request->QueryInterface<Exchange::IApplicationServiceConfig>();
        _listener = _request->QueryInterface<Exchange::IApplicationServiceListener>();
        _systemInfo = _request->QueryInterface<Exchange::IApplicationServiceSystemInfo>();
        _systemSettings = _request->QueryInterface<Exchange::IApplicationServiceSystemSettings>();
        _testPrefs = _request->QueryInterface<Exchange::IApplicationServiceTestPreferences>();
        _diagnostics = _request->QueryInterface<Exchange::IApplicationServiceDiagnostics>();

        // Register JSON bindings
        Exchange::JApplicationServiceRequest::Register(*this, _request);
        if (_config) {
            Exchange::JApplicationServiceConfig::Register(*this, _config);
        }
        if (_listener) {
            _listener->Register(&_notificationSink);
            Exchange::JApplicationServiceListener::Register(*this, _listener);
        }
        if (_systemInfo) {
            Exchange::JApplicationServiceSystemInfo::Register(*this, _systemInfo);
        }
        if (_systemSettings) {
            Exchange::JApplicationServiceSystemSettings::Register(*this, _systemSettings);
        }
        if (_testPrefs) {
            Exchange::JApplicationServiceTestPreferences::Register(*this, _testPrefs);
        }
        if (_diagnostics) {
            Exchange::JApplicationServiceDiagnostics::Register(*this, _diagnostics);
        }
    } else {
        message = _T("RDKAppManagers could not be instantiated.");
    }

    if (!message.empty()) {
        Deinitialize(service);
    }

    return message;
}

void RDKAppManagers::Deinitialize(PluginHost::IShell* service) {
    ASSERT(_service == service);

    SYSLOG(Logging::Shutdown, (string(_T("RDKAppManagers Deinitialize"))));

    _service->Unregister(&_notificationSink);

    // Unregister listener notifications first
    if (_listener != nullptr) {
        // _listener->Unregister(&_notificationSink);
    }


    // Unregister JSON bindings
    Exchange::JApplicationServiceRequest::Unregister(*this);
    Exchange::JApplicationServiceConfig::Unregister(*this);
    Exchange::JApplicationServiceListener::Unregister(*this);
    Exchange::JApplicationServiceSystemInfo::Unregister(*this);
    Exchange::JApplicationServiceSystemSettings::Unregister(*this);
    Exchange::JApplicationServiceTestPreferences::Unregister(*this);
    Exchange::JApplicationServiceDiagnostics::Unregister(*this);

    RPC::IRemoteConnection* connection = service->RemoteConnection(_connectionId);

    // Release interfaces
    if (_diagnostics) { VARIABLE_IS_NOT_USED uint32_t r = _diagnostics->Release(); _diagnostics = nullptr; ASSERT(r == Core::ERROR_DESTRUCTION_SUCCEEDED); }
    if (_testPrefs) { VARIABLE_IS_NOT_USED uint32_t r = _testPrefs->Release(); _testPrefs = nullptr; ASSERT(r == Core::ERROR_DESTRUCTION_SUCCEEDED); }
    if (_systemSettings) { VARIABLE_IS_NOT_USED uint32_t r = _systemSettings->Release(); _systemSettings = nullptr; ASSERT(r == Core::ERROR_DESTRUCTION_SUCCEEDED); }
    if (_systemInfo) { VARIABLE_IS_NOT_USED uint32_t r = _systemInfo->Release(); _systemInfo = nullptr; ASSERT(r == Core::ERROR_DESTRUCTION_SUCCEEDED); }
    if (_listener) { VARIABLE_IS_NOT_USED uint32_t r = _listener->Release(); _listener = nullptr; ASSERT(r == Core::ERROR_DESTRUCTION_SUCCEEDED); }
    if (_config) { VARIABLE_IS_NOT_USED uint32_t r = _config->Release(); _config = nullptr; ASSERT(r == Core::ERROR_DESTRUCTION_SUCCEEDED); }
    if (_request) { VARIABLE_IS_NOT_USED uint32_t r = _request->Release(); _request = nullptr; ASSERT(r == Core::ERROR_DESTRUCTION_SUCCEEDED); }

    if (connection != nullptr) {
        connection->Terminate();
        connection->Release();
    }

    _connectionId = 0;

    _service->Release();
    _service = nullptr;
}

string RDKAppManagers::Information() const {
    return string();
}

void RDKAppManagers::Deactivated(RPC::IRemoteConnection* connection) {
    if (connection->Id() == _connectionId) {
        ASSERT(_service != nullptr);
        Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
    }
}

} // namespace Plugin
} // namespace WPEFramework
