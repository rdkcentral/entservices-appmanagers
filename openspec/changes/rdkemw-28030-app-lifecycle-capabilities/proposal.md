## Why

Resource Monitor needs each app's supported lifecycle states and Suspended-state limits before lifecycle enforcement can be built (RDKEMW-28030). RALF metadata already defines them (`urn:rdk:config:application-lifecycle`), but PackageManager does not surface them.

## What Changes

- PackageManager copies the locked app's `urn:rdk:config:application-lifecycle` object, unchanged, into the runtime configuration payload as `applicationLifecycle`. It reads the object from the RALF metadata referenced by `ralfPkgPath`. Metadata failures never fail a lock.
- ResourceMonitor gains internal utilities that resolve an `appId` to its locked package and report:
  - the supported states;
  - whether a given state is supported;
  - the Suspended configuration (`maxSuspendedSystemMemory`, `maxTimeToSuspendMemoryState`).
- Unconfigured or unavailable data is reported explicitly and never fails ResourceMonitor.
- The Suspended CPU threshold stays unavailable until its metadata key is defined.

## Capabilities

### New Capabilities
- `app-lifecycle-capabilities`: PackageManager-sourced lifecycle-state support and Suspended configuration for ResourceMonitor.

### Modified Capabilities
None.

## Impact

- **Modules:** PackageManager (`openspec/specs/PackageManager.spec.md`), ResourceMonitor and helpers.
- **APIs:** None. CI stays on `INTERFACES_REF` `a6e324d8`.
- **Build:** ResourceMonitor links `AppManagersHelpers`. New `ResourceMonitor_L0Test` target. No new dependencies or CMake options.
- **Runtime:** RALF packages only, and only while locked. Legacy packages report "unavailable".
- **Next:** RDKEMW-28031 (lifecycle enforcement) is the first production caller.

## Non-goals

- Lifecycle enforcement, timers or terminations.
- Defining the CPU-threshold metadata key.
- Changes to ralf-utils, libpackage, eshelpers or `IResourceMonitor`.
- Interpreting `startupTimeout` or `watchdogInterval`.
