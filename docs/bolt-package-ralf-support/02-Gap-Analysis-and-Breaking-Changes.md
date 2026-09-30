# Gap Analysis & Breaking Changes in Existing RALF Code

This document answers the explicit question: *"consider if any breaking change is there in
existing RALF related code inside PackageManager/RuntimeManager, whether we can directly lift
and use it, and whether something we need will break existing RALF usage inside
entservices-appmanagers."*

---

## GAP-1 🔴 BLOCKER — `RALF_PACKAGE_SUPPORT_ENABLED` is compile-time *exclusive*, not additive

`RuntimeManager/RuntimeManagerImplementation.cpp:524`

```cpp
bool RuntimeManagerImplementation::generate(const ApplicationConfiguration &config,
                                            const Exchange::RuntimeConfig &runtimeConfigObject,
                                            std::string &dobbySpec)
{
#ifdef RALF_PACKAGE_SUPPORT_ENABLED
    ralf::RalfPackageBuilder ralfBuilder;
    return ralfBuilder.generateRalfDobbySpec(config, runtimeConfigObject, dobbySpec);
#else
    DobbySpecGenerator generator(*mAIConfiguration);
    ...
    return generator.generate(config, runtimeConfigObject, dobbySpec);
#endif
}
```

**With `RALF_PACKAGE_SUPPORT=ON` the legacy `DobbySpecGenerator` path is compiled out entirely.**
An EntOS Widget lock produces an empty `ralfPkgPath`, `parseRalPkgInfo()` fails to open the
file, `generateRalfDobbySpec()` returns `false`, and the launch dies. A single image therefore
**cannot** serve both package types today.

This blocks **AC2** (unified support) and **AC6** (widget backward compatibility) outright.

### Additional sites with the same defect

| Line | `#ifdef` effect | Breaks widgets because… |
|---|---|---|
| 256, 287 | `RalfPackageBuilder::unmountOverlayfsIfExists(appInstanceId)` on TERMINATED / CONTAINERFAILED | Runs for widget instances that never created an overlayfs → spurious `rmdir`/`umount` and error logs |
| 622 | `ralf::getRalfUserInfo(uid, gid)` overrides `runtimeConfigObject.userId/groupId` for **every** app | Widgets lose their per-app `UserIdManager` uid/gid → data-image ownership and file ACLs break |
| 675–682 | `appStorageInfo.userId/groupId = uid/gid` instead of `0/0` | Widget storage provisioning changes ownership semantics |
| 728–735 | `legacyContainer = false` for **every** app | Widgets get `StartContainer(ociRootfsPath,...)` instead of `StartContainerFromDobbySpec(dobbySpec,...)` — guaranteed launch failure |
| 757–762 | `requiresRialto = true` unconditionally; `rialtoSocket = "rlto-"+appInstanceId` | Widgets that don't need Rialto get a forced session; socket naming diverges from what the widget's Dobby spec expects |
| 903–908 | overlayfs unmount on container-start failure | Same as 256/287 |

### Required fix (additive, backward compatible)

Keep `#ifdef RALF_PACKAGE_SUPPORT_ENABLED` as the **compile gate for the RALF code**, but make
the **behaviour** a runtime decision on `runtimeConfigObject.ralfPkgPath`:

```cpp
bool RuntimeManagerImplementation::generate(const ApplicationConfiguration &config,
                                            const Exchange::RuntimeConfig &runtimeConfigObject,
                                            std::string &dobbySpec)
{
#ifdef RALF_PACKAGE_SUPPORT_ENABLED
    if (!runtimeConfigObject.ralfPkgPath.empty())
    {
        LOGINFO("Generating Ralf Package Config : %s", runtimeConfigObject.ralfPkgPath.c_str());
        ralf::RalfPackageBuilder ralfBuilder;
        return ralfBuilder.generateRalfDobbySpec(config, runtimeConfigObject, dobbySpec);
    }
    LOGINFO("ralfPkgPath empty - using legacy Dobby spec generation");
#endif
    if (nullptr == mAIConfiguration) { LOGERR("AIConfiguration not initialized"); return false; }
    DobbySpecGenerator generator(*mAIConfiguration);
    if (!mGstRegistrySourcePath.empty())
        generator.setGstreamerRegistryPath(mGstRegistrySourcePath);
    return generator.generate(config, runtimeConfigObject, dobbySpec);
}
```

**Is this a breaking change for the existing generic-libpackage RALF users? No.**
`libpackage`'s `RalfPackageImpl::Lock` sets `ralfPkgPath` on **every** successful lock
(`RalfPackageHandler.cpp:607`), and `libpackage` builds *only* RALF packages. So for that
deployment the predicate is always true and behaviour is bit-identical to today.
The one behavioural delta is the **failure** case: today a `libpackage` lock that somehow
left `ralfPkgPath` empty would fail inside `parseRalPkgInfo`; after the change it would
silently fall through to `DobbySpecGenerator`. Mitigate with an explicit guard/telemetry:

