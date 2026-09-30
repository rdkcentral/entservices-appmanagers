# Phasing, Risks and Rollback

## 1. Delivery phases

Each phase is independently mergeable and leaves the tree shippable.

### Phase 0 — Unblock and de-risk (no product code)
* Recreate the appserviced RALF worktree and diff `PackageMetadata.h` between appserviced and
  `libpackage-sky-ralf`; publish the field delta.
* Diff `IMountedPackage` (appserviced) vs `IExtractedPackage` (libpackage-sky).
* Settle **Open Question #3**: identify the real build-time `IPackageImpl.h` in the Sky sysroot.
* Settle **Open Question #1**: confirm whether Sky `.ralf` packages carry
  `application/vnd.rdk.package.config.v1+json`.
* Obtain a sample Sky Bolt Package + its signing CA.
* **Exit:** a written delta report + a real `.ralf` fixture in `tests/`.

### Phase 1 — RuntimeManager runtime dispatch (this repo)
* CHANGE-1 … CHANGE-6 from `04-Implementation-Plan-entservices.md`.
* L0-01/L0-02, L1-01 … L1-07.
* **Ships value on its own:** an image with `RALF_PACKAGE_SUPPORT=ON` can finally run widgets
  *and* RALF packages. Today it can only run one.
* **Exit:** generic-libpackage RALF image behaves identically; a widget launches on a
  `RALF_PACKAGE_SUPPORT=ON` build.

### Phase 2 — libralf plumbing in libpackage-sky (no behaviour change)
* `ENABLE_RALF_PACKAGE_SUPPORT` option, `find_package(libralf)`, empty `Ralf*.cpp` skeletons.
* Widen `IPackageExtractor`; add `PackageFormat`; config knobs.
* `PackageExtractor` split into `…Zip()` private methods + `selectExtractor()` that always
  returns `this` while `mRalfExtractor` is null.
* **Exit:** both ON and OFF builds green; OFF build binary-equivalent; CI job added.

### Phase 3 — Metadata + extraction (the bulk)
* Port `RalfMetadataConverter` (XL) and `RalfPackageExtractor`.
* U-01 … U-09.
* **Exit:** a `.ralf` can be opened, verified and converted into a `PackageMetadata`;
  `Install` of a Bolt Package succeeds and the package appears in `ListPackages`. **AC1, AC2.**

### Phase 4 — Mount + config emission
* `RalfMountedPackage`, `RalfPackageMounter`, `PackageDatabase` format awareness.
* U-10 … U-18.
* **Exit:** `Lock` mounts the tree and writes a valid `ralfPkgPath` document. **AC3.**

### Phase 5 — End-to-end launch
* `PackageImpl::Lock`/`Unlock`/`GetFileMetadata` branching.
* U-19 … U-25, D-01 … D-07.
* **Exit:** a Bolt app launches and renders; widgets unregressed. **AC4, AC5, AC6, AC7.**

### Phase 6 — Cleanup
* Delete/quarantine `ref/libpackage-sky-ralf/RALF/`.
* Update `RuntimeManager/DobbySpecGeneration.md`, `SkyEpgMigrationBindMount_ImplPlan.md`,
  `spec_coverage.md`.
* Decide on CHANGE-8 (`packageFormat` propagation) as a follow-up ticket.

---

## 2. Risk register

