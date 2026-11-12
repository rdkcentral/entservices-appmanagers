#include <memory>
#include <string>

#include "AppLifecycleConfig.h"
#include "PackageLifecycleConfigProvider.h"
#include "ResourceMonitorL0Fakes.hpp"
#include "common/L0Expect.hpp"
#include "common/L0TestTypes.hpp"

namespace {

using WPEFramework::Core::ERROR_CONNECTION_CLOSED;
using WPEFramework::Core::ERROR_INVALID_PARAMETER;
using WPEFramework::Core::ERROR_NONE;
using WPEFramework::Plugin::AppLifecycleCapabilities;
using WPEFramework::Plugin::AppLifecycleState;
using WPEFramework::Plugin::LifecycleConfigStatus;
using WPEFramework::Plugin::PackageLifecycleConfigProvider;
using WPEFramework::Plugin::SuspendedStateConfig;
namespace AppLifecycleConfig = WPEFramework::Plugin::AppLifecycleConfig;

const char* const kAllStates =
    "{\"supportedNonActiveStates\":[\"paused\",\"suspended\",\"hibernated\"],"
    "\"maxSuspendedSystemMemory\":\"16M\",\"maxTimeToSuspendMemoryState\":10}";

const char* const kPausedSuspended =
    "{\"supportedNonActiveStates\":[\"paused\",\"suspended\"],"
    "\"maxSuspendedSystemMemory\":\"100M\",\"maxTimeToSuspendMemoryState\":15}";

std::string StatusName(const LifecycleConfigStatus status)
{
    switch (status) {
    case LifecycleConfigStatus::CONFIGURED:
        return "CONFIGURED";
    case LifecycleConfigStatus::NOT_CONFIGURED:
        return "NOT_CONFIGURED";
    default:
        return "UNAVAILABLE";
    }
}

void ExpectStatus(L0Test::TestResult& tr, const AppLifecycleCapabilities& caps, const LifecycleConfigStatus expected, const std::string& what)
{
    L0Test::ExpectEqStr(tr, StatusName(caps.status), StatusName(expected), what);
}

void ExpectStates(L0Test::TestResult& tr, const AppLifecycleCapabilities& caps, const bool paused, const bool suspended,
    const bool hibernated, const std::string& what)
{
    L0Test::ExpectTrue(tr, caps.IsSupported(AppLifecycleState::ACTIVE), what + ": ACTIVE is always supported");
    L0Test::ExpectTrue(tr, caps.IsSupported(AppLifecycleState::PAUSED) == paused, what + ": PAUSED support");
    L0Test::ExpectTrue(tr, caps.IsSupported(AppLifecycleState::SUSPENDED) == suspended, what + ": SUSPENDED support");
    L0Test::ExpectTrue(tr, caps.IsSupported(AppLifecycleState::HIBERNATED) == hibernated, what + ": HIBERNATED support");
}

void ExpectNoSuspendedConfig(L0Test::TestResult& tr, const SuspendedStateConfig& config, const std::string& what)
{
    L0Test::ExpectTrue(tr, !config.hasMaxSystemMemory && !config.hasMaxTimeToSuspend && !config.hasCpuThreshold,
        what + ": Suspended configuration is unavailable");
}

AppLifecycleCapabilities ParseRalf(const std::string& lifecycle)
{
    AppLifecycleCapabilities caps;
    AppLifecycleConfig::Parse(L0Test::RalfPayload(lifecycle), caps);
    return caps;
}

struct ProviderFixture {
    L0Test::FakePackageLifecycleSource* source;
    PackageLifecycleConfigProvider provider;

    ProviderFixture()
        : source(new L0Test::FakePackageLifecycleSource())
        , provider(std::unique_ptr<WPEFramework::Plugin::IPackageLifecycleSource>(source))
    {
    }
};

L0Test::FakePackageLifecycleSource::Version Locked(const std::string& payload, const bool locked = true)
{
    L0Test::FakePackageLifecycleSource::Version version;
    version.locked = locked;
    version.payload = payload;
    return version;
}

} // namespace

