#include <cstdint>
#include <iostream>

#include "common/L0Bootstrap.hpp"
#include "common/L0TestTypes.hpp"

extern uint32_t Test_VS_NonRamEvictionIsRejected();
extern uint32_t Test_VS_SoftEvictionSelectsPausedCandidate();

int main()
{
    L0Test::EnsureWorkerPool();

    const struct {
        const char* name;
        uint32_t (*function)();
    } tests[] = {
        { "NonRamEvictionIsRejected", Test_VS_NonRamEvictionIsRejected },
        { "SoftEvictionSelectsPausedCandidate", Test_VS_SoftEvictionSelectsPausedCandidate },
    };

    uint32_t failures = 0;
    for (const auto& test : tests) {
        const uint32_t testFailures = test.function();
        if (0 == testFailures) {
            std::cout << "PASS: " << test.name << std::endl;
        }
        failures += testFailures;
    }
    L0Test::PrintTotals(std::cout, "VictimSelector L0 tests", failures);
    return L0Test::ResultToExitCode(failures);
}