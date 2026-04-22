/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2026 RDK Management
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
 * Each test runs in a separate child process for isolation.
 */

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common/L0Bootstrap.hpp"
#include "common/L0TestTypes.hpp"

// ── AppStorageManager_LifecycleTests.cpp ─────────────────────────────────────
extern uint32_t Test_ASM_Lifecycle_InitializeFailsWhenRootNull();
extern uint32_t Test_ASM_Lifecycle_InitializeSuccessAndDeinitialize();
extern uint32_t Test_ASM_Lifecycle_InitializeFailsWhenConfigMissing();
extern uint32_t Test_ASM_Lifecycle_InitializeFailsWhenConfigureFails();
extern uint32_t Test_ASM_Lifecycle_InformationReturnsServiceName();

// ── AppStorageManager_ImplementationTests.cpp ────────────────────────────────
extern uint32_t Test_Impl_ConfigureWithValidService();
extern uint32_t Test_Impl_ConfigureWithNullService();
extern uint32_t Test_Impl_CreateStorageWithValidInput();
extern uint32_t Test_Impl_CreateStorageWithEmptyAppId();
extern uint32_t Test_Impl_CreateStorageWithZeroSize();
extern uint32_t Test_Impl_CreateStorageDuplicateAppId();
extern uint32_t Test_Impl_GetStorageWithValidAppId();
extern uint32_t Test_Impl_GetStorageWithEmptyAppId();
extern uint32_t Test_Impl_GetStorageWithNonExistentAppId();
extern uint32_t Test_Impl_DeleteStorageWithValidAppId();
extern uint32_t Test_Impl_DeleteStorageWithEmptyAppId();
extern uint32_t Test_Impl_DeleteStorageWithNonExistentAppId();
extern uint32_t Test_Impl_ClearStorageWithValidAppId();
extern uint32_t Test_Impl_ClearStorageWithEmptyAppId();
extern uint32_t Test_Impl_ClearAllWithNoExemptions();
extern uint32_t Test_Impl_ClearAllWithExemptions();
extern uint32_t Test_Impl_ClearAllWithInvalidJsonFormat();

// ── AppStorageManager_ComponentTests.cpp ─────────────────────────────────────
extern uint32_t Test_RH_GetInstanceReturnsSingleton();
extern uint32_t Test_RH_SetBaseStoragePathValid();
extern uint32_t Test_RH_CreatePersistentStoreObjectSuccess();
extern uint32_t Test_RH_CreatePersistentStoreObjectFailsWithNullService();
extern uint32_t Test_RH_IsValidAppStorageDirectoryValid();
extern uint32_t Test_RH_IsValidAppStorageDirectoryInvalid();
extern uint32_t Test_Tel_GetInstanceReturnsSingleton();

