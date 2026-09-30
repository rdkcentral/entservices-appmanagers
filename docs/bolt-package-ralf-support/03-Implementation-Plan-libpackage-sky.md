# Implementation Plan — `ref/libpackage-sky-ralf`

All work in this document happens in the private fork `ref/libpackage-sky-ralf`.
Follow that repo's **existing style** (Sky header comments, `AI_LOG_*` logging, 4-space
indent, `final override`) — do **not** import `entservices-appmanagers` plugin conventions.

Reference worktree for porting:
```bash
cd ref/legacyappserviced && git fetch --depth=1 origin RDKEMW-24204_ralf_perms_workaround
git worktree add /tmp/appsvc-ralf FETCH_HEAD
# sources: /tmp/appsvc-ralf/RDK/AppManager/PackageManager/source/Ralf*.{h,cpp}
```

---

## 0. Task checklist

| # | Task | Files | Est. |
|---|---|---|---|
| 1 | Widen `IPackageExtractor` | `IPackageExtractor.h`, `PackageExtractor.cpp` | S |
| 2 | Add config knobs | `common/IPackageManagerConfig.h`, `Config/` | S |
| 3 | Port `RalfMetadataConverter` | new `RalfMetadataConverter.{h,cpp}` | **XL** |
| 4 | Port `RalfPackageExtractor` | new `RalfPackageExtractor.{h,cpp}` | M |
| 5 | Port `RalfMountedPackage` | new `RalfMountedPackage.{h,cpp}` | S |
| 6 | New `RalfPackageMounter` | new `RalfPackageMounter.{h,cpp}` | L |
| 7 | Turn `PackageExtractor` into a dispatcher | `PackageExtractor.{h,cpp}` | M |
| 8 | Format-aware `PackageDatabase` | `PackageDatabase.{h,cpp}`, `Package.{h,cpp}` | M |
| 9 | `PackageImpl` Lock/Unlock/GetFileMetadata branching | `PackageImpl.{h,cpp}` | M |
| 10 | CMake option + libralf wiring | `CMakeLists.txt` | S |
| 11 | Unit tests | `tests/` | L |
| 12 | Delete or quarantine dead `RALF/` dir | `RALF/` | XS |

---

## 1. Widen `IPackageExtractor` (`IPackageExtractor.h`)

