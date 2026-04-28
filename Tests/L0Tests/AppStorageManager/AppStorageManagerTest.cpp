/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

/**
 * @file AppStorageManagerTest.cpp
 *
 * Main test runner for AppStorageManager L0 tests.
 * Each test case runs in an isolated child process via fork() to prevent
 * state leakage and ensure coverage data is properly collected.
 */

#include <cstdint>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common/L0Bootstrap.hpp"
#include "common/L0TestTypes.hpp"

// ── AppStorageManager_LifecycleTests.cpp ────────────────────────────────────
extern uint32_t Test_ASM_Lifecycle_InitializeSucceedsWithInstantiateFallback();
extern uint32_t Test_ASM_Lifecycle_InitializeSuccessAndDeinitialize();
extern uint32_t Test_ASM_Lifecycle_InitializeSucceedsWithFallbackCreation();
extern uint32_t Test_ASM_Lifecycle_InformationReturnsEmptyString();
extern uint32_t Test_ASM_Lifecycle_DeinitializeSuccessWithValidService();

// ── AppStorageManager_ImplementationTests.cpp ───────────────────────────────
extern uint32_t Test_Impl_ConfigureWithValidService();
extern uint32_t Test_Impl_ConfigureWithNullService();
extern uint32_t Test_Impl_CreateStorageWithValidInput();
extern uint32_t Test_Impl_CreateStorageWithEmptyAppId();
extern uint32_t Test_Impl_GetStorageWithValidAppId();
extern uint32_t Test_Impl_GetStorageWithEmptyAppId();
extern uint32_t Test_Impl_GetStorageWithNonExistentAppId();
extern uint32_t Test_Impl_DeleteStorageWithValidAppId();
extern uint32_t Test_Impl_DeleteStorageWithEmptyAppId();
extern uint32_t Test_Impl_ClearStorageWithValidAppId();
extern uint32_t Test_Impl_ClearStorageWithEmptyAppId();
extern uint32_t Test_Impl_ClearAllWithNoExemptions();
extern uint32_t Test_Impl_ClearAllWithExemptions();

// ── AppStorageManager_ComponentTests.cpp ────────────────────────────────────
extern uint32_t Test_RH_GetInstanceReturnsSingleton();
extern uint32_t Test_RH_SetBaseStoragePathValid();
extern uint32_t Test_Tel_GetInstanceReturnsSingleton();

int main()
{
    struct TestCase {
        const char* name;
        uint32_t (*fn)();
    };

    const TestCase cases[] = {
        // Lifecycle tests
        {"Test_ASM_Lifecycle_InitializeSucceedsWithInstantiateFallback", Test_ASM_Lifecycle_InitializeSucceedsWithInstantiateFallback},
        {"Test_ASM_Lifecycle_InitializeSuccessAndDeinitialize", Test_ASM_Lifecycle_InitializeSuccessAndDeinitialize},
        {"Test_ASM_Lifecycle_InitializeSucceedsWithFallbackCreation", Test_ASM_Lifecycle_InitializeSucceedsWithFallbackCreation},
        {"Test_ASM_Lifecycle_InformationReturnsEmptyString", Test_ASM_Lifecycle_InformationReturnsEmptyString},
        {"Test_ASM_Lifecycle_DeinitializeSuccessWithValidService", Test_ASM_Lifecycle_DeinitializeSuccessWithValidService},

        // Implementation tests
        {"Test_Impl_ConfigureWithValidService", Test_Impl_ConfigureWithValidService},
        {"Test_Impl_ConfigureWithNullService", Test_Impl_ConfigureWithNullService},
        {"Test_Impl_CreateStorageWithValidInput", Test_Impl_CreateStorageWithValidInput},
        {"Test_Impl_CreateStorageWithEmptyAppId", Test_Impl_CreateStorageWithEmptyAppId},
        {"Test_Impl_GetStorageWithValidAppId", Test_Impl_GetStorageWithValidAppId},
        {"Test_Impl_GetStorageWithEmptyAppId", Test_Impl_GetStorageWithEmptyAppId},
        {"Test_Impl_GetStorageWithNonExistentAppId", Test_Impl_GetStorageWithNonExistentAppId},
        {"Test_Impl_DeleteStorageWithValidAppId", Test_Impl_DeleteStorageWithValidAppId},
        {"Test_Impl_DeleteStorageWithEmptyAppId", Test_Impl_DeleteStorageWithEmptyAppId},
        {"Test_Impl_ClearStorageWithValidAppId", Test_Impl_ClearStorageWithValidAppId},
        {"Test_Impl_ClearStorageWithEmptyAppId", Test_Impl_ClearStorageWithEmptyAppId},
        {"Test_Impl_ClearAllWithNoExemptions", Test_Impl_ClearAllWithNoExemptions},
        {"Test_Impl_ClearAllWithExemptions", Test_Impl_ClearAllWithExemptions},

        // Component tests
        {"Test_RH_GetInstanceReturnsSingleton", Test_RH_GetInstanceReturnsSingleton},
        {"Test_RH_SetBaseStoragePathValid", Test_RH_SetBaseStoragePathValid},
        {"Test_Tel_GetInstanceReturnsSingleton", Test_Tel_GetInstanceReturnsSingleton},
    };

    uint32_t totalFailures = 0;
    const size_t numTests = sizeof(cases) / sizeof(cases[0]);

    std::cout << "Running AppStorageManager L0 Tests (" << numTests << " tests)\n";
    std::cout << "================================================\n";

    for (size_t i = 0; i < numTests; ++i) {
        pid_t pid = fork();

        if (pid == 0) {
            // Child process: run the test
            L0Test::L0BootstrapGuard bootstrap;
            uint32_t failures = cases[i].fn();
            // Clamp to 0-255 since WEXITSTATUS only preserves 8 bits
            std::exit(failures > 255 ? 255 : failures);
        } else if (pid > 0) {
            // Parent process: wait for child
            int status;
            waitpid(pid, &status, 0);

            if (WIFEXITED(status)) {
                uint32_t childFailures = WEXITSTATUS(status);
                if (childFailures > 0) {
                    std::cerr << "FAIL: " << cases[i].name
                              << " (" << childFailures << " assertion failures)\n";
                    totalFailures += childFailures;
                } else {
                    std::cout << "PASS: " << cases[i].name << "\n";
                }
            } else if (WIFSIGNALED(status)) {
                std::cerr << "FAIL: " << cases[i].name
                          << " (terminated by signal " << WTERMSIG(status) << ")\n";
                totalFailures++;
            } else {
                std::cerr << "FAIL: " << cases[i].name
                          << " (abnormal termination)\n";
                totalFailures++;
            }
        } else {
            std::cerr << "ERROR: Failed to fork for test " << cases[i].name << "\n";
            totalFailures++;
        }
    }

    std::cout << "================================================\n";
    L0Test::PrintTotals(std::cerr, "AppStorageManager L0 Tests", totalFailures);
    return L0Test::ResultToExitCode({totalFailures});
}
