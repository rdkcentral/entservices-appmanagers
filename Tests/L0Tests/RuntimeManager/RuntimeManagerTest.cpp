#include <cstdint>
#include <iostream>

#include "common/L0Bootstrap.hpp"
#include "common/L0TestTypes.hpp"

extern uint32_t Test_RuntimeManager_InitializeFailsWhenRootCreationFails();
extern uint32_t Test_RuntimeManager_InitializeSucceedsAndDeinitializeCleansUp();
extern uint32_t Test_RuntimeManager_InitializeFailsWhenConfigurationInterfaceMissing();
extern uint32_t Test_RuntimeManager_InformationReturnsServiceName();

int main()
{
    L0Test::L0BootstrapGuard bootstrap;

    uint32_t failures = 0;
    failures += Test_RuntimeManager_InitializeFailsWhenRootCreationFails();
    failures += Test_RuntimeManager_InitializeSucceedsAndDeinitializeCleansUp();
    failures += Test_RuntimeManager_InitializeFailsWhenConfigurationInterfaceMissing();
    failures += Test_RuntimeManager_InformationReturnsServiceName();

    L0Test::PrintTotals(std::cout, "RuntimeManager l0test", failures);
    return L0Test::ResultToExitCode(failures);
}
