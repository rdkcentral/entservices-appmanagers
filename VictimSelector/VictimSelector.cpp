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
    if (service == nullptr) {
        return "VictimSelector received an invalid service";
    }

    mService = service;
    mService->AddRef();
    mImplementation = mService->Root<Exchange::IVictimSelector>(mConnectionId, 5000, _T("VictimSelectorImplementation"));
    if (mImplementation == nullptr) {
        Deinitialize(service);
        return "VictimSelector implementation could not be created";
    }
    mConfigure = mImplementation->QueryInterface<Exchange::IConfiguration>();
    if (mConfigure == nullptr) {
        Deinitialize(service);
        return "VictimSelector implementation has no configuration interface";
    }
    if (mConfigure->Configure(mService) != Core::ERROR_NONE) {
        Deinitialize(service);
        return "VictimSelector could not be configured";
    }

    Exchange::JVictimSelector::Register(*this, mImplementation);
    return "";
}

void VictimSelector::Deinitialize(PluginHost::IShell* service) {
    if (mImplementation != nullptr) {
        Exchange::JVictimSelector::Unregister(*this);
        if (mConfigure != nullptr) {
            mConfigure->Release();
            mConfigure = nullptr;
        }
        RPC::IRemoteConnection* connection = service->RemoteConnection(mConnectionId);
        mImplementation->Release();
        mImplementation = nullptr;
        if (connection != nullptr) {
            connection->Terminate();
            connection->Release();
        }
    }
    mConnectionId = 0;
    if (mService != nullptr) {
        mService->Release();
        mService = nullptr;
    }
}

string VictimSelector::Information() const {
    return "Victim Selector plugin";
}

} // namespace Plugin
} // namespace WPEFramework
