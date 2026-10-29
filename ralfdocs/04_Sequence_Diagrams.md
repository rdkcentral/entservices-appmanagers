# 04 – Sequence Diagrams

All diagrams reflect the code on `topic/RDKEMW-25785` (entservices-appmanagers) and
`topic/entos-ralf-support` (libpackage-sky). Participants:

| Alias | Component |
|-------|-----------|
| AM | AppManager (`AppManagerImplementation`) |
| LCM | LifecycleManager |
| PM | PackageManager (`PackageManagerImplementation`) |
| PI | libpackage-sky `PackageImpl` |
| PDB | libpackage-sky `PackageDatabase` |
| RH | libpackage-sky `RalfPackageHandler` |
| LR | libralf (`ralf::Package`, `ralf::PackageMount`) |
| SCP | `ISystemConfigProvider` (rdkappmanagers-sysconfig) |
| RM | RuntimeManager (`RuntimeManagerImplementation`) |
| RPB | `ralf::RalfPackageBuilder` / `RalfOCIConfigGenerator` |
| DSG | RuntimeManager `DobbySpecGenerator` |
| ASM | AppStorageManager |
| OCI | OCIContainer plugin → Dobby |

---

## 1. Boot / cache initialisation

```mermaid
sequenceDiagram
    autonumber
    participant PM
    participant PI
    participant PDB
    participant RH
    participant LR

    PM->>PI: IPackageImpl::instance()
    PI->>PDB: new PackageDatabase(...)
    activate PDB
    PDB->>PDB: populateFromDir(packagesDir)
    loop each {appId}/package.wgt
        PDB->>LR: RalfPackageHandler::isRalfPackageFile() → detectPackageFormat
        alt Format::Ralf
            PDB->>PDB: keep file (toDelete.erase), skip ZIP parsing
        else ZIP widget
            PDB->>PDB: legacy metadata parse / corrupt-cleanup
        end
    end
    deactivate PDB
    PI->>RH: new RalfPackageHandler(cfg, installRoot, mountRoot, extractRoot, ...)
    RH->>RH: buildVerificationBundle(CA files)
    RH->>RH: getpwnam("ralf") → mUserId/mGroupId
    PM->>PI: Initialize(configStr, aConfigMetadata)
    PI->>PDB: getPackages / loadPackage / getPackageMetaData (ZIP)
    PI->>PDB: purgeCaches(ExtractedPackageCache)
    PI->>RH: initialize(aConfigMetadata)
    loop each install-root dir with RALF package.wgt
        RH->>LR: Package::open(path, bundle, None)
        RH->>RH: registerPackage(): build PackageMetadata
        RH->>PDB: registerVerifiedPackage(appId, metadata)
        RH-->>PI: aConfigMetadata[{id, ver}] += ConfigMetaData
    end
    PI-->>PM: aConfigMetadata
    PM->>PM: populateRuntime() + mState[{id,ver}] for ZIP and RALF
```

---

## 2. Install – RALF package

```mermaid
sequenceDiagram
    autonumber
    participant Client as DownloadManager / installer
    participant PM
    participant PI
    participant RH
    participant LR
    participant PDB

    Client->>PM: Install(packageId, version, metadata, fileLocator)
    PM->>PI: Install(...)
    PI->>LR: isRalfPackageFile(fileLocator)
    LR-->>PI: Format::Ralf
    PI->>RH: install(id, ver, fileLocator, cfg)
    RH->>LR: Package::open(src, bundle, CheckCertificateExpiry)
    RH->>LR: package.verify()  (all blob digests)
    alt verification fails
        RH-->>PI: VERIFICATION_FAILURE
    end
    RH->>RH: id/version == requested ? else VERSION_MISMATCH
    RH->>RH: remove stale package.wgt.tmp, copy src → .tmp
    RH->>LR: open(.tmp, Install) + verify()   (TOCTOU guard)
    RH->>RH: fsync(.tmp)
    RH->>RH: rename(.tmp → package.wgt), fsync(dir)
    RH->>RH: registerPackage()
    RH->>PDB: registerVerifiedPackage() → emit PackageInstalled/Upgraded
    RH-->>PI: SUCCESS (cfg.appPath = dest)
    PI-->>PM: SUCCESS
    PM->>PM: mState update, notify listeners
```

## 3. Install – ZIP widget (hardened, shared slot)

