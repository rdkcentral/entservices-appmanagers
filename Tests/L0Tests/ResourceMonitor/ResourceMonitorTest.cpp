#include <cstdint>
#include <iostream>

#include "common/L0Bootstrap.hpp"
#include "common/L0TestTypes.hpp"

extern uint32_t Test_RM_Lifecycle_AllStatesConfigured();
extern uint32_t Test_RM_Lifecycle_SubsetOfStates();
extern uint32_t Test_RM_Lifecycle_CaseInsensitiveAndUnknownStates();
extern uint32_t Test_RM_Lifecycle_NonStandardCombination();
extern uint32_t Test_RM_Lifecycle_EmptyStateList();
extern uint32_t Test_RM_Lifecycle_SingleStateQuery();
extern uint32_t Test_RM_Lifecycle_RalfPackageWithoutBlock();
extern uint32_t Test_RM_Lifecycle_LegacyPackageIsUnavailable();
extern uint32_t Test_RM_Lifecycle_MalformedPayloadIsUnavailable();
extern uint32_t Test_RM_Lifecycle_PackageManagerUnavailableThenRetried();
extern uint32_t Test_RM_Lifecycle_NoLockedPackageIsUnavailable();
extern uint32_t Test_RM_Lifecycle_SuspendedConfigPresent();
extern uint32_t Test_RM_Lifecycle_SuspendedConfigPartialOrInvalid();
extern uint32_t Test_RM_Lifecycle_CpuThresholdUnavailable();
extern uint32_t Test_RM_Lifecycle_SingleLockedVersionUsed();
extern uint32_t Test_RM_Lifecycle_ExplicitVersionSkipsListing();
extern uint32_t Test_RM_Lifecycle_MultipleLockedVersionsUnavailable();
extern uint32_t Test_RM_Lifecycle_PackageManagerErrors();
extern uint32_t Test_RM_Lifecycle_ProductionSourceWithoutService();
extern uint32_t Test_RM_Lifecycle_ConcurrentQueries();
extern uint32_t Test_RM_Lifecycle_SlowPackageManagerDoesNotSerializeCallers();
extern uint32_t Test_RM_Enforcement_ManagerLifetime();
extern uint32_t Test_RM_Enforcement_SnapshotLifetime();
extern uint32_t Test_RM_Enforcement_CapabilitySnapshot();
extern uint32_t Test_RM_Enforcement_GpuStatsValidation();
extern uint32_t Test_RM_Enforcement_SuspendedConfigParsing();
extern uint32_t Test_RM_Enforcement_GPUMemoryParsing();
extern uint32_t Test_RM_Enforcement_GPUMemoryValueExtraction();
extern uint32_t Test_RM_Enforcement_SuspendedSupportCheck();
extern uint32_t Test_RM_Enforcement_HibernatedSupportCheck();
extern uint32_t Test_RM_Enforcement_ConfigurationParsing();
extern uint32_t Test_RM_Enforcement_TimerScheduling();
extern uint32_t Test_RM_Enforcement_TimerCancellationAndShutdown();

extern uint32_t Test_RM_AC1_ActiveToPausedStartsTimer();
extern uint32_t Test_RM_AC2_PausedTimeoutSelectsAction();
extern uint32_t Test_RM_AC3_RequestsSuspended();
extern uint32_t Test_RM_AC4_GpuReleasedAfterGrace();
extern uint32_t Test_RM_AC5_TerminatesGpuConsumer();
extern uint32_t Test_RM_AC6_ConfigurableGpuGrace();
extern uint32_t Test_RM_AC7_RequestsHibernated();
extern uint32_t Test_RM_AC8_RejectsUnsupportedStates();
extern uint32_t Test_RM_AC9_TerminatesNonApp();

int main()
{
    L0Test::L0BootstrapGuard bootstrap;

    const struct {
        const char* name;
        uint32_t (*function)();
    } tests[] = {
        { "AC1_ActiveToPausedStartsTimer", Test_RM_AC1_ActiveToPausedStartsTimer },
        { "AC2_PausedTimeoutSelectsAction", Test_RM_AC2_PausedTimeoutSelectsAction },
        { "AC3_RequestsSuspended", Test_RM_AC3_RequestsSuspended },
        { "AC4_GpuReleasedAfterGrace", Test_RM_AC4_GpuReleasedAfterGrace },
        { "AC5_TerminatesGpuConsumer", Test_RM_AC5_TerminatesGpuConsumer },
        { "AC6_ConfigurableGpuGrace", Test_RM_AC6_ConfigurableGpuGrace },
        { "AC7_RequestsHibernated", Test_RM_AC7_RequestsHibernated },
        { "AC8_RejectsUnsupportedStates", Test_RM_AC8_RejectsUnsupportedStates },
        { "AC9_TerminatesNonApp", Test_RM_AC9_TerminatesNonApp },
        { "Lifecycle_AllStatesConfigured", Test_RM_Lifecycle_AllStatesConfigured },
        { "Lifecycle_SubsetOfStates", Test_RM_Lifecycle_SubsetOfStates },
        { "Lifecycle_CaseInsensitiveAndUnknownStates", Test_RM_Lifecycle_CaseInsensitiveAndUnknownStates },
        { "Lifecycle_NonStandardCombination", Test_RM_Lifecycle_NonStandardCombination },
        { "Lifecycle_EmptyStateList", Test_RM_Lifecycle_EmptyStateList },
        { "Lifecycle_SingleStateQuery", Test_RM_Lifecycle_SingleStateQuery },
        { "Lifecycle_RalfPackageWithoutBlock", Test_RM_Lifecycle_RalfPackageWithoutBlock },
        { "Lifecycle_LegacyPackageIsUnavailable", Test_RM_Lifecycle_LegacyPackageIsUnavailable },
        { "Lifecycle_MalformedPayloadIsUnavailable", Test_RM_Lifecycle_MalformedPayloadIsUnavailable },
        { "Lifecycle_PackageManagerUnavailableThenRetried", Test_RM_Lifecycle_PackageManagerUnavailableThenRetried },
        { "Lifecycle_NoLockedPackageIsUnavailable", Test_RM_Lifecycle_NoLockedPackageIsUnavailable },
        { "Lifecycle_SuspendedConfigPresent", Test_RM_Lifecycle_SuspendedConfigPresent },
        { "Lifecycle_SuspendedConfigPartialOrInvalid", Test_RM_Lifecycle_SuspendedConfigPartialOrInvalid },
        { "Lifecycle_CpuThresholdUnavailable", Test_RM_Lifecycle_CpuThresholdUnavailable },
        { "Lifecycle_SingleLockedVersionUsed", Test_RM_Lifecycle_SingleLockedVersionUsed },
        { "Lifecycle_ExplicitVersionSkipsListing", Test_RM_Lifecycle_ExplicitVersionSkipsListing },
        { "Lifecycle_MultipleLockedVersionsUnavailable", Test_RM_Lifecycle_MultipleLockedVersionsUnavailable },
        { "Lifecycle_PackageManagerErrors", Test_RM_Lifecycle_PackageManagerErrors },
        { "Lifecycle_ProductionSourceWithoutService", Test_RM_Lifecycle_ProductionSourceWithoutService },
        { "Lifecycle_ConcurrentQueries", Test_RM_Lifecycle_ConcurrentQueries },
        { "Lifecycle_SlowPackageManagerDoesNotSerializeCallers", Test_RM_Lifecycle_SlowPackageManagerDoesNotSerializeCallers },
        { "Enforcement_ManagerLifetime", Test_RM_Enforcement_ManagerLifetime },
        { "Enforcement_SnapshotLifetime", Test_RM_Enforcement_SnapshotLifetime },
        { "Enforcement_CapabilitySnapshot", Test_RM_Enforcement_CapabilitySnapshot },
        { "Enforcement_GpuStatsValidation", Test_RM_Enforcement_GpuStatsValidation },
        { "Enforcement_SuspendedConfigParsing", Test_RM_Enforcement_SuspendedConfigParsing },
        { "Enforcement_GPUMemoryParsing", Test_RM_Enforcement_GPUMemoryParsing },
        { "Enforcement_GPUMemoryValueExtraction", Test_RM_Enforcement_GPUMemoryValueExtraction },
        { "Enforcement_SuspendedSupportCheck", Test_RM_Enforcement_SuspendedSupportCheck },
        { "Enforcement_HibernatedSupportCheck", Test_RM_Enforcement_HibernatedSupportCheck },
        { "Enforcement_ConfigurationParsing", Test_RM_Enforcement_ConfigurationParsing },
        { "Enforcement_TimerScheduling", Test_RM_Enforcement_TimerScheduling },
        { "Enforcement_TimerCancellationAndShutdown", Test_RM_Enforcement_TimerCancellationAndShutdown },
    };

    uint32_t failures = 0;
    for (const auto& test : tests) {
        const uint32_t testFailures = test.function();
        if (0 == testFailures) {
            std::cout << "PASS: " << test.name << std::endl;
        } else {
            std::cerr << "FAIL: " << test.name << " failures=" << testFailures << std::endl;
        }
        failures += testFailures;
    }
    L0Test::PrintTotals(std::cout, "ResourceMonitor L0 tests", failures);
    return L0Test::ResultToExitCode(failures);
}
