# LifecycleManager Detailed Requirements

## Scope
LifecycleManager owns per-app lifecycle state machine transitions and coordinates runtime and window dependencies for each transition.

## Functional Requirements

### FR-LM-001: Spawn app instance
The system SHALL create a lifecycle context and initiate runtime startup for a new app instance.

Acceptance criteria:
- Returns success with appInstanceId when spawn succeeds.
- Invalid input returns INVALID_ARGUMENT.
- Runtime startup failure returns DEPENDENCY_UNAVAILABLE or INTERNAL_ERROR.
- Failed spawn does not leave orphan lifecycle context.

### FR-LM-002: Target state transition
The system SHALL support transitions to supported target states (ACTIVE, PAUSED, SUSPENDED, HIBERNATED where supported).

Acceptance criteria:
- Valid transition succeeds and updates current state.
- Unsupported transition returns UNSUPPORTED_OPERATION or INVALID_STATE.
- Unknown appInstanceId returns NOT_FOUND.

### FR-LM-003: Unload and kill operations
The system SHALL support graceful unload and force kill for loaded app instances.

Acceptance criteria:
- Unload attempts graceful runtime stop and cleanup.
- Kill triggers immediate termination path.
- Unknown app reference returns NOT_FOUND.

### FR-LM-004: Lifecycle state notifications
The system SHALL notify registered observers on state transitions.

Acceptance criteria:
- Event includes appId, appInstanceId, oldState, newState.
- Notification fanout tolerates one subscriber failure.
- Event order is deterministic per app instance.

### FR-LM-005: Ready and completion callbacks
The system SHALL process app-originated readiness/completion callbacks for transition finalization.

Acceptance criteria:
- Callback for unknown app is rejected deterministically.
- Valid callback finalizes pending transition.
- Duplicate callback is handled idempotently.

## Non-Functional Requirements

### NFR-LM-001: Transition integrity
Concurrent requests for same app instance SHALL not corrupt lifecycle context.

### NFR-LM-002: Deterministic transition rules
State transition validation SHALL be table-driven or equivalent deterministic logic.

### NFR-LM-003: Recovery
Unexpected runtime disconnects SHALL move lifecycle context into safe recoverable state.

## Failure Taxonomy Mapping
LifecycleManager SHALL map failures to canonical categories:
- INVALID_ARGUMENT
- NOT_FOUND
- INVALID_STATE
- DEPENDENCY_UNAVAILABLE
- UNSUPPORTED_OPERATION
- TIMEOUT
- PERMISSION_DENIED
- TRANSIENT_FAILURE
- INTERNAL_ERROR
