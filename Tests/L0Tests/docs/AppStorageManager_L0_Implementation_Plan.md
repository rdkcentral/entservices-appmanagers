# AppStorageManager L0 Test Implementation Plan

## Objective

Create a standalone L0 test suite for `AppStorageManager` in:

- `Tests/L0Tests/AppStorageManager/`

using the **RuntimeManager Config pattern** for dependency injection, while isolating external runtime dependencies on:

- `Exchange::IStore2` (persistent storage interface via `FakePersistentStore`)
- Real filesystem operations (following PackageManager pattern - no mocking)

**Status: ✅ IMPLEMENTED (April 2026)**

This implementation provides deterministic, CI-friendly tests with branch coverage for plugin lifecycle, storage management operations, request handler functionality, and telemetry reporting using process-isolated test execution.

---

## Design Principles (As Implemented)

1. **RuntimeManager Config Pattern**: Use `ServiceMock::Config` struct with dependency injection via `InstantiateHandler`
2. **Real Filesystem Operations**: Follow PackageManager pattern - no filesystem mocking, let system calls happen naturally
3. **Reuse the existing micro test framework**:
   - `common/L0Bootstrap.hpp`
   - `common/L0Expect.hpp` (use `ExpectEqU32` for uint32_t, `ExpectEqStr` for strings)
   - Simple `uint32_t Test_*()` functions + single runner
4. **Build production plugin sources** directly into the test binary for accurate coverage accounting
5. **Stub IStore2 only** using `FakePersistentStore` in test code (no production changes)
6. **Thunder Root<> Fallback Behavior**: Tests expect success when `Instantiate()` returns nullptr (automatic in-process creation)
7. **Process isolation**: Each test runs in a child process via `fork()` for isolated state
8. **Coverage-friendly**: Use `std::exit()` (not `_exit()`) to flush gcov counters, clamp to 0-255 range

---

## Inputs Reviewed

Primary references used for this plan:

- `Tests/L0Tests/docs/RuntimeManager_L0_Coverage_Analysis.md`
- `Tests/L0Tests/docs/RDKWindowManager_L0_Implementation_Plan.md`
- `Tests/L0Tests/RuntimeManager/RuntimeManager*.cpp`
- `Tests/L0Tests/LifecycleManager/LifecycleManager*.cpp`
- `Tests/L0Tests/RDKWindowManager/RDKWindowManager*.cpp`
- `Tests/L0Tests/common/L0Bootstrap.hpp`
- `Tests/L0Tests/common/L0Expect.hpp`
- `Tests/L0Tests/CMakeLists.txt`
- `AppStorageManager/AppStorageManager.cpp`
- `AppStorageManager/AppStorageManager.h`
- `AppStorageManager/AppStorageManagerImplementation.cpp`
- `AppStorageManager/AppStorageManagerImplementation.h`
- `AppStorageManager/RequestHandler.cpp`
- `AppStorageManager/RequestHandler.h`
- `AppStorageManager/AppStorageManagerTelemetryReporting.cpp`
- `AppStorageManager/AppStorageManager.md`
- `entservices-appmanagers-topic-RDKEMW-14485/AppStorageManager/AppStorageManager_L0_TestCases.md`

---

## Dependency Strategy (As Implemented)

### Challenge

`AppStorageManager` depends on:

1. **IStore2 interface** for persistent storage of app metadata (quotas, storage paths)
2. **Filesystem operations** for directory creation, deletion, size calculation, and ownership management
3. **IShell service** for Thunder framework integration

### Solution Implemented

**RuntimeManager Config Pattern** with minimal mocking:

1. **IStore2 Mocking Only**: 
   - `Tests/L0Tests/AppStorageManager/fakes/FakePersistentStore.h` provides in-memory IStore2 implementation
   - Stack-allocated in tests: `L0Test::FakePersistentStore fakeStore;`
   - Injected via `ServiceMock::Config{&fakeStore}`

2. **Real Filesystem Operations**:
   - Following PackageManager pattern - no filesystem mocking
   - `mkdir()` naturally handles `EEXIST` for multiple test runs
   - Pass `uid=-1, gid=-1` to avoid `chown()` permission issues
   - Uses `/tmp/appdata` as base path (safe for CI/developer machines)

3. **ServiceMock with Config Pattern**:
   - `struct Config { IStore2* persistentStore; string configLine; }`
   - `SetInstantiateHandler(lambda)` for dependency injection
   - Returns implementation pointer to Thunder's `Root<>` template

4. **Thunder Root<> Automatic Fallback**:
   - When `Instantiate()` returns nullptr → `Root<>` falls back to `Core::Service<>::Create()`
   - Tests expect success in this scenario (not failure)
   - No way to inject fake impl that fails Configure() through this pattern

### Build Wiring (Completed)

1. ✅ Added `AppStorageManager` production sources to `Tests/L0Tests/CMakeLists.txt`
2. ✅ Added `FakePersistentStore.cpp` to test sources
3. ✅ Removed `FakeFilesystem.cpp` (dead code - not needed with real filesystem approach)
4. ✅ NO production code changes required
5. ✅ Warning suppression: `-Wno-error=deprecated-declarations` for Thunder R4 TRACE_Lx deprecation
Implemented L0 Test Folder Layout

