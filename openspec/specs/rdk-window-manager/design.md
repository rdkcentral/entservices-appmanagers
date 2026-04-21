# RDKWindowManager Design

## Overview
RDKWindowManager provides a controlled interface over compositor/window subsystems for app display management and input policy operations.

## Architecture

```text
Window API facade
   |
   v
RDKWindowManagerImplementation
   |---- Client/session registry
   |---- Compositor adapter
   |---- Input intercept manager
   |---- Inactivity tracker
   |---- Z-order manager
```

## Component Responsibilities

### Session registry
Tracks app/display/client mapping needed for focus, visibility, and z-order actions.

### Compositor adapter
Encapsulates backend calls for display create/destroy/focus/visibility.

### Input intercept manager
Maintains key intercept/listener registrations and de-duplicates entries.

### Inactivity tracker
Tracks last input event timing and periodic inactivity reporting.

## Operation Flows

### Create display
1. Validate payload.
2. Allocate compositor display/session.
3. Register session in local registry.
4. Return display identity.

### Focus/visibility
1. Resolve client from registry.
2. Execute compositor operation.
3. Return normalized status.

### Input intercept update
1. Validate key payload and client.
2. Apply add/remove operation in registry.
3. Propagate update to backend input path.

## Concurrency Model
- Registry access is synchronized.
- Input and inactivity events avoid holding shared locks for long operations.
- Backend callbacks are funneled through safe update path.

## Error Handling
- Backend errors map to canonical categories.
- Unknown client/session returns NOT_FOUND.
- Invalid payload returns INVALID_ARGUMENT.

## Observability
- Logs include client identity, operation, and category.
- Metrics include display create latency and intercept update outcomes.

## Security and Safety
- Reject malformed input payloads.
- Avoid exposing internal compositor identifiers beyond required interface fields.
- Preserve deterministic behavior for duplicate registration requests.
