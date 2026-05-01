# PreinstallManager L1 Coverage Gap Analysis

## Scope
- Coverage artifacts analyzed from: `ref/coverage/PreinstallManager`
- Source analyzed from: `PreinstallManager/`
- Current L1 test analyzed: `Tests/L1Tests/tests/test_PreinstallManager.cpp`

## Coverage Snapshot (from LCOV)
- PreinstallManager overall: 56.5% lines (173/306), 60.3% functions (38/63)
- `PreinstallManager.cpp`: 72.6% lines (45/62), 54.5% functions (6/11)
- `PreinstallManager.h`: 54.2% lines (13/24), 50.0% functions (6/12)
- `PreinstallManagerImplementation.cpp`: 47.3% lines (89/188), 75.0% functions (15/20)
- `PreinstallManagerImplementation.h`: 83.9% lines (26/31), 61.1% functions (11/18)
- `Module.cpp`: 0.0% lines (0/1), 0.0% functions (0/2)

---

## Key Root Cause Summary
The single biggest reason for low coverage is that most `StartPreinstall()` execution paths are never reached.

In the current test setup:
- `ServiceMock::ConfigLine()` is not configured, so `Configure()` keeps `mAppPreinstallDirectory` empty.
- `SetUpPreinstallDirectoryMocks()` uses `__real_opendir(pathname)`, so when `pathname` is empty, `opendir` fails.
- `readPreinstallDirectory()` returns false early.
- `StartPreinstall()` exits before:
  - package filtering logic,
  - semantic version comparison,
  - install success/failure handling,
  - summary/finalization paths.

This explains why many lines in `PreinstallManagerImplementation.cpp` are uncovered even though `StartPreinstall()` is called in two tests.

---

## Uncovered Functions (Actionable)

### `PreinstallManager.cpp`
1. `PreinstallManager::Information()`
2. `PreinstallManager::Deactivated(RPC::IRemoteConnection*)`

Reason:
- No test invokes `Information()`.
- No test simulates remote-process deactivation callback path.

Complexity:
- `Information()`: Low
- `Deactivated()`: Medium (needs connection id alignment + workerpool submission path)

### `PreinstallManagerImplementation.cpp`
1. `PreinstallManagerImplementation::isNewerVersion(const std::string&, const std::string&)`
2. `PreinstallManagerImplementation::getFailReason(FailReason)`

Reason:
- Both are reached only through deeper `StartPreinstall(false)` and install failure paths, currently short-circuited.

Complexity:
- `isNewerVersion`: Medium (multiple parsing and compare branches)
- `getFailReason`: Low

### `PreinstallManagerImplementation.h` (inline callback)
1. `PackageManagerNotification::OnAppInstallationStatus(const string&)`

Reason:
- Tests call `handleOnAppInstallationStatus()` directly, but do not trigger callback through the package-installer sink registered by `createPackageManagerObject()`.

Complexity:
- Medium (requires using captured package-installer notification sink)

### `Module.cpp`
1. `ModuleBuildRef`
2. `ModuleServiceMetadata`

Reason:
- Build/registration translation unit not exercised by unit test runtime path.

Complexity:
- Low (often intentionally uncovered in unit tests)

---

## Uncovered Lines by Source File (Actionable Groups)

## `PreinstallManager/PreinstallManager.cpp`
- Initialize error branches: lines **89-90, 94-95, 106-107, 113-114, 119**
- Deinitialize remote connection branch: lines **163-164**
- `Information()` path: lines **174, 177**
- `Deactivated()` path: lines **180, 182, 185, 187**

Why not covered:
- Current tests only validate success-oriented initialization/deinitialization and normal implementation availability.
- No fault-injection around `Root<>`, `QueryInterface<IConfiguration>`, `Configure()`, or null service.
- No remote connection object is returned for terminate/release branch.

Complexity:
- Medium (lifecycle fault injection + mock interaction ordering)

## `PreinstallManager/PreinstallManagerImplementation.cpp`
- Notification unregister-miss path: line **106**
- Configure special branches: lines **128** (config value present), **136** (null service)
- Event dispatch error branches: lines **155**, **169-171**, **188**
- Package manager creation null-service branch: line **198**
- Version compare logic: lines **226, 229-256**
- Directory traversal and package extraction main path: lines **283-310**
- Fail-reason mapping: lines **313-319**
- `StartPreinstall(false)` and installation flow: lines **353-487** (multiple uncovered branches)

Why not covered:
- Empty preinstall directory path leads to early return before core logic.
- No tests for malformed/empty event payloads.
- No tests for `ListPackages` failure/null iterator.
- No tests for install success/failure outcomes and fail-reason mapping.
- No tests that force version-upgrade/skip decisions.

Complexity:
- High (largest function, multi-branch, stateful behavior, and several mock-dependent loops)

## `PreinstallManager/PreinstallManager.h`
- Notification activation/deactivation callbacks: lines **63-72**

Why not covered:
- Plugin notification sink exists but callback entry points are not explicitly exercised.

Complexity:
- Medium (needs lifecycle callback stimulation)

## `PreinstallManager/PreinstallManagerImplementation.h`
- PackageManager callback inline path: lines **104, 107-108**

Why not covered:
- Package-manager event flow is not executed via registered sink object.

Complexity:
- Medium

## `PreinstallManager/Module.cpp`
- Module declaration line: **22**

Why not covered:
- Not usually reached by unit test runtime behavior.

Complexity:
- Low

---

