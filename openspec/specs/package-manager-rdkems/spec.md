# Capability: PackageManagerRDKEMS

## Requirements

### Requirement: Download application packages
The system SHALL support downloading application packages from remote sources and tracking download progress.

#### Scenario: Download package
Given a valid package URL and download options
When download is requested
Then the system starts package download and returns a download identifier

#### Scenario: Download rejected for invalid URL
Given download URL is malformed or unsupported
When download is requested
Then the system returns invalid-parameter failure

### Requirement: Install and uninstall packages
The system SHALL support package installation and uninstallation with clear success/failure reporting.

#### Scenario: Install package
Given a package artifact is available locally
When install is requested with package identity/version metadata
Then the system installs the package and records package state

#### Scenario: Install fails on manifest or payload validation
Given package artifact is present
And package manifest or payload validation fails
When install is requested
Then installation is rejected with validation failure reason

### Requirement: Prevent uninstall of in-use packages
The system SHALL support lock/unlock semantics that prevent uninstallation while a package is actively used by running applications.

#### Scenario: Uninstall blocked by lock
Given a package has one or more active locks
When uninstall is requested
Then the system rejects uninstall until locks are released

#### Scenario: Unlock of non-locked package
Given package has no active lock entry
When unlock is requested
Then the system returns not-found or invalid-state failure

### Requirement: Provide package metadata and state queries
The system SHALL provide methods to list packages and retrieve package lock/config state information.

#### Scenario: Query lock info
Given a package identity and version
When lock info is requested
Then the system returns whether the package is locked and associated metadata

#### Scenario: Query unknown package info
Given package identity and version do not exist
When state or lock metadata is requested
Then the system returns a deterministic not-found response

### Requirement: Integrate with app storage for package lifecycle
The system SHALL coordinate with application storage services during package lifecycle operations where storage allocation/cleanup is required.

#### Scenario: Install allocates storage
Given package installation requires app storage
When installation proceeds
Then the system invokes storage allocation and fails installation if required storage operations fail

#### Scenario: Storage cleanup on uninstall failure path
Given uninstall partially removes package assets
And storage cleanup call fails
When uninstall processing completes
Then the system returns failure
And remaining state is marked for recovery or retry

### Requirement: Use standardized failure taxonomy
The system SHALL classify operation failures using canonical categories: INVALID_ARGUMENT, NOT_FOUND, INVALID_STATE, DEPENDENCY_UNAVAILABLE, UNSUPPORTED_OPERATION, TIMEOUT, PERMISSION_DENIED, TRANSIENT_FAILURE, and INTERNAL_ERROR.

#### Scenario: Consistent category mapping
Given a package operation fails
When response is returned
Then one canonical taxonomy category is included
And equivalent package failures map consistently across APIs

#### Scenario: Preserve installer/downloader detail
Given installer or downloader returns native failure detail
When failure is propagated
Then canonical category is returned
And native detail is preserved as supplemental context
