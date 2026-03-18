#include <cstdint>
#include <iostream>

#include "common/L0Bootstrap.hpp"
#include "common/L0TestTypes.hpp"

// ── RuntimeManager_LifecycleTests.cpp ────────────────────────────────────────
extern uint32_t Test_RuntimeManager_InitializeFailsWhenRootCreationFails();
extern uint32_t Test_RuntimeManager_InitializeSucceedsAndDeinitializeCleansUp();
extern uint32_t Test_RuntimeManager_InitializeFailsWhenConfigurationInterfaceMissing();
extern uint32_t Test_RuntimeManager_InformationReturnsServiceName();

// ── RuntimeManager_ImplementationTests.cpp ───────────────────────────────────
extern uint32_t Test_Impl_RegisterNotification();
extern uint32_t Test_Impl_RegisterDuplicate();
extern uint32_t Test_Impl_UnregisterNotification();
extern uint32_t Test_Impl_UnregisterNotRegistered();
extern uint32_t Test_Impl_ConfigureWithNullService();
extern uint32_t Test_Impl_ConfigureWithServiceMissingOCIPlugin();
extern uint32_t Test_Impl_ConfigureSetsRuntimePortalFromConfigLine();
extern uint32_t Test_Impl_TerminateEmptyAppInstanceId();
extern uint32_t Test_Impl_TerminateNoOCIPlugin();
extern uint32_t Test_Impl_KillEmptyAppInstanceId();
extern uint32_t Test_Impl_KillNoOCIPlugin();
extern uint32_t Test_Impl_HibernateEmptyAppInstanceId();
extern uint32_t Test_Impl_HibernateNoOCIPlugin();
extern uint32_t Test_Impl_WakeEmptyAppInstanceId();
extern uint32_t Test_Impl_WakeNoRunningContainer();
extern uint32_t Test_Impl_SuspendEmptyAppInstanceId();
extern uint32_t Test_Impl_SuspendNoOCIPlugin();
extern uint32_t Test_Impl_ResumeEmptyAppInstanceId();
extern uint32_t Test_Impl_ResumeNoOCIPlugin();
extern uint32_t Test_Impl_AnnotateEmptyAppInstanceId();
extern uint32_t Test_Impl_AnnotateEmptyKey();
extern uint32_t Test_Impl_AnnotateNoOCIPlugin();
extern uint32_t Test_Impl_GetInfoEmptyAppInstanceId();
extern uint32_t Test_Impl_GetInfoNoOCIPlugin();
extern uint32_t Test_Impl_RunEmptyAppInstanceId();
extern uint32_t Test_Impl_RunNoWindowManagerConnector();
extern uint32_t Test_Impl_MountReturnsSuccess();
extern uint32_t Test_Impl_UnmountReturnsSuccess();
extern uint32_t Test_Impl_GetInstanceReturnsSelf();
extern uint32_t Test_Impl_GetInstanceBeforeConstruction();
extern uint32_t Test_Impl_OCIContainerStartedEvent();
extern uint32_t Test_Impl_OCIContainerStoppedEvent();
extern uint32_t Test_Impl_OCIContainerFailureEvent();
extern uint32_t Test_Impl_OCIContainerStateChangedEvent();
extern uint32_t Test_Impl_OCIContainerAllEventsCoverage();
extern uint32_t Test_Impl_OCIEventsWithRegisteredNotification();
extern uint32_t Test_Impl_QueryInterfaceIRuntimeManager();
extern uint32_t Test_Impl_QueryInterfaceIConfiguration();
extern uint32_t Test_Impl_QueryInterfaceUnknownID();
extern uint32_t Test_Impl_MultipleNotificationsRegisterUnregister();
extern uint32_t Test_Impl_DispatchContainerStartedFiresOnStartedCallback();
extern uint32_t Test_Impl_DispatchContainerStoppedFiresOnTerminatedCallback();
extern uint32_t Test_Impl_DispatchContainerFailedFiresOnFailureCallback();
extern uint32_t Test_Impl_DispatchStateChangedFiresOnStateChangedCallback();
extern uint32_t Test_Impl_DispatchPortalPrefixStrippingWithRegisteredNotification();
extern uint32_t Test_Impl_DispatchMultipleNotificationsAllReceiveStartedEvent();

