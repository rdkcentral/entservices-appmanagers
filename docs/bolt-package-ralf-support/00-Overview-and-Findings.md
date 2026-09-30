# Bolt / RALF Package Support in `libpackage-sky` — Overview & Verified Findings

> **STATUS: implemented on branch `topic/entos-ralf-support`.**
> The delivered design is a *simplification* of Model C described in
> `01-Architecture-and-Design.md`. Instead of porting appsserviced's
> `RalfPackageExtractor`/`RalfMetadataConverter`/`RalfMountedPackage` into the
> `PackageExtractor`/`PackageDatabase` pipeline, `libpackage-sky` gained a single
> self-contained `RalfPackageHandler` that owns the RALF flow end to end, with
> `PackageImpl` dispatching to it. Rationale and the list of intentionally omitted
> appsserviced behaviour are documented in the header comment of
> `ref/libpackage-sky-ralf/RalfPackageHandler.h`. Everything below about the data
> contracts, the `ralfPkgPath` discriminator and the `RuntimeManager` gap analysis
> still applies verbatim.
>
> Also resolved since this document was written: **Open Question #3** —
> `rdkcentral/eshelpers` `origin/develop` (tag `1.6.0`) is the authoritative
> `IPackageImpl.h`; it *does* declare `GetInstalledPackageMetadata` and
> `GetConfigListForInstalledPackages`, so **no interface change was required**.

> Scope: add RALF-Utils (`libralf`) backed package support to `ref/libpackage-sky-ralf`
> (the private fork of `libpackage-sky`) so that **Bolt Packages** install and launch through
> `PackageManager` → `RuntimeManager` → `RALFOCIGenerator` → Dobby, **without regressing**
> the existing EntOS Widget (`.wgt`) flow and **without breaking** the generic
> `libpackage` RALF users already consuming `entservices-appmanagers`.

---

## 1. Repository map (as cloned in this workspace)

| Path | Role | Edit? |
|---|---|---|
| `ref/libpackage` | Generic/vendor-neutral `libPackage.so`. **Now 100% RALF-only** — `src/CMakeLists.txt` builds only `RalfPackageHandler.cpp` and hard-requires `find_package(libralf REQUIRED)`. | Read-only reference |
| `ref/libpackage-sky-ralf` | **Private fork of Comcast/Sky `libPackage.so`. THE TARGET REPO.** Currently EntOS-Widget-only. | **All libpackage edits go here** |
| `ref/libpackage-sky` | Upstream `rdk-e/libpackage-sky`, byte-identical to the fork at `74f197c`. | Read-only diff baseline |
| `ref/legacyappserviced` | `entos-xe/appinfrastructure`. `main` has **no** RALF code, but branch `RDKEMW-24204_ralf_perms_workaround` (HEAD `8de682d`) carries a **complete, production RALF implementation**. | Read-only — **the parity target** |
| `PackageManager/`, `RuntimeManager/` | Thunder plugins in this repo. Already RALF-aware for the generic backend. | Minimal, additive, gated edits |
| `ref/eshelpers/packager/IPackageImpl.h` | **Authoritative** `IPackageImpl` / `ConfigMetaData` — this is the copy `build_dependencies.sh` installs into the libPackage build. | Read-only |

### Correction to an earlier assumption

A previous investigation recorded that `legacyappserviced` "has zero references to ralf".
That is true **only of `main`**. `git ls-remote` shows RALF branches, and
`RDKEMW-24204_ralf_perms_workaround` contains the full appserviced RALF stack:

```
RDK/AppManager/PackageManager/source/RalfPackageExtractor.{h,cpp}   (70 / 444 lines)
RDK/AppManager/PackageManager/source/RalfMetadataConverter.{h,cpp}  (96 / 1070 lines)
RDK/AppManager/PackageManager/source/RalfMountedPackage.{h,cpp}     (75 / 103 lines)
RDK/AppManager/PackageManager/test/testRalfPackageExtractor.cpp
RDK/AppManager/PackageManager/test/testLibRalfMetaData.cpp
RDK/AppManager/PackageManager/test/widgets/*.ralf, *.ralf.tar
```

**This is the "appserviced level of RALF support" the work must reach parity with, and it is
directly portable because `libpackage-sky` is a lift of the very same
`RDK/AppManager/PackageManager` code base.**

To re-create the reference worktree:

```bash
cd ref/legacyappserviced
git fetch --depth=1 origin RDKEMW-24204_ralf_perms_workaround
git worktree add /tmp/appsvc-ralf FETCH_HEAD
```

---

## 2. Terminology resolution — what is a "Bolt Package"?

