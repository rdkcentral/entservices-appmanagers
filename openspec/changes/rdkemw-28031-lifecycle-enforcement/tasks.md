Branch: `RDKEMW-28031` (from `RDKEMW-28030` commit `7d27b9ec`). All commands run from `../local-build/` through `docker-run.sh` on `linux/amd64`, per `AGENTS.md`.

## 1. Enable APP_MANAGER_RESOURCE_MONITOR in AppManager

- [ ] 1.1 Add `APP_MANAGER_RESOURCE_MONITOR=ON` to AppManager CMake configuration in local-build setup. Enable the feature gate that compiles in `AppStateTransitionManager` and related code. Acceptance: AppManager builds with `AppStateTransitionManager` included, `APP_MANAGER_RESOURCE_MONITOR` is defined.

## 2. AppManager: Query ResourceMonitor for GPU stats

- [ ] 2.1 Add ResourceMonitor interface query to AppManager's `AppStateTransitionManager` or `AppManagerImplementation`. Query `org.rdk.ResourceMonitor` during initialization. Acceptance: ResourceMonitor interface is acquired successfully, unavailability is logged gracefully.
- [ ] 2.2 Add method to query GPU memory usage for a specific app instance. Use ResourceMonitor's `GetStats()` API to get GPU memory. Acceptance: GPU memory is queried successfully, returns 0 if GPU stats unavailable.

## 3. AppManager: Query Dobby for memory floor

- [ ] 3.1 Add method to query `hard_non_reclaimable_floor` for a specific app instance via RuntimeManager/Dobby API. Use RuntimeManager's `GetInfo` API which interfaces with Dobby. Acceptance: memory floor is queried successfully, error is logged if query fails.

## 4. AppManager: Extend SUSPENDED enforcement in AppStateTransitionManager

- [ ] 4.1 In `AppStateTransitionManager::Process`, after SUSPENDED timer expires, add GPU memory check using the ResourceMonitor query method. Compare against `maxGPUallowedInSuspended` (20MiB default, platform setting). If GPU exceeds threshold, call `TerminateApp`. Acceptance: GPU check is performed, termination is requested if threshold exceeded, error logged if GPU stats unavailable.
- [ ] 4.2 In `AppStateTransitionManager::Process`, if GPU is within limits, add memory floor check using the Dobby query method. Compare against `maxSuspendedSystemMemory` from RDKEMW-28030's `GetSuspendedStateConfig` (via Package Manager). If memory floor exceeds limit, call `TerminateApp`. Acceptance: memory floor check is performed, termination is requested if limit exceeded, error logged if query fails.
- [ ] 4.3 Update existing hibernation logic in `AppStateTransitionManager::Process` to include GPU and memory checks as prerequisites. Only proceed to hibernation if both GPU and memory checks pass. Acceptance: hibernation only occurs when GPU and memory conditions are met, termination occurs otherwise.

## 5. AppManager: Non-app workload handling

- [ ] 5.1 In `AppStateTransitionManager`, add check for non-app workloads using AppManager's `GetLoadedApps` to query `LoadedAppInfo.type`. If type is non-app (e.g., "SYSTEM"), terminate after PAUSED timeout regardless of SUSPENDED support. Acceptance: non-app workloads are identified, termination is requested after PAUSED timeout.

## 6. AppManager: Lifecycle capability integration

- [ ] 6.1 Ensure `AppStateTransitionManager` uses RDKEMW-28030's lifecycle capability APIs when checking SUSPENDED/HIBERNATED support. The existing implementation may use app properties; update to use the new ResourceMonitor capability provider if needed. Acceptance: SUSPENDED/HIBERNATED support is checked via RDKEMW-28030 APIs, behavior matches spec.

## 7. Error handling and resilience

- [ ] 7.1 Add error handling for ResourceMonitor COM-RPC calls. If ResourceMonitor is unavailable, log a warning and skip GPU check (proceed with memory check or hibernation). Acceptance: ResourceMonitor unavailability is logged gracefully, enforcement continues with best-effort behavior.
- [ ] 7.2 Add error handling for Dobby memory floor queries. If query fails, log a warning and proceed with hibernation check (or terminate if memory is critical). Acceptance: query failures are logged, enforcement continues with best-effort behavior.

## 8. Testing

- [ ] 8.1 Add L0 tests in Tests/L0Tests/AppManager/ for SUSPENDED GPU and memory checking logic. Mock ResourceMonitor and Dobby APIs. Acceptance: L0 tests pass, GPU and memory enforcement logic is verified.
- [ ] 8.2 Add L0 tests for non-app workload handling. Mock `GetLoadedApps` to return non-app type. Acceptance: L0 tests pass, non-app termination logic is verified.
- [ ] 8.3 Add L1 tests in Tests/L1Tests/tests/test_AppManager.cpp for end-to-end SUSPENDED enforcement with real plugin initialization. Cover GPU check, memory floor check, and termination scenarios. Acceptance: L1 tests pass, integration with ResourceMonitor and Dobby is verified.
- [ ] 8.4 Register new L0 test target in Tests/L0Tests/CMakeLists.txt if new test files are added. Acceptance: CMake configuration succeeds, L0 tests are included in the build.

## 9. Documentation

- [ ] 9.1 Update AppManager/AppStateTransitionManager.h/cpp with comments explaining GPU grace period and memory floor checking. Document the ResourceMonitor and Dobby API usage. Acceptance: code is documented, GPU and memory enforcement logic is clear.
- [ ] 9.2 Update AppManager/ResourceMonitoring.md (from RDKEMW-17171) with a section on GPU and memory enforcement added by RDKEMW-28031. Document the new checks and their integration with existing timer logic. Acceptance: documentation is updated, new functionality is documented.

## 10. Verification

- [ ] 10.1 Plugin build: `NO_TTY=1 ./docker-run.sh build.sh entservices-appmanagers plugins --clean`. Zero compile or link errors. Acceptance: build succeeds.
- [ ] 10.2 L0: `NO_TTY=1 ./docker-run.sh build.sh entservices-appmanagers l0 run --clean`. Every `*_l0test` runs, including new AppManager SUSPENDED enforcement tests. Record pass and fail counts. Acceptance: all L0 tests pass.
- [ ] 10.3 L1: run the AppManager L1 suite with exit code 0 and no failures. Acceptance: L1 tests pass.
- [ ] 10.4 Run `git diff --check` with no whitespace errors, then write the completion report: modules built, tests added, exact commands, counts, warnings and anything not run. Acceptance: report is complete.
