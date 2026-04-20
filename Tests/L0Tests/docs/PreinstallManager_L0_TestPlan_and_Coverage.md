# PreinstallManager L0 Unit Test Plan and Coverage Strategy

## Scope

This document defines comprehensive L0 unit test scenarios for the `PreinstallManager` plugin, including:

- Positive scenarios
- Negative scenarios
- Boundary scenarios
- Method-level test mapping
- Coverage target plan (minimum 75% across `PreinstallManager` plugin `.cpp` and testable `.h` logic)
- Explicit list of methods/areas not covered in L0 with rationale
- Guidance for generating L0 code using `Tests/L0Tests/RuntimeManager` as reference

Plugin files in scope:

- `PreinstallManager/PreinstallManager.cpp`
- `PreinstallManager/PreinstallManager.h`
- `PreinstallManager/PreinstallManagerImplementation.cpp`
- `PreinstallManager/PreinstallManagerImplementation.h`
- `PreinstallManager/Module.cpp`
- `PreinstallManager/Module.h`

## Coverage Goal

- Minimum target: **75% line coverage** and **75% function coverage** across unit-testable plugin logic.
- Practical target with this plan: **80%+ line coverage** for `.cpp` files and **85%+ function coverage** excluding non-testable macro/ABI lines.

## Test Architecture (L0)

Reference pattern to reuse from `Tests/L0Tests/RuntimeManager`:

- Test layout style:
  - `Tests/L0Tests/PreinstallManager/PreinstallManagerTest.cpp` (main)
  - `Tests/L0Tests/PreinstallManager/PreinstallManager_LifecycleTests.cpp`
  - `Tests/L0Tests/PreinstallManager/PreinstallManager_ImplementationTests.cpp`
  - `Tests/L0Tests/PreinstallManager/PreinstallManager_ComponentTests.cpp`
  - `Tests/L0Tests/PreinstallManager/ServiceMock.h`
- Common test bootstrapping:
  - Reuse `Tests/L0Tests/common/L0Bootstrap.cpp/.hpp`
  - Reuse `Tests/L0Tests/common/L0Expect.hpp`
- Build integration:
  - Extend `Tests/L0Tests/CMakeLists.txt` source list with PreinstallManager production and test files.
- Mocking strategy:
  - Mock `PluginHost::IShell`
  - Mock `Exchange::IPackageInstaller`
  - Mock `IPackageIterator`
  - Mock `Exchange::IPreinstallManager::INotification`
  - Stub worker pool/event dispatch observation where required.

## Quick Summary Table (Test Case to File/Method)

