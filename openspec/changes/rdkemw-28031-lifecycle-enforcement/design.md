## Context

RDKEMW-28030 provides internal C++ methods in ResourceMonitor to query an application's lifecycle capabilities and Suspended-state configuration from Package Manager. However, these capabilities are not yet enforced. The HLA defines timeouts for PAUSED and SUSPENDED states (60s for PAUSED→SUSPENDED, 15s for SUSPENDED→HIBERNATED with GPU grace period) to prevent inactive apps from consuming system resources.

RDKEMW-17171 implemented PAUSED→SUSPENDED→HIBERNATED timers in AppManager's `AppStateTransitionManager`, but did not include GPU grace period checking or memory floor checking (`hard_non_reclaimable_floor`). RDKEMW-28031 extends `AppStateTransitionManager` to add these missing enforcement checks.

ResourceMonitor provides resource statistics APIs (RDKDEV-1479) that AppManager can query for GPU memory and other resource information during enforcement.

See proposal.md for motivation.

## Goals / Non-Goals

**Goals:**
- Extend AppManager's `AppStateTransitionManager` to add GPU grace period checking during SUSPENDED timeout
- Extend AppManager's `AppStateTransitionManager` to add memory floor (`hard_non_reclaimable_floor`) checking during SUSPENDED timeout
- Terminate apps that violate GPU or memory requirements after the grace period
- Query GPU memory usage via ResourceMonitor's public APIs
- Query memory floor via Dobby API
- Maintain separation of concerns: AppManager owns lifecycle operations, ResourceMonitor owns resource monitoring

**Non-Goals:**
- Duplicating PAUSED→SUSPENDED timer logic (already implemented in RDKEMW-17171)
- Moving timer logic to ResourceMonitor (AppManager owns lifecycle operations per architectural decision)
- Continuous CPU/memory monitoring in Suspended (only one-time deadline checks are in scope)
- Modifying the RDKEMW-28030 capability (it is used as-is)

## Decisions

### Architecture: AppManager owns timers, ResourceMonitor monitors resources
**Decision:** AppManager's `AppStateTransitionManager` (from RDKEMW-17171) owns the PAUSED and SUSPENDED timers. ResourceMonitor provides resource statistics APIs that AppManager queries during enforcement. This separates lifecycle operations from resource monitoring.

**Rationale:** RDKEMW-17171 already implemented PAUSED→SUSPENDED→HIBERNATED timers in AppManager. Duplicating this in ResourceMonitor would create maintenance burden and potential conflicts. AppManager is the natural place for lifecycle state machine bookkeeping. ResourceMonitor specializes in resource monitoring and provides statistics APIs.

**Alternatives considered:**
- Implement timers in ResourceMonitor: Would duplicate RDKEMW-17171 work, conflict with existing architecture
- Move `AppStateTransitionManager` to a shared library: Adds dependency, both components need rebuild on changes

### GPU and memory checking location
**Decision:** Add GPU grace period and memory floor checking to AppManager's `AppStateTransitionManager::Process` method during SUSPENDED timeout handling. AppManager queries ResourceMonitor APIs for GPU memory stats and Dobby API for memory floor.

**Rationale:** The SUSPENDED timeout already exists in `AppStateTransitionManager`. Adding the GPU and memory checks there keeps all enforcement logic in one place. ResourceMonitor's public APIs (RDKDEV-1479) provide GPU memory stats. Dobby provides memory floor stats.

**Alternatives considered:**
- Implement checks in ResourceMonitor and call AppManager to act: Would require new COM-RPC APIs, adds complexity
- Create a separate enforcement component: Adds new component, unclear ownership

### ResourceMonitor API usage
**Decision:** AppManager queries ResourceMonitor's `GetStats()` or `GetMemInfo()` API to get GPU memory usage for a specific app instance. The API returns per-app GPU memory statistics.

**Rationale:** ResourceMonitor's public APIs (RDKDEV-1479) are designed for external consumption. AppManager is a natural consumer for lifecycle enforcement. Using COM-RPC keeps the interface clean and allows ResourceMonitor to own the statistics collection logic.

