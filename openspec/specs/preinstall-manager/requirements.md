# PreinstallManager Detailed Requirements

## Scope
PreinstallManager discovers preloaded package artifacts and triggers installation workflows according to force and non-force policy.

## Functional Requirements

### FR-PRM-001: Discover package candidates
The system SHALL scan configured preinstall paths for installable package candidates.

Acceptance criteria:
- Valid directories are scanned and candidates are detected.
- Inaccessible directory yields categorized failure but does not crash processing.

### FR-PRM-002: Trigger install
The system SHALL invoke package installation for eligible discovered candidates.

Acceptance criteria:
- Eligible candidate triggers install request.
- Install failure is reported with package identity and category.

### FR-PRM-003: Force vs non-force policy
The system SHALL apply force/non-force behavior when evaluating already installed versions.

Acceptance criteria:
- Non-force skips same/newer installed versions.
- Force allows reinstall attempt.

### FR-PRM-004: StartPreinstall control
The system SHALL provide explicit entry point to start preinstall workflow.

Acceptance criteria:
- StartPreinstall triggers scan and evaluation.
- Concurrent start request returns deterministic busy/coalesced behavior.

### FR-PRM-005: Notification registration
The system SHALL support registration and unregistration of notification sinks.

Acceptance criteria:
- Valid sink receives workflow notifications.
- Duplicate registration follows deterministic idempotent behavior.

## Non-Functional Requirements

### NFR-PRM-001: Startup robustness
Preinstall workflow SHALL tolerate malformed candidates and continue processing remaining candidates.

### NFR-PRM-002: Determinism
Candidate ordering and policy evaluation SHALL be deterministic for same input state.

### NFR-PRM-003: Traceability
Each candidate decision (install/skip/fail) SHALL be externally diagnosable.

## Failure Taxonomy Mapping
PreinstallManager SHALL map failures to canonical categories:
- INVALID_ARGUMENT
- NOT_FOUND
- INVALID_STATE
- DEPENDENCY_UNAVAILABLE
- UNSUPPORTED_OPERATION
- TIMEOUT
- PERMISSION_DENIED
- TRANSIENT_FAILURE
- INTERNAL_ERROR
