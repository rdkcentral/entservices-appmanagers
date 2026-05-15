# EntServices-AppManagers Plugin Spec - AppManager

## Overview
AppManager is the primary app lifecycle facade in EntServices-AppManagers. It exposes app discovery and launch-oriented APIs and coordinates state/event propagation with lifecycle and package subsystems.

## Description
This plugin handles app-facing orchestration operations and emits app lifecycle notifications. It owns app metadata access, dispatch/event glue, and coordination points with lifecycle transitions.

## Requirements
### Requirement: AppManager provides app lifecycle entry points
AppManager MUST expose stable operations for app discovery, launch coordination, and lifecycle event dispatch across supported clients.

#### Scenario: App discovery and metadata retrieval
- **WHEN** a client queries installed applications
- **THEN** AppManager returns metadata from the internal app-info layer with deterministic mapping

### Requirement: AppManager dispatches lifecycle notifications
AppManager MUST publish app lifecycle notifications for install/uninstall and runtime state changes through registered listeners.

#### Scenario: Lifecycle event propagation
- **WHEN** an app changes lifecycle state
- **THEN** AppManager emits the corresponding notification event to registered listeners

### Requirement: AppManager integrates with lifecycle subsystem
AppManager SHALL coordinate with lifecycle handlers for launch and unload sequencing without violating state machine constraints.

#### Scenario: Launch request delegation
- **WHEN** a launch request is received
- **THEN** AppManager delegates sequencing to lifecycle/runtime handlers and returns consistent status

## Architecture / Design (if applicable)
- Uses AppInfo/AppInfoManager for metadata persistence and lookup.
- Bridges API-facing methods to lifecycle connectors and telemetry reporting.

## External Interfaces (if applicable)
- AppManager plugin API registration (`Exchange::JAppManager::Register`, `Unregister`)
- App lifecycle notifications (`OnAppInstalled`, `OnAppUninstalled`, `OnAppLifecycleStateChanged`)

## Performance (if applicable)
- App metadata lookup and dispatch paths SHOULD remain low-latency for interactive launch flows.

## Security (if applicable)
- AppManager MUST avoid exposing unauthorized metadata and must route only validated lifecycle operations.

## Versioning & Compatibility (if applicable)
- API behavior MUST remain backward compatible for existing app discovery and event consumers in minor revisions.

## Conformance Testing & Validation (if applicable)
- Validate registration, dispatch, and event emission correctness using component tests and lifecycle integration tests.

## Covered Code
- entservices-appmanagers/AppManager/AppManager.cpp:
    - AppManager::Initialize
    - AppManager::Deinitialize
    - AppManager::Information
- entservices-appmanagers/AppManager/AppManagerImplementation.cpp:
    - WPEFramework::Plugin::AppManagerImplementation::Configure
    - WPEFramework::Plugin::AppManagerImplementation::Dispatch
    - WPEFramework::Plugin::AppManagerImplementation::dispatchEvent
    - WPEFramework::Plugin::AppManagerImplementation::handleOnAppLifecycleStateChanged
- entservices-appmanagers/AppManager/AppInfoManager.cpp:
    - AppInfoManager::get
    - AppInfoManager::update
    - AppInfoManager::upsert

---

## Open Queries
- Should AppManager enforce stricter launch-rate throttling for burst launch requests?

## References
- entservices-appmanagers/AppManager/AppManager.cpp
- entservices-appmanagers/AppManager/AppManagerImplementation.cpp
- entservices-appmanagers/AppManager/AppInfoManager.cpp
- openspec/specs/entservices-apis/spec.md

## Change History
- [2026-05-03] - openspec-explore - Added per-plugin AppManager specification.