**Alternatives considered:**
- Direct Dobby API calls: Would bypass ResourceMonitor's abstraction, duplicate logic
- cgroup file reads directly: Fragile across cgroup v1/v2 differences

### Configuration
**Decision:** The PAUSED timeout (`pausedToSuspendedTimeout`, 60s) and SUSPENDED timeout (`suspendedToHibernatedTimeout`, 15s) are already configured in AppManager (RDKEMW-17171). The GPU threshold (`maxGPUallowedInSuspended`, 20MiB) is a platform setting from the HLA. No new configuration is needed in ResourceMonitor.

**Rationale:** RDKEMW-17171 already added these configuration options to AppManager. The GPU threshold is a platform-wide setting that applies to all apps. Keeping configuration in AppManager aligns with timer ownership.

**Alternatives considered:**
- Adding configuration to ResourceMonitor: Would split timer configuration from timer implementation
- Hardcoding all timeouts: Would not allow tuning for different device profiles

## Risks / Trade-offs

### ResourceMonitor or Dobby unavailable
**Risk:** If ResourceMonitor or Dobby is unavailable when SUSPENDED timer expires, the GPU and memory checks cannot be performed, and the app may be incorrectly terminated or hibernated.

**Mitigation:** Log the unavailability and skip the unavailable check. If both checks are unavailable, proceed with hibernation if supported (existing RDKEMW-17171 behavior). Do not crash or block on the enforcement action.

### GPU stats unavailable on some platforms
**Risk:** Some platforms may not support GPU memory statistics via Dobby/ResourceMonitor. The GPU grace period check cannot be performed on such platforms.

**Mitigation:** If GPU stats are unavailable, skip the GPU check and proceed to memory floor check. Log a warning that GPU enforcement is not available on this platform.

### Memory floor query unavailable
**Risk:** If Dobby does not expose `hard_non_reclaimable_floor` or the RuntimeManager API fails, the memory floor check cannot be performed.

**Mitigation:** If memory floor query fails, log a warning and proceed with hibernation if GPU check passed and hibernation is supported. Consider terminating if memory is critical (heuristic based on available system memory).

### Non-app workload identification
**Risk:** The values for `LoadedAppInfo.type` are not yet documented or standardized. Misidentifying a non-app workload could lead to incorrect enforcement.

**Mitigation:** Confirm the expected values (e.g., "SYSTEM") with the AppManager team before implementation. If identification is ambiguous, treat the workload as an app and enforce lifecycle capabilities normally.

### Concurrent enforcement actions
**Risk:** If multiple apps have SUSPENDED timers that expire simultaneously, the enforcement actions may contend for COM-RPC calls to ResourceMonitor and Dobby.

**Mitigation:** Serialize enforcement actions through the existing `AppStateTransitionManager` worker thread. The worker already processes timers sequentially, so no additional synchronization is needed.

### Configuration inconsistency
**Risk:** If `suspendedToHibernatedTimeout` or `maxGPUallowedInSuspended` are configured differently across devices, enforcement behavior may vary unexpectedly.

**Mitigation:** These are platform settings from the HLA with documented defaults. Document the settings in AppManager README and provide validation in configuration loading.

## Migration Plan

No migration is required for existing deployments. This extends the existing `AppStateTransitionManager` (from RDKEMW-17171) with additional checks. The feature is behind the `APP_MANAGER_RESOURCE_MONITOR` CMake flag, which can be enabled independently.

**Deployment steps:**
1. Enable `APP_MANAGER_RESOURCE_MONITOR=ON` in AppManager build configuration
2. Deploy the updated AppManager plugin with extended `AppStateTransitionManager`
3. Monitor logs for GPU and memory check results and enforcement actions
4. Adjust platform settings (`maxGPUallowedInSuspended`) if needed based on device characteristics

**Rollback strategy:**
- Disable `APP_MANAGER_RESOURCE_MONITOR=ON` to revert to RDKEMW-17171 behavior (no GPU/memory checks)
- Uninstall the updated plugin to revert to the previous version
- No data migration or schema changes are involved

## Open Questions

None. The design decisions are based on the HLA requirements and the architectural clarifications provided. All implementation details can be resolved during coding without changing the approach or the spec.
