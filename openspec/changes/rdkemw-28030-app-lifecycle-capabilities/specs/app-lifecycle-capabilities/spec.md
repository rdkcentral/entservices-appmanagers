## Purpose

Provide Resource Monitor with each application's supported lifecycle states and Suspended-state resource configuration, using the application package's RALF metadata as delivered by Package Manager as the only source.

## ADDED Requirements

### Requirement: Package Manager exposes package lifecycle configuration
When a RALF application package is locked, Package Manager SHALL include that package's `urn:rdk:config:application-lifecycle` metadata object in the package's runtime configuration payload under the member `applicationLifecycle`. It SHALL copy the object without changing member names, values or unknown members. Package Manager SHALL omit `applicationLifecycle` when the package metadata does not define the object. Package Manager SHALL NOT invent or default lifecycle values.

#### Scenario: Locked package defines lifecycle configuration
- **WHEN** a RALF application package whose metadata contains `"urn:rdk:config:application-lifecycle": {"supportedNonActiveStates": ["paused","suspended"], "maxSuspendedSystemMemory": "16M", "maxTimeToSuspendMemoryState": 10}` is locked
- **THEN** the runtime configuration payload returned by `Lock` and `GetLockedInfo` SHALL contain `applicationLifecycle` equal to that object

#### Scenario: Unknown lifecycle members are preserved
- **WHEN** the package's lifecycle object contains a member that Package Manager does not recognise
- **THEN** `applicationLifecycle` SHALL contain that member with its original value

#### Scenario: Package does not define lifecycle configuration
- **WHEN** a locked package's metadata has no `urn:rdk:config:application-lifecycle` object, or the package is not a RALF package
- **THEN** the runtime configuration payload SHALL NOT contain `applicationLifecycle`
- **AND** every other payload member SHALL be unchanged

#### Scenario: Package metadata cannot be read
- **WHEN** the metadata referenced by `ralfPkgPath` is missing, unreadable, malformed, or does not identify the locked package
- **THEN** Package Manager SHALL log a warning, omit `applicationLifecycle`, and complete the lock with the same result it would have returned otherwise

### Requirement: Supported lifecycle states are reported from Package Manager configuration
Resource Monitor SHALL report, for an application identifier, the set of supported lifecycle states. ACTIVE SHALL always be reported as supported. PAUSED, SUSPENDED and HIBERNATED SHALL be reported as supported only when listed, case-insensitively, in `applicationLifecycle.supportedNonActiveStates` from Package Manager. Unrecognised state names SHALL be ignored and logged. Resource Monitor SHALL NOT keep its own lifecycle configuration or defaults for these states.

#### Scenario: All lifecycle states configured
- **WHEN** Package Manager reports `supportedNonActiveStates` `["paused","suspended","hibernated"]` for the application
- **THEN** Resource Monitor SHALL report ACTIVE, PAUSED, SUSPENDED and HIBERNATED as supported and the configuration as present

#### Scenario: Subset of lifecycle states configured
- **WHEN** Package Manager reports `supportedNonActiveStates` `["paused"]`
- **THEN** Resource Monitor SHALL report ACTIVE and PAUSED as supported
- **AND** SUSPENDED and HIBERNATED SHALL be reported as unsupported

#### Scenario: Non-standard combination of states
- **WHEN** Package Manager reports a combination outside the metadata specification's valid sets, such as `["hibernated"]`
- **THEN** Resource Monitor SHALL report exactly ACTIVE plus the listed states as supported and log a warning

#### Scenario: Empty list of states
- **WHEN** Package Manager reports `supportedNonActiveStates` `[]`
- **THEN** Resource Monitor SHALL report only ACTIVE as supported and the configuration as present

### Requirement: Single lifecycle-state support query
Resource Monitor SHALL answer whether one specific lifecycle state is supported for an application, consistent with the reported set of supported states.

#### Scenario: Query a supported state
- **WHEN** support for SUSPENDED is queried for an application whose configured states include `suspended`
- **THEN** the answer SHALL be supported

#### Scenario: Query an unsupported state
- **WHEN** support for HIBERNATED is queried for an application whose configured states do not include `hibernated`
- **THEN** the answer SHALL be unsupported

### Requirement: Missing optional lifecycle configuration
Resource Monitor SHALL distinguish lifecycle data that is configured, not configured, and unavailable:
- **Not configured:** Package Manager returns a payload with a non-empty `ralfPkgPath` but without `applicationLifecycle`. This means a RALF package whose metadata does not define the object. Only ACTIVE SHALL be supported.
- **Unavailable:** the payload has no `ralfPkgPath` or an empty one, so the package metadata could not be consulted (for example a legacy, non-RALF package). Also unavailable: the application has no locked package, Package Manager is unreachable, or the payload cannot be parsed. Only ACTIVE SHALL be reported as supported, and callers SHALL be able to tell this case apart from not configured.