| Test ID | Short Summary | File(s) | Method(s) Covered |
|---|---|---|---|
| PIM-POS-001 | Initialize succeeds with valid service/root/configure | `PreinstallManager.cpp` | `PreinstallManager::Initialize` |
| PIM-POS-002 | Initialize registers JSON-RPC and notifications | `PreinstallManager.cpp` | `PreinstallManager::Initialize` |
| PIM-NEG-001 | Initialize fails when Root returns null | `PreinstallManager.cpp` | `PreinstallManager::Initialize`, `Deinitialize` |
| PIM-NEG-002 | Initialize fails when IConfiguration missing | `PreinstallManager.cpp` | `PreinstallManager::Initialize`, `Deinitialize` |
| PIM-NEG-003 | Initialize fails when Configure fails | `PreinstallManager.cpp` | `PreinstallManager::Initialize`, `Deinitialize` |
| PIM-POS-003 | Deinitialize releases implementation/config/connection | `PreinstallManager.cpp` | `PreinstallManager::Deinitialize` |
| PIM-POS-004 | Information returns empty string | `PreinstallManager.cpp` | `PreinstallManager::Information` |
| PIM-POS-005 | Deactivated submits DEACTIVATED job for matching connection | `PreinstallManager.cpp` | `PreinstallManager::Deactivated` |
| PIM-NEG-004 | Deactivated ignores non-matching connection id | `PreinstallManager.cpp` | `PreinstallManager::Deactivated` |
| PIM-POS-006 | Notification forwards OnPreinstallationComplete | `PreinstallManager.h` | `Notification::OnPreinstallationComplete` |
| PIM-POS-007 | Notification forwards OnAppInstallationStatus | `PreinstallManager.h` | `Notification::OnAppInstallationStatus` |
| PIM-POS-008 | Register adds new observer (unique) | `PreinstallManagerImplementation.cpp` | `Register` |
| PIM-BND-001 | Register duplicate observer ignored | `PreinstallManagerImplementation.cpp` | `Register` |
| PIM-POS-009 | Unregister existing observer succeeds | `PreinstallManagerImplementation.cpp` | `Unregister` |
| PIM-NEG-005 | Unregister missing observer returns ERROR_GENERAL | `PreinstallManagerImplementation.cpp` | `Unregister` |
| PIM-POS-010 | Configure parses appPreinstallDirectory | `PreinstallManagerImplementation.cpp` | `Configure` |
| PIM-NEG-006 | Configure null service fails | `PreinstallManagerImplementation.cpp` | `Configure` |
| PIM-POS-011 | createPackageManagerObject success | `PreinstallManagerImplementation.cpp` | `createPackageManagerObject` |
| PIM-NEG-007 | createPackageManagerObject fails when service null | `PreinstallManagerImplementation.cpp` | `createPackageManagerObject` |
| PIM-NEG-008 | createPackageManagerObject fails when query returns null | `PreinstallManagerImplementation.cpp` | `createPackageManagerObject` |
| PIM-POS-012 | releasePackageManagerObject null-safe and release path | `PreinstallManagerImplementation.cpp` | `releasePackageManagerObject` |
| PIM-POS-013 | readPreinstallDirectory loads valid packages | `PreinstallManagerImplementation.cpp` | `readPreinstallDirectory` |
| PIM-NEG-009 | readPreinstallDirectory fails when directory open fails | `PreinstallManagerImplementation.cpp` | `readPreinstallDirectory` |
| PIM-BND-002 | readPreinstallDirectory skips dot entries only | `PreinstallManagerImplementation.cpp` | `readPreinstallDirectory` |
| PIM-POS-014 | getFailReason maps all enums correctly | `PreinstallManagerImplementation.cpp` | `getFailReason` |
| PIM-BND-003 | getFailReason default branch returns NONE | `PreinstallManagerImplementation.cpp` | `getFailReason` |
| PIM-POS-015 | isNewerVersion major/minor/patch/build comparisons | `PreinstallManagerImplementation.cpp` | `isNewerVersion` |
| PIM-NEG-010 | isNewerVersion invalid version input returns false | `PreinstallManagerImplementation.cpp` | `isNewerVersion` |
| PIM-BND-004 | isNewerVersion handles prerelease/build suffix stripping | `PreinstallManagerImplementation.cpp` | `isNewerVersion` |
| PIM-POS-016 | StartPreinstall(force=true) spawns install thread | `PreinstallManagerImplementation.cpp` | `StartPreinstall`, `installPackages` |
| PIM-POS-017 | StartPreinstall(force=false) filters same/older installed versions | `PreinstallManagerImplementation.cpp` | `StartPreinstall`, `isNewerVersion` |
| PIM-POS-018 | StartPreinstall force=false keeps newer package for install | `PreinstallManagerImplementation.cpp` | `StartPreinstall`, `isNewerVersion` |
| PIM-NEG-011 | StartPreinstall returns error when package manager creation fails | `PreinstallManagerImplementation.cpp` | `StartPreinstall` |
| PIM-NEG-012 | StartPreinstall returns error when directory read fails | `PreinstallManagerImplementation.cpp` | `StartPreinstall` |
| PIM-POS-019 | StartPreinstall handles empty directory and sends completion event | `PreinstallManagerImplementation.cpp` | `StartPreinstall`, `sendOnPreinstallationCompleteEvent` |
| PIM-POS-020 | StartPreinstall all filtered-out path sends completion event | `PreinstallManagerImplementation.cpp` | `StartPreinstall`, `sendOnPreinstallationCompleteEvent` |
| PIM-NEG-013 | StartPreinstall when IN_PROGRESS returns ERROR_GENERAL | `PreinstallManagerImplementation.cpp` | `StartPreinstall` |
| PIM-BND-005 | StartPreinstall joins previous completed thread before relaunch | `PreinstallManagerImplementation.cpp` | `StartPreinstall` |
| PIM-POS-021 | installPackages success updates states and emits event | `PreinstallManagerImplementation.cpp` | `installPackages`, `GetPreinstallState` |
| PIM-NEG-014 | installPackages handles invalid package fields | `PreinstallManagerImplementation.cpp` | `installPackages` |
| PIM-NEG-015 | installPackages handles install failure with reason | `PreinstallManagerImplementation.cpp` | `installPackages`, `getFailReason` |
| PIM-NEG-016 | installPackages createPackageManagerObject failure exits safely | `PreinstallManagerImplementation.cpp` | `installPackages` |
| PIM-POS-022 | Dispatch known event notifies all observers | `PreinstallManagerImplementation.cpp` | `Dispatch` |
| PIM-NEG-017 | Dispatch unknown event path logs error | `PreinstallManagerImplementation.cpp` | `Dispatch` |
| PIM-POS-023 | sendOnPreinstallationCompleteEvent enqueues worker job | `PreinstallManagerImplementation.cpp` | `sendOnPreinstallationCompleteEvent`, `dispatchEvent` |
| PIM-POS-024 | GetPreinstallState returns current state | `PreinstallManagerImplementation.cpp` | `GetPreinstallState` |
| PIM-POS-025 | Constructor/destructor singleton behavior | `PreinstallManagerImplementation.cpp`, `PreinstallManager.cpp` | ctor/dtor, `getInstance` |

