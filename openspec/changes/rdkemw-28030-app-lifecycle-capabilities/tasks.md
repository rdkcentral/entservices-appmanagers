Branch: `RDKEMW-28030` (from `origin/develop` `0b6fa4c9`). API pin unchanged: `INTERFACES_REF` `a6e324d8`. All commands run from `../local-build/` through `docker-run.sh` on `linux/amd64`, per `AGENTS.md`.

## 1. Shared helpers

- [x] 1.1 Add `SetObject(key, serializedObject, error)` and `GetObject(key, serializedObject, present, error)` to `helpers/RuntimeConfigPayload.{h,cpp}`.
  - Acceptance: a nested object round-trips with nested and unknown members intact.
  - Acceptance: `SetObject` with a non-object or invalid JSON returns false and leaves the payload unchanged.
  - Acceptance: `GetObject` on a missing key sets `present=false`.
  - Acceptance: `GetObject` on a non-object member returns false with an error.
- [x] 1.2 Add header-only `helpers/UtilsMemorySize.h` providing `bool Utils::ParseMemorySize(const std::string&, uint64_t&)`. C++11; digits, optional `K|M|G|B` (case-insensitive), optional trailing `B`; 1024-based; false on empty input, unknown suffix, trailing characters or overflow.
  - Acceptance: `"16M"` gives 16777216.
  - Acceptance: `"100B"` gives 100.
  - Acceptance: `"1g"` gives 1073741824.
  - Acceptance: `"16X"`, `""` and `"M"` all return false.
- [x] 1.3 Add L0 cases for 1.1 and 1.2 to `Tests/L0Tests/PackageManager/PackageManager_ImplementationTests.cpp`, next to the existing `RuntimeConfigPayload` tests.
  - Depends on 1.1 and 1.2.

## 2. PackageManager: expose `applicationLifecycle`

- [x] 2.1 Add a private helper in `PackageManager/PackageManagerImplementation.{h,cpp}`: `addApplicationLifecycle(const string& packageId, const string& ralfPkgPath, string& runtimeConfigPayload)`. Using Thunder `JsonObject` (see design §2), it:
  - reads `packages[].pkgMetaDataPath`;
  - selects the metadata whose top-level `id` equals `packageId`;
  - copies `configuration["urn:rdk:config:application-lifecycle"]` with `SetObject("applicationLifecycle", ...)` only when it is an object;
  - on any failure, logs `LOGWARN` and leaves the payload unchanged.
  - Depends on 1.1.
- [x] 2.2 Call the helper in `LockPackage()` right after `getRuntimeConfig()` succeeds on the first lock (about `PackageManagerImplementation.cpp:945`), only when `config.ralfPkgPath` is non-empty.
  - Acceptance: the lock result codes are identical to before for every path.
  - Acceptance: `Lock` and `GetLockedInfo` return the enriched payload.
  - Acceptance: a runtime-package lock never receives the app's block.
- [x] 2.3 Extend `Tests/mocks/IPackageImplDummy.h` so a test-controlled package's `Lock` sets `ralfPkgPath` to a fixture file. Add fixtures for:
  - a full block with an unknown member;
  - no block;
  - a malformed metadata file;
  - a missing file;
  - an `id` mismatch;
  - an app entry plus a runtime dependency entry.

  Keep the existing `Initialize`, `Install` and `Lock` behaviour for current tests.
  - Implemented as the `PACKAGEMANAGER_TEST_RALF_PKG_PATH` environment variable, which works across the L1 test runner and plugin libraries. The fixtures are written to a temporary directory at test run time.
- [x] 2.4 Add L0 tests in `Tests/L0Tests/PackageManager/PackageManager_ImplementationTests.cpp` for each fixture in 2.3, covering both `Lock` and `GetLockedInfo`, and confirming every other payload member is unchanged.
  - Depends on 2.2 and 2.3.
  - Spec: "Package Manager exposes package lifecycle configuration" (all four scenarios).
- [x] 2.5 Add an L1 test to `Tests/L1Tests/tests/test_PackageManager.cpp` covering `Lock` and `GetLockedInfo` with the block present and absent, comparing JSON semantically as the existing tests do.
  - Depends on 2.2.

## 3. ResourceMonitor: parser

- [x] 3.1 Add `ResourceMonitor/AppLifecycleConfig.{h,cpp}` with `AppLifecycleState`, `LifecycleConfigStatus`, `SuspendedStateConfig`, `AppLifecycleCapabilities` (with `IsSupported`) and `AppLifecycleConfig::Parse()`, as in design §4. C++11 and no exceptions escape.
  - Depends on 1.1 and 1.2.
  - Acceptance: ACTIVE is always set.
  - Acceptance: states are matched case-insensitively; unknown names and non-standard combinations are logged.
  - Acceptance: the status follows the order in design §4:
    - malformed payload → `UNAVAILABLE`;
    - missing or empty `ralfPkgPath` (legacy) → `UNAVAILABLE`;
    - `ralfPkgPath` set and no block → `NOT_CONFIGURED`;
    - block that isn't an object → `UNAVAILABLE`;
    - block object → `CONFIGURED`.
  - Acceptance: each Suspended value is reported as present or unavailable independently.
  - Acceptance: the CPU threshold is always unavailable.

