# Runtime Manager

## Overview
Specification for the Runtime Manager capability, which runs applications in isolated runtime containers (via Dobby/OCI and/or Rialto), provides suspend/resume/hibernate/wake lifecycle controls, handles terminate and force-kill paths, and coordinates with display and storage dependencies during container startup.

## Description
The Runtime Manager starts applications in isolated OCI containers using Dobby and/or the Rialto backend. It generates container specs (`DobbySpecGenerator`), manages runtime context per app instance, handles suspend/resume/hibernate/wake via backend-specific controls, and provides terminate and kill paths. During startup it coordinates with the Window Manager and storage layer to satisfy runtime dependencies. It listens to container lifecycle events (`DobbyEventListener`) and normalises all failures to the canonical failure taxonomy.

## Requirements

### Requirement: Run applications in managed runtime containers
The system SHALL start applications in isolated runtime environments using provided runtime configuration.

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

### Requirement: Support runtime pause and resume
The system SHALL provide suspend and resume operations for running app instances where supported by the runtime backend.

#### Scenario: Suspend and resume
Given an app instance is running
When suspend is requested and later resume is requested
Then the runtime pauses the app instance and later returns it to running state

#### Scenario: Suspend unknown app instance
Given no runtime context exists for the requested app instance
When suspend is requested
Then the system returns not-found failure

### Requirement: Support hibernate and wake flows
The system SHALL support hibernation/checkpoint and wake/restore flows for eligible app instances.

#### Scenario: Hibernate and wake
Given an app instance supports runtime checkpointing
When hibernate is requested and subsequently wake is requested
Then the system checkpoints state and later restores the app instance

#### Scenario: Hibernate rejected when feature unavailable
Given runtime backend does not support checkpoint operations
When hibernate is requested
Then the system returns unsupported-operation failure

### Requirement: Provide terminate and kill controls
The system SHALL support graceful termination and force kill for app instances.

#### Scenario: Graceful termination
Given an app instance is running
When terminate is requested
Then the system performs graceful shutdown and cleanup for the instance

#### Scenario: Terminate timeout escalates to failure
Given graceful terminate does not complete within configured timeout
When terminate processing completes
Then the system returns failure status

### Requirement: Integrate display/runtime dependencies during run
The system SHALL coordinate with dependent services needed for app execution, such as display and storage-related runtime inputs.

#### Scenario: Runtime dependency coordination
Given runtime startup requires display and path configuration
When run is executed
Then dependency calls are issued before runtime activation
And startup fails with a clear error if dependencies cannot be satisfied

### Requirement: Use standardized failure taxonomy
The system SHALL classify operation failures using canonical categories: INVALID_ARGUMENT, NOT_FOUND, INVALID_STATE, DEPENDENCY_UNAVAILABLE, UNSUPPORTED_OPERATION, TIMEOUT, PERMISSION_DENIED, TRANSIENT_FAILURE, and INTERNAL_ERROR.

#### Scenario: Consistent category mapping
Given a runtime operation fails
When error is reported to caller
Then one canonical failure category is returned
And category usage remains consistent across runtime APIs

#### Scenario: Preserve dependency failure detail
Given runtime backend or dependency returns native error data
When failure is returned
Then canonical category is included
And native detail is retained as supplemental context

## Architecture / Design
Design is documented in [openspec/specs/runtime-manager/design.md](design.md). Key components:
- **Plugin facade** (`RuntimeManager.cpp/.h`) — Thunder RPC registration.
- **Implementation** (`RuntimeManagerImplementation.cpp/.h`) — container lifecycle orchestration.
- **DobbySpecGenerator** (`DobbySpecGenerator.cpp/.h`) — generates OCI/Dobby container specifications.
- **DobbyEventListener** (`DobbyEventListener.cpp/.h`) — receives container lifecycle events.
- **WindowManagerConnector** (`WindowManagerConnector.cpp/.h`) — integrates with RDKWindowManager.
- **RialtoConnector** (`RialtoConnector.cpp/.h`) — Rialto backend integration.
- **Gateway submodules** (`Gateway/ContainerUtils`, `Gateway/NetFilter`, `Gateway/WebInspector`) — networking, inspection, and utility operations.
- **Ralf submodules** (`ralf/RalfPackageBuilder`, `ralf/RalfOCIConfigGenerator`, `ralf/RalfSupport`) — OCI config generation utilities.

