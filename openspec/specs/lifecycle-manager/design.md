# LifecycleManager Design

## Overview
LifecycleManager implements and enforces application lifecycle state transitions, mediating RuntimeManager and RDKWindowManager operations required by each transition.

## Architecture

```text
Lifecycle API facade
   |
   v
LifecycleManagerImplementation
   |---- ApplicationContext registry
   |---- Transition validator
   |---- Transition executor
   |---- Runtime handler
   |---- Window handler
   |
   v
Observer notifications
```

## Core Data Model

### ApplicationContext
Holds:
- appId
- appInstanceId
- currentState
- targetState
- runtime metadata and transition metadata

### Transition request
Includes:
- source state
- target state
- trigger operation
- timeout/policy context

## Transition Pipeline
1. Validate app instance and request.
2. Validate source-to-target transition rule.
3. Execute dependency operations (runtime/window) in ordered steps.
4. Persist state change.
5. Emit notifications.

## Supported State Families
- Unloaded/startup path
- Foreground/background path
- Suspend/hibernate path
- Termination path

## Failure and Rollback Strategy
- If dependency step fails, transition fails with canonical category.
- State remains in last confirmed valid state.
- Cleanup action is attempted for partially applied operations.

## Concurrency Model
- Per-app synchronization for state mutation.
- Independent app instances can transition concurrently.
- Notification dispatch is isolated from transition lock duration.

## Integration Points
- RuntimeManager for run/suspend/resume/hibernate/terminate/kill.
- RDKWindowManager for display/focus/visibility changes.
- Upstream AppManager for request orchestration and user-facing events.

## Observability
- Trace each transition with operation stage markers.
- Log rejected transitions with source/target and reason category.
- Emit metrics for transition duration and failure distribution.