## 4. ResourceMonitor: package resolution and wiring

- [x] 4.1 Add `ResourceMonitor/PackageLifecycleConfigProvider.{h,cpp}`. It holds the internal `IPackageLifecycleSource` interface plus a production implementation that lazily obtains `Exchange::IPackageInstaller` and `Exchange::IPackageHandler` from `org.rdk.AppPackageManager`. The implementation:
  - guards the pointers with its own `Core::CriticalSection`;
  - releases them on destruction and on `ERROR_CONNECTION_CLOSED` or `ERROR_UNAVAILABLE`;
  - follows the concurrency contract in design §5: `AddRef` local copies under the lock, release the lock before any COM call, keep parsing per call, and never take `_adminLock`, `_reconcileMutex` or `_reactiveMonitorMutex`.
  - Depends on 3.1.
- [x] 4.2 Implement `GetLifecycleCapabilities`, `IsLifecycleStateSupported` and `GetSuspendedStateConfig`, with an optional `version`. Resolution is: `ListPackages`, then filter `packageId == appId`, then `GetLockedInfo` on each version.
  - Acceptance: a single locked version is used.
  - Acceptance: zero or several locked versions give `UNAVAILABLE` with a warning.
  - Acceptance: an explicit version skips the listing.
  - Acceptance: an empty `appId` returns `ERROR_INVALID_PARAMETER`; every other outcome returns `ERROR_NONE` with a status.
