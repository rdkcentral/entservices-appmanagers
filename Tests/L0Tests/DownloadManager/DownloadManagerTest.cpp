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
 * @file DownloadManagerTest.cpp
 *
 * Entry point for the DownloadManager L0 test executable.
 * Declares all test functions from the individual test files and
 * runs them in sequence using the shared L0 framework.
 *
 * Build target: downloadmanager_l0test
 * Coverage target: ≥75% of DownloadManager plugin .cpp / .h files
 */

#include <cstdint>
#include <iostream>

#include "common/L0Bootstrap.hpp"
#include "common/L0TestTypes.hpp"

// ─────────────────────────────────────────────────────────────────────────────
// DownloadManager_LifecycleTests.cpp  (Shell plugin lifecycle tests)
// ─────────────────────────────────────────────────────────────────────────────
extern uint32_t Test_Shell_InitializeRootCreationFailureReturnsError();
extern uint32_t Test_Shell_InitializeSuccessRegistersNotification();
extern uint32_t Test_Shell_DeinitializeCleansUp();
extern uint32_t Test_Shell_DeinitializeNoOpWhenServiceNull();
extern uint32_t Test_Shell_InformationReturnsEmpty();
extern uint32_t Test_Shell_DeactivatedSubmitsJobForMatchingId();
extern uint32_t Test_Shell_DeactivatedIgnoresMismatchedId();
extern uint32_t Test_Shell_NotificationHandlerOnAppDownloadStatus();

// ─────────────────────────────────────────────────────────────────────────────
// DownloadManager_ImplementationTests.cpp  (Implementation tests)
// ─────────────────────────────────────────────────────────────────────────────
extern uint32_t Test_Impl_RegisterNotification();
extern uint32_t Test_Impl_RegisterDuplicate();
extern uint32_t Test_Impl_RegisterNullptrDoesNotCrash();
extern uint32_t Test_Impl_UnregisterRegisteredNotification();
extern uint32_t Test_Impl_UnregisterUnknownReturnsError();
extern uint32_t Test_Impl_MultipleNotificationsRegisterUnregister();
extern uint32_t Test_Impl_InitializeWithNullService();
extern uint32_t Test_Impl_InitializeValidServiceAndDeinitialize();
extern uint32_t Test_Impl_InitializeReadsDownloadDir();
extern uint32_t Test_Impl_InitializeReadsDownloadId();
extern uint32_t Test_Impl_InitializeExistingDirReturnsNone();
extern uint32_t Test_Impl_DeinitializeClearsQueues();
extern uint32_t Test_Impl_DownloadNoInternetReturnsUnavailable();
extern uint32_t Test_Impl_DownloadEmptyUrlReturnsError();
extern uint32_t Test_Impl_DownloadPriorityAndRegularQueues();
extern uint32_t Test_Impl_DownloadReturnsIncrementedId();
extern uint32_t Test_Impl_TwoDownloadsReturnUniqueIds();
extern uint32_t Test_Impl_PauseEmptyIdReturnsError();
extern uint32_t Test_Impl_PauseNoActiveDownloadReturnsError();
extern uint32_t Test_Impl_ResumeEmptyIdReturnsError();
extern uint32_t Test_Impl_ResumeNoActiveDownloadReturnsError();
extern uint32_t Test_Impl_CancelEmptyIdReturnsError();
extern uint32_t Test_Impl_CancelNoActiveDownloadReturnsError();
extern uint32_t Test_Impl_DeleteEmptyFileLocatorReturnsError();
extern uint32_t Test_Impl_DeleteNonCurrentFileReturnsNone();
extern uint32_t Test_Impl_DeleteNonExistentFileReturnsError();
extern uint32_t Test_Impl_ProgressNoActiveDownloadReturnsError();
extern uint32_t Test_Impl_GetStorageDetailsReturnsNone();
extern uint32_t Test_Impl_RateLimitEmptyIdReturnsError();
extern uint32_t Test_Impl_RateLimitNoActiveDownloadReturnsError();
extern uint32_t Test_Impl_PickDownloadJobPriorityFirst();
extern uint32_t Test_Impl_NotifyStatusFiresAllNotifications();
extern uint32_t Test_Impl_DownloadInfoFieldsViaDownload();
extern uint32_t Test_Impl_DownloadInfoZeroRetriesClampsToMin();
extern uint32_t Test_Impl_DtorReleasesUnregisteredNotifications();
extern uint32_t Test_Impl_InitializeFailsWhenMkdirFails();
extern uint32_t Test_Impl_DownloaderRoutineSuccessPath();
extern uint32_t Test_Impl_DownloaderRoutineDiskErrorPath();
extern uint32_t Test_Impl_DownloaderRoutineHttpErrorPath();
extern uint32_t Test_Impl_ActiveDownloadMatchIdOperations();
extern uint32_t Test_Impl_ActiveDownloadMismatchIdOperations();
extern uint32_t Test_Impl_DeleteInProgressFileReturnsError();
extern uint32_t Test_Impl_PickDownloadJobRegularQueueUsed();
extern uint32_t Test_Impl_NotifyStatusIncludesFailReason();
extern uint32_t Test_Impl_CancelWithActiveDownloadMatchId();

