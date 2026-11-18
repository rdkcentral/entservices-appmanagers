## Context

See `proposal.md` for the motivation and `specs/app-lifecycle-capabilities/spec.md` for the behaviour this design must deliver. The relevant current state:

- **Metadata source.** RALF metadata (`rdkcentral/oci-package-spec`, `metadata.md`) defines `urn:rdk:config:application-lifecycle` with three required members (`supportedNonActiveStates`, `maxSuspendedSystemMemory`, `maxTimeToSuspendMemoryState`) and two optional ones (`startupTimeout`, `watchdogInterval`). There is no CPU-threshold member yet.
- **What the libraries already expose.** ralf-utils v1.3.0 (`6bf14f39`, the Yocto pin) parses the states into `supportedLifecycleStates()`. It discards the two Suspended values (`FIXME` comments). Its default of `Paused | LowPower` cannot tell an absent block from `[]`. None of these values reach `packagemanager::ConfigMetaData`.
- **How the raw metadata is already reachable.** `rdkcentral/libpackage` `RalfPackageImpl::Lock()` mounts the app and its dependencies. It writes each package's metadata file unchanged to `/tmp/mounts/<id>_<ver>/config.json`, writes `/tmp/<id>_<ver>_metadata.json` listing `{pkgMountPath, pkgMetaDataPath}` for every mounted package, and sets `ConfigMetaData::ralfPkgPath` to that list. `Install` does not set it.
- **Payload handling.** `PackageManagerImplementation::LockPackage()` turns `ConfigMetaData` into the opaque runtime configuration payload (`getRuntimeConfig`). It stores the payload per package and version, and `Lock` and `GetLockedInfo` return it (RDKEMW-24163, already on develop).
- **Consumers ignore unknown members.** AppManager and LifecycleManager keep payload members they don't own, and RuntimeManager ignores unknown members. A new member is therefore safe end to end.
- **AppManager's identifier rule.** AppManager uses `appId` directly as the PackageManager `packageId` (`Lock(appId, version, ...)`, and `checkInstallDetails` matching `package.packageId == appId`).
- **ResourceMonitor today.** It is C++11 and only talks to VictimSelector. It does not link `AppManagersHelpers` and has no L0 target.

## Goals / Non-Goals

**Goals:**
- PackageManager remains the only source and copies the metadata unchanged. All interpretation happens in one parser in ResourceMonitor.
- The parser is pure, so it can be tested at L0 without COM. COM access sits in a thin adapter behind an interface that can be faked.
- No public interface change, and no behaviour change for existing payload consumers.

**Non-Goals:**
- Caching lifecycle data in ResourceMonitor. Calls are infrequent and the data is immutable per version. RDKEMW-28031 keeps its own per-instance snapshot (see Decision 8).
- Identifying non-app workloads. RDKEMW-28031 takes that from AppManager's `LoadedAppInfo.type`, not from this capability.
- Refactoring RuntimeManager's RALF helpers.

## Decisions

### 1. PackageManager copies the metadata block unchanged and ResourceMonitor interprets it

```
RALF pkg ─(libpackage Lock)─► /tmp/<id>_<ver>_metadata.json ─► packages[].pkgMetaDataPath ─► config.json
                                         ▲ ralfPkgPath                                     │ configuration
                                         │                                                 ▼ ["urn:rdk:config:application-lifecycle"]
PackageManager::LockPackage ── getRuntimeConfig ── + applicationLifecycle (verbatim) ──► payload (Lock / GetLockedInfo)
                                                                                           │
ResourceMonitor ── PackageLifecycleConfigProvider ── ListPackages + GetLockedInfo ─────────┘
                        │
                        ▼
                AppLifecycleConfig::Parse(payload) ──► AppLifecycleCapabilities
```

Payload example (the other members are unchanged):

```json
{"appPath":"/apps/x","ralfPkgPath":"/tmp/x_1.0.0_metadata.json",
 "applicationLifecycle":{"supportedNonActiveStates":["paused","suspended"],
                         "maxSuspendedSystemMemory":"16M","maxTimeToSuspendMemoryState":10,
                         "startupTimeout":60}}
```

**Alternatives considered:**
- **Have PackageManager parse the block into typed members.** Rejected. It duplicates interpretation, and a future CPU key would need another PackageManager change.
- **Have ResourceMonitor read the `/tmp` files itself.** Rejected. That bypasses PackageManager (AC5) and couples ResourceMonitor to libpackage's file layout.
- **Add the values to `ConfigMetaData`.** Rejected. It needs eshelpers, libpackage and ralf-utils changes outside this story.
- **Use ralf-utils `vendorConfig()` inside libpackage.** Rejected, for the same reason.

