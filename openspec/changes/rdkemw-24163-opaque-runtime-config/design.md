## Context

See `proposal.md` for motivation and `specs/opaque-runtime-config/spec.md` for the behavioral contract. Runtime configuration originates in libPackage metadata, crosses four Thunder COM-RPC plugin boundaries, and ultimately drives Dobby or RALF OCI generation. The affected architecture is documented in `openspec/specs/{PackageManager,AppManager,LifecycleManager,RuntimeManager}.spec.md`; PreinstallManager is a mechanical consumer.

## Goals / Non-Goals

**Goals:**
- Remove the duplicated public typed contract while retaining existing property semantics.
- Permit intermediate managers to enrich payloads without coordinated changes for unknown future fields.
- Keep complete schema knowledge and typed consumption private to RuntimeManager.
- Fail transactionally on invalid JSON and before launch side effects.

**Non-Goals:**
- Preserve binary ABI for clients compiled against `Exchange::RuntimeConfig`.
- Change package metadata, lifecycle transitions, or generated container behavior.
- Make JSON formatting or member ordering stable.

## Decisions

### 1. Flat JSON object transported as an opaque string

The payload is a compact serialized JSON object using existing field names. A string avoids generated COM structures coupling every manager to every field. A nested versioned structure was rejected because it adds migration complexity without solving an immediate requirement; a generic key/value iterator was rejected because it loses JSON types and nesting.

Representative payload:

```json
{"command":"launcher","appPath":"/apps/example","envVariables":["A=1"],"logLevels":["INFO"],"vendor":{"future":true}}
```

Public COM changes are direct replacements:

```text
IAppPackageManager::{Config,Lock,GetLockedInfo,GetConfigForPackage}
    RuntimeConfig& out -> string& runtimeConfigPayload out @opaque
ILifecycleManager::SpawnApp
    const RuntimeConfig& -> const string& runtimeConfigPayload @opaque
IRuntimeManager::Run
    const RuntimeConfig& -> const string& runtimeConfigPayload @opaque
```

For JSON-RPC/generated bindings, the payload is therefore one opaque string rather than an expanded struct. This is a breaking response/signature change and requires coordinated rebuild and deployment.

### 2. Producer and mutation ownership

```text
libPackage ConfigMetaData
        |
        v
PackageManager -- create/store canonical JSON
        |
        v
AppManager -- unpackedPath, launch env, RALF identity, debug overrides
        |
        v
LifecycleManager -- dialId, FIREBOLT_ENDPOINT, TARGET_STATE
        |
        v
RuntimeManager -- validate/decode once into private RuntimeConfiguration
        |                                      |
        +--> legacy DobbySpecGenerator         +--> RALF package/OCI generators
```

Each intermediate stage parses the whole object, changes only owned members, and serializes the whole object. Unknown values survive semantically; whitespace and ordering may change.

### 3. Schema-neutral shared mutation helper

`helpers/RuntimeConfigPayload` owns object parsing, compact serialization, scalar/array setters, and environment append/upsert operations. It does not define the complete runtime schema. Mutations operate on temporary state and publish a replacement string only after parse, mutation, and serialization succeed.

Keeping field ownership in each plugin avoids recreating the removed shared structure as a helper. Direct ad hoc JSON manipulation was rejected because transactional and unknown-member behavior would be duplicated.

### 4. Real JSON arrays

`envVariables`, `fkpsFiles`, and `logLevels` are arrays of strings. Embedded JSON text was rejected because it requires double parsing and obscures type validation. RuntimeManager rejects wrong array and element types.

### 5. Private RuntimeManager decoder

RuntimeManager decodes known members once at the beginning of `Run` into `RuntimeConfiguration`. Unknown members are ignored there because upstream preservation, not downstream interpretation, provides extensibility. UID/GID use existing `Run` arguments as fallback, payload identity can override them, and legacy Dobby requires `command`; RALF may derive launch configuration from package metadata.

Validation failure returns the established invalid-parameter result and notification before display, runtime-map, bundle, or container side effects. A permissive coercing decoder was rejected because malformed strings would otherwise reach security- and resource-sensitive launch generation.

### 6. Preserve launch generators

Dobby and RALF generators consume the private type but retain their established decisions for command paths, capabilities, environment, resources, network, mounts, logging, display, storage, Rialto, and optional features. Compatibility is judged by normalized generated specifications and observed launch inputs, not by JSON text equality.

### 7. Build and optional-feature impact

Existing JSONCPP support is reused; no external dependency or CMake option is added. `helpers/RuntimeConfigPayload.cpp` is added to the helper library and explicit test source lists, while `RuntimeManager/RuntimeConfiguration.cpp` is added to RuntimeManager and relevant tests.

- `RALF_PACKAGE_SUPPORT`: command may be optional and RALF generators use the private model.
- `ENABLE_RIALTO`: capability decisions continue from decoded capability data.
- `AIMANAGERS_TELEMETRY_METRICS_SUPPORT`: telemetry flow is unchanged.
- Debug/runtime-config flags: existing override behavior remains, but values mutate the payload before final decode.

## Risks / Trade-offs

- **Breaking COM ABI and JSON-RPC shape** → Major-version/coordinated rollout; rebuild every plugin and external client against the matching API revision.
- **Stricter validation rejects previously tolerated malformed data** → Preserve optional defaults, test representative metadata, and reject only wrong types/ranges or launch-required omissions.
- **Subtle container behavior drift** → Compare normalized pre/post Dobby specs, RALF OCI configs, environment, mounts, resources, and capabilities.
- **Unknown values lose textual formatting/order** → Guarantee semantic value preservation only.
- **Feature branch API pin is temporary** → Replace it with the merged/released revision before production integration.
- **Main flat module specs already describe the implemented state** → Reconcile duplicate wording when this retroactive change is archived.

## Migration Plan

1. Land and release the `entservices-apis` signature change with breaking-change documentation.
2. Build all app-manager plugins and generated proxies against that exact revision.
3. Run the repository GitHub workflows for plugin builds, L0, L1, legacy Dobby, RALF, and L2 verification in the production CI setup.
4. Compare normalized baseline and refactored OCI outputs for representative applications.
5. Deploy API and all participating plugin binaries atomically; mixed old/new binaries are unsupported.
6. After final verification, replace branch pins with the released API revision and archive/sync the OpenSpec change.

Rollback requires restoring both the previous API package and every plugin binary built against it; rolling back only one side is unsafe.
