# PackageManager L0 Test Plan

## Goal
Create L0 coverage for PackageManager that follows the existing L0 style used by LifecycleManager, RDKWindowManager, and RuntimeManager, while treating libpackage-sky as an external dependency boundary.

## Scope
- Component under test:
  - PackageManager/PackageManager.cpp
  - PackageManager/PackageManagerImplementation.cpp
  - PackageManager/PackageManagerTelemetryReporting.cpp
  - PackageManager/HttpClient.cpp
- L0 framework style to reuse:
  - Tests/L0Tests/common/L0Bootstrap.hpp
  - Tests/L0Tests/common/L0Expect.hpp
  - Tests/L0Tests/common/L0ServiceMock.hpp
  - Tests/L0Tests/common/L0TestTypes.hpp
- Behavior references:
  - Tests/L0Tests/LifecycleManager/*
  - Tests/L0Tests/RDKWindowManager/*
  - Tests/L0Tests/RuntimeManager/*
- External package reference boundary:
  - ref/libpackage-sky/

## Test Design Principles
- Keep L0 tests deterministic and component-local.
- Mock Thunder shell dependencies and AppStorageManager interactions.
- Do not test libpackage-sky internals in L0; validate PackageManager behavior at the interface boundary.
- Prefer explicit state-transition checks and notification delivery checks.
- Use fast wait/poll helpers only for asynchronous paths.

## Proposed Test Folder Layout
Create a new folder:
- Tests/L0Tests/PackageManager/

Proposed files:
- PackageManagerTest.cpp
- PackageManager_ShellTests.cpp
- PackageManager_ImplementationTests.cpp
- PackageManager_NotificationTests.cpp
- ServiceMock.h

## Common Fixtures and Mocks
### Service Mock
Implement a PackageManager-specific service mock patterned after:
- Tests/L0Tests/RuntimeManager/ServiceMock.h
- Tests/L0Tests/LifecycleManager/LifecycleManagerServiceMock.h

Capabilities needed:
- QueryInterfaceByCallsign for org.rdk.AppStorageManager
- ConfigLine() for downloadDir and optional config variants
- Subsystem and register/unregister tracking where required

### Fake AppStorageManager
Provide a fake/stub implementing IAppStorageManager methods used by PackageManagerImplementation:
- success path responses
- failure path responses
- null/absent service behavior

### Notification Fakes
Provide two counters/event capture fakes:
- IPackageDownloader::INotification
- IPackageInstaller::INotification

Track:
- callback invocation count
- last payload values (id/version/reason/fileLocator)
- registration/unregistration correctness

## Test Matrix
### A) Shell/Lifecycle
1. Constructor/Destructor basic safety.
2. Initialize success with valid shell and config.
3. Initialize with missing AppStorageManager.
4. Initialize with malformed config line.
5. Deinitialize idempotency and resource cleanup.
6. Information() returns expected plugin info.

### B) Downloader API
1. Register/Unregister notification success.
2. Duplicate register handling.
3. Unregister unknown/null notification.
4. Download with valid URL/options returns downloadId.
5. Download with invalid URL fails gracefully.
6. Pause/Resume/Cancel with invalid id.
7. Pause/Resume/Cancel with known id.
8. Delete with invalid/valid fileLocator.
9. Progress for known and unknown downloads.
10. GetStorageInformation with storage manager success/failure.
11. RateLimit with valid and invalid arguments.

### C) Installer API
1. Install success (mocked package boundary).
2. Install fail reason mapping coverage:
   - signature verification failure
   - package mismatch failure
   - invalid metadata failure
   - persistence failure
   - version not found/general failure
3. Uninstall success/failure.
4. ListPackages empty/non-empty.
5. Config(packageId, version) valid/invalid.
6. PackageState for installed/uninstalled/blocked/failure states.
7. GetConfigForPackage valid/invalid locator.

### D) Handler API
1. Lock success for valid package/version.
2. Lock failure for unknown package/version.
3. Unlock success/failure.
4. GetLockedInfo for locked/unlocked/not-found.
5. Additional lock metadata behavior.

### E) Notification and Event Dispatch
1. OnAppDownloadStatus delivered to all registered downloader listeners.
2. OnAppInstallationStatus delivered to all registered installer listeners.
3. No callback after unregister.
4. Multi-listener sequencing and payload integrity.

### F) Concurrency/Robustness (L0-safe)
1. Interleaved register/unregister with download events.
2. Repeated rapid Download/Pause/Resume/Cancel on same id.
3. Concurrent read APIs (Progress/PackageState) under state changes.

## libpackage-sky Boundary Strategy
Use ref/libpackage-sky only as API/behavior reference.

L0 approach:
- Mock or substitute package implementation interfaces consumed by PackageManagerImplementation.
- Validate error/state mapping and event behavior from PackageManager outward.
- Avoid filesystem/archive/signature deep validation of libpackage-sky internals in L0.

## CMake Integration Plan
Update Tests/L0Tests/CMakeLists.txt:
1. Add option:
   - PLUGIN_PACKAGE_MANAGER
2. Add source list:
   - PackageManager plugin sources + test sources + common/L0Bootstrap.cpp
3. Add executable:
   - packagemanager_l0test
4. Add compile definitions aligned with existing targets:
   - MODULE_NAME=PackageManager_L0Test
   - USE_THUNDER_R4=1
   - DISABLE_SECURITY_TOKEN=1
   - THUNDER_PLATFORM_PC_UNIX=1
   - UNIT_TEST=1 (if required by PackageManagerImplementation test path)
5. Include directories and library links analogous to current L0 target patterns.
6. Add optional coverage flags following APPMGR/APPLCM/APPWM model.

## Execution Plan
1. Add scaffolding files and minimal main runner in PackageManagerTest.cpp.
2. Implement shell tests first.
3. Implement downloader/installer/handler suites incrementally.
4. Add notification/concurrency tests.
5. Build target:
   - cmake -S Tests/L0Tests -B Tests/L0Tests/build
   - cmake --build Tests/L0Tests/build --target packagemanager_l0test
6. Execute:
   - Tests/L0Tests/build/packagemanager_l0test
7. Iterate until stable zero-failure run.

## Milestones and Exit Criteria
### Milestone 1
- PackageManager L0 target compiles and runs with baseline shell tests.

### Milestone 2
- Downloader/Installer/Handler API coverage with success and negative paths.

### Milestone 3
- Notification and concurrency matrix implemented.

### Exit Criteria
- packagemanager_l0test exits with 0 failures.
- All key API groups covered (lifecycle, downloader, installer, handler, notifications).
- No dependency on libpackage-sky internals beyond mocked boundary behavior.
- Coverage trend comparable with existing mature L0 components.

## Risks and Mitigations
1. Async flakiness from worker/thread timing.
   - Mitigation: bounded wait helper and deterministic fake callbacks.
2. Tight coupling to external package implementation details.
   - Mitigation: strict interface-boundary mocks.
3. Service mock incompleteness.
   - Mitigation: start from RuntimeManager/LifecycleManager mock patterns and extend minimally.
