# RuntimeManager Design

## Overview
RuntimeManager abstracts runtime backend operations and translates app-level execution requests into backend/container operations with deterministic status mapping.

## Architecture

```text
Runtime API facade
   |
   v
RuntimeManagerImplementation
   |---- Runtime app registry
   |---- Spec/config resolver
   |---- Runtime backend adapter
   |---- Window manager connector
   |---- Event listener bridge
   |
   v
Lifecycle callbacks
```

## Core Components

### Runtime app registry
Tracks runtime state and identifiers per app instance.

### Spec/config resolver
Builds runtime startup input (paths, env, mounts, limits).

### Backend adapter
Executes run/suspend/resume/hibernate/wake/terminate/kill.

### Event bridge
Receives backend events and forwards normalized state events.

## Operation Flows

### Run
1. Validate request and app metadata.
2. Resolve runtime inputs and display linkage.
3. Start backend runtime/container.
4. Record runtime context.
5. Emit start event.

### Hibernate/Wake
1. Validate state eligibility.
2. Execute checkpoint/restore operation.
3. Update runtime state and emit correlated events.

### Terminate/Kill
1. Validate appInstanceId.
2. Trigger graceful or force stop.
3. Cleanup display/runtime artifacts.
4. Emit terminal event.

## Concurrency and Synchronization
- Registry updates are synchronized.
- Long backend operations run without holding global locks.
- Event callbacks synchronize only minimal mutable state.

## Error Handling
- Native backend errors are mapped to canonical categories.
- Operation stage is included in supplemental detail.
- Unknown backend outcomes default to INTERNAL_ERROR.

## Observability
- Logs include operation, appInstanceId, stage, and failure category.
- Metrics include startup latency, suspend/resume latency, and terminal failure rates.

## Safety and Recovery
- Partial run failures trigger cleanup handlers.
- Stale runtime contexts are purged on backend disconnect.
- Idempotent stop handling prevents repeated teardown faults.
