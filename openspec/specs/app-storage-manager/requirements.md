# AppStorageManager Detailed Requirements

## Scope
AppStorageManager provides app-scoped storage provisioning, usage query, clear, and delete operations.

## Functional Requirements

### FR-SM-001: Create storage
The system SHALL create app-scoped storage allocation for a valid app identifier.

Acceptance criteria:
- Valid appId returns created storage path.
- Invalid appId returns INVALID_ARGUMENT.
- Filesystem creation failure returns INTERNAL_ERROR or PERMISSION_DENIED.

### FR-SM-002: Query storage
The system SHALL return storage path and usage metadata.

Acceptance criteria:
- Existing storage returns configured size and used value.
- Missing storage returns NOT_FOUND.

### FR-SM-003: Delete storage
The system SHALL delete app-scoped storage on request.

Acceptance criteria:
- Existing storage is removed and acknowledged.
- Already deleted path follows deterministic idempotent behavior.

### FR-SM-004: Clear storage data
The system SHALL clear app data while preserving base allocation where policy requires.

Acceptance criteria:
- Clear removes app data content.
- Base allocation remains reusable post-clear.
- Permission error returns PERMISSION_DENIED.

### FR-SM-005: Clear all with exemptions
The system SHALL clear storage for all apps except specified exemptions.

Acceptance criteria:
- Non-exempt app storage is cleared.
- Exempt app storage remains untouched.
- Invalid exemption payload returns INVALID_ARGUMENT.

## Non-Functional Requirements

### NFR-SM-001: Data safety
Operations SHALL not cross app storage boundaries.

### NFR-SM-002: Performance
Usage query and clear operations SHALL complete within bounded time for normal storage sizes.

### NFR-SM-003: Consistency
Storage metadata returned to callers SHALL reflect post-operation state.

## Failure Taxonomy Mapping
AppStorageManager SHALL map failures to canonical categories:
- INVALID_ARGUMENT
- NOT_FOUND
- INVALID_STATE
- DEPENDENCY_UNAVAILABLE
- UNSUPPORTED_OPERATION
- TIMEOUT
- PERMISSION_DENIED
- TRANSIENT_FAILURE
- INTERNAL_ERROR
