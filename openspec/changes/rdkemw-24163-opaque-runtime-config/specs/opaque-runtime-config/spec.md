## Purpose

Define an evolvable runtime configuration contract that crosses app-manager plugin boundaries as an opaque JSON object while preserving established package and container launch semantics.

## ADDED Requirements

### Requirement: Opaque inter-plugin contract
The system SHALL transport runtime configuration between PackageManager, AppManager, LifecycleManager, and RuntimeManager as a string containing one JSON object. The public Exchange interfaces SHALL NOT expose a shared runtime configuration structure.

#### Scenario: Runtime configuration crosses each boundary
- **WHEN** an installed package is prepared for launch
- **THEN** PackageManager SHALL return a serialized JSON object and AppManager and LifecycleManager SHALL forward serialized JSON objects to the next manager
- **AND** RuntimeManager SHALL receive the final payload with all upstream additions and updates

#### Scenario: A new opaque property is introduced
- **WHEN** a producer adds a property that an intermediate plugin does not own or interpret
- **THEN** the intermediate plugin SHALL require no shared structure or complete-schema update to preserve and forward it

#### Scenario: Coordinated interface deployment
- **WHEN** the opaque-string interface revision is deployed
- **THEN** every participating plugin SHALL be rebuilt against the same compatible `entservices-apis` revision

### Requirement: Canonical payload production
PackageManager SHALL produce the payload from package metadata using the established runtime property names, values, derivations, and defaults. `envVariables`, `fkpsFiles`, and `logLevels` SHALL be JSON arrays containing only strings.

#### Scenario: Package metadata contains collections
- **WHEN** package metadata contains environment variables, FKPS files, or log levels
- **THEN** PackageManager SHALL encode each collection as a JSON string array rather than JSON text embedded in a string

#### Scenario: Runtime package association changes
- **WHEN** a runtime package is associated with an existing application package
- **THEN** PackageManager SHALL update only `runtimePath` and preserve every other payload member

### Requirement: Ownership-preserving enrichment
AppManager and LifecycleManager SHALL modify only properties owned by their stage and SHALL preserve all unmodified known and unknown values, including nested objects and arrays. A failed parse or mutation SHALL NOT replace the caller's original payload with a partially modified value.

#### Scenario: AppManager enriches a valid payload
- **WHEN** AppManager receives a valid package payload for launch or preload
- **THEN** it SHALL set `unpackedPath`, apply existing conditional identity and debug overrides, append launch environment values, and retain unrelated members

#### Scenario: LifecycleManager enriches a valid payload
- **WHEN** LifecycleManager dispatches a launch to RuntimeManager
- **THEN** it SHALL set `dialId`, upsert `FIREBOLT_ENDPOINT` and `TARGET_STATE` in `envVariables`, retain unrelated members, and preserve the existing Firebolt port behavior
- **AND** RuntimeManager SHALL receive the updated values and every preserved unknown value

#### Scenario: Application respawns
- **WHEN** LifecycleManager respawns an application
- **THEN** it SHALL reuse the saved opaque payload so previously established runtime configuration remains available

### Requirement: Invalid upstream payload rejection
AppManager and LifecycleManager SHALL reject an empty, malformed, or non-object payload through their established failure paths before forwarding it or creating launch state that depends on it.

#### Scenario: AppManager receives malformed lock output
- **WHEN** PackageManager lock output is not a valid JSON object
- **THEN** AppManager SHALL fail the launch or preload and SHALL NOT call LifecycleManager to spawn the application

#### Scenario: LifecycleManager receives a non-object payload
- **WHEN** `SpawnApp` receives valid JSON whose root is not an object
- **THEN** LifecycleManager SHALL report failure and SHALL NOT queue or create an application context

### Requirement: RuntimeManager-owned interpretation
RuntimeManager SHALL be the sole component with complete knowledge of known runtime payload members. It SHALL decode the payload once into a private typed model, tolerate unknown members, apply established optional defaults, and validate known member types and launch-required values.