No repository in scope contains the string "Bolt". The only concrete RALF package format
available anywhere is the `ralf-utils` / `libralf` format.

**Decision:** treat *Bolt Package* ≡ *`libralf`-format package* (`.ralf`, aka
`ralf::Package::Format::Ralf`, referred to as "OCI / RALF package" in appserviced config
comments). If evidence of a distinct layered format surfaces in upstream `rdkcentral/ralf-utils`
docs, **stop and reconcile** rather than guessing a second format.

---

## 3. The single most important technical finding

**`libralf` is a *unified* reader for BOTH package formats.** From
`RalfPackageExtractor::extractAndVerify` (appserviced):

```cpp
if (package->format() == ralf::Package::Format::Widget)
    *packageFormat = packagemanager::PackageFormat::ZIP;
else
    *packageFormat = packagemanager::PackageFormat::RALF;
```

and from `PackageExtractor::selectExtractor` (appserviced, libralf ≥ 1.0.2):

```cpp
isWidget = (LIBRALF_NS::Package::detectPackageFormat(widgetFd) == LIBRALF_NS::Package::Format::Widget);
```

This is exactly what AC2 asks for — *"correctly identify the package type and process the
package using the appropriate RALF Utils workflow **without requiring separate installation
mechanisms**"*. Detection is by **content**, from an already-open `fd`, not by file extension.

Consequence: package-type dispatch is a one-line libralf call, not a bespoke magic/manifest
sniffer, and it does **not** require any new field in `ConfigMetaData`, `IPackageManager.h`
or the JSON-RPC contract.

---

## 4. Three RALF implementations compared

| | `ref/libpackage` `RalfPackageImpl` | appserviced `Ralf*` (branch) | `ref/libpackage-sky-ralf/RALF/PackageImpl.cpp` |
|---|---|---|---|
| Status | Current, shipping, interface-complete | Current, shipping, tested | **Dead code** — not in any `add_subdirectory`, builds a separate `PackageRALF` target |
| Shape | Whole-alternate `IPackageImpl` | **Pluggable `IPackageExtractor` + `IMountedPackage` inside the existing pipeline** | Whole-alternate `IPackageImpl` |
| Handles widgets? | No (RALF only) | **Yes — one extractor, both formats** | No |
| Metadata model | Writes `ConfigMetaData` directly | Converts to the shared `PackageMetadata` struct (1070-line converter) | Writes `ConfigMetaData` directly |
| Downstream container config | `ralfPkgPath` JSON → `RalfOCIConfigGenerator` | appserviced's own Dobby spec path | `ralfPkgPath` JSON |
| Reuses Sky logic (secrets, DIAL, DataImage, UserId, sysconfig) | No | **Yes — all of it, unchanged** | No |
| Missing current interface methods | — | n/a | `GetInstalledPackageMetadata`, `GetConfigListForInstalledPackages` |
| Install layout | `<AppInstallationPath>/<id>/<ver>/package.ralf` | appserviced package store | Hardcoded `/media/apps/sky/packages/`, `package.wgt`, `/etc/sky/certs/production.crt` |

**Verdict on `RALF/PackageImpl.cpp`:** do **not** resurrect or wire it in. It bypasses
`PackageExtractor`/`PackageDatabase` entirely, so every Sky feature added since it was written
(`SystemConfigProviderRegistry`, `SystemInfoRegistry`, `PackageSecretsParser` +
`XmlDSigVerifierFactory`, `DataImageManager`, `UserIdManager`, the `epgAppId` /`extraMounts`
migration bind-mount, `resolution=` capability, DIAL/XCast registration) would be silently lost.
Harvest ideas only — its `populateConfigMetaData()` permission parsing
(`FIREBOLT_PERMISSION`, thunder/internet/homeApp flags) is a useful cross-check when writing
the permission→capability mapping.

---

## 5. Must-not-break Sky logic in `PackageImpl.cpp` (803 lines, the only live impl)

| Feature | Where | Why it matters |
|---|---|---|
| `BuiltinDeviceSystemInfo` + `SystemInfoRegistry` | ctor, anon namespace | Reads `MODEL_NUM`/`COUNTRY_CODE`; **keys secrets decryption**. External `rdkappmanagers-sysconfig` may replace it via `setSystemInfo()`. |
| `PackageSecretsParser` + `XmlDSigVerifierFactory` | ctor | Per-device app-secret decryption |
| `DataImageManager`, `UserIdManager` | ctor | `data.img` provisioning, stable per-app uid/gid |
| `epgAppId` → `extraMounts` capability | `Install`, `GetPackageMetaData` | Deliberate: migration bind-mount rides the generic `capabilities` transport so Sky-proprietary paths stay out of the shared repo |
| `resolution=<height>` capability | `GetPackageMetaData` | Consumed by `SetScale` in appmanagers |
| `spec=<escaped dobby json>` capability | `Lock`, via `ISystemConfigProvider::getDobbySpec()` | **The entire EntOS Widget container-config path.** RALF packages must *not* go through this. |
| `GetConfigListForInstalledPackages("dial")` | DIAL/XCast registration | `AI_Dial_Utilities.cpp` |