## External Interfaces
_The RuntimeManager plugin exposes the following Thunder JSON-RPC methods (confirm against generated stubs):_
- `run(appInstanceId, runtimeConfig)` → success/failure
- `suspend(appInstanceId)` → success/failure
- `resume(appInstanceId)` → success/failure
- `hibernate(appInstanceId)` → success/failure
- `wake(appInstanceId)` → success/failure
- `terminate(appInstanceId)` → success/failure
- `kill(appInstanceId)` → success/failure
- **Events:** `onContainerStarted(appInstanceId)`, `onContainerStopped(appInstanceId, reason)`

_Confirm exact parameter names and types against the generated Thunder interface definition._

## Performance
_Not yet defined — add container start latency and suspend/resume latency targets when SLAs are established._

## Security
- Runtime containers are isolated via Dobby OCI sandboxing; container spec is generated and validated before launch.
- Network filtering is applied via Gateway/NetFilter to restrict container network access.
- AI configuration inputs are validated before inclusion in container specs.
- _Threat model not yet formal — container escape and privilege escalation scenarios should be reviewed._

## Versioning & Compatibility
_Not yet defined — add versioning scheme and backward-compatibility guarantees when the API is stabilised._

## Conformance Testing & Validation
- **L0 tests:** [Tests/L0Tests/RuntimeManager/](../../../Tests/L0Tests/RuntimeManager/)
- **L1 tests:** [Tests/L1Tests/tests/test_RunTimeManager.cpp](../../../Tests/L1Tests/tests/test_RunTimeManager.cpp)
- _Test documentation not yet written — add as follow-up._

## Covered Code
- RuntimeManager/RuntimeManager.cpp — `RuntimeManager` (plugin facade)
- RuntimeManager/RuntimeManager.h — `RuntimeManager`
- RuntimeManager/RuntimeManagerImplementation.cpp — `RuntimeManagerImplementation`
- RuntimeManager/RuntimeManagerImplementation.h — `RuntimeManagerImplementation`
- RuntimeManager/DobbySpecGenerator.cpp — `DobbySpecGenerator`
- RuntimeManager/DobbyEventListener.cpp — `DobbyEventListener`
- RuntimeManager/WindowManagerConnector.cpp — `WindowManagerConnector`
- RuntimeManager/RialtoConnector.cpp — `RialtoConnector`
- RuntimeManager/AIConfiguration.cpp — `AIConfiguration`
- RuntimeManager/UserIdManager.cpp — `UserIdManager`
- RuntimeManager/Gateway/ContainerUtils.cpp — container utilities
- RuntimeManager/Gateway/NetFilter.cpp — network filtering
- RuntimeManager/Gateway/WebInspector.cpp — web inspector support
- RuntimeManager/ralf/RalfPackageBuilder.cpp — OCI package building
- RuntimeManager/ralf/RalfOCIConfigGenerator.cpp — OCI config generation
- RuntimeManager/Module.cpp, RuntimeManager/Module.h — Thunder module registration

---

## Open Queries
- Container escape and privilege escalation threat scenarios should be formally reviewed.
- Performance targets (container start, suspend/resume) not yet defined.
- Formal versioning scheme not yet established.
- AIConfiguration input validation rules need to be explicitly specified.

## References
- [openspec/specs/runtime-manager/design.md](design.md)
- [openspec/specs/runtime-manager/requirements.md](requirements.md)

## Change History
- [2026-04-23] - openspec-templater - Restructured to match spec template.
