# EntServices-AppManagers Plugin Spec - PreinstallManager

## Overview
PreinstallManager automates discovery and controlled installation of preloaded package sets during device initialization/maintenance windows.

## Description
This plugin coordinates preinstall package enumeration, version comparison, and staged installation dispatch through PackageManager interfaces. It emits completion and failure signals for system orchestration.

## Requirements
### Requirement: PreinstallManager discovers and evaluates preload bundles
PreinstallManager MUST scan configured preinstall directories and evaluate candidate package versions before installation.

#### Scenario: Preinstall discovery pass
- **WHEN** startup or maintenance trigger runs preinstall
- **THEN** plugin enumerates configured bundle locations and computes install candidates

### Requirement: PreinstallManager performs controlled installation sequence
PreinstallManager MUST invoke package installation in deterministic order and track per-package outcomes.

#### Scenario: Batch preinstall execution
- **WHEN** multiple preinstall candidates are selected
- **THEN** plugin installs packages in defined order and records success/failure outcomes

### Requirement: PreinstallManager publishes completion semantics
PreinstallManager SHALL emit completion events that include aggregate result and failure context.

#### Scenario: Preinstall completion event
- **WHEN** preinstall workflow terminates
- **THEN** plugin dispatches completion notification with summarized status

## Architecture / Design (if applicable)
- Singleton-like implementation coordinates package manager client object and dispatch pipeline.
- Version comparison and failure reason mapping helpers define install decision logic.

## External Interfaces (if applicable)
- Preinstall plugin configuration/dispatch APIs
- Completion event dispatch path (`sendOnPreinstallationCompleteEvent`)

## Performance (if applicable)
- Preinstall scheduling SHOULD minimize impact on foreground application launch paths.

## Security (if applicable)
- Preinstall package source validation MUST follow package manager trust and verification policy.

## Versioning & Compatibility (if applicable)
- Preinstall behavior SHALL remain compatible with established package manifest/version interpretation rules.

## Conformance Testing & Validation (if applicable)
- Validate preinstall discovery, ordering, version-selection, and completion events with targeted integration tests.

## Covered Code
- entservices-appmanagers/PreinstallManager/PreinstallManager.cpp:
    - PreinstallManager::Initialize
    - PreinstallManager::Deinitialize
- entservices-appmanagers/PreinstallManager/PreinstallManagerImplementation.cpp:
    - PreinstallManagerImplementation::Configure
    - PreinstallManagerImplementation::Dispatch
    - PreinstallManagerImplementation::StartPreinstall
    - PreinstallManagerImplementation::readPreinstallDirectory
    - PreinstallManagerImplementation::installPackages
    - PreinstallManagerImplementation::sendOnPreinstallationCompleteEvent
    - PreinstallManagerImplementation::isNewerVersion

---

## Open Queries
- Should preinstall support per-app policy overrides for deferred installation windows?

## References
- entservices-appmanagers/PreinstallManager/PreinstallManager.cpp
- entservices-appmanagers/PreinstallManager/PreinstallManagerImplementation.cpp
- entservices-appmanagers/PackageManager/PackageManagerImplementation.cpp

## Change History
- [2026-05-03] - openspec-explore - Added per-plugin PreinstallManager specification.
