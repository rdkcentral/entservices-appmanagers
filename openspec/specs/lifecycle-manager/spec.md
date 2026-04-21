# Capability: Lifecycle Manager

## Requirements

### Requirement: Enforce application lifecycle state machine
The system SHALL maintain a lifecycle state machine per app instance and SHALL allow only valid state transitions.

#### Scenario: Valid transition accepted
Given an app instance is in a valid source state for a requested transition
When a transition request is submitted
Then the system applies the transition and updates the app state

#### Scenario: Invalid transition rejected
Given an app instance is in a source state that does not allow the requested target
When a transition request is submitted
Then the system rejects the request with invalid-state failure
And app state remains unchanged

### Requirement: Spawn application instances
The system SHALL support spawning a new application instance and associating it with lifecycle context data.

#### Scenario: Spawn app
Given a client provides app identity and runtime configuration
When spawn is requested
Then the system creates lifecycle context for the app instance
And reports progress toward initialization/ready states

#### Scenario: Spawn fails on runtime startup failure
Given lifecycle context is created
And runtime startup fails
When spawn processing continues
Then the system returns spawn failure with error reason
And context is cleaned up to avoid orphaned state

### Requirement: Support target state changes
The system SHALL allow requests to move an existing app instance to a target state such as active, paused, suspended, or hibernated where supported.

#### Scenario: Move app to paused
Given an app instance is active
When the client requests paused target state
Then the system coordinates required downstream operations
And updates lifecycle state to paused on success

#### Scenario: Unsupported target state request
Given a target state is unsupported for current runtime policy
When target state is requested
Then the system returns failure indicating unsupported transition

### Requirement: Support unload and kill operations
The system SHALL provide both graceful unload and force-kill paths for managed app instances.

#### Scenario: Force kill
Given an app instance is loaded
When a force-kill request is submitted
Then the system attempts immediate termination
And reports completion/failure to the caller

#### Scenario: Unload unknown instance
Given requested app instance id is not present
When unload is requested
Then the system returns not-found failure

### Requirement: Publish lifecycle state notifications
The system SHALL notify registered observers of app lifecycle changes.

#### Scenario: Observer receives state change
Given an observer is registered
When an app lifecycle state changes
Then the observer receives a notification including app instance identity and transition information

#### Scenario: Notification fanout survives one observer failure
Given multiple observers are registered
And one observer fails to consume notification
When lifecycle state changes
Then notification delivery proceeds for remaining observers

### Requirement: Use standardized failure taxonomy
The system SHALL classify operation failures using canonical categories: INVALID_ARGUMENT, NOT_FOUND, INVALID_STATE, DEPENDENCY_UNAVAILABLE, UNSUPPORTED_OPERATION, TIMEOUT, PERMISSION_DENIED, TRANSIENT_FAILURE, and INTERNAL_ERROR.

#### Scenario: Consistent category mapping
Given lifecycle operation fails for a known reason
When failure is returned
Then response includes one canonical taxonomy category
And equivalent failure reasons map consistently across lifecycle APIs

#### Scenario: Preserve service-specific detail
Given dependency interaction fails with native detail
When failure is propagated
Then response includes canonical category
And preserves native detail as supplemental context
