## 1. Transition Event Implementation

- [x] 1.1 Update `LifecycleManager/StateHandler.cpp` so every lifecycle transition event includes the `ApplicationContext`'s most recent navigation intent, without changing existing event fields or unexpected-termination handling.
- [x] 1.2 Verify the updated event payload meets the `LifecycleManager` requirements in `openspec/changes/populate-navigation-intent-on-all-transitions/specs/LifecycleManager/spec.md` and preserves existing behavior for active transitions and intent-only notifications.

## 2. Regression Coverage

- [x] 2.1 Add or update `Tests/L0Tests/LifecycleManager/LifecycleManager_ContextTests.cpp` to assert that a preload request with a non-empty intent targeting `PAUSED` includes that intent in every transition notification.
- [x] 2.2 Add or update transition coverage for a non-preload state-change request with an intent. Local execution of the focused LifecycleManager L0 test target is deferred to GitHub Actions.