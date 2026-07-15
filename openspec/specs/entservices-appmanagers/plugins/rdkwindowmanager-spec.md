# EntServices-AppManagers Plugin Spec - RDKWindowManager

## Overview
RDKWindowManager plugin in EntServices-AppManagers provides application-facing window/display operations and bridges display lifecycle controls to compositor/window-manager subsystems.

## Description
This plugin exposes display creation and client/surface operations used by app and lifecycle flows. It is a dependent integration surface for RuntimeManager/LifecycleManager orchestration even when runtime metadata changes do not directly modify plugin source.

## Requirements
### Requirement: RDKWindowManager exposes display management APIs
RDKWindowManager MUST provide display/session operations including display creation and client queries.

#### Scenario: Display creation request
- **WHEN** a client requests display creation
- **THEN** plugin creates display resources and returns deterministic result metadata

### Requirement: RDKWindowManager supports surface/client control operations
RDKWindowManager SHALL support surface/client visibility and composition-affecting controls used by application orchestration.

#### Scenario: Visibility and opacity control
- **WHEN** a control request updates visibility or opacity
- **THEN** plugin applies change and reports operation status coherently

### Requirement: RDKWindowManager compatibility is maintained with lifecycle/runtime orchestration
RDKWindowManager MUST remain behaviorally compatible with RuntimeManager and LifecycleManager-managed app launch/display sequencing.

#### Scenario: Runtime/lifecycle-driven display interaction
- **WHEN** app lifecycle transitions trigger display-related operations
- **THEN** plugin behavior remains consistent with lifecycle/runtime expectations

## Architecture / Design (if applicable)
- Plugin registration layer maps API requests to implementation handlers.
- Implementation routes calls to compositor/client management paths and telemetry hooks.

## External Interfaces (if applicable)
- RDK window manager API registration (`Exchange::JRDKWindowManager::Register`, `Unregister`)
- Client/display operations (`CreateDisplay`, `GetClients`, `SetOpacity`, `SetVisibility`)

## Performance (if applicable)
- Display and client control operations SHOULD execute with interactive latency for UI workflows.

## Security (if applicable)
- Window/display operations MUST enforce caller authorization and client isolation rules.

## Versioning & Compatibility (if applicable)
- API compatibility SHALL be preserved for established display/surface operation contracts.

## Conformance Testing & Validation (if applicable)
- Validate display creation, client queries, and control operations with plugin-level and integration tests.

## Covered Code
- entservices-appmanagers/RDKWindowManager/RDKWindowManager.cpp:
    - RDKWindowManager::Initialize
    - RDKWindowManager::Deinitialize
    - RDKWindowManager::Information
- entservices-appmanagers/RDKWindowManager/RDKWindowManagerImplementation.cpp:
    - WPEFramework::Plugin::RDKWindowManagerImplementation::CreateDisplay
    - WPEFramework::Plugin::RDKWindowManagerImplementation::GetClients
    - WPEFramework::Plugin::RDKWindowManagerImplementation::SetOpacity
    - WPEFramework::Plugin::RDKWindowManagerImplementation::SetVisibility
- entservices-appmanagers/RDKWindowManager/RDKWindowManagerTelemetryReporting.cpp:
    - RDKWindowManagerTelemetryReporting::recordCreateDisplayTelemetry

---

## Open Queries
- Should RDKWindowManager plugin expose explicit compatibility assertions for RuntimeManager capability-driven launch modes?

## References
- entservices-appmanagers/RDKWindowManager/RDKWindowManager.cpp
- entservices-appmanagers/RDKWindowManager/RDKWindowManagerImplementation.cpp
- entservices-appmanagers/RDKWindowManager/RDKWindowManagerTelemetryReporting.cpp

## Change History
- [2026-05-03] - openspec-explore - Added per-plugin RDKWindowManager specification.
