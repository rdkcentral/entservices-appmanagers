# 07 – Troubleshooting and Log Markers

## 1. Log markers by stage

| Stage | Component | Log line (substring) | Meaning |
|-------|-----------|----------------------|---------|
| boot | libpackage-sky | `RALF/Bolt package support enabled` | handler constructed (`ENABLE_RALF_PACKAGE_SUPPORT` build) |
| boot | libpackage-sky | `no usable signing certificates found - RALF packages will fail verification` | empty CA bundle |
| boot | sysconfig (syslog) | `ralfContainerEnvVars: %zu var(s) for '%s'` | env produced for a RALF-layered container |
| boot | PackageDatabase | `preserving RALF package '...' for RALF backend` | ZIP scanner skipped a RALF file |
| boot | libpackage-sky | `skipping unreadable RALF package` | open/verify failed at initialize |
| install | libpackage-sky | `Install: '...' is a RALF/Bolt package` | content detection chose RALF |
| install | libpackage-sky | `installed RALF package '%s' version '%s' at '%s'` | success |
| install | libpackage-sky | `verification failed for` / `staged package verification failed` | signature/expiry/digest failure |
| lock | libpackage-sky | `mounted '%s' at '%s' (volume '%s')` | libralf EROFS mount succeeded |
| lock | libpackage-sky | `mounting '%s' failed, trying verified extraction fallback` | mount failed → `verify()` + `extractTo()` |
| lock | libpackage-sky | `detaching leftover package mount at` | stale mount from a previous process |
| lock | libpackage-sky | `no installed version of '%s' satisfies the constraint required by '%s'` | dependency resolution failure |
| lock | libpackage-sky | `Lock '%s': ralfPkgPath='%s' layers=%zu caps='%s'` | RALF app locked; manifest path |
| lock | libpackage-sky | `Locked RALF runtime '%s' version '%s': runtimePath='%s' command='%s'` | **Mode 3** runtime locked (no base) |
| lock | libpackage-sky | `Locked RALF runtime '%s' version '%s' for widget '%s' as OCI bundle: ralfPkgPath='%s' layers=%zu` | **Mode 3b** (runtime with base) |
| lock | libpackage-sky | `failed to compose OCI layers for widget '%s' on RALF runtime '%s'` | Mode 3b widget layer/manifest failure |
| lock | libpackage-sky | `Lock '%s': failed to lock RALF runtime '%s'` | Mode 3 failure → Lock FAILED |
| lock | libpackage-sky | `can't find runtimePackage for '%s'` | ZIP runtime load failed (or pre-fix RALF runtime) |
| lock | libpackage-sky | `Lock '%s': runtimeExec='%s' runtimePath='%s'` | widget lock summary – empty values mean no runtime |
| run | RuntimeManager | `RALF launch support enabled/disabled by runtime configuration` | `ralfEnabled` |
| run | RuntimeManager | `[ralf-debug] Run selected RALF/legacy container flow` | dispatch result |
| run | RuntimeManager | `Base-free RALF launch: launching %s from %s using classic Dobby spec` | **Mode 4** |
| run | RuntimeManager | `RALF widget private image not found at` | Mode 4 without `data.img` |
| run | RuntimeManager | `Generating Ralf Package Config` | **Mode 5** |
| run | RuntimeManager | `Failed to create shared vendor GPU layer` | EntOS vendor layer failed (retried on next launch) |
| run | RuntimeManager | `RALF launch has no runtime or application entryPoint after the base init` | OCI bundle without any executable |
| run | RuntimeManager | `RALF launch requires a base package with an init entryPoint` | EntOS Mode 5 without base init |
| run | RuntimeManager | `Starting  container in RALF mode` | `StartContainer(bundle)` |
| run | RuntimeManager | `RALF launch disabled by RuntimeManager configuration` / `RALF launch requested without RALF support` | **Mode 6** |
| run | RuntimeManager | `Invalid classic RALF executable path` | entryPoint failed traversal checks |

## 2. On-device verification