// --- Supported lifecycle states -------------------------------------------------------------

uint32_t Test_RM_Lifecycle_AllStatesConfigured()
{
    L0Test::TestResult tr;
    const AppLifecycleCapabilities caps = ParseRalf(kAllStates);
    ExpectStatus(tr, caps, LifecycleConfigStatus::CONFIGURED, "All states: configured");
    ExpectStates(tr, caps, true, true, true, "All states");
    return tr.failures;
}

uint32_t Test_RM_Lifecycle_SubsetOfStates()
{
    L0Test::TestResult tr;
    AppLifecycleCapabilities caps = ParseRalf("{\"supportedNonActiveStates\":[\"paused\"],\"maxSuspendedSystemMemory\":\"16M\",\"maxTimeToSuspendMemoryState\":10}");
    ExpectStatus(tr, caps, LifecycleConfigStatus::CONFIGURED, "Paused only: configured");
    ExpectStates(tr, caps, true, false, false, "Paused only");

    caps = ParseRalf(kPausedSuspended);
    ExpectStates(tr, caps, true, true, false, "Paused and Suspended");
    return tr.failures;
}

uint32_t Test_RM_Lifecycle_CaseInsensitiveAndUnknownStates()
{
    L0Test::TestResult tr;
    const AppLifecycleCapabilities caps = ParseRalf(
        "{\"supportedNonActiveStates\":[\"Paused\",\"SUSPENDED\",\"frozen\",7],\"maxSuspendedSystemMemory\":\"16M\",\"maxTimeToSuspendMemoryState\":10}");
    ExpectStatus(tr, caps, LifecycleConfigStatus::CONFIGURED, "Mixed-case states: configured");
    ExpectStates(tr, caps, true, true, false, "Mixed-case states with unknown entries");
    return tr.failures;
}

uint32_t Test_RM_Lifecycle_NonStandardCombination()
{
    L0Test::TestResult tr;
    const AppLifecycleCapabilities caps = ParseRalf("{\"supportedNonActiveStates\":[\"hibernated\"],\"maxSuspendedSystemMemory\":\"16M\",\"maxTimeToSuspendMemoryState\":10}");
    ExpectStatus(tr, caps, LifecycleConfigStatus::CONFIGURED, "Non-standard combination: configured");
    ExpectStates(tr, caps, false, false, true, "Non-standard combination reports exactly the listed states");
    return tr.failures;
}

uint32_t Test_RM_Lifecycle_EmptyStateList()
{
    L0Test::TestResult tr;
    AppLifecycleCapabilities caps = ParseRalf("{\"supportedNonActiveStates\":[],\"maxSuspendedSystemMemory\":\"16M\",\"maxTimeToSuspendMemoryState\":10}");
    ExpectStatus(tr, caps, LifecycleConfigStatus::CONFIGURED, "Empty list: configured");
    ExpectStates(tr, caps, false, false, false, "Empty list");

    caps = ParseRalf("{\"supportedNonActiveStates\":\"paused\"}");
    ExpectStatus(tr, caps, LifecycleConfigStatus::CONFIGURED, "States not an array: block still present");
    ExpectStates(tr, caps, false, false, false, "States not an array");

    caps = ParseRalf("{\"maxSuspendedSystemMemory\":\"16M\"}");
    ExpectStates(tr, caps, false, false, false, "States missing");
    return tr.failures;
}

// --- Single-state query -------------------------------------------------------------------------

