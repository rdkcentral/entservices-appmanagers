# Test Plan & Acceptance-Criteria Traceability

## 1. Existing RALF test assets in this repo (reuse, don't recreate)

| Path | Covers |
|---|---|
| `Tests/L0Tests/RuntimeManager/ralf/Ralf_SupportTests.cpp` | `parseRalPkgInfo`, `getRalfUserInfo`, overlay helpers |
| `Tests/L0Tests/RuntimeManager/ralf/Ralf_PackageBuilderTests.cpp` | `generateRalfDobbySpec` incl. empty-`ralfPkgPath` case |
| `Tests/L0Tests/RuntimeManager/ralf/Ralf_OCIConfigGeneratorTests.cpp` | `generateRalfOCIConfig`, base-spec-missing handling |
| `Tests/L1Tests/tests/test_Ralf.cpp` | End-to-end `parseRalPkgInfo` failure/success branches through `RuntimeManagerImplementation` |
| `Tests/L1Tests/tests/test_LifecycleManager.cpp:294` | `runtimeConfigObject.ralfPkgPath = "/tmp/ralf"` plumb-through |
| `Tests/mocks/IPackageImplDummy.h:80` | Mock `ConfigMetaData.ralfPkgPath` |

⚠️ `Test_RalfPackageBuilder_GenerateRalfDobbySpec_EmptyRalfPkgPathReturnsFalse` asserts that
an empty `ralfPkgPath` makes `generateRalfDobbySpec()` return `false`. **That test stays valid**
— CHANGE-1 moves the empty-path decision *up* into `generate()`, it does not change
`generateRalfDobbySpec()`. Do **not** modify this test.

---

## 2. New/updated tests by layer

### 2.1 `libpackage-sky-ralf` unit tests (`tests/`)

| ID | Test | Asserts |
|---|---|---|
| U-01 | `RalfPackageExtractor` opens+verifies a valid `.ralf` | non-null `PackageMetadata`, `format == RALF` |
| U-02 | `RalfPackageExtractor` opens a `.wgt` through libralf | `format == ZIP`, metadata equals the zip-path result |
| U-03 | Truncated / corrupted `.ralf` | returns `nullptr`, no exception, no filesystem writes |
| U-04 | `.ralf` signed by an untrusted CA | returns `nullptr` |
| U-05 | `.ralf` exceeding `maxExtractedBytes` / `maxExtractedFiles` | extraction refused |
| U-06 | `RalfMetadataConverter` over `all-configs.ralf` | every `convert*` field mapped (golden-value table) |
| U-07 | `RalfMetadataConverter` secrets, `AllowSecretsFailure` set/unset | tolerant vs strict behaviour |
| U-08 | `PackageExtractor::selectExtractor` dispatch | `.wgt`→zip, `.ralf`→ralf, `.ralf` with support OFF→`nullptr` |
| U-09 | `PackageExtractor` fd position | fd is rewound to 0 after `detectPackageFormat` |
| U-10 | `RalfPackageMounter` mount/unmount roundtrip | mount point created, `config.json` emitted, removed on last unlock |
| U-11 | `RalfPackageMounter` refcounting | 2×lock + 1×unlock keeps the mount; 2nd unlock removes it |
| U-12 | `RalfPackageMounter` dependency recursion | **`MountInfo` order is post-order (deps first, app last)** |
| U-13 | `RalfPackageMounter` unsatisfiable dependency | `false`, **zero** residual mounts (rollback) |
| U-14 | `RalfPackageMounter` verify failure mid-tree | rollback of already-mounted deps |
| U-15 | `RalfPackageMounter` path-traversal id/version (`../`, `/`) | rejected before any filesystem call |
| U-16 | `serializeToJson` output | exact keys `packages[].pkgMountPath` / `pkgMetaDataPath` |
| U-17 | `emitPackageConfig` aux-metadata present | file copied verbatim |
| U-18 | `emitPackageConfig` aux-metadata absent | synthesized doc has `configuration` object + `entryPoint` |
| U-19 | `PackageImpl::Lock` on a RALF package | `ralfPkgPath` set, `capabilities` contains **no** `spec=` |
| U-20 | `PackageImpl::Lock` on a widget | `capabilities` contains `spec=`, `ralfPkgPath` **empty** |
| U-21 | `PackageImpl::Lock` RALF mount failure | returns `FAILED`, `ralfPkgPath` **empty** |
| U-22 | `PackageImpl::Unlock` on a RALF package | unmounted, `/tmp/<id>_<ver>_metadata.json` removed |
| U-23 | `PackageImpl::Install` id/version mismatch | `VERSION_MISMATCH`, nothing persisted |
| U-24 | EPG widget still gets `extraMounts=` | migration bind-mount capability unchanged |
| U-25 | Widget still gets `resolution=<h>` | unchanged |

Fixtures to copy from the appserviced worktree:
`all-configs.ralf`, `simple.ralf.tar`, `simple-firstparty.ralf.tar`,
`ralf-144mb.tar`, `ralf-16k-files.tar`, `test_0{1,2}_package.ralf`.

### 2.2 `entservices-appmanagers` L0 tests

| ID | Test | Asserts |
|---|---|---|
| L0-01 | `generate()` with empty `ralfPkgPath`, `RALF_PACKAGE_SUPPORT_ENABLED` built | routes to `DobbySpecGenerator`, returns a Dobby JSON spec |
| L0-02 | `generate()` with non-empty `ralfPkgPath` | routes to `RalfPackageBuilder` |
| L0-03 | Existing `Ralf_*Tests.cpp` suites | **unchanged, all still green** |