// ─────────────────────────────────────────────────────────────────────────────
// DownloadManager_HttpClientTests.cpp  (HttpClient tests)
// ─────────────────────────────────────────────────────────────────────────────
extern uint32_t Test_HttpClient_ConstructionInitializesCurl();
extern uint32_t Test_HttpClient_DestructionCleanupWithoutCrash();
extern uint32_t Test_HttpClient_DownloadFileInvalidUrlReturnsHttpError();
extern uint32_t Test_HttpClient_DownloadFileUnwritableDestReturnsDiskError();
extern uint32_t Test_HttpClient_DownloadFile404HandledGracefully();
extern uint32_t Test_HttpClient_PauseDoesNotCrash();
extern uint32_t Test_HttpClient_ResumeDoesNotCrash();
extern uint32_t Test_HttpClient_CancelSetsBCancelFlagProgressCbReturnsNonzero();
extern uint32_t Test_HttpClient_SetRateLimitDoesNotCrash();
extern uint32_t Test_HttpClient_GetProgressReturnsZeroInitially();
extern uint32_t Test_HttpClient_GetStatusCodeReturnsZeroInitially();
extern uint32_t Test_HttpClient_ProgressCbZeroDltotalDoesNotUpdateProgress();
extern uint32_t Test_HttpClient_WriteDataCallsFwrite();

// ─────────────────────────────────────────────────────────────────────────────
// DownloadManager_TelemetryTests.cpp  (Telemetry tests)
// ─────────────────────────────────────────────────────────────────────────────
extern uint32_t Test_Telemetry_GetInstanceReturnsSingleton();
extern uint32_t Test_Telemetry_InitializeWithValidServiceDoesNotCrash();
extern uint32_t Test_Telemetry_ResetDoesNotCrash();
extern uint32_t Test_Telemetry_RecordDownloadTimeWhenDisabledIsNoop();
extern uint32_t Test_Telemetry_RecordDownloadErrorWhenDisabledIsNoop();
extern uint32_t Test_Telemetry_RecordDownloadTimeWithValidParamsDoesNotCrash();
extern uint32_t Test_Telemetry_RecordDownloadErrorWithValidParamsDoesNotCrash();

// ─────────────────────────────────────────────────────────────────────────────
// RUN_TEST macro — mirrors the pattern in RuntimeManagerTest.cpp
// ─────────────────────────────────────────────────────────────────────────────