```cpp
#if defined(RALF_PACKAGE_SUPPORT_ENABLED) && defined(RALF_STRICT_MODE)
    if (runtimeConfigObject.ralfPkgPath.empty()) { LOGERR("RALF strict mode: empty ralfPkgPath"); return false; }
#endif
```
`RALF_STRICT_MODE` defaults OFF; the generic-libpackage product build may set it ON to keep
the old fail-fast semantics exactly.

---

## GAP-2 🟠 Per-instance RALF mode is not tracked

`unmountOverlayfsIfExists()` is called from the TERMINATED / CONTAINERFAILED event handlers,
which only have `appInstanceId` — `runtimeConfigObject` is long gone. A runtime predicate is
therefore impossible at that point.

**Fix:** add `bool ralfMode = false;` to `RuntimeAppInfo` (`RuntimeManagerImplementation.h`),
set it in `Run()` from `!runtimeConfigObject.ralfPkgPath.empty()`, and guard the cleanup:

```cpp
#ifdef RALF_PACKAGE_SUPPORT_ENABLED
{
    bool wasRalf = false;
    { Core::SafeSyncType<Core::CriticalSection> lock(mRuntimeManagerImplLock);
      auto it = mRuntimeAppInfo.find(appInstanceId);
      if (it != mRuntimeAppInfo.end()) wasRalf = it->second.ralfMode; }
    if (wasRalf) { ralf::RalfPackageBuilder ralfBuilder; ralfBuilder.unmountOverlayfsIfExists(appInstanceId); }
}
#endif
```

⚠️ Note the existing ordering bug in the TERMINATED handler: `mRuntimeAppInfo.erase(appInstanceId)`
happens **before** the unmount block in the CONTAINERFAILED case, so the lookup must be done
*before* the erase. Capture `ralfMode` at the top of the handler.

---

## GAP-3 🟠 `USE_LIBPACKAGE_RALF` changes `PackageManager` Lock semantics

`PackageManagerImplementation.cpp:786` — under `USE_LIBPACKAGE_RALF` the runtime companion
package is additionally `LockPackage()`d and `state.runtimeConfig.runtimePath` is overwritten
from the **runtime's** `ConfigMetaData`:

```cpp
#ifdef USE_LIBPACKAGE_RALF
    uint32_t lockId = 0;
    result = LockPackage(rtPackageId, rtVersion, lockReason, lockId, rtUnpackedPath, config, locks);
    if (result == packagemanager::SUCCESS) state.runtimeConfig.runtimePath = config.runtimePath;
#else
    LOGWARN("Not runtime locking in old libpackage");
#endif
```

`libpackage-sky`'s `PackageImpl::Lock` **already** resolves and reports the runtime itself
(`mPackageDatabase->findRuntime()` → `configMetadata.command` / `.runtimePath`, plus
`additionalLocks.push_back(...)`). Enabling `USE_LIBPACKAGE_RALF` would double-lock the
runtime and clobber `runtimePath` with a second, unrelated `ConfigMetaData`.

**Decision: the Sky image must build with `USE_LIBPACKAGE_RALF=OFF`.**
The two flags are independent:

| Flag | Owner | Sky image | Generic image |
|---|---|---|---|
| `USE_LIBPACKAGE_RALF` (PackageManager) | libpackage backend semantics | **OFF** | ON |
| `RALF_PACKAGE_SUPPORT` (RuntimeManager) | RALF OCI launch path | **ON** | ON |
| `ENABLE_RALF_PACKAGE_SUPPORT` (libpackage-sky, new) | libralf backend in the Sky lib | **ON** | n/a |

The only other `USE_LIBPACKAGE_RALF` use (`:1117`) is cosmetic — it tags
`backend = "ralf"` in `InitializeState()` logs. Losing that tag for the Sky image is
acceptable; optionally derive it at runtime from `packageImpl` instead.

---

## GAP-4 🟡 `getRuntimeConfig(ConfigMetaData&, RuntimeConfig&)` drops fields

`PackageManagerImplementation.cpp:907-959` does **not** copy:
`md5Hash`, `mimeType`, `packageFormat`, `dialId`, `fireboltVersion`, `dataImageSize` (it does
copy `dataImageSize`), `logLevels` (it does), `unpackedPath` (set elsewhere).

For **this** design nothing new is required — `ralfPkgPath` and `capabilities` are both copied.
`packageFormat` would be a *nice-to-have* for telemetry/observability. If added, it must be
purely additive and default-empty so the generic caller is unaffected.

**Action: no change in scope. Log as a follow-up.**

---

## GAP-5 🟡 Stale local `IPackageImpl.h` copies