#### Scenario: Valid payload contains an unknown extension
- **WHEN** RuntimeManager receives a valid payload containing known fields and an unknown nested property
- **THEN** it SHALL decode the known fields and SHALL NOT reject the payload because of the unknown property

#### Scenario: RuntimeManager receives malformed or incomplete data
- **WHEN** RuntimeManager receives an empty payload, malformed JSON, a non-object root, or missing launch-required data
- **THEN** it SHALL report the failure through the established error mechanism
- **AND** it SHALL NOT crash, use undefined configuration values, create a display, add a runtime entry, generate a RALF bundle, or launch a container

#### Scenario: Known field has the wrong type
- **WHEN** a known scalar or collection member has an incompatible JSON type or an invalid numeric range
- **THEN** RuntimeManager SHALL return the established invalid-parameter result before creating a display, runtime entry, RALF bundle, or container

#### Scenario: Optional values are absent
- **WHEN** a valid payload omits an optional member
- **THEN** RuntimeManager and the launch generators SHALL use the same default behavior as before the interface migration

#### Scenario: Command requirement varies by runtime
- **WHEN** a legacy Dobby launch omits its required command
- **THEN** RuntimeManager SHALL reject the payload
- **AND** a RALF launch SHALL remain eligible to proceed without that command when RALF metadata supplies launch configuration

### Requirement: Launch behavior equivalence
For equivalent package metadata and launch inputs, the opaque-payload path SHALL preserve existing Dobby and RALF behavior for identity, capabilities, environment, networking, resource limits, mounts, logging, FKPS, Firebolt, Rialto, GStreamer, storage, display, debug settings, and telemetry inputs.

#### Scenario: Legacy application is launched
- **WHEN** the refactored path receives inputs equivalent to a pre-refactor legacy launch
- **THEN** the normalized generated Dobby specification and launch environment SHALL be semantically equivalent except for the opaque transport representation

#### Scenario: RALF application is launched
- **WHEN** the refactored path receives inputs equivalent to a pre-refactor RALF launch
- **THEN** the normalized generated OCI configuration and launch behavior SHALL be semantically equivalent except for the opaque transport representation

### Requirement: Automated regression coverage
Automated tests SHALL cover payload creation, forwarding, owned mutation, unknown-property preservation, final RuntimeManager consumption, invalid payload handling, and behavior equivalence. All applicable existing PackageManager, AppManager, LifecycleManager, and RuntimeManager tests SHALL pass.

#### Scenario: Affected automated suites are executed
- **WHEN** the refactored flow is verified by the repository's GitHub workflows in the production CI setup
- **THEN** L0, affected L1, and applicable L2 tests SHALL execute rather than only compile
- **AND** they SHALL report no failures

#### Scenario: Runtime output compatibility is tested
- **WHEN** representative legacy Dobby and RALF configurations pass through the new flow
- **THEN** tests SHALL compare their normalized generated runtime configuration and launch inputs with pre-refactor expectations

### Requirement: Interface and documentation alignment
Affected public interfaces and technical documentation SHALL describe the runtime configuration as an opaque mutable payload and SHALL NOT instruct consumers to construct or depend on the removed shared `RuntimeConfig` contract.

#### Scenario: Contracts and documentation are reviewed
- **WHEN** the interface headers, generated API documentation, component documentation, and architecture specifications are inspected
- **THEN** each affected boundary SHALL document the opaque payload and its ownership or validation responsibility
- **AND** no affected consumer guidance SHALL require the removed shared structure

### Requirement: Incidental consumers remain opaque
A component that does not interpret runtime configuration, including PreinstallManager, SHALL treat the payload as an opaque string and SHALL NOT depend on its member schema.

#### Scenario: PreinstallManager inspects a package
- **WHEN** PreinstallManager calls the package configuration interface
- **THEN** it SHALL receive the opaque output without parsing or changing it
