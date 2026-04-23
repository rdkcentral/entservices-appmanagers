# RDK Window Manager

## Overview
Specification for the RDK Window Manager capability, which manages display/window resource allocation, focus and visibility control, key intercept and listener registration, inactivity reporting, and Z-order management for application instances via a compositor backend.

## Description
The RDK Window Manager creates and manages display/window resources for application instances using the underlying compositor (rdkwindowmanager/compositorcontroller). It exposes focus, visibility, key intercept, key listener, inactivity reporting, and Z-order controls to clients via a Thunder plugin facade. All compositor interactions are abstracted so that failures from the backend are normalised to the canonical failure taxonomy.

## Requirements

### Requirement: Create and manage application displays
The system SHALL create and manage display/window resources for application instances.

#### Scenario: Create display
Given display parameters for an app instance
When display creation is requested
Then the system allocates compositor/display resources
And returns display identity information

#### Scenario: Display creation fails for invalid parameters
Given display request payload is malformed or incomplete
When display creation is requested
Then the system returns invalid-parameter failure

#### Scenario: Display creation fails when compositor is unavailable
Given compositor backend is not reachable
When display creation is requested
Then the system returns dependency-unavailable failure

### Requirement: Control focus and visibility
The system SHALL support focus and visibility management for registered app clients.

#### Scenario: Set focus
Given multiple app clients are present
When focus is set to a specific client
Then the targeted client becomes focused according to compositor policy

#### Scenario: Set focus for unknown client
Given requested client is not known to window manager
When focus is requested
Then the system returns not-found failure

### Requirement: Manage key interception and key listeners
The system SHALL allow key intercept registration/removal and key listener registration/removal for clients.

#### Scenario: Register key intercept
Given a client provides a valid key intercept configuration
When add key intercept is requested
Then the intercept is registered and applied to matching input events

#### Scenario: Duplicate key intercept registration
Given a matching key intercept is already registered for the same client
When the same intercept is added again
Then the system returns deterministic behavior as either idempotent success or duplicate failure

### Requirement: Support inactivity reporting controls
The system SHALL support enabling/disabling inactivity reporting and configuring inactivity interval.

#### Scenario: Inactivity timer configured
Given inactivity reporting is enabled
When a new inactivity interval is configured
Then inactivity events follow the configured interval

#### Scenario: Invalid inactivity interval rejected
Given interval value is outside accepted bounds
When interval configuration is requested
Then the system returns invalid-parameter failure

### Requirement: Support window stacking controls
The system SHALL support setting and retrieving Z-order for app instances.

#### Scenario: Set z-order
Given an app instance is known to window manager
When z-order is set for that app instance
Then the system stores and applies the requested stacking order

### Requirement: Use standardized failure taxonomy
The system SHALL classify operation failures using canonical categories: INVALID_ARGUMENT, NOT_FOUND, INVALID_STATE, DEPENDENCY_UNAVAILABLE, UNSUPPORTED_OPERATION, TIMEOUT, PERMISSION_DENIED, TRANSIENT_FAILURE, and INTERNAL_ERROR.

#### Scenario: Consistent category mapping
Given a window management operation fails
When failure is returned
Then one canonical taxonomy category is included
And repeated equivalent failures map to the same category

#### Scenario: Preserve backend detail
Given compositor/backend returns native failure detail
When failure is propagated
Then canonical category is returned
And backend detail is preserved as supplemental context

## Architecture / Design
Design is documented in [openspec/specs/rdk-window-manager/design.md](design.md). Key components:
- **Plugin facade** (`RDKWindowManager.cpp/.h`) — Thunder RPC registration.
- **Implementation** (`RDKWindowManagerImplementation.cpp/.h`) — window/display lifecycle, focus, key, and Z-order management.
- **Telemetry bridge** (`RDKWindowManagerTelemetryReporting.cpp/.h`) — timing markers.

## External Interfaces
_The RDKWindowManager plugin exposes the following Thunder JSON-RPC methods (confirm against generated stubs):_
- `createDisplay(appId, displayParams)` → `{displayId}` or failure
- `destroyDisplay(displayId)` → success/failure
- `setFocus(clientId)` → success/failure
- `setVisibility(clientId, visible)` → success/failure
- `addKeyIntercept(clientId, keyConfig)` → success/failure
- `removeKeyIntercept(clientId, keyConfig)` → success/failure
- `addKeyListener(clientId, keyConfig)` → success/failure
- `removeKeyListener(clientId, keyConfig)` → success/failure
- `setInactivityInterval(intervalMs)` → success/failure
- `enableInactivityReporting(enable)` → success/failure
- `setZOrder(appId, zOrder)` → success/failure
- **Events:** `onInactivity()`, `onKeyInput(keyEvent)`

_Confirm exact parameter names and types against the generated Thunder interface definition._

## Performance
_Not yet defined — add display creation latency and focus-switch latency targets when SLAs are established._

## Security
- Compositor resource access is gated through the Thunder framework access controls.
- Key intercept registrations from untrusted callers should be validated — policy not yet fully defined.
- _Threat model partially addressed in design.md Security and Safety section — formalise as follow-up._

## Versioning & Compatibility
_Not yet defined — add versioning scheme and backward-compatibility guarantees when the API is stabilised._

## Conformance Testing & Validation
- **L0 tests:** [Tests/L0Tests/RDKWindowManager/](../../../Tests/L0Tests/RDKWindowManager/)
- **L1 tests:** [Tests/L1Tests/tests/test_RDKWindowManager.cpp](../../../Tests/L1Tests/tests/test_RDKWindowManager.cpp)
- _Test documentation not yet written — add as follow-up._

## Covered Code
- RDKWindowManager/RDKWindowManager.cpp — `RDKWindowManager` (plugin facade)
- RDKWindowManager/RDKWindowManager.h — `RDKWindowManager`
- RDKWindowManager/RDKWindowManagerImplementation.cpp — `RDKWindowManagerImplementation`
- RDKWindowManager/RDKWindowManagerImplementation.h — `RDKWindowManagerImplementation`
- RDKWindowManager/RDKWindowManagerTelemetryReporting.cpp — `RDKWindowManagerTelemetryReporting`
- RDKWindowManager/Module.cpp, RDKWindowManager/Module.h — Thunder module registration

---

## Open Queries
- Key intercept authorization policy for untrusted callers needs to be specified.
- Display parameter schema (resolution, layer, etc.) not yet formally defined.
- Performance targets not yet defined.
- Formal versioning scheme not yet established.

## References
- [openspec/specs/rdk-window-manager/design.md](design.md)
- [openspec/specs/rdk-window-manager/requirements.md](requirements.md)

## Change History
- [2026-04-23] - openspec-templater - Restructured to match spec template.
