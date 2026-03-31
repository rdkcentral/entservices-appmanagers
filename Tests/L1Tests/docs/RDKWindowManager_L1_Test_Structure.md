# RDKWindowManager L1 Test Structure

## Purpose

This document explains how the WindowManager L1 tests are structured, and how mocking plus real implementation execution are combined to validate behavior.

Primary test source:
- Tests/L1Tests/tests/test_RDKWindowManager.cpp

## High-Level Strategy

The L1 suite uses a two-layer strategy:

1. Plugin wrapper layer tests
- Validates JSON-RPC registration and request forwarding in the RDKWindowManager plugin class.
- Uses a fully mocked IRDKWindowManager backend.
- Confirms wrapper method wiring, argument passing, and error code propagation.

2. Implementation layer tests
- Executes the real RDKWindowManagerImplementation logic.
- Mocks only external dependencies (compositor and wrapper backends, Thunder service/system interfaces).
- Validates parsing, branching, event fanout, listener callback flow, and lifecycle behavior.

This split keeps wrapper tests deterministic while still driving real implementation code for branch and function coverage.

## Fixture Design

### Wrapper Fixture: RDKWindowManagerTest

Core setup pattern:
- Create plugin proxy and JSON-RPC handler.
- Inject ServiceMock and COMLinkMock.
- COMLink Instantiate returns WindowManagerMock (mock IRDKWindowManager).
- Expect plugin Initialize to call backend Initialize and Register.

Teardown pattern:
- Expect backend Unregister and Deinitialize.
- Deactivate dispatcher.
- Reset PluginHost::IFactories.

What this fixture proves:
- Method registration exists for all exposed JSON-RPC APIs.
- Handler Invoke calls reach backend methods.
- Backend return values are surfaced correctly to caller.

### Implementation Fixture: RDKWindowManagerImplementationTest

Core setup pattern:
- Create real RDKWindowManagerImplementation proxy.
- Assign WorkerPoolImplementation.
- Install compositor shim mocks:
  - CompositorControllerImplMock
  - RdkWindowManagerImplMock
- Assign test factories.

Teardown pattern:
- Clear factories.
- Reset compositor/wrapper shim implementations to null.
- Reset worker pool assignment.

What this fixture proves:
- Real implementation methods execute and exercise internal logic.
- External system behavior is controlled through mocks.
- Event dispatch and callback conversion logic is validated end-to-end within implementation.

## Mocking Mechanisms Used

### Thunder and plugin boundary mocks
- ServiceMock
  - Emulates PluginHost::IShell behavior such as AddRef, Release, SubSystems, COMLink.
- COMLinkMock
  - Supplies mocked backend object for wrapper-level tests.
- WindowManagerMock
  - Full IRDKWindowManager mock for plugin forwarding tests.
- SubSystemMock
  - Validates subsystem Set calls in implementation Initialize path.

### Backend shim mocks for real implementation execution
- CompositorControllerImplMock
  - Controls behavior for input, visibility, z-order, key APIs, listeners, VNC, screenshot trigger points.
- RdkWindowManagerImplMock
  - Controls wrapper lifecycle functions (initialize, draw, update, deinitialize, timing helpers).

### Async/dispatch support
- WorkerPoolImplementation
  - Required to execute implementation dispatch jobs and listener-to-notification fanout.
- Promise/Future synchronization in tests
  - Confirms asynchronous dispatch from listener callbacks reaches notifications.

## Implementation Mechanisms Verified

The suite validates these mechanisms directly in RDKWindowManagerImplementation:

- JSON string parsing and input validation paths.
- Branch handling for success/failure returns from compositor APIs.
- Modifier-to-flag translation for key handling methods.
- Event registration, duplicate registration protection, unregister failure path.
- Dispatch fanout to multiple notifications for all event types.
- Listener callbacks invoking dispatchEvent and asynchronous delivery.
- Lifecycle path in Initialize and Deinitialize including:
  - Service reference handling.
  - Event listener setup/cleanup.
  - Subsystem signaling.
  - Renderer wrapper initialization/deinitialization.
- Screenshot request queueing and completion event dispatch path coverage.

## Why This Pattern Works

- Wrapper correctness and implementation correctness are tested separately, reducing false positives.
- Real implementation logic is exercised without requiring real compositor/runtime dependencies.
- External interactions remain deterministic because all side-effecting boundaries are mocked.
- Coverage improves meaningfully because tests target branch conditions, negative paths, and event-driven flows.

## Practical Guidance for Future Tests

When adding new WindowManager APIs:

1. Add wrapper test cases first
- Confirm method registration and Invoke forwarding.
- Verify backend error propagation.

2. Add implementation test cases next
- Execute real implementation method.
- Mock only external dependency call points.
- Add both positive and negative expectations.

3. Cover asynchronous or callback-driven behavior
- Use promises/futures or equivalent synchronization.
- Assert notification methods are invoked with expected payloads.

4. Keep expectation-driven assertions
- Prefer EXPECT_CALL before invoking API.
- Include Times(0) assertions on paths where calls must not occur.

This keeps the test suite robust, readable, and aligned with current L1 design for WindowManager.
