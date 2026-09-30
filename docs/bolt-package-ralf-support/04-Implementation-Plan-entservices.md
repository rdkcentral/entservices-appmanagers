# Implementation Plan — `entservices-appmanagers` (`RuntimeManager/`, `PackageManager/`)

> **Principle:** every change here is **additive, runtime-gated and backward compatible with
> the existing generic-`libpackage` RALF deployment.** No change to `RuntimeManager/ralf/*`.
>
> Files under `RuntimeManager/` and `PackageManager/` are governed by
> `.github/instructions/General.instructions.md`, `Pluginimplementation.instructions.md` and
> `Plugincmake.instructions.md` — follow them (logging macros, namespace placement, no
> free-standing helpers).

---

## CHANGE-1 (required) — `generate()` becomes a runtime dispatch

**File:** `RuntimeManager/RuntimeManagerImplementation.cpp` (~line 522)

```cpp
        bool RuntimeManagerImplementation::generate(const ApplicationConfiguration &config,
                                                    const WPEFramework::Exchange::RuntimeConfig &runtimeConfigObject,
                                                    std::string &dobbySpec)
        {
#ifdef RALF_PACKAGE_SUPPORT_ENABLED
            if (!runtimeConfigObject.ralfPkgPath.empty())
            {
                LOGINFO("Generating Ralf Package Config : %s", runtimeConfigObject.ralfPkgPath.c_str());
                ralf::RalfPackageBuilder ralfBuilder;
                return ralfBuilder.generateRalfDobbySpec(config, runtimeConfigObject, dobbySpec);
            }
            LOGINFO("ralfPkgPath is empty - falling back to legacy Dobby spec generation");
#endif // RALF_PACKAGE_SUPPORT_ENABLED
            if (nullptr == mAIConfiguration)
            {
                LOGERR("AIConfiguration not initialized");
                return false;
            }
            DobbySpecGenerator generator(*mAIConfiguration);
            if (!mGstRegistrySourcePath.empty())
            {
                generator.setGstreamerRegistryPath(mGstRegistrySourcePath);
            }
            return generator.generate(config, runtimeConfigObject, dobbySpec);
        }
```

**Impact on the existing generic RALF deployment:** none — `libpackage` sets `ralfPkgPath`
on every successful `Lock`, so the predicate is always true. See
`02-Gap-Analysis-and-Breaking-Changes.md#gap-1` for the optional `RALF_STRICT_MODE` guard
that preserves today's fail-fast semantics byte-for-byte if a product build wants it.

---

## CHANGE-2 (required) — track RALF mode per app instance

**File:** `RuntimeManager/RuntimeManagerImplementation.h`

```cpp
            struct RuntimeAppInfo
            {
                ...
                bool usesRialto = false;
                bool ralfMode   = false;   // launched via RalfPackageBuilder/RalfOCIConfigGenerator
            };
```

**File:** `RuntimeManager/RuntimeManagerImplementation.cpp`, inside `Run()` where
`runtimeAppInfo` is populated (~line 866):

```cpp
                        runtimeAppInfo.ralfMode = !runtimeConfigObject.ralfPkgPath.empty();
```

---

## CHANGE-3 (required) — gate overlayfs cleanup on `ralfMode`

Three sites: `:256` (TERMINATED), `:287` (CONTAINERFAILED), `:903` (container start failed).

```cpp
#ifdef RALF_PACKAGE_SUPPORT_ENABLED
                {
                    bool wasRalfInstance = false;
                    {
                        Core::SafeSyncType<Core::CriticalSection> lock(mRuntimeManagerImplLock);
                        auto it = mRuntimeAppInfo.find(appInstanceId);
                        if (it != mRuntimeAppInfo.end())
                        {
                            wasRalfInstance = it->second.ralfMode;
                        }
                    }
                    if (wasRalfInstance)
                    {
                        ralf::RalfPackageBuilder ralfBuilder;
                        ralfBuilder.unmountOverlayfsIfExists(appInstanceId);
                    }
                }
#endif // RALF_PACKAGE_SUPPORT_ENABLED
```

> ⚠️ **Ordering:** in the CONTAINERFAILED handler `mRuntimeAppInfo.erase(appInstanceId)`
> currently runs *before* the RALF block. Capture `ralfMode` **before** the erase (hoist a
> local at the top of the handler) or the lookup always misses and mounts leak.

---

## CHANGE-4 (required) — `legacyContainer` becomes a runtime decision

**File:** `RuntimeManager/RuntimeManagerImplementation.cpp` (~line 727)

```cpp
            // Widgets launch from a Dobby spec; RALF/Bolt packages launch from an OCI bundle.
            bool legacyContainer = true;
#ifdef RALF_PACKAGE_SUPPORT_ENABLED
            legacyContainer = runtimeConfigObject.ralfPkgPath.empty();
#endif
```

This automatically fixes the `StartContainer` vs `StartContainerFromDobbySpec` selection at
`:886-895` and the `appPath = dobbySpec` assignment at `:854` — both already key off
`legacyContainer`. No further edit there.

---

## CHANGE-5 (required) — uid/gid override only for RALF instances

**File:** `RuntimeManager/RuntimeManagerImplementation.cpp` (~line 622)

```cpp
#ifdef RALF_PACKAGE_SUPPORT_ENABLED
            // In a Ralf package all apps run as the shared 'ralf' user; widgets keep their
            // per-app uid/gid assigned by UserIdManager.
            if (!runtimeConfigObject.ralfPkgPath.empty())
            {
                if (!ralf::getRalfUserInfo(uid, gid))
                {
                    LOGERR("Failed to get Ralf user info. This can lead to failure in launching "
                           "the app. uid: %d, gid: %d", uid, gid);
                }
            }
#endif
```