The Sky interface is the *pre-RALF* appserviced shape. Widen it to the appserviced RALF shape,
minus the `id` parameter (Sky call sites don't have an app id at extract time):

```cpp
namespace packagemanager { enum class PackageFormat { ZIP, RALF }; }   // new, or reuse if present

class IPackageExtractor
{
public:
    virtual ~IPackageExtractor() = default;

    enum Flags
    {
        VerifyWithoutTime         = 0x1,
        ParseSecrets              = 0x4,
        SkipSignatureVerification = 0x8,   // new
        AllowSecretsFailure       = 0x10,  // new
    };

    inline std::shared_ptr<PackageMetadata> extractAndVerify(int widgetFd, int targetDirFd, unsigned flags) const
    { return extractAndVerify(widgetFd, targetDirFd, flags, nullptr, nullptr); }

    inline std::shared_ptr<PackageMetadata> extractAndVerify(int widgetFd, int targetDirFd, unsigned flags, size_t *extractedSize) const
    { return extractAndVerify(widgetFd, targetDirFd, flags, extractedSize, nullptr); }

    virtual std::shared_ptr<PackageMetadata> extractAndVerify(int widgetFd, int targetDirFd, unsigned flags,
                                                              size_t *extractedSize,
                                                              packagemanager::PackageFormat *packageFormat) const = 0;

    virtual bool extractAndVerifyFile(int widgetFd, const std::string &fileName, std::string *fileContent,
                                      size_t maxSize, unsigned flags) const = 0;

    virtual std::shared_ptr<PackageMetadata> extractAndVerifyMetaData(int widgetFd, unsigned flags) const = 0;
};
```

Inline overloads keep the two existing call sites in `PackageImpl.cpp`
(`extractAndVerify(filefd, -1, 0, nullptr)`) compiling unchanged.
Update the existing `PackageExtractor::extractAndVerify` signature and have it write
`PackageFormat::ZIP` when `packageFormat != nullptr`.

> `PackageFormat` does not currently exist in `libpackage-sky`. Add it to a small new header
> `PackageFormat.h` (or to `IPackage.h` alongside the existing capability enums) inside
> `namespace packagemanager`, mirroring appserviced's `IPackage.h:41`.

---

## 2. Config knobs (`common/IPackageManagerConfig.h` + `Config/`)

Mirror appserviced `IPackageManagerConfig.h:87-88` and `ConfigFile.cpp:278-279`:

```cpp
// common/IPackageManagerConfig.h
virtual bool getRalfPackagesEnabled()  const = 0;   // .packages.ralf.enable
virtual bool useRalfUtilsForWidgets()  const = 0;   // .packages.ralf.useRalfUtilsForWidgets
```

Read from `/etc/sky/aisettings.json` in `Config/` with **`false` defaults**:

```jsonc
"packages": {
  // Enables support for RALF (RDK Application Layer Format) / Bolt packages.
  // Requires the ralf-utils library to be present and libpackage-sky built with
  // ENABLE_RALF_PACKAGE_SUPPORT=ON.
  "ralf": {
    "enable": false,
    // If true, libralf also processes traditional (non-RALF) widgets.
    // Leave false in production until widget parity is proven.
    "useRalfUtilsForWidgets": false
  }
}
```

When `ENABLE_RALF_PACKAGE_SUPPORT=OFF`, both accessors return `false` unconditionally
(stub implementation compiled in place of the config read) so nothing else needs `#ifdef`s.

---

## 3. `RalfMetadataConverter` — the critical port (XL)

Source: `/tmp/appsvc-ralf/.../RalfMetadataConverter.{h,cpp}` (96 / 1070 lines).

**This class is why Model C works**: it produces the *same* `PackageMetadata` struct the
entire Sky pipeline already consumes, so secrets, DIAL, data-image sizing, user ids, resolution
and capability derivation all keep working untouched for RALF packages.

### Port mechanics

| appserviced dependency | libpackage-sky substitute |
|---|---|
| `AIPlatform::ISystemInfo` | ✅ exists (`common/ISystemInfo.h`) — same type |
| `packagemanager::IPackageSecretsParser` | ✅ exists (`IPackageSecretsParser.h`) — same type |
| `ISiftAnalyticsProducer`, `AI_SIFT_ERROR`, `SiftAnalytics.h`, `AnalyticsSiftHelper.h` | ❌ **not present** — strip. Replace every `AI_SIFT_ERROR(level, code, status, id, ver, msg, ...)` with `AI_LOG_ERROR(msg, ...)` |
| `EnumFlags.h` / `DECLARE_ENUM_FLAGS` | ❌ **not present** — replace with a plain `unsigned` bitmask + `enum { None=0, ParseSecrets=1<<0, AllowSecretsFailure=1<<1 }` |
| `Tracing.h` / `AI_TRACE_EVENT` | ❌ strip |
| `PackageMetadata.h` | ✅ exists — **diff the two versions first** |

### ⚠️ Pre-port gate: diff the `PackageMetadata` structs

```bash
diff -u /tmp/appsvc-ralf/AppInfrastructure/PackageManager/lib/include/PackageMetadata.h \
        ref/libpackage-sky-ralf/PackageMetadata.h
```

The appserviced version carries RALF-only fields that libpackage-sky may lack:
```cpp
std::vector<std::string> execArgs;                    // RALF packages only
std::map<std::string,std::string> environment;        // RALF packages only
```
Any field referenced by the converter but absent in the Sky struct must be **added
additively** (default-constructed, so the widget path is unaffected). Record the exact delta
in the PR description.

### Conversion surface to port

`convertApplicationInfo`, `convertServiceInfo`, `convertApplicationAndServiceInfo`,
`convertPermissions`, `convertIcons`, `convertDialInfo`, `convertPorts`, `convertAudioInfo`,
`convertInputHandlingInfo`, `convertLoggingLevels`, `convertDisplayInfo`,
`convertLifecycleStates`, `addVendorConfigs`, and the vendor-config family
(`convertPinManagementConfig`, `convertMediariteConfig`, `convertPreLaunchConfig`,
`convertAgeRatingConfig`, `convertAgePolicyConfig`, `convertContentPartnerIdConfig`,
`convertCataloguerIdConfig`, `convertFkpsConfig`, `convertParentControlConfig`,
`convertMulticastConfig`, `convertDrmConfig`, `convertMarketplaceInterceptConfig`,
`convertFirstPartyConfig`), `setEnvironment`, `processAppSecrets`.

Namespace it: `namespace packagemanager { class RalfMetadataConverter { ... }; }`.

> Port faithfully and **do not "improve"** the conversions — any deviation changes app
> behaviour in ways that are extremely hard to debug at container level.

---

## 4. `RalfPackageExtractor`

Source: `/tmp/appsvc-ralf/.../RalfPackageExtractor.{h,cpp}` (70 / 444 lines). Near-verbatim port.

Constructor substitutions:

| appserviced | libpackage-sky |
|---|---|
| `IXmlDSigVerifierConfig` (6 CA accessors) | ✅ exists — `XmlDSigVerifier/` provides `IXmlDSigVerifierConfig` with the same production/SI/developer/Comcast-ECC/Comcast-RSA accessors. Port `createVerificationBundle()` **verbatim**. |
| `ISecureDebugVerifier` + `ENABLE_SECURE_DEBUG_FEATURES` | ❌ not present — drop the `#if defined(...)` block and the ctor parameter |
| `ISiftAnalyticsProducer` | ❌ drop (see §3) |

Drop the `const std::string &id` first parameter from all three public methods to match the
Sky `IPackageExtractor` shape; use `package->id()` in log messages instead.

Keep verbatim:
* `openPackage()` — including the libralf-version-gated
  `setMaxExtractionBytes/Entries()` calls
* `extractPackage()` + `getDirectorySize()` (walks `/proc/self/fd/<dirFd>`)
* `readFile()` — `ralf::PackageReader` iteration with `maxSize` guard
* The `lseek(fd, 0, SEEK_SET)` rewind at the top of `extractAndVerify` / `extractAndVerifyFile`

Wire limits from config exactly as appserviced does:
```cpp
ralfExtractor->setMaxExtractedBytes(config->getMaxExtractedWidgetSize());
ralfExtractor->setMaxExtractedFiles(config->getMaxExtractedWidgetFiles());
```
(both accessors already exist on `IPackageManagerConfig`).

---

## 5. `RalfMountedPackage`

Source: `/tmp/appsvc-ralf/.../RalfMountedPackage.{h,cpp}` (75 / 103 lines).

appserviced implements `IMountedPackage`; libpackage-sky's equivalent is `IExtractedPackage`
(`IExtractedPackage.h`). **Diff the two interfaces first** and implement whichever methods the
Sky interface demands. Likely mapping:

| `IMountedPackage` | `IExtractedPackage` |
|---|---|
| `mountPoint()` / `path()` | `path()` / `extractedPath()` |
| `volumeName()` | may not exist → drop |
| `metaData()` | ✅ same |
| `appId()`, `parentAppId()` | ✅ same |
| `format()`, `cacheLocation()` | add if present, else drop |
| `addEventListener` / `removeEventListener` / `corruptionDetected` | `AICommon::EventListenerSet` is available via `eventloop/` |
| `updateLastUsed()` / `lastUsed()` | ✅ same pattern as `ExtractedPackage` |

Keep the destructor semantics verbatim — **explicit `mMount.unmount()` before
`std::filesystem::remove(mountPoint)`**, so the directory can actually be removed.

---

## 6. `RalfPackageMounter` (new)

This is the one genuinely new class. It blends two proven implementations:

* Recursion / refcounting / rollback / JSON emission ← `libpackage::RalfPackageImpl::lockPackage`,
  `unlockPackage`, `dumpPackageInfo`, `serializeToJson` (`ref/libpackage/src/RalfPackageHandler.cpp:734-1043`)
* Mount flags / verification bundle ← appserviced `MountedPackagesManager::mountWithLibRalf`

```cpp
namespace packagemanager {

class RalfPackageMounter
{
public:
    RalfPackageMounter(const std::shared_ptr<const IXmlDSigVerifierConfig> &verifierConfig,
                       std::filesystem::path mountRoot,          // e.g. "/tmp/mounts/"
                       std::filesystem::path packagesRoot);      // persistent store dir
    ~RalfPackageMounter();

    struct MountInfo { std::string pkgMountPath; std::string pkgMetaDataPath; };

    /// Recursively verifies, mounts and refcounts \a packagePath and its dependencies.
    /// On any failure everything locked by THIS call is rolled back and false is returned.
    bool lock(const std::string &packageId, const std::string &version,
              std::vector<MountInfo> &mountInfo);

    bool unlock(const std::string &packageId, const std::string &version);

    /// Writes the {"packages":[{pkgMountPath,pkgMetaDataPath},...]} document.
    bool serializeToJson(const std::vector<MountInfo> &mountInfo,
                         const std::filesystem::path &outputPath) const;

private:
    struct Entry {
        int mountCount = 1;
        std::string pkgJsonPath;
        std::vector<std::pair<std::string,std::string>> dependencies;   // direct deps only
        std::unique_ptr<LIBRALF_NS::PackageMount> mount;
    };

    bool lockRecursive(const LIBRALF_NS::Package &pkg, std::vector<MountInfo> &out);
    bool unlockRecursive(const std::pair<std::string,std::string> &key);
    bool identifyDependencyVersion(const std::string &depId,
                                   const LIBRALF_NS::VersionConstraint &constraint,
                                   std::string &resolvedVersion) const;
    bool emitPackageConfig(const LIBRALF_NS::Package &pkg, const std::filesystem::path &configPath) const;
    static bool isPathSafePackageKey(const std::string &id, const std::string &version);

    std::map<std::pair<std::string,std::string>, std::unique_ptr<Entry>> mMounted;
    LIBRALF_NS::VerificationBundle mVerificationBundle;
    std::filesystem::path mMountRoot;
    std::filesystem::path mPackagesRoot;
    mutable std::mutex mLock;
};

} // namespace packagemanager
```

### Rules to replicate exactly

1. **Post-order push.** Push dependency `MountInfo`s **before** the app's own, exactly as
   `RalfPackageImpl::lockPackage` does. `RalfPackageBuilder::generateOCIRootfsPackage`
   reverse-iterates to build the overlayfs `lowerdir` chain — wrong order = wrong layer
   precedence = subtly broken app.
2. **Refcount, don't re-mount.** If the key is already in `mMounted`, `incMountCount()` and
   return the existing `pkgMountPath`/`pkgJsonPath`.
3. **Rollback on failure.** Unlock every dependency locked by the current call before
   returning `false`. Never leave a partial mount tree.
4. **Verify then mount.** `package.verify()` before `package.mount(...)`; on verify failure
   roll back and return false — **before** any `ralfPkgPath` is produced (AC7).
5. **Mount flags** (appserviced): `ReadOnly | DirectIO | NoSuid`.
6. **Path-safe keys.** Reject any `packageId`/`version` containing `/`, `..`, or control
   characters before building filesystem paths (port `isPathSafePackageKey` from
   `RalfPackageHandler.cpp`). Guards against path traversal via caller-supplied ids.
7. **Unmount cleanup.** On refcount 0: `unmount()`, `remove_all(<mountRoot>/<id>_<ver>)`,
   erase from map.

### `emitPackageConfig` — the AC3 core (and Open Question #1)

```
1. If <mountDir>/config.json already exists → reuse (package was mounted before).
2. Else if pkg.auxMetaDataFile("application/vnd.rdk.package.config.v1+json") exists
     → readAll() and write it out verbatim.                    [same as libpackage]
3. Else → SYNTHESIZE from the converted PackageMetadata:
       {
         "packageType": "application" | "runtime",
         "configuration": {
           "entryPoint": <metaData.path / execFilePath>,
           "entryArgs":  <metaData.execArgs>,
           "urn:rdk:config:env":     { ...metaData.environment, ...systemEnvVars },
           "urn:rdk:config:memory":  { "systemMemory": "<metaData.sysMemoryLimit>" },
           "urn:rdk:config:storage": { "maxLocalStorage": "<dataImageSize>" },
           "urn:rdk:permission:internet": <capabilities.test(WanLanAccess)>,
           "urn:rdk:permission:thunder":  <thunder>,
           "urn:rdk:permission:firebolt": <firebolt permission from RALF metadata>
         }
       }
4. If neither is possible → return false → Lock fails with FAILED, ralfPkgPath stays empty.
```

Key names come from `RuntimeManager/ralf/OCISpecConstants.h` — do not invent new ones.
Step 3 exists solely to de-risk Open Question #1; if Sky `.ralf` packages are confirmed to
always carry the aux metadata file, keep step 3 behind a log warning but still implement it
as a safety net.

---

## 7. `PackageExtractor` as dispatcher

Mirror appserviced `PackageExtractor.cpp:41-135`. Keep the existing zip implementation
**byte-for-byte**; only add selection around it.

```cpp
// PackageExtractor.h
private:
    std::unique_ptr<IPackageExtractor> mRalfExtractor;   // null unless RALF support built & enabled
    bool mUseRalfUtilsForWidgets  = false;
    bool mUseRalfUtilsForOCIPackages = false;

    const IPackageExtractor *selectExtractor(int widgetFd, bool *isRalf) const;
```

```cpp
// PackageExtractor.cpp
const IPackageExtractor *PackageExtractor::selectExtractor(int widgetFd, bool *isRalf) const
{
    bool isWidget = true;
#if defined(ENABLE_RALF_PACKAGE_SUPPORT) && defined(LIBRALF_VERSION)
#  if LIBRALF_VERSION >= LIBRALF_VERSION_CHECK(1, 0, 2)
    isWidget = (LIBRALF_NS::Package::detectPackageFormat(widgetFd) == LIBRALF_NS::Package::Format::Widget);
#  endif
#endif
    if (isRalf) *isRalf = !isWidget;

    if (isWidget)
        return (mUseRalfUtilsForWidgets && mRalfExtractor) ? mRalfExtractor.get() : this;
    if (mUseRalfUtilsForOCIPackages && mRalfExtractor)
        return mRalfExtractor.get();

    AI_LOG_ERROR("RALF package presented but RALF support is disabled");
    return nullptr;   // -> extractAndVerify returns nullptr -> Install fails cleanly (AC7)
}
```

> ⚠️ Returning `this` for widgets means `PackageExtractor` must **not** re-enter
> `selectExtractor` from its own zip implementation. Either split the zip logic into a private
> `extractAndVerifyZip(...)` called by the public entry point, or extract it into a separate
> `ZipPackageExtractor` class. The private-method split is the smaller diff; the separate
> class is cleaner. **Recommended: private `…Zip()` methods, so the existing code moves
> without edits.**

`lseek(widgetFd, 0, SEEK_SET)` **before and after** `detectPackageFormat()` — it consumes
bytes from the fd.

With `ENABLE_RALF_PACKAGE_SUPPORT=OFF`, `mRalfExtractor` is never constructed, the
`detectPackageFormat` call is preprocessed away, `isWidget` stays `true`, and `selectExtractor`
returns `this` unconditionally → **byte-identical behaviour to today** (AC6).

---

## 8. Format-aware `PackageDatabase` / `Package`

1. Persist `PackageFormat` per installed package (a new column/field in the database record,
   or derive it from the stored file name — `package.ralf` vs `package.wgt`). Deriving from the
   filename is the smaller change and matches `libpackage`'s `RalfPackage` constant.
2. `PackageDatabase::loadPackage(appId)` must return a `RalfMountedPackage` (mounting via
   `RalfPackageMounter`) for RALF entries, and the existing `ExtractedPackage` for widgets.
3. Add `mRalfSupportEnabled` to the ctor and to the `operator<<` dump, mirroring appserviced
   `PackageDatabase.cpp:146,2603`.
4. `Package::formatToString()` → add `case PackageFormat::RALF: return "RALF";`
   (appserviced `Package.cpp:898`).
5. `getPackageMd5()`, `privateDataImageSize()`, `preLaunchCheck()`, `consoleLogPath()`,
   `userId()`, `groupId()` must keep working for RALF entries — verify each, since
   `PackageImpl::Lock` and `GetPackageMetaData` call all of them unconditionally.

---

## 9. `PackageImpl` changes

### 9.1 Constructor
Construct `mRalfPackageMounter` when `config->getRalfPackagesEnabled()`.
Pass `getRalfPackagesEnabled()` / `useRalfUtilsForWidgets()` into `PackageExtractor`.

### 9.2 `Install()`
Only two edits:
```cpp
packagemanager::PackageFormat format = packagemanager::PackageFormat::ZIP;
auto metaData = mPackageExtractor->extractAndVerify(filefd, -1, 0, nullptr, &format);
...
bool added = mPackageDatabase->addPackageAt(metaData->appId, metaData, fd, fileName, format);
```
Everything else — id/version match check, `VERSION_MISMATCH`, `runtime.capabilities`,
`epgAppId`/`extraMounts`, `md5Hash`, `dataImageSize` — stays **exactly** as-is and applies to
both formats. This is AC2 with essentially no new installation mechanism.

### 9.3 `Lock()` — the one real branch

```cpp
Result PackageImpl::Lock(const string &packageId, const string &version, string &unpackedPath,
                         ConfigMetaData &configMetadata, NameValues &additionalLocks)
{
    ... existing loadPackage / getPackageMetaData / GetPackageMetaData(...) ...
    ... existing runtime resolution (findRuntime / additionalLocks) ...
    package->preLaunchCheck();
    unpackedPath = package->privateDataImagePath();

    if (package->format() == PackageFormat::RALF && mRalfPackageMounter)
    {
        std::vector<RalfPackageMounter::MountInfo> mountInfo;
        if (!mRalfPackageMounter->lock(packageId, version, mountInfo)) {
            AI_LOG_ERROR("Lock '%s': RALF mount failed", packageId.c_str());
            return FAILED;                                  // ralfPkgPath NOT set  (AC7)
        }
        const auto metaPath = std::filesystem::temp_directory_path()
                            / (packageId + "_" + version + "_metadata.json");
        if (!mRalfPackageMounter->serializeToJson(mountInfo, metaPath)) {
            mRalfPackageMounter->unlock(packageId, version);
            return FAILED;                                  // ralfPkgPath NOT set  (AC7)
        }
        configMetadata.ralfPkgPath = metaPath.string();
        configMetadata.packageFormat = "ralf";
        // NOTE: deliberately no getDobbySpec()/"spec=" capability — the OCI bundle
        // produced by RalfOCIConfigGenerator replaces the Dobby spec entirely.
        for (auto &var : systemEnvVars()) configMetadata.envVars.push_back(std::move(var));
        return SUCCESS;
    }

    ... existing ISystemConfigProvider::getDobbySpec() + "spec=" capability block, verbatim ...
    return result;
}
```

The `ralfPkgPath`/`spec=` split is mutually exclusive by construction, which is exactly the
invariant `RuntimeManagerImplementation::generate()` will key off.

### 9.4 `Unlock()`
Currently a no-op returning `SUCCESS`. Add:
```cpp
if (mRalfPackageMounter) {
    mRalfPackageMounter->unlock(packageId, version);        // no-op if not mounted
    // remove /tmp/<id>_<ver>_metadata.json (path-safe key check first)
}
return SUCCESS;
```
Keep returning `SUCCESS` for widgets so behaviour is unchanged.

### 9.5 `GetFileMetadata()`
Add the `packageFormat` out-param plumb-through and set
`configMetadata.packageFormat = (format == RALF) ? "ralf" : "wgt"`. No other change —
the existing `sysMemoryLimit` / `gpuMemoryLimit` / `runtime.capabilities` /
`APPLICATION_IS_SYSTEM_APP` handling already works off `PackageMetadata`, which the converter
populates for RALF packages too.

### 9.6 `Initialize()` / `GetConfigListForInstalledPackages()`
**No changes.** Both operate purely on `PackageDatabase` + `PackageMetadata`, so RALF packages
are picked up automatically — including DIAL registration, because
`RalfMetadataConverter::convertDialInfo` populates `dialIds` and the DIAL capability bits.

---

## 10. CMake

```cmake
# ref/libpackage-sky-ralf/CMakeLists.txt

option(ENABLE_RALF_PACKAGE_SUPPORT "Enable RALF/Bolt package support via ralf-utils" OFF)

set(PACKAGE_SOURCES
    PackageImpl.cpp DataImageManager.cpp Logging.cpp ... )   # unchanged list

if(ENABLE_RALF_PACKAGE_SUPPORT)
    find_package(libralf REQUIRED)
    message(STATUS "RALF/Bolt package support ENABLED")
    add_definitions(-DENABLE_RALF_PACKAGE_SUPPORT)
    list(APPEND PACKAGE_SOURCES
        RalfPackageExtractor.cpp
        RalfMetadataConverter.cpp
        RalfMountedPackage.cpp
        RalfPackageMounter.cpp)
endif()

add_library(Package SHARED ${PACKAGE_SOURCES} $<TARGET_OBJECTS:...>)

if(ENABLE_RALF_PACKAGE_SUPPORT)
    target_link_libraries(Package ralf)
    target_include_directories(Package PRIVATE
        $<TARGET_PROPERTY:ralf-utils::libralf,INTERFACE_INCLUDE_DIRECTORIES>)
endif()
```

The `$<TARGET_PROPERTY:ralf-utils::libralf,INTERFACE_INCLUDE_DIRECTORIES>` form is lifted from
appserviced `RDK/AppManager/PackageManager/CMakeLists.txt:95` and is what makes
`#include <ralf/Package.h>` resolve.

**AC6 guarantee:** with the option OFF, neither `find_package(libralf)` nor any `Ralf*.cpp`
nor any `-D` is emitted; the resulting `libPackage.so` has no `libralf` `DT_NEEDED` entry.
Verify in CI with `readelf -d libPackage.so | grep -i ralf` (must be empty).

---

## 11. Tests (`ref/libpackage-sky-ralf/tests/`)

The directory currently holds a single file. Establish a proper unit-test target and port the
appserviced fixtures.

| Test | Ported from | Asserts |
|---|---|---|
| `testRalfPackageExtractor.cpp` | `/tmp/appsvc-ralf/.../test/testRalfPackageExtractor.cpp` | open/verify/extract, size & file-count limits, corrupt package → nullptr |
| `testRalfMetadataConverter.cpp` | `/tmp/appsvc-ralf/.../test/testLibRalfMetaData.cpp` | every `convert*` mapping into `PackageMetadata` |
| `testPackageExtractorDispatch.cpp` | new | `detectPackageFormat` routing for `.wgt` vs `.ralf`; RALF fd with support OFF → nullptr |
| `testRalfPackageMounter.cpp` | new | refcounting, recursive dep lock, **post-order MountInfo ordering**, rollback on verify failure, unmount+rmdir on last unlock, path-traversal rejection |
| `testPackageImplRalfLock.cpp` | new | RALF lock sets `ralfPkgPath` and **no** `spec=`; widget lock sets `spec=` and **empty** `ralfPkgPath`; failure paths leave `ralfPkgPath` empty |

Test fixtures available for copy:
```
/tmp/appsvc-ralf/RDK/AppManager/PackageManager/test/widgets/all-configs.ralf
/tmp/appsvc-ralf/RDK/AppManager/PackageManager/test/widgets/simple.ralf.tar
/tmp/appsvc-ralf/RDK/AppManager/PackageManager/test/widgets/simple-firstparty.ralf.tar
/tmp/appsvc-ralf/RDK/AppManager/SecureDebugManager/test/data/test_0{1,2}_package.ralf
```

---

## 12. Dispose of the dead `RALF/` directory

`ref/libpackage-sky-ralf/RALF/{CMakeLists.txt,PackageImpl.cpp,PackageImpl.h}` is unreferenced,
targets `/media/apps/sky/packages` + `package.wgt` + `/etc/sky/certs/production.crt`, and does
not satisfy the current `IPackageImpl` contract.

**Action:** delete it in the same PR (git history preserves it), or move it to
`RALF/README.md` + `attic/` with a note pointing at this plan. Leaving it in place guarantees
someone will eventually build it by mistake.

Before deleting, harvest its `populateConfigMetaData()` for the permission→capability mapping
(`FIREBOLT_PERMISSION`, thunder / internet / homeApp flags) and cross-check it against
`RalfMetadataConverter::convertPermissions` and `libpackage`'s
`addPackagePermissionsToConfigMetadata`.
