# PackageManagerRDKEMS Design

## Overview
PackageManagerRDKEMS orchestrates package lifecycle operations and protects runtime usage through lock semantics.

## Architecture

```text
Package API facade
   |
   v
PackageManagerImplementation
   |---- Downloader adapter
   |---- Installer adapter
   |---- Lock manager
   |---- Package state registry
   |---- Storage manager connector
```

## Core Structures
- Package state map keyed by packageId/version.
- Lock metadata per package identity.
- Install and failure metadata for diagnostics and retry.

## Workflow Design

### Install
1. Validate input and package artifact.
2. Parse metadata and prepare package destination.
3. Allocate required app storage.
4. Persist installed state and emit status.

### Uninstall
1. Validate package state and lock status.
2. Refuse uninstall when active locks exist.
3. Remove package assets and cleanup storage.
4. Update state registry and emit status.

### Lock/Unlock
1. Resolve package identity/version.
2. Create or update lock record.
3. Return lock metadata on lock.
4. Remove lock when count reaches zero.

## Concurrency Model
- Package state and lock maps are synchronized.
- Download and install operations avoid long critical sections.
- State updates are serialized per package key.

## Error Handling
- Validation failures return INVALID_ARGUMENT.
- Missing package returns NOT_FOUND.
- Active lock uninstall returns INVALID_STATE.
- External service failures map to DEPENDENCY_UNAVAILABLE.

## Observability
- Logs include packageId/version, operation stage, and failure category.
- Metrics include install duration, uninstall duration, and lock contention statistics.

## Security and Integrity
- Package identity and version are validated before state mutation.
- Lock semantics prevent unsafe uninstall during runtime use.
- Partial failures preserve recovery metadata.