Add L0-01/L0-02 to `Tests/L0Tests/RuntimeManager/ralf/Ralf_PackageBuilderTests.cpp` (or a new
`Ralf_GenerateDispatchTests.cpp` registered in `RuntimeManagerTest.cpp`).

### 2.3 `entservices-appmanagers` L1 tests

| ID | Test | Asserts |
|---|---|---|
| L1-01 | Widget launch with `RALF_PACKAGE_SUPPORT=ON` | `StartContainerFromDobbySpec` called, **not** `StartContainer`; per-app uid/gid preserved; no overlayfs mount attempted |
| L1-02 | RALF launch | `StartContainer(containerId, ociRootfsPath, ...)` called; uid/gid = `ralf` user |
| L1-03 | Mixed sequence: widget → RALF → widget | each takes its own path; no cross-contamination |
| L1-04 | RALF container terminate | `unmountOverlayfsIfExists` called once |
| L1-05 | Widget container terminate | `unmountOverlayfsIfExists` **not** called |
| L1-06 | RALF container start failure | overlayfs unmounted, `mRuntimeAppInfo` cleaned |
| L1-07 | `ralfPkgPath` points at a non-existent file | `generate()` returns false, **no** `StartContainer` call |

Extend `Tests/L1Tests/tests/test_Ralf.cpp` — it already builds `RuntimeConfig` with
`ralfPkgPath` and has a valid `mPkgInfoFile` fixture.

### 2.4 Device / integration

| ID | Test |
|---|---|
| D-01 | Install + launch an EntOS Widget on a RALF-enabled image; compare Dobby spec byte-for-byte with a pre-change build |
| D-02 | Install + launch a Bolt Package; verify `/tmp/ralf/<instance>/config.json` is a valid OCI config and the container reaches RUNNING |
| D-03 | Bolt app renders (validates GAP-7: westeros socket bind-mounted by the OCI config, since RuntimeManager passes `""`) |
| D-04 | Bolt app with a runtime dependency: confirm overlayfs `lowerdir` order (`mount \| grep overlay`) |
| D-05 | Corrupted `.ralf` over the real install path → error surfaced to the caller, no container, no `/tmp/ralf/<instance>` |
| D-06 | `.packages.ralf.enable = false` runtime kill switch → widgets still work, `.ralf` install refused |
| D-07 | DIAL registration for a Bolt app with `dialInfo` |

---

## 3. AC traceability matrix

| AC | Requirement | Design element | Verified by |
|---|---|---|---|
| **AC1** | Bolt Package installed via RALF Utils; metadata & contents registered | `RalfPackageExtractor` + `RalfMetadataConverter` + `PackageDatabase::addPackageAt(..., format)` | U-01, U-06, U-23, D-02 |
| **AC2** | Correct type identification; one installation mechanism | `ralf::Package::detectPackageFormat(fd)` in `PackageExtractor::selectExtractor`; `PackageImpl::Install` is format-agnostic | U-02, U-08, U-09, L1-03, D-01, D-02 |
| **AC3** | Config generated from installed artifacts; OCI-compatible | `RalfPackageMounter::emitPackageConfig` (aux-metadata or synthesized) + `serializeToJson` → `ralfPkgPath` | U-10, U-16, U-17, U-18, D-02 |
| **AC4** | RuntimeManager invokes RALFOCIGenerator; OCI config produced | `generate()` runtime dispatch → `RalfPackageBuilder` → `RalfOCIConfigGenerator` (unmodified) | L0-02, L1-02, D-02 |
| **AC5** | Container launched from the generated OCI config; reaches running | `legacyContainer == false` → `StartContainer(ociRootfsPath, ...)` | L1-02, D-02, D-03, D-04 |
| **AC6** | EntOS Widget install/config/launch unchanged | `ENABLE_RALF_PACKAGE_SUPPORT=OFF` default build; `selectExtractor` returns `this`; `ralfPkgPath` empty → `DobbySpecGenerator` + `StartContainerFromDobbySpec`; `spec=`/`extraMounts=`/`resolution=` untouched | U-19, U-20, U-24, U-25, L0-01, L1-01, L1-05, D-01, D-06, CI `readelf` assertion |
| **AC7** | Invalid/incomplete Bolt Package fails gracefully; no OCI config, no launch | Staged-verify-then-commit in `Install`; verify-before-mount and rollback in `RalfPackageMounter::lock`; `ralfPkgPath` assigned **only after** successful mount + serialize | U-03, U-04, U-05, U-13, U-14, U-15, U-21, L1-07, D-05 |

---

## 4. Regression gate before merge

1. `libpackage-sky-ralf` builds with the option **OFF** and
   `readelf -d libPackage.so | grep -i ralf` is empty.
2. Full existing `libpackage-sky-ralf` test suite green in the OFF build.
3. All pre-existing `Tests/L0Tests/RuntimeManager/ralf/*` and
   `Tests/L1Tests/tests/test_Ralf.cpp` cases green **unmodified**.
4. Widget Dobby spec diff (D-01) is empty against a pre-change build.
5. Generic-`libpackage` image (`USE_LIBPACKAGE_RALF=ON`, `RALF_PACKAGE_SUPPORT=ON`) launches a
   RALF app exactly as before CHANGE-1.
