# Victim Selector

## Overview

Victim Selector is a Thunder plugin that chooses an application to terminate when a resource-reclamation request is received. It exposes one Thunder API, `evict`, and uses the AppManager COM-RPC interface to inspect loaded applications and request termination.

The current implementation covers the **system memory** path. GPU memory and hibernation flash storage are reserved in the interface but are not implemented yet.

## Public API

The COM-RPC contract is defined in `entservices-apis/apis/VictimSelector/IVictimSelector.h`.

### `evict`

```text
evict(reason, type) -> Core::hresult
```

Parameters:

- `reason`: Resource that requires reclamation.
  - `RAM`
  - `GPU`
  - `FLASH`
- `type`: Termination mode.
  - `HARD`: calls AppManager `KillApp`.
  - `SOFT`: calls AppManager `TerminateApp`.

The method is synchronous with respect to selecting a victim and submitting the termination request. The actual completion is reported asynchronously through `onEvictComplete`.

### `onEvictComplete`

```text
onEvictComplete(evicted, errorCode)
```

Parameters:

- `evicted`: `true` after the selected application reaches the unloaded state; otherwise `false`.
- `errorCode`:
  - `NONE`
  - `NO_CANDIDATE_FOUND`
  - `TERMINATION_FAILED`
  - `TIMEOUT`

The current implementation reports `TIMEOUT` in the contract for future use, but does not yet run a timeout timer. A termination request that fails immediately or produces an AppManager lifecycle error reports `TERMINATION_FAILED`.

## Runtime Flow

1. Thunder loads `org.rdk.VictimSelector`.
2. The plugin connects to `org.rdk.AppManager` through `QueryInterfaceByCallsign`.
3. Victim Selector registers an `IAppManager::INotification` listener.
4. A client calls `evict` with a resource reason and termination type.
5. For `RAM`, Victim Selector calls `GetLoadedApps`.
6. Loaded applications are filtered by lifecycle state and priority.
7. For each eligible application, Victim Selector calls `RuntimeManager.GetInfo(appInstanceId)`.
8. Victim Selector parses `memory.user.usage` from the returned Dobby statistics JSON.
9. Candidates are sorted by priority, memory usage, and last-active order.
10. Victim Selector calls either `TerminateApp` or `KillApp`.
11. AppManager lifecycle notifications are monitored.
12. When the pending application reaches `APP_STATE_UNLOADED`, Victim Selector sends `onEvictComplete(true, NONE)`.
13. If no eligible application exists, it sends `onEvictComplete(false, NO_CANDIDATE_FOUND)`.

## System Memory Selection

The current state-based selection order follows the first part of the HLA:

1. A single application in `PAUSED` state.
2. Applications in `SUSPENDED` state.
3. Applications in `HIBERNATED` state as a last resort.

Priority levels are assigned by AppManager as follows:

- `0`: system applications. These are never eligible for Victim Selector termination.
- `1`: Netflix and YouTube.
- `2`: other interactive applications.

Within the selected state group, priority `2` applications are selected before priority `1` applications. For candidates with the same priority, the highest `memory.user.usage` is selected first. If memory usage is equal or unavailable, the smallest `lastActiveIndex` is selected first, which represents the oldest position in the ordered last-active list. The actual wall-clock timestamp is not used.

Applications in other states, including `RUNNING`, `ACTIVE`, `LOADING`, `INITIALIZING`, `TERMINATING`, and `UNLOADED`, are not eligible.

If more than one application is paused, the paused group is not selected because the HLA describes the paused candidate as the only paused application. Selection then continues with suspended or hibernated candidates.

The interface carries `priority` and `lastActiveIndex` in `LoadedAppInfo`. RuntimeManager supplies per-app memory usage through its existing `GetInfo(appInstanceId, info)` method. Victim Selector reads `memory.user.usage` from that JSON response. If the stats request or field parsing fails, usage defaults to zero and last-active order remains the next tie-breaker. Application ID is used only as the final deterministic tie-breaker.