## Detailed Test Cases (Inputs and Expected Results)

| Test ID | Type | Description | Inputs / Setup | Expected Result |
|---|---|---|---|---|
| PIM-POS-001 | Positive | Initialize plugin happy path | Valid `IShell` mock, Root returns `IPreinstallManager`, `QueryInterface<IConfiguration>` valid, `Configure` returns `ERROR_NONE` | `Initialize()` returns empty string, service and impl registration performed |
| PIM-POS-002 | Positive | Validate registration calls in Initialize | Same as POS-001 with call counters | `Register(notification)` and `Exchange::JPreinstallManager::Register` each called once |
| PIM-NEG-001 | Negative | Root object creation failure | `IShell::Root` returns null | `Initialize()` returns error message and triggers `Deinitialize()` cleanup |
| PIM-NEG-002 | Negative | IConfiguration unavailable | `QueryInterface<IConfiguration>` returns null | `Initialize()` returns configuration interface error and deinitializes |
| PIM-NEG-003 | Negative | Configure failure path | `Configure()` returns non-zero | `Initialize()` returns configuration failure message and deinitializes |
| PIM-POS-003 | Positive | Deinitialize full cleanup | Initialized plugin, valid remote connection | `Unregister` called, `Release` called, remote `Terminate` called, pointers reset |
| PIM-POS-004 | Positive | Information API behavior | None | Returns empty string |
| PIM-POS-005 | Positive | Deactivated handles matching connection | `connection->Id() == mConnectionId` | WorkerPool submit called with `DEACTIVATED/FAILURE` job |
| PIM-NEG-004 | Negative | Deactivated ignores non-matching connection | `connection->Id() != mConnectionId` | No submit/job call |
| PIM-POS-006 | Positive | Notification forwards preinstall completion event | Trigger `Notification::OnPreinstallationComplete()` | JSONRPC event forwarded to plugin event channel |
| PIM-POS-007 | Positive | Notification forwards status event payload | Trigger `Notification::OnAppInstallationStatus(json)` | Payload passed through unchanged |
| PIM-POS-008 | Positive | Register observer | Valid notification ptr | Returns `ERROR_NONE`, list size increments, `AddRef` called |
| PIM-BND-001 | Boundary | Register duplicate observer | Register same ptr twice | List contains single entry, no duplicate AddRef |
| PIM-POS-009 | Positive | Unregister existing observer | Observer previously registered | Returns `ERROR_NONE`, entry removed, `Release` called |
| PIM-NEG-005 | Negative | Unregister non-existing observer | Fresh observer not in list | Returns `ERROR_GENERAL` |
| PIM-POS-010 | Positive | Configure parses config line | Service config line contains `appPreinstallDirectory` | Member set correctly and returns `ERROR_NONE` |
| PIM-NEG-006 | Negative | Configure null service | `service=nullptr` | Returns `ERROR_GENERAL` |
| PIM-POS-011 | Positive | createPackageManagerObject success | `mCurrentservice` valid and query returns installer | Returns `ERROR_NONE` and installer pointer non-null |
| PIM-NEG-007 | Negative | createPackageManagerObject with null service | `mCurrentservice=nullptr` | Returns `ERROR_GENERAL`, output installer null |
| PIM-NEG-008 | Negative | createPackageManagerObject query failure | `QueryInterfaceByCallsign` returns null | Returns `ERROR_GENERAL` |
| PIM-POS-012 | Positive | releasePackageManagerObject behavior | Pass valid installer and null installer | Valid pointer released and set null, null pointer no crash |
| PIM-POS-013 | Positive | Read directory with valid package entries | Temporary dir with app folders, `GetConfigForPackage` success | Returns true and fills package list with parsed packageId/version |
| PIM-NEG-009 | Negative | Read directory open failure | Invalid/non-existent path | Returns false |
| PIM-BND-002 | Boundary | Dot entries filtering | Directory contains only `.` and `..` plus one folder | Only real folder processed |
| PIM-POS-014 | Positive | Fail reason enum mapping | Iterate known fail reasons | Expected string for each enum value |
| PIM-BND-003 | Boundary | Unknown fail reason mapping | Pass out-of-range enum cast | Returns `NONE` |
| PIM-POS-015 | Positive | Newer version compare matrix | `2.0.0 > 1.9.9`, `1.2.4 > 1.2.3`, `1.2.3.5 > 1.2.3.4` | Returns true for each |
| PIM-NEG-010 | Negative | Invalid semver handling | Inputs like `abc`, `1.x.3` | Returns false |
| PIM-BND-004 | Boundary | Version suffix stripping | Compare `1.2.3-beta+11` vs `1.2.2` | Stripped compare works, returns true |
| PIM-POS-016 | Positive | Force install all packages | forceInstall=true with 2 valid packages | Install called for all packages, state reaches COMPLETED |
| PIM-POS-017 | Positive | Non-force filter equal/older versions | installed map has same/newer versions | Older/equal preinstall packages removed before install |
| PIM-POS-018 | Positive | Non-force allows newer package | installed has older version | Newer package retained and installed |
| PIM-NEG-011 | Negative | StartPreinstall package manager create fail | createPackageManagerObject returns error | Returns `ERROR_GENERAL` and no thread start |
| PIM-NEG-012 | Negative | StartPreinstall read directory fail | readPreinstallDirectory false | Returns `ERROR_GENERAL` |
| PIM-POS-019 | Positive | StartPreinstall empty package list | read succeeds but list empty | State set COMPLETED, completion event dispatched, returns `ERROR_NONE` |
| PIM-POS-020 | Positive | StartPreinstall all filtered-out package list | forceInstall=false and all filtered | State COMPLETED, completion event dispatched |
| PIM-NEG-013 | Negative | StartPreinstall in-progress guard | mPreinstallState=IN_PROGRESS or thread active | Returns `ERROR_GENERAL` and warns preinstall already in progress |
| PIM-BND-005 | Boundary | Join previous thread then relaunch | Prior thread joinable and state COMPLETED | Previous thread joined and new worker started |
| PIM-POS-021 | Positive | installPackages success path | list of valid packages and Install returns success | install statuses marked SUCCESS, state COMPLETED, completion event sent |
| PIM-NEG-014 | Negative | installPackages empty field package item | packageId/version/fileLocator empty | Item skipped/failed, no crash, failure count increments |
| PIM-NEG-015 | Negative | installPackages handles install error reason | Install returns failure with failReason | status marked FAILED with reason text |
| PIM-NEG-016 | Negative | installPackages cannot create package manager | createPackageManagerObject fails | state moved to COMPLETED and completion event still sent |
| PIM-POS-022 | Positive | Dispatch known event branch | PREINSTALL_MANAGER_ONPREINSTALLATIONCOMPLETE with N observers | Every observer receives `OnPreinstallationComplete()` |
| PIM-NEG-017 | Negative | Dispatch unknown event branch | Event id not in switch | Unknown-event error path executed safely |
| PIM-POS-023 | Positive | sendOnPreinstallationCompleteEvent path | call API directly | Worker pool receives one job via `dispatchEvent` |
| PIM-POS-024 | Positive | GetPreinstallState API | set state transitions and call API | Returns `ERROR_NONE` and exact state value |
| PIM-POS-025 | Positive | Singleton pointer behavior | create/destroy implementation and plugin | `_instance` set at construction and reset at destruction |

