# RuntimeManager L0 Coverage Analysis and Improvement Plan

## Scope

- Coverage report analyzed: `artifacts-L0-appmanagers/coverage-html/RuntimeManager/*` (generated on 2026-03-18)
- Source analyzed: `RuntimeManager/*`, `RuntimeManager/Gateway/*`, `RuntimeManager/ralf/*`
- Existing L0 tests reviewed:
  - `Tests/L0Tests/RuntimeManager/RuntimeManagerTest.cpp` (test runner — 213 total test cases)
  - `Tests/L0Tests/RuntimeManager/RuntimeManager_LifecycleTests.cpp`
  - `Tests/L0Tests/RuntimeManager/RuntimeManager_ImplementationTests.cpp`
  - `Tests/L0Tests/RuntimeManager/RuntimeManager_ComponentTests.cpp`
  - `Tests/L0Tests/RuntimeManager/ralf/Ralf_SupportTests.cpp`
  - `Tests/L0Tests/RuntimeManager/ralf/Ralf_PackageBuilderTests.cpp`
  - `Tests/L0Tests/RuntimeManager/ralf/Ralf_OCIConfigGeneratorTests.cpp`
  - `Tests/L0Tests/RuntimeManager/Gateway/Gateway_ContainerUtilsTests.cpp`
  - `Tests/L0Tests/RuntimeManager/Gateway/Gateway_WebInspectorTests.cpp`
  - `Tests/L0Tests/RuntimeManager/Gateway/Gateway_NetFilterTests.cpp`

## Coverage Baseline (RuntimeManager package)

- **Line coverage:** `1091 / 1605` (**68.0%**) — core `RuntimeManager/` only
- **Function coverage:** `120 / 143` (**83.9%**) — core `RuntimeManager/` only

Primary low-coverage files:

1. `DobbySpecGenerator.cpp` — 371/537 lines (69.1%), 26/30 functions (86.7%)
2. `RuntimeManagerImplementation.cpp` — 332/546 lines (60.8%), 29/36 functions (80.6%)
3. `AIConfiguration.cpp` — 116/214 lines (54.2%), 27/30 functions (90.0%)
4. `DobbyEventListener.h` — 4/7 lines (57.1%), 2/3 functions (66.7%)
5. `Module.cpp` — 0/1 lines, 0/2 functions

Sub-package coverage (compiled alongside core):

- `RuntimeManager/Gateway/` — 119/665 lines (17.9%), 21/46 functions (45.7%)
- `RuntimeManager/ralf/` — 139/503 lines (27.6%), 16/38 functions (42.1%)

---

## Functions with 0 Hits (Uncovered)

### `AIConfiguration.cpp`

- `AIConfiguration::generate()` — full Dobby JSON spec generation path requires a running Dobby environment
- ABI ctor/dtor symbol variants (`C2 / D0 / D1`)

### `DobbyEventListener.h`

- `OCIContainerNotification::QueryInterface(uint32_t)` — ABI interface-map macro path (not directly callable)

### `DobbySpecGenerator.cpp`

- `~DobbySpecGenerator()` (one ABI destructor symbol variant `D0`)
- `createMulticastSocketPlugin(...)` — always returns empty in current codebase (TODO stub)
- `addHolePunchPortToSpec(...)` — requires real hole-punch port config in JSON

### `Module.cpp`

- `ModuleBuildRef`
- `ModuleServiceMetadata`

### `RuntimeManager.cpp`

- `RuntimeManager::Deactivated(RPC::IRemoteConnection*)` — RPC connection deactivation lifecycle, not reachable via public API in L0
- ABI ctor/dtor symbol variants

### `RuntimeManagerImplementation.cpp`

- `RuntimeManagerImplementation::Run(...)` — full success path requires real OCI plugin + WM connector
- `RuntimeManagerImplementation::Terminate(...)` success path — requires tracked running container
- `RuntimeManagerImplementation::releaseStorageManagerPluginObject()`
- `RuntimeManagerImplementation::createStorageManagerObject()`
- ABI ctor/dtor symbol variants (`D0 / D1`)

### `RuntimeManagerImplementation.h`

- `Configuration::~Configuration()` (ABI destructor variant)
- `Job::~Job()` (ABI destructor variant)

### `WindowManagerConnector.h`

- `WindowManagerNotification::QueryInterface(uint32_t)` — ABI interface-map macro path

### `ContainerUtils.cpp`

- `getContainerIpAddress()` success path — requires live `/proc/net/fib_trie` and a running container network namespace
- `getContainerNamespaceId()` variants requiring real container pid/ns file

