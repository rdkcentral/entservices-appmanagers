# Capability: Preinstall Manager

## Requirements

### Requirement: Discover preinstalled packages from configured paths
The system SHALL scan configured preinstall directories and discover package candidates.

#### Scenario: Scan configured directory
Given one or more preinstall directories are configured
When preinstall scan runs
Then package candidates in those directories are discovered for evaluation

#### Scenario: Configured directory is inaccessible
Given preinstall directory exists in configuration
And filesystem access fails for that directory
When scan runs
Then scan records failure for that directory and continues with remaining directories when possible

### Requirement: Trigger installation of discovered packages
The system SHALL request package installation for discovered preinstall packages according to preinstall policy.

#### Scenario: Install discovered package
Given a discovered package candidate is eligible for install
When preinstall execution runs
Then the system invokes package installation for that candidate

#### Scenario: Install invocation fails
Given package candidate passes discovery checks
And package installation API returns failure
When preinstall execution processes that candidate
Then failure is reported with candidate identity and reason

### Requirement: Respect force/non-force install behavior
The system SHALL support force install mode and non-force mode with version/state checks.

#### Scenario: Non-force skips already installed version
Given a package version is already installed
And preinstall runs in non-force mode
When candidate evaluation occurs
Then installation is skipped for that package/version

#### Scenario: Force mode reinstalls matching version
Given package version is already installed
And preinstall runs in force mode
When candidate evaluation occurs
Then installation is attempted despite existing version

### Requirement: Expose preinstall execution entry point
The system SHALL expose an operation to trigger preinstall processing on demand.

#### Scenario: Start preinstall request
Given preinstall manager is initialized
When start preinstall is requested
Then scan and install workflow is executed and completion status is returned

#### Scenario: Start preinstall while previous run active
Given preinstall execution is already in progress
When another start request is received
Then system returns deterministic busy or coalesced response

### Requirement: Support notification sink registration
The system SHALL support registering and unregistering notification sinks for preinstall events.

#### Scenario: Notification sink registration
Given a client provides a valid notification sink
When register is requested
Then the sink is added for subsequent preinstall notifications

#### Scenario: Duplicate sink registration
Given same notification sink is already registered
When register is requested again
Then system responds with idempotent success or duplicate failure consistently

### Requirement: Use standardized failure taxonomy
The system SHALL classify operation failures using canonical categories: INVALID_ARGUMENT, NOT_FOUND, INVALID_STATE, DEPENDENCY_UNAVAILABLE, UNSUPPORTED_OPERATION, TIMEOUT, PERMISSION_DENIED, TRANSIENT_FAILURE, and INTERNAL_ERROR.

#### Scenario: Consistent category mapping
Given a preinstall operation fails
When failure is returned
Then one canonical taxonomy category is included
And equivalent failures map consistently across preinstall operations

#### Scenario: Preserve package-manager detail
Given package manager API returns native failure detail
When preinstall propagates failure
Then canonical category is returned
And native detail is preserved as supplemental context
