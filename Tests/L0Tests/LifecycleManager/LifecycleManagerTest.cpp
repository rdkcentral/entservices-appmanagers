/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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
 * @file LifecycleManagerTest.cpp
 *
 * Main L0 test runner for the LifecycleManager plugin.
 * Registers and runs all test functions from:
 *   - LifecycleManager_ShellTests.cpp        (LCM-001 to LCM-015)
 *   - LifecycleManager_ImplementationTests.cpp (LCM-016 to LCM-073)
 *   - LifecycleManager_ContextTests.cpp       (LCM-074 to LCM-123)
 */

#include <cstdint>
#include <iostream>

#include "common/L0Bootstrap.hpp"
#include "common/L0TestTypes.hpp"

// ── LifecycleManager_ShellTests.cpp ──────────────────────────────────────────
extern uint32_t Test_LCM_ConstructorSetsSInstance();
extern uint32_t Test_LCM_DestructorResetsSInstance();
extern uint32_t Test_LCM_InitializeSucceedsWithFakeImpl();
extern uint32_t Test_LCM_InitializeSucceedsWithInProcessImpl();
extern uint32_t Test_LCM_InitializeCallsConfigureWhenConfigAvailable();
extern uint32_t Test_LCM_InitializeRegistersStateNotification();
extern uint32_t Test_LCM_DeinitializeUnregistersStateNotification();
extern uint32_t Test_LCM_DeinitializeReleasesImplementation();
extern uint32_t Test_LCM_DeinitializeHandlesNullState();
extern uint32_t Test_LCM_InformationReturnsServiceName();
extern uint32_t Test_LCM_NotificationActivatedDoesNotCrash();
extern uint32_t Test_LCM_NotificationDeactivatedDoesNotCrash();
extern uint32_t Test_LCM_NotificationOnAppLifecycleStateChangedDoesNotCrash();
extern uint32_t Test_LCM_NotificationQueryInterfaceReturnsValidPointer();

