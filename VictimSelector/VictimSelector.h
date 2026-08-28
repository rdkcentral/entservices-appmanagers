#pragma once

#include "Module.h"
#include <interfaces/IVictimSelector.h>
#include <interfaces/IConfiguration.h>
#include <interfaces/json/JVictimSelector.h>

namespace WPEFramework {
namespace Plugin {

class VictimSelector : public PluginHost::IPlugin,
                       public PluginHost::JSONRPC {
public:
    VictimSelector(const VictimSelector&) = delete;
    VictimSelector& operator=(const VictimSelector&) = delete;

    VictimSelector();
    ~VictimSelector() override;

    BEGIN_INTERFACE_MAP(VictimSelector)
    INTERFACE_ENTRY(PluginHost::IPlugin)
    INTERFACE_ENTRY(PluginHost::IDispatcher)
    INTERFACE_AGGREGATE(Exchange::IVictimSelector, mImplementation)
    END_INTERFACE_MAP

    const string Initialize(PluginHost::IShell* service) override;
    void Deinitialize(PluginHost::IShell* service) override;
    string Information() const override;

    static const string SERVICE_NAME;

private:
    PluginHost::IShell* mService;
    Exchange::IVictimSelector* mImplementation;
    Exchange::IConfiguration* mConfigure;
    uint32_t mConnectionId;
};

} // namespace Plugin
} // namespace WPEFramework