#define RUN_TEST(fn)                                                             \
    do {                                                                         \
        const uint32_t _f = (fn)();                                              \
        totalFailures += _f;                                                     \
        if (0u == _f) {                                                          \
            std::cout << "PASS: " #fn << std::endl;                              \
        } else {                                                                 \
            std::cerr << "FAIL: " #fn << " (" << _f << " failure(s))" << std::endl; \
        }                                                                        \
    } while (0)

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main()
{
    L0Test::L0BootstrapGuard guard;
    uint32_t totalFailures = 0u;

    std::cout << "=== DownloadManager L0 Tests ===" << std::endl;

    // ── Shell / Lifecycle tests (DownloadManager.cpp / .h) ──────────────────
    std::cout << "\n-- Shell Plugin Lifecycle --" << std::endl;
    RUN_TEST(Test_Shell_InitializeRootCreationFailureReturnsError);
    RUN_TEST(Test_Shell_InitializeSuccessRegistersNotification);
    RUN_TEST(Test_Shell_DeinitializeCleansUp);
    RUN_TEST(Test_Shell_DeinitializeNoOpWhenServiceNull);
    RUN_TEST(Test_Shell_InformationReturnsEmpty);
    RUN_TEST(Test_Shell_DeactivatedSubmitsJobForMatchingId);
    RUN_TEST(Test_Shell_DeactivatedIgnoresMismatchedId);
    RUN_TEST(Test_Shell_NotificationHandlerOnAppDownloadStatus);

    // ── Implementation tests (DownloadManagerImplementation.cpp / .h) ───────
    std::cout << "\n-- Implementation Register/Unregister --" << std::endl;
    RUN_TEST(Test_Impl_RegisterNotification);
    RUN_TEST(Test_Impl_RegisterDuplicate);
    RUN_TEST(Test_Impl_RegisterNullptrDoesNotCrash);
    RUN_TEST(Test_Impl_UnregisterRegisteredNotification);
    RUN_TEST(Test_Impl_UnregisterUnknownReturnsError);
    RUN_TEST(Test_Impl_MultipleNotificationsRegisterUnregister);

    std::cout << "\n-- Implementation Initialize/Deinitialize --" << std::endl;
    RUN_TEST(Test_Impl_InitializeWithNullService);
    RUN_TEST(Test_Impl_InitializeValidServiceAndDeinitialize);
    RUN_TEST(Test_Impl_InitializeReadsDownloadDir);
    RUN_TEST(Test_Impl_InitializeReadsDownloadId);
    RUN_TEST(Test_Impl_InitializeExistingDirReturnsNone);
    RUN_TEST(Test_Impl_DeinitializeClearsQueues);

    std::cout << "\n-- Implementation Download --" << std::endl;
    RUN_TEST(Test_Impl_DownloadNoInternetReturnsUnavailable);
    RUN_TEST(Test_Impl_DownloadEmptyUrlReturnsError);
    RUN_TEST(Test_Impl_DownloadPriorityAndRegularQueues);
    RUN_TEST(Test_Impl_DownloadReturnsIncrementedId);
    RUN_TEST(Test_Impl_TwoDownloadsReturnUniqueIds);

    std::cout << "\n-- Implementation Pause/Resume/Cancel --" << std::endl;
    RUN_TEST(Test_Impl_PauseEmptyIdReturnsError);
    RUN_TEST(Test_Impl_PauseNoActiveDownloadReturnsError);
    RUN_TEST(Test_Impl_ResumeEmptyIdReturnsError);
    RUN_TEST(Test_Impl_ResumeNoActiveDownloadReturnsError);
    RUN_TEST(Test_Impl_CancelEmptyIdReturnsError);
    RUN_TEST(Test_Impl_CancelNoActiveDownloadReturnsError);

    std::cout << "\n-- Implementation Delete/Progress/RateLimit --" << std::endl;
    RUN_TEST(Test_Impl_DeleteEmptyFileLocatorReturnsError);
    RUN_TEST(Test_Impl_DeleteNonCurrentFileReturnsNone);
    RUN_TEST(Test_Impl_DeleteNonExistentFileReturnsError);
    RUN_TEST(Test_Impl_ProgressNoActiveDownloadReturnsError);
    RUN_TEST(Test_Impl_GetStorageDetailsReturnsNone);
    RUN_TEST(Test_Impl_RateLimitEmptyIdReturnsError);
    RUN_TEST(Test_Impl_RateLimitNoActiveDownloadReturnsError);

    std::cout << "\n-- Implementation Queue / Notify / DownloadInfo --" << std::endl;
    RUN_TEST(Test_Impl_PickDownloadJobPriorityFirst);
    RUN_TEST(Test_Impl_NotifyStatusFiresAllNotifications);
    RUN_TEST(Test_Impl_DownloadInfoFieldsViaDownload);
    RUN_TEST(Test_Impl_DownloadInfoZeroRetriesClampsToMin);

    std::cout << "\n-- Implementation Dtor / mkdir-fail / Downloader thread --" << std::endl;
    RUN_TEST(Test_Impl_DtorReleasesUnregisteredNotifications);
    RUN_TEST(Test_Impl_InitializeFailsWhenMkdirFails);
    RUN_TEST(Test_Impl_DownloaderRoutineSuccessPath);
    RUN_TEST(Test_Impl_DownloaderRoutineDiskErrorPath);
    RUN_TEST(Test_Impl_DownloaderRoutineHttpErrorPath);

    std::cout << "\n-- Implementation Active-download operations --" << std::endl;
    RUN_TEST(Test_Impl_ActiveDownloadMatchIdOperations);
    RUN_TEST(Test_Impl_ActiveDownloadMismatchIdOperations);
    RUN_TEST(Test_Impl_DeleteInProgressFileReturnsError);
    RUN_TEST(Test_Impl_PickDownloadJobRegularQueueUsed);
    RUN_TEST(Test_Impl_NotifyStatusIncludesFailReason);
    RUN_TEST(Test_Impl_CancelWithActiveDownloadMatchId);

    // ── HTTP client tests (DownloadManagerHttpClient.cpp / .h) ──────────────
    std::cout << "\n-- HttpClient --" << std::endl;
    RUN_TEST(Test_HttpClient_ConstructionInitializesCurl);
    RUN_TEST(Test_HttpClient_DestructionCleanupWithoutCrash);
    RUN_TEST(Test_HttpClient_DownloadFileInvalidUrlReturnsHttpError);
    RUN_TEST(Test_HttpClient_DownloadFileUnwritableDestReturnsDiskError);
    RUN_TEST(Test_HttpClient_DownloadFile404HandledGracefully);
    RUN_TEST(Test_HttpClient_PauseDoesNotCrash);
    RUN_TEST(Test_HttpClient_ResumeDoesNotCrash);
    RUN_TEST(Test_HttpClient_CancelSetsBCancelFlagProgressCbReturnsNonzero);
    RUN_TEST(Test_HttpClient_SetRateLimitDoesNotCrash);
    RUN_TEST(Test_HttpClient_GetProgressReturnsZeroInitially);
    RUN_TEST(Test_HttpClient_GetStatusCodeReturnsZeroInitially);
    RUN_TEST(Test_HttpClient_ProgressCbZeroDltotalDoesNotUpdateProgress);
    RUN_TEST(Test_HttpClient_WriteDataCallsFwrite);

    // ── Telemetry tests (DownloadManagerTelemetryReporting.cpp / .h) ────────
    std::cout << "\n-- Telemetry --" << std::endl;
    RUN_TEST(Test_Telemetry_GetInstanceReturnsSingleton);
    RUN_TEST(Test_Telemetry_InitializeWithValidServiceDoesNotCrash);
    RUN_TEST(Test_Telemetry_ResetDoesNotCrash);
    RUN_TEST(Test_Telemetry_RecordDownloadTimeWhenDisabledIsNoop);
    RUN_TEST(Test_Telemetry_RecordDownloadErrorWhenDisabledIsNoop);
    RUN_TEST(Test_Telemetry_RecordDownloadTimeWithValidParamsDoesNotCrash);
    RUN_TEST(Test_Telemetry_RecordDownloadErrorWithValidParamsDoesNotCrash);

    std::cout << std::endl;
    L0Test::PrintTotals(std::cout, "DownloadManager_L0Test", totalFailures);
    return L0Test::ResultToExitCode(totalFailures);
}