And at `:675-682`, replace the `#ifdef`-based storage uid/gid selection with the same runtime
predicate:

```cpp
                const bool ralfInstance = !runtimeConfigObject.ralfPkgPath.empty();
                appStorageInfo.userId  = ralfInstance ? uid : 0;
                appStorageInfo.groupId = ralfInstance ? gid : 0;
```

(The subsequent `config.mAppStorageInfo.userId/groupId = uid/gid` assignment is identical in
both existing `#ifdef` arms — collapse it to a single unconditional assignment.)

---

## CHANGE-6 (required) — Rialto gating

**File:** `RuntimeManager/RuntimeManagerImplementation.cpp` (~lines 735, 757)

```cpp
#ifdef ENABLE_RIALTO
            const bool ralfInstance = !runtimeConfigObject.ralfPkgPath.empty();
            bool requiresRialto;
            if (ralfInstance)
            {
                requiresRialto = true;
            }
            else
            {
                std::vector<std::pair<std::string, std::string>> parsedCaps;
                DobbySpecGenerator::parseCapabilities(runtimeConfigObject.capabilities, parsedCaps);
                const bool appRequiresRialto = DobbySpecGenerator::hasCapability(parsedCaps, "rialto");
                const int rialtoOverride = (nullptr != mAIConfiguration) ? mAIConfiguration->getRialtoOverride() : -1;
                requiresRialto = (rialtoOverride >= 0) ? (rialtoOverride > 0) : appRequiresRialto;
            }
            ...
            std::string rialtoSocket = "rialto-" + appId;
            if (ralfInstance)
            {
                // RALF creates a socket named after appInstanceId; prefix to avoid a clash.
                rialtoSocket = "rlto-" + appInstanceId;
                LOGINFO("[RIALTO] RALF instance: rialtoSocket updated to '%s'", rialtoSocket.c_str());
            }
#endif
```

Note this removes the current `#else` branch's dependency on `DobbySpecGenerator` being
compiled in — which it now always is (CHANGE-1). Guard with
`#ifdef RALF_PACKAGE_SUPPORT_ENABLED` only where `ralf::` symbols are referenced.

---

## CHANGE-7 (build config, no code) — flag matrix

**File:** `RuntimeManager/CMakeLists.txt` — **no change.** `RALF_PACKAGE_SUPPORT` keeps its
current meaning: "compile the `ralf/` sources and enable the RALF launch path".
After CHANGE-1..6 it is no longer *exclusive*, which is the whole point.

**File:** `PackageManager/CMakeLists.txt` — **no change.** Document the matrix instead:

| Product image | `USE_LIBPACKAGE_RALF` | `RALF_PACKAGE_SUPPORT` | `libPackage.so` provider |
|---|---|---|---|
| Generic RDK (today) | ON | ON | `ref/libpackage` (`RalfPackageHandler`) |
| Generic RDK (legacy) | OFF | OFF | older libpackage |
| **Sky / Comcast (target)** | **OFF** | **ON** | **`ref/libpackage-sky-ralf` with `ENABLE_RALF_PACKAGE_SUPPORT=ON`** |
| Sky / Comcast (today) | OFF | OFF | `ref/libpackage-sky-ralf` default build |

Rationale for `USE_LIBPACKAGE_RALF=OFF` on the Sky image: see
`02-Gap-Analysis-and-Breaking-Changes.md#gap-3` — `libpackage-sky::PackageImpl::Lock` already
resolves and reports the runtime companion package itself; the `USE_LIBPACKAGE_RALF` block
would double-lock it and clobber `state.runtimeConfig.runtimePath`.

---

## CHANGE-8 (optional, deferred) — propagate `packageFormat`

`PackageManagerImplementation::getRuntimeConfig(const packagemanager::ConfigMetaData &, Exchange::RuntimeConfig &)`
does not copy `packageFormat`. Nothing in this design needs it, but it is useful for telemetry
and for debugging "which path did this app take". If added:

* Requires a new `packageFormat` string in `Exchange::RuntimeConfig` (entservices-apis change).
* Must default to empty so the generic caller is unaffected.

**Out of scope for this delivery. Track separately.**

---

## CHANGE-9 (docs) — update existing markdown

Per repo convention, update rather than create:

* `RuntimeManager/DobbySpecGeneration.md` — add a section: *RALF vs legacy selection is now
  runtime, driven by `RuntimeConfig.ralfPkgPath`; a non-empty value routes to
  `RalfPackageBuilder` → `RalfOCIConfigGenerator` and launches via `StartContainer`, an empty
  value routes to `DobbySpecGenerator` and launches via `StartContainerFromDobbySpec`.*
* `RuntimeManager/SkyEpgMigrationBindMount_ImplPlan.md` — note that the `extraMounts`
  capability transport is **unchanged for widgets**, and state explicitly whether it applies
  to Sky RALF packages (it should, since `PackageImpl::GetPackageMetaData` runs for both).
* `PackageManager/PackageManager.md` (if it documents backends) — add the flag matrix
  from CHANGE-7.
* `spec_coverage.md` — add AC1–AC7 coverage rows.

---

## Files touched — summary

| File | Type | Risk |
|---|---|---|
| `RuntimeManager/RuntimeManagerImplementation.cpp` | 6 edits, all `#ifdef` → runtime predicate | **Medium** — touches the launch hot path |
| `RuntimeManager/RuntimeManagerImplementation.h` | +1 struct field | Low |
| `RuntimeManager/ralf/*` | **none** | — |
| `RuntimeManager/CMakeLists.txt` | **none** | — |
| `PackageManager/*` | **none** (docs only) | — |
| `Tests/L0Tests/RuntimeManager/ralf/*`, `Tests/L1Tests/tests/test_Ralf.cpp` | new cases | Low |
