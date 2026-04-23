# App Manager

## Overview
Specification for the App Manager capability, which orchestrates application lifecycle by coordinating package, lifecycle, and runtime manager dependencies. It manages loading, launching, closing, and terminating applications, maintains loaded application state, emits lifecycle notifications, and exposes app data management operations.

## Description
The App Manager is the primary orchestrator for application lifecycle on the platform. It accepts client requests to launch, close, terminate, or kill applications identified by app id. For each operation it coordinates with the Package Manager (lock/unlock, metadata), Lifecycle Manager (spawn/target-state/unload), and Storage Manager (clear-data). It tracks per-app context including lifecycle state, emits state-change notifications to subscribers, and normalises all dependency failures into a canonical failure taxonomy before returning responses to callers.

## Requirements

### Requirement: Launch applications through lifecycle orchestration
The system SHALL launch an installed application by app identifier and intent, and SHALL coordinate package, lifecycle, and runtime dependencies before reporting success.

#### Scenario: Launch request succeeds
Given an installed application package is available
And lifecycle and runtime dependencies are reachable
When a client submits a launch request with app id and launch intent
Then the system returns success with an app instance identifier
And the app enters lifecycle progression toward active or configured target state

#### Scenario: Launch fails when package is unavailable
Given the requested app package is not installed
When a client submits a launch request
Then the system returns a failure with a package-not-installed reason
And no app instance is created

#### Scenario: Launch fails when dependent service is unavailable
Given package data can be resolved
And lifecycle or runtime dependency is unavailable
When launch is requested
Then the system returns a failure indicating dependency unavailability
And partial launch state is rolled back

### Requirement: Validate launch and control request parameters
The system SHALL reject malformed or incomplete app control requests with explicit failure status.

#### Scenario: Invalid launch request rejected
Given launch request is missing required app id
When launch is requested
Then the system returns invalid-parameter failure
And no downstream dependency calls are issued

### Requirement: Manage application shutdown paths
The system SHALL support close, terminate, and force-kill operations for loaded applications.

#### Scenario: Graceful termination
Given an application is loaded
When a client requests termination
Then the system executes orderly unload behavior
And reports the app as unloaded after completion

#### Scenario: Shutdown request for unknown app
Given no loaded app matches the requested app identity
When close, terminate, or kill is requested
Then the system returns not-found or invalid-state failure

### Requirement: Expose loaded application state
The system SHALL provide loaded application inventory including stable app identity and lifecycle state.

#### Scenario: Query loaded apps
Given one or more applications are loaded
When loaded application information is requested
Then the response includes each app identity and current lifecycle state

### Requirement: Emit lifecycle notifications
The system SHALL notify subscribers on app lifecycle state transitions with old and new state context.

#### Scenario: State transition notification
Given a subscriber is registered for app lifecycle updates
When an app transitions state
Then the subscriber receives app identity, old state, and new state

### Requirement: Handle app data operations via storage integration
The system SHALL delegate app clear-data operations to storage service and propagate result status.

#### Scenario: Clear app data success
Given an application has allocated storage
When clear app data is requested
Then the system invokes storage clear for that app
And returns success when storage clear succeeds

#### Scenario: Clear app data dependency failure
Given storage service is unavailable or clear operation fails
When clear app data is requested
Then the system returns failure with storage-related error reason

### Requirement: Use standardized failure taxonomy
The system SHALL classify operation failures using canonical categories: INVALID_ARGUMENT, NOT_FOUND, INVALID_STATE, DEPENDENCY_UNAVAILABLE, UNSUPPORTED_OPERATION, TIMEOUT, PERMISSION_DENIED, TRANSIENT_FAILURE, and INTERNAL_ERROR.

#### Scenario: Consistent category mapping
Given an operation fails for a known reason
When failure is returned to caller
Then the failure includes one canonical category from the taxonomy
And equivalent failures map to the same category across operations