// ── RuntimeManager_ComponentTests.cpp ────────────────────────────────────────
extern uint32_t Test_UserIdManager_GetUserIdReturnsValidRange();
extern uint32_t Test_UserIdManager_GetUserIdSameAppReturnsSameUid();
extern uint32_t Test_UserIdManager_GetUserIdDifferentAppsGetDifferentUids();
extern uint32_t Test_UserIdManager_ClearUserIdReturnsUidToPool();
extern uint32_t Test_UserIdManager_ClearUserIdForUnknownAppDoesNotCrash();
extern uint32_t Test_UserIdManager_GetAppsGidReturns30000();
extern uint32_t Test_UserIdManager_ExhaustPoolReturnsZero();
extern uint32_t Test_UserIdManager_MultipleGetAndClearCycles();
extern uint32_t Test_AIConfig_DefaultConsoleLogCap();
extern uint32_t Test_AIConfig_DefaultNonHomeAppMemoryLimit();
extern uint32_t Test_AIConfig_DefaultNonHomeAppGpuLimit();
extern uint32_t Test_AIConfig_DefaultAppsCpuSet();
extern uint32_t Test_AIConfig_DefaultDialServerPort();
extern uint32_t Test_AIConfig_DefaultIonHeapQuota();
extern uint32_t Test_AIConfig_DefaultEnvVariablesNonEmpty();
extern uint32_t Test_AIConfig_ResourceManagerClientDisabledByDefault();
extern uint32_t Test_AIConfig_SvpDisabledByDefault();
extern uint32_t Test_AIConfig_IPv6DisabledByDefault();
extern uint32_t Test_AIConfig_PrintAIConfigurationDoesNotCrash();
extern uint32_t Test_DobbyEventListener_ConstructionAndDestruction();
extern uint32_t Test_DobbyEventListener_InitializeWithNullOCIPlugin();
extern uint32_t Test_DobbyEventListener_DeinitializeWithoutInitialize();
extern uint32_t Test_DobbyEventListener_DoubleDeinitialize();
extern uint32_t Test_WindowManagerConnector_ConstructionAndDestruction();
extern uint32_t Test_WindowManagerConnector_IsPluginInitializedReturnsFalseInitially();
extern uint32_t Test_WindowManagerConnector_InitializeWithNullServiceReturnsFalse();
extern uint32_t Test_WindowManagerConnector_InitializeWithServiceMissingWindowManagerPlugin();
extern uint32_t Test_WindowManagerConnector_ReleasePluginWithoutInitDoesNotCrash();
extern uint32_t Test_WindowManagerConnector_GetDisplayInfoWithoutInitDoesNotCrash();
extern uint32_t Test_DobbySpecGenerator_GenerateWithValidConfig();
extern uint32_t Test_DobbySpecGenerator_GenerateWithZeroUserId();
extern uint32_t Test_DobbySpecGenerator_GenerateWithEmptyCommand();
extern uint32_t Test_DobbySpecGenerator_GenerateSpecContainsVersion();
extern uint32_t Test_DobbySpecGenerator_GenerateSpecContainsArgs();
extern uint32_t Test_DobbySpecGenerator_GenerateSpecContainsMemLimit();
extern uint32_t Test_DobbySpecGenerator_ConstructionAndDestruction();
extern uint32_t Test_DobbySpecGenerator_GenerateWithRuntimePath();
extern uint32_t Test_DobbySpecGenerator_GenerateWithoutRuntimePath();
extern uint32_t Test_DobbySpecGenerator_GenerateSpecContainsCpu();
extern uint32_t Test_DobbySpecGenerator_GenerateSpecContainsNetwork();
extern uint32_t Test_DobbySpecGenerator_GenerateSpecContainsRdkPlugins();
extern uint32_t Test_DobbySpecGenerator_GenerateWithMemLimitFromConfig();
extern uint32_t Test_AIConfig_GetGstreamerRegistryEnabledDefault();
extern uint32_t Test_AIConfig_GetEnableUsbMassStorageDefault();
extern uint32_t Test_AIConfig_GetMapiPortsEmptyByDefault();
extern uint32_t Test_AIConfig_GetVpuAccessBlacklistEmptyByDefault();
extern uint32_t Test_AIConfig_GetAppsRequiringDBusEmptyByDefault();
extern uint32_t Test_AIConfig_GetDialUsnEmptyByDefault();
extern uint32_t Test_AIConfig_GetDialServerPathPrefixEmptyByDefault();
extern uint32_t Test_AIConfig_GetPreloadsEmptyByDefault();
extern uint32_t Test_AIConfig_GetIonHeapQuotasEmptyByDefault();
extern uint32_t Test_WindowManagerConnector_CreateDisplayWithoutInitReturnsFalse();
extern uint32_t Test_WindowManagerConnector_GetDisplayInfoSetsNonEmptyDisplayName();
// ── ralf/RalfSupport tests ────────────────────────────────────────────────────
extern uint32_t Test_Ralf_ParseMemorySize_EmptyStringReturnsZero();
extern uint32_t Test_Ralf_ParseMemorySize_PlainIntegerNoSuffix();
extern uint32_t Test_Ralf_ParseMemorySize_KilobytesSuffix();
extern uint32_t Test_Ralf_ParseMemorySize_MegabytesSuffix();
extern uint32_t Test_Ralf_ParseMemorySize_GigabytesSuffix();
extern uint32_t Test_Ralf_ParseMemorySize_LowercaseSuffix();
extern uint32_t Test_Ralf_ParseMemorySize_SuffixWithByte();
extern uint32_t Test_Ralf_ParseMemorySize_InvalidSuffixReturnsZero();
extern uint32_t Test_Ralf_ParseMemorySize_NonNumericInputReturnsZero();
extern uint32_t Test_Ralf_ParseMemorySize_ZeroValueReturnsZero();
extern uint32_t Test_Ralf_ParseMemorySize_KbSuffixWithB();
extern uint32_t Test_Ralf_SerializeJsonNode_SimpleObject();
extern uint32_t Test_Ralf_SerializeJsonNode_EmptyObject();
extern uint32_t Test_Ralf_SerializeJsonNode_IntegerValue();
extern uint32_t Test_Ralf_SerializeJsonNode_ArrayValue();
extern uint32_t Test_Ralf_SerializeJsonNode_NoWhitespace();
extern uint32_t Test_Ralf_CheckIfPathExists_NonExistentPathReturnsFalse();
extern uint32_t Test_Ralf_CheckIfPathExists_ExistingPathReturnsTrue();
extern uint32_t Test_Ralf_CheckIfPathExists_ExistingFileReturnsTrue();
extern uint32_t Test_Ralf_CreateDirectories_EmptyPathReturnsFalse();
extern uint32_t Test_Ralf_CreateDirectories_SingleLevelPath();
extern uint32_t Test_Ralf_CreateDirectories_MultiLevelPath();
extern uint32_t Test_Ralf_CreateDirectories_ExistingPathReturnsTrue();
extern uint32_t Test_Ralf_JsonFromFile_NonExistentFileReturnsFalse();
extern uint32_t Test_Ralf_JsonFromFile_ValidJsonFileReturnsTrue();
extern uint32_t Test_Ralf_JsonFromFile_InvalidJsonFileReturnsFalse();
extern uint32_t Test_Ralf_JsonFromFile_EmptyFileReturnsFalse();
extern uint32_t Test_Ralf_ParseRalPkgInfo_NonExistentFileReturnsFalse();
extern uint32_t Test_Ralf_ParseRalPkgInfo_MissingPackagesFieldReturnsFalse();
extern uint32_t Test_Ralf_ParseRalPkgInfo_ValidPackagesFileReturnsTrue();
extern uint32_t Test_Ralf_ParseRalPkgInfo_EmptyPackagesArraySucceeds();
extern uint32_t Test_Ralf_GetDevNodeMajorMinor_NonExistentNodeReturnsFalse();
extern uint32_t Test_Ralf_GetDevNodeMajorMinor_DevNullReturnsTrue();
extern uint32_t Test_Ralf_GetGroupId_UnknownGroupReturnsFalse();
extern uint32_t Test_Ralf_GetGroupId_RootGroupReturnsTrue();
extern uint32_t Test_Ralf_GetRalfUserInfo_ReturnsConsistentResult();

// ── ralf/RalfPackageBuilder tests ─────────────────────────────────────────────
extern uint32_t Test_RalfPackageBuilder_ConstructionAndDestruction();
extern uint32_t Test_RalfPackageBuilder_UnmountOverlayfsIfExists_NonExistentPathReturnsFalse();
extern uint32_t Test_RalfPackageBuilder_UnmountOverlayfsIfExists_EmptyAppInstanceId();
extern uint32_t Test_RalfPackageBuilder_GenerateRalfDobbySpec_EmptyRalfPkgPathReturnsFalse();
extern uint32_t Test_RalfPackageBuilder_GenerateRalfDobbySpec_NonExistentPkgFileReturnsFalse();
extern uint32_t Test_RalfPackageBuilder_GenerateRalfDobbySpec_MalformedPkgFileReturnsFalse();
extern uint32_t Test_RalfPackageBuilder_GenerateRalfDobbySpec_PkgFileMissingPackagesFieldReturnsFalse();
extern uint32_t Test_RalfPackageBuilder_GenerateRalfDobbySpec_OutputDobbySpecEmptyOnFailure();