uint32_t Test_RM_Lifecycle_SingleStateQuery()
{
    L0Test::TestResult tr;
    ProviderFixture fx;
    fx.source->Add("com.example.app", "1.0.0", Locked(L0Test::RalfPayload(kPausedSuspended)));

    bool supported = false;
    L0Test::ExpectEqU32(tr, fx.provider.IsLifecycleStateSupported("com.example.app", AppLifecycleState::SUSPENDED, supported),
        ERROR_NONE, "IsLifecycleStateSupported(SUSPENDED) returns ERROR_NONE");
    L0Test::ExpectTrue(tr, supported, "SUSPENDED is supported when configured");

    supported = true;
    fx.provider.IsLifecycleStateSupported("com.example.app", AppLifecycleState::HIBERNATED, supported);
    L0Test::ExpectTrue(tr, !supported, "HIBERNATED is unsupported when not configured");

    fx.provider.IsLifecycleStateSupported("com.example.app", AppLifecycleState::ACTIVE, supported);
    L0Test::ExpectTrue(tr, supported, "ACTIVE is always supported");
    return tr.failures;
}

// --- Missing optional lifecycle configuration --------------------------------------------------

uint32_t Test_RM_Lifecycle_RalfPackageWithoutBlock()
{
    L0Test::TestResult tr;
    const AppLifecycleCapabilities caps = ParseRalf("");
    ExpectStatus(tr, caps, LifecycleConfigStatus::NOT_CONFIGURED, "RALF package without the block: not configured");
    ExpectStates(tr, caps, false, false, false, "RALF package without the block");
    ExpectNoSuspendedConfig(tr, caps.suspended, "RALF package without the block");
    return tr.failures;
}

uint32_t Test_RM_Lifecycle_LegacyPackageIsUnavailable()
{
    L0Test::TestResult tr;
    AppLifecycleCapabilities caps;
    AppLifecycleConfig::Parse(L0Test::RuntimePayload("", ""), caps);
    ExpectStatus(tr, caps, LifecycleConfigStatus::UNAVAILABLE, "Payload without ralfPkgPath: unavailable");
    ExpectStates(tr, caps, false, false, false, "Payload without ralfPkgPath");

    AppLifecycleConfig::Parse("{\"appPath\":\"/apps/test\",\"ralfPkgPath\":\"\"}", caps);
    ExpectStatus(tr, caps, LifecycleConfigStatus::UNAVAILABLE, "Empty ralfPkgPath: unavailable");

    AppLifecycleConfig::Parse(L0Test::RuntimePayload("", kAllStates), caps);
    ExpectStatus(tr, caps, LifecycleConfigStatus::UNAVAILABLE, "Block without RALF metadata source: unavailable");
    ExpectStates(tr, caps, false, false, false, "Block without RALF metadata source is not trusted");
    return tr.failures;
}

uint32_t Test_RM_Lifecycle_MalformedPayloadIsUnavailable()
{
    L0Test::TestResult tr;
    AppLifecycleCapabilities caps;
    AppLifecycleConfig::Parse("{not json", caps);
    ExpectStatus(tr, caps, LifecycleConfigStatus::UNAVAILABLE, "Malformed payload: unavailable");
    AppLifecycleConfig::Parse("", caps);
    ExpectStatus(tr, caps, LifecycleConfigStatus::UNAVAILABLE, "Empty payload: unavailable");
    AppLifecycleConfig::Parse(L0Test::RalfPayload("[\"paused\"]"), caps);
    ExpectStatus(tr, caps, LifecycleConfigStatus::UNAVAILABLE, "applicationLifecycle that is not an object: unavailable");
    ExpectStates(tr, caps, false, false, false, "applicationLifecycle that is not an object");
    return tr.failures;
}

