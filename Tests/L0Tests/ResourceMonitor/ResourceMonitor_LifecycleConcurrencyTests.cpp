#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "PackageLifecycleConfigProvider.h"
#include "ResourceMonitorL0Fakes.hpp"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

namespace {

using WPEFramework::Core::ERROR_NONE;
using WPEFramework::Plugin::AppLifecycleCapabilities;
using WPEFramework::Plugin::AppLifecycleState;
using WPEFramework::Plugin::LifecycleConfigStatus;
using WPEFramework::Plugin::PackageLifecycleConfigProvider;

const char* const kAllStates =
    "{\"supportedNonActiveStates\":[\"paused\",\"suspended\",\"hibernated\"],"
    "\"maxSuspendedSystemMemory\":\"16M\",\"maxTimeToSuspendMemoryState\":10}";

L0Test::FakePackageLifecycleSource::Version Version(const std::string& payload, const uint32_t delayMs)
{
    L0Test::FakePackageLifecycleSource::Version version;
    version.locked = true;
    version.payload = payload;
    version.delayMs = delayMs;
    return version;
}

} // namespace

uint32_t Test_RM_Lifecycle_ConcurrentQueries()
{
    L0Test::TestResult tr;
    auto* source = new L0Test::FakePackageLifecycleSource();
    source->Add("com.configured.app", "1.0.0", Version(L0Test::RalfPayload(kAllStates), 2));
    source->Add("com.unconfigured.app", "1.0.0", Version(L0Test::RalfPayload(""), 2));
    PackageLifecycleConfigProvider provider { std::unique_ptr<WPEFramework::Plugin::IPackageLifecycleSource>(source) };

    constexpr uint32_t kThreads = 8;
    constexpr uint32_t kIterations = 25;
    std::atomic<uint32_t> mismatches(0);
    std::vector<std::future<void>> workers;
    for (uint32_t thread = 0; thread < kThreads; ++thread) {
        workers.push_back(std::async(std::launch::async, [&provider, &mismatches, thread]() {
            for (uint32_t i = 0; i < kIterations; ++i) {
                const bool configured = ((thread + i) % 2) == 0;
                AppLifecycleCapabilities caps;
                const uint32_t rc = provider.GetLifecycleCapabilities(configured ? "com.configured.app" : "com.unconfigured.app", caps);
                const bool ok = (rc == ERROR_NONE) &&
                    (configured ? (caps.status == LifecycleConfigStatus::CONFIGURED && caps.IsSupported(AppLifecycleState::HIBERNATED) &&
                                      caps.suspended.maxSystemMemoryBytes == 16777216ULL)
                                : (caps.status == LifecycleConfigStatus::NOT_CONFIGURED && !caps.IsSupported(AppLifecycleState::PAUSED)));
                if (!ok) {
                    ++mismatches;
                }
            }
        }));
    }

    bool completed = true;
    for (auto& worker : workers) {
        completed = (worker.wait_for(std::chrono::seconds(60)) == std::future_status::ready) && completed;
    }
    L0Test::ExpectTrue(tr, completed, "Concurrent queries complete without deadlock");
    L0Test::ExpectEqU32(tr, mismatches.load(), 0U, "Every concurrent query returns the result it would return on its own");
    if (!completed) {
        // Do not destroy the provider while workers may still use it.
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return tr.failures;
}

uint32_t Test_RM_Lifecycle_SlowPackageManagerDoesNotSerializeCallers()
{
    L0Test::TestResult tr;
    auto* source = new L0Test::FakePackageLifecycleSource();
    source->Add("com.slow.app", "1.0.0", Version(L0Test::RalfPayload(kAllStates), 1500));
    source->Add("com.fast.app", "1.0.0", Version(L0Test::RalfPayload(kAllStates), 0));
    PackageLifecycleConfigProvider provider { std::unique_ptr<WPEFramework::Plugin::IPackageLifecycleSource>(source) };

    std::atomic<bool> slowDone(false);
    std::future<void> slow = std::async(std::launch::async, [&provider, &slowDone]() {
        AppLifecycleCapabilities caps;
        provider.GetLifecycleCapabilities("com.slow.app", caps);
        slowDone = true;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    const auto start = std::chrono::steady_clock::now();
    AppLifecycleCapabilities caps;
    provider.GetLifecycleCapabilities("com.fast.app", caps);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

    L0Test::ExpectTrue(tr, caps.status == LifecycleConfigStatus::CONFIGURED, "Fast query succeeds while a slow query is pending");
    L0Test::ExpectTrue(tr, !slowDone.load(), "Fast query finished before the slow Package Manager call");
    L0Test::ExpectTrue(tr, elapsed.count() < 1000, "Fast query is not blocked by the slow Package Manager call");
    L0Test::ExpectTrue(tr, slow.wait_for(std::chrono::seconds(30)) == std::future_status::ready, "Slow query completes");
    return tr.failures;
}