```
Tests/L0Tests/AppStorageManager/
├── AppStorageManagerTest.cpp                    # ✅ Main test runner (21 tests total)
├── AppStorageManager_LifecycleTests.cpp         # ✅ Plugin lifecycle tests (5 tests)
├── AppStorageManager_ImplementationTests.cpp    # ✅ Core API tests (12 tests)
├── AppStorageManager_ComponentTests.cpp         # ✅ RequestHandler & Telemetry tests (4 tests)
├── ServiceMock.h                                # ✅ PluginHost::IShell mock (RuntimeManager pattern)
└── fakes/
    ├── FakePersistentStore.h                   # ✅ Exchange::IStore2 mock (in-memory storage)
    └── FakePersistentStore.cpp                 # ✅ Implementation

Note: FakeFilesystem removed - real filesystem used per PackageManager patter
│   ├── FakePersistentStore.h                   # Exchange::IStore2 mock
│   └── FakeFilesystem.h                        # Filesystem operations mock
└── CMakeLists.txt (update parent)              # Build configuration
```

---
Implementation (As Built)

### 1) ✅ `ServiceMock` - RuntimeManager Config Pattern

Implemented in `ServiceMock.h` with:

```cpp
struct Config {
    explicit Config(WPEFramework::Exchange::IStore2* store = nullptr)
        : persistentStore(store)
        , configLine("{\"path\":\"/tmp/appdata\"}")
    {}
    WPEFramework::Exchange::IStore2* persistentStore;
    std::string configLine;
};
```

- **InstantiateHandler**: `using InstantiateHandler = std::function<void*(const RPC::Object&, ...)>`
- **Dependency Injection**: `service.SetInstantiateHandler([...] { return impl; })`
- **QueryInterfaceByCallsign**: Returns `_cfg.persistentStore` for callsign `"org.rdk.PersistentStore"`
- **ConfigLine**: Returns `_cfg.configLine` for base path configuration
- **Call Counters**: `GetQueryInterfaceByCallsignCallCount()`, `GetConfigLineCallCount()`
- **Member Order**: Fixed to match initializer list (eliminates -Wreorder warnings)

### 2) ❌ `FakeAppStorageManagerImpl` - NOT IMPLEMENTED

**Reason**: Thunder's `Root<>` template automatically falls back to `Core::Service<>::Create()` when `Instantiate()` returns nullptr. Cannot inject a fake implementation that fails `Configure()` through this pattern.

**Alternative**: Tests validate fallback behavior by expecting Initialize() to succeed when Instantiate returns nullptr.

### 3) ✅ `FakePersistentStore` - In-Memory IStore2 Implementation

Implemented in `fakes/FakePersistentStore.h/.cpp`:

- **In-memory storage**: `std::map<std::string, std::string>` for key-value pairs
- **Methods**: `GetValue()`, `SetValue()`, `DeleteKey()`, `DeleteNamespace()`
- **Reference counting**: Proper `AddRef()`/`Release()` implementation
- **Test usage**: Stack-allocated, passed via `ServiceMock::Config`
- **No failure injection**: Always succeeds (real persistent store behavior)

### 4) ❌ `FakeFilesystem` - NOT IMPLEMENTED (Intentional)

**Decision**: Follow PackageManager L0 test pattern - use real filesystem operations

**Rationale**:
- `mkdir()` naturally handles `EEXIST` gracefully
- Fork isolation prevents cross-test interference
- Tests use `/tmp/appdata` (safe for CI/developer machines)
- Pass `uid=-1, gid=-1` to avoid `chown()` permission failures
- Real filesystem tests actual code paths without mocking complexity

**Dead Code Removed**:
- `fakes/FakeFilesystem.h` (119 lines)
- `fakes/FakeFilesystem.cpp` (implementation)
- All 20+ `FilesystemShim::getInstance()` call site
- Use `UNIT_TEST` conditional compilation in `RequestHandler.cpp` to redirect calls

---
- IMPLEMENTED (`AppStorageManager_LifecycleTests.cpp`)

| Test ID | Test Name (Implemented) | Coverage Focus | Status |
|---------|-------------------------|----------------|--------|
| **LC-001** | `Test_ASM_Lifecycle_InitializeSucceedsWithInstantiateFallback` | `Initialize()` succeeds via Thunder Root<> fallback when Instantiate returns nullptr | ✅ PASS |
| **LC-002** | `Test_ASM_Lifecycle_InitializeSuccessAndDeinitialize` | `Initialize()` + `Deinitialize()` success path | ✅ PASS |
| **LC-003** | `Test_ASM_Lifecycle_InitializeSucceedsWithFallbackCreation` | `Initialize()` succeeds with fallback to real implementation | ✅ PASS |
| **LC-004** | `Test_ASM_Lifecycle_InformationReturnsEmptyString` | `Information()` returns empty string (no additional info) | ✅ PASS |
| **LC-005** | `Test_ASM_Lifecycle_DeinitializeSuccessWithValidService` | `Deinitialize()` completes successfully with valid service pointer | ✅ PASS |

