## 1. Public API Contract

- [x] 1.1 Replace the four public `RuntimeConfig` definitions and affected method parameters in `entservices-apis/apis/{AppManager,AppPackageManager,LifecycleManager,RuntimeManager}/*.h` with opaque strings; acceptance: no public `Exchange::RuntimeConfig` declaration remains and generated changed-header validation succeeds.
- [x] 1.2 Regenerate `entservices-apis/docs/apis/AppPackageManager.md` and migrate legacy package-installer documentation; acceptance: runtime configuration is documented as an opaque serialized object with real arrays.
- [ ] 1.3 Add the breaking API/ABI migration and coordinated deployment requirement to the applicable `entservices-apis` release notes or `CHANGELOG.md`; depends on 1.1.

## 2. Payload Creation and Mutation

- [x] 2.1 Add `helpers/RuntimeConfigPayload.{h,cpp}` and `helpers/CMakeLists.txt` wiring for object-only parsing, compact serialization, scalar/array mutation, environment append/upsert, transactional publication, and unknown-value preservation.
- [x] 2.2 Migrate `PackageManager/PackageManagerImplementation.{h,cpp}` state and `Config`, `Lock`, `GetLockedInfo`, and `GetConfigForPackage` outputs to the canonical payload; acceptance: existing metadata names/defaults are retained and runtime association modifies only `runtimePath`.
- [x] 2.3 Migrate `AppManager/AppManagerTypes.h`, `AppInfoManager.*`, `AppManagerImplementation.*`, and `LifecycleInterfaceConnector.*` to store, enrich, reject invalid input, and forward the opaque payload without dropping unowned values.
- [x] 2.4 Migrate `LifecycleManager/ApplicationContext.*`, `LifecycleManagerImplementation.*`, `RuntimeManagerHandler.*`, and state handling to validate/store/reuse the payload and enrich `dialId`, `FIREBOLT_ENDPOINT`, and `TARGET_STATE` transactionally.

## 3. Final Runtime Consumption

- [x] 3.1 Add the private `RuntimeManager/RuntimeConfiguration.{h,cpp}` model and decoder; acceptance: known fields/types and required launch data are validated, optional defaults and UID/GID fallback are preserved, and unknown properties are tolerated.
- [x] 3.2 Decode at the start of `RuntimeManagerImplementation::Run` and reject invalid payloads before display, runtime-map, RALF-bundle, or container side effects.
- [x] 3.3 Migrate `RuntimeManager/DobbySpecGenerator.*`, `RuntimeManager/ralf/RalfPackageBuilder.*`, and `RuntimeManager/ralf/RalfOCIConfigGenerator.*` to the private type while preserving legacy Dobby, RALF, Rialto, resource, network, mount, logging, and environment behavior.
- [x] 3.4 Wire `RuntimeConfiguration.cpp` into `RuntimeManager/CMakeLists.txt` and required L0/L1 source lists.

## 4. Incidental Consumers and Documentation

- [x] 4.1 Mechanically migrate `PreinstallManager/PreinstallManagerImplementation.{h,cpp}` and all affected mocks under `Tests/mocks/` and `Tests/L0Tests/common/` without adding payload interpretation.
- [x] 4.2 Update AppManager, PackageManager, LifecycleManager, RuntimeManager, and PreinstallManager Markdown plus `openspec/specs/*.spec.md` architecture descriptions for ownership and data flow.
- [x] 4.3 Point `.github/workflows/L0-tests.yml` and `.github/workflows/L1-tests.yml` at the coordinated API feature revision for branch CI.
- [ ] 4.4 Replace the temporary `feature/RDKEMW-24163` `INTERFACES_REF` with the final merged/released API revision before production integration; depends on the API change landing.

## 5. Automated Coverage

- [x] 5.1 Add L0 coverage for payload serialization, real arrays, transactional mutation, environment upsert, malformed JSON, and nested unknown-value preservation in `Tests/L0Tests/PackageManager/PackageManager_ImplementationTests.cpp`.
- [x] 5.2 Add/update AppManager, LifecycleManager, RuntimeManager, RALF, PreinstallManager, and mock L0/L1 sources for opaque signatures, enrichment, invalid input, respawn reuse, private decoding, and no RuntimeManager side effects on invalid input.
- [ ] 5.3 Extend `Tests/L2Tests/tests/AppManager_L2Test.cpp` with an end-to-end launch fixture that captures every handoff, checks AppManager/LifecycleManager additions, and verifies an unknown nested property survives; depends on 2.2-3.3.
- [ ] 5.4 Add representative pre-refactor expected fixtures and semantic comparisons for normalized legacy Dobby specs, RALF OCI configs, environment, display capabilities, resources, mounts, and telemetry inputs; depends on 3.3.

## 6. Build and Compatibility Verification

- [ ] 6.1 Remove trailing whitespace from the committed app-manager change and verify `git diff --check` plus the equivalent commit patch check produce no errors.
- [ ] 6.2 Run the repository GitHub plugin-build workflow against the coordinated API revision; acceptance: all affected plugins compile and link successfully in the production CI setup.
- [ ] 6.3 Run `.github/workflows/L0-tests.yml`; acceptance: every configured affected L0 executable runs with zero failures, not merely compiles.
- [ ] 6.4 Run `.github/workflows/L1-tests.yml`; acceptance: every configured affected L1 target compiles, each affected executable runs, and all report zero failures; depends on 6.2.
- [ ] 6.5 Run the applicable GitHub workflow jobs for both legacy Dobby and RALF-enabled configurations plus the AppManager L2 launch path; acceptance: all variants pass and 5.3-5.4 demonstrate semantic equivalence.
- [x] 6.6 Confirm `entservices-apis` changed-header validation, generated documentation check, and Ubuntu/ThunderComponents build pass in PR CI.
- [ ] 6.7 Record the unrelated legacy-header findings from the full `entservices-apis` validator as an accepted baseline or resolve them through the owning workstream; acceptance: this change has no untriaged CI failure.
- [x] 6.8 Audit production and public API sources for `Exchange::RuntimeConfig`, public `struct RuntimeConfig`, and the old guard; acceptance: zero relevant references remain.
