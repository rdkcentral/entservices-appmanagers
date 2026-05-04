# PreinstallManager L1 Segfault Root Cause and Fix

## Verdict

This is a **test-side ABI/ODR mismatch issue** around `WPEFramework::Exchange::RuntimeConfig` destructor resolution, not a PreinstallManager business-logic defect.

## What was observed

- Provided crash report shows abort in worker thread with allocator failure:
	- `free(): double free detected in tcache 2`
	- stack includes `_PackageInfo::~_PackageInfo()` and `RuntimeConfig::~RuntimeConfig()` while clearing package list.

Additional verification showed both test binary and plugin `.so` export weak symbols for:

- `WPEFramework::Exchange::RuntimeConfig::~RuntimeConfig()`

When multiple translation units define `RuntimeConfig` differently (because of `#ifndef RUNTIME_CONFIG` include-order behavior across interface headers), the process can resolve a destructor implementation that does not match the object layout being destroyed.

That mismatch is consistent with the observed `double free` during `_PackageInfo` list cleanup in worker thread.

## Root cause details

### 1. Conflicting `RuntimeConfig` layouts exist in interface headers

`RuntimeConfig` is conditionally defined in multiple headers under `#ifndef RUNTIME_CONFIG` (for example `IAppPackageManager.h`, `IRuntimeManager.h`, `ILifecycleManager.h`, `IAppManager.h`).

Because layout depends on first include order in each translation unit, different TUs can compile with different structure definitions.

### 2. Destructor symbol interposition happened at runtime

Both the test executable and `libWPEFrameworkPreinstallManagerImplementation.so` contain weak `RuntimeConfig` destructor symbols.

At runtime, the destructor used for objects created in plugin code can come from the executable, causing invalid destruction if the selected layout differs.

This matches the backtrace location:

- `RuntimeConfig::~RuntimeConfig()`
- `_PackageInfo::~_PackageInfo()`
- `std::list<_PackageInfo>::_M_clear()`

## Implemented test-side fix

In `Tests/L1Tests/tests/test_PreinstallManager.cpp`, a canonical `RuntimeConfig` definition was added before interface headers are included, with an explicit out-of-line destructor:

- defines `RUNTIME_CONFIG` in this TU
- defines `WPEFramework::Exchange::RuntimeConfig` with Preinstall-relevant layout
- defines `RuntimeConfig::~RuntimeConfig() = default;` out-of-line

This provides a consistent destructor/layout boundary for this test path and removes the crash in the reproducible scenario.

## Validation after fix

1. Reproduced-crash test `PreinstallManagerTest.StartPreinstallWithForceInstallInstallFailure` now passes.
2. Stress run of that test passed (`STRESS_PASS=10`).
3. Full no-filter L1 run completed without segfault/abort.

## Current full-suite status

No segfault observed after this test-side fix.

There are still non-segfault functional failures in other suites (RuntimeManager/StorageManager), which are unrelated to this crash root cause.

## Conclusion

The segfault root cause was test-side `RuntimeConfig` ABI/destructor mismatch from weak symbol interposition, and the implemented test-side canonicalization fix removes the reproducible PreinstallManager crash.