## Boundary Test Data Matrix

| Data Set ID | Input | Purpose |
|---|---|---|
| B-VER-001 | `1.2.3` vs `1.2.3` | Equal version path |
| B-VER-002 | `1.2.3.0` vs `1.2.3` | Optional build component handling |
| B-VER-003 | `1.2.3-beta` vs `1.2.2` | Pre-release suffix handling |
| B-VER-004 | `1.2.3+build7` vs `1.2.3+build6` | Build metadata stripping |
| B-DIR-001 | Empty preinstall directory | No-package path |
| B-DIR-002 | Directory with invalid package config | Skip invalid package entry |
| B-LIST-001 | Installed list includes mixed states | Filter only `INSTALLED` entries |
| B-THREAD-001 | joinable thread + COMPLETED state | join-then-relaunch path |

## Projected Coverage by File

| File | Planned Coverage Focus | Projected Coverage |
|---|---|---|
| `PreinstallManager/PreinstallManager.cpp` | Initialize/Deinitialize lifecycle, Deactivated, Information, ctor/dtor | 80-90% |
| `PreinstallManager/PreinstallManager.h` | Notification forwarding methods | 75-85% (excluding interface-map macros) |
| `PreinstallManager/PreinstallManagerImplementation.cpp` | Register/Unregister, Configure, StartPreinstall, installPackages, version/filter logic, dispatch/event paths | 80-90% |
| `PreinstallManager/PreinstallManagerImplementation.h` | Inline/Job create-dispatch paths, state APIs via implementation calls | 75-85% (excluding macro-generated ABI paths) |
| `PreinstallManager/Module.cpp` | Module declaration macro line | 0-100% depending on compilation symbol instrumentation |
| `PreinstallManager/Module.h` | Macro declarations/header glue | Non-actionable runtime coverage |