```mermaid
sequenceDiagram
    autonumber
    participant PI
    participant PDB
    participant PX as PackageExtractor

    PI->>PDB: addPackage(appId, metaData, dirFd, file)
    PDB->>PDB: mkstemp({id}/package.wgt.zip.XXXXXX)
    PDB->>PDB: copyFileAt(stage ← file)
    PDB->>PX: extractAndVerify(stagedFd, -1, 0)
    PX-->>PDB: verified metadata
    PDB->>PDB: id/version match? fchmod 0644, fsync
    PDB->>PDB: rename(stage → package.wgt)  (no unlink first)
    PDB->>PDB: fsync({id} dir)
```

---

## 4. End-to-end launch – RALF app with base layer (OCI bundle mode)

```mermaid
sequenceDiagram
    autonumber
    participant AM
    participant PM
    participant PI
    participant RH
    participant LR
    participant SCP
    participant LCM
    participant RM
    participant ASM
    participant RPB
    participant OCI

    AM->>PM: Lock(appId, ver, reason)
    PM->>PI: Lock(appId, ver, unpackedPath, cfg, locks)
    PI->>RH: ralfHandlerFor(appId, ver) → packageFormat=="ralf"
    PI->>RH: lock(...)
    RH->>LR: Package::open({root}/{app}/package.wgt, None)
    RH->>RH: lockRecursive(app)
    Note over RH: post-order over dependencies
    RH->>RH: resolveDependencyVersion(base) → RALF
    RH->>RH: lockRecursive(base)
    RH->>LR: base.isMountable() / base.mount({mnt}/base_v/rootfs, RO+DirectIO+NoSuid)
    RH->>RH: emitPackageConfig(base) → packageType=base
    RH->>RH: resolveDependencyVersion(runtime) → RALF
    RH->>RH: lockRecursive(runtime) → mount + config
    RH->>LR: app.mount({mnt}/app_v/rootfs, ...)
    RH->>RH: emitPackageConfig(app)
    RH->>RH: populateConfigMetaData(app) (caps, env), setRunAsIds (app uid/gid)
    RH->>RH: env = deps env + app env
    RH->>RH: serializeMountInfo → $TMPDIR/app_v_metadata.json [base, runtime, app]
    RH-->>PI: SUCCESS, cfg.ralfPkgPath, appPath, unpackedPath={root}/{app}
    PI->>SCP: getRalfContainerEnvVars(context: appId, caps, appSecret, watchdog)
    SCP-->>PI: platform + secret + watchdog + launch method env (prepended)
    PI-->>PM: SUCCESS
    PM->>PM: getRuntimeConfig(cfg) → payload {ralfPkgPath, envVariables, ...}
    PM-->>AM: lockId, unpackedPath, runtimeConfigPayload
    AM->>LCM: SpawnApp(..., payload)
    LCM->>RM: Run(appId, instanceId, uid, gid, ..., payload)
    RM->>RM: Decode, check ralfEnabled, isRalfPackage → true
    RM->>RM: getClassicRalfLaunchInfo → false (3 layers / base present)
    RM->>ASM: GetStorage(appId, app uid/gid)
    RM->>RM: Rialto session (socket rlto-{instance}) [ENABLE_RIALTO]
    RM->>RPB: generateRalfDobbySpec(config, runtimeCfg)
    RPB->>RPB: parseRalPkgInfo(manifest)
    RPB->>RPB: [EntOS] RalfVendorLayer::createShared → {bundleRoot}/vendor (once per process)
    RPB->>RPB: lowerdir = app:runtime:vendor:base  (EntOS)
    RPB->>RPB: generateOCIRootfs → mount overlay {bundle}/rootfs
    RPB->>RPB: RalfOCIConfigGenerator: base spec + graphics + per-layer config<br/>+ permissions + secrets + env + [EntOS] legacy args + hooks + app cfg
    RPB->>RPB: ensureMergedRootfsMountTargets
    RPB-->>RM: bundle path (/run/sky/bundles/{instance})
    RM->>RM: runtimeAppInfo.ralfMode = true
    RM->>OCI: StartContainer(containerId, bundlePath, "", "")
    OCI-->>RM: success / descriptor
```

## 5. Launch – base-free RALF app (classic Dobby spec mode)

