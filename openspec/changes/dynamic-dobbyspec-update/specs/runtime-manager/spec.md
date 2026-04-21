## MODIFIED Requirements

### Requirement: Run applications in managed runtime containers
The system SHALL start applications in isolated runtime environments using provided runtime configuration, and SHALL apply any registered per-instance Dobby spec override after base spec generation before starting the container.

#### Scenario: Start runtime instance
Given a valid app instance id and runtime configuration
When run is requested
Then the system starts the runtime container
And records runtime context for that app instance

#### Scenario: Run rejected for invalid runtime input
Given required runtime input is missing or malformed
When run is requested
Then the system returns invalid-parameter failure
And no container start is attempted

#### Scenario: Run fails when dependency precheck fails
Given display or storage dependency is required for startup
And dependency precheck fails
When run is requested
Then the system returns dependency failure
And runtime context is not marked as running

#### Scenario: Registered Dobby spec override is applied before container start
Given a Dobby spec override has been registered for the app instance id
When run is requested
Then the system generates the base Dobby spec
And applies the override using the configured merge strategy
And starts the container with the merged spec