// ── ralf/RalfOCIConfigGenerator tests ────────────────────────────────────────
extern uint32_t Test_RalfOCIConfigGenerator_ConstructionAndDestruction();
extern uint32_t Test_RalfOCIConfigGenerator_ConstructionWithEmptyPackages();
extern uint32_t Test_RalfOCIConfigGenerator_ConstructionWithMultiplePackages();
extern uint32_t Test_RalfOCIConfigGenerator_GenerateFailsWhenBaseSpecFileMissing();
extern uint32_t Test_RalfOCIConfigGenerator_GenerateFailsWhenGraphicsConfigMissing();
extern uint32_t Test_RalfOCIConfigGenerator_GenerateWithMockBaseSpec();
extern uint32_t Test_RalfOCIConfigGenerator_MultipleGenerateCallsDoNotCrash();
extern uint32_t Test_RalfOCIConfigGenerator_GenerateWithDifferentAppInstances();

// ── Gateway/ContainerUtils tests ─────────────────────────────────────────────
extern uint32_t Test_ContainerUtils_GetContainerIpAddress_UnknownContainerReturnsZero();
extern uint32_t Test_ContainerUtils_GetContainerIpAddress_EmptyContainerIdReturnsZero();
extern uint32_t Test_ContainerUtils_GetContainerIpAddress_MultipleCallsDoNotCrash();
extern uint32_t Test_ContainerUtils_NamespaceEnumValues();
extern uint32_t Test_ContainerUtils_GetContainerIpAddress_VeryLongContainerIdDoesNotCrash();

// ── Gateway/WebInspector tests ────────────────────────────────────────────────
extern uint32_t Test_Debugger_TypeEnum_WebInspectorValue();
extern uint32_t Test_WebInspector_Attach_WithInvalidIpReturnsNullptrOrNonNull();
extern uint32_t Test_WebInspector_Attach_ZeroIpReturnsNullptrOrNonNull();
extern uint32_t Test_WebInspector_Attach_EmptyAppIdDoesNotCrash();
extern uint32_t Test_WebInspector_Attach_MultipleCallsDoNotCrash();
extern uint32_t Test_WebInspector_TypeReturnsWebInspector();
extern uint32_t Test_WebInspector_DestructorRunsCleanupWithoutCrash();

// ── Gateway/NetFilter & NetFilterLock tests ───────────────────────────────────
extern uint32_t Test_NetFilterLock_ConstructionAndDestruction();
extern uint32_t Test_NetFilterLock_TryLockDoesNotCrash();
extern uint32_t Test_NetFilterLock_LockUnlockDoesNotCrash();
extern uint32_t Test_NetFilterLock_TryLockForZeroDurationDoesNotCrash();
extern uint32_t Test_NetFilterLock_MultipleInstancesDoNotCrash();
extern uint32_t Test_NetFilter_Protocol_TcpAndUdpValuesDistinct();
extern uint32_t Test_NetFilter_RemoveAllRulesMatchingComment_EmptyCommentDoesNotCrash();
extern uint32_t Test_NetFilter_RemoveAllRulesMatchingComment_NoMatchCommentDoesNotCrash();
extern uint32_t Test_NetFilter_OpenExternalPort_DoesNotCrash();
extern uint32_t Test_NetFilter_AddContainerPortForwarding_DoesNotCrash();
extern uint32_t Test_NetFilter_AddContainerPortForwarding_ReturnsFalseOrTrue();
extern uint32_t Test_NetFilter_GetContainerPortForwardList_ReturnsListType();
extern uint32_t Test_NetFilter_GetContainerPortForwardList_DefaultIpReturnsListType();
extern uint32_t Test_NetFilter_PortForward_StructMembersAccessible();

// ── Additional coverage tests ─────────────────────────────────────────────────
// DobbyEventListener coverage
extern uint32_t Test_DobbyEventListener_InitializeWithValidOCIContainerSucceeds();
extern uint32_t Test_DobbyEventListener_InitializeWithRegisterFailureReturnsFalse();
extern uint32_t Test_DobbyEventListener_DeinitializeWithValidOCIContainerCallsUnregister();
extern uint32_t Test_DobbyEventListener_OnContainerStartedCallsEventHandler();
extern uint32_t Test_DobbyEventListener_OnContainerStoppedCallsEventHandler();
extern uint32_t Test_DobbyEventListener_OnContainerFailedCallsEventHandler();
extern uint32_t Test_DobbyEventListener_OnContainerStateChangedStartingState();
extern uint32_t Test_DobbyEventListener_OnContainerStateChangedRunningState();
extern uint32_t Test_DobbyEventListener_OnContainerStateChangedAllStates();
extern uint32_t Test_DobbyEventListener_OnContainerStateChangedNullEventHandler();
// WindowManagerConnector coverage
extern uint32_t Test_WindowManagerConnector_InitializeWithValidWindowManagerSucceeds();
extern uint32_t Test_WindowManagerConnector_ReleasePluginAfterInitCallsUnregister();
extern uint32_t Test_WindowManagerConnector_CreateDisplayAfterInitReturnsTrue();
extern uint32_t Test_WindowManagerConnector_CreateDisplayAfterInitReturnsFalseOnError();
extern uint32_t Test_WindowManagerConnector_OnUserInactivityDoesNotCrash();
extern uint32_t Test_WindowManagerConnector_GetDisplayInfoWithXdgRuntimeDirSet();
extern uint32_t Test_WindowManagerConnector_GetDisplayInfoWithoutXdgRuntimeDirFallsToTmp();
// DobbySpecGenerator additional branch coverage
extern uint32_t Test_DobbySpecGenerator_GenerateWithSpecChangeFile();
extern uint32_t Test_DobbySpecGenerator_GenerateWithWanLanAccessFalse();
extern uint32_t Test_DobbySpecGenerator_GenerateWithAppPath();
extern uint32_t Test_DobbySpecGenerator_GenerateThunderPluginEnabled();
extern uint32_t Test_DobbySpecGenerator_GenerateThunderPluginDisabled();
extern uint32_t Test_DobbySpecGenerator_GenerateWithEmptyWesterosSocket();
extern uint32_t Test_DobbySpecGenerator_GenerateSysMemLimitZeroFallsBack();
extern uint32_t Test_DobbySpecGenerator_GenerateWithNonEmptyAppPorts();
extern uint32_t Test_DobbySpecGenerator_GetVpuEnabledReturnsFalseForSystemApp();
extern uint32_t Test_DobbySpecGenerator_GenerateWithEnvVariablesInRuntimeConfig();

