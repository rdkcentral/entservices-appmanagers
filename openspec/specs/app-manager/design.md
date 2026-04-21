# AppManager Design

## Overview
AppManager orchestrates app lifecycle requests by validating client input, invoking dependent managers, tracking instance context, and emitting lifecycle notifications.

## Architecture

```text
Client API
   |
   v
AppManager (plugin facade)
   |
   v
AppManagerImplementation (orchestrator)
   |---- Package manager integration (lock/unlock, metadata)
   |---- Lifecycle manager integration (spawn/target-state/unload)
   |---- Storage integration (clear operations)
   |---- Property store integration (get/set app properties)
   |
   v
Notification fanout (state/event subscribers)
```

## Components

### Plugin facade
- Registers and exposes RPC surface.
- Handles initialization and deinitialization.
- Bridges generated RPC stubs to implementation interface.

### Orchestrator
- Maintains per-app context.
- Coordinates launch, close, terminate, and kill workflows.
- Translates dependency failures to canonical categories.

### Lifecycle connector
- Encapsulates interactions with lifecycle manager.
- Handles lifecycle event callbacks and updates local state.

### Telemetry bridge
- Reports launch and state transition timing markers.
- Emits failure markers with normalized category.

## Key Flows

### Launch flow
1. Validate appId and launch request payload.
2. Resolve/lock package metadata.
3. Request lifecycle spawn with runtime config.
4. Track app instance context.
5. Emit state notifications as transitions occur.

### Terminate flow
1. Validate app reference.
2. Request lifecycle unload.
3. Unlock package and clear in-memory app context.
4. Emit unloaded event.

### Clear data flow
1. Validate request and exemptions.
2. Invoke storage clear API.
3. Return normalized status.

## State Model
AppManager tracks externally visible app states and target state intent per app instance. It does not replace lifecycle manager state authority.

## Concurrency Model
- Shared app context is protected under synchronization.
- Event callbacks and request handlers coordinate through serialized state updates.
- Long-running operations avoid blocking event notification paths.

## Error Handling
- All dependency errors are normalized to canonical taxonomy.
- Supplemental detail includes dependent subsystem and operation stage.
- Partial launch paths include rollback/cleanup hooks.

## Observability
- Entry/exit and failure points are logged.
- Metrics include launch latency, close latency, and failure counts by category.
- Notification emission outcomes are traceable.

## Security and Safety
- Input validation enforced at API boundary.
- Unknown app references do not leak internal state.
- Force operations require explicit API call and are auditable.