### `Debugger.h`

- ABI symbol variants for the abstract debugger type enum class

### `NetFilter.cpp`

- `NetFilter::addContainerPortForwardList()` — requires real iptables `ip4tc` / `ip6tc` chain manipulation
- `NetFilter::removeContainerPortForwarding()` — same dependency
- `NetFilter::closeExternalPort()` — iptables-dependent

### `NetFilterUtils.c`

- All `nft_*` rule manipulation functions — require running nftables kernel subsystem (not available in L0 user-space)
- `populatePortForwardList()` — requires live iptables chain data

### `WebInspector.cpp`

- `WebInspector::attach()` success path — requires real TCP connection to a live Dobby WebInspector port
- `WebInspector::detach()`, `send()`, `receive()` — network I/O
- Internal socket setup / teardown functions

### `RalfOCIConfigGenerator.cpp`

- All `RalfOCIConfigGenerator::generate()` internal helper functions — require real base OCI spec files and graphics config on disk
- File I/O, mount-point merging, rootfs overlay helpers

### `RalfPackageBuilder.cpp`

- `RalfPackageBuilder::unmountOverlayfsIfExists()` success path — requires a real mounted overlayfs at the specified path

### `RalfSupport.cpp`

- `chownRecursive()` — requires root privileges
- `generateOCIRootfs()` success path — requires real filesystem mount support

---

## Uncovered Lines (by file)

> Full detail is in the LCOV pages. Key ranges are summarized here.

- `AIConfiguration.cpp`: **98 lines**
  - `generate()` method body — entire Dobby JSON construction block, `addContainerEnv()` / `addAppPorts()` helpers, per-field JSON setters only reachable from full spec generation.
- `DobbyEventListener.h`: **3 lines**
  - `BEGIN_INTERFACE_MAP` / `INTERFACE_ENTRY` / `END_INTERFACE_MAP` macro lines.
- `DobbySpecGenerator.cpp`: **166 lines**
  - `createMulticastSocketPlugin()` return-null body, `addHolePunchPortToSpec()` both rdkPlugins and legacy-plugins format branches, filesystem permission / `chmod` / `chown` error branches, `isValidIpv4()` helper, FKPS-dependent blocks, `/opt` and `/tmp` path validation branches.
- `Module.cpp`: **1 line**
  - `MODULE_NAME_DECLARATION(BUILD_REFERENCE)`
- `RuntimeManager.cpp`: **9 lines**
  - `Deactivated(connection)` method body, `Initialize()` error-path null root guard / Deinitialize-on-failure cleanup, OCI plugin registration failure path in `Initialize()`.
- `RuntimeManagerImplementation.cpp`: **214 lines**
  - `Run()` success path, `Terminate()` / `Kill()` success paths, `Wake()` success path, `Annotate()` / `GetInfo()` success paths, `createStorageManagerObject()` and `releaseStorageManagerPluginObject()`, portal-prefix strip with non-empty portal, `dispatchEvent()` default/unknown event branch.
- `UserIdManager.cpp`: **2 lines**
  - Existing-user early return path in `getUserId()`, `clearUserId()` pool-full guard.
- `WindowManagerConnector.cpp`: **12 lines**
  - `initialize()` re-init guard path, Wayland display connection failure branch inside `createDisplay()`, `releasePlugin()` WM `Unregister()` failure path.
- `WindowManagerConnector.h`: **3 lines**
  - `BEGIN_INTERFACE_MAP` / `INTERFACE_ENTRY` / `END_INTERFACE_MAP` macro lines.
- `ContainerUtils.cpp`: **58 lines**
  - `getContainerIpAddress()` success body — `/proc/net/fib_trie` parsing loop, `getContainerNamespaceId()` internal fd open / read / parse block.
- `NetFilterUtils.c`: **426 lines**
  - All `nft_chain_*`, `nft_rule_*`, `nft_table_*` manipulation functions, `populatePortForwardList()` — kernel-level calls unavailable in L0 sandbox.
- `NetFilter.cpp`: **32 lines**
  - `removeAllRulesMatchingComment()` success inner loop, `addContainerPortForwardList()` ip4tc/ip6tc chain commit path, `closeExternalPort()` iptables DELETE rule path.
- `NetFilterLock.cpp`: **13 lines**
  - Timeout expiry branch in `tryLockFor()`, error-code mapping in lock failure branch.
- `WebInspector.cpp`: **16 lines**
  - `attach()` socket `connect()` success branch, `send()` / `recv()` network I/O block, `detach()` socket teardown.