### 2. Where PackageManager does the enrichment

The enrichment runs in `LockPackage()` right after `getRuntimeConfig(config, runtimeConfigPayload)` succeeds on the first lock (around `PackageManagerImplementation.cpp:945`), and only when `config.ralfPkgPath` is non-empty. A private helper does the work:

1. Read the `ralfPkgPath` JSON.
2. For each `packages[].pkgMetaDataPath`, read the metadata file and select the entry whose top-level `id` equals the locked `packageId`. Dependency entries such as the runtime and base packages are skipped.
3. If `configuration["urn:rdk:config:application-lifecycle"]` is an object, write it into the payload with `RuntimeConfigPayload::SetObject("applicationLifecycle", ...)`.

Any failure logs `LOGWARN` and leaves the payload untouched; the lock result is unaffected. Runtime-package locks go through the same function with their own `packageId`, so no app block can leak into a runtime's payload. Later locks of the same version reuse the stored payload, and `GetLockedInfo` returns it, so it covers AppManager and ResourceMonitor alike.

The reader uses Thunder's `JsonObject` (`Core::JSON::VariantContainer`), which PackageManager already uses and links. jsoncpp was rejected because PackageManager only links it in L1 test builds. The reader is private to PackageManager. `RuntimeManager/ralf/RalfSupport.cpp::parseRalPkgInfo` is not reused, because sharing it would make PackageManager depend on RuntimeManager.

### 3. Helper additions in `helpers/`

- **`RuntimeConfigPayload::SetObject(key, serializedObject, error)` and `GetObject(key, serializedObject, present, error)`.** Both use the existing `JsonObject` (`Core::JSON::VariantContainer`). `SetObject` rejects input that isn't a JSON object and leaves the payload unchanged. These are additive and don't change the semantics of existing methods.
- **`helpers/UtilsMemorySize.h`: `bool Utils::ParseMemorySize(const std::string&, uint64_t& bytes)`.** It is header-only and C++11. It accepts digits optionally followed by `K`, `M`, `G` or `B` (case-insensitive), optionally followed by `B`, with 1024-based multipliers, and returns false on error or overflow.
  - `ralf::parseMemorySize` is left alone. It returns `0` on error and rejects a lone `B`, which the spec pattern `^\d+[GMBgmb]?$` allows. Changing it would alter RuntimeManager's OCI behaviour and its L1 expectations.
  - Moving RuntimeManager onto the new helper is a possible follow-up.

### 4. Types and parser in ResourceMonitor (C++11, no `std::optional`)

New files `ResourceMonitor/AppLifecycleConfig.{h,cpp}`:

```cpp
enum class AppLifecycleState : uint8_t { ACTIVE = 1, PAUSED = 2, SUSPENDED = 4, HIBERNATED = 8 };
enum class LifecycleConfigStatus : uint8_t { CONFIGURED, NOT_CONFIGURED, UNAVAILABLE };
struct SuspendedStateConfig {
    bool     hasMaxSystemMemory = false;  uint64_t maxSystemMemoryBytes = 0;
    bool     hasMaxTimeToSuspend = false; uint32_t maxTimeToSuspendSec = 0;
    bool     hasCpuThreshold = false;     double   cpuThresholdPercent = 0.0;   // always false until key defined
};
struct AppLifecycleCapabilities {
    LifecycleConfigStatus status = LifecycleConfigStatus::UNAVAILABLE;
    uint8_t supportedStates = static_cast<uint8_t>(AppLifecycleState::ACTIVE);
    SuspendedStateConfig suspended;
    bool IsSupported(AppLifecycleState s) const;
};
namespace AppLifecycleConfig {
    // Parses a runtime configuration payload; never throws; logs and degrades per spec.
    LifecycleConfigStatus Parse(const std::string& runtimeConfigPayload, AppLifecycleCapabilities& out);
}
```

The parser:
- reads `applicationLifecycle` through `RuntimeConfigPayload::GetObject`;
- matches state names case-insensitively, the same way ralf-utils does;
- ignores unknown state names and reports non-standard combinations exactly as listed, with a warning;
- ignores unknown members, which is where a future CPU key would land;
- treats a wrongly typed or negative `maxTimeToSuspendMemoryState` as unavailable.

