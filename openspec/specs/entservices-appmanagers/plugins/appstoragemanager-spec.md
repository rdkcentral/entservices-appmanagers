# EntServices-AppManagers Plugin Spec - AppStorageManager

## Overview
AppStorageManager provisions and manages per-application persistent storage quotas and storage metadata access.

## Description
This plugin exposes storage create/get/delete workflows, tracks quota and size state, and mediates requests to persistent store components used by application data paths.

## Requirements
### Requirement: AppStorageManager provisions per-app storage
AppStorageManager MUST create and initialize persistent storage records/directories for valid app identities.

#### Scenario: Storage creation request
- **WHEN** a client requests storage provisioning for an app
- **THEN** plugin validates app identity and creates storage metadata and backing path

### Requirement: AppStorageManager enforces quota constraints
AppStorageManager SHALL verify available space and configured quota limits before approving storage operations.

#### Scenario: Quota/space validation
- **WHEN** requested storage exceeds available or allowed size
- **THEN** plugin rejects operation with deterministic failure status

### Requirement: AppStorageManager supports storage lifecycle operations
AppStorageManager MUST provide retrieval and deletion semantics for app storage descriptors and associated metadata.

#### Scenario: Storage deletion
- **WHEN** a delete request is issued for existing app storage
- **THEN** plugin removes metadata and storage artifacts according to policy

## Architecture / Design (if applicable)
- RequestHandler encapsulates app storage validation, quota checks, and persistent-store interactions.
- Plugin registration layer exposes RPC-facing methods to callers.

## External Interfaces (if applicable)
- App storage API registration (`Exchange::JAppStorageManager::Register`, `Unregister`)
- Storage request operations (`CreateStorage`, `GetStorage`, `DeleteStorage`)

## Performance (if applicable)
- Storage metadata lookup SHOULD remain efficient for launch-time storage access paths.

## Security (if applicable)
- Storage operations MUST validate app ownership and avoid cross-app data exposure.

## Versioning & Compatibility (if applicable)
- Storage API behavior SHALL preserve backward compatibility for existing quota and retrieval clients.

## Conformance Testing & Validation (if applicable)
- Validate quota enforcement, metadata persistence, and create/get/delete behavior with plugin tests.

## Covered Code
- entservices-appmanagers/AppStorageManager/AppStorageManager.cpp:
    - AppStorageManager::Initialize
    - AppStorageManager::Deinitialize
- entservices-appmanagers/AppStorageManager/AppStorageManagerImplementation.cpp:
    - AppStorageManagerImplementation::CreateStorage
    - AppStorageManagerImplementation::GetStorage
    - AppStorageManagerImplementation::DeleteStorage
- entservices-appmanagers/AppStorageManager/RequestHandler.cpp:
    - RequestHandler::CreateStorage
    - RequestHandler::GetStorage
    - RequestHandler::DeleteStorage
    - RequestHandler::hasEnoughStorageFreeSpace
    - RequestHandler::getDirectorySizeInBytes

---

## Open Queries
- Should storage provisioning expose soft quota warnings separate from hard quota violations?

## References
- entservices-appmanagers/AppStorageManager/AppStorageManager.cpp
- entservices-appmanagers/AppStorageManager/AppStorageManagerImplementation.cpp
- entservices-appmanagers/AppStorageManager/RequestHandler.cpp

## Change History
- [2026-05-03] - openspec-explore - Added per-plugin AppStorageManager specification.