uint32_t Test_RM_Lifecycle_PackageManagerUnavailableThenRetried()
{
    L0Test::TestResult tr;
    ProviderFixture fx;
    fx.source->Add("com.example.app", "1.0.0", Locked(L0Test::RalfPayload(kAllStates)));
    fx.source->available = false;

    AppLifecycleCapabilities caps;
    L0Test::ExpectEqU32(tr, fx.provider.GetLifecycleCapabilities("com.example.app", caps), ERROR_NONE,
        "Package Manager unavailable is reported without an error code");
    ExpectStatus(tr, caps, LifecycleConfigStatus::UNAVAILABLE, "Package Manager unavailable: unavailable");
    ExpectStates(tr, caps, false, false, false, "Package Manager unavailable");

    fx.source->available = true;
    fx.provider.GetLifecycleCapabilities("com.example.app", caps);
    ExpectStatus(tr, caps, LifecycleConfigStatus::CONFIGURED, "Next request after Package Manager returns is configured");
    ExpectStates(tr, caps, true, true, true, "Retry after Package Manager returns");
    return tr.failures;
}

uint32_t Test_RM_Lifecycle_NoLockedPackageIsUnavailable()
{
    L0Test::TestResult tr;
    ProviderFixture fx;
    fx.source->Add("com.example.app", "1.0.0", Locked(L0Test::RalfPayload(kAllStates), false));

    AppLifecycleCapabilities caps;
    L0Test::ExpectEqU32(tr, fx.provider.GetLifecycleCapabilities("com.example.app", caps), ERROR_NONE,
        "No locked package returns ERROR_NONE");
    ExpectStatus(tr, caps, LifecycleConfigStatus::UNAVAILABLE, "No locked package: unavailable");

    fx.provider.GetLifecycleCapabilities("com.not.installed", caps);
    ExpectStatus(tr, caps, LifecycleConfigStatus::UNAVAILABLE, "Unknown application: unavailable");
    return tr.failures;
}

// --- Suspended-state configuration ------------------------------------------------------------

uint32_t Test_RM_Lifecycle_SuspendedConfigPresent()
{
    L0Test::TestResult tr;
    ProviderFixture fx;
    fx.source->Add("com.example.app", "1.0.0", Locked(L0Test::RalfPayload(kAllStates)));

    SuspendedStateConfig config;
    L0Test::ExpectEqU32(tr, fx.provider.GetSuspendedStateConfig("com.example.app", config), ERROR_NONE,
        "GetSuspendedStateConfig() returns ERROR_NONE");
    L0Test::ExpectTrue(tr, config.hasMaxSystemMemory && config.maxSystemMemoryBytes == 16777216ULL,
        "maxSuspendedSystemMemory \"16M\" is reported as 16777216 bytes");
    L0Test::ExpectTrue(tr, config.hasMaxTimeToSuspend && config.maxTimeToSuspendSec == 10U,
        "maxTimeToSuspendMemoryState is reported as 10 seconds");
    L0Test::ExpectTrue(tr, !config.hasCpuThreshold, "CPU threshold is unavailable");
    return tr.failures;
}

uint32_t Test_RM_Lifecycle_SuspendedConfigPartialOrInvalid()
{
    L0Test::TestResult tr;

    AppLifecycleCapabilities caps = ParseRalf("{\"supportedNonActiveStates\":[\"paused\",\"suspended\"],\"maxSuspendedSystemMemory\":\"16M\"}");
    L0Test::ExpectTrue(tr, caps.suspended.hasMaxSystemMemory && !caps.suspended.hasMaxTimeToSuspend,
        "Missing maxTimeToSuspendMemoryState: memory still reported, time unavailable");
    ExpectStates(tr, caps, true, true, false, "Missing time does not affect states");

    caps = ParseRalf("{\"supportedNonActiveStates\":[\"paused\"],\"maxSuspendedSystemMemory\":\"16X\",\"maxTimeToSuspendMemoryState\":10}");
    L0Test::ExpectTrue(tr, !caps.suspended.hasMaxSystemMemory && caps.suspended.hasMaxTimeToSuspend,
        "Unparsable memory: memory unavailable, time still reported");

    const char* const invalidTimes[] = { "-5", "1.5", "\"10\"", "99999999999" };
    for (const char* time : invalidTimes) {
        caps = ParseRalf(std::string("{\"maxSuspendedSystemMemory\":\"1G\",\"maxTimeToSuspendMemoryState\":") + time + "}");
        L0Test::ExpectTrue(tr, !caps.suspended.hasMaxTimeToSuspend && caps.suspended.hasMaxSystemMemory,
            std::string("maxTimeToSuspendMemoryState ") + time + " is unavailable");
    }

    caps = ParseRalf("{\"maxSuspendedSystemMemory\":16,\"maxTimeToSuspendMemoryState\":0}");
    L0Test::ExpectTrue(tr, !caps.suspended.hasMaxSystemMemory, "Numeric maxSuspendedSystemMemory is unavailable");
    L0Test::ExpectTrue(tr, caps.suspended.hasMaxTimeToSuspend && caps.suspended.maxTimeToSuspendSec == 0U,
        "Zero seconds is a valid maxTimeToSuspendMemoryState");
    return tr.failures;
}