The status is decided in this order:

| Payload | `status` |
|---|---|
| Not valid JSON | `UNAVAILABLE` |
| `ralfPkgPath` missing or empty (legacy package, or no RALF metadata to consult) | `UNAVAILABLE` |
| `ralfPkgPath` non-empty, `applicationLifecycle` missing | `NOT_CONFIGURED` |
| `ralfPkgPath` non-empty, `applicationLifecycle` present but not an object | `UNAVAILABLE` (logged) |
| `ralfPkgPath` non-empty, `applicationLifecycle` is an object | `CONFIGURED` |

Legacy packages are reported as `UNAVAILABLE` rather than `NOT_CONFIGURED` so that enforcement in RDKEMW-28031 doesn't treat them as "supports nothing beyond Active". Treating them that way would stop them being suspended and could get them terminated, even though LifecycleManager suspends them today. `ralfPkgPath` is already in the payload, so this needs no PackageManager change.

### 5. Package resolution adapter

`ResourceMonitor/PackageLifecycleConfigProvider.{h,cpp}` depends on a small internal interface, `IPackageLifecycleSource`, with two methods: `ListPackages(std::vector<Package>&)` and `GetLockedInfo(id, ver, payload, locked)`. The production implementation:
- wraps `Exchange::IPackageInstaller` and `Exchange::IPackageHandler`, obtained with `QueryInterfaceByCallsign` on `org.rdk.AppPackageManager`;
- queries them lazily on first use, so ResourceMonitor startup has no ordering dependency on PackageManager;
- guards the pointers with its own `Core::CriticalSection`;
- releases them on destruction, and also on `ERROR_CONNECTION_CLOSED` or `ERROR_UNAVAILABLE` (from `RPC` / `CHECK_CACHE`) so the next request re-queries.

**Concurrency contract.** The provider may be called from several threads at once: today's L0/L1 tests, and later RDKEMW-28031's AppManager-event and timer threads, running alongside the existing reactive-monitor and reconcile threads.
- **Locking:** its lock protects only acquiring, `AddRef`-ing and releasing the interface pointers. Each call takes an `AddRef`'d local copy of the pointers under the lock, releases the lock, and only then makes the COM calls.
- **No shared state:** parsing works on per-call data only.
- **Lock ordering:** callers must not hold `ResourceMonitorImplementation::_adminLock`, `_reconcileMutex` or `_reactiveMonitorMutex` while calling the provider. The provider never takes those locks, so it can't deadlock with the existing threads and a slow Package Manager can't stall them.

Provider API (used by `ResourceMonitorImplementation`, not exported over COM):

```cpp
Core::hresult GetLifecycleCapabilities(const string& appId, AppLifecycleCapabilities& caps, const string& version = "");
Core::hresult IsLifecycleStateSupported(const string& appId, AppLifecycleState state, bool& supported, const string& version = "");
Core::hresult GetSuspendedStateConfig(const string& appId, SuspendedStateConfig& config, const string& version = "");
```

Without a version, the provider lists packages, keeps those with `packageId == appId`, and calls `GetLockedInfo` on each. If exactly one is locked it is used. If none are locked, or more than one is, the result is `UNAVAILABLE` and a warning is logged.

Every outcome described in the spec returns `ERROR_NONE` with the matching `status`. Only an empty `appId` returns `ERROR_INVALID_PARAMETER`. This keeps "handled without failure" explicit for callers.

**Alternatives considered:**
- **A new PackageManager "get locked version" API.** Rejected; it needs an `entservices-apis` change.
- **Asking AppManager for the running version.** Rejected; `LoadedAppInfo` has no version, and it would add a dependency cycle risk.

### 6. Wiring into ResourceMonitor and CMake

- `ResourceMonitorImplementation` owns a `std::unique_ptr<PackageLifecycleConfigProvider>`, created in `Configure(service)`, and exposes the three methods as public C++ members.
- `ResourceMonitor/CMakeLists.txt` adds the two `.cpp` files to `${PLUGIN_IMPLEMENTATION}` and links `AppManagersHelpers`. `<interfaces/IAppPackageManager.h>` is already installed by `entservices-apis`.
- No new CMake options and no changes to `.conf.in` or `.config`.
- `Tests/L0Tests/CMakeLists.txt` gains `option(PLUGIN_RESOURCE_MONITOR ...)` and a `ResourceMonitor_L0Test` target built from:
  - `AppLifecycleConfig.cpp`, `PackageLifecycleConfigProvider.cpp` and `helpers/RuntimeConfigPayload.cpp`;
  - the tests, which use the existing `common/` harness (`L0Bootstrap`, `L0Expect`) and a fake `IPackageLifecycleSource`.
