# App Storage Manager

## Overview
Specification for the App Storage Manager capability, which provides per-application dedicated storage lifecycle management including allocation, querying, deletion, and data clearing, with standardized failure reporting.

## Description
The App Storage Manager is responsible for managing dedicated storage locations for applications. It uses application identity inputs to create, query, clear, and delete app-scoped storage. The manager supports global clear operations with optional exemption lists and exposes a canonical failure taxonomy to ensure consistent error propagation across all storage operations.

## Requirements

### Requirement: Create app-specific storage allocations
The system SHALL create dedicated storage locations for applications using app identity inputs.

#### Scenario: Create storage
Given an application id and requested storage parameters
When create storage is requested
Then the system creates the app storage location and returns its path

#### Scenario: Create storage with invalid app id
Given app id is empty or malformed
When create storage is requested
Then the system returns invalid-parameter failure

#### Scenario: Create storage fails due to filesystem error
Given app id is valid
And filesystem path creation fails
When create storage is requested
Then the system returns storage-creation failure reason

### Requirement: Provide app storage usage information
The system SHALL return storage path and usage metadata for an application.

#### Scenario: Query storage
Given storage exists for an application
When get storage is requested
Then the system returns storage path, configured size, and current usage details

#### Scenario: Query non-existing storage
Given storage does not exist for requested app
When get storage is requested
Then the system returns not-found failure

### Requirement: Delete app storage on request
The system SHALL support deletion of all storage associated with an application.

#### Scenario: Delete storage
Given an application has allocated storage
When delete storage is requested
Then the system removes the app storage and reports success/failure

#### Scenario: Delete already-removed storage
Given requested storage path no longer exists
When delete storage is requested
Then system returns deterministic response as idempotent success or not-found failure

### Requirement: Clear app data while preserving allocation
The system SHALL clear application data without requiring full storage de-allocation.

#### Scenario: Clear storage data
Given an application has stored data
When clear is requested
Then stored data is removed according to clear policy
And base allocation remains available for subsequent app use

#### Scenario: Clear operation filesystem permission failure
Given storage exists but filesystem permissions prevent clear
When clear is requested
Then operation fails with permission-related reason

### Requirement: Support global clear with exemptions
The system SHALL support clearing storage for all apps with an optional exemption list.

#### Scenario: Clear all with exemptions
Given multiple apps have storage allocations
And an exemption list contains selected app ids
When clear all is requested
Then data is cleared for non-exempt apps only

#### Scenario: Clear all with invalid exemption list input
Given exemption list payload cannot be parsed
When clear all is requested
Then the system returns invalid-parameter failure

### Requirement: Use standardized failure taxonomy
The system SHALL classify operation failures using canonical categories: INVALID_ARGUMENT, NOT_FOUND, INVALID_STATE, DEPENDENCY_UNAVAILABLE, UNSUPPORTED_OPERATION, TIMEOUT, PERMISSION_DENIED, TRANSIENT_FAILURE, and INTERNAL_ERROR.

#### Scenario: Consistent category mapping
Given a storage operation fails
When failure is returned
Then one canonical taxonomy category is included
And equivalent failures map consistently across storage APIs

#### Scenario: Preserve filesystem/native detail
Given filesystem or native layer returns detailed error
When failure is propagated
Then canonical category is returned
And native detail is preserved as supplemental context

## Architecture / Design
Design is documented in [openspec/specs/app-storage-manager/design.md](design.md). Key components:
- **Plugin facade** (`AppStorageManager.cpp/.h`) — Thunder RPC registration.
- **Implementation** (`AppStorageManagerImplementation.cpp/.h`) — storage allocation, query, delete, and clear orchestration.
- **RequestHandler** (`RequestHandler.cpp/.h`) — request routing and validation.
- **Telemetry bridge** (`AppStorageManagerTelemetryReporting.cpp/.h`) — timing markers.

## External Interfaces
_The AppStorageManager plugin exposes the following Thunder JSON-RPC methods (confirm against generated stubs):_
- `createStorage(appId, sizeBytes, [options])` → `{path}` or failure
- `getStorage(appId)` → `{path, configuredSize, usedBytes}` or failure
- `deleteStorage(appId)` → success/failure
- `clearStorage(appId, [clearPolicy])` → success/failure
- `clearAllStorage([exemptionList])` → success/failure

_Confirm exact parameter names and types against the generated Thunder interface definition._

## Performance
_Not yet defined — add storage creation latency and query latency targets when SLAs are established._

## Security
- App identity inputs are validated at the API boundary to prevent path traversal in storage path generation.
- Storage paths are isolated per app id; cross-app data access is not permitted.
- Clear operations are logged to support auditability.
- _Threat model not yet formal — add as follow-up._

## Versioning & Compatibility
_Not yet defined — add versioning scheme and backward-compatibility guarantees when the API is stabilised._

## Conformance Testing & Validation
- **L1 tests:** [Tests/L1Tests/tests/test_AppStorageManager.cpp](../../../Tests/L1Tests/tests/test_AppStorageManager.cpp)
- **L2 tests:** [Tests/L2Tests/tests/AppStorageManager_L2Test.cpp](../../../Tests/L2Tests/tests/AppStorageManager_L2Test.cpp)
- _Test documentation not yet written — add as follow-up._

## Covered Code
- AppStorageManager/AppStorageManager.cpp — `AppStorageManager` (plugin facade)
- AppStorageManager/AppStorageManager.h — `AppStorageManager`
- AppStorageManager/AppStorageManagerImplementation.cpp — `AppStorageManagerImplementation`
- AppStorageManager/AppStorageManagerImplementation.h — `AppStorageManagerImplementation`
- AppStorageManager/RequestHandler.cpp — `RequestHandler`
- AppStorageManager/RequestHandler.h — `RequestHandler`
- AppStorageManager/AppStorageManagerTelemetryReporting.cpp — `AppStorageManagerTelemetryReporting`
- AppStorageManager/Module.cpp, AppStorageManager/Module.h — Thunder module registration

---

## Open Queries
- Clear policy referenced in the "Clear storage data" scenario is not defined — what data is retained vs. removed? Does it differ by app type or configuration?
- "Delete already-removed storage" scenario allows either idempotent success or not-found failure — the expected behaviour should be made deterministic.
- Performance targets not yet defined.
- Formal versioning scheme not yet established.
- Path traversal mitigation in `appId`-to-path mapping needs explicit specification.

## References
- [openspec/specs/app-storage-manager/design.md](design.md)
- [openspec/specs/app-storage-manager/requirements.md](requirements.md)

## Change History
- [2026-04-23] - openspec-templater - Restructured to match spec template.
- [2026-04-23] - improve-openspec-coverage - Enriched External Interfaces, Security, Performance, Versioning, Conformance Testing, and Covered Code sections.