---

## 6. Data contract that already works end-to-end (generic backend)

```
libpackage RalfPackageImpl::Lock
  └─ writes /tmp/<id>_<ver>_metadata.json
       { "packages": [ { "pkgMountPath": "...", "pkgMetaDataPath": "..." }, ... ] }
  └─ ConfigMetaData.ralfPkgPath = <that path>
        │
PackageManagerImplementation::getRuntimeConfig(ConfigMetaData&, RuntimeConfig&)   [line ~959]
  └─ runtimeConfig.ralfPkgPath = config.ralfPkgPath
        │
RuntimeManagerImplementation::generate()          [line ~524, #ifdef RALF_PACKAGE_SUPPORT_ENABLED]
  └─ ralf::RalfPackageBuilder::generateRalfDobbySpec()
       ├─ ralf::parseRalPkgInfo()  → vector<RalfPkgInfoPair> = <pkgMetaDataPath, pkgMountPath>
       ├─ generateOCIRootfs()      → overlayfs lowerdirs = gpu-layer:pkgMount1:pkgMount2...
       └─ RalfOCIConfigGenerator::generateRalfOCIConfig()  → /tmp/ralf/<instance>/config.json
        │
mOciContainerObject->StartContainer(containerId, <ociRootfsPath>, ...)   [legacyContainer == false]
```

Per-package `pkgMetaDataPath` JSON is the RDK package config
(`application/vnd.rdk.package.config.v1+json`), read by `RalfOCIConfigGenerator` for
`configuration`, `packageType`, `entryPoint`, `entryArgs`, and the
`urn:rdk:config:{env,memory,storage,network,overrides}` sections.

---

## 7. Open questions to resolve before/while coding

1. **Do Sky-built `.ralf` packages carry the `application/vnd.rdk.package.config.v1+json`
   aux-metadata file?** `RalfOCIConfigGenerator` is useless without it.
   If not, `libpackage-sky` must **synthesize** it from the converted `PackageMetadata`
   (see `03-Implementation-Plan-libpackage-sky.md` §6). Confirm with the RALF packaging team.
2. **`libralf` version available in the Sky/RDK-E sysroot.** `detectPackageFormat()` needs
   ≥ 1.0.2; `setMaxExtractionBytes/Entries()` need ≥ 1.0.1. This repo's
   `build_dependencies.sh` pins `ralf-utils` tag `v1.2.0`, which satisfies both.
3. **Authoritative `IPackageImpl.h`.** `ref/IPackageImpl.h` is stale;
   `ref/eshelpers/packager/IPackageImpl.h` adds `capabilities` + `md5Hash` and is what CI
   installs. **Neither** declares `GetInstalledPackageMetadata` /
   `GetConfigListForInstalledPackages` as virtuals, yet
   `PackageManagerImplementation.cpp:1426` calls them — the real build-time header in the
   Sky sysroot must be newer than both local copies. Confirm before changing signatures.
4. **Does a Sky RALF app still need a `data.img` / private data image?** The generic RALF path
   never creates one. Decide whether `DataImageManager` participates in the RALF branch.

---

## 8. Document set

| Doc | Contents |
|---|---|
| `00-Overview-and-Findings.md` | This file |
| `01-Architecture-and-Design.md` | Target architecture, integration model, diagrams, contracts |
| `02-Gap-Analysis-and-Breaking-Changes.md` | What breaks today; required changes in `PackageManager`/`RuntimeManager` |
| `03-Implementation-Plan-libpackage-sky.md` | File-by-file work inside `ref/libpackage-sky-ralf` |
| `04-Implementation-Plan-entservices.md` | Minimal additive work in `PackageManager/` and `RuntimeManager/` |
| `05-Build-CI-and-Packaging.md` | CMake options, `build_dependencies.sh`, Yocto recipe notes |
| `06-Test-Plan-and-AC-Traceability.md` | Unit/L0/L1 tests mapped to AC1–AC7 |
| `07-Phasing-Risks-and-Rollback.md` | Delivery phases, risk register, rollback |