int main()
{
    struct Case {
        const char* name;
        uint32_t (*fn)();
    };

    const Case cases[] = {
        // Lifecycle Tests
        { "ASM_Lifecycle_InitializeFailsWhenRootNull", Test_ASM_Lifecycle_InitializeFailsWhenRootNull },
        { "ASM_Lifecycle_InitializeSuccessAndDeinitialize", Test_ASM_Lifecycle_InitializeSuccessAndDeinitialize },
        { "ASM_Lifecycle_InitializeFailsWhenConfigMissing", Test_ASM_Lifecycle_InitializeFailsWhenConfigMissing },
        { "ASM_Lifecycle_InitializeFailsWhenConfigureFails", Test_ASM_Lifecycle_InitializeFailsWhenConfigureFails },
        { "ASM_Lifecycle_InformationReturnsServiceName", Test_ASM_Lifecycle_InformationReturnsServiceName },
        
        // Implementation Tests
        { "Impl_ConfigureWithValidService", Test_Impl_ConfigureWithValidService },
        { "Impl_ConfigureWithNullService", Test_Impl_ConfigureWithNullService },
        { "Impl_CreateStorageWithValidInput", Test_Impl_CreateStorageWithValidInput },
        { "Impl_CreateStorageWithEmptyAppId", Test_Impl_CreateStorageWithEmptyAppId },
        { "Impl_CreateStorageWithZeroSize", Test_Impl_CreateStorageWithZeroSize },
        { "Impl_CreateStorageDuplicateAppId", Test_Impl_CreateStorageDuplicateAppId },
        { "Impl_GetStorageWithValidAppId", Test_Impl_GetStorageWithValidAppId },
        { "Impl_GetStorageWithEmptyAppId", Test_Impl_GetStorageWithEmptyAppId },
        { "Impl_GetStorageWithNonExistentAppId", Test_Impl_GetStorageWithNonExistentAppId },
        { "Impl_DeleteStorageWithValidAppId", Test_Impl_DeleteStorageWithValidAppId },
        { "Impl_DeleteStorageWithEmptyAppId", Test_Impl_DeleteStorageWithEmptyAppId },
        { "Impl_DeleteStorageWithNonExistentAppId", Test_Impl_DeleteStorageWithNonExistentAppId },
        { "Impl_ClearStorageWithValidAppId", Test_Impl_ClearStorageWithValidAppId },
        { "Impl_ClearStorageWithEmptyAppId", Test_Impl_ClearStorageWithEmptyAppId },
        { "Impl_ClearAllWithNoExemptions", Test_Impl_ClearAllWithNoExemptions },
        { "Impl_ClearAllWithExemptions", Test_Impl_ClearAllWithExemptions },
        { "Impl_ClearAllWithInvalidJsonFormat", Test_Impl_ClearAllWithInvalidJsonFormat },
        
        // Component Tests
        { "RH_GetInstanceReturnsSingleton", Test_RH_GetInstanceReturnsSingleton },
        { "RH_SetBaseStoragePathValid", Test_RH_SetBaseStoragePathValid },
        { "RH_CreatePersistentStoreObjectSuccess", Test_RH_CreatePersistentStoreObjectSuccess },
        { "RH_CreatePersistentStoreObjectFailsWithNullService", Test_RH_CreatePersistentStoreObjectFailsWithNullService },
        { "RH_IsValidAppStorageDirectoryValid", Test_RH_IsValidAppStorageDirectoryValid },
        { "RH_IsValidAppStorageDirectoryInvalid", Test_RH_IsValidAppStorageDirectoryInvalid },
        { "Tel_GetInstanceReturnsSingleton", Test_Tel_GetInstanceReturnsSingleton },
    };

    uint32_t failed = 0;
    uint32_t passed = 0;

    // Run each test in a child process for isolation
    for (const auto& t : cases) {
        pid_t pid = fork();
        if (pid < 0) {
            failed++;
            std::cout << "[  FAILED  ] " << t.name << " (fork failed)" << std::endl;
            continue;
        }

        if (0 == pid) {
            // Child process: bootstrap and run test
            L0Test::L0BootstrapGuard bootstrap;
            const uint32_t rc = t.fn();
            // Use exit() so coverage counters are flushed to .gcda
            std::exit(static_cast<int>(rc > 255U ? 255U : rc));
        }

        // Parent process: wait for child
        int status = 0;
        if (waitpid(pid, &status, 0) < 0) {
            failed++;
            std::cout << "[  FAILED  ] " << t.name << " (waitpid failed)" << std::endl;
            continue;
        }

        if (WIFEXITED(status) && 0 == WEXITSTATUS(status)) {
            passed++;
            std::cout << "[       OK ] " << t.name << std::endl;
        } else if (WIFEXITED(status)) {
            failed++;
            std::cout << "[  FAILED  ] " << t.name << " (" << WEXITSTATUS(status) << " failures)" << std::endl;
        } else if (WIFSIGNALED(status)) {
            failed++;
            std::cout << "[  FAILED  ] " << t.name << " (signal " << WTERMSIG(status) << ")" << std::endl;
        } else {
            failed++;
            std::cout << "[  FAILED  ] " << t.name << " (abnormal termination)" << std::endl;
        }
    }

    std::cout << "\n==== AppStorageManager L0 Summary ====" << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;

    return (0U == failed) ? 0 : 1;
}
