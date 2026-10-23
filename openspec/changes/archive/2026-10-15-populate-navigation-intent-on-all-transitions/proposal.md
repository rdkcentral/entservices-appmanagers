## Why

LifecycleManager currently populates `navigationIntent` only for the `PAUSED` to `ACTIVE` transition; other `onAppLifecycleStateChanged` events report the literal string `"null"`. When an app is preloaded with an intent and reaches `PAUSED`, App Gateway therefore receives no intent to pass to the app. This change makes the launch intent available throughout the lifecycle transition notifications.

## What Changes

- Require `onAppLifecycleStateChanged` notifications to carry the navigation intent associated with the lifecycle request for every state transition, including transitions during preload.
- Preserve the existing notification/API shape; do not change lifecycle state handling or intent delivery to active apps.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `LifecycleManager`: Define navigation-intent propagation on all lifecycle state change notifications.

## Impact

The change affects LifecycleManager's transition notification implementation and its existing `LifecycleManager` OpenSpec contract. App Gateway benefits from receiving the preload intent in notifications. No API schema, dependency, CMake option, or build impact is expected; this is not a breaking change.

## Non-goals

- Changing lifecycle transition rules or target-state selection.
- Changing how App Gateway or the app consumes or delivers the intent.
- Altering intent behavior for APIs other than lifecycle state change notifications.