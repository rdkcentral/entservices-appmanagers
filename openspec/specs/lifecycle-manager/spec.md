# Lifecycle Manager

## Overview
Specification for the Lifecycle Manager capability, which maintains a per-application-instance lifecycle state machine, orchestrates spawn/target-state/unload/kill operations, and publishes state-change notifications to registered observers.

## Description
The Lifecycle Manager is the authoritative source for application instance lifecycle state on the platform. It enforces a defined state machine per app instance, accepting transition requests from App Manager and rejecting invalid transitions. It coordinates with Runtime Manager and Window Manager to execute spawn, target-state, and unload operations, cleans up context on failure, and notifies all registered observers of state changes. All failures are normalised to the canonical failure taxonomy.

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

## Architecture / Design
Design is documented in [openspec/specs/lifecycle-manager/design.md](design.md). Key components:
- **Plugin facade** (`LifecycleManager.cpp/.h`) — Thunder RPC registration.
- **Implementation** (`LifecycleManagerImplementation.cpp/.h`) — state machine authority and orchestration.
- **State / StateHandler / StateTransitionHandler** (`State.cpp/.h`, `StateHandler.h`, `StateTransitionHandler.cpp/.h`) — state machine definitions and transition execution.
- **ApplicationContext** (`ApplicationContext.cpp/.h`) — per-instance context storage.
- **RuntimeManagerHandler / WindowManagerHandler** — adapters for dependency service interactions.
- **Telemetry bridge** (`LifecycleManagerTelemetryReporting.cpp/.h`) — timing markers.

## External Interfaces
_The LifecycleManager plugin exposes the following Thunder JSON-RPC methods (confirm against generated stubs):_
- `spawn(appId, runtimeConfig)` → `{instanceId}` or failure
- `setTargetState(instanceId, targetState)` → success/failure
- `unload(instanceId)` → success/failure
- `kill(instanceId)` → success/failure
- `getState(instanceId)` → `{state}` or failure
- **Events:** `onLifecycleStateChanged(instanceId, appId, oldState, newState)`

_Confirm exact parameter names and types against the generated Thunder interface definition._

## Performance
_Not yet defined — add spawn latency and state-transition latency targets when SLAs are established._

## Security
- Input validation is enforced at the API boundary; requests for unknown instance ids return not-found without leaking state.
- Force-kill operations are explicitly logged and auditable.
- _Threat model not yet formal — add as follow-up._

## Versioning & Compatibility
_Not yet defined — add versioning scheme and backward-compatibility guarantees when the API is stabilised._

## Conformance Testing & Validation
- **L1 tests:** [Tests/L1Tests/tests/test_LifecycleManager.cpp](../../../Tests/L1Tests/tests/test_LifecycleManager.cpp)
- _Test documentation not yet written — add as follow-up._

## Covered Code
- LifecycleManager/LifecycleManager.cpp — `LifecycleManager` (plugin facade)
- LifecycleManager/LifecycleManager.h — `LifecycleManager`
- LifecycleManager/LifecycleManagerImplementation.cpp — `LifecycleManagerImplementation`
- LifecycleManager/LifecycleManagerImplementation.h — `LifecycleManagerImplementation`
- LifecycleManager/ApplicationContext.cpp — `ApplicationContext`
- LifecycleManager/ApplicationContext.h — `ApplicationContext`
- LifecycleManager/State.cpp — state definitions
- LifecycleManager/State.h — `State`
- LifecycleManager/StateHandler.h — `StateHandler`
- LifecycleManager/StateTransitionHandler.cpp — `StateTransitionHandler`
- LifecycleManager/StateTransitionHandler.h — `StateTransitionHandler`
- LifecycleManager/RequestHandler.cpp — `RequestHandler`
- LifecycleManager/RuntimeManagerHandler.cpp — `RuntimeManagerHandler`
- LifecycleManager/WindowManagerHandler.cpp — `WindowManagerHandler`
- LifecycleManager/LifecycleManagerTelemetryReporting.cpp — `LifecycleManagerTelemetryReporting`
- LifecycleManager/Module.cpp, LifecycleManager/Module.h — Thunder module registration

---

## Open Queries
- State machine diagram (allowed transitions) is in design.md — should be cross-referenced or reproduced here.
- Performance targets (spawn latency, transition latency) not yet defined.
- Formal versioning scheme not yet established.

## References
- [openspec/specs/lifecycle-manager/design.md](design.md)
- [openspec/specs/lifecycle-manager/requirements.md](requirements.md)

## Change History
- [2026-04-23] - openspec-templater - Restructured to match spec template.