- `RalfOCIConfigGenerator.cpp`: **309 lines**
  - Entire `generate()` implementation — requires base OCI spec JSON file and graphics config on disk, all internal merge / overlay / mount helpers.
- `RalfPackageBuilder.cpp`: **24 lines**
  - `unmountOverlayfsIfExists()` success branch, `generateRalfDobbySpec()` post-parse OCI config generation steps.
- `RalfSupport.cpp`: **31 lines**
  - `chownRecursive()` — requires root privileges, `generateOCIRootfs()` success path — requires real bind-mount capability.

---

## Why These Are Not Covered (Root-Cause) + Complexity

## 1) Kernel / system-call dependent code (**Not actionable in L0**)

- `Gateway/NetFilterUtils.c` (426 lines, 11 functions): all nftables/iptables rule-manipulation functions require a live kernel nftables subsystem unavailable in a user-space L0 environment.
- `Gateway/WebInspector.cpp` (16 lines, 6 functions): real TCP socket connection to a running Dobby WebInspector port — not simulatable without a mock socket layer.
- `RalfOCIConfigGenerator.cpp` (309 lines, 17 functions): requires base OCI spec JSON and graphics config files at well-known paths on disk.
- `RalfSupport::chownRecursive`: requires root privileges.

## 2) OCI container runtime paths (**Not actionable in L0 without OCI mock**)

`RuntimeManagerImplementation::Run()` success path, `Terminate()` / `Kill()` with a live container, `Wake()` with PAUSED state, `Annotate()` / `GetInfo()` — all require a real or mocked OCI plugin that accepts the call. In the current L0 setup `ServiceMock::QueryInterfaceByCallsign` returns nullptr so `mOciContainerObject` is always null; only the null-guard error paths are reachable.

## 3) Error/retry/fallback branches under-tested (**Medium to High complexity**)

- `RuntimeManagerImplementation` storage manager creation / release paths.
- `WindowManagerConnector` re-init guard and Wayland failure branches.
- `dispatchEvent()` default/unknown event branch.

Reason: current tests validate parameter-invalid and no-plugin error paths but do not reach staged failure/recovery sequences or default branch fall-throughs.

## 4) Optional / TODO / environment-dependent branches (**High complexity**)

- `DobbySpecGenerator` blocks around FKPS, hole-punching, multicast plugin, mount/permission handling, `/tmp` and `/opt` dependent logic.
- `AIConfiguration::generate()` — full Dobby JSON blob construction invoked only through a live OCI spec pipeline.

## 5) ABI symbol artifacts in coverage reports (**Low priority / often non-actionable**)

- Duplicate constructor/destructor symbols (`C1/C2`, `D0/D1` forms) across `RuntimeManagerImplementation.h`, `RuntimeManager.cpp`, `DobbySpecGenerator.cpp`.
- Interface-map macro lines (`BEGIN_INTERFACE_MAP` / `END_INTERFACE_MAP`) in `DobbyEventListener.h` and `WindowManagerConnector.h` — dispatched only by the COM-RPC runtime.

---

## How to Improve Coverage with Additional Tests

## Phase 1 (Quick Wins, high value / low effort)

1. **Add OCI mock returning a fake plugin for Configure()**
   - Extend `ServiceMock` so `QueryInterfaceByCallsign("OCIContainer")` returns a minimal stub `IOCIContainer*`.
   - Unlocks `Configure()` success path and downstream `Terminate()` / `Kill()` / `Annotate()` / `GetInfo()` success paths once a container is tracked in `mRuntimeAppInfo`.

2. **Add Storage manager mock to cover createStorageManagerObject()**
   - Return a minimal `IStorageManager` stub from `QueryInterfaceByCallsign("AppStorageManager")`.
   - Reaches `createStorageManagerObject()` and `releaseStorageManagerPluginObject()`.

3. **Add UserIdManager edge-case paths**
   - Call `getUserId(appId)` twice for the same appId to hit the existing-user early-return branch.
   - Exhaust the UID pool then call `clearUserId()` to hit the pool-full guard line.

4. **Add DobbySpecGenerator createMulticastSocketPlugin / addHolePunchPortToSpec tests**
   - Confirm `createMulticastSocketPlugin()` deliberately returns empty (TODO stub) given multicast-enabled config.
   - Pass `appPorts` entries in both rdkPlugins format and legacy format to reach both branches in `addHolePunchPortToSpec()`.

## Phase 2 (Event callback path coverage)

