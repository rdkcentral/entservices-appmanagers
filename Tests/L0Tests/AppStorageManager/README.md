# AppStorageManager L0 Unit Tests

This directory contains L0 (unit level) tests for the AppStorageManager plugin.

## Overview

The L0 tests verify the functionality of the AppStorageManager plugin at the unit level, covering:
- Plugin lifecycle (Initialize, Deinitialize, Information)
- Storage operations (CreateStorage, GetStorage, DeleteStorage, Clear, ClearAll)
- RequestHandler helper methods
- Telemetry reporting

## Test Structure

```
AppStorageManager/
├── AppStorageManagerTest.cpp                   # Main test runner
├── AppStorageManager_LifecycleTests.cpp        # Plugin lifecycle tests
├── AppStorageManager_ImplementationTests.cpp   # Core storage operation tests
├── AppStorageManager_ComponentTests.cpp        # Helper component tests
├── ServiceMock.h                               # Mock IShell implementation
├── fakes/
│   └── FakePersistentStore.h                   # Mock IStore2 implementation
└── README.md                                   # This file
```

## Test Categories

### Lifecycle Tests (5 tests)
- Initialize with null root
- Initialize success and deinitialize
- Initialize fails when IConfiguration missing
- Initialize fails when Configure() returns error
- Information returns service name

### Implementation Tests (17 tests)
- Configure with valid/null service
- CreateStorage: valid input, empty appId, zero size, duplicate appId
- GetStorage: valid appId, empty appId, non-existent appId
- DeleteStorage: valid appId, empty appId, non-existent appId
- Clear: valid appId, empty appId
- ClearAll: no exemptions, with exemptions, invalid JSON format

### Component Tests (7 tests)
- RequestHandler: getInstance, SetBaseStoragePath, CreatePersistentStore, isValidAppStorageDirectory
- Telemetry: getInstance

**Total: 29 tests**

## Building the Tests

### Prerequisites
- CMake 3.16 or higher
- Thunder/WPEFramework R4.4.1 or compatible
- C++17 compiler (gcc 7.5+, clang 6+)
- Required libraries: pthread, dl, m

### Build Commands

```bash
# From the repository root
cd Tests/L0Tests

# Configure
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Debug \
  -DPREFIX=/path/to/thunder/install \
  -DPLUGIN_APP_STORAGE_MANAGER=ON \
  -DAPPSM_ENABLE_COVERAGE=ON

# Build
cmake --build build --target appstoragemanager_l0test -j$(nproc)

# Or use the convenience target
cmake --build build --target run_appstoragemanager
```

### Build Options
- `PLUGIN_APP_STORAGE_MANAGER` - Enable AppStorageManager L0 tests (default: ON)
- `APPSM_ENABLE_COVERAGE` - Enable coverage instrumentation (default: ON)
- `L0_ENABLE_WARNINGS` - Enable additional compiler warnings (default: ON)
- `PREFIX` - Path to Thunder installation prefix

## Running the Tests

### Direct Execution
```bash
cd Tests/L0Tests/build
./appstoragemanager_l0test
```

### Using CMake Target
```bash
cmake --build build --target run_appstoragemanager
```

### Test Output
Tests print results in the following format:
```
[       OK ] ASM_Lifecycle_InitializeSuccessAndDeinitialize
[  FAILED  ] Impl_CreateStorageWithEmptyAppId (2 failures)

==== AppStorageManager L0 Summary ====
Passed: 27
Failed: 2
```

### Exit Codes
- `0` - All tests passed
- `1` - One or more tests failed

## Test Isolation

Each test case runs in a **separate child process** (via `fork()`) to ensure:
- No state pollution between tests
- Proper isolation of Thunder Core worker pools
- Independent coverage data collection

## Code Coverage

