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
    ASSERT(nullptr == mService);
    ASSERT(nullptr == mImplementation);
    ASSERT(nullptr == mConfigure);
    ASSERT(0 == mConnectionId);

    SYSLOG(Logging::Startup, (_T("VictimSelector::Initialize: PID=%u"), getpid()));

    string result;
    if (nullptr == service) {
        result = "VictimSelector received an invalid service";
    } else {
        mService = service;
        mService->AddRef();
        mImplementation = mService->Root<Exchange::IVictimSelector>(mConnectionId, 5000, _T("VictimSelectorImplementation"));
        if (nullptr == mImplementation) {
            result = "VictimSelector implementation could not be created";
        } else {
            mConfigure = mImplementation->QueryInterface<Exchange::IConfiguration>();
            if (nullptr == mConfigure) {
                result = "VictimSelector implementation has no configuration interface";
            } else if (Core::ERROR_NONE != mConfigure->Configure(mService)) {
                result = "VictimSelector could not be configured";
            } else {
                Exchange::JVictimSelector::Register(*this, mImplementation);
            }
        }

        if (!result.empty()) {
            Deinitialize(service);
        }
    }

    SYSLOG(Logging::Startup, (_T("VictimSelector::Initialize exit: %s"),
        result.empty() ? _T("success") : result.c_str()));
    return result;
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
