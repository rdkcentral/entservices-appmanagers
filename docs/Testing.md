# Testing Framework Documentation

> Test Infrastructure for ENT Services AppManagers

## 1. Overview

The **entservices-appmanagers** repository includes a comprehensive testing framework organized into three levels:

| Level | Purpose | Location |
|-------|---------|----------|
| L0 Tests | Unit tests for individual functions/classes | `Tests/L0Tests/` |
| L1 Tests | Integration tests for plugin interactions | `Tests/L1Tests/` |
| L2 Tests | System/end-to-end tests | `Tests/L2Tests/` |

---

## 2. Test Directory Structure

```
Tests/
├── L0Tests/                    # Unit tests
├── L1Tests/                    # Integration tests
│   ├── CMakeLists.txt         # Build configuration
│   ├── tests/                 # Test source files
│   │   ├── test_AppManager.cpp
│   │   ├── test_AppStorageManager.cpp
│   │   ├── test_DownloadManager.cpp
│   │   ├── test_LifecycleManager.cpp
│   │   ├── test_PackageManager.cpp
│   │   ├── test_PreinstallManager.cpp
│   │   ├── test_RDKWindowManager.cpp
│   │   ├── test_RunTimeManager.cpp
│   │   ├── test_UtilsFile.cpp
│   │   └── test_Ralf.cpp
│   ├── docs/                  # Test documentation
│   ├── .lcovrc_l1            # Coverage configuration
│   └── WindowManagerMock.h    # Mock implementations
├── L2Tests/                    # System tests
├── mocks/                      # Shared mock implementations
├── patches/                    # Test patches
├── CopilotFiles/              # AI-assisted test files
├── clang.cmake                # Clang build configuration
├── gcc-with-coverage.cmake    # GCC coverage configuration
├── README.md                  # Test documentation
├── rebuild_l1_from_l1build.sh # Rebuild script
├── run_l1_coverage_from_l1build.sh # Coverage script
└── run_l1_from_l1build.sh     # Test execution script
```

---

## 3. L1 Test Files Overview

### test_AppManager.cpp

Tests for the AppManager plugin:

| Test Category | Description |
|---------------|-------------|
| Initialization | Plugin initialization and configuration |
| LaunchApp | Application launch with various parameters |
| CloseApp | Graceful application closure |
| TerminateApp | Forced application termination |
| KillApp | Immediate application kill |
| GetLoadedApps | Retrieve loaded applications list |
| GetInstalledApps | Query installed applications |
| ClearAppData | Clear application storage |
| Properties | Get/set application properties |
| Notifications | Event notification delivery |

### test_LifecycleManager.cpp

Tests for the LifecycleManager plugin:

| Test Category | Description |
|---------------|-------------|
| SpawnApp | Application spawning |
| SetTargetAppState | State transitions |
| UnloadApp | Application unloading |
| KillApp | Forced termination |
| StateTransitions | Valid state transition paths |
| InvalidTransitions | Rejection of invalid transitions |
| Events | Lifecycle event delivery |
| AppReady | Application ready signaling |

### test_RunTimeManager.cpp

Tests for the RuntimeManager plugin:

| Test Category | Description |
|---------------|-------------|
| Run | Container start |
| Suspend/Resume | Container pause/unpause |
| Hibernate/Wake | Container checkpoint/restore |
| Terminate | Graceful container stop |
| Kill | Forced container stop |
| OCI Spec | Specification generation |
| Events | Container event handling |

### test_PackageManager.cpp

Tests for the PackageManager plugin:

| Test Category | Description |
|---------------|-------------|
| Download | Package download |
| Install | Package installation |
| Uninstall | Package removal |
| Lock/Unlock | Package locking mechanism |
| ListPackages | Package enumeration |
| Config | Configuration retrieval |

### test_AppStorageManager.cpp

Tests for the AppStorageManager plugin:

| Test Category | Description |
|---------------|-------------|
| CreateStorage | Storage creation |
| GetStorage | Storage retrieval |
| DeleteStorage | Storage deletion |
| Clear | Individual app clear |
| ClearAll | Clear all with exemptions |

### test_DownloadManager.cpp

Tests for the DownloadManager plugin:

| Test Category | Description |
|---------------|-------------|
| Download | Basic download |
| Priority | Priority queue handling |
| Pause/Resume | Download control |
| Cancel | Download cancellation |
| Progress | Progress reporting |

### test_PreinstallManager.cpp

Tests for the PreinstallManager plugin:

| Test Category | Description |
|---------------|-------------|
| StartPreinstall | Scan and install |
| ForceInstall | Forced reinstallation |
| GetState | State retrieval |
| Notifications | Completion events |

### test_RDKWindowManager.cpp

Tests for the RDKWindowManager plugin:

| Test Category | Description |
|---------------|-------------|
| CreateDisplay | Display creation |
| SetFocus | Focus management |
| KeyIntercept | Key interception |
| Visibility | Window visibility |
| ZOrder | Z-order management |
| Inactivity | Inactivity detection |

---

## 4. Mock Infrastructure

### Location: `Tests/mocks/`

The mocks directory contains mock implementations for external dependencies:

```
mocks/
├── IPackageImplDummy.h       # Package implementation mock
├── ThunderMocks.h            # WPEFramework/Thunder mocks
├── OCIContainerMock.h        # OCI container mock
├── WindowManagerMock.h       # Window manager mock
└── ...
```