### Generate Coverage Report
```bash
# Run tests with coverage enabled
cmake --build build --target appstoragemanager_l0test
./build/appstoragemanager_l0test

# Generate lcov report
lcov -c -o coverage.info -d build/
lcov -r coverage.info '/usr/include/*' '*/Tests/*' -o filtered_coverage.info
genhtml -o coverage_html filtered_coverage.info

# View report
firefox coverage_html/index.html
```

### Coverage Target
The AppStorageManager L0 tests aim for **minimum 75% code coverage** across all source files.

## Mock Objects

### ServiceMock
Mocks `PluginHost::IShell` interface for testing plugin initialization and service interactions.

Key features:
- Configurable instantiate handler for Root<>() calls
- Tracks AddRef/Release calls for reference counting validation
- Supports PersistentStore injection

### FakePersistentStore
Mocks `Exchange::IStore2` interface for testing quota property storage.

Key features:
- In-memory key-value store
- Supports SetValue, GetValue, DeleteKey operations
- Scoped storage (DEVICE scope)

## Debugging Tests

### Run Under GDB
```bash
gdb --args ./build/appstoragemanager_l0test
(gdb) break Test_Impl_CreateStorageWithValidInput
(gdb) run
```

### Verbose Output
Set environment variables for Thunder logging:
```bash
export THUNDER_LOG_LEVEL=4
export THUNDER_LOG_OUTPUT=console
./build/appstoragemanager_l0test
```

### Valgrind Memory Check
```bash
valgrind --leak-check=full --show-leak-kinds=all \
  ./build/appstoragemanager_l0test
```

## Writing New Tests

### Test Function Template
```cpp
uint32_t Test_Your_New_Test()
{
    L0Test::TestResult tr;

    // Setup
    auto* impl = CreateImpl();
    L0Test::ServiceMock service;
    
    // Configure
    service.SetConfigLine("{\"path\":\"/tmp/test\"}");
    impl->Configure(&service);

    // Execute test
    std::string path, errorReason;
    const auto status = impl->CreateStorage("testapp", 1024, path, errorReason);

    // Verify results
    L0Test::ExpectEqU32(tr, status, WPEFramework::Core::ERROR_NONE, 
                       "CreateStorage succeeds");
    L0Test::ExpectTrue(tr, !path.empty(), "Path returned");

    // Cleanup
    impl->Release();
    return tr.failures;
}
```

### Register New Test
1. Declare function in `AppStorageManagerTest.cpp`
2. Add to test cases array in `main()`

## Continuous Integration

The L0 tests run automatically in GitHub Actions on:
- Pull requests
- Commits to develop/main branches
- Manual workflow dispatch

See `.github/workflows/L0-tests.yml` for CI configuration.

## Known Issues & Limitations

1. **Filesystem Operations**: Tests create actual directories in `/tmp/test_appstorage`. Cleanup is performed at test start/end.

2. **PersistentStore Dependency**: Some tests require mocked IStore2. In actual deployments, org.rdk.PersistentStore must be available.

3. **Thread Safety**: Multi-threading tests are limited. Full concurrency testing requires integration tests.

4. **Coverage Gaps**: Some RALF-specific code paths (conditional compilation) have reduced coverage in standard builds.

## Troubleshooting

### Test Fails with "Root<IAppStorageManager>() failed"
- Ensure Thunder libraries are in `LD_LIBRARY_PATH`
- Verify PREFIX points to correct Thunder installation

### Coverage Data Missing
- Ensure built with `-DAPPSM_ENABLE_COVERAGE=ON`
- Check that tests actually execute (don't crash early)

### Fork Failures
- Check system limits: `ulimit -u` (max user processes)
- Reduce parallel test execution if hitting limits

## References

- [AppStorageManager L0 Test Cases Documentation](AppStorageManager_L0_TestCases.md)
- Thunder Documentation: https://github.com/rdkcentral/Thunder
- L0 Test Patterns: See `Tests/L0Tests/RuntimeManager` for additional examples

## Contact

For questions or issues with L0 tests, contact the AppStorageManager maintainers or open an issue in the repository.