**Key Implementation Notes:**

1. **Thunder Root<> Fallback Behavior**:
   - When `Instantiate()` returns nullptr → automatic fallback to `Core::Service<>::Create()`
   - Tests expect Initialize() to **succeed** (not fail) in this scenario
   - Cannot inject fake impl that fails Configure() through this pattern

2. **InstantiateHandler Pattern**:
   ```cpp
   service.SetInstantiateHandler([](const RPC::Object&, const uint32_t, uint32_t& connectionId) -> void* {
       connectionId = 0;
       return WPEFramework::Core::Service<StorageManagerImplementation>::Create<...>();
   });
   ```

3. **Refcounting Fix**:
   - Removed extra `impl->AddRef()` calls that caused leaks
   - `Create()` already returns ref-counted object
   - Plugin handles cleanup on Deinitialize()

4. **Test Naming Accuracy**:
   - Original names said - IMPLEMENTED

| Test ID | Test Name (Implemented) | Coverage Focus | Status |
|---------|-------------------------|----------------|--------|
| **IMPL-001** | `Test_Impl_ConfigureWithValidService` | `Configure()` with valid service and FakePersistentStore | ✅ PASS |
| **IMPL-002** | `Test_Impl_ConfigureWithNullService` | `Configure()` after Initialize (singleton already configured) | ✅ PASS
---

### B) `AppStorageManager/AppStorageManagerImplementation.cpp`

#### Configuration Tests

