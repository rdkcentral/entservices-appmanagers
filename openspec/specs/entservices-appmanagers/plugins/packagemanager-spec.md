# EntServices-AppManagers Plugin Spec - PackageManager

## Overview
PackageManager is responsible for package acquisition, installation lifecycle, metadata extraction handoff, and runtime configuration assembly for app launch.

## Description
This plugin manages download/install/uninstall/lock flows and converts package metadata into runtime-facing configuration. It also coordinates storage interactions and emits installation status telemetry/events.

## Requirements
### Requirement: PackageManager controls install and uninstall workflows
PackageManager MUST implement install/uninstall operations with deterministic status reporting and cleanup semantics.

#### Scenario: Install workflow progression
- **WHEN** an install request is received
- **THEN** PackageManager performs acquisition, validation, extraction, registration, and status notification steps in sequence

### Requirement: PackageManager assembles runtime configuration
PackageManager MUST map package metadata into runtime configuration, including normalized `RuntimeConfig.capabilities` and existing runtime fields.

#### Scenario: Runtime configuration generation
- **WHEN** package metadata is available for an app version
- **THEN** PackageManager generates runtime configuration with consistent field mapping and capabilities propagation

### Requirement: PackageManager supports operational controls
PackageManager SHALL support pause/resume/cancel/progress/rate-limit controls for active download operations.

#### Scenario: Download control request
- **WHEN** a client issues pause/resume/cancel for an active job
- **THEN** PackageManager updates execution state and returns current status coherently

## Architecture / Design (if applicable)
- Core implementation in `PackageManagerImplementation` with telemetry and HTTP/storage helpers.
- Runtime config copy/assembly paths bridge package metadata and launch-time consumers.

## External Interfaces (if applicable)
- Package installation status events (`Exchange::JPackageInstaller::Event::OnAppInstallationStatus`)
- Package management API methods (Install, Uninstall, Lock, Unlock, Download controls)

## Performance (if applicable)
- Download and install orchestration SHOULD support concurrent operations without starving status/reporting paths.

## Security (if applicable)
- Package operations MUST enforce validation boundaries and avoid unsafe metadata propagation to runtime fields.

## Versioning & Compatibility (if applicable)
- PackageManager API behavior SHALL preserve compatibility for existing install/uninstall and runtime-config consumers.

## Conformance Testing & Validation (if applicable)
- Validate installation workflows, runtime-config assembly, and download controls with component and integration tests.

## Covered Code
- entservices-appmanagers/PackageManager/PackageManager.cpp:
    - PackageManager::Initialize
    - PackageManager::Deinitialize
- entservices-appmanagers/PackageManager/PackageManagerImplementation.cpp:
    - PackageManagerImplementation::Install
    - PackageManagerImplementation::Uninstall
    - PackageManagerImplementation::Lock
    - PackageManagerImplementation::Unlock
    - PackageManagerImplementation::Download
    - PackageManagerImplementation::Pause
    - PackageManagerImplementation::Resume
    - PackageManagerImplementation::Cancel
    - PackageManagerImplementation::RateLimit
    - PackageManagerImplementation::getRuntimeConfig
- entservices-appmanagers/PackageManager/HttpClient.cpp:
    - HttpClient::download

---

## Open Queries
- Should runtime configuration serialization emit structured validation diagnostics for malformed package metadata?

## References
- entservices-appmanagers/PackageManager/PackageManager.cpp
- entservices-appmanagers/PackageManager/PackageManagerImplementation.cpp
- entservices-appmanagers/PackageManager/HttpClient.cpp
- openspec/specs/libpackage-sky/spec.md

## Change History
- [2026-05-03] - openspec-explore - Added per-plugin PackageManager specification.
