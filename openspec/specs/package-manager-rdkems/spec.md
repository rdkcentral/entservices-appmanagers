# Package Manager (RDKEMS)

## Overview
Specification for the Package Manager (RDKEMS) capability, which manages the full lifecycle of application packages including download, installation, uninstallation, lock/unlock semantics, metadata queries, and storage integration.

## Description
The Package Manager handles downloading application packages from remote URLs, installing and uninstalling them with validation, preventing uninstallation of in-use packages via lock/unlock semantics, providing metadata and state queries, and coordinating with the App Storage Manager for storage allocation and cleanup. All failures use the canonical failure taxonomy.

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

## Architecture / Design
Design is documented in [openspec/specs/package-manager-rdkems/design.md](design.md). Key components:
- **Plugin facade** (`PackageManager.cpp/.h`) — Thunder RPC registration.
- **Implementation** (`PackageManagerImplementation.cpp/.h`) — download, install, uninstall, lock/unlock orchestration.
- **HTTP client** (`HttpClient.cpp/.h`) — package download transport.
- **Telemetry bridge** (`PackageManagerTelemetryReporting.cpp/.h`) — timing markers.

## External Interfaces
_The PackageManager plugin exposes the following Thunder JSON-RPC methods (confirm against generated stubs):_
- `downloadPackage(url, options)` → `{downloadId}` or failure
- `installPackage(packageId, version, metadata)` → success/failure
- `uninstallPackage(packageId, version)` → success/failure
- `lockPackage(packageId, version, lockId)` → success/failure
- `unlockPackage(packageId, version, lockId)` → success/failure
- `getLockInfo(packageId, version)` → `{locked, locks}` or failure
- `listPackages()` → array of package records

_Confirm exact parameter names and types against the generated Thunder interface definition._

## Performance
_Not yet defined — add install/uninstall latency and download throughput targets when SLAs are established._

## Security
- Package manifest and payload validation is enforced before installation.
- Lock semantics prevent race conditions between running apps and uninstall requests.
- Download URLs are validated before use; redirect handling should be constrained.
- _Threat model not yet formal — security review of package integrity verification is recommended._

## Versioning & Compatibility
_Not yet defined — add versioning scheme and backward-compatibility guarantees when the API is stabilised._

## Conformance Testing & Validation
- **L1 tests:** [Tests/L1Tests/tests/test_PackageManager.cpp](../../../Tests/L1Tests/tests/test_PackageManager.cpp)
- _Test documentation not yet written — add as follow-up._

## Covered Code
- PackageManager/PackageManager.cpp — `PackageManager` (plugin facade)
- PackageManager/PackageManager.h — `PackageManager`
- PackageManager/PackageManagerImplementation.cpp — `PackageManagerImplementation`
- PackageManager/PackageManagerImplementation.h — `PackageManagerImplementation`
- PackageManager/HttpClient.cpp — `HttpClient`
- PackageManager/HttpClient.h — `HttpClient`
- PackageManager/PackageManagerTelemetryReporting.cpp — `PackageManagerTelemetryReporting`
- PackageManager/Module.cpp, PackageManager/Module.h — Thunder module registration

---

## Open Queries
- Package integrity verification (signature checking, hash validation) is mentioned in the security design but requirements are not yet fully specified.
- Download URL redirect policy needs to be defined.
- Performance targets not yet defined.
- Formal versioning scheme not yet established.

## References
- [openspec/specs/package-manager-rdkems/design.md](design.md)
- [openspec/specs/package-manager-rdkems/requirements.md](requirements.md)

## Change History
- [2026-04-23] - openspec-templater - Restructured to match spec template.
