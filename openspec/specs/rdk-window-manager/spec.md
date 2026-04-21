# Capability: RDK Window Manager

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
