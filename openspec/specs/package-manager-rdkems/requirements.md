# PackageManagerRDKEMS Detailed Requirements

## Scope
PackageManagerRDKEMS manages package download integration, install/uninstall, lock semantics, and package metadata/state queries.

## Functional Requirements

### FR-PM-001: Package install and uninstall
The system SHALL install and uninstall packages with explicit status and failure reason.

Acceptance criteria:
- Install succeeds only for valid package artifacts.
- Invalid package content returns INVALID_ARGUMENT or INTERNAL_ERROR.
- Uninstall fails with INVALID_STATE when package is actively locked.

### FR-PM-002: Package lock and unlock
The system SHALL provide lock/unlock semantics to protect in-use packages.

Acceptance criteria:
- Lock returns lock metadata and usage context.
- Unlock reduces lock state deterministically.
- Unlock unknown lock context returns NOT_FOUND or INVALID_STATE.

### FR-PM-003: Package metadata and list query
The system SHALL provide package list and lock/config metadata retrieval.

Acceptance criteria:
- ListPackages returns deterministic inventory payload.
- Query for unknown package returns NOT_FOUND.

### FR-PM-004: Download controls
The system SHALL expose package download-related controls where downloader integration is configured.

Acceptance criteria:
- Download/Cancel/RateLimit/Progress return explicit status.
- Invalid URL or options return INVALID_ARGUMENT.

### FR-PM-005: Storage integration
The system SHALL coordinate storage allocation and cleanup during install/uninstall.

Acceptance criteria:
- Install triggers required storage setup.
- Storage setup failure aborts install with dependency category.
- Uninstall cleanup failures are surfaced with recoverable detail.

## Non-Functional Requirements

### NFR-PM-001: Consistency
Package state transitions SHALL be atomic from API perspective.

### NFR-PM-002: Integrity
Install path SHALL validate package identity/version and avoid partial success without state marking.

### NFR-PM-003: Recoverability
Partial failures SHALL leave enough metadata for cleanup or retry.

## Failure Taxonomy Mapping
PackageManagerRDKEMS SHALL map failures to canonical categories:
- INVALID_ARGUMENT
- NOT_FOUND
- INVALID_STATE
- DEPENDENCY_UNAVAILABLE
- UNSUPPORTED_OPERATION
- TIMEOUT
- PERMISSION_DENIED
- TRANSIENT_FAILURE
- INTERNAL_ERROR
