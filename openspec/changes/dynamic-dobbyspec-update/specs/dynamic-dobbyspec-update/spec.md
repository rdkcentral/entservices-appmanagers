## ADDED Requirements

### Requirement: Register per-instance Dobby spec override
The system SHALL allow callers to register a custom Dobby spec JSON fragment keyed by `appInstanceId` before the container is started, specifying a merge strategy of `append` or `replace`.

#### Scenario: Override registered successfully
- **WHEN** `SetDobbySpecOverride` is called with a valid `appInstanceId` and valid JSON string and a valid `mergeStrategy`
- **THEN** the system stores the override in memory keyed by `appInstanceId` and returns `ERROR_NONE`

#### Scenario: Override rejected for empty appInstanceId
- **WHEN** `SetDobbySpecOverride` is called with an empty `appInstanceId`
- **THEN** the system returns `INVALID_ARGUMENT` failure and does not store any override

#### Scenario: Override rejected for malformed JSON
- **WHEN** `SetDobbySpecOverride` is called with a non-empty `appInstanceId` but invalid JSON for the spec fragment
- **THEN** the system returns `INVALID_ARGUMENT` failure and does not store any override

#### Scenario: Override replaces any previous pending override for same instance
- **WHEN** `SetDobbySpecOverride` is called twice for the same `appInstanceId`
- **THEN** the system stores only the latest override, discarding the earlier one

### Requirement: Apply Dobby spec override during container launch
The system SHALL apply a registered Dobby spec override after base spec generation and before the container is started during `Run()`, using the configured merge strategy.

#### Scenario: Append strategy merges additively
- **WHEN** a `Run()` is executed for an `appInstanceId` that has an `append` override registered
- **THEN** the system deep-merges the override JSON into the generated base spec, adding new keys and appending to arrays without removing existing fields

#### Scenario: Replace strategy overwrites matching keys
- **WHEN** a `Run()` is executed for an `appInstanceId` that has a `replace` override registered
- **THEN** the system deep-merges the override JSON into the generated base spec, overwriting any keys present in the override with the override values

#### Scenario: No override registered leaves spec unchanged
- **WHEN** a `Run()` is executed for an `appInstanceId` with no registered override
- **THEN** the system uses the base generated spec unchanged with no modification

#### Scenario: Override is consumed and cleared after use
- **WHEN** `Run()` applies an override for an `appInstanceId`
- **THEN** the override is removed from the in-memory store and subsequent `Run()` calls for the same instance use no override unless re-registered

### Requirement: Clear a pending Dobby spec override
The system SHALL allow callers to remove a previously registered override for a given `appInstanceId` before the container is started.

#### Scenario: Clear registered override
- **WHEN** `ClearDobbySpecOverride` is called with a valid `appInstanceId` that has a pending override
- **THEN** the system removes the stored override and returns `ERROR_NONE`

#### Scenario: Clear on non-existent override returns not-found
- **WHEN** `ClearDobbySpecOverride` is called for an `appInstanceId` with no registered override
- **THEN** the system returns `NOT_FOUND` failure
