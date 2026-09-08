## Why

Runtime configuration is duplicated as divergent `Exchange::RuntimeConfig` structures across PackageManager, AppManager, LifecycleManager, and RuntimeManager, forcing coordinated interface changes whenever fields evolve. Replace that shared typed contract with one mutable opaque payload while preserving existing launch behavior.

## What Changes

- **BREAKING** Replace public `RuntimeConfig` COM parameters with an opaque serialized JSON string; all consumers must rebuild against the matching `entservices-apis` revision.
- Make PackageManager create and retain a flat payload using existing field names and defaults, with `envVariables`, `fkpsFiles`, and `logLevels` as JSON string arrays.
- Let AppManager and LifecycleManager mutate only owned properties while retaining unknown values.
- Make RuntimeManager validate once and decode known members into a private type used by Dobby and RALF.
- Mechanically migrate PreinstallManager, mocks, tests, generated API documentation, and CMake wiring.

## Capabilities

### New Capabilities

- `opaque-runtime-config`: End-to-end production, enrichment, forwarding, validation, and consumption of runtime configuration as an opaque JSON payload.

### Modified Capabilities

None. Existing flat module documents are not registered OpenSpec capabilities.

## Impact

Affected modules/specs are PackageManager, AppManager, LifecycleManager, RuntimeManager, and incidental PreinstallManager integration. Public interfaces change in `entservices-apis`. Existing JSONCPP support is reused; no external dependency or CMake option is added.

## Non-goals

- Change package metadata names, defaults, or ownership.
- Change lifecycle transitions, telemetry, storage, display, Dobby, RALF, Rialto, networking, resource, mount, logging, or debug behavior.
- Provide backward ABI compatibility with binaries built against the removed structures.
- Define JSON member ordering or whitespace as stable.
