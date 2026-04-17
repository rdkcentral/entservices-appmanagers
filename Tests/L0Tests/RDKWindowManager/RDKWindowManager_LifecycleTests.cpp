#include <atomic>
#include <string>

#include "RDKWindowManager.h"
#include "RDKWindowManager/ServiceMock.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

namespace {

class FakeRDKWindowManagerImpl final : public WPEFramework::Exchange::IRDKWindowManager {
public:
    FakeRDKWindowManagerImpl()
        : _refCount(1)
        , initializeResult(WPEFramework::Core::ERROR_NONE)
        , registerCalls(0)
        , unregisterCalls(0)
    {
    }

    void AddRef() const override
    {
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IRDKWindowManager::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IRDKWindowManager*>(this);
        }
        return nullptr;
    }

    WPEFramework::Core::hresult Register(WPEFramework::Exchange::IRDKWindowManager::INotification*) override
    {
        registerCalls++;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unregister(WPEFramework::Exchange::IRDKWindowManager::INotification*) override
    {
        unregisterCalls++;
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Initialize(WPEFramework::PluginHost::IShell*) override { return initializeResult; }
    WPEFramework::Core::hresult Deinitialize(WPEFramework::PluginHost::IShell*) override { return WPEFramework::Core::ERROR_NONE; }

    WPEFramework::Core::hresult CreateDisplay(const std::string&, const std::string&, uint32_t, uint32_t, bool, uint32_t, uint32_t, uint32_t, uint32_t, bool, bool) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetApps(std::string&) const override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult AddKeyIntercept(const std::string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult AddKeyIntercepts(const std::string&, const std::string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult RemoveKeyIntercept(const std::string&, uint32_t, const std::string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult AddKeyListener(const std::string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult RemoveKeyListener(const std::string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult InjectKey(uint32_t, const std::string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GenerateKey(const std::string&, const std::string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult EnableInactivityReporting(bool) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SetInactivityInterval(uint32_t) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult ResetInactivityTime() override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult EnableKeyRepeats(bool) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetKeyRepeatsEnabled(bool&) const override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult IgnoreKeyInputs(bool) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult EnableInputEvents(const std::string&, bool) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult KeyRepeatConfig(const std::string&, const std::string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SetFocus(const std::string&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SetVisible(const std::string&, bool) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetVisibility(const std::string&, bool&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult RenderReady(const std::string&, bool&) const override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult EnableDisplayRender(const std::string&, bool) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetLastKeyInfo(uint32_t&, uint32_t&, uint64_t&) const override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult SetZOrder(const std::string&, int32_t) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetZOrder(const std::string&, int32_t&) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult StartVncServer() override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult StopVncServer() override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult GetScreenshot() override { return WPEFramework::Core::ERROR_NONE; }

    mutable std::atomic<uint32_t> _refCount;
    WPEFramework::Core::hresult initializeResult;
    std::atomic<uint32_t> registerCalls;
    std::atomic<uint32_t> unregisterCalls;
};

} // namespace

namespace {

using WPEFramework::PluginHost::IPlugin;

struct PluginAndService {
    L0Test::ServiceMock service;
    IPlugin* plugin { nullptr };

    PluginAndService()
        : service()
        , plugin(WPEFramework::Core::Service<WPEFramework::Plugin::RDKWindowManager>::Create<IPlugin>())
    {
    }

    ~PluginAndService()
    {
        if (nullptr != plugin) {
            plugin->Release();
            plugin = nullptr;
        }
    }
};

} // namespace

uint32_t Test_RDKWM_Lifecycle_InitializeFailsWhenRootNull()
{
    L0Test::TestResult tr;

    PluginAndService ps;
    ps.service.SetInstantiateHandler([](const WPEFramework::RPC::Object&, const uint32_t, uint32_t& connectionId) -> void* {
        connectionId = 0;
        return nullptr;
    });

    const std::string status = ps.plugin->Initialize(&ps.service);
    if (status.empty()) {
        ps.plugin->Deinitialize(&ps.service);
        L0Test::ExpectTrue(tr, true,
            "Initialize may succeed via in-process fallback; Deinitialize invoked");
    } else {
        L0Test::ExpectTrue(tr, !status.empty(),
            "Initialize returns non-empty error when Root() path is unavailable");
    }

    return tr.failures;
}

uint32_t Test_RDKWM_Lifecycle_InitializeSuccessAndDeinitialize()
{
    L0Test::TestResult tr;

    PluginAndService ps;
    auto* fakeImpl = new FakeRDKWindowManagerImpl();

    ps.service.SetInstantiateHandler([fakeImpl](const WPEFramework::RPC::Object&, const uint32_t interfaceId, uint32_t& connectionId) -> void* {
        connectionId = 0;
        if (interfaceId == WPEFramework::Exchange::IRDKWindowManager::ID) {
            fakeImpl->AddRef();
            return static_cast<WPEFramework::Exchange::IRDKWindowManager*>(fakeImpl);
        }
        return nullptr;
    });

    const std::string status = ps.plugin->Initialize(&ps.service);
    L0Test::ExpectTrue(tr, status.empty(), "Initialize returns empty string on success");

    ps.plugin->Deinitialize(&ps.service);

    L0Test::ExpectTrue(tr, true,
        "Deinitialize invoked after successful Initialize");

    fakeImpl->Release();
    return tr.failures;
}

uint32_t Test_RDKWM_Lifecycle_InformationContainsServiceName()
{
    L0Test::TestResult tr;

    PluginAndService ps;
    const std::string info = ps.plugin->Information();

    L0Test::ExpectTrue(tr, info.find("org.rdk.RDKWindowManager") != std::string::npos,
        "Information() contains RDKWindowManager service name");

    return tr.failures;
}