- [x] 4.3 Wire the provider into `ResourceMonitor/ResourceMonitorImplementation.{h,cpp}`: create it in `Configure(service)` and expose the three public C++ methods. Not added to `IResourceMonitor` or JSON-RPC.
  - Acceptance: Configure and destructor ordering is safe.
  - Acceptance: existing VictimSelector and reconcile behaviour is unchanged.
  - Acceptance: the public methods are called without holding any `ResourceMonitorImplementation` lock, and the header documents the usage guidance in design §8 (snapshot per `appInstanceId`, `UNAVAILABLE` means don't enforce).
- [x] 4.4 Update `ResourceMonitor/CMakeLists.txt`: add `AppLifecycleConfig.cpp` and `PackageLifecycleConfigProvider.cpp` to `${PLUGIN_IMPLEMENTATION}` and link `AppManagersHelpers`.
  - Acceptance: `NO_TTY=1 ./docker-run.sh build.sh entservices-appmanagers plugins --clean` succeeds with no new warnings in the ResourceMonitor sources.
  - Status: done. CMake updated, and `local-build/build.sh` now enables `PLUGIN_RESOURCEMONITOR` and `PLUGIN_VICTIM_SELECTOR`. The plugin build passes with `--clean` once `INTERFACES_REF` is moved (see section 9).

## 5. ResourceMonitor tests

- [x] 5.1 Create `Tests/L0Tests/ResourceMonitor/` with:
  - `ResourceMonitorTest.cpp` (the harness entry point, following `VictimSelector/VictimSelectorTest.cpp`);
  - `ResourceMonitor_LifecycleConfigTests.cpp`, covering every scenario in "Supported lifecycle states...", "Single lifecycle-state support query", "Missing optional lifecycle configuration" (including "Legacy package without RALF metadata"), "Suspended-state configuration" and "Application-to-package resolution";
  - `ResourceMonitor_LifecycleConcurrencyTests.cpp`, covering the "Concurrent use" scenarios: several threads querying the same and different apps with a fake source that adds delay, checking for consistent results, no deadlock, and a completion timeout;
  - a fake `IPackageLifecycleSource` that can be unavailable, return errors, return multiple locked versions, or delay its responses.
  - Depends on 4.2.
- [x] 5.2 Register `resourcemonitor_l0test` in `Tests/L0Tests/CMakeLists.txt`: `option(PLUGIN_RESOURCE_MONITOR ... ON)`, a source list (the two new `.cpp` files, `helpers/RuntimeConfigPayload.cpp` and `common/L0Bootstrap.cpp`), with flags, includes and coverage matching `victimselector_l0test`.
- [x] 5.3 Add `resourcemonitor_l0test` to the build-target list and the binary-run checks in `.github/workflows/L0-tests.yml` (lines 396 and 407–425).
  - Depends on 5.2.
- [x] 5.4 Extend `Tests/L1Tests/tests/test_ResourceMonitor.cpp` with `ServiceMock` returning `PackageManagerMock` (`Tests/mocks/PackageManagerMock.h`) for `org.rdk.AppPackageManager`. Add one test per AC:
  - AC1: all states configured;
  - AC2: only `["paused","suspended"]`;
  - AC3: RALF package (`ralfPkgPath` set) with no block gives `NOT_CONFIGURED`, and a legacy package (empty `ralfPkgPath`) gives `UNAVAILABLE`;
  - AC4: Suspended values present, and partially invalid;
  - AC5: the reported values change when the mocked Package Manager payload changes.

  Mocked payloads must set `ralfPkgPath` explicitly for every case. Also add tests for Package Manager unavailable, then available on retry.
  - Depends on 4.3.
- [x] 5.5 Check whether `Tests/L1Tests/CMakeLists.txt` (`RESOURCEMONITOR_INC`) and `.github/workflows/L1-tests.yml` need changes for the new sources, includes or `AppManagersHelpers` linkage. While there, fix the missing `/` in `${CMAKE_SOURCE_DIR}../entservices-appmanagers/helpers` (line 187).
  - Depends on 5.4.

## 6. Documentation

- [x] 6.1 Update `ResourceMonitor/README.md` with:
  - the lifecycle-capability utilities;
  - their status semantics (`CONFIGURED`, `NOT_CONFIGURED`, `UNAVAILABLE`, including legacy packages);
  - the concurrency contract;
  - the usage guidance for RDKEMW-28031 (snapshot per instance, `UNAVAILABLE` means don't enforce);
  - the CPU-threshold dependency.
- [x] 6.2 Update `PackageManager/PackageManager.md` and `openspec/specs/PackageManager.spec.md` with the `applicationLifecycle` payload member: its source, when it is present and what happens on failure.

## 7. Verification (per `AGENTS.md`)

- [ ] 7.1 Plugin build: `NO_TTY=1 ./docker-run.sh build.sh entservices-appmanagers plugins --clean`. Zero compile or link errors.
- [ ] 7.2 L0: `NO_TTY=1 ./docker-run.sh build.sh entservices-appmanagers l0 run --clean`. Every `*_l0test` runs, including `resourcemonitor_l0test` and `packagemanager_l0test`. Record pass and fail counts.
- [ ] 7.3 L1: reproduce `.github/workflows/L1-tests.yml` inside Docker (configure, build, run `RdkServicesL1Test`). Run at least `*ResourceMonitor*` and `*PackageManager*` with exit code 0 and no failures, then run the full suite to check for regressions.
- [ ] 7.4 Run `git diff --check` with no whitespace errors, then write the completion report: modules built, tests added, exact commands, counts, warnings and anything not run.

## 8. Removed items

- [x] 8.1 CPU threshold: removed from scope. The AC's "CPU threshold" was a mistake; the resource is memory, not CPU. The memory resource is already covered by `maxSuspendedSystemMemory` (see architecture questions §2.2). No CPU metadata field is needed for RDKEMW-28030.

## 9. Additional changes needed to verify this change (pre-existing on `develop`)

- [x] 9.1 Move `INTERFACES_REF` in `L0-tests.yml`, `L1-tests.yml` and `native_full_build.yml` from `a6e324d8` to `60135849` (tip of `entservices-apis` `topic/RDKEMW-23628`). That revision is a superset of `a6e324d8`: it contains the RDKEMW-24163 squash merge (`23828c94`) and the RDKDEV-1479 `IResourceMonitor` API (`4726a032`). Under `apis/`, only `IResourceMonitor.h`, `ITextTrack.h` and `Ids.h` differ. Without this, ResourceMonitor does not compile, and `develop` CI fails at `0b6fa4c9`. `local-build/setup-deps.sh` defaults to the same SHA.
- [x] 9.2 Restore `Test_StateHandlerTransitionEventsIncludeNavigationIntent` in `Tests/L0Tests/LifecycleManager/LifecycleManager_ContextTests.cpp`, which `0b6fa4c9` left as a garbled merge that stops the whole L0 build. It now uses the `1.2.rc` version, plus the `onFirstFrame` completion step that `develop`'s ACTIVE transition needs.
- [x] 9.3 Use the generated camelCase JSON-RPC method names (`getMemInfo`, `getPsiMetrics`, `getSwapUsed`, `getFlashSpace`, `getStats`) in `Tests/L1Tests/tests/test_ResourceMonitor.cpp` and the `ResourceMonitor/README.md` examples, matching `@text` in `IResourceMonitor.h`.
- [x] 9.4 Add a real `entservices-appmanagers` `l1 [run]` mode to `local-build/build.sh` that mirrors `L1-tests.yml` (gcc, without coverage). `GTEST_FILTER` is honoured.

**Known L1 failure left as is (decision: leave):** `ResourceMonitorInitializedTest.ReconcileMissingRamTarget`, from PR #661.
- The test expects `reconcile` without `ramTargetMB` to fail.
- The R4.4-generated JSON-RPC handler only traces the missing parameter and passes `ramTargetMB = 0` on.
- `ResourceMonitorImplementation::Reconcile()` validates only `appId`, so the call returns `ERROR_NONE`.
- RDKEMW-28030 doesn't touch this path; it belongs to the reconcile owner.