Estimated aggregate outcome:

- `.cpp` files: **>=80% line coverage** feasible
- `.h` files with executable code: **>=75% where directly testable**
- Plugin total (practical): **>=75% minimum satisfied**

## Methods Not Covered (or Not Fully Coverable) in L0 and Why

| File | Method / Area | Reason | Recommendation |
|---|---|---|---|
| `PreinstallManager/Module.cpp` | `MODULE_NAME_DECLARATION(BUILD_REFERENCE)` | Macro/metadata line often not hit in runtime unit tests | Accept as non-actionable for L0 |
| `PreinstallManager/Module.h` | Module macro definitions | Header-only compile-time glue, no runtime path | Exclude from runtime coverage gate |
| `PreinstallManager/PreinstallManager.h` | `BEGIN_INTERFACE_MAP` / `END_INTERFACE_MAP` generated paths | COM-RPC runtime dispatch internals not directly unit-callable | Validate indirectly by interface usage in lifecycle tests |
| `PreinstallManager/PreinstallManagerImplementation.h` | Interface map macro expansion lines | Same as above | Treat as non-actionable |
| `PreinstallManager/PreinstallManagerImplementation.h` | `isValidSemVer(const std::string&)` | Declared but no definition/usage in current source | Either implement or remove declaration |
| `PreinstallManager/PreinstallManager.cpp` | `Notification::Activated` | Requires remote connection lifecycle trigger | Cover in integration/L1 if L0 mocking is insufficient |

