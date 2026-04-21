## 1. Interface Changes (entservices-apis)

- [ ] 1.1 Add `MergeStrategy` enum (`MERGE_STRATEGY_APPEND`, `MERGE_STRATEGY_REPLACE`) to `IRuntimeManager.h`
- [ ] 1.2 Add `virtual Core::hresult SetDobbySpecOverride(const string& appInstanceId, const string& specJson, const MergeStrategy mergeStrategy)` to `IRuntimeManager`
- [ ] 1.3 Add `virtual Core::hresult ClearDobbySpecOverride(const string& appInstanceId)` to `IRuntimeManager`
- [ ] 1.4 Bump `IRuntimeManager` interface version (update `ID_RUNTIME_MANAGER` version tag)
- [ ] 1.5 Regenerate Thunder JSON-RPC proxy/skeleton stubs for the updated interface

## 2. RuntimeManagerImplementation — Override Storage

- [ ] 2.1 Define `DobbySpecOverride` struct (holding `specJson: string` and `mergeStrategy: MergeStrategy`) in `RuntimeManagerImplementation.h`
- [ ] 2.2 Add `std::unordered_map<string, DobbySpecOverride> mDobbySpecOverrides` member to `RuntimeManagerImplementation`
- [ ] 2.3 Implement `SetDobbySpecOverride`: validate `appInstanceId` non-empty, parse `specJson` with jsoncpp to verify valid JSON, store in `mDobbySpecOverrides` under `mRuntimeManagerImplLock`, return `INVALID_ARGUMENT` on validation failures
- [ ] 2.4 Implement `ClearDobbySpecOverride`: look up `appInstanceId` in `mDobbySpecOverrides` under lock, erase if found and return `ERROR_NONE`, return `NOT_FOUND` if absent

## 3. RuntimeManagerImplementation — Merge Logic

- [ ] 3.1 Add private helper `void mergeJsonSpec(Json::Value& base, const Json::Value& overlay, MergeStrategy strategy)` implementing recursive deep-merge for `append` (add missing keys, append to arrays) and `replace` (overwrite keys, replace arrays) strategies using jsoncpp
- [ ] 3.2 Add private helper `bool applyDobbySpecOverride(const string& appInstanceId, std::string& dobbySpec)` that: checks `mDobbySpecOverrides` for the instance, parses base spec and overlay, calls `mergeJsonSpec`, serialises merged result back to `dobbySpec`, erases the override from the map, returns true if override was applied
- [ ] 3.3 In `Run()`, after successful `generate()` and before `StartContainerFromDobbySpec`, call `applyDobbySpecOverride(appInstanceId, dobbySpec)` and log whether an override was applied

## 4. Cleanup

- [ ] 4.1 Remove the `/tmp/specchange` debug workaround block from `RuntimeManagerImplementation::Run()` (the `std::ifstream inFile("/tmp/specchange")` block at line ~545)

## 5. Tests

- [ ] 5.1 Add L0 test: `SetDobbySpecOverride` with valid inputs stores override and returns `ERROR_NONE`
- [ ] 5.2 Add L0 test: `SetDobbySpecOverride` with empty `appInstanceId` returns `INVALID_ARGUMENT`
- [ ] 5.3 Add L0 test: `SetDobbySpecOverride` with malformed JSON returns `INVALID_ARGUMENT`
- [ ] 5.4 Add L0 test: calling `SetDobbySpecOverride` twice for same instance retains only the latest override
- [ ] 5.5 Add L0 test: `ClearDobbySpecOverride` removes a pending override and returns `ERROR_NONE`
- [ ] 5.6 Add L0 test: `ClearDobbySpecOverride` on non-existent override returns `NOT_FOUND`
- [ ] 5.7 Add L0 test: `Run()` with `append` override deep-merges new keys without removing existing spec fields
- [ ] 5.8 Add L0 test: `Run()` with `replace` override overwrites matching keys in the base spec
- [ ] 5.9 Add L0 test: `Run()` with no registered override leaves spec unchanged
- [ ] 5.10 Add L0 test: override is consumed after `Run()` so subsequent run for same instance uses no override