// ── LifecycleManager_ImplementationTests.cpp ─────────────────────────────────
extern uint32_t Test_Impl_ConstructorInitialisesEmptyLists();
extern uint32_t Test_Impl_RegisterILcmNotification();
extern uint32_t Test_Impl_RegisterILcmNotificationDuplicate();
extern uint32_t Test_Impl_UnregisterILcmNotification();
extern uint32_t Test_Impl_UnregisterILcmNotificationUnknown();
extern uint32_t Test_Impl_RegisterILcmStateNotification();
extern uint32_t Test_Impl_RegisterILcmStateNotificationDuplicate();
extern uint32_t Test_Impl_UnregisterILcmStateNotification();
extern uint32_t Test_Impl_UnregisterILcmStateNotificationUnknown();
extern uint32_t Test_Impl_GetLoadedAppsEmpty();
extern uint32_t Test_Impl_GetLoadedAppsWithApps();
extern uint32_t Test_Impl_GetLoadedAppsNonVerboseOmitsRuntimeStats();
extern uint32_t Test_Impl_IsAppLoadedTrue();
extern uint32_t Test_Impl_IsAppLoadedFalse();
extern uint32_t Test_Impl_SendIntentToActiveAppUnknownApp();
extern uint32_t Test_Impl_SendIntentToActiveAppNonActive();
extern uint32_t Test_Impl_AppReadyUnknownAppId();
extern uint32_t Test_Impl_AppReadyKnownAppId();
extern uint32_t Test_Impl_StateChangeCompleteAlwaysReturnsErrorNone();
extern uint32_t Test_Impl_CloseAppUnknownAppId();
extern uint32_t Test_Impl_ConfigureWithNullService();
extern uint32_t Test_Impl_OnRippleEventIsNoOp();
extern uint32_t Test_Impl_DispatchAppStateChangedCallsLcmNotifications();
extern uint32_t Test_Impl_DispatchAppStateChangedCallsStateNotifications();
extern uint32_t Test_Impl_DispatchOnFailureCallsLcmNotification();
extern uint32_t Test_Impl_DispatchUnknownEventDoesNotCrash();
extern uint32_t Test_Impl_GetContextByAppInstanceId();
extern uint32_t Test_Impl_GetContextReturnsNullForEmptyIds();
extern uint32_t Test_Impl_GetContextReturnsNullForNoMatch();
extern uint32_t Test_Impl_DispatchRuntimeEventDoesNotCrash();
extern uint32_t Test_Impl_DispatchWindowEventDoesNotCrash();
extern uint32_t Test_Impl_SetTargetAppStateUnknownInstance();
extern uint32_t Test_Impl_UnloadAppUnknownInstance();
extern uint32_t Test_Impl_KillAppUnknownInstance();
extern uint32_t Test_Impl_GetLoadedAppsVerboseNoRuntimeHandler();
extern uint32_t Test_Impl_SendIntentToActiveAppActiveApp();
extern uint32_t Test_Impl_DispatchAppStateChangedUnloadedRemovesApp();
extern uint32_t Test_Impl_RuntimeEventOnTerminatedUnknownApp();
extern uint32_t Test_Impl_RuntimeEventOnTerminatedAppInTerminatingState();
extern uint32_t Test_Impl_RuntimeEventOnTerminatedUnexpected();
extern uint32_t Test_Impl_RuntimeEventOnStateChangedRunningUnknownApp();
extern uint32_t Test_Impl_RuntimeEventOnStateChangedRunningKnownApp();
extern uint32_t Test_Impl_RuntimeEventOnFailure();
extern uint32_t Test_Impl_NotifyOnFailureKnownApp();
extern uint32_t Test_Impl_RuntimeEventOnStarted();
extern uint32_t Test_Impl_WindowEventOnUserInactivity();
extern uint32_t Test_Impl_WindowEventOnDisconnect();
extern uint32_t Test_Impl_WindowEventOnReadyUnknownApp();
extern uint32_t Test_Impl_WindowEventOnReadyKnownAppWrongTransition();
extern uint32_t Test_Impl_RuntimeEventOnStateChangedMatchingPendingTransition();

