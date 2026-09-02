#include "VictimSelector.h"

const string WPEFramework::Plugin::VictimSelector::SERVICE_NAME = "org.rdk.VictimSelector";

namespace WPEFramework {
namespace Plugin {

namespace {
static Plugin::Metadata<Plugin::VictimSelector> metadata(
    1, 0, 0,
    {},
    {},
    {});
}

SERVICE_REGISTRATION(VictimSelector, 1, 0, 0);

VictimSelector::VictimSelector()
    : mService(nullptr)
    , mImplementation(nullptr)
    , mConfigure(nullptr)
    , mConnectionId(0) {
}

VictimSelector::~VictimSelector() {
}

const string VictimSelector::Initialize(PluginHost::IShell* service) {
    SYSLOG(Logging::Startup, (_T("VictimSelector::Initialize: PID=%u"), getpid()));

    if (nullptr == service) {
        return "VictimSelector received an invalid service";
    }

    mService = service;
    mService->AddRef();
    mImplementation = mService->Root<Exchange::IVictimSelector>(mConnectionId, 5000, _T("VictimSelectorImplementation"));
    if (nullptr == mImplementation) {
        Deinitialize(service);
        return "VictimSelector implementation could not be created";
    }
    mConfigure = mImplementation->QueryInterface<Exchange::IConfiguration>();
    if (nullptr == mConfigure) {
        Deinitialize(service);
        return "VictimSelector implementation has no configuration interface";
    }
    if (Core::ERROR_NONE != mConfigure->Configure(mService)) {
        Deinitialize(service);
        return "VictimSelector could not be configured";
    }

    Exchange::JVictimSelector::Register(*this, mImplementation);
    return "";
}

void VictimSelector::Deinitialize(PluginHost::IShell* service) {
    SYSLOG(Logging::Shutdown, (_T("VictimSelector::Deinitialize entry")));

    if (nullptr != mService) {
        ASSERT(mService == service);
    }
    if (nullptr != mImplementation) {
        Exchange::JVictimSelector::Unregister(*this);
        if (nullptr != mConfigure) {
            mConfigure->Release();
            mConfigure = nullptr;
        }
        RPC::IRemoteConnection* connection = (nullptr != mService)
            ? mService->RemoteConnection(mConnectionId)
            : nullptr;
        mImplementation->Release();
        mImplementation = nullptr;
        if (nullptr != connection) {
            connection->Terminate();
            connection->Release();
        }
    }
    mConnectionId = 0;
    if (nullptr != mService) {
        mService->Release();
        mService = nullptr;
    }

    SYSLOG(Logging::Shutdown, (_T("VictimSelector::Deinitialize exit")));
}

string VictimSelector::Information() const {
    return "Victim Selector plugin";
}

} // namespace Plugin
} // namespace WPEFramework