### Mock Usage Pattern

```cpp
// Example mock usage in tests
class MockOCIContainer : public Exchange::IOCIContainer {
public:
    MOCK_METHOD(Core::hresult, StartContainerFromSpec, 
                (const string& id, const string& spec, uint32_t& descriptor), 
                (override));
    MOCK_METHOD(Core::hresult, StopContainer, 
                (const string& id), 
                (override));
    // ... additional mock methods
};
```

---

## 5. Running Tests

### Prerequisites

```bash
# Install test dependencies
sudo apt-get install -y \
    googletest \
    libgmock-dev \
    lcov

# Build Thunder/WPEFramework test dependencies
./build_dependencies.sh
```

### Building L1 Tests

```bash
cd Tests/L1Tests

# Configure with coverage
cmake -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_CODE_COVERAGE=ON

# Build
cmake --build build
```

### Running L1 Tests

```bash
# Run all tests
./run_l1_from_l1build.sh

# Or run directly
cd build
ctest --output-on-failure

# Run specific test
./build/tests/test_AppManager
```

### Generating Coverage Reports

```bash
# Run with coverage
./run_l1_coverage_from_l1build.sh

# Coverage report generated in build/coverage/
```

---

## 6. Test Configuration

### CMake Test Configuration

```cmake
# From Tests/L1Tests/CMakeLists.txt
enable_testing()

# Add test executables
add_executable(test_AppManager tests/test_AppManager.cpp)
target_link_libraries(test_AppManager
    GTest::GTest
    GTest::Main
    GMock::GMock
    ${NAMESPACE}AppManagerImplementation
)

add_test(NAME AppManagerTests COMMAND test_AppManager)
```

### Coverage Configuration (.lcovrc_l1)

```ini
# LCOV configuration for L1 tests
genhtml_branch_coverage = 1
lcov_branch_coverage = 1
```

### GCC Coverage Flags

```cmake
# From gcc-with-coverage.cmake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
```

---

## 7. Writing New Tests

### Test File Template

```cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Include implementation header
#include "MyPluginImplementation.h"

// Include mocks
#include "mocks/ThunderMocks.h"

using namespace WPEFramework;
using namespace WPEFramework::Plugin;
using ::testing::_;
using ::testing::Return;

class MyPluginTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test fixtures
        mPlugin = std::make_unique<MyPluginImplementation>();
    }

    void TearDown() override {
        // Cleanup
        mPlugin.reset();
    }

    std::unique_ptr<MyPluginImplementation> mPlugin;
};

TEST_F(MyPluginTest, InitializationSucceeds) {
    // Arrange
    MockShell mockShell;
    EXPECT_CALL(mockShell, /* expectations */);

    // Act
    auto result = mPlugin->Configure(&mockShell);

    // Assert
    EXPECT_EQ(result, Core::ERROR_NONE);
}

TEST_F(MyPluginTest, OperationHandlesError) {
    // Test error handling
}
```

### Best Practices

1. **Use Fixtures**: Group related tests in test fixtures
2. **Mock External Dependencies**: Don't rely on real external services
3. **Test Error Paths**: Test both success and failure scenarios
4. **Meaningful Names**: Use descriptive test names
5. **Single Responsibility**: Each test should verify one thing
6. **Clean State**: Reset state between tests

---

## 8. Coverage Requirements

### Target Coverage

| Component | Target | Current |
|-----------|--------|---------|
| AppManager | 80% | TBD |
| LifecycleManager | 80% | TBD |
| RuntimeManager | 80% | TBD |
| PackageManager | 80% | TBD |
| Other Plugins | 70% | TBD |

### Coverage Gaps to Address

Based on code analysis, these areas need additional coverage:

1. **Error Handling Paths**: Many error scenarios lack tests
2. **Concurrent Operations**: Thread safety not fully tested
3. **Edge Cases**: Boundary conditions and unusual inputs
4. **State Transitions**: Complex state machine paths
5. **Notification Delivery**: Event ordering and delivery

---

## 9. CI/CD Integration

### GitHub Workflow Integration

The test suite integrates with GitHub Actions workflows:

```yaml
# .github/workflows/test.yml (conceptual)
jobs:
  l1-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build dependencies
        run: ./build_dependencies.sh
      - name: Build and run L1 tests
        run: |
          cd Tests/L1Tests
          cmake -B build -DENABLE_CODE_COVERAGE=ON
          cmake --build build
          ctest --test-dir build --output-on-failure
      - name: Generate coverage report
        run: ./run_l1_coverage_from_l1build.sh
      - name: Upload coverage
        uses: codecov/codecov-action@v2
```

---

## 10. Troubleshooting

### Common Issues

| Issue | Solution |
|-------|----------|
| Missing dependencies | Run `./build_dependencies.sh` |
| Link errors | Check Thunder SDK path |
| Test timeouts | Increase timeout or check for deadlocks |
| Coverage not generated | Ensure `-DENABLE_CODE_COVERAGE=ON` |
| Mock not working | Verify mock expectations |

### Debug Tips

```bash
# Run single test with verbose output
./build/tests/test_AppManager --gtest_filter="AppManagerTest.LaunchApp" --gtest_verbose

# Run with GDB
gdb ./build/tests/test_AppManager

# Check for memory issues
valgrind ./build/tests/test_AppManager
```