// ── LifecycleManager_ContextTests.cpp ────────────────────────────────────────
extern uint32_t Test_AppCtx_ConstructorInitialisesState();
extern uint32_t Test_AppCtx_DestructorDoesNotCrash();
extern uint32_t Test_AppCtx_AppInstanceIdRoundTrip();
extern uint32_t Test_AppCtx_ActiveSessionIdRoundTrip();
extern uint32_t Test_AppCtx_MostRecentIntentRoundTrip();
extern uint32_t Test_AppCtx_LastLifecycleStateChangeTimeRoundTrip();
extern uint32_t Test_AppCtx_TargetLifecycleStateRoundTrip();
extern uint32_t Test_AppCtx_StateChangeIdRoundTrip();
extern uint32_t Test_AppCtx_ApplicationLaunchParamsRoundTrip();
extern uint32_t Test_AppCtx_ApplicationKillParamsRoundTrip();
extern uint32_t Test_AppCtx_StateRoundTrip();
extern uint32_t Test_AppCtx_GetCurrentLifecycleStateReturnsUnloadedInitially();
extern uint32_t Test_AppCtx_ApplicationLaunchParamsDefaultConstructor();
extern uint32_t Test_AppCtx_ApplicationKillParamsDefaultConstructor();
extern uint32_t Test_RequestHandler_NullifyStaleEventHandler();
extern uint32_t Test_StateHandler_InitializePopulatesMap();
extern uint32_t Test_StateHandler_ChangeStateNullContextReturnsFalse();
extern uint32_t Test_StateHandler_ChangeStateAlreadyAtTargetReturnsTrue();
extern uint32_t Test_StateHandler_TransitionUnloadedToLoadingIsValid();
extern uint32_t Test_StateHandler_TransitionActiveToUnloadedIsInvalid();
extern uint32_t Test_StateHandler_UpdateStateTransitionsContext();
extern uint32_t Test_StateHandler_UpdateStateAlreadyAtStateReturnsTrue();
extern uint32_t Test_StateHandler_CreateStateReturnsCorrectSubclass();
extern uint32_t Test_State_UnloadedHandleReturnsTrue();
extern uint32_t Test_State_LoadingHandleGeneratesAppInstanceId();
extern uint32_t Test_State_InitializingHandleReturnsFalseWhenHandlerNull();
extern uint32_t Test_State_PausedHandleFromInitializingReturnsTrue();
extern uint32_t Test_State_PausedHandleFromActiveReturnsTrue();
extern uint32_t Test_State_PausedHandleFromSuspendedNoHandlerReturnsFalse();
extern uint32_t Test_State_ActiveHandleNoWindowHandlerReturnsTrue();
extern uint32_t Test_State_SuspendedHandleNoRuntimeHandlerReturnsFalse();
extern uint32_t Test_State_HibernatedHandleNoRuntimeHandlerReturnsFalse();
extern uint32_t Test_State_TerminatingHandleNoRuntimeHandlerReturnsFalse();
extern uint32_t Test_State_TerminatingHandleForceKillNoRuntimeHandlerReturnsFalse();
extern uint32_t Test_StateHandler_CreateStatePausedViaInitializingToPaused();
extern uint32_t Test_StateHandler_CreateStateInitializingHandleReturnsFalse();
extern uint32_t Test_StateHandler_CreateStateActiveViaPausedToActive();
extern uint32_t Test_StateHandler_CreateStateSuspendedViaPausedToSuspended();
extern uint32_t Test_StateHandler_CreateStateHibernatedViaSuspendedToHibernated();
extern uint32_t Test_StateHandler_InvalidTransitionReturnsFalse();
extern uint32_t Test_RequestHandler_LaunchDelegatesToUpdateState();
extern uint32_t Test_RequestHandler_SendIntentStoresIntentOnContext();
extern uint32_t Test_RequestHandler_GetWindowManagerHandlerReturnsNull();
extern uint32_t Test_AppCtx_GetRequestTypeDefaultIsNone();
extern uint32_t Test_StateHandler_ChangeStateLoadingToTerminatingPath();

// ── Telemetry tests (TelemetryMetricsClient) ─────────────────────────────────
extern uint32_t Test_TelemetryMetricsClient_IsAvailableReturnsTrue();
extern uint32_t Test_TelemetryMetricsClient_EnsureReturnsErrorNone();
extern uint32_t Test_TelemetryMetricsClient_RecordReturnsErrorNone();
extern uint32_t Test_TelemetryMetricsClient_PublishReturnsErrorNone();