None of these conditions SHALL cause a Resource Monitor failure or affect its other functions.

#### Scenario: RALF package without lifecycle configuration
- **WHEN** Package Manager returns a payload with a non-empty `ralfPkgPath` and without `applicationLifecycle` for the application
- **THEN** Resource Monitor SHALL report the configuration as not configured, PAUSED, SUSPENDED and HIBERNATED as unsupported, and the Suspended configuration as unavailable

#### Scenario: Legacy package without RALF metadata
- **WHEN** Package Manager returns a payload whose `ralfPkgPath` is missing or empty
- **THEN** Resource Monitor SHALL report the configuration as unavailable with only ACTIVE supported, not as not configured

#### Scenario: Package Manager unavailable
- **WHEN** Package Manager cannot be reached
- **THEN** Resource Monitor SHALL report the configuration as unavailable with only ACTIVE supported
- **AND** Resource Monitor SHALL keep running and retry Package Manager on a later request

#### Scenario: Application has no locked package
- **WHEN** no installed version of the application's package is locked
- **THEN** Resource Monitor SHALL report the configuration as unavailable with only ACTIVE supported

### Requirement: Suspended-state configuration
Resource Monitor SHALL report the Suspended-state configuration from `applicationLifecycle`:
- the maximum system memory allowed in SUSPENDED, from `maxSuspendedSystemMemory`, in bytes;
- the maximum time in seconds to reach that memory level after entering SUSPENDED, from `maxTimeToSuspendMemoryState`;
- a Suspended CPU-usage threshold.

Each value SHALL be reported as present or unavailable independently. `maxSuspendedSystemMemory` SHALL accept a decimal number optionally followed by one of `G`, `M`, `K` or `B`, case-insensitive, optionally followed by `B`, using 1024-based multipliers. A missing, wrongly typed, negative or unparsable value SHALL be reported as unavailable and logged. The CPU threshold SHALL be reported as unavailable until a metadata member for it is defined.

#### Scenario: Suspended configuration present
- **WHEN** `applicationLifecycle` contains `"maxSuspendedSystemMemory": "16M"` and `"maxTimeToSuspendMemoryState": 10`
- **THEN** Resource Monitor SHALL report a maximum Suspended system memory of 16777216 bytes and a maximum time of 10 seconds

#### Scenario: Suspended configuration partially missing or invalid
- **WHEN** `applicationLifecycle` lacks `maxTimeToSuspendMemoryState` or contains `"maxSuspendedSystemMemory": "16X"`
- **THEN** the affected value SHALL be reported as unavailable, the remaining values SHALL still be reported, and Resource Monitor SHALL NOT fail

#### Scenario: CPU threshold not yet defined
- **WHEN** the Suspended configuration is requested for any application
- **THEN** the Suspended CPU-usage threshold SHALL be reported as unavailable

### Requirement: Application-to-package resolution
Resource Monitor SHALL accept an application identifier and use it, unchanged, as the Package Manager package identifier. It SHALL find the locked installed version through Package Manager. If exactly one installed version of the package is locked, its configuration SHALL be used. If more than one version is locked at once, Resource Monitor SHALL report the configuration as unavailable and log the ambiguity, unless the caller supplies a version, in which case that version SHALL be used.

#### Scenario: Single locked version
- **WHEN** versions `1.0.0` and `1.1.0` of the application's package are installed and only `1.1.0` is locked
- **THEN** Resource Monitor SHALL report the lifecycle configuration of version `1.1.0`

#### Scenario: Explicit version supplied
- **WHEN** a caller requests the lifecycle configuration for an application with a specific locked version
- **THEN** Resource Monitor SHALL report that version's configuration without listing other packages

#### Scenario: Multiple locked versions without an explicit version
- **WHEN** two versions of the application's package are locked and no version is supplied
- **THEN** Resource Monitor SHALL report the configuration as unavailable with only ACTIVE supported and log a warning

### Requirement: Concurrent use
The lifecycle-capability queries SHALL be safe to call at the same time from several Resource Monitor threads. A query SHALL NOT block Resource Monitor's existing memory monitoring or reconciliation while it waits for Package Manager.

#### Scenario: Concurrent queries
- **WHEN** several threads request lifecycle capabilities for the same and for different applications at the same time
- **THEN** each request SHALL return the result it would return on its own, and no request SHALL crash, deadlock or corrupt shared state

#### Scenario: Package Manager is slow to respond
- **WHEN** a lifecycle-capability query is waiting for Package Manager
- **THEN** Resource Monitor's reactive memory monitoring and reconcile processing SHALL continue unaffected