#### Scenario: Preserve service-specific detail
Given a dependency returns a native error message
When failure is propagated
Then canonical category is returned
And native detail is preserved as supplemental context

## Architecture / Design
The design is documented in [openspec/specs/app-manager/design.md](design.md). Key components:
- **Plugin facade** (`AppManager.cpp/.h`) — registers and exposes the Thunder RPC surface.
- **Orchestrator** (`AppManagerImplementation.cpp/.h`) — coordinates launch/close/terminate/kill workflows and maintains per-app context.
- **Lifecycle connector** (`LifecycleInterfaceConnector.cpp/.h`) — encapsulates interactions with LifecycleManager; handles lifecycle event callbacks.
- **App info store** (`AppInfo.cpp/.h`, `AppInfoManager.cpp/.h`) — maintains in-memory app instance state.
- **Telemetry bridge** (`AppManagerTelemetryReporting.cpp/.h`) — emits launch/state-transition timing markers.

## External Interfaces
_The AppManager plugin exposes the following Thunder JSON-RPC methods (confirm against generated stubs):_
- `launch(appId, intent)` → `{instanceId}` or failure
- `close(appId)` → success/failure
- `terminate(appId)` → success/failure
- `kill(appId)` → success/failure
- `getLoadedApps()` → array of `{appId, instanceId, state}`
- `clearAppData(appId)` → success/failure
- **Events:** `onAppStateChanged(appId, instanceId, oldState, newState)`

_Confirm exact parameter names and types against the generated Thunder interface definition._

## Performance
_Not yet defined — add latency targets for launch, close, and terminate operations when SLAs are established._

## Security
- Input validation is enforced at the API boundary; unknown app references do not leak internal state.
- Force-kill operations are explicit API calls and are auditable via logging.
- No caller authentication is implemented at this layer; access control is delegated to the Thunder framework.
- _Threat model not yet formal — add as follow-up._

## Versioning & Compatibility
_Not yet defined — add versioning scheme and backward-compatibility guarantees when the API is stabilised._

## Conformance Testing & Validation
- **L1 tests:** [Tests/L1Tests/tests/test_AppManager.cpp](../../../Tests/L1Tests/tests/test_AppManager.cpp)
- **L2 tests:** [Tests/L2Tests/tests/AppManager_L2Test.cpp](../../../Tests/L2Tests/tests/AppManager_L2Test.cpp)
- _Test documentation (how to run, interpret results) not yet written — add as follow-up._

## Covered Code
- AppManager/AppManager.cpp — `AppManager` (plugin facade)
- AppManager/AppManager.h — `AppManager`
- AppManager/AppManagerImplementation.cpp — `AppManagerImplementation`
- AppManager/AppManagerImplementation.h — `AppManagerImplementation`
- AppManager/AppInfo.cpp — `AppInfo`
- AppManager/AppInfo.h — `AppInfo`
- AppManager/AppInfoManager.cpp — `AppInfoManager`
- AppManager/AppInfoManager.h — `AppInfoManager`
- AppManager/LifecycleInterfaceConnector.cpp — `LifecycleInterfaceConnector`
- AppManager/LifecycleInterfaceConnector.h — `LifecycleInterfaceConnector`
- AppManager/AppManagerTelemetryReporting.cpp — `AppManagerTelemetryReporting`
- AppManager/AppManagerTelemetryReporting.h — `AppManagerTelemetryReporting`
- AppManager/AppManagerTypes.h — shared type definitions
- AppManager/Module.cpp, AppManager/Module.h — Thunder module registration

---

## Open Queries
- External interface parameter names and types need to be verified against generated Thunder stubs.
- Performance (launch latency, close latency) targets not yet defined.
- Formal versioning scheme not yet established.
- Threat model not formalised — follow-up security review needed.

## References
- [openspec/specs/app-manager/design.md](design.md)
- [openspec/specs/app-manager/requirements.md](requirements.md)

## Change History
- [2026-04-23] - openspec-templater - Restructured to match spec template.
