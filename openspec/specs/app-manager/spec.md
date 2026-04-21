# Capability: App Manager

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