```sh
# Which package format is installed? (RALF archives can also be ZIP containers –
# the distinguishing content is the OCI layout, not the magic bytes)
for d in /media/apps/sky/packages/*/; do
  f="$d/package.wgt"; [ -f "$f" ] || continue
  if unzip -l "$f" 2>/dev/null | grep -q ' oci-layout$'; then echo "RALF  $d";
  elif unzip -l "$f" 2>/dev/null | grep -q ' config.xml$'; then echo "ZIP   $d";
  else echo "other $d"; fi
done

# libralf mounts
grep -E 'erofs' /proc/self/mountinfo
ls /var/run/sky/mounts/

# Mount manifest for a locked RALF app
cat /tmp/<appId>_<version>_metadata.json

# Per-layer normalised config
cat /var/run/sky/mounts/<id>_<ver>/config.json

# Mode 3 – runtime mount + bind into container
journalctl | grep -E 'Locked RALF runtime|failed to lock RALF runtime'
DobbyTool info <containerId> | grep -A3 '"/runtime"'

# Mode 5 – overlay bundle (EntOS)
ls /run/sky/bundles/<appInstanceId>/
grep overlay /proc/self/mountinfo | grep bundles

# Backing devices of an EROFS mount (read-only inspection)
readlink -f /sys/dev/block/<maj>:<min>
cat /sys/dev/block/<maj>:<min>/dm/name 2>/dev/null
```

## 3. Known failure → cause → fix

| Symptom | Cause | Fix |
|---------|-------|-----|
| `package widget is missing files (signature1.xml no / config.xml no)` for a runtime | RALF runtime loaded via ZIP extractor (pre-`lockRuntime`) | libpackage-sky ≥ `01d0ec6` |
| Widget launches with `runtimeExec=''` and fails later | runtime load error swallowed (pre-fix) | now Lock returns `FAILED` for RALF runtimes |
| RALF file disappears after reboot | ZIP `populateFromDir` deleted it as corrupt | build with `ENABLE_RALF_PACKAGE_SUPPORT` |
| `ENTOS_RALF_SUPPORT requires RALF_PACKAGE_SUPPORT` at configure | F1 in [06](06_EntOS_Ifdefs_and_NonEntOS_Protection.md) | pass `-DENTOS_RALF_SUPPORT=OFF` for non-RALF builds |
| `ERROR_RALF_DISABLED` | `ralfEnabled=false` or non-RALF RuntimeManager | enable / rebuild |
| EntOS OCI bundle fails with missing base init | app/runtime chain has no `base` with `entryPoint` | add base dependency or make it base-free (Mode 4) |
| Container cannot read `/runtime` files (Mode 3) | EROFS image file modes not readable by widget uid | fix image permissions (world-readable) |
| Upgrade does not take effect | old version still mounted (other locks) | release all locks; next lock mounts new inode |
| Missing REGION/PARTNER_ID/APPLICATION_SECRET env in RALF container | sysconfig `.so` not loaded, or provider without the context override | check `ralfContainerEnvVars` syslog line; load `rdkappmanagers-sysconfig` |
| GPU library missing in OCI container (e.g. `libwayland-egl.so.1.20.0`) | `vendor-gpu.json` binds a source that the image does not ship (no automatic fallback any more) | bind the soname the image ships and add `symlink` entries for versioned names in `vendor-gpu.json` |

## 4. Known limits / parity gaps

- Mode 3b: widget capabilities are mapped to RALF permissions/memory only; DIAL/MAPI/TSB/
  multicast spec features of the classic widget spec are not reproduced in the OCI bundle.
- No mount cache after the last unlock – every launch re-creates the dm-verity/EROFS mount.
- `.packages.erofsWidgets.loadMode=extract` (appserviced) has no equivalent; mount is always tried first.
- `PackageMetadata` / capability parity TODOs (`TODO(RALF-AI1-PARITY)`): hole-punch protocols,
  local socket ports, start timeout, game mode, age policy, marketplace intercept, picture mode,
  post-intents/TSB/external storage/compositor/chromecast capability bits.