int main()
{
    L0Test::L0BootstrapGuard bootstrap;

    struct Case {
        const char* name;
        uint32_t (*fn)();
    };

    const Case cases[] = {
        // ── Shell tests ──────────────────────────────────────────────────────
        { "LCM_ConstructorSetsSInstance",                           Test_LCM_ConstructorSetsSInstance },
        { "LCM_DestructorResetsSInstance",                          Test_LCM_DestructorResetsSInstance },
        { "LCM_InitializeSucceedsWithFakeImpl",                     Test_LCM_InitializeSucceedsWithFakeImpl },
        { "LCM_InitializeSucceedsWithInProcessImpl",                  Test_LCM_InitializeSucceedsWithInProcessImpl },
        { "LCM_InitializeCallsConfigureWhenConfigAvailable",        Test_LCM_InitializeCallsConfigureWhenConfigAvailable },
        { "LCM_InitializeRegistersStateNotification",               Test_LCM_InitializeRegistersStateNotification },
        { "LCM_DeinitializeUnregistersStateNotification",           Test_LCM_DeinitializeUnregistersStateNotification },
        { "LCM_DeinitializeReleasesImplementation",                 Test_LCM_DeinitializeReleasesImplementation },
        { "LCM_DeinitializeHandlesNullState",                       Test_LCM_DeinitializeHandlesNullState },
        { "LCM_InformationReturnsServiceName",                      Test_LCM_InformationReturnsServiceName },
        { "LCM_NotificationActivatedDoesNotCrash",                  Test_LCM_NotificationActivatedDoesNotCrash },
        { "LCM_NotificationDeactivatedDoesNotCrash",                Test_LCM_NotificationDeactivatedDoesNotCrash },
        { "LCM_NotificationOnAppLifecycleStateChangedDoesNotCrash", Test_LCM_NotificationOnAppLifecycleStateChangedDoesNotCrash },
        { "LCM_NotificationQueryInterfaceReturnsValidPointer",      Test_LCM_NotificationQueryInterfaceReturnsValidPointer },

        // ── Implementation tests ─────────────────────────────────────────────
        { "Impl_ConstructorInitialisesEmptyLists",                  Test_Impl_ConstructorInitialisesEmptyLists },
        { "Impl_RegisterILcmNotification",                          Test_Impl_RegisterILcmNotification },
        { "Impl_RegisterILcmNotificationDuplicate",                 Test_Impl_RegisterILcmNotificationDuplicate },
        { "Impl_UnregisterILcmNotification",                        Test_Impl_UnregisterILcmNotification },
        { "Impl_UnregisterILcmNotificationUnknown",                 Test_Impl_UnregisterILcmNotificationUnknown },
        { "Impl_RegisterILcmStateNotification",                     Test_Impl_RegisterILcmStateNotification },
        { "Impl_RegisterILcmStateNotificationDuplicate",            Test_Impl_RegisterILcmStateNotificationDuplicate },
        { "Impl_UnregisterILcmStateNotification",                   Test_Impl_UnregisterILcmStateNotification },
        { "Impl_UnregisterILcmStateNotificationUnknown",            Test_Impl_UnregisterILcmStateNotificationUnknown },
        { "Impl_GetLoadedAppsEmpty",                                Test_Impl_GetLoadedAppsEmpty },
        { "Impl_GetLoadedAppsWithApps",                             Test_Impl_GetLoadedAppsWithApps },
        { "Impl_GetLoadedAppsNonVerboseOmitsRuntimeStats",          Test_Impl_GetLoadedAppsNonVerboseOmitsRuntimeStats },
        { "Impl_IsAppLoadedTrue",                                   Test_Impl_IsAppLoadedTrue },
        { "Impl_IsAppLoadedFalse",                                  Test_Impl_IsAppLoadedFalse },
        { "Impl_SendIntentToActiveAppUnknownApp",                   Test_Impl_SendIntentToActiveAppUnknownApp },
        { "Impl_SendIntentToActiveAppNonActive",                    Test_Impl_SendIntentToActiveAppNonActive },
        { "Impl_AppReadyUnknownAppId",                              Test_Impl_AppReadyUnknownAppId },
        { "Impl_AppReadyKnownAppId",                                Test_Impl_AppReadyKnownAppId },
        { "Impl_StateChangeCompleteAlwaysReturnsErrorNone",         Test_Impl_StateChangeCompleteAlwaysReturnsErrorNone },
        { "Impl_CloseAppUnknownAppId",                              Test_Impl_CloseAppUnknownAppId },
        { "Impl_ConfigureWithNullService",                          Test_Impl_ConfigureWithNullService },
        { "Impl_OnRippleEventIsNoOp",                               Test_Impl_OnRippleEventIsNoOp },
        { "Impl_DispatchAppStateChangedCallsLcmNotifications",      Test_Impl_DispatchAppStateChangedCallsLcmNotifications },
        { "Impl_DispatchAppStateChangedCallsStateNotifications",    Test_Impl_DispatchAppStateChangedCallsStateNotifications },
        { "Impl_DispatchOnFailureCallsLcmNotification",             Test_Impl_DispatchOnFailureCallsLcmNotification },
        { "Impl_DispatchUnknownEventDoesNotCrash",                  Test_Impl_DispatchUnknownEventDoesNotCrash },
        { "Impl_GetContextByAppInstanceId",                         Test_Impl_GetContextByAppInstanceId },
        { "Impl_GetContextReturnsNullForEmptyIds",                  Test_Impl_GetContextReturnsNullForEmptyIds },
        { "Impl_GetContextReturnsNullForNoMatch",                   Test_Impl_GetContextReturnsNullForNoMatch },
        { "Impl_DispatchRuntimeEventDoesNotCrash",                  Test_Impl_DispatchRuntimeEventDoesNotCrash },
        { "Impl_DispatchWindowEventDoesNotCrash",                   Test_Impl_DispatchWindowEventDoesNotCrash },
        { "Impl_SetTargetAppStateUnknownInstance",                  Test_Impl_SetTargetAppStateUnknownInstance },
        { "Impl_UnloadAppUnknownInstance",                          Test_Impl_UnloadAppUnknownInstance },
        { "Impl_KillAppUnknownInstance",                            Test_Impl_KillAppUnknownInstance },
        { "Impl_GetLoadedAppsVerboseNoRuntimeHandler",              Test_Impl_GetLoadedAppsVerboseNoRuntimeHandler },
        { "Impl_SendIntentToActiveAppActiveApp",                    Test_Impl_SendIntentToActiveAppActiveApp },
        { "Impl_DispatchAppStateChangedUnloadedRemovesApp",         Test_Impl_DispatchAppStateChangedUnloadedRemovesApp },
        { "Impl_RuntimeEventOnTerminatedUnknownApp",                  Test_Impl_RuntimeEventOnTerminatedUnknownApp },
        { "Impl_RuntimeEventOnTerminatedAppInTerminatingState",        Test_Impl_RuntimeEventOnTerminatedAppInTerminatingState },
        { "Impl_RuntimeEventOnTerminatedUnexpected",                   Test_Impl_RuntimeEventOnTerminatedUnexpected },
        { "Impl_RuntimeEventOnStateChangedRunningUnknownApp",          Test_Impl_RuntimeEventOnStateChangedRunningUnknownApp },
        { "Impl_RuntimeEventOnStateChangedRunningKnownApp",            Test_Impl_RuntimeEventOnStateChangedRunningKnownApp },
        { "Impl_RuntimeEventOnFailure",                               Test_Impl_RuntimeEventOnFailure },
        { "Impl_NotifyOnFailureKnownApp",                             Test_Impl_NotifyOnFailureKnownApp },
        { "Impl_RuntimeEventOnStarted",                               Test_Impl_RuntimeEventOnStarted },
        { "Impl_WindowEventOnUserInactivity",                         Test_Impl_WindowEventOnUserInactivity },
        { "Impl_WindowEventOnDisconnect",                             Test_Impl_WindowEventOnDisconnect },
        { "Impl_WindowEventOnReadyUnknownApp",                        Test_Impl_WindowEventOnReadyUnknownApp },
        { "Impl_WindowEventOnReadyKnownAppWrongTransition",            Test_Impl_WindowEventOnReadyKnownAppWrongTransition },
        { "Impl_RuntimeEventOnStateChangedMatchingPendingTransition",  Test_Impl_RuntimeEventOnStateChangedMatchingPendingTransition },

        // ── ApplicationContext tests ─────────────────────────────────────────
        { "AppCtx_ConstructorInitialisesState",                     Test_AppCtx_ConstructorInitialisesState },
        { "AppCtx_DestructorDoesNotCrash",                          Test_AppCtx_DestructorDoesNotCrash },
        { "AppCtx_AppInstanceIdRoundTrip",                          Test_AppCtx_AppInstanceIdRoundTrip },
        { "AppCtx_ActiveSessionIdRoundTrip",                        Test_AppCtx_ActiveSessionIdRoundTrip },
        { "AppCtx_MostRecentIntentRoundTrip",                       Test_AppCtx_MostRecentIntentRoundTrip },
        { "AppCtx_LastLifecycleStateChangeTimeRoundTrip",           Test_AppCtx_LastLifecycleStateChangeTimeRoundTrip },
        { "AppCtx_TargetLifecycleStateRoundTrip",                   Test_AppCtx_TargetLifecycleStateRoundTrip },
        { "AppCtx_StateChangeIdRoundTrip",                          Test_AppCtx_StateChangeIdRoundTrip },
        { "AppCtx_ApplicationLaunchParamsRoundTrip",                Test_AppCtx_ApplicationLaunchParamsRoundTrip },
        { "AppCtx_ApplicationKillParamsRoundTrip",                  Test_AppCtx_ApplicationKillParamsRoundTrip },
        { "AppCtx_StateRoundTrip",                                  Test_AppCtx_StateRoundTrip },
        { "AppCtx_GetCurrentLifecycleStateReturnsUnloadedInitially",Test_AppCtx_GetCurrentLifecycleStateReturnsUnloadedInitially },
        { "AppCtx_ApplicationLaunchParamsDefaultConstructor",       Test_AppCtx_ApplicationLaunchParamsDefaultConstructor },
        { "AppCtx_ApplicationKillParamsDefaultConstructor",         Test_AppCtx_ApplicationKillParamsDefaultConstructor },
        { "AppCtx_GetRequestTypeDefaultIsNone",                     Test_AppCtx_GetRequestTypeDefaultIsNone },

        // ── StateHandler tests ───────────────────────────────────────────────
        { "RequestHandler_NullifyStaleEventHandler",                         Test_RequestHandler_NullifyStaleEventHandler },
        { "StateHandler_InitializePopulatesMap",                             Test_StateHandler_InitializePopulatesMap },
        { "StateHandler_ChangeStateNullContextReturnsFalse",                 Test_StateHandler_ChangeStateNullContextReturnsFalse },
        { "StateHandler_ChangeStateAlreadyAtTargetReturnsTrue",              Test_StateHandler_ChangeStateAlreadyAtTargetReturnsTrue },
        { "StateHandler_TransitionUnloadedToLoadingIsValid",                 Test_StateHandler_TransitionUnloadedToLoadingIsValid },
        { "StateHandler_TransitionActiveToUnloadedIsInvalid",                Test_StateHandler_TransitionActiveToUnloadedIsInvalid },
        { "StateHandler_UpdateStateTransitionsContext",                      Test_StateHandler_UpdateStateTransitionsContext },
        { "StateHandler_UpdateStateAlreadyAtStateReturnsTrue",               Test_StateHandler_UpdateStateAlreadyAtStateReturnsTrue },
        { "StateHandler_CreateStateReturnsCorrectSubclass",                  Test_StateHandler_CreateStateReturnsCorrectSubclass },
        { "StateHandler_CreateStatePausedViaInitializingToPaused",           Test_StateHandler_CreateStatePausedViaInitializingToPaused },
        { "StateHandler_CreateStateInitializingHandleReturnsFalse",          Test_StateHandler_CreateStateInitializingHandleReturnsFalse },
        { "StateHandler_CreateStateActiveViaPausedToActive",                 Test_StateHandler_CreateStateActiveViaPausedToActive },
        { "StateHandler_CreateStateSuspendedViaPausedToSuspended",           Test_StateHandler_CreateStateSuspendedViaPausedToSuspended },
        { "StateHandler_CreateStateHibernatedViaSuspendedToHibernated",      Test_StateHandler_CreateStateHibernatedViaSuspendedToHibernated },
        { "StateHandler_InvalidTransitionReturnsFalse",                      Test_StateHandler_InvalidTransitionReturnsFalse },
        { "StateHandler_ChangeStateLoadingToTerminatingPath",                    Test_StateHandler_ChangeStateLoadingToTerminatingPath },

        // ── State subclass tests ─────────────────────────────────────────────
        { "State_UnloadedHandleReturnsTrue",                                     Test_State_UnloadedHandleReturnsTrue },
        { "State_LoadingHandleGeneratesAppInstanceId",                           Test_State_LoadingHandleGeneratesAppInstanceId },
        { "State_InitializingHandleReturnsFalseWhenHandlerNull",                 Test_State_InitializingHandleReturnsFalseWhenHandlerNull },
        { "State_PausedHandleFromInitializingReturnsTrue",                       Test_State_PausedHandleFromInitializingReturnsTrue },
        { "State_PausedHandleFromActiveReturnsTrue",                             Test_State_PausedHandleFromActiveReturnsTrue },
        { "State_PausedHandleFromSuspendedNoHandlerReturnsFalse",                Test_State_PausedHandleFromSuspendedNoHandlerReturnsFalse },
        { "State_ActiveHandleNoWindowHandlerReturnsTrue",                        Test_State_ActiveHandleNoWindowHandlerReturnsTrue },
        { "State_SuspendedHandleNoRuntimeHandlerReturnsFalse",                   Test_State_SuspendedHandleNoRuntimeHandlerReturnsFalse },
        { "State_HibernatedHandleNoRuntimeHandlerReturnsFalse",                  Test_State_HibernatedHandleNoRuntimeHandlerReturnsFalse },
        { "State_TerminatingHandleNoRuntimeHandlerReturnsFalse",                 Test_State_TerminatingHandleNoRuntimeHandlerReturnsFalse },
        { "State_TerminatingHandleForceKillNoRuntimeHandlerReturnsFalse",        Test_State_TerminatingHandleForceKillNoRuntimeHandlerReturnsFalse },

        // ── RequestHandler tests ─────────────────────────────────────────────
        { "RequestHandler_LaunchDelegatesToUpdateState",                         Test_RequestHandler_LaunchDelegatesToUpdateState },
        { "RequestHandler_SendIntentStoresIntentOnContext",                       Test_RequestHandler_SendIntentStoresIntentOnContext },
        { "RequestHandler_GetWindowManagerHandlerReturnsNull",                   Test_RequestHandler_GetWindowManagerHandlerReturnsNull },
        // ── Telemetry tests ──────────────────────────────────────────────────
        { "TelemetryMetricsClient_IsAvailableReturnsTrue",                       Test_TelemetryMetricsClient_IsAvailableReturnsTrue },
        { "TelemetryMetricsClient_EnsureReturnsErrorNone",                       Test_TelemetryMetricsClient_EnsureReturnsErrorNone },
        { "TelemetryMetricsClient_RecordReturnsErrorNone",                       Test_TelemetryMetricsClient_RecordReturnsErrorNone },
        { "TelemetryMetricsClient_PublishReturnsErrorNone",                      Test_TelemetryMetricsClient_PublishReturnsErrorNone },
    };

    uint32_t totalFailures = 0;
    uint32_t totalRun = 0;

    for (const auto& c : cases) {
        std::cerr << "[ RUN      ] " << c.name << std::endl;
        const uint32_t f = c.fn();
        totalRun++;
        if (0U < f) {
            std::cerr << "[  FAILED  ] " << c.name << " failures=" << f << std::endl;
            totalFailures += f;
        } else {
            std::cerr << "[       OK ] " << c.name << std::endl;
        }
    }

    L0Test::PrintTotals(std::cout, "LifecycleManager L0 Tests", totalFailures);
    std::cout << "Total: " << totalRun << " tests, " << totalFailures << " failure(s).\n";

    return L0Test::ResultToExitCode(totalFailures);
}
