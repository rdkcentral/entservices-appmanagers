# EntServices-AppManagers Plugin Spec - LifecycleManager

## Overview
LifecycleManager is the app state machine and transition orchestrator for EntServices-AppManagers. It coordinates app state progressions and ties runtime/window handlers into consistent transition flows.

## Description
This plugin governs legal state transitions, transition-side effects, and event propagation for app lifecycle control. It manages transition handlers, runtime interaction, and state persistence pathways.

## Requirements
### Requirement: LifecycleManager enforces valid state transitions
LifecycleManager MUST permit only valid transitions across lifecycle states and reject invalid transition attempts deterministically.

#### Scenario: Invalid transition request
- **WHEN** a client requests a disallowed state transition
- **THEN** LifecycleManager rejects the request and preserves prior state

### Requirement: LifecycleManager orchestrates runtime and window side effects
LifecycleManager SHALL coordinate runtime and window manager handlers while entering and exiting lifecycle states.

#### Scenario: State change invokes side-effect handlers
- **WHEN** an app transitions between lifecycle states
- **THEN** runtime/window handlers are invoked in defined order for that transition

### Requirement: LifecycleManager emits state-change events
LifecycleManager MUST emit lifecycle state notifications for observers and dependent services.

#### Scenario: Observer notification on transition
- **WHEN** a transition completes
- **THEN** the plugin publishes an app lifecycle state-change event with updated state

## Architecture / Design (if applicable)
- Core orchestration through StateHandler and StateTransitionHandler.
- RequestHandler mediates client requests and invokes runtime/window abstractions.

## External Interfaces (if applicable)
- Lifecycle API registration (`Exchange::JLifecycleManagerState::Event::OnAppLifecycleStateChanged`)
- Runtime and window delegation via RuntimeManagerHandler and WindowManagerHandler

## Performance (if applicable)
- Transition processing SHOULD avoid blocking operations in control path to keep launch/suspend latency bounded.

## Security (if applicable)
- Transition operations MUST validate caller intents and avoid unauthorized state mutation.

## Versioning & Compatibility (if applicable)
- Lifecycle transition semantics SHALL remain backward compatible for existing app states unless major-version migration is documented.

## Conformance Testing & Validation (if applicable)
- Validate transition graph correctness, event publication, and handler sequencing with L0/L1 lifecycle tests.

## Covered Code
- entservices-appmanagers/LifecycleManager/LifecycleManager.cpp:
    - LifecycleManager::Initialize
    - LifecycleManager::Deinitialize
- entservices-appmanagers/LifecycleManager/LifecycleManagerImplementation.cpp:
    - WPEFramework::Plugin::LifecycleManagerImplementation::SpawnApp
    - WPEFramework::Plugin::LifecycleManagerImplementation::SetTargetAppState
    - WPEFramework::Plugin::LifecycleManagerImplementation::UnloadApp
    - WPEFramework::Plugin::LifecycleManagerImplementation::KillApp
- entservices-appmanagers/LifecycleManager/StateHandler.cpp:
    - StateHandler::changeState
    - StateHandler::isValidTransition
- entservices-appmanagers/LifecycleManager/RuntimeManagerHandler.cpp:
    - WPEFramework::Plugin::RuntimeManagerHandler::run
    - WPEFramework::Plugin::RuntimeManagerHandler::suspend
    - WPEFramework::Plugin::RuntimeManagerHandler::resume

---

## Open Queries
- Should transition failure policy include configurable retry for transient runtime errors?

## References
- entservices-appmanagers/LifecycleManager/LifecycleManager.cpp
- entservices-appmanagers/LifecycleManager/LifecycleManagerImplementation.cpp
- entservices-appmanagers/LifecycleManager/StateHandler.cpp
- entservices-appmanagers/LifecycleManager/RuntimeManagerHandler.cpp

## Change History
- [2026-05-03] - openspec-explore - Added per-plugin LifecycleManager specification.