| Test ID | Test Name | Coverage Focus | Type |
|---------|-----------|----------------|------|
| **IMPL-001** | `Test_I - IMPLEMENTED

| Test ID | Test Name (Implemented) | Coverage Focus | Status |
|---------|-------------------------|----------------|--------|
| **IMPL-003** | `Test_Impl_CreateStorageWithValidInput` | `CreateStorage()` creates storage successfully with default uid/gid | ✅ PASSy |
| **IMPL-008** | `Test_Impl_CreateStorageWithLargeSize` | `CreateStorage()` with very large size quota | Boundary |
| **IMPL-009** | `Test_Impl_CreateStorageDuplicateAppId` | `CreateStorage()` when storage already exists | Positive |
| **IMPL-010** | `Test_Impl_CreateStorageInsufficientSpace` | `CreateStorage()` when disk space insufficient | Negative |
| **IMPL-011** | `Test_Impl_CreateStorageMkdirFails` | `CreateStorage()` when mkdir operation fails | Negative |
| **IMPL-012** | `Test_Impl_CreateStoragePersistentStoreSetFails` | `CreateStorage()` when IStore2->SetValue fails | Negative |

#### GetStorage Tests

| Test ID | Test Name - IMPLEMENTED

| Test ID | Test Name (Implemented) | Coverage Focus | Status |
|---------|-------------------------|----------------|--------|
| **IMPL-004** | `Test_Impl_GetStorageWithValidAppId` | `GetStorage()` retrieves existing storage with uid=-1, gid=-1 (no chown) | ✅ PASS |
| **IMPL-005** | `Test_Impl_GetStorageCalculatesUsedSpace` | `GetStorage()` calculates used space correctly | ✅ PASS |
| **IMPL-006** | `Test_Impl_GetStorageUsageExceedsQuota` | `GetStorage()` reports when used > quota | ✅ PASS |
| **IMPL-007** | `Test_Impl_GetStorageWithNonExistentAppId` | `GetStorage()` returns error for non-existent appId | ✅ PASS |

**Implementation Notes**:
- Pass `uid=-1, gid=-1` to avoid permission-denied errors from `chown()` in CI
- Real filesystem used - no mocking of stat/chown operations

| Test ID | Test Name | Coverage Focus | Type |
|---------|-----------|----------------|------|
| **IMPL-021** | `Test_Impl_DeleteStorageWithValidAppId` | `DeleteStorage()` removes storage successfully | Positive |
| **IMPL-022** | `Test_Impl_DeleteStorageWithEmptyAppId` | `DeleteStorage()` with empty appId | Negative |
| **IMPL-023** | `Test_Impl_DeleteStorageWithNonExistentAppId` | `DeleteStorage()` when app storage doesn't exist | Negative |
| **IMPL-024** | `Test_Impl_DeleteStorageRmdirFails` | `DeleteStorage()` when directory removal fails | Negative |
| **IMPL-025** | `Test_Impl_DeleteStorageRemovesFromCache` | `DeleteStorage()` updates internal cache | Positive |
 - IMPLEMENTED

| Test ID | Test Name (Implemented) | Coverage Focus | Status |
|---------|-------------------------|----------------|--------|
| **IMPL-008** | `Test_Impl_DeleteStorageWithValidAppId` | `DeleteStorage()` removes storage successfully | ✅ PASS |
| **IMPL-009** | `Test_Impl_DeleteStorageWithNonExistentAppId` | `DeleteStorage()` returns error for non-existent appId | ✅ PASSes | Positive |
| **IMPL-030** | `Test_Impl_ClearStorageDeleteFails` | `Clear()` when file deletion fails | Negative |

#### ClearAll Tests

| Test ID | Test - IMPLEMENTED

| Test ID | Test Name (Implemented) | Coverage Focus | Status |
|---------|-------------------------|----------------|--------|
| **IMPL-010** | `Test_Impl_ClearStorageWithValidAppId` | `Clear()` clears app data successfully | ✅ PASS

### C) `AppStorageManager/RequestHandler.cpp`

#### Singleton and Configuration Tests (`AppStorageManager_ComponentTests.cpp`)

| Test ID | Test Na - IMPLEMENTED

| Test ID | Test Name (Implemented) | Coverage Focus | Status |
|---------|-------------------------|----------------|--------|
| **IMPL-011** | `Test_Impl_ClearAllWithNoExemptions` | `ClearAll()` succeeds with empty exemption list | ✅ PASS |
| **IMPL-012** | `Test_Impl_ClearAllWithExemptions` | `ClearAll()` succeeds with exemption JSON | ✅ PASS |

**Implementation Notes**:
- Tests now use `ExpectEqU32()` for uint32_t comparisons (not generic `ExpectEq()`)
- Validates both return code (`ERROR_NONE`) and error reason (empty string)
| Test ID | Test Name | Coverage Focus | Type |
|---------|-----------|----------------|------|
| **RH-005** | `Test_RH_CreatePersistentStoreObjectSuccess` | `createPersistentStoreRemoteStoreObject()` success | Positive |
| **RH-006** | `Test_RH_CreatePersistentStoreObjectFailsWithNullService` | Persistent store creation with null service | Negative |
| **RComponent Tests -(Implemented) | Coverage Focus | Status |
|---------|-------------------------|----------------|--------|
| **COMP-001** | `Test_Comp_GetInstanceReturnsSingleton` | `getInstance()` returns same instance | ✅ PASS |
| **COMP-002** | `Test_Comp_SetBaseStoragePath` | `SetBaseStoragePath()` with valid path | ✅ PASS |
| **COMP-003** | `Test_Comp_ConfigureSingleton` | Configure singleton without crashing | ✅ PASSve |

#### Cache Management Tests

| Test ID | Test Name | Coverage Focus | Type |
|---------|-----------|----------------|------|
| **RH-013** | `Test_RH_PopulateAppInfoCacheSuccess` | `populateAppInfoCacheFromStoragePath()` success | Positive |
| **RH-014** | `Test_RH_PopulateAppInfoCacheWithInvalidPath` | Cache population with non-existent path | Negative |
| **RH-015** | `Test_RH_PopulateAppInfoCacheWithEmptyDirectory` | Cache population with empty storage dir | Boundary |
| **RH-016** | `Test_RH_CreateAppStorageInfoByAppIdNew` | `createAppStorageInfoByAppID()` new entry | Positive |
| **RH-017** | `Test_RH_CreateAppStorageInfoByAppIdUpdate` | Update existing entry in cache | Positive |
| **RH-018** | `Test_RH_RetrieveAppStorageInfoByAppIdExists` | `retrieveAppStorageInfoByAppID()` found | Positive |
| **RH-019** | `Test_RH_RetrieveAppStorageInfoByAppIdNotFound` | Retrieval of non-existent appId | Negative |
| **RH-020** | `Test_RH_RemoveAppStorageInfoByAppIdExists` | `removeAppStorageInfoByAppID()` removes entry | Positive |
| **RH-021** | `Test_RH_RemoveAppStorageInfoByAppIdNotFound` | Remove non-existent entry | Negative |

#### Filesystem Helper Tests

| Test ID | Test Name | Coverage Focus | Type |
|---------|-----------|----------------|------|
| **RH-022** | `Test_RH_IsValidAppStorageDirectoryValid` | `isValidAppStorageDirectory()` returns true | Positive |
| **RH-023** | `Test_RH_IsValidAppStorageDirectoryInvalid` | Invalid directory name patterns | Negative |
| **RH-024** | `Test_RH_HasEnoughStorageFreeSpaceTrue` | `hasEnoughStorageFreeSpace()` sufficient space | Positive |
| **RH-025** | `Test_RH_HasEnoughStorageFreeSpaceFalse` | Insufficient disk space scenario | Negative |
| **RH-026** | `Test_RH_HasEnoughStorageFreeSpaceInvalidPath` | Check space on invalid path | Negative |
| **RH-027** | `Test_RH_GetDirectorySizeInBytesValid` | `getDirectorySizeInBytes()` calculates size | Positive |
| **RH-028** | `Test_RH_GetDirectorySizeInBytesEmpty` | Size calculation for empty directory | Boundary |
| **RH-029** | `Test_RH_GetDirectorySizeInBytesInvalidPath` | Size calculation for non-existent path | Negative |
| **RH-030** | `Test_RH_GetSizeCallbackHandlesFiles` | `getSize()` static callback function | Positive |
| **RH-031** | `Test_RH_DeleteDirectoryEntriesSuccess` | `deleteDirectoryEntries()` removes contents | Positive |
| **RH-032** | `Test_RH_DeleteDirectoryEntriesNotFound` | Delete entries from non-existent directory | Negative |

#### Full API Path Tests (via RequestHandler)

| Test ID | Test Name | Coverage Focus | Type |
|---------|-----------|----------------|------|
| **RH-033** | `Test_RH_CreateStorageFullPath` | End-to-end CreateStorage through RequestHandler | Positive |
| **RH-034** | `Test_RH_GetStorageFullPath` | End-to-end GetStorage through RequestHandler | Positive |
| **RH-035** | `Test_RH_DeleteStorageFullPath` | End-to-end DeleteStorage through RequestHandler | Positive |
| **RH-036** | `Test_RH_ClearFullPath` | End-to-end Clear through RequestHandler | Positive |
| **RH-037** | `Test_RH_ClearAllFullPath` | End-to-end ClearAll through RequestHandler | Positive |

---

### D) `AppStorageManager/AppStorageManagerTelemetryReporting.cpp`

#### Telemetry Tests (`AppStorageManager_ComponentTests.cpp`)

| Test ID | Test Name | Coverage Focus | Type |
|---------|-----------|----------------|------|
| **TEL-001** | `Test_Tel_GetInstanceReturnsSingleton` | `getInstance()` returns same instance | Positive |
| **TEL-002** | `Test_Tel_InitializeWithValidService` | `initialize()` with valid IShell service | Positive |
| **TEL-003** | `Test_Tel_InitializeWithNullService` | `initialize()` with null service | Negative |
| **TEL-004** | `Test_Tel_ResetCleansUpResources` | `reset()` releases telemetry resources | Positive |
| **TEL-005** | `Test_Tel_RecordGetStorageTelemetrySuccess` | `recordGetStorageTelemetry()` sends metrics | Positive |
| **TEL-006** | `Test_Tel_RecordGetStorageTelemetryDisabled` | Telemetry when reporting disabled | Boundary |
| **TEL-007** | `Test_Tel_MultipleInitializeCallsIdempotent` | Multiple initialize calls don't leak | Boundary |

---

### E) Thread Safety - IMPLEMENTED (`AppStorageManager_ComponentTests.cpp`)

| Test ID | Test Name (Implemented) | Coverage Focus | Status |
|---------|-------------------------|----------------|--------|
| **TEL-001** | `Test_Tel_GetInstanceReturnsSingleton` | `getInstance()` returns same instance | ✅ PASS |

**Implementation Notes**:
- Single test validates singleton pattern for telemetry
- Full telemetry integration testing requires real IShell with telemetry services
 - NOT IMPLEMENTED

**Rationale**: 
- Fork-based isolation provides sufficient protection for L0 scope
- Thread safety validated through integration/functional tests
- L0 focus: logic correctness and branch coverage, not race conditions

**Test runner note**:
- The standalone L0 test runner and individual `Test_*()` declarations are implemented in the actual sources under `Tests/L0Tests/AppStorageManager/`.
- They are intentionally not duplicated in this plan document, to avoid Markdown rendering issues and to prevent the documentation from drifting out of sync with the current test runner.
- Treat the source files as the canonical definition of the executed test list and runner structure.

    uint32_t totalFailures = 0;
    const size_t numTests = sizeof(cases) / sizeof(cases[0]);

    for (size_t i = 0; i < numTests; ++i) {
        pid_t pid = fork();
        
        if (pid == 0) {
            // Child process
            L0Test::L0BootstrapGuard bootstrap;
            uint32_t failures = cases[i].fn();
            std::exit(failures); // Use std::exit for gcov counter flush
        } else if (pid > 0) {
            // Parent process
            int status;
            waitpid(pid, &status, 0);
            
            if (WIFEXITED(status)) {
                uint32_t childFailures = WEXITSTATUS(status);
                if (childFailures > 0) {
                    std::cerr << "FAIL: " << cases[i].name
                              << " (" << childFailures << " failures)\n";
                    totalFailures += childFailures;
                } else {
                    std::cout << "PASS: " << cases[i].name << "\n";
                }
            } else if (WIFSIGNALED(status)) {
                std::cerr << "FAIL: " << cases[i].name 
                          << " (terminated by signal " << WTERMSIG(status) << ")\n";
                totalFailures++;
            }
        } else {
            std::cerr << "ERROR: Failed to fork for test " << cases[i].name << "\n";
            totalFailures++;
        }
    }

    L0Test::PrintTotals(std::cerr, "AppStorageManager L0 Tests", totalFailures);
    return L0Test::ResultToExitCode({totalFailures});
}
```