| ID | Risk | L | I | Mitigation |
|---|---|---|---|---|
| R-01 | `RalfMetadataConverter` port (1070 lines) silently mis-maps a field → subtle app misbehaviour | H | H | Port verbatim, no "improvements". Golden-value table test (U-06) over `all-configs.ralf`. Port appserviced's `testLibRalfMetaData.cpp` too. |
| R-02 | `PackageMetadata` structs have diverged; converter references absent fields | H | M | Phase 0 diff gate. Add missing fields **additively** with defaults. |
| R-03 | Sky `.ralf` packages lack the RDK aux config.json → `RalfOCIConfigGenerator` hard-fails | M | **H** | Open Question #1 resolved in Phase 0. `emitPackageConfig` synthesis fallback (§6 of doc 03) implemented regardless. |
| R-04 | Touching `RuntimeManagerImplementation::generate()` regresses the shipping generic RALF product | M | **H** | Predicate is always-true for that product. Keep all existing RALF L0/L1 tests unmodified. Optional `RALF_STRICT_MODE`. |
| R-05 | `MountInfo` ordering wrong → overlayfs layer precedence inverted; app loads the wrong files | M | H | Explicit test U-12. Document the reverse-iteration in `RalfPackageBuilder::generateOCIRootfsPackage`. |
| R-06 | `PackageExtractor` returning `this` from `selectExtractor` causes infinite recursion | M | M | Split zip logic into private `…Zip()` methods; add a unit test with a widget fd. |
| R-07 | `detectPackageFormat()` consumes fd bytes; later readers see a short file | M | M | `lseek(fd,0,SEEK_SET)` before and after; test U-09. |
| R-08 | Widget uid/gid silently changed to the `ralf` user on a RALF-enabled image | M | H | CHANGE-5 runtime predicate + L1-01 asserting per-app uid/gid. |
| R-09 | Overlayfs mounts leak when `mRuntimeAppInfo` is erased before the cleanup block | M | M | CHANGE-3 ordering note; test L1-06. |
| R-10 | Image prerequisites (`oci-base-spec.json`, gpu-layer, `ralf` user) missing on the Sky box | M | H | Prerequisite table in doc 05 tracked as recipe work; D-02 fails loudly without them. |
| R-11 | `USE_LIBPACKAGE_RALF` accidentally enabled on the Sky image → double runtime lock | L | H | Flag matrix documented; add a CI/recipe assertion. |
| R-12 | `libralf` version on the Sky sysroot < 1.0.2 → no `detectPackageFormat` | L | H | Version gates already in the ported code; falls back to "always widget", which fails safe. Pin ≥ 1.2.0. |
| R-13 | Path traversal via a hostile `packageId`/`version` into mount paths | L | H | `isPathSafePackageKey()` ported from libpackage; test U-15. |
| R-14 | Partial install leaves a half-written package on power loss | L | M | Staged `.tmp` + verify-staged-copy + fsync + atomic rename, exactly as `RalfPackageHandler::Install`. |
| R-15 | Bolt app launches but renders nothing (westeros socket not bind-mounted by the OCI config) | M | H | GAP-7; validated by D-03 before sign-off. |
| R-16 | `resolution=` / `extraMounts=` capabilities not applied to Sky RALF packages | M | M | `GetPackageMetaData` runs for both formats — assert explicitly in U-24/U-25 variants for RALF. |

---

## 3. Rollback

| Level | Action | Blast radius |
|---|---|---|
| Field (no rebuild) | `"packages": { "ralf": { "enable": false } }` in `/etc/sky/aisettings.json` | Bolt install refused; widgets unaffected; `ralfPkgPath` never set so RuntimeManager takes the legacy path for everything |
| Recipe | `ENABLE_RALF_PACKAGE_SUPPORT=OFF` for `libpackage-sky` | `libPackage.so` back to today's binary behaviour |
| Recipe | `RALF_PACKAGE_SUPPORT=OFF` for `entservices-appmanagers` | All `ralf/` code compiled out; `generate()` is the legacy path only |
| Git | Revert Phase 1 commit | Restores compile-time exclusivity (and reintroduces GAP-1) |

The layered gating is deliberate: **no phase requires a git revert to disable in the field.**

---

## 4. Decisions log

| # | Decision | Rationale |
|---|---|---|
| D1 | "Bolt Package" ≡ `libralf`-format package (`.ralf`) | Only concrete RALF format in any repo in scope; no competing spec found |
| D2 | Model C hybrid (libralf reader inside the Sky pipeline + `ralfPkgPath` transport) | Model A fails AC4/AC5; Model B fails AC6 by discarding all Sky-specific logic |
| D3 | Port appserviced `Ralf*` classes rather than `libpackage`'s `RalfPackageImpl` | appserviced is the direct ancestor of `libpackage-sky`; its RALF design already coexists with widgets in one binary |
| D4 | `ref/libpackage-sky-ralf/RALF/PackageImpl.cpp` is **not** revived | Bypasses `PackageExtractor`/`PackageDatabase`; loses sysconfig, secrets, DIAL, DataImage, UserId, `extraMounts`, `resolution=`; missing current interface methods |
| D5 | Package-type detection uses `ralf::Package::detectPackageFormat(fd)`, not file extension | Content-based, already proven in appserviced, satisfies AC2's "no separate mechanism" |
| D6 | `ralfPkgPath` non-empty is the **sole** RALF-mode discriminator end to end | Already plumbed through `getRuntimeConfig`; needs no new field, no API change, no JSON-RPC change |
| D7 | RALF packages do **not** get the `spec=` capability | The OCI bundle from `RalfOCIConfigGenerator` replaces the Dobby spec; emitting both invites a consumer to pick the wrong one |
| D8 | Sky image builds with `USE_LIBPACKAGE_RALF=OFF` | `libpackage-sky::Lock` already resolves the runtime companion; the flag would double-lock and clobber `runtimePath` |
| D9 | New CMake option named `ENABLE_RALF_PACKAGE_SUPPORT`, default OFF | Matches `libpackage-sky`'s `option(...)` style and mirrors `RALF_PACKAGE_SUPPORT`; default OFF guarantees AC6 |
| D10 | Config knobs mirror appserviced (`.packages.ralf.enable`, `.packages.ralf.useRalfUtilsForWidgets`) | Operational parity and a field-serviceable kill switch |