## How to Generate L0 Test Code (Using RuntimeManager L0 as Reference)

1. Create test folder and files.

- Create `Tests/L0Tests/PreinstallManager/`.
- Add:
  - `PreinstallManagerTest.cpp`
  - `PreinstallManager_LifecycleTests.cpp`
  - `PreinstallManager_ImplementationTests.cpp`
  - `PreinstallManager_ComponentTests.cpp`
  - `ServiceMock.h`

2. Reuse test bootstrap and helpers.

- Include `Tests/L0Tests/common/L0Bootstrap.hpp` in main test runner.
- Include `Tests/L0Tests/common/L0Expect.hpp` for consistent assertions.

3. Add production sources for test target.

- Include PreinstallManager production `.cpp` in `Tests/L0Tests/CMakeLists.txt` source list.
- Keep RuntimeManager target naming pattern (`appmanagers_l0test`) for consistency, or create a dedicated target if preferred.

4. Introduce mocks similar to RuntimeManager style.

- `ServiceMock` should provide:
  - `ConfigLine()` return path config JSON with `appPreinstallDirectory`
  - `QueryInterfaceByCallsign<IPackageInstaller>` behavior control
  - `Root<Exchange::IPreinstallManager>` behavior control
  - register/unregister/addref/release counters
- `PackageInstallerMock` should provide:
  - `GetConfigForPackage`
  - `ListPackages`
  - `Install`
- `PackageIteratorMock` should implement `Next()` over deterministic package sets.

5. Build test suites by behavior block.

- Lifecycle suite: plugin Initialize/Deinitialize/Deactivated/Information.
- Implementation suite: Configure/Register/Unregister/StartPreinstall/GetPreinstallState.
- Component suite: version comparison, directory read, install pipeline and event dispatch.

6. Add deterministic filesystem test harness.

- Use temporary directory fixtures for preinstall package folders.
- Keep package metadata controlled by mocks, not actual package parsing.

7. Add coverage gate.

- Generate coverage via existing `lcov/genhtml` flow in `.github/workflows/L0-tests.yml`.
- Add pass criteria at report stage: fail if aggregate PreinstallManager file coverage drops below 75%.

## Suggested Test File Ownership Map

| Test File | Primary Area | Target Methods |
|---|---|---|
| `PreinstallManagerTest.cpp` | Runner/bootstrap | worker pool init, suite registration |
| `PreinstallManager_LifecycleTests.cpp` | Plugin lifecycle wrapper | `Initialize`, `Deinitialize`, `Information`, `Deactivated`, notification forwards |
| `PreinstallManager_ImplementationTests.cpp` | Core implementation APIs | `Configure`, `Register`, `Unregister`, `StartPreinstall`, `GetPreinstallState` |
| `PreinstallManager_ComponentTests.cpp` | Internal behavior and algorithmic paths | `readPreinstallDirectory`, `isNewerVersion`, `installPackages`, `getFailReason`, `Dispatch` |

## Exit Criteria

- All test cases in this plan executed and passing.
- No memory/resource leaks in L0 run (mock object refcounts balanced).
- At least 75% line coverage for PreinstallManager plugin runtime code.
- Any non-covered methods listed in this document with rationale and action item.

## Open Action Items

- Decide whether `isValidSemVer` should be implemented or removed from header.
- Confirm whether `Notification::Activated` will be covered in L0 via mock remote connection or deferred to L1/L2.
- Add PreinstallManager sources and tests into `Tests/L0Tests/CMakeLists.txt` before enabling CI gate.
