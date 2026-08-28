#include "VictimSelector.h"
#include "VictimSelectorImplementation.h"

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
    , mImplementation(nullptr) {
}

VictimSelector::~VictimSelector() {
}

const string VictimSelector::Initialize(PluginHost::IShell* service) {
    if (service == nullptr) {
        return "VictimSelector received an invalid service";
    }

    mService = service;
    mService->AddRef();
    auto* implementation = new VictimSelectorImplementation();
    mImplementation = implementation;
    const uint32_t status = implementation->Configure(service);
    if (status != Core::ERROR_NONE) {
        mImplementation->Release();
        mImplementation = nullptr;
        mService->Release();
        mService = nullptr;
        return "VictimSelector could not connect to AppManager";
    }

    Exchange::JVictimSelector::Register(*this, mImplementation);
    return "";
}

void VictimSelector::Deinitialize(PluginHost::IShell*) {
    if (mImplementation != nullptr) {
        Exchange::JVictimSelector::Unregister(*this);
        mImplementation->Release();
        mImplementation = nullptr;
    }
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