- The L1 build already enables `-DPLUGIN_RESOURCEMONITOR=ON`, so `test_ResourceMonitor.cpp` is extended with `PackageManagerMock.h`. No workflow filter change is expected; this is confirmed during implementation.

### 7. Compatibility and optional features

- **Payload consumers.** The payload gains a member only for locked RALF packages with the block. AppManager and LifecycleManager keep it. RuntimeManager's decoder (`RuntimeConfiguration.cpp`) ignores unknown members, and Dobby and RALF specs are unchanged.
- **JSON-RPC.** `lock` and `getLockedInfo` responses (`@opaque`) gain the `applicationLifecycle` object. This is additive.
- **Without `USE_LIBPACKAGE_RALF`.** `ralfPkgPath` is empty, so nothing is added and ResourceMonitor reports `UNAVAILABLE`.
- **`AIMANAGERS_TELEMETRY_METRICS_SUPPORT`.** Unaffected.

### 8. Intended use by RDKEMW-28031 (lifecycle enforcement)

RDKEMW-28031 is the only planned production caller. This change doesn't implement any of the following; it is guidance so the interface fits that use.
- **Snapshot per app instance.** Request the capabilities once per `appInstanceId`, at the first lifecycle event for that instance (normally launch, when exactly one version is locked). Keep the result for the instance's lifetime.
  - Why: a fresh lookup during a deferred update (a new version locked while the old instance is Suspended) sees two locked versions and returns `UNAVAILABLE`.
  - The optional `version` argument covers callers that already know the version.
- **Treat `UNAVAILABLE` as "do not enforce".** It covers legacy packages, Package Manager outages and ambiguous versions. Only `CONFIGURED` and `NOT_CONFIGURED` describe the package.
- **Non-app detection is not part of this capability.** RDKEMW-28031 uses AppManager's `LoadedAppInfo.type`.
- **Call from enforcement threads without holding Resource Monitor locks** (see the concurrency contract in Decision 5).

## Risks / Trade-offs

- **[Risk]** The libpackage file layout (`ralfPkgPath` schema, `/tmp` paths) changes. → Only the documented `packages[].pkgMetaDataPath` contract is used, the same one RuntimeManager already relies on. L0 fixtures pin it.
- **[Risk]** There is extra file I/O on the first lock. → Two small JSON reads happen once per version lock, against RuntimeManager's existing reads of the same files.
- **[Risk]** The app's metadata `id` differs from the `packageId`. → libpackage already rejects that mismatch at `Lock`. If no entry matches, the block is omitted with a warning.
- **[Trade-off]** Data exists only while a package is locked. → Enforcement only targets running apps, which are always locked. Otherwise the status is `UNAVAILABLE`.
- **[Trade-off]** Several locked versions produce `UNAVAILABLE`. → This is rare (deferred replacement of a suspended app). RDKEMW-28031 snapshots per instance at launch (Decision 8), and callers that know the version can pass it.
- **[Trade-off]** Legacy packages are `UNAVAILABLE`, so RDKEMW-28031 won't enforce lifecycle on them. → This keeps today's behaviour for legacy apps. Extending enforcement to them would need another source of capability data.
- **[Risk]** The ResourceMonitor plugin links an extra shared library. → `AppManagersHelpers` is already installed and loaded by six other app-manager plugins.

## Migration Plan

This is additive only. Deploy the PackageManager and ResourceMonitor plugin binaries together; no API or ABI change is involved.

Rolling back either plugin on its own is safe while nothing enforces lifecycle:
- Old ResourceMonitor binaries ignore the new member.
- New ResourceMonitor binaries against an old PackageManager report `NOT_CONFIGURED` for RALF packages (`ralfPkgPath` set but no block) and `UNAVAILABLE` for legacy packages.

Once RDKEMW-28031 enforces lifecycle, an old PackageManager would make RALF apps look "Active only". That mixed deployment must be avoided.

## Open Questions

- What is the name, type and unit of the Suspended CPU-threshold metadata member? Once it is defined, only the parser and its tests change. `SuspendedStateConfig` already has the field.
