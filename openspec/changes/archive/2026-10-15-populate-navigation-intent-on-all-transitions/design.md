## Context

LifecycleManager already stores the launch intent in `ApplicationContext` as the most recent intent. However, `StateHandler::sendEvent()` currently adds `navigationIntent` to transition event data only when the destination is `ACTIVE`. Notifications for earlier transitions, including a preload ending in `PAUSED`, therefore lack the intent before they are serialized and forwarded to listeners.

The existing notification contract is defined in [LifecycleManager.spec.md](../../specs/LifecycleManager.spec.md). The change stays within LifecycleManager and does not alter the JSON-RPC or `ILifecycleManagerState::INotification` method signatures.

## Goals / Non-Goals

**Goals:**
- Include the context's most recent request intent in event data for every lifecycle state transition.
- Ensure a preloaded app's `UNLOADED` → `LOADING` → `INITIALIZING` → `PAUSED` notifications all carry the launch intent.
- Preserve existing behavior for intent-only notifications and unexpected-termination reporting.

**Non-Goals:**
- Change lifecycle state progression, launch target selection, or intent delivery to the running app.
- Change notification fields, listener APIs, App Gateway behavior, build configuration, or dependencies.

## Decisions

1. **Read the intent from `ApplicationContext` for each transition.** `SpawnApp` records the request intent before starting the transition flow, and the context is the shared source of truth for transition notifications. Populate `eventData["navigationIntent"]` from `context->getMostRecentIntent()` regardless of destination state. This avoids adding per-transition intent parameters or special-casing `PAUSED`.

   Alternatives considered: propagating the intent through every transition callback would duplicate state already held by the context; modifying notification serialization would be later than the state event producer and would not reliably associate the correct request intent.

2. **Keep the external event contract unchanged.** The event continues to contain the existing app and state fields plus `navigationIntent`; only the value's availability changes. For example, a preload notification remains shaped as:

   ```json
   {"jsonrpc":"2.0","method":"client.events.1.onAppLifecycleStateChanged","params":{"appId":"YouTube","appInstanceId":"<instance-id>","oldLifecycleState":"INITIALIZING","newLifecycleState":"PAUSED","navigationIntent":"<launch-intent>"}}
   ```

   No CMake configuration changes or optional-feature interactions are required. The behavior is backward compatible for consumers because it populates an existing field without changing its type or event signature.

3. **Validate through transition-event tests.** Extend LifecycleManager L0 coverage to launch with a non-empty intent targeting `PAUSED` and assert that each emitted transition event carries that intent. Retain coverage for transitions to `ACTIVE` and other existing event behavior.

   ```text
   SpawnApp(intent, target=PAUSED)
       → ApplicationContext stores intent
       → StateHandler emits each transition with stored intent
       → LifecycleManager dispatches notification to listeners
       → App Gateway can forward intent to the preloaded app
   ```

## Risks / Trade-offs

- [A later request may replace the context's most recent intent before a queued transition emits] → Transitions are serialized per app; tests should cover the intent associated with the request being processed and confirm the stored value is read at event creation.
- [Consumers may have relied on the string `"null"` for non-active transitions] → The field is already part of the event contract; verify downstream consumers accept the actual supplied intent, which is the intended behavior.

## Migration Plan

No migration is required. Deploy the LifecycleManager implementation with the expanded event payload behavior. Rollback consists of reverting the implementation change; no persisted data or API negotiation is involved.

## Open Questions

None.