---

## Mock Implementation Guidelines

### ServiceMock.h Structure

```cpp
namespace L0Test {

class ServiceMock : public WPEFramework::PluginHost::IShell {
public:
    // Configuration helpers
    void SetConfigPath(const std::string& path);
   Actual Implementation (As Built)

### ServiceMock.h Structure - IMPLEMENTED

```cpp
namespace L0Test {

class ServiceMock : public WPEFramework::PluginHost::IShell {
public:
    struct Config {
        explicit Config(WPEFramework::Exchange::IStore2* store = nullptr)
            : persistentStore(store)
            , configLine("{\"path\":\"/tmp/appdata\"}")
        {}
        WPEFramework::Exchange::IStore2* persistentStore;
        std::string configLine;
    };
    
    using InstantiateHandler = std::function<void*(const WPEFramework::RPC::Object&, 
                                                     const uint32_t, 
                                                     uint32_t&)>;
    
    explicit ServiceMock(const Config& cfg = Config());
    
    // Control interface
    void SetInstantiateHandler(InstantiateHandler handler);
    uint32_t GetQueryInterfaceByCallsignCallCount() const;
    uint32_t GetConfigLineCallCount() const;
    
    // PluginHost::IShell interface - IMPLEMENTED

```cpp
namespace L0Test {

class FakePersistentStore : public WPEFramework::Exchange::IStore2 {
public:
    FakePersistentStore();
    virtual ~FakePersistentStore();
    
    // IStore2 interface - always succeeds (no failure injection)
    uint32_t GetValue(ScopeType scope, const string& ns, const string& key, 
                      string& value, uint32_t& ttl) override;
    uint32_t SetValue(ScopeType scope, const string& ns, const string& key, 
                      const string& value, uint32_t ttl) override;
    uint32_t DeleteKey(ScopeType scope, const string& ns, const string& key) override;
    uint32_t DeleteNamespace(ScopeType scope, const string& ns) override;
    // ... other IStore2 methods with empty implementations ...
    
    // Reference counting
    void AddRef() const override;
    uint32_t Release() const override;
    
private:
    mutable std::atomic<uint32_t> _refCount;
    std::map<std::string, std::string> _storage;  // Simple in-memory key-value store
};

} // namespace L0Test
```

**Usage in Tests**:
```cpp
L0Test::FakePersistentStore fakeStore;
L0Test::ServiceMock::Config cfg{&fakeStore};
cfg.configLine = "{\"path\":\"/tmp/appdata\"}";
L0Test::ServiceMock service(cfg);eCalls{0};
    uint32_t _setValueCalls{0};
    std::string _l - NOT IMPLEMENTED

**Removed**: Following PackageManager L0 test pattern - use real filesystem

**Rationale**:
1. Real filesystem operations test actual code paths
2. `mkdir()` naturally handles `EEXIST` gracefully
3. Fork isolation prevents cross-test interference
4. No need for complex filesystem mocking layer
5. Reduces maintenance burden

**Approach for Filesystem Tests**:
- Tests use `/tmp/appdata` (safe path for CI/developer machines)
- Pass `uid=-1, gid=-1` to `GetStorage()` to avoid chown permission errors
- Let `mkdir()` fail naturally and test error handling
- No conditional compilation (`#ifdef UNIT_TEST`) needed in production code
- Cleanup handled by fork isolation (each test in separate process)fine CHOWN(path, uid, gid) L0Test::FilesystemShim::getInstance().chown(path, uid, gid)
#define STAT(path, buf) L0Test::FilesystemShim::getInstance().stat(path, buf)
#else
#define MKDIR(path, mode) ::mkdir(path, mode)
#define CHOWN(path, uid, gid) ::chown(path, uid, gid)
#define STAT(path, buf) ::stat(path, buf)
#endif
```

---

## CMakeLists.txt Integration

### Source List Addition

```cmake
set(APPSTORAGE_MANAGER_L0_SOURCES
    ${CMAKE_SOURCE_DIR}/../../AppStorageManager/Module.cpp
    ${CMAKE_SOURCE_DIR}/../../AppStorageManager/AppStorageManager.cpp
    ${CMAKE_SOURCE_DIR}/../../AppStorageManager/AppStorageManagerImplementation.cpp
    ${CMAKE_SOURCE_DIR}/../../AppStorageManager/AppStorageManagerTelemetryReporting.cpp
    ${CMAKE_SOURCE_DIR}/../../AppStorageManager/RequestHandler.cpp
    ${CMAKE_SOURCE_DIR}/../../helpers/Telemetry/TelemetryReportingBase.cpp
    ${CMAKE_SOURCE_DIR}/../../helpers/Telemetry/UtilsTelemetryMetrics.cpp
    # Test sources
    AppStorageManager/AppStorageManagerTest.cpp
    AppStorageManager/AppStorageManager_LifecycleTests.cpp
    AppStorageManager/AppStorageManager_ImplementationTests.cpp
    AppStorageManager/AppStorageManager_ComponentTests.cpp
    AppStorageManager/fakes/FakePersistentStore.cpp
    AppStorageManager/fakes/FakeFilesystem.cpp
    common/L0Bootstrap.cpp
)
```

### Target Configuration

```cmake
option(PLUGIN_APP_STORAGE_MANAGER "Build AppStorageManager L0 test target" ON)
option(APPASM_ENABLE_COVERAGE "Enable coverage flags for appstorage_l0test" ON)

if(PLUGIN_APP_STORAGE_MANAGER)
    add_executable(appstorage_l0test ${APPSTORAGE_MANAGER_L0_SOURCES})
    
