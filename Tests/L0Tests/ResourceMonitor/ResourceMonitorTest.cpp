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

int main()
{
    L0Test::L0BootstrapGuard bootstrap;

    const struct {
        const char* name;
        uint32_t (*function)();
    } tests[] = {
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
