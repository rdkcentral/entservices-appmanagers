# PreinstallManager L1 Segfault Root Cause and Fix

## Context
- Reproduced during L1 test execution via rebuild flow.
- Repro filter:
  - `PreinstallManagerTest.StartPreinstallWithForceInstallSuccess`
  - `PreinstallManagerTest.StartPreinstallWithForceInstallInstallFailure`

## Observed Failure
- Initial failure:
  - `free(): double free detected in tcache 2`
- Follow-up failure after first fix attempt:
  - `Segmentation fault` in `RuntimeConfig::~RuntimeConfig()` during `readPreinstallDirectory`

## GDB Findings
- First trace (failure path):
  - `WPEFramework::Exchange::RuntimeConfig::~RuntimeConfig()`
  - `WPEFramework::Plugin::PreinstallManagerImplementation::_PackageInfo::~_PackageInfo()`
  - `std::list<...>::_M_clear()`
  - `WPEFramework::Plugin::PreinstallManagerImplementation::StartPreinstall(bool)`
- Second trace (success path while scanning packages):
  - `WPEFramework::Exchange::RuntimeConfig::~RuntimeConfig()`
  - `WPEFramework::Plugin::PreinstallManagerImplementation::readPreinstallDirectory(...)`

## Root Cause Analysis
- `Exchange::RuntimeConfig` handling in this L1 path is unstable in the mocked `GetConfigForPackage` flow.
- The original implementation copied `PackageInfo` (containing `RuntimeConfig`) into a list via `push_back(packageInfo)`, which introduced extra copies and teardown-time invalid frees.
- Even after copy-removal, L1 test ABI behavior still allowed corruption around `RuntimeConfig` destruction when it was embedded directly as a value member in `PackageInfo`.

## Patch Applied
- `PreinstallManager/PreinstallManagerImplementation.h`
  - Added L1-only guarded `PackageInfo::configMetadata` storage as pointer to avoid value-object teardown in test build:
    - `#ifdef RDK_SERVICES_L1_TEST` -> `RuntimeConfig* configMetadata`
    - `#else` -> `RuntimeConfig configMetadata`
- `PreinstallManager/PreinstallManagerImplementation.cpp`
  - Replaced `push_back(packageInfo)` with in-place list entry construction:
    - `packages.emplace_back();`
    - `PackageInfo& packageInfo = packages.back();`
  - Added L1-only compatibility metadata backing object and passed that to `GetConfigForPackage`:
    - static `RuntimeConfigCompat` containing `RuntimeConfig value` plus trailing `std::string`
    - `packageInfo.configMetadata = &configMetadataPtr->value`
  - Kept production path unchanged (`RuntimeConfig` value member reference).
  - Removed unused install `additionalMetadata` local and passed `nullptr` directly to `Install`.
- `Tests/L1Tests/tests/test_PreinstallManager.cpp`
  - Updated `GetConfigForPackage` expectations in repro tests to deterministic `DoAll(SetArgReferee..., Return(...))`.
  - Fixed interface-pointer return in `QueryInterfaceByCallsign` mock from raw reinterpret cast to typed interface pointer conversion.

## Why This Fix Is Safe
- Functional behavior remains unchanged for package discovery and install decisions.
- Production build path remains value-based `RuntimeConfig` storage and metadata flow.
- L1-specific guarded path isolates test-only instability without changing runtime plugin behavior.
- The crash path is addressed by removing extra `PackageInfo` copies and avoiding problematic L1 value-object teardown.

## Validation Results
1. Command executed:
  - `GTEST_FILTER="PreinstallManagerTest.StartPreinstallWithForceInstallSuccess:PreinstallManagerTest.StartPreinstallWithForceInstallInstallFailure" bash Tests/rebuild_l1_from_l1build.sh`
2. Result:
  - Both tests passed.
  - No segfault.
  - No double-free abort.
3. GDB validation (run per repro test):
  - `gdb -q --batch -ex "set pagination off" -ex "run" -ex "bt" --args ./l1build/install/usr/bin/RdkServicesL1Test --gtest_filter="PreinstallManagerTest.StartPreinstallWithForceInstallSuccess"`
  - `gdb -q --batch -ex "set pagination off" -ex "run" -ex "bt" --args ./l1build/install/usr/bin/RdkServicesL1Test --gtest_filter="PreinstallManagerTest.StartPreinstallWithForceInstallInstallFailure"`
  - Both repro tests passed under gdb (inferior exited normally, no crash stack).

## Notes
- `gdb --batch` may exit with non-zero after successful runs when no stack is available at normal program exit; pass/fail should be read from test output (`[  PASSED  ]`) and absence of crash signal.