* `ref/IPackageImpl.h` — oldest; **missing `capabilities` and `md5Hash`**, which
  `libpackage-sky/PackageImpl.cpp` uses on nearly every path.
* `ref/eshelpers/packager/IPackageImpl.h` — has both; this is what `build_dependencies.sh`
  copies into `libPackage/include/` for the native/CI build.
* **Neither** declares `GetInstalledPackageMetadata` / `GetConfigListForInstalledPackages`
  as virtuals, yet `PackageManagerImplementation.cpp:1426` calls
  `packageImpl->GetInstalledPackageMetadata(...)`, and both `libpackage-sky/PackageImpl.h`
  and `libpackage/RalfPackageImpl.h` declare them `override`.

**Conclusion:** the authoritative header in the Sky/RDK-E sysroot is newer than every local
copy. **Resolve the real header version before touching any `IPackageImpl` signature**, and
prefer designs (like this one) that need **zero** interface change.

---

## GAP-6 🟡 `RalfOCIConfigGenerator` hard-depends on artefacts that may not exist on a Sky box

| Artefact | Constant | Risk |
|---|---|---|
| `/usr/share/ralf/oci-base-spec.json` | `RALF_OCI_BASE_SPEC_FILE` | `generateRalfOCIConfig()` returns false immediately if absent |
| `/usr/share/gpu-layer/rootfs` + `config.json` | `RALF_GRAPHICS_LAYER_*` | Always prepended as the bottom overlayfs lowerdir |
| `ralf` system user | `RALF_USER_NAME` | `getRalfUserInfo()` fails → uid/gid fall back with an error log |
| `/tmp/ralf/` writable | `RALF_APP_ROOTFS_DIR` | overlay workdir/upperdir |
| Per-package `config.json` with `configuration` object | `RalfOCIConfigGenerator.cpp:440` | **Hard fail** if absent or not an object |

These are **image/recipe prerequisites**, not code. They must be added to the Sky Yocto recipe
alongside `ralf-utils`. See `05-Build-CI-and-Packaging.md`.

---

## GAP-7 🟡 RALF mode suppresses westeros socket mounting

`RuntimeManagerImplementation.cpp:893`:

```cpp
// For RALF we are not mounting the westeros socket from dobby. It can be done from RALF itself.
status = mOciContainerObject->StartContainer(containerId, appPath, command, "", descriptor, success, errorReason);
```

The westeros socket is passed as `""` in RALF mode — the OCI config generated by
`RalfOCIConfigGenerator` is expected to bind-mount it. Verify that Sky's
`oci-base-spec.json` / per-package `config.json` actually does this, or Sky Bolt apps will
launch but render nothing. **Validate during AC5 testing.**

---

## GAP-8 🟢 `capabilities` semantics differ between backends

| Backend | `ConfigMetaData.capabilities` content |
|---|---|
| `libpackage` `RalfPackageImpl` | comma-joined **RALF permission URNs** (`addPackagePermissionsToConfigMetadata`) |
| `libpackage-sky` widgets | Sky capability tokens + `extraMounts=…` + `resolution=<h>` + `spec=<escaped dobby json>` |

Both are comma-separated strings, and `RuntimeManagerImplementation` passes
`runtimeConfigObject.capabilities` to `mWindowManagerConnector->createDisplay(...)` on both
paths. For Sky RALF packages, decide explicitly:

* **Recommended:** emit the Sky token set (so `resolution=`, `extraMounts=`, window-manager
  capabilities keep working) **plus** the RALF permission URNs, and **omit `spec=`**
  (the OCI config replaces it).
* Do **not** emit `spec=` for RALF packages — it is dead weight and risks a downstream
  consumer preferring the Dobby spec over the OCI bundle.

---

## Summary — can we lift the existing RALF code directly?

| Component | Lift as-is? |
|---|---|
| `RuntimeManager/ralf/RalfOCIConfigGenerator` | ✅ Yes, unmodified |
| `RuntimeManager/ralf/RalfPackageBuilder` | ✅ Yes, unmodified |
| `RuntimeManager/ralf/RalfSupport`, `RalfConstants`, `NetworkConfigurationHelper` | ✅ Yes, unmodified |
| `PackageManagerImplementation::getRuntimeConfig` | ✅ Yes, unmodified |
| `RuntimeManagerImplementation` RALF `#ifdef` blocks | ❌ **No — must become runtime dispatch** (GAP-1, GAP-2) |
| `USE_LIBPACKAGE_RALF` in `PackageManager` | ⚠️ Keep **OFF** for the Sky image (GAP-3) |
| `libpackage`'s `RalfPackageImpl` as a whole | ❌ No — mine it for the mount/serialize/verify algorithms only |
| appserviced `RalfPackageExtractor` / `RalfMetadataConverter` / `RalfMountedPackage` | ✅ **Port ~1:1** — this is the parity target |
