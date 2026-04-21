# AppManager Detailed Requirements

## Scope
AppManager is the user-facing application lifecycle API boundary for launch, state control, data operations, and lifecycle notifications.

## Functional Requirements

### FR-AM-001: Launch API
The system SHALL expose an API to launch an app using appId, intent, and optional launch arguments.

Acceptance criteria:
- Returns success with appInstanceId when launch succeeds.
- Returns INVALID_ARGUMENT for malformed input.
- Returns NOT_FOUND if package does not exist.
- Returns DEPENDENCY_UNAVAILABLE when required downstream service is unavailable.

### FR-AM-002: App control API
The system SHALL provide CloseApp, TerminateApp, and KillApp controls.

Acceptance criteria:
- CloseApp performs policy-based background transition where applicable.
- TerminateApp performs graceful stop path.
- KillApp performs force stop path.
- Unknown app reference returns NOT_FOUND or INVALID_STATE.

### FR-AM-003: Loaded app inventory
The system SHALL return loaded apps with stable identity and lifecycle state.

Acceptance criteria:
- Response includes appId and appInstanceId.
- Response includes current lifecycle state.
- Empty inventory is returned as success when no app is loaded.

### FR-AM-004: App property operations
The system SHALL support reading and writing app-scoped properties through configured backing store.

Acceptance criteria:
- Get and Set operations return explicit status.
- Missing key returns deterministic not-found behavior.
- Store dependency failure returns DEPENDENCY_UNAVAILABLE or INTERNAL_ERROR.

### FR-AM-005: App data clear operations
The system SHALL support ClearAppData(appId) and ClearAllAppData(exemptions).

Acceptance criteria:
- Requests are delegated to storage manager.
- Exemptions are honored for clear-all.
- Storage failure propagates explicit error category.

### FR-AM-006: Lifecycle notifications
The system SHALL emit lifecycle state transition events to subscribers.

Acceptance criteria:
- Event payload includes app identity, old state, new state.
- Notifications are emitted for all externally visible transitions.
- Failure in one subscriber does not block others.

## Non-Functional Requirements

### NFR-AM-001: Latency
Control-path API calls SHALL return bounded responses under nominal conditions and avoid unbounded blocking on dependent services.

### NFR-AM-002: Consistency
For each app instance, state progression observed via APIs and notifications SHALL be monotonic and internally consistent.

### NFR-AM-003: Observability
All launch/control failures SHALL be diagnosable via category and supplemental failure detail.

## Failure Taxonomy Mapping
AppManager SHALL map failures to canonical categories:
- INVALID_ARGUMENT
- NOT_FOUND
- INVALID_STATE
- DEPENDENCY_UNAVAILABLE
- UNSUPPORTED_OPERATION
- TIMEOUT
- PERMISSION_DENIED
- TRANSIENT_FAILURE
- INTERNAL_ERROR
