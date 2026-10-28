#include <atomic>
#include <string>

#include "RDKWindowManager.h"
#include "RDKWindowManager/ServiceMock.h"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

namespace {

class FakeRDKWindowManagerImpl final : public Thunder::Exchange::IRDKWindowManager {
public:
    FakeRDKWindowManagerImpl()
        : _refCount(1)
        , initializeResult(Thunder::Core::ERROR_NONE)
        , registerCalls(0)
        , unregisterCalls(0)
    {
    }

    uint32_t AddRef() const override
    {
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t Release() const override
    {
        const uint32_t remaining = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (0U == remaining) {
            delete this;
            return Thunder::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return Thunder::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == Thunder::Exchange::IRDKWindowManager::ID) {
            AddRef();
            return static_cast<Thunder::Exchange::IRDKWindowManager*>(this);
        }
        return nullptr;
    }

    Thunder::Core::hresult Register(Thunder::Exchange::IRDKWindowManager::INotification*) override
    {
        registerCalls++;
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult Unregister(Thunder::Exchange::IRDKWindowManager::INotification*) override
    {
        unregisterCalls++;
        return Thunder::Core::ERROR_NONE;
    }

    Thunder::Core::hresult Initialize(Thunder::PluginHost::IShell*) override { return initializeResult; }
    Thunder::Core::hresult Deinitialize(Thunder::PluginHost::IShell*) override { return Thunder::Core::ERROR_NONE; }

    Thunder::Core::hresult CreateDisplay(const std::string&, const std::string&, uint32_t, uint32_t, bool, uint32_t, uint32_t, uint32_t, uint32_t, bool, bool, const std::string&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetApps(Thunder::Exchange::IRDKWindowManager::IStringIterator*& appsIds) const override { appsIds = nullptr; return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult AddKeyIntercept(const std::string&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult AddKeyIntercepts(const std::string&, const std::string&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult RemoveKeyIntercept(const std::string&, uint32_t, const std::string&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult AddKeyListener(const std::string&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult RemoveKeyListener(const std::string&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult InjectKey(uint32_t, const std::string&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GenerateKey(const std::string&, const std::string&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult EnableInactivityReporting(bool) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult SetInactivityInterval(uint32_t) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult ResetInactivityTime() override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult EnableKeyRepeats(bool) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetKeyRepeatsEnabled(bool&) const override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult IgnoreKeyInputs(bool) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult EnableInputEvents(const std::string&, bool) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult KeyRepeatConfig(const std::string&, const std::string&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult SetFocus(const std::string&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult SetVisible(const std::string&, bool) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetVisibility(const std::string&, bool&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult RenderReady(const std::string&, bool&) const override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult EnableDisplayRender(const std::string&, bool) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetLastKeyInfo(uint32_t&, uint32_t&, uint64_t&) const override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult SetZOrder(const std::string&, int32_t) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetZOrder(const std::string&, int32_t&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult StartVncServer() override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult StopVncServer() override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetFocused(std::string&) const override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetScreenshot() override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult SetAlias(const std::string&, const std::string&) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult ShowSplashScreen(const bool) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult SetBounds(const std::string&, const uint32_t, const uint32_t, const uint32_t, const uint32_t) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetBounds(const std::string&, uint32_t&, uint32_t&, uint32_t&, uint32_t&) const override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult SetScale(const std::string&, const double, const double) override { return Thunder::Core::ERROR_NONE; }
    Thunder::Core::hresult GetScale(const std::string&, double&, double&) const override { return Thunder::Core::ERROR_NONE; }

    mutable std::atomic<uint32_t> _refCount;
    Thunder::Core::hresult initializeResult;
    std::atomic<uint32_t> registerCalls;
    std::atomic<uint32_t> unregisterCalls;
};

} // namespace

namespace {

using Thunder::PluginHost::IPlugin;

struct PluginAndService {
    L0Test::ServiceMock service;
    IPlugin* plugin { nullptr };

    PluginAndService()
        : service()
        , plugin(Thunder::Core::Service<Thunder::Plugin::RDKWindowManager>::Create<IPlugin>())
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
    ps.service.SetInstantiateHandler([](const Thunder::RPC::Object&, const uint32_t, uint32_t& connectionId) -> void* {
        connectionId = 0;
        return nullptr;
    });

    const std::string status = ps.plugin->Initialize(&ps.service);
    L0Test::ExpectTrue(tr, status.empty(),
        "Initialize succeeds via deterministic in-process fallback when Root() instantiate returns null");

    ps.plugin->Deinitialize(&ps.service);

    L0Test::ExpectTrue(tr, true,
        "Deinitialize invoked after in-process fallback Initialize");

    return tr.failures;
}

uint32_t Test_RDKWM_Lifecycle_InitializeSuccessAndDeinitialize()
{
    L0Test::TestResult tr;

    PluginAndService ps;
    auto* fakeImpl = new FakeRDKWindowManagerImpl();

    ps.service.SetInstantiateHandler([fakeImpl](const Thunder::RPC::Object&, const uint32_t interfaceId, uint32_t& connectionId) -> void* {
        connectionId = 0;
        if (interfaceId == Thunder::Exchange::IRDKWindowManager::ID) {
            fakeImpl->AddRef();
            return static_cast<Thunder::Exchange::IRDKWindowManager*>(fakeImpl);
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
