# RDKWindowManager Detailed Requirements

## Scope
RDKWindowManager owns app display lifecycle, focus, visibility, input interception, inactivity reporting, and stacking order operations.

## Functional Requirements

### FR-WM-001: Display lifecycle
The system SHALL create and manage display/session resources for app instances.

Acceptance criteria:
- CreateDisplay returns display identity on success.
- Invalid display payload returns INVALID_ARGUMENT.
- Compositor unavailable returns DEPENDENCY_UNAVAILABLE.

### FR-WM-002: Focus and visibility control
The system SHALL support per-client focus and visibility operations.

Acceptance criteria:
- SetFocus updates active client when target exists.
- SetVisible/GetVisibility operate deterministically.
- Unknown client returns NOT_FOUND.

### FR-WM-003: Input interception
The system SHALL support adding/removing key intercepts and key listeners.

Acceptance criteria:
- Valid intercept registration is applied.
- Duplicate registration follows deterministic idempotent behavior.
- Invalid key payload returns INVALID_ARGUMENT.

### FR-WM-004: Inactivity reporting
The system SHALL support enable/disable inactivity reporting and inactivity interval configuration.

Acceptance criteria:
- Reporting toggle is persisted and observable.
- Invalid interval returns INVALID_ARGUMENT.

### FR-WM-005: Z-order management
The system SHALL support SetZOrder and GetZOrder by app instance.

Acceptance criteria:
- Known appInstanceId accepts z-order update.
- Unknown appInstanceId returns NOT_FOUND.

## Non-Functional Requirements

### NFR-WM-001: Responsiveness
Control operations SHALL return bounded responses and avoid indefinite waits on compositor calls.

### NFR-WM-002: Input safety
Input routing changes SHALL not cause event storms or duplicate dispatch for the same intercept.

### NFR-WM-003: Stability
Failure in one client session SHALL not invalidate unrelated client sessions.

## Failure Taxonomy Mapping
RDKWindowManager SHALL map failures to canonical categories:
- INVALID_ARGUMENT
- NOT_FOUND
- INVALID_STATE
- DEPENDENCY_UNAVAILABLE
- UNSUPPORTED_OPERATION
- TIMEOUT
- PERMISSION_DENIED
- TRANSIENT_FAILURE
- INTERNAL_ERROR
