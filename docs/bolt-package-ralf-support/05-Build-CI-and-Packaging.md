# Build, CI and Packaging

## 1. CMake option matrix

| Repo | Option | Default | Effect |
|---|---|---|---|
| `libpackage-sky-ralf` | `ENABLE_RALF_PACKAGE_SUPPORT` (**new**) | `OFF` | `find_package(libralf REQUIRED)`, compile `Ralf*.cpp`, define `-DENABLE_RALF_PACKAGE_SUPPORT`, link `ralf` |
| `entservices-appmanagers` | `RALF_PACKAGE_SUPPORT` | `OFF` | `-DRALF_PACKAGE_SUPPORT_ENABLED`, glob `RuntimeManager/ralf/*.cpp`, link `jsoncpp` |
| `entservices-appmanagers` | `USE_LIBPACKAGE_RALF` | `OFF` | `-DUSE_LIBPACKAGE_RALF`: extra runtime-package lock in `PackageManagerImplementation::Lock`, `backend="ralf"` log tag |
| `entservices-appmanagers` | `RALF_STRICT_MODE` (**new, optional**) | `OFF` | Restores fail-fast when `ralfPkgPath` is empty in a RALF-only image |
| `libpackage` (generic) | — | always RALF | `src/CMakeLists.txt` hard-requires `libralf` |

**Sky target image:** `ENABLE_RALF_PACKAGE_SUPPORT=ON`, `RALF_PACKAGE_SUPPORT=ON`,
`USE_LIBPACKAGE_RALF=OFF`, `RALF_STRICT_MODE=OFF`.

---

## 2. `ralf-utils` dependency

Already wired for the generic `libPackage` in this repo's `build_dependencies.sh`:

```bash
git clone -b v1.2.0 https://github.com/rdkcentral/ralf-utils.git

cmake -G Ninja -S ralf-utils -B build/ralf-utils \
    -DCMAKE_INSTALL_PREFIX="$GITHUB_WORKSPACE/install/usr" \
    -DCMAKE_MODULE_PATH="$GITHUB_WORKSPACE/install/tools/cmake" \
    -DGENERIC_CMAKE_MODULE_PATH="$GITHUB_WORKSPACE/install/tools/cmake"
cmake --build build/ralf-utils --target install
```

`v1.2.0` satisfies both version gates used by the ported code:
* `detectPackageFormat()` → libralf ≥ **1.0.2**
* `setMaxExtractionBytes()` / `setMaxExtractionEntries()` → libralf ≥ **1.0.1**

### Known configure hazards (recorded from the generic `libPackage` integration)

`find_package(libralf)` resolving is not sufficient — headers must also be on the include
path. Both flags below were required and must be replicated for `libpackage-sky-ralf`:

```bash
cmake -G Ninja -S libpackage-sky -B build/libpackage-sky \
    -DENABLE_RALF_PACKAGE_SUPPORT=ON \
    -DCMAKE_PREFIX_PATH="$GITHUB_WORKSPACE/install/usr" \
    -DCMAKE_CXX_FLAGS="-I${GITHUB_WORKSPACE}/install/usr/include -I/usr/include/jsoncpp" \
    -DCMAKE_SHARED_LINKER_FLAGS="-L${GITHUB_WORKSPACE}/install/usr/lib -L${GITHUB_WORKSPACE}/install/usr/lib64" \
    -DCMAKE_INSTALL_PREFIX="$GITHUB_WORKSPACE/install/usr" \
    -DCMAKE_MODULE_PATH="$GITHUB_WORKSPACE/install/tools/cmake"
```

Using `$<TARGET_PROPERTY:ralf-utils::libralf,INTERFACE_INCLUDE_DIRECTORIES>` in
`target_include_directories` (as appserviced does) is the cleaner fix and should make the
`-I` hack unnecessary — prefer it, keep the `-I` only if the imported target is unavailable.

---

## 3. `libPackage.so` provider is a packaging decision, not a link-time branch

`PackageManager/CMakeLists.txt` links a **fixed path**:

```
${SYSROOT_PATH}${CMAKE_INSTALL_PREFIX}/lib/libPackage.so
```

Both `ref/libpackage` and `ref/libpackage-sky-ralf` build a target that installs to exactly
that name. **Which one is present in the sysroot is chosen by the Yocto recipe / image
manifest**, not by CMake in this repo. Nothing in this plan changes that.

