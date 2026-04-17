#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common/L0Bootstrap.hpp"
#include "common/L0TestTypes.hpp"

extern uint32_t Test_RDKWM_Lifecycle_InitializeFailsWhenRootNull();
extern uint32_t Test_RDKWM_Lifecycle_InitializeSuccessAndDeinitialize();
extern uint32_t Test_RDKWM_Lifecycle_InformationContainsServiceName();

extern uint32_t Test_RDKWM_Impl_RegisterUnregister();
extern uint32_t Test_RDKWM_Impl_InitializeDeinitialize();
extern uint32_t Test_RDKWM_Impl_AddKeyInterceptValidationAndSuccess();
extern uint32_t Test_RDKWM_Impl_GetAppsAndFocusPaths();
extern uint32_t Test_RDKWM_Impl_VncAndZOrder();
extern uint32_t Test_RDKWM_Impl_EventDispatchViaListener();
extern uint32_t Test_RDKWM_Impl_VisibilityAndRenderApis();
extern uint32_t Test_RDKWM_Impl_KeyListenerAndInjectionPaths();
extern uint32_t Test_RDKWM_Impl_KeyRepeatAndInputEvents();
extern uint32_t Test_RDKWM_Impl_LastKeyInfoAndScreenshotEvent();
extern uint32_t Test_RDKWM_Impl_CreateDisplayAndEventVariants();
extern uint32_t Test_RDKWM_Impl_InterceptsAndInactivityBranches();
extern uint32_t Test_RDKWM_Impl_ErrorPathMatrix();

int main()
{
    struct Case {
        const char* name;
        uint32_t (*fn)();
    };

    const Case cases[] = {
        { "RDKWM_Lifecycle_InitializeFailsWhenRootNull", Test_RDKWM_Lifecycle_InitializeFailsWhenRootNull },
        { "RDKWM_Lifecycle_InitializeSuccessAndDeinitialize", Test_RDKWM_Lifecycle_InitializeSuccessAndDeinitialize },
        { "RDKWM_Lifecycle_InformationContainsServiceName", Test_RDKWM_Lifecycle_InformationContainsServiceName },
        { "RDKWM_Impl_RegisterUnregister", Test_RDKWM_Impl_RegisterUnregister },
        { "RDKWM_Impl_InitializeDeinitialize", Test_RDKWM_Impl_InitializeDeinitialize },
        { "RDKWM_Impl_AddKeyInterceptValidationAndSuccess", Test_RDKWM_Impl_AddKeyInterceptValidationAndSuccess },
        { "RDKWM_Impl_GetAppsAndFocusPaths", Test_RDKWM_Impl_GetAppsAndFocusPaths },
        { "RDKWM_Impl_VncAndZOrder", Test_RDKWM_Impl_VncAndZOrder },
        { "RDKWM_Impl_EventDispatchViaListener", Test_RDKWM_Impl_EventDispatchViaListener },
        { "RDKWM_Impl_VisibilityAndRenderApis", Test_RDKWM_Impl_VisibilityAndRenderApis },
        { "RDKWM_Impl_KeyListenerAndInjectionPaths", Test_RDKWM_Impl_KeyListenerAndInjectionPaths },
        { "RDKWM_Impl_KeyRepeatAndInputEvents", Test_RDKWM_Impl_KeyRepeatAndInputEvents },
        { "RDKWM_Impl_LastKeyInfoAndScreenshotEvent", Test_RDKWM_Impl_LastKeyInfoAndScreenshotEvent },
        { "RDKWM_Impl_CreateDisplayAndEventVariants", Test_RDKWM_Impl_CreateDisplayAndEventVariants },
        { "RDKWM_Impl_InterceptsAndInactivityBranches", Test_RDKWM_Impl_InterceptsAndInactivityBranches },
        { "RDKWM_Impl_ErrorPathMatrix", Test_RDKWM_Impl_ErrorPathMatrix },
    };

    uint32_t failed = 0;
    uint32_t passed = 0;

    // Run each case in a child process so static/plugin lifecycle state
    // from one test cannot affect the next test.
    for (const auto& t : cases) {
        pid_t pid = fork();
        if (pid < 0) {
            failed++;
            std::cout << "[  FAILED  ] " << t.name << " (fork failed)" << std::endl;
            continue;
        }

        if (0 == pid) {
            // Bootstrap in the child process so Core worker-pool state is not inherited across fork.
            L0Test::L0BootstrapGuard bootstrap;
            const uint32_t rc = t.fn();
            // Use exit() so coverage counters in the child are flushed to .gcda.
            std::exit(static_cast<int>(rc > 255U ? 255U : rc));
        }

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
            // Non-zero exit indicates assertion/test failures in that case.
            failed++;
            std::cout << "[  FAILED  ] " << t.name << " (" << WEXITSTATUS(status) << " failures)" << std::endl;
        } else if (WIFSIGNALED(status)) {
            // Signal termination highlights crashes like SIGSEGV/SIGABRT.
            failed++;
            std::cout << "[  FAILED  ] " << t.name << " (signal " << WTERMSIG(status) << ")" << std::endl;
        } else {
            failed++;
            std::cout << "[  FAILED  ] " << t.name << " (abnormal termination)" << std::endl;
        }
    }

    std::cout << "\n==== RDKWindowManager L0 Summary ====" << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;

    return (0U == failed) ? 0 : 1;
}
