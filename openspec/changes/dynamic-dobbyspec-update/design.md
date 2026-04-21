## Context

RuntimeManager generates OCI/Dobby container specs via `DobbySpecGenerator` (or `RalfPackageBuilder` when `RALF_PACKAGE_SUPPORT_ENABLED`). The generated JSON spec is passed directly to the Dobby daemon via the `OCIContainer` plugin's `StartContainerFromDobbySpec` API. There is currently no way for callers to contribute additional spec fields (e.g., extra bind-mounts, environment variables, rdkPlugins, or OCI-level overrides) without modifying generator code.

A file-based workaround (`/tmp/specchange`) is already present in the codebase for debug UID changes, demonstrating the need for a proper API-driven override path.

The interface lives in `entservices-apis/apis/RuntimeManager/IRuntimeManager.h` and implementation in `RuntimeManager/RuntimeManagerImplementation.{h,cpp}`.

## Goals / Non-Goals

**Goals:**
- Add `SetDobbySpecOverride(appInstanceId, specJson, mergeStrategy)` and `ClearDobbySpecOverride(appInstanceId)` to `IRuntimeManager`.
- In `Run()`, after the base Dobby spec is generated and before `StartContainerFromDobbySpec` is called, apply any registered override for the given `appInstanceId`.
- Support two merge strategies: `append` (additive deep-merge, never removes keys or shrinks arrays) and `replace` (deep-merge that overwrites matching keys with override values).
- Overrides are per-`appInstanceId`, in-memory only, consumed once on use.

**Non-Goals:**
- Persistent spec overrides across daemon restarts.
- Overriding the entire spec (callers supply a fragment, not a full replacement).
- Modifying `DobbySpecGenerator` internals.
- RALF path-specific spec fragments (merge is applied to the final JSON regardless of generator path).

## Decisions

### Decision 1: Override stored in `RuntimeManagerImplementation`, not in `DobbySpecGenerator`
The merge happens after generation, keeping `DobbySpecGenerator` a pure value-producing component. The implementation holds an `std::unordered_map<string, DobbySpecOverride>` where `DobbySpecOverride` contains the JSON fragment string and the merge strategy enum.

**Alternatives considered:**
- Passing override into `DobbySpecGenerator::generate()` — rejected: tightly couples the generator to the override mechanism and makes RALF path harder to support uniformly.
- File-based override (extend the `/tmp/specchange` pattern) — rejected: insecure, not API-discoverable, no cleanup guarantee.

### Decision 2: Deep-merge using jsoncpp `Json::Value` traversal
After generating the base spec string, parse both the base spec and the override JSON using jsoncpp (already a dependency in `DobbySpecGenerator`). Implement a recursive merge function:
- `append` mode: for objects, add missing keys; for arrays, append override entries; for scalars, leave base unchanged unless key is absent.
- `replace` mode: for objects, recurse and overwrite; for arrays, replace the entire array with override value; for scalars, overwrite.

Serialize the merged `Json::Value` back to a string before passing to `StartContainerFromDobbySpec`.

**Alternatives considered:**
- String-level JSON patch (RFC 6902) — rejected: adds a new dependency and is complex for the append use case.
- Thunder's `Core::JSON` — rejected: known slash-escaping issues already documented in `DobbySpecGenerator.h` comments.

### Decision 3: Override is consumed after `Run()` succeeds or fails
The override is removed from the map after `Run()` attempts to use it, regardless of whether the container start succeeds. This prevents stale overrides accumulating for failed launches.

**Alternatives considered:**
- Keep override until explicitly cleared — rejected: risk of unintended override application on retry/relaunch.

### Decision 4: New interface methods added to `IRuntimeManager` (interface version bump)
`SetDobbySpecOverride` and `ClearDobbySpecOverride` are added as virtual methods with Thunder stub generation annotations. The interface ID (`ID_RUNTIME_MANAGER`) version is incremented. Callers using old stubs will fail at RPC dispatch with a known version mismatch, which is the standard Thunder upgrade path.

### Decision 5: Merge strategy as an enum in `IRuntimeManager`
```cpp
enum MergeStrategy : uint8_t {
    MERGE_STRATEGY_APPEND  = 0,
    MERGE_STRATEGY_REPLACE = 1
};
```
An enum is preferred over a string to enable type-safe dispatch and future extensibility.

## Risks / Trade-offs

- **Risk**: A malformed override JSON fragment could produce an invalid merged spec → **Mitigation**: Validate override JSON on `SetDobbySpecOverride` (reject with `INVALID_ARGUMENT` if parse fails); log and skip merge if re-parsing fails at `Run()` time.
- **Risk**: Large or deeply nested override fragments could affect launch latency → **Mitigation**: The JSON merge is in-process and non-blocking; acceptable for the typical fragment size. No async path needed.
- **Risk**: Interface version bump requires coordinated update in `entservices-apis` and all stub-generated code → **Mitigation**: Document as part of tasks; no runtime ABI break since Thunder loads by version.
- **Risk**: Thread safety — multiple threads calling `SetDobbySpecOverride` concurrently → **Mitigation**: Override map access is guarded by the existing `mRuntimeManagerImplLock`.

## Migration Plan

1. Update `IRuntimeManager.h` in `entservices-apis` with new methods and bump interface version.
2. Regenerate Thunder stubs (JSON-RPC proxy/skeleton).
3. Implement new methods and merge logic in `RuntimeManagerImplementation`.
4. Remove the `/tmp/specchange` debug workaround from `Run()` (separate clean-up).
5. Add L0 unit tests for new API and merge scenarios.
6. No rollback needed — feature is additive and opt-in; callers that do not call `SetDobbySpecOverride` are unaffected.

## Open Questions

- Should `SetDobbySpecOverride` be exposed over the JSON-RPC surface (Thunder plugin API) or remain an internal C++ interface only? Currently proposed as internal `IRuntimeManager` only.
- Is there a need to support override chaining (multiple fragments per instance)? Currently scoped to a single fragment per instance per launch.
