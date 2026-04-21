## Why

RuntimeManager generates Dobby container specs via `DobbySpecGenerator` using a fixed set of inputs (`ApplicationConfiguration` and `RuntimeConfig`). There is currently no supported mechanism for callers to inject or overlay custom Dobby spec fields (e.g., extra mounts, environment variables, rdkPlugins, or OCI-level overrides) at launch time without modifying the generator itself. This prevents per-app or per-launch customisation needed by platform integrators and test scenarios, today worked around with file-based hacks like `/tmp/specchange`.

## What Changes

- Add a new `SetDobbySpecOverride` API on `IRuntimeManager` that accepts a JSON string representing custom Dobby spec data keyed by `appInstanceId`.
- During `Run()`, after the base Dobby spec is generated, any registered override for that `appInstanceId` is deep-merged into the generated spec before the container is started.
- Callers can use **append** mode (additive merge — new keys and array entries are added) or **replace** mode (existing keys are overwritten).
- The override is consumed once per run and not persisted across restarts.
- Add a `ClearDobbySpecOverride` API to remove a pending override before launch.

## Capabilities

### New Capabilities

- `dynamic-dobbyspec-update`: Allows callers to register per-instance custom Dobby spec data that is merged into the generated spec at container launch time in RuntimeManager.

### Modified Capabilities

- `runtime-manager`: The `Run` operation is extended to apply any registered Dobby spec override after base spec generation. The `IRuntimeManager` interface gains two new methods: `SetDobbySpecOverride` and `ClearDobbySpecOverride`.

## Impact

- **`IRuntimeManager` interface** — new methods `SetDobbySpecOverride` and `ClearDobbySpecOverride` added (interface version bump required).
- **`RuntimeManagerImplementation`** — `Run()` updated to merge override after `generate()`; new methods implemented; in-memory override map added.
- **`DobbySpecGenerator`** — no changes required (merge happens in `RuntimeManagerImplementation`).
- **Tests** — L0 tests for new API and merge behaviour.
- **No breaking changes** to existing `Run()` callers — override is opt-in and defaults to no-op when not set.