uint32_t Test_RM_Lifecycle_CpuThresholdUnavailable()
{
    L0Test::TestResult tr;
    const AppLifecycleCapabilities caps = ParseRalf(
        "{\"supportedNonActiveStates\":[\"paused\",\"suspended\"],\"maxSuspendedSystemMemory\":\"16M\","
        "\"maxTimeToSuspendMemoryState\":10,\"maxSuspendedCpuUsage\":1}");
    L0Test::ExpectTrue(tr, !caps.suspended.hasCpuThreshold, "CPU threshold stays unavailable even with unknown CPU members");
    ExpectStates(tr, caps, true, true, false, "Unknown members do not affect states");
    return tr.failures;
}

// --- Application-to-package resolution --------------------------------------------------------

uint32_t Test_RM_Lifecycle_SingleLockedVersionUsed()
{
    L0Test::TestResult tr;
    ProviderFixture fx;
    fx.source->Add("com.example.app", "1.0.0", Locked(L0Test::RalfPayload(kAllStates), false));
    fx.source->Add("com.example.app", "1.1.0", Locked(L0Test::RalfPayload(kPausedSuspended)));
    fx.source->Add("com.other.app", "2.0.0", Locked(L0Test::RalfPayload(kAllStates)));

    AppLifecycleCapabilities caps;
    fx.provider.GetLifecycleCapabilities("com.example.app", caps);
    ExpectStatus(tr, caps, LifecycleConfigStatus::CONFIGURED, "Single locked version: configured");
    ExpectStates(tr, caps, true, true, false, "The locked version 1.1.0 is used");
    L0Test::ExpectTrue(tr, caps.suspended.maxTimeToSuspendSec == 15U, "Suspended values come from the locked version");
    return tr.failures;
}

uint32_t Test_RM_Lifecycle_ExplicitVersionSkipsListing()
{
    L0Test::TestResult tr;
    ProviderFixture fx;
    fx.source->Add("com.example.app", "1.0.0", Locked(L0Test::RalfPayload(kAllStates)));
    fx.source->Add("com.example.app", "1.1.0", Locked(L0Test::RalfPayload(kPausedSuspended)));

    AppLifecycleCapabilities caps;
    fx.provider.GetLifecycleCapabilities("com.example.app", caps, "1.0.0");
    ExpectStatus(tr, caps, LifecycleConfigStatus::CONFIGURED, "Explicit version: configured");
    ExpectStates(tr, caps, true, true, true, "Explicit version 1.0.0 is used");
    L0Test::ExpectEqU32(tr, fx.source->listCalls.load(), 0U, "Explicit version does not list packages");

    fx.provider.GetLifecycleCapabilities("com.example.app", caps, "9.9.9");
    ExpectStatus(tr, caps, LifecycleConfigStatus::UNAVAILABLE, "Explicit version that is not locked: unavailable");
    return tr.failures;
}