```mermaid
sequenceDiagram
    autonumber
    participant PM
    participant PI
    participant RH
    participant RM
    participant DSG
    participant OCI

    PM->>PI: Lock(app)
    PI->>RH: lock(app)
    RH->>RH: lockRecursive(app): deps = [runtime] (RALF or ZIP via lockWidget)
    RH-->>PI: ralfPkgPath manifest = [runtime, app]
    PI-->>PM: SUCCESS
    PM-->>RM: Run(..., payload{ralfPkgPath})
    RM->>RM: isRalfPackage → true
    RM->>RM: getClassicRalfLaunchInfo(manifest)
    Note over RM: 2 entries, front=runtime, back=application,<br/>relative safe entryPoint → true
    RM->>RM: launchConfig.appPath = app mount<br/>runtimePath = runtime mount (if runtime has entryPoint)<br/>command = runtime/app entryPoint
    RM->>RM: unpackedPath = {root}/{app}/data.img if exists, else empty
    RM->>RM: uid/gid = app's own uid from payload
    RM->>DSG: generate(config, launchConfig)
    DSG->>DSG: /package ← appPath (ro,bind,nosuid,nodev)
    DSG->>DSG: /runtime ← runtimePath (ro,bind,nosuid,nodev)
    DSG->>DSG: private data loop mount only if unpackedPath
    DSG->>DSG: rdkPlugins.networking + appservicesrdk (ralfPkgPath set)
    DSG-->>RM: Dobby spec JSON
    RM->>OCI: StartContainerFromDobbySpec(id, spec, "", westerosSocket)
```

## 6. Launch – ZIP widget app with RALF runtime (YouTube + com.sky.cobalt)

```mermaid
sequenceDiagram
    autonumber
    participant AM
    participant PM
    participant PI
    participant PDB
    participant RH
    participant LR
    participant SCP
    participant RM
    participant DSG
    participant OCI

    AM->>PM: Lock(YouTube, 100.2.30+rialto)
    PM->>PI: Lock(...)
    PI->>RH: ralfHandlerFor(YouTube) → nullptr (ZIP)
    PI->>PDB: loadPackage(YouTube) (ZIP extract + verify)
    PI->>PDB: getPackageMetaData / GetPackageMetaData → caps
    PI->>PDB: findRuntime("application/cobalt") → com.sky.cobalt
    PI->>RH: isRalfPackageInstalled(com.sky.cobalt) → true
    PI->>RH: lockRuntime(com.sky.cobalt, widget, caps)
    RH->>LR: Package::open({root}/com.sky.cobalt/package.wgt, None)
    RH->>RH: lockRecursive(cobalt) → 1 layer (no base)
    RH->>LR: isMountable() → mount({mnt}/com.sky.cobalt_{v}/rootfs, RO+DirectIO+NoSuid)
    RH->>RH: emitPackageConfig → config.json {packageType: runtime, entryPoint}
    RH->>RH: runtimePath = rootfs, command = entryPoint.relative_path()
    RH-->>PI: SUCCESS, version
    PI->>PI: cfg.command / runtimePath set, additionalLocks += cobalt
    PI->>SCP: getDobbySpec(package, caps, command, runtimePath)
    SCP-->>PI: spec (escaped into "spec=" capability)
    PI->>PI: mLockedWidgets[YouTube] += {pinned, ralfRuntime=cobalt}
    PI-->>PM: SUCCESS
    PM-->>RM: Run(..., payload{runtimePath, command, ralfPkgPath=""})
    RM->>RM: isRalfPackage → false (widget flow, per-app uid)
    RM->>DSG: generate()
    DSG->>DSG: /runtime ← runtimePath (EROFS mount) ro,nosuid,nodev
    DSG->>DSG: args = /runtime/{command}
    RM->>OCI: StartContainerFromDobbySpec(...)
```

### 6b. ZIP widget app with a RALF runtime that depends on a base (Mode 3b, OCI bundle)

```mermaid
sequenceDiagram
    autonumber
    participant PM
    participant PI
    participant RH
    participant LR
    participant SCP
    participant RM
    participant RPB
    participant OCI

    PM->>PI: Lock(YouTube)
    PI->>PI: loadPackage(YouTube) ZIP extract, caps, per-app uid
    PI->>RH: lockRuntime(com.sky.cobalt, widget, caps)
    RH->>RH: lockRecursive(cobalt) → base mounted, cobalt mounted (2 layers)
    RH->>RH: widget layer = extractedPath + {mnt}/YouTube_{v}_app/config.json
    Note over RH: config.json: packageType application, entryPoint execFilePath,<br/>permissions from caps (wan-lan → internet ...), sys-memory-limit
    RH->>RH: serializeMountInfo → [base, cobalt, YouTube] = ralfPkgPath
    RH->>RH: prepend base/cobalt RALF env
    RH-->>PI: SUCCESS (command/runtimePath cleared)
    PI->>SCP: getRalfContainerEnvVars(context: YouTube, caps, secret, watchdog)
    Note over PI: getDobbySpec skipped - no classic spec for this launch
    PI-->>PM: SUCCESS (ralfPkgPath, unpackedPath = YouTube data.img)
    PM-->>RM: Run(payload)
    RM->>RM: isRalfPackage true, classic false (3 layers)
    RM->>RPB: generateRalfDobbySpec
    RPB->>RPB: lowerdir YouTube:cobalt:vendor:base
    RPB->>RPB: args = [base init, cobalt entryPoint, widget execFilePath?]
    RPB->>RPB: storage plugin loop-mounts data.img at /home/private
    RPB->>RPB: APPLICATION_TOKEN = appInstanceId
    RM->>OCI: StartContainer(id, bundle)
    Note over PM,PI: Unlock → unlockWidgetRuntime: unlock cobalt tree,<br/>remove widget layer config dir and manifest
```