## Non-Actionable / Low-Value Coverage Gaps
Some uncovered entries are ABI-generated constructor/destructor variants (`C1`, `D0`, `D1`, etc.) and macro-expansion artifacts from interface maps. These are generally low value for direct unit targeting and can be deprioritized.

---

## How To Improve Coverage (Concrete Additional L1 Tests)

## A. Lifecycle and plugin-level branches (`PreinstallManager.cpp`)
1. `Initialize_RootCreationFails_ReturnsError`
- Mock `Root<Exchange::IPreinstallManager>()` to return null.
- Validate returned error string and cleanup behavior.

2. `Initialize_QueryInterfaceConfigurationFails_ReturnsError`
- Return implementation, but make `QueryInterface<Exchange::IConfiguration>()` return null.

3. `Initialize_ConfigureFails_ReturnsError`
- Return mock IConfiguration with `Configure()` failure.

4. `Deinitialize_WithRemoteConnection_TerminatesAndReleases`
- Return non-null remote connection object from shell/COMLink path and expect `Terminate()` + `Release()`.

5. `Information_ReturnsEmptyString`
- Directly call and validate empty output.

6. `Deactivated_WhenConnectionIdMatches_SubmitsDeactivationJob`
- Simulate callback with matching connection id.
- Verify submission behavior using workerpool harness expectations.

## B. Implementation behavior (`PreinstallManagerImplementation.cpp`)
7. `Configure_WithAppPreinstallDirectory_ParsesAndStores`
- Return config line containing `appPreinstallDirectory`.
- Ensure later directory scan uses configured path.

8. `Configure_NullService_ReturnsError`
- Call `Configure(nullptr)` and validate error branch.

9. `Dispatch_MissingJsonResponse_LogsErrorPath`
- Call `Dispatch(PREINSTALL_MANAGER_APP_INSTALLATION_STATUS, paramsWithoutLabel)`.

10. `Dispatch_UnknownEvent_DefaultBranch`
- Call `Dispatch(PREINSTALL_MANAGER_UNKNOWN, params)`.

11. `HandleOnAppInstallationStatus_EmptyString`
- Call with empty string to cover line 188 branch.

12. `StartPreinstall_ForceFalse_ListPackagesFailure`
- Make `ListPackages` fail and verify early return branch.

13. `StartPreinstall_ForceFalse_NonNewerVersion_SkipsInstall`
- Provide installed version >= preinstall version and validate removal path.

14. `StartPreinstall_ForceFalse_NewerVersion_Installs`
- Provide installed lower version and validate install path.

15. `StartPreinstall_InstallFailure_UsesFailReasonMapping`
- Make `Install()` fail with each fail reason enum to hit `getFailReason()` cases.

16. `StartPreinstall_InvalidVersionFormat_CoversVersionParseFailure`
- Provide malformed version to cover `isNewerVersion()` parse-fail branches.

17. `StartPreinstall_EmptyPackageFields_SkipAndCountFailure`
- Return package with missing fields to cover empty-field branch.

18. `ReadPreinstallDirectory_GetConfigFails_MarksSkipped`
- Mock directory entries and make `GetConfigForPackage` fail on one entry.

19. `ReadPreinstallDirectory_Success_PathWithDotEntries`
- Include `.`, `..`, and valid entries in mocked readdir sequence.

20. `PackageManagerNotification_Callback_ThroughRegisteredSink`
- Capture sink from `Register()` and invoke `OnAppInstallationStatus()` through sink object instead of direct implementation call.

## C. Module translation unit
21. Optional: Add minimal module-load smoke test if framework test harness supports it.
- Otherwise treat as acceptable uncovered code in L1 unit scope.

---

## Mock Strategy: Reuse and Additions

## Reuse from existing project mocks
Already present in this repo under `Tests/mocks`:
- `ServiceMock.h`
- `PackageManagerMock.h`
- `COMLinkMock.h`
- `WrapsMock.h`

These are sufficient for most new tests.

## Reuse from `ref/entservices-testframework` (if syncing needed)
- `ref/entservices-testframework/Tests/mocks/thunder/ServiceMock.h`
- `ref/entservices-testframework/Tests/mocks/PackageManagerMock.h`
- `ref/entservices-testframework/Tests/mocks/thunder/COMLinkMock.h`
- `ref/entservices-testframework/Tests/mocks/PlayerInfoMock.h` (contains a reusable `MockRemoteConnection`)

## Recommended new mock to add locally
Add a focused mock file for clarity and dependency isolation:
- `Tests/mocks/RemoteConnectionMock.h`

Proposed interface coverage in this mock:
- `Id()`
- `Terminate()`
- `Release()`
- minimal other pure virtuals from `RPC::IRemoteConnection`

This allows reliable tests for:
- `PreinstallManager::Deinitialize()` remote connection branch
- `PreinstallManager::Deactivated()` matching connection-id branch

---

## Priority Plan
1. Fix preinstall-path setup in tests (set `ConfigLine()` with `appPreinstallDirectory`) and stop relying on `__real_opendir`.
2. Add `StartPreinstall(false)` branch tests first (largest uncovered area).
3. Add install failure + fail reason tests (`getFailReason`).
4. Add lifecycle fault-injection tests in `PreinstallManager.cpp`.
5. Add callback path tests through package-installer sink and remote deactivation path.

Expected impact:
- Major line coverage gain in `PreinstallManagerImplementation.cpp` (currently 47.3%).
- Better function coverage in `PreinstallManager.cpp` and header-inline callback code.