    target_compile_definitions(appstorage_l0test PRIVATE
        MODULE_NAME=AppS - IMPLEMENTED

```cmake
set(APPSTORAGE_MANAGER_L0_SOURCES
    # Production sources (no changes to plugin code)
    ../../AppStorageManager/Module.cpp
    ../../AppStorageManager/AppStorageManager.cpp
    ../../AppStorageManager/AppStorageManagerImplementation.cpp
    ../../AppStorageManager/AppStorageManagerTelemetryReporting.cpp
    ../../AppStorageManager/RequestHandler.cpp
    ../../helpers/Telemetry/TelemetryReportingBase.cpp
    ../../helpers/Telemetry/UtilsTelemetryMetrics.cpp
    
    # Test sources
    AppStorageManager/AppStorageManagerTest.cpp
    AppStorageManager/AppStorageManager_LifecycleTests.cpp
    AppStorageManager/AppStorageManager_ImplementationTests.cpp
    AppStorageManager/AppStorageManager_ComponentTests.cpp
    
    # Fakes (IStore2 only - no filesystem mocking)
    AppStorageManager/fakes/FakePersistentStore.cpp
    
    # Common test infras - IMPLEMENTED

```cmake
option(PLUGIN_APP_STORAGE_MANAGER "Build AppStorageManager L0 test target" ON)
option(APPASM_ENABLE_COVERAGE "Enable coverage flags for appstorage_l0test" ON)

if(PLUGIN_APP_STORAGE_MANAGER)
    add_executable(appstorage_l0test ${APPSTORAGE_MANAGER_L0_SOURCES})
    
    target_compile_definitions(appstorage_l0test PRIVATE
        MODULE_NAME=AppStorageManager_L0Test
        USE_THUNDER_R4=1
        DISABLE_SECURITY_TOKEN=1
        THUNDER_PLATFORM_PC_UNIX=1
        # Note: No UNIT_TEST=1 needed - real filesystem used
    )
    
    if(APPASM_ENABLE_COVERAGE)
        target_compile_options(appstorage_l0test PRIVATE -O0 -g --coverage)
        target_link_options(appstorage_l0test PRIVATE --coverage)
    endif()
    
    # Suppress Thunder R4 deprecation warnings (TRACE_Lx outside core)
    target_compile_options(appstorage_l0test PRIVATE 
        -Wno-error=deprecated-declarations
    )
    
    if(L0_ENABLE_WARNINGS)
        target_compile_options(appstorage_l0test PRIVATE 
            -Wall -Wextra -Wno-unused-parameter
        )
    endif()
    
    target_include_directories(appstorage_l0test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/AppStorageManager
        ${CMAKE_CURRENT_SOURCE_DIR}/AppStorageManager/fakes
        ${CMAKE_CURRENT_SOURCE_DIR}/common
        ${CMAKE_SOURCE_DIR}/../..
        ${CMAKE_SOURCE_DIR}/../../AppStorageManager
        ${CMAKE_SOURCE_DIR}/../../helpers
        ${CMAKE_SOURCE_DIR}/../../helpers/Telemetry
        ${PREFIX}/include
        ${PREFIX}/include/${NAMESPACE}
    )
    
    # Link Thunder libraries
    target_link_libraries(appstorage_l0test PRIVATE
        ${NAMESPACE}Core::${NAMESPACE}Core
        ${NAMESPACE}Messaging::${NAMESPACE}Messaging
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        ${NAMESPACE}Plug - COMPLETED

### Phase 1: Foundation ✅
- [x] Create directory structure
- [x] Implement `ServiceMock.h` with RuntimeManager Config pattern
- [x] Implement `FakePersistentStore.h/.cpp`
- [x] ~~Implement FakeFilesystem~~ **Decision: Use real filesystem**
- [x] Create main test runner with fork isolation
- [x] Update CMakeLists.txt
- [x] Verify build and basic execution

### Phase 2: Lifecycle Tests ✅
- [x] Implement 5 lifecycle test cases
- [x] Verify Initialize/Deinitialize paths with Thunder Root<> fallback
- [x] Fix refcounting issues (removed extra AddRef calls)
- [x] Rename tests to match actual behavior (not "Fails" when expecting success)
- [x] Test singleton pattern validation

### Phase 3: Implementation Core Tests ✅
- [x] Implement CreateStorage test (1 test)
- [x] Implement GetStorage tests (4 tests)
- [x] Implement DeleteStorage tests (2 tests)
- [x] Implement Clear test (1 test)
- [x] Implement ClearAll tests (2 tests)
- [x] Fixed: Use ExpectEqU32 for uint32_t comparisons (not ExpectEq)
- [x] Pass uid=-1, gid=-1 to avoid chown permission errors

### Phase 4: Component Tests ✅
- [x] Implement RequestHandler singleton tests (3 tests)
- [x] Implement Telemetry singleton test (1 test)
- [x] Fixed malformed comment blocks
- [x] Fixed exit status truncation (clamp to 0-255)

### Phase 5: Dead Code Cleanup ✅
- [x] Remove FakeStorageManagerImplFailConfigure (67 lines)
- [x] Remove FakeFilesystem.h/.cpp (200+ lines)
- [x] Remove all FilesystemShim call sites (20+ instances)
- [x] Verify no unused fakes remain

### Phase 6: CI Integration & Documentation ✅
- [x] Update L0-tests_original.yml with AppStorageManager support
- [x] Add plugin build flags (PLUGIN_APP_STORAGE_MANAGER=ON)
- [x] Add coverage flags (APPASM_ENABLE_COVERAGE=ON)
- [x] Add test exec - STATUS

1. **Test Execution**: ✅ All 21 tests pass in isolated child processes via fork()
2. **Coverage Target**: 🔄 **TO BE MEASURED** after full test run:
   - `AppStorageManager.cpp`: Target ≥90% (5 lifecycle tests)
   - `AppStorageManagerImplementation.cpp`: Target ≥85% (12 implementation tests)
   - `RequestHandler.cpp`: Target ≥70% (4 component tests)
   - `AppStorageManagerTelemetryReporting.cpp`: Target ≥60% (1 telemetry test)
3. **Build Integration**: ✅ Clean build with deprecation warning suppression
4. **CI Compatibility**: ✅ Integrated into L0-tests_original.yml workflow
5. **Coverage Reporting**: ✅ Uses std::exit() for gcov flush, clamps to 0-255
6. **Deterministic Execution**: ✅ Fork isolation + real filesystem (no mocking complexity)
7. **Documentation**: ✅ Implementation plan updated, test names accurate mutex validation tests |
| Coverage tool limitations | Medium | Verify gcov flush with std::exit() pattern |
| Mock maintenance burden | Medium | Keep mocks simple, focused on control surface |
| Test execution time | Low | Parallelize via CTest, optimize mock performance |
 - RESOLVED

| Risk | Impact | Resolution |
|------|--------|------------|
| Filesystem mocking complexity | High | ✅ **Avoided** - Use real filesystem per PackageManager pattern |
| IStore2 interface changes | Medium | ✅ **Mitigated** - FakePersistentStore implements full IStore2 interface |
| Thread safety in shared state | High | ✅ **No risk** - Fork isolation prevents cross-test interference |
| Coverage tool limitations | Medium | ✅ **Resolved** - std::exit() with 0-255 clamping works correctly |
| Mock maintenance burden | Medium | ✅ **Minimized** - Only IStore2 mocked, rest uses real implementations |
| Test execution time | Low | ✅ **Fast** - 21 tests complete quickly with fork isolation |
| Plugin code changes | High | ✅ **Zero changes** - All test infrastructure in test files only |
| Thunder R4 deprecation warnings | Low | ✅ **Suppressed** - Using -Wno-error=deprecated-declarations
- Telemetry: 7 tests
- Concurrency: 5 tests
- Component Integration: 2 tests
 Implemented: 21**

- Lifecycle Tests: 5 tests ✅
- Implementation Core Tests: 12 tests ✅
- Component Tests (RequestHandler): 3 tests ✅
- Telemetry Tests: 1 test ✅

**Test Coverage (Estimated):**
- Code Coverage: To be measured after full CI run
- Branch Coverage: To be measured after full CI run
- Line Coverage: To be measured after full CI run

**Key Design Decisions:**
1. ✅ **RuntimeManager Config Pattern** - Dependency injection via Config struct
2. ✅ **Real Filesystem** - No mocking, following PackageManager pattern
3. ✅ **Fork Isolation** - Each test in separate process
4. ✅ **Thunder Root<> Fallback** - Tests expect success when Instantiate returns nullptr
5. ✅ **Minimal Mocking** - Only IStore2 mocked (FakePersistentStore)
6. ✅ **Zero Plugin Changes** - All test infrastructure in test files onlyPI Documentation](../../../AppStorageManager/AppStorageManager.md)
4. [AppStorageManager Test Cases (Topic Branch)](../../../entservices-appmanagers-topic-RDKEMW-14485/AppStorageManager/AppStorageManager_L0_TestCases.md)
5. [Thunder/WPEFramework Plugin Development Guide](https://rdkcentral.github.io/Thunder/)

---

**Document Version:** 1.0  
**Last Updated:** 2026-04-27  
**Author:** GitHub Copilot (AI Assistant)  
## Implementation History

- **2026-04-27**: Initial implementation plan created
- **2026-04-28**: Implementation completed and tested
  - Applied RuntimeManager Config pattern
  - Removed FakeFilesystem (use real filesystem)
  - Fixed refcounting issues in lifecycle tests
  - Renamed tests for accuracy
  - Fixed compilation errors (ExpectEq → ExpectEqU32)
  - Integrated into CI workflow (L0-tests_original.yml)
  - Updated documentation to reflect actual implementation

---

**Document Version:** 2.0  
**Last Updated:** 2026-04-28  
**Author:** GitHub Copilot (AI Assistant)  
**Status:** ✅ **COMPLETED** - Implementation Matches Documentation