Add a CI assertion so the wrong provider cannot silently ship:

```bash
readelf -d $SYSROOT/usr/lib/libPackage.so | grep -q 'NEEDED.*libralf' \
  && echo "libPackage.so is RALF-enabled" || echo "libPackage.so is legacy"
```

---

## 4. CI additions

### 4.1 Add a `libpackage-sky-ralf` build job

The repo's CI currently builds `libPackage` (generic) only. Add a matrix job that builds the
Sky fork in **both** configurations, because AC6 depends on the OFF build being untouched:

| Job | Config | Assertion |
|---|---|---|
| `libpackage-sky-default` | `ENABLE_RALF_PACKAGE_SUPPORT=OFF` | `readelf -d libPackage.so \| grep -i ralf` is **empty**; unit tests pass |
| `libpackage-sky-ralf` | `ENABLE_RALF_PACKAGE_SUPPORT=ON` | links `libralf`; full unit-test suite incl. new RALF tests passes |

### 4.2 Header provisioning

`build_dependencies.sh` copies the authoritative interface header before building `libPackage`:

```bash
cp "${GITHUB_WORKSPACE}/eshelpers/packager/IPackageImpl.h" "${LIBPACKAGE_INCLUDE_DIR}/IPackageImpl.h"
```

Do the same for `libpackage-sky-ralf`. **This is also how to settle Open Question #3** —
whatever header this step installs is what the build actually compiles against. If it lacks
`GetInstalledPackageMetadata` / `GetConfigListForInstalledPackages` as virtuals, the CI build
of `libpackage-sky-ralf` will already be failing on the `override` specifiers today; check
that first.

### 4.3 Coverity / test-workflow onboarding

Per `.github/instructions/PluginOnboardingCompliance.instructions.md`, any new source files
added under the main tree must be reflected in the Coverity scan config and the test
workflows. This delivery adds **no new files** under `RuntimeManager/` or `PackageManager/`,
so no onboarding change is needed — only new **test cases** in existing test files.

---

## 5. Runtime / image prerequisites for the Sky RALF path

`RalfOCIConfigGenerator` and `RalfPackageBuilder` hard-depend on these being present on the
device. They are **recipe work, not code work**, and must land with the feature:

| Artefact | Constant | Consequence if missing |
|---|---|---|
| `/usr/share/ralf/oci-base-spec.json` | `RALF_OCI_BASE_SPEC_FILE` | `generateRalfOCIConfig()` returns false → launch fails |
| `/usr/share/gpu-layer/rootfs` | `RALF_GRAPHICS_LAYER_ROOTFS` | overlayfs lowerdir missing → mount fails |
| `/usr/share/gpu-layer/config.json` | `RALF_GRAPHICS_LAYER_CONFIG` | GPU dev-nodes / group-ids not injected |
| `ralf` system user + group | `RALF_USER_NAME` | `getRalfUserInfo()` fails, container runs with the wrong uid |
| `/tmp/ralf/` writable tmpfs | `RALF_APP_ROOTFS_DIR` | overlay upperdir/workdir creation fails |
| `/tmp/mounts/` writable | libpackage-sky mount root (configurable) | package mount fails |
| erofs + dm-verity + loop kernel support | — | `ralf::Package::mount()` fails |
| `libralf.so` on target | — | `libPackage.so` fails to load → PackageManager dead |
| Package signing CAs at the `IXmlDSigVerifierConfig` locations | — | `VerificationBundle` empty → every package fails verification |

`/tmp/mounts/` (libpackage default `RDK_PACKAGE_MOUNT_PATH`) should be made configurable in
`RalfPackageMounter` rather than hardcoded, so the Sky layout can differ.

---

## 6. Rollback switch

Because everything is gated, rollback is a rebuild, not a revert:

1. `ENABLE_RALF_PACKAGE_SUPPORT=OFF` in the `libpackage-sky` recipe → `libPackage.so`
   returns to today's widget-only behaviour.
2. `.packages.ralf.enable = false` in `/etc/sky/aisettings.json` → **runtime** disable
   without rebuilding: `mRalfExtractor` is never constructed, RALF fds are rejected,
   `ralfPkgPath` is never set, and `RuntimeManagerImplementation::generate()` takes the
   legacy branch for every app.

Option 2 is the field-serviceable kill switch and is the main reason the config knobs are
part of the design rather than compile-time-only.