int main()
{
    L0Test::L0BootstrapGuard bootstrap;

    struct Case {
        const char* name;
        uint32_t (*fn)();
    };

    const Case cases[] = {
        // ── Lifecycle tests ──────────────────────────────────────────────────
        { "RuntimeManager_InitializeFailsWhenRootCreationFails",                     Test_RuntimeManager_InitializeFailsWhenRootCreationFails },
        { "RuntimeManager_InitializeSucceedsAndDeinitializeCleansUp",                Test_RuntimeManager_InitializeSucceedsAndDeinitializeCleansUp },
        { "RuntimeManager_InitializeFailsWhenConfigurationInterfaceMissing",         Test_RuntimeManager_InitializeFailsWhenConfigurationInterfaceMissing },
        { "RuntimeManager_InformationReturnsServiceName",                            Test_RuntimeManager_InformationReturnsServiceName },

        // ── Implementation tests ─────────────────────────────────────────────
        { "Impl_RegisterNotification",                                               Test_Impl_RegisterNotification },
        { "Impl_RegisterDuplicate",                                                  Test_Impl_RegisterDuplicate },
        { "Impl_UnregisterNotification",                                             Test_Impl_UnregisterNotification },
        { "Impl_UnregisterNotRegistered",                                            Test_Impl_UnregisterNotRegistered },
        { "Impl_ConfigureWithNullService",                                           Test_Impl_ConfigureWithNullService },
        { "Impl_ConfigureWithServiceMissingOCIPlugin",                               Test_Impl_ConfigureWithServiceMissingOCIPlugin },
        { "Impl_ConfigureSetsRuntimePortalFromConfigLine",                           Test_Impl_ConfigureSetsRuntimePortalFromConfigLine },
        { "Impl_TerminateEmptyAppInstanceId",                                        Test_Impl_TerminateEmptyAppInstanceId },
        { "Impl_TerminateNoOCIPlugin",                                               Test_Impl_TerminateNoOCIPlugin },
        { "Impl_KillEmptyAppInstanceId",                                             Test_Impl_KillEmptyAppInstanceId },
        { "Impl_KillNoOCIPlugin",                                                    Test_Impl_KillNoOCIPlugin },
        { "Impl_HibernateEmptyAppInstanceId",                                        Test_Impl_HibernateEmptyAppInstanceId },
        { "Impl_HibernateNoOCIPlugin",                                               Test_Impl_HibernateNoOCIPlugin },
        { "Impl_WakeEmptyAppInstanceId",                                             Test_Impl_WakeEmptyAppInstanceId },
        { "Impl_WakeNoRunningContainer",                                             Test_Impl_WakeNoRunningContainer },
        { "Impl_SuspendEmptyAppInstanceId",                                          Test_Impl_SuspendEmptyAppInstanceId },
        { "Impl_SuspendNoOCIPlugin",                                                 Test_Impl_SuspendNoOCIPlugin },
        { "Impl_ResumeEmptyAppInstanceId",                                           Test_Impl_ResumeEmptyAppInstanceId },
        { "Impl_ResumeNoOCIPlugin",                                                  Test_Impl_ResumeNoOCIPlugin },
        { "Impl_AnnotateEmptyAppInstanceId",                                         Test_Impl_AnnotateEmptyAppInstanceId },
        { "Impl_AnnotateEmptyKey",                                                   Test_Impl_AnnotateEmptyKey },
        { "Impl_AnnotateNoOCIPlugin",                                                Test_Impl_AnnotateNoOCIPlugin },
        { "Impl_GetInfoEmptyAppInstanceId",                                          Test_Impl_GetInfoEmptyAppInstanceId },
        { "Impl_GetInfoNoOCIPlugin",                                                 Test_Impl_GetInfoNoOCIPlugin },
        { "Impl_RunEmptyAppInstanceId",                                              Test_Impl_RunEmptyAppInstanceId },
        { "Impl_RunNoWindowManagerConnector",                                        Test_Impl_RunNoWindowManagerConnector },
        { "Impl_MountReturnsSuccess",                                                Test_Impl_MountReturnsSuccess },
        { "Impl_UnmountReturnsSuccess",                                              Test_Impl_UnmountReturnsSuccess },
        { "Impl_GetInstanceReturnsSelf",                                             Test_Impl_GetInstanceReturnsSelf },
        { "Impl_GetInstanceBeforeConstruction",                                      Test_Impl_GetInstanceBeforeConstruction },
        { "Impl_OCIContainerStartedEvent",                                           Test_Impl_OCIContainerStartedEvent },
        { "Impl_OCIContainerStoppedEvent",                                           Test_Impl_OCIContainerStoppedEvent },
        { "Impl_OCIContainerFailureEvent",                                           Test_Impl_OCIContainerFailureEvent },
        { "Impl_OCIContainerStateChangedEvent",                                      Test_Impl_OCIContainerStateChangedEvent },
        { "Impl_OCIContainerAllEventsCoverage",                                      Test_Impl_OCIContainerAllEventsCoverage },
        { "Impl_OCIEventsWithRegisteredNotification",                                Test_Impl_OCIEventsWithRegisteredNotification },
        { "Impl_QueryInterfaceIRuntimeManager",                                      Test_Impl_QueryInterfaceIRuntimeManager },
        { "Impl_QueryInterfaceIConfiguration",                                       Test_Impl_QueryInterfaceIConfiguration },
        { "Impl_QueryInterfaceUnknownID",                                            Test_Impl_QueryInterfaceUnknownID },
        { "Impl_MultipleNotificationsRegisterUnregister",                            Test_Impl_MultipleNotificationsRegisterUnregister },
        { "Impl_DispatchContainerStartedFiresOnStartedCallback",                     Test_Impl_DispatchContainerStartedFiresOnStartedCallback },
        { "Impl_DispatchContainerStoppedFiresOnTerminatedCallback",                  Test_Impl_DispatchContainerStoppedFiresOnTerminatedCallback },
        { "Impl_DispatchContainerFailedFiresOnFailureCallback",                      Test_Impl_DispatchContainerFailedFiresOnFailureCallback },
        { "Impl_DispatchStateChangedFiresOnStateChangedCallback",                    Test_Impl_DispatchStateChangedFiresOnStateChangedCallback },
        { "Impl_DispatchPortalPrefixStrippingWithRegisteredNotification",            Test_Impl_DispatchPortalPrefixStrippingWithRegisteredNotification },
        { "Impl_DispatchMultipleNotificationsAllReceiveStartedEvent",                Test_Impl_DispatchMultipleNotificationsAllReceiveStartedEvent },

        // ── Component tests ──────────────────────────────────────────────────
        { "UserIdManager_GetUserIdReturnsValidRange",                                Test_UserIdManager_GetUserIdReturnsValidRange },
        { "UserIdManager_GetUserIdSameAppReturnsSameUid",                            Test_UserIdManager_GetUserIdSameAppReturnsSameUid },
        { "UserIdManager_GetUserIdDifferentAppsGetDifferentUids",                    Test_UserIdManager_GetUserIdDifferentAppsGetDifferentUids },
        { "UserIdManager_ClearUserIdReturnsUidToPool",                               Test_UserIdManager_ClearUserIdReturnsUidToPool },
        { "UserIdManager_ClearUserIdForUnknownAppDoesNotCrash",                      Test_UserIdManager_ClearUserIdForUnknownAppDoesNotCrash },
        { "UserIdManager_GetAppsGidReturns30000",                                    Test_UserIdManager_GetAppsGidReturns30000 },
        { "UserIdManager_ExhaustPoolReturnsZero",                                    Test_UserIdManager_ExhaustPoolReturnsZero },
        { "UserIdManager_MultipleGetAndClearCycles",                                 Test_UserIdManager_MultipleGetAndClearCycles },
        { "AIConfig_DefaultConsoleLogCap",                                           Test_AIConfig_DefaultConsoleLogCap },
        { "AIConfig_DefaultNonHomeAppMemoryLimit",                                   Test_AIConfig_DefaultNonHomeAppMemoryLimit },
        { "AIConfig_DefaultNonHomeAppGpuLimit",                                      Test_AIConfig_DefaultNonHomeAppGpuLimit },
        { "AIConfig_DefaultAppsCpuSet",                                              Test_AIConfig_DefaultAppsCpuSet },
        { "AIConfig_DefaultDialServerPort",                                          Test_AIConfig_DefaultDialServerPort },
        { "AIConfig_DefaultIonHeapQuota",                                            Test_AIConfig_DefaultIonHeapQuota },
        { "AIConfig_DefaultEnvVariablesNonEmpty",                                    Test_AIConfig_DefaultEnvVariablesNonEmpty },
        { "AIConfig_ResourceManagerClientDisabledByDefault",                         Test_AIConfig_ResourceManagerClientDisabledByDefault },
        { "AIConfig_SvpDisabledByDefault",                                           Test_AIConfig_SvpDisabledByDefault },
        { "AIConfig_IPv6DisabledByDefault",                                          Test_AIConfig_IPv6DisabledByDefault },
        { "AIConfig_PrintAIConfigurationDoesNotCrash",                               Test_AIConfig_PrintAIConfigurationDoesNotCrash },
        { "DobbyEventListener_ConstructionAndDestruction",                           Test_DobbyEventListener_ConstructionAndDestruction },
        { "DobbyEventListener_InitializeWithNullOCIPlugin",                          Test_DobbyEventListener_InitializeWithNullOCIPlugin },
        { "DobbyEventListener_DeinitializeWithoutInitialize",                        Test_DobbyEventListener_DeinitializeWithoutInitialize },
        { "DobbyEventListener_DoubleDeinitialize",                                   Test_DobbyEventListener_DoubleDeinitialize },
        { "WindowManagerConnector_ConstructionAndDestruction",                       Test_WindowManagerConnector_ConstructionAndDestruction },
        { "WindowManagerConnector_IsPluginInitializedReturnsFalseInitially",         Test_WindowManagerConnector_IsPluginInitializedReturnsFalseInitially },
        { "WindowManagerConnector_InitializeWithNullServiceReturnsFalse",            Test_WindowManagerConnector_InitializeWithNullServiceReturnsFalse },
        { "WindowManagerConnector_InitializeWithServiceMissingWindowManagerPlugin",  Test_WindowManagerConnector_InitializeWithServiceMissingWindowManagerPlugin },
        { "WindowManagerConnector_ReleasePluginWithoutInitDoesNotCrash",             Test_WindowManagerConnector_ReleasePluginWithoutInitDoesNotCrash },
        { "WindowManagerConnector_GetDisplayInfoWithoutInitDoesNotCrash",            Test_WindowManagerConnector_GetDisplayInfoWithoutInitDoesNotCrash },
        { "DobbySpecGenerator_GenerateWithValidConfig",                              Test_DobbySpecGenerator_GenerateWithValidConfig },
        { "DobbySpecGenerator_GenerateWithZeroUserId",                               Test_DobbySpecGenerator_GenerateWithZeroUserId },
        { "DobbySpecGenerator_GenerateWithEmptyCommand",                             Test_DobbySpecGenerator_GenerateWithEmptyCommand },
        { "DobbySpecGenerator_GenerateSpecContainsVersion",                          Test_DobbySpecGenerator_GenerateSpecContainsVersion },
        { "DobbySpecGenerator_GenerateSpecContainsArgs",                             Test_DobbySpecGenerator_GenerateSpecContainsArgs },
        { "DobbySpecGenerator_GenerateSpecContainsMemLimit",                         Test_DobbySpecGenerator_GenerateSpecContainsMemLimit },
        { "DobbySpecGenerator_ConstructionAndDestruction",                           Test_DobbySpecGenerator_ConstructionAndDestruction },
        { "DobbySpecGenerator_GenerateWithRuntimePath",                              Test_DobbySpecGenerator_GenerateWithRuntimePath },
        { "DobbySpecGenerator_GenerateWithoutRuntimePath",                           Test_DobbySpecGenerator_GenerateWithoutRuntimePath },
        { "DobbySpecGenerator_GenerateSpecContainsCpu",                              Test_DobbySpecGenerator_GenerateSpecContainsCpu },
        { "DobbySpecGenerator_GenerateSpecContainsNetwork",                          Test_DobbySpecGenerator_GenerateSpecContainsNetwork },
        { "DobbySpecGenerator_GenerateSpecContainsRdkPlugins",                       Test_DobbySpecGenerator_GenerateSpecContainsRdkPlugins },
        { "DobbySpecGenerator_GenerateWithMemLimitFromConfig",                       Test_DobbySpecGenerator_GenerateWithMemLimitFromConfig },
        { "AIConfig_GetGstreamerRegistryEnabledDefault",                             Test_AIConfig_GetGstreamerRegistryEnabledDefault },
        { "AIConfig_GetEnableUsbMassStorageDefault",                                 Test_AIConfig_GetEnableUsbMassStorageDefault },
        { "AIConfig_GetMapiPortsEmptyByDefault",                                     Test_AIConfig_GetMapiPortsEmptyByDefault },
        { "AIConfig_GetVpuAccessBlacklistEmptyByDefault",                            Test_AIConfig_GetVpuAccessBlacklistEmptyByDefault },
        { "AIConfig_GetAppsRequiringDBusEmptyByDefault",                             Test_AIConfig_GetAppsRequiringDBusEmptyByDefault },
        { "AIConfig_GetDialUsnEmptyByDefault",                                       Test_AIConfig_GetDialUsnEmptyByDefault },
        { "AIConfig_GetDialServerPathPrefixEmptyByDefault",                          Test_AIConfig_GetDialServerPathPrefixEmptyByDefault },
        { "AIConfig_GetPreloadsEmptyByDefault",                                      Test_AIConfig_GetPreloadsEmptyByDefault },
        { "AIConfig_GetIonHeapQuotasEmptyByDefault",                                 Test_AIConfig_GetIonHeapQuotasEmptyByDefault },
        { "WindowManagerConnector_CreateDisplayWithoutInitReturnsFalse",             Test_WindowManagerConnector_CreateDisplayWithoutInitReturnsFalse },
        { "WindowManagerConnector_GetDisplayInfoSetsNonEmptyDisplayName",            Test_WindowManagerConnector_GetDisplayInfoSetsNonEmptyDisplayName },

        // ── Additional coverage tests ────────────────────────────────────────
        { "DobbyEventListener_InitializeWithValidOCIContainerSucceeds",              Test_DobbyEventListener_InitializeWithValidOCIContainerSucceeds },
        { "DobbyEventListener_InitializeWithRegisterFailureReturnsFalse",            Test_DobbyEventListener_InitializeWithRegisterFailureReturnsFalse },
        { "DobbyEventListener_DeinitializeWithValidOCIContainerCallsUnregister",     Test_DobbyEventListener_DeinitializeWithValidOCIContainerCallsUnregister },
        { "DobbyEventListener_OnContainerStartedCallsEventHandler",                  Test_DobbyEventListener_OnContainerStartedCallsEventHandler },
        { "DobbyEventListener_OnContainerStoppedCallsEventHandler",                  Test_DobbyEventListener_OnContainerStoppedCallsEventHandler },
        { "DobbyEventListener_OnContainerFailedCallsEventHandler",                   Test_DobbyEventListener_OnContainerFailedCallsEventHandler },
        { "DobbyEventListener_OnContainerStateChangedStartingState",                 Test_DobbyEventListener_OnContainerStateChangedStartingState },
        { "DobbyEventListener_OnContainerStateChangedRunningState",                  Test_DobbyEventListener_OnContainerStateChangedRunningState },
        { "DobbyEventListener_OnContainerStateChangedAllStates",                     Test_DobbyEventListener_OnContainerStateChangedAllStates },
        { "DobbyEventListener_OnContainerStateChangedNullEventHandler",              Test_DobbyEventListener_OnContainerStateChangedNullEventHandler },
        { "WindowManagerConnector_InitializeWithValidWindowManagerSucceeds",         Test_WindowManagerConnector_InitializeWithValidWindowManagerSucceeds },
        { "WindowManagerConnector_ReleasePluginAfterInitCallsUnregister",            Test_WindowManagerConnector_ReleasePluginAfterInitCallsUnregister },
        { "WindowManagerConnector_CreateDisplayAfterInitReturnsTrue",                Test_WindowManagerConnector_CreateDisplayAfterInitReturnsTrue },
        { "WindowManagerConnector_CreateDisplayAfterInitReturnsFalseOnError",        Test_WindowManagerConnector_CreateDisplayAfterInitReturnsFalseOnError },
        { "WindowManagerConnector_OnUserInactivityDoesNotCrash",                     Test_WindowManagerConnector_OnUserInactivityDoesNotCrash },
        { "WindowManagerConnector_GetDisplayInfoWithXdgRuntimeDirSet",               Test_WindowManagerConnector_GetDisplayInfoWithXdgRuntimeDirSet },
        { "WindowManagerConnector_GetDisplayInfoWithoutXdgRuntimeDirFallsToTmp",     Test_WindowManagerConnector_GetDisplayInfoWithoutXdgRuntimeDirFallsToTmp },
        { "DobbySpecGenerator_GenerateWithSpecChangeFile",                           Test_DobbySpecGenerator_GenerateWithSpecChangeFile },
        { "DobbySpecGenerator_GenerateWithWanLanAccessFalse",                        Test_DobbySpecGenerator_GenerateWithWanLanAccessFalse },
        { "DobbySpecGenerator_GenerateWithAppPath",                                  Test_DobbySpecGenerator_GenerateWithAppPath },
        { "DobbySpecGenerator_GenerateThunderPluginEnabled",                         Test_DobbySpecGenerator_GenerateThunderPluginEnabled },
        { "DobbySpecGenerator_GenerateThunderPluginDisabled",                        Test_DobbySpecGenerator_GenerateThunderPluginDisabled },
        { "DobbySpecGenerator_GenerateWithEmptyWesterosSocket",                      Test_DobbySpecGenerator_GenerateWithEmptyWesterosSocket },
        { "DobbySpecGenerator_GenerateSysMemLimitZeroFallsBack",                     Test_DobbySpecGenerator_GenerateSysMemLimitZeroFallsBack },
        { "DobbySpecGenerator_GenerateWithNonEmptyAppPorts",                         Test_DobbySpecGenerator_GenerateWithNonEmptyAppPorts },
        { "DobbySpecGenerator_GetVpuEnabledReturnsFalseForSystemApp",                Test_DobbySpecGenerator_GetVpuEnabledReturnsFalseForSystemApp },
        { "DobbySpecGenerator_GenerateWithEnvVariablesInRuntimeConfig",              Test_DobbySpecGenerator_GenerateWithEnvVariablesInRuntimeConfig },

        // ── ralf/RalfSupport tests ───────────────────────────────────────────
        { "Ralf_ParseMemorySize_EmptyStringReturnsZero",                             Test_Ralf_ParseMemorySize_EmptyStringReturnsZero },
        { "Ralf_ParseMemorySize_PlainIntegerNoSuffix",                               Test_Ralf_ParseMemorySize_PlainIntegerNoSuffix },
        { "Ralf_ParseMemorySize_KilobytesSuffix",                                    Test_Ralf_ParseMemorySize_KilobytesSuffix },
        { "Ralf_ParseMemorySize_MegabytesSuffix",                                    Test_Ralf_ParseMemorySize_MegabytesSuffix },
        { "Ralf_ParseMemorySize_GigabytesSuffix",                                    Test_Ralf_ParseMemorySize_GigabytesSuffix },
        { "Ralf_ParseMemorySize_LowercaseSuffix",                                    Test_Ralf_ParseMemorySize_LowercaseSuffix },
        { "Ralf_ParseMemorySize_SuffixWithByte",                                     Test_Ralf_ParseMemorySize_SuffixWithByte },
        { "Ralf_ParseMemorySize_InvalidSuffixReturnsZero",                           Test_Ralf_ParseMemorySize_InvalidSuffixReturnsZero },
        { "Ralf_ParseMemorySize_NonNumericInputReturnsZero",                         Test_Ralf_ParseMemorySize_NonNumericInputReturnsZero },
        { "Ralf_ParseMemorySize_ZeroValueReturnsZero",                               Test_Ralf_ParseMemorySize_ZeroValueReturnsZero },
        { "Ralf_ParseMemorySize_KbSuffixWithB",                                      Test_Ralf_ParseMemorySize_KbSuffixWithB },
        { "Ralf_SerializeJsonNode_SimpleObject",                                     Test_Ralf_SerializeJsonNode_SimpleObject },
        { "Ralf_SerializeJsonNode_EmptyObject",                                      Test_Ralf_SerializeJsonNode_EmptyObject },
        { "Ralf_SerializeJsonNode_IntegerValue",                                     Test_Ralf_SerializeJsonNode_IntegerValue },
        { "Ralf_SerializeJsonNode_ArrayValue",                                       Test_Ralf_SerializeJsonNode_ArrayValue },
        { "Ralf_SerializeJsonNode_NoWhitespace",                                     Test_Ralf_SerializeJsonNode_NoWhitespace },
        { "Ralf_CheckIfPathExists_NonExistentPathReturnsFalse",                      Test_Ralf_CheckIfPathExists_NonExistentPathReturnsFalse },
        { "Ralf_CheckIfPathExists_ExistingPathReturnsTrue",                          Test_Ralf_CheckIfPathExists_ExistingPathReturnsTrue },
        { "Ralf_CheckIfPathExists_ExistingFileReturnsTrue",                          Test_Ralf_CheckIfPathExists_ExistingFileReturnsTrue },
        { "Ralf_CreateDirectories_EmptyPathReturnsFalse",                            Test_Ralf_CreateDirectories_EmptyPathReturnsFalse },
        { "Ralf_CreateDirectories_SingleLevelPath",                                  Test_Ralf_CreateDirectories_SingleLevelPath },
        { "Ralf_CreateDirectories_MultiLevelPath",                                   Test_Ralf_CreateDirectories_MultiLevelPath },
        { "Ralf_CreateDirectories_ExistingPathReturnsTrue",                          Test_Ralf_CreateDirectories_ExistingPathReturnsTrue },
        { "Ralf_JsonFromFile_NonExistentFileReturnsFalse",                           Test_Ralf_JsonFromFile_NonExistentFileReturnsFalse },
        { "Ralf_JsonFromFile_ValidJsonFileReturnsTrue",                              Test_Ralf_JsonFromFile_ValidJsonFileReturnsTrue },
        { "Ralf_JsonFromFile_InvalidJsonFileReturnsFalse",                           Test_Ralf_JsonFromFile_InvalidJsonFileReturnsFalse },
        { "Ralf_JsonFromFile_EmptyFileReturnsFalse",                                 Test_Ralf_JsonFromFile_EmptyFileReturnsFalse },
        { "Ralf_ParseRalPkgInfo_NonExistentFileReturnsFalse",                        Test_Ralf_ParseRalPkgInfo_NonExistentFileReturnsFalse },
        { "Ralf_ParseRalPkgInfo_MissingPackagesFieldReturnsFalse",                   Test_Ralf_ParseRalPkgInfo_MissingPackagesFieldReturnsFalse },
        { "Ralf_ParseRalPkgInfo_ValidPackagesFileReturnsTrue",                       Test_Ralf_ParseRalPkgInfo_ValidPackagesFileReturnsTrue },
        { "Ralf_ParseRalPkgInfo_EmptyPackagesArraySucceeds",                         Test_Ralf_ParseRalPkgInfo_EmptyPackagesArraySucceeds },
        { "Ralf_GetDevNodeMajorMinor_NonExistentNodeReturnsFalse",                   Test_Ralf_GetDevNodeMajorMinor_NonExistentNodeReturnsFalse },
        { "Ralf_GetDevNodeMajorMinor_DevNullReturnsTrue",                            Test_Ralf_GetDevNodeMajorMinor_DevNullReturnsTrue },
        { "Ralf_GetGroupId_UnknownGroupReturnsFalse",                                Test_Ralf_GetGroupId_UnknownGroupReturnsFalse },
        { "Ralf_GetGroupId_RootGroupReturnsTrue",                                    Test_Ralf_GetGroupId_RootGroupReturnsTrue },
        { "Ralf_GetRalfUserInfo_ReturnsConsistentResult",                            Test_Ralf_GetRalfUserInfo_ReturnsConsistentResult },

        // ── ralf/RalfPackageBuilder tests ────────────────────────────────────
        { "RalfPackageBuilder_ConstructionAndDestruction",                           Test_RalfPackageBuilder_ConstructionAndDestruction },
        { "RalfPackageBuilder_UnmountOverlayfsIfExists_NonExistentPathReturnsFalse", Test_RalfPackageBuilder_UnmountOverlayfsIfExists_NonExistentPathReturnsFalse },
        { "RalfPackageBuilder_UnmountOverlayfsIfExists_EmptyAppInstanceId",          Test_RalfPackageBuilder_UnmountOverlayfsIfExists_EmptyAppInstanceId },
        { "RalfPackageBuilder_GenerateRalfDobbySpec_EmptyRalfPkgPathReturnsFalse",   Test_RalfPackageBuilder_GenerateRalfDobbySpec_EmptyRalfPkgPathReturnsFalse },
        { "RalfPackageBuilder_GenerateRalfDobbySpec_NonExistentPkgFileReturnsFalse", Test_RalfPackageBuilder_GenerateRalfDobbySpec_NonExistentPkgFileReturnsFalse },
        { "RalfPackageBuilder_GenerateRalfDobbySpec_MalformedPkgFileReturnsFalse",   Test_RalfPackageBuilder_GenerateRalfDobbySpec_MalformedPkgFileReturnsFalse },
        { "RalfPackageBuilder_GenerateRalfDobbySpec_PkgFileMissingPackagesField",    Test_RalfPackageBuilder_GenerateRalfDobbySpec_PkgFileMissingPackagesFieldReturnsFalse },
        { "RalfPackageBuilder_GenerateRalfDobbySpec_OutputDobbySpecEmptyOnFailure",  Test_RalfPackageBuilder_GenerateRalfDobbySpec_OutputDobbySpecEmptyOnFailure },

        // ── ralf/RalfOCIConfigGenerator tests ────────────────────────────────
        { "RalfOCIConfigGenerator_ConstructionAndDestruction",                       Test_RalfOCIConfigGenerator_ConstructionAndDestruction },
        { "RalfOCIConfigGenerator_ConstructionWithEmptyPackages",                    Test_RalfOCIConfigGenerator_ConstructionWithEmptyPackages },
        { "RalfOCIConfigGenerator_ConstructionWithMultiplePackages",                 Test_RalfOCIConfigGenerator_ConstructionWithMultiplePackages },
        { "RalfOCIConfigGenerator_GenerateFailsWhenBaseSpecFileMissing",             Test_RalfOCIConfigGenerator_GenerateFailsWhenBaseSpecFileMissing },
        { "RalfOCIConfigGenerator_GenerateFailsWhenGraphicsConfigMissing",           Test_RalfOCIConfigGenerator_GenerateFailsWhenGraphicsConfigMissing },
        { "RalfOCIConfigGenerator_GenerateWithMockBaseSpec",                         Test_RalfOCIConfigGenerator_GenerateWithMockBaseSpec },
        { "RalfOCIConfigGenerator_MultipleGenerateCallsDoNotCrash",                  Test_RalfOCIConfigGenerator_MultipleGenerateCallsDoNotCrash },
        { "RalfOCIConfigGenerator_GenerateWithDifferentAppInstances",                Test_RalfOCIConfigGenerator_GenerateWithDifferentAppInstances },

        // ── Gateway/ContainerUtils tests ─────────────────────────────────────
        { "ContainerUtils_GetContainerIpAddress_UnknownContainerReturnsZero",        Test_ContainerUtils_GetContainerIpAddress_UnknownContainerReturnsZero },
        { "ContainerUtils_GetContainerIpAddress_EmptyContainerIdReturnsZero",        Test_ContainerUtils_GetContainerIpAddress_EmptyContainerIdReturnsZero },
        { "ContainerUtils_GetContainerIpAddress_MultipleCallsDoNotCrash",            Test_ContainerUtils_GetContainerIpAddress_MultipleCallsDoNotCrash },
        { "ContainerUtils_NamespaceEnumValues",                                      Test_ContainerUtils_NamespaceEnumValues },
        { "ContainerUtils_GetContainerIpAddress_VeryLongContainerIdDoesNotCrash",    Test_ContainerUtils_GetContainerIpAddress_VeryLongContainerIdDoesNotCrash },

        // ── Gateway/WebInspector tests ────────────────────────────────────────
        { "Debugger_TypeEnum_WebInspectorValue",                                     Test_Debugger_TypeEnum_WebInspectorValue },
        { "WebInspector_Attach_WithInvalidIpReturnsNullptrOrNonNull",                Test_WebInspector_Attach_WithInvalidIpReturnsNullptrOrNonNull },
        { "WebInspector_Attach_ZeroIpReturnsNullptrOrNonNull",                       Test_WebInspector_Attach_ZeroIpReturnsNullptrOrNonNull },
        { "WebInspector_Attach_EmptyAppIdDoesNotCrash",                              Test_WebInspector_Attach_EmptyAppIdDoesNotCrash },
        { "WebInspector_Attach_MultipleCallsDoNotCrash",                             Test_WebInspector_Attach_MultipleCallsDoNotCrash },
        { "WebInspector_TypeReturnsWebInspector",                                    Test_WebInspector_TypeReturnsWebInspector },
        { "WebInspector_DestructorRunsCleanupWithoutCrash",                          Test_WebInspector_DestructorRunsCleanupWithoutCrash },

        // ── Gateway/NetFilter & NetFilterLock tests ───────────────────────────
        { "NetFilterLock_ConstructionAndDestruction",                                Test_NetFilterLock_ConstructionAndDestruction },
        { "NetFilterLock_TryLockDoesNotCrash",                                       Test_NetFilterLock_TryLockDoesNotCrash },
        { "NetFilterLock_LockUnlockDoesNotCrash",                                    Test_NetFilterLock_LockUnlockDoesNotCrash },
        { "NetFilterLock_TryLockForZeroDurationDoesNotCrash",                        Test_NetFilterLock_TryLockForZeroDurationDoesNotCrash },
        { "NetFilterLock_MultipleInstancesDoNotCrash",                               Test_NetFilterLock_MultipleInstancesDoNotCrash },
        { "NetFilter_Protocol_TcpAndUdpValuesDistinct",                              Test_NetFilter_Protocol_TcpAndUdpValuesDistinct },
        { "NetFilter_RemoveAllRulesMatchingComment_EmptyCommentDoesNotCrash",        Test_NetFilter_RemoveAllRulesMatchingComment_EmptyCommentDoesNotCrash },
        { "NetFilter_RemoveAllRulesMatchingComment_NoMatchCommentDoesNotCrash",      Test_NetFilter_RemoveAllRulesMatchingComment_NoMatchCommentDoesNotCrash },
        { "NetFilter_OpenExternalPort_DoesNotCrash",                                 Test_NetFilter_OpenExternalPort_DoesNotCrash },
        { "NetFilter_AddContainerPortForwarding_DoesNotCrash",                       Test_NetFilter_AddContainerPortForwarding_DoesNotCrash },
        { "NetFilter_AddContainerPortForwarding_ReturnsFalseOrTrue",                 Test_NetFilter_AddContainerPortForwarding_ReturnsFalseOrTrue },
        { "NetFilter_GetContainerPortForwardList_ReturnsListType",                   Test_NetFilter_GetContainerPortForwardList_ReturnsListType },
        { "NetFilter_GetContainerPortForwardList_DefaultIpReturnsListType",          Test_NetFilter_GetContainerPortForwardList_DefaultIpReturnsListType },
        { "NetFilter_PortForward_StructMembersAccessible",                           Test_NetFilter_PortForward_StructMembersAccessible },
    };

    uint32_t failures = 0;

    for (const auto& c : cases) {
        std::cerr << "[ RUN      ] " << c.name << std::endl;
        const uint32_t f = c.fn();
        if (f == 0) {
            std::cerr << "[       OK ] " << c.name << std::endl;
        } else {
            std::cerr << "[  FAILED  ] " << c.name << " failures=" << f << std::endl;
        }
        failures += f;
    }

    L0Test::PrintTotals(std::cout, "RuntimeManager l0test", failures);
    return L0Test::ResultToExitCode(failures);
}
