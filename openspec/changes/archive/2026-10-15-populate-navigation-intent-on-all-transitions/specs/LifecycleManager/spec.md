## ADDED Requirements

### Requirement: Lifecycle transition notifications include the request navigation intent
When a lifecycle request supplies a navigation intent, LifecycleManager SHALL include that intent in every `onAppLifecycleStateChanged` notification emitted for transitions caused by that request, regardless of the destination state.

#### Scenario: Preload transitions retain the launch intent
- **WHEN** an app is spawned with a navigation intent and a target state of `PAUSED`
- **THEN** each `onAppLifecycleStateChanged` notification for the resulting transitions, including `UNLOADED` to `LOADING`, `LOADING` to `INITIALIZING`, and `INITIALIZING` to `PAUSED`, contains the supplied navigation intent

#### Scenario: Other target states retain the request intent
- **WHEN** a lifecycle state-change request supplies a navigation intent and causes one or more state transitions
- **THEN** every `onAppLifecycleStateChanged` notification for those transitions contains the supplied navigation intent