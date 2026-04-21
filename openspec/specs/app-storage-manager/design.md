# AppStorageManager Design

## Overview
AppStorageManager abstracts filesystem operations into app-level storage lifecycle APIs.

## Architecture

```text
Storage API facade
   |
   v
StorageManagerImplementation
   |---- Request validation
   |---- Path resolver
   |---- Filesystem operation layer
   |---- Usage calculator
```

## Design Elements

### Path resolver
Builds canonical storage root and app-scoped paths while preventing path traversal.

### Filesystem operation layer
Performs create/delete/clear operations with operation-specific safeguards.

### Usage calculator
Computes storage usage metrics using directory traversal/stat calls.

## Operation Flows

### Create storage
1. Validate appId and options.
2. Resolve canonical app path.
3. Create required directories and ownership.
4. Return created path.

### Clear storage
1. Resolve app path.
2. Remove clearable content.
3. Recreate baseline structure where required.
4. Return normalized status.

### Clear all
1. Enumerate app directories under storage root.
2. Filter exemptions.
3. Apply clear operation per target app.
4. Aggregate and return status.

## Concurrency Model
- App-level operations are synchronized to avoid conflicting mutations.
- Global clear acquires broader coordination to avoid race with per-app operations.

## Error Handling
- Validation issues map to INVALID_ARGUMENT.
- Missing app storage maps to NOT_FOUND.
- Filesystem permission issues map to PERMISSION_DENIED.

## Observability
- Logs include appId, path scope, operation, and failure category.
- Metrics include create/delete/clear duration and failure counts.

## Safety
- Enforce canonical path resolution before destructive operations.
- Avoid deleting outside configured storage root.
- Preserve deterministic idempotent behavior for repeated delete/clear requests.