Failure variant – Cobalt cannot be mounted:

```mermaid
sequenceDiagram
    participant PI
    participant RH
    participant PM
    PI->>RH: lockRuntime(cobalt)
    RH-->>PI: FAILED ("failed to mount RALF runtime" / "has no entryPoint")
    PI-->>PM: FAILED ("Lock 'YouTube': failed to lock RALF runtime 'com.sky.cobalt'")
    PM-->>PM: LOGERR Lock Failed → ERROR_GENERAL (launch aborted early)
```

---

## 7. Terminate / container stop / unlock

```mermaid
sequenceDiagram
    autonumber
    participant AM
    participant RM
    participant RPB
    participant OCI
    participant PM
    participant PI
    participant RH
    participant LR

    AM->>RM: Terminate(instanceId) (via LifecycleManager)
    RM->>OCI: StopContainer
    OCI-->>RM: onContainerStopped
    RM->>RM: ralfMode = mRuntimeAppInfo[instance].ralfMode
    RM->>RPB: cleanupRalfInstance(instance, ralfMode)
    alt ralfMode (OCI bundle)
        RPB->>RPB: unmountOverlayfsIfExists → umount overlay, rm bundle dir
    else classic / widget
        RPB-->>RM: no-op
    end
    AM->>PM: Unlock(appId, ver)
    PM->>PI: Unlock(appId, ver)
    alt ZIP app (in mLockedWidgets)
        PI->>PI: pop LockedWidget (drop pinned IPackage refs)
        opt ralfRuntime set
            PI->>RH: unlockWidgetRuntime(app, cobalt)
        end
    else RALF app
        PI->>RH: unlock(app, ver)
    end
    RH->>RH: unlockRecursive: deps first, --mountCount
    RH->>LR: PackageMount::unmount() at count 0
    RH->>RH: detachMount (MNT_DETACH) if still mounted, remove workDir
    RH->>RH: remove $TMPDIR/{id}_{ver}_metadata.json
```

## 8. Upgrade while running

```mermaid
sequenceDiagram
    participant Inst as Installer
    participant RH
    participant FS as Filesystem
    participant Kern as Kernel (loop / dm-verity)
    Note over Kern: app v1 mounted from inode A
    Inst->>RH: install(app, v2)
    RH->>FS: write package.wgt.tmp (inode B), verify, fsync
    RH->>FS: rename(tmp → package.wgt)  (path now → inode B)
    Note over Kern: running container keeps reading inode A
    Inst->>RH: (later) unlock(app, v1)
    RH->>RH: key {app,v1} not found? exactly one version mounted → release it
    RH->>Kern: unmount → inode A freed
    Note over RH: next lock mounts inode B as v2
```

## 9. Uninstall

```mermaid
sequenceDiagram
    participant PM
    participant PI
    participant RH
    participant PDB
    PM->>PI: Uninstall(appId)
    PI->>RH: ralfHandlerFor(appId)
    alt RALF
        PI->>RH: uninstall(appId)
        RH->>PDB: removePackage(appId)
    else ZIP
        PI->>PDB: removePackage(appId)
    end
```

## 10. Kill-switch / unsupported build

```mermaid
sequenceDiagram
    participant LCM
    participant RM
    LCM->>RM: Run(..., payload{ralfPkgPath non-empty})
    alt RALF build && ralfEnabled == false
        RM-->>LCM: ERROR_UNAVAILABLE + onFailure "ERROR_RALF_DISABLED"
    else non-RALF build (RALF_PACKAGE_SUPPORT=OFF)
        RM-->>LCM: ERROR_UNAVAILABLE + "ERROR_RALF_DISABLED"
    else
        RM->>RM: continue launch
    end
```