uint32_t Test_RM_Lifecycle_MultipleLockedVersionsUnavailable()
{
    L0Test::TestResult tr;
    ProviderFixture fx;
    fx.source->Add("com.example.app", "1.0.0", Locked(L0Test::RalfPayload(kAllStates)));
    fx.source->Add("com.example.app", "1.1.0", Locked(L0Test::RalfPayload(kAllStates)));

    AppLifecycleCapabilities caps;
    L0Test::ExpectEqU32(tr, fx.provider.GetLifecycleCapabilities("com.example.app", caps), ERROR_NONE,
        "Ambiguous versions return ERROR_NONE");
    ExpectStatus(tr, caps, LifecycleConfigStatus::UNAVAILABLE, "Two locked versions without a version: unavailable");
    ExpectStates(tr, caps, false, false, false, "Two locked versions without a version");
    return tr.failures;
}

uint32_t Test_RM_Lifecycle_PackageManagerErrors()
{
    L0Test::TestResult tr;

    {
        ProviderFixture fx;
        fx.source->Add("com.example.app", "1.0.0", Locked(L0Test::RalfPayload(kAllStates)));
        fx.source->listResult = WPEFramework::Core::ERROR_GENERAL;
        AppLifecycleCapabilities caps;
        L0Test::ExpectEqU32(tr, fx.provider.GetLifecycleCapabilities("com.example.app", caps), ERROR_NONE,
            "ListPackages() failure returns ERROR_NONE");
        ExpectStatus(tr, caps, LifecycleConfigStatus::UNAVAILABLE, "ListPackages() failure: unavailable");
    }
    {
        ProviderFixture fx;
        L0Test::FakePackageLifecycleSource::Version dropped = Locked(L0Test::RalfPayload(kAllStates));
        dropped.result = ERROR_CONNECTION_CLOSED;
        fx.source->Add("com.example.app", "1.0.0", dropped);
        fx.source->Add("com.example.app", "1.1.0", Locked(L0Test::RalfPayload(kAllStates)));
        AppLifecycleCapabilities caps;
        fx.provider.GetLifecycleCapabilities("com.example.app", caps);
        ExpectStatus(tr, caps, LifecycleConfigStatus::UNAVAILABLE, "Connection lost during GetLockedInfo(): unavailable");
    }
    {
        ProviderFixture fx;
        AppLifecycleCapabilities caps;
        bool supported = true;
        SuspendedStateConfig config;
        config.hasMaxSystemMemory = true;
        L0Test::ExpectEqU32(tr, fx.provider.GetLifecycleCapabilities("", caps), ERROR_INVALID_PARAMETER,
            "Empty appId returns ERROR_INVALID_PARAMETER");
        L0Test::ExpectEqU32(tr, fx.provider.IsLifecycleStateSupported("", AppLifecycleState::ACTIVE, supported),
            ERROR_INVALID_PARAMETER, "IsLifecycleStateSupported() with empty appId returns ERROR_INVALID_PARAMETER");
        L0Test::ExpectTrue(tr, !supported, "Empty appId reports the state as unsupported");
        L0Test::ExpectEqU32(tr, fx.provider.GetSuspendedStateConfig("", config), ERROR_INVALID_PARAMETER,
            "GetSuspendedStateConfig() with empty appId returns ERROR_INVALID_PARAMETER");
        ExpectNoSuspendedConfig(tr, config, "Empty appId");
    }
    return tr.failures;
}

uint32_t Test_RM_Lifecycle_ProductionSourceWithoutService()
{
    L0Test::TestResult tr;
    PackageLifecycleConfigProvider provider(std::unique_ptr<WPEFramework::Plugin::IPackageLifecycleSource>(
        new WPEFramework::Plugin::PackageManagerLifecycleSource(nullptr)));
    AppLifecycleCapabilities caps;
    L0Test::ExpectEqU32(tr, provider.GetLifecycleCapabilities("com.example.app", caps), ERROR_NONE,
        "Production source without a shell returns ERROR_NONE");
    ExpectStatus(tr, caps, LifecycleConfigStatus::UNAVAILABLE, "Production source without a shell: unavailable");
    return tr.failures;
}
