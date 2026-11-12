## Purpose

Enforces lifecycle state timers for PAUSED and SUSPENDED states to prevent inactive applications from consuming system resources indefinitely, protecting platform stability.

## ADDED Requirements

### Requirement: Enforce PAUSED state timeout
The system SHALL enforce a timeout for applications in the PAUSED state. When the timeout expires, the system SHALL check whether the application supports the SUSPENDED state. If supported, the system SHALL request a transition to SUSPENDED. If not supported, the system SHALL request termination of the application.

#### Scenario: PAUSED timer expires for app supporting SUSPENDED
- **WHEN** an application enters the PAUSED state
- **THEN** the system starts a timer for the configured `pausedToSuspendedTimeout` (default 60 seconds)
- **WHEN** the timer expires
- **THEN** the system queries the application's supported lifecycle states
- **THEN** if SUSPENDED is supported, the system requests a SUSPENDED transition via LifecycleManager

#### Scenario: PAUSED timer expires for app not supporting SUSPENDED
- **WHEN** an application enters the PAUSED state
- **THEN** the system starts a timer for the configured `pausedToSuspendedTimeout`
- **WHEN** the timer expires
- **THEN** the system queries the application's supported lifecycle states
- **THEN** if SUSPENDED is not supported, the system requests termination of the application via AppManager

#### Scenario: Application leaves PAUSED before timeout expires
- **WHEN** an application in PAUSED transitions to ACTIVE, SUSPENDED, HIBERNATED, or TERMINATED
- **THEN** the system cancels the PAUSED timer for that application

### Requirement: Enforce SUSPENDED GPU grace period
The system SHALL enforce a GPU grace period when an application enters the SUSPENDED state. During the grace period, the application SHALL have time to reduce its GPU memory usage. After the grace period expires, if GPU memory usage exceeds the configured threshold, the system SHALL request termination of the application.

#### Scenario: GPU memory reduced below threshold within grace period
- **WHEN** an application enters the SUSPENDED state
- **THEN** the system starts a timer for `suspendedToHibernatedTimeout` (default 15 seconds)
- **WHEN** the timer expires
- **THEN** the system queries the application's GPU memory usage via RuntimeManager/Dobby
- **THEN** if GPU memory usage is at or below `maxGPUallowedInSuspended` (default 20 MiB), the system proceeds to hibernation condition checks

#### Scenario: GPU memory still above threshold after grace period
- **WHEN** an application enters the SUSPENDED state
- **THEN** the system starts a timer for `suspendedToHibernatedTimeout`
- **WHEN** the timer expires
- **THEN** the system queries the application's GPU memory usage
- **THEN** if GPU memory usage exceeds `maxGPUallowedInSuspended`, the system requests termination of the application via AppManager

### Requirement: Check hibernation conditions after SUSPENDED timeout
The system SHALL check hibernation conditions after the SUSPENDED GPU grace period expires and GPU memory is within limits. The conditions SHALL include: device supports hibernation, application supports hibernation, and application memory floor is below the configured limit. If all conditions are met, the system SHALL request a transition to HIBERNATED. If any condition is not met, the system SHALL request termination of the application.

#### Scenario: All hibernation conditions met
- **WHEN** an application is in SUSPENDED and GPU memory is within limits after the grace period
- **THEN** the system checks if the device supports hibernation
- **THEN** the system checks if the application supports hibernation (via lifecycle capabilities)
- **THEN** the system queries the application's `hard_non_reclaimable_floor` via RuntimeManager/Dobby
- **THEN** if the memory floor is at or below `maxSuspendedSystemMemory`, the system requests a HIBERNATED transition via LifecycleManager

#### Scenario: Hibernation not supported by device
- **WHEN** an application is in SUSPENDED and GPU memory is within limits after the grace period
- **THEN** if the device does not support hibernation, the system requests termination of the application via AppManager

#### Scenario: Application does not support hibernation
- **WHEN** an application is in SUSPENDED and GPU memory is within limits after the grace period
- **THEN** if the device supports hibernation but the application does not, the system requests termination of the application via AppManager

#### Scenario: Memory floor exceeds limit
- **WHEN** an application is in SUSPENDED and GPU memory is within limits after the grace period
- **THEN** if the application's `hard_non_reclaimable_floor` exceeds `maxSuspendedSystemMemory`, the system requests termination of the application via AppManager

### Requirement: Cancel SUSPENDED timer on state change
The system SHALL cancel the SUSPENDED timer when an application transitions out of the SUSPENDED state to ACTIVE, PAUSED, HIBERNATED, or TERMINATED.

#### Scenario: Application leaves SUSPENDED before timer expires
- **WHEN** an application in SUSPENDED transitions to ACTIVE, PAUSED, HIBERNATED, or TERMINATED
- **THEN** the system cancels the SUSPENDED timer for that application

### Requirement: Configure PAUSED timeout
The system SHALL provide a configurable global timeout for the PAUSED state, with a default value of 60 seconds.

#### Scenario: Default PAUSED timeout
- **WHEN** the system starts
- **THEN** the PAUSED timeout is set to 60 seconds unless explicitly configured otherwise

#### Scenario: Custom PAUSED timeout
- **WHEN** an administrator configures `pausedToSuspendedTimeout` in the ResourceMonitor configuration
- **THEN** the system uses the configured value for PAUSED timer enforcement

### Requirement: Handle lifecycle capability unavailability
The system SHALL treat applications with unavailable lifecycle capabilities as not supporting SUSPENDED or HIBERNATED. When a PAUSED timer expires for such an application, the system SHALL request termination.

#### Scenario: Lifecycle capabilities unavailable at PAUSED timeout
- **WHEN** the PAUSED timer expires for an application
- **THEN** if lifecycle capabilities are unavailable (status UNAVAILABLE from RDKEMW-28030)
- **THEN** the system requests termination of the application via AppManager

#### Scenario: Lifecycle capabilities unavailable at hibernation check
- **WHEN** the system checks hibernation conditions for a SUSPENDED application
- **THEN** if lifecycle capabilities are unavailable
- **THEN** the system treats the application as not supporting hibernation and requests termination

### Requirement: Identify non-app workloads for termination
The system SHALL identify non-app workloads (e.g., system services) using AppManager's `LoadedAppInfo.type` and request termination when their PAUSED timer expires, regardless of lifecycle capability support.

#### Scenario: Non-app workload PAUSED timer expires
- **WHEN** a non-app workload (e.g., system service) enters the PAUSED state
- **THEN** the system starts a PAUSED timer
- **WHEN** the timer expires
- **THEN** the system identifies the workload as a non-app via `LoadedAppInfo.type`
- **THEN** the system requests termination of the workload via AppManager

### Requirement: Subscribe to lifecycle state changes
AppManager SHALL detect lifecycle state changes internally via its own notification handlers to determine when applications enter PAUSED or SUSPENDED states and when they transition out of these states.

#### Scenario: Application enters PAUSED
- **WHEN** AppManager detects an application enters PAUSED state
- **THEN** AppManager's `AppStateTransitionManager` starts a PAUSED timer for the application

#### Scenario: Application enters SUSPENDED
- **WHEN** AppManager detects an application enters SUSPENDED state
- **THEN** AppManager's `AppStateTransitionManager` starts a SUSPENDED timer (GPU grace period) for the application

#### Scenario: Application transitions away from PAUSED or SUSPENDED
- **WHEN** AppManager detects an application transitions away from PAUSED or SUSPENDED
- **THEN** AppManager's `AppStateTransitionManager` cancels the corresponding timer for the application