## Completion Handling

Victim Selector stores the application ID for the active eviction request. It listens to AppManager's `onAppLifecycleStateChanged` event.

A pending eviction is completed when either:

- The pending application reaches `APP_STATE_UNLOADED`, which is reported as success.
- AppManager reports a non-`APP_ERROR_NONE` lifecycle error, which is reported as a termination failure.

Completion notifications are copied with an additional reference and invoked outside the internal mutex. This prevents a client callback from causing lock re-entry or notification registration changes from deadlocking the plugin.

## Source Changes

### API repository

- `entservices-apis/apis/AppManager/IAppManager.h`
  - Extends `LoadedAppInfo` with `priority` and `lastActiveIndex` so COM-RPC consumers can apply the App Manager priority policy.
- `entservices-apis/apis/RuntimeManager/IRuntimeManager.h`
  - Existing `GetInfo(appInstanceId, info)` API provides the Dobby cgroup statistics JSON consumed by Victim Selector.
- `entservices-apis/apis/VictimSelector/IVictimSelector.h`
  - Defines RAM, GPU, and FLASH reasons.
  - Defines HARD and SOFT termination types.
  - Defines completion error codes.
  - Defines `Register`, `Unregister`, and `Evict`.
  - Defines the `onEvictComplete(bool, errorCode)` notification.
- `entservices-apis/apis/Ids.h`
  - Reserves `ID_VICTIM_SELECTOR` and `ID_VICTIM_SELECTOR_NOTIFICATION`.
  - The IDs use the `0x570` interface group.

### Appmanagers repository

- `VictimSelector/VictimSelector.h`
  - Thunder plugin wrapper.
  - Aggregates the COM-RPC implementation and JSON-RPC dispatcher.
- `VictimSelector/VictimSelector.cpp`
  - Registers `org.rdk.VictimSelector`.
  - Creates and configures the implementation.
  - Registers generated `JVictimSelector` dispatch methods.
- `VictimSelector/VictimSelectorImplementation.h`
  - Declares the COM-RPC implementation and AppManager notification sink.
- `VictimSelector/VictimSelectorImplementation.cpp`
  - Implements candidate selection, RuntimeManager statistics queries, memory parsing, AppManager connection, termination requests, and completion events.
- `VictimSelector/Module.h` and `VictimSelector/Module.cpp`
  - Define the plugin module dependencies.
- `VictimSelector/CMakeLists.txt`
  - Builds the plugin and implementation libraries.
  - Installs both libraries.
  - Generates the plugin configuration.
- `VictimSelector/VictimSelector.config`
  - Configures callsign, mode, autostart, startup order, and implementation locator.
- Top-level `CMakeLists.txt`
  - Adds the opt-in `PLUGIN_VICTIM_SELECTOR` build option.

## Build Configuration

Enable the plugin with:

```sh
cmake -DPLUGIN_VICTIM_SELECTOR=ON ...
```

The generated plugin callsign is:

```text
org.rdk.VictimSelector
```

The default mode is `Off`, consistent with the other app-manager plugins. Product builds can override:

```text
PLUGIN_VICTIM_SELECTOR_MODE
PLUGIN_VICTIM_SELECTOR_AUTOSTART
PLUGIN_VICTIM_SELECTOR_STARTUPORDER
```

## Current Limitations and Next Steps

1. Implement GPU selection using per-application GPU usage.
2. Implement FLASH selection using hibernation storage usage.
3. Add a termination timeout and report `TIMEOUT` when the lifecycle completion event does not arrive.
4. Reject or queue concurrent `evict` requests instead of allowing a later request to replace the pending application ID.
5. Add focused unit tests for candidate ordering, memory-stat parsing, no-candidate behavior, termination failures, lifecycle completion, and unsupported resource reasons.

## Validation

The modified files pass VS Code diagnostics where the required project headers are available, and `git diff --check` passes in both repositories.

A full CMake build could not be run in the development environment because CMake and the Thunder/WPEFramework development headers were unavailable.