5. **Portal-prefix stripping with real non-empty portal**
   - Configure impl with `ServiceMock` returning `{"runtimeAppPortal":"com.sky.apps."}` from `ConfigLine()`.
   - Fire `onOCIContainerStartedEvent` with `containerId = "com.sky.apps.youTube"` and verify `OnStarted("youTube")` fires.

6. **dispatchEvent() default/unknown event branch**
   - Inject a `JsonObject` with an unrecognised `eventName` value into the dispatch path.
   - Confirm the default branch guard at the end of `Dispatch()` is reached.

## Phase 3 (Filesystem / system-call interception)

7. **ContainerUtils IP resolution with synthetic /proc file**
   - Create a temp file with known `/proc/net/fib_trie` format content.
   - Wrap `fopen` to redirect the real path to the temp file; exercise `getContainerIpAddress()` success parse path.

8. **NetFilter rules with iptables wrapper (limited)**
   - Wrap `iptc_init` / `iptc_commit` to return synthetic handles.
   - Cover `closeExternalPort`, `removeContainerPortForwarding`, `addContainerPortForwardList` (~32 lines).

## Phase 4 (Ralf / OCI config generator)

9. **RalfOCIConfigGenerator::generate() with disk-backed spec files**
   - Write minimal base OCI spec and graphics config JSON files to `/tmp` in the test setup.
   - Pass these paths to `RalfOCIConfigGenerator::generate()` — currently the largest single gap (309 lines, 17 functions).

10. **RalfSupport::generateOCIRootfs with bind-mount wrapper**
    - Wrap the `mount()` syscall to return 0 (success).
    - Pass a temp directory as rootfs path to reach the OCI rootfs success path.

11. **RalfPackageBuilder::unmountOverlayfsIfExists success path**
    - Wrap `umount2()` to simulate a successful unmount without requiring a real overlayfs.

---

## Fakes / Stubs Available in L0 Tests

The following L0-specific test infrastructure is in `Tests/L0Tests/` and should be treated as the canonical reuse assets:

- `Tests/L0Tests/RuntimeManager/ServiceMock.h` — minimal `IShell` / `IPlugin` service stub used by all implementation tests
- `Tests/L0Tests/common/L0Expect.hpp` — lightweight assertion helpers (`ExpectTrue`, `ExpectEqU32`)
- `Tests/L0Tests/common/L0Bootstrap.hpp` — WPEFramework worker-pool / messaging initialisation guard
- `FakeNotification` (defined inline in `RuntimeManager_ImplementationTests.cpp`) — atomic-counter `IRuntimeManager::INotification` stub used in dispatch callback tests

### Recommended stub additions (not currently present)

To improve `RuntimeManagerImplementation` OCI-path coverage, add a concrete `IOCIContainer` stub class with stub implementations of:

- `Run(...)` / `Terminate(...)` / `Kill(...)` / `Hibernate()` / `Wake(...)`
- `Annotate(...)` / `GetInfo(...)`
- `Register(INotification*)` / `Unregister(INotification*)`

This enables exercising the full `Configure()` success path and all downstream operations without a real Dobby daemon.

---

## Suggested Prioritized Test Backlog (Top 10)

1. OCI container stub — `Configure()` success path + `createStorageManagerObject()`
2. `Terminate()` / `Kill()` success paths with tracked container in `mRuntimeAppInfo`
3. `Annotate()` / `GetInfo()` success paths via OCI stub
4. `dispatchEvent()` default/unknown-event branch
5. Portal-prefix stripping with non-empty `mRuntimeAppPortal`
6. `UserIdManager` existing-user early-return + pool-full guard
7. `addHolePunchPortToSpec()` two-format (rdkPlugins vs legacy) tests
8. `createMulticastSocketPlugin()` empty-return confirmation
9. `RalfOCIConfigGenerator::generate()` with disk-backed spec files
10. `ContainerUtils::getContainerIpAddress()` with synthetic `/proc/net/fib_trie` file

---

## Expected Coverage Impact

- **Fast uplift target:** +4% to +7% line coverage (core `RuntimeManager/`) by implementing Phases 1–2.
- **Medium uplift target:** additional +2% to +4% with filesystem/wrapper tests (Phase 3).
- **Hard uplift target:** +5% to +10% from RalfOCIConfigGenerator disk-backed tests (Phase 4).

Total realistic improvement with disciplined test additions: **~10% to 18%** line coverage uplift for the RuntimeManager package (core), with the overall binary uplift limited by the non-actionable kernel / socket / privilege gaps in Gateway and ralf sub-packages.
