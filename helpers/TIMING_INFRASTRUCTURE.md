# RDK App Managers Debug Timing Infrastructure

## Overview

This infrastructure provides comprehensive timing instrumentation across all RDK App Manager plugins to measure:
- Individual API execution times
- Cross-plugin COM-RPC call latencies
- Plugin invocation response times

## Components

### 1. RDKAppManagersDebugTime.h

A header file that provides:
- `ScopeLogger` class: RAII-based timing logger that automatically logs entry/exit
- `RDKAPPMANAGERS_DEBUG_TIME_SCOPE(pluginName, apiName)` macro: Instruments any API scope

**Features:**
- Only active when `ENABLE_RDKAPPMANAGERS_DEBUG` is defined
- Logs microsecond-precision timestamps
- Thread-safe logging with thread ID tracking
- Zero overhead when disabled (macro expands to no-op)

**Log Format:**
```
[RDKAPPMANAGERS_TIMING] ENTRY plugin=<PluginName> api=<APIName> timestamp=<microseconds> thread=<threadID>
[RDKAPPMANAGERS_TIMING] EXIT plugin=<PluginName> api=<APIName> timestamp=<microseconds> duration_us=<duration> thread=<threadID>
```

### 2. Instrumented Plugins

All major APIs across these plugins are instrumented:

- **AppManager**: LaunchApp, CloseApp, TerminateApp, KillApp, GetLoadedApps, SendIntent, PreloadApp, GetAppProperty, SetAppProperty, GetInstalledApps
- **LifecycleManager**: GetLoadedApps, IsAppLoaded, SpawnApp, SetTargetAppState, UnloadApp, KillApp, SendIntentToActiveApp
- **RuntimeManager**: Run, Hibernate, Wake, Suspend, Resume
- **PackageManager**: Download, Pause, Resume, Cancel, Delete
- **DownloadManager**: Download, Pause, Resume, Cancel, Delete
- **PreinstallManager**: StartPreinstall
- **StorageManager**: CreateStorage, GetStorage, DeleteStorage, Clear
- **TelemetryMetrics**: Record, Publish

### 3. rdkappmanagers_time_analyzer.py

Python tool to analyze timing logs and generate comprehensive reports.

**Usage:**
```bash
# Basic analysis
python rdkappmanagers_time_analyzer.py wpeframework.log

# Filter specific plugin pair
python rdkappmanagers_time_analyzer.py wpeframework.log --plugin-a AppManager --plugin-b LifecycleManager

# Verbose output with individual call details
python rdkappmanagers_time_analyzer.py wpeframework.log --verbose

# Save report to file
python rdkappmanagers_time_analyzer.py wpeframework.log --output timing_report.txt
```

**Capabilities:**
- **API Timing Statistics**: Min/Max/Avg/Median execution time per API
- **Cross-Plugin Call Analysis**: Measures latencies between plugin boundaries
  - Call Start Delta: Time from caller ENTRY to callee ENTRY (COM-RPC overhead)
  - Callee Duration: Time spent in the called API
  - Return Delta: Total time from caller ENTRY to callee EXIT
- **Thread-Aware Analysis**: Tracks call stacks per thread
- **Filtering**: Focus analysis on specific plugin interactions

## Building with Debug Timing

To enable timing instrumentation, build with the debug flag:

```bash
# CMake build
cmake -DENABLE_RDKAPPMANAGERS_DEBUG=ON ..
make

# Or add to your build configuration
-DENABLE_RDKAPPMANAGERS_DEBUG
```

## Usage Example

1. **Enable and Build:**
   ```bash
   cmake -DENABLE_RDKAPPMANAGERS_DEBUG=ON -B build
   cmake --build build
   ```

2. **Run your application** and collect logs to `wpeframework.log`

3. **Analyze timing:**
   ```bash
   python helpers/rdkappmanagers_time_analyzer.py wpeframework.log
   ```

4. **Example output:**
   ```
   ================================================================================
   RDK APP MANAGERS TIMING ANALYSIS REPORT
   ================================================================================
   
   Total timing events parsed: 1234
   Unique APIs instrumented: 45
   Cross-plugin calls detected: 234
   
   --------------------------------------------------------------------------------
   API EXECUTION TIME STATISTICS
   --------------------------------------------------------------------------------
   
   Plugin::API                              Count        Min         Avg         Max      Median
   --------------------------------------------------------------------------------
   AppManager::LaunchApp                       42    110.23 ms    125.45 ms    156.78 ms    122.34 ms
   LifecycleManager::SpawnApp                  42    108.45 ms    123.12 ms    154.23 ms    120.67 ms
   RuntimeManager::Run                         42    105.67 ms    120.89 ms    151.45 ms    118.23 ms
   
   --------------------------------------------------------------------------------
   CROSS-PLUGIN CALL LATENCY ANALYSIS
   --------------------------------------------------------------------------------
   
   AppManager::LaunchApp → LifecycleManager::SpawnApp
     Invocations: 42
     Call Start Delta (caller ENTRY → callee ENTRY):
       Min: 1.23 ms, Avg: 1.67 ms, Max: 2.45 ms, Median: 1.56 ms
     Callee Execution Time:
       Min: 108.45 ms, Avg: 123.12 ms, Max: 154.23 ms, Median: 120.67 ms
     Return Delta (caller ENTRY → callee EXIT):
       Min: 109.78 ms, Avg: 124.89 ms, Max: 156.34 ms, Median: 122.23 ms
   ```

## Performance Impact

- **Disabled (default)**: Zero overhead - macros expand to no-op
- **Enabled**: Minimal overhead:
  - ~1-2 microseconds per API call for timestamp capture
  - Log I/O is buffered and handled by existing logging infrastructure
  - Negligible impact on production performance analysis

## Advanced Analysis

### Identifying Bottlenecks

1. **API-level bottlenecks**: Look for APIs with high Max/Avg times
2. **COM-RPC overhead**: High "Call Start Delta" indicates inter-plugin communication delays
3. **Cascading delays**: Compare parent and child API durations to find cascading issues

### Filtering Cross-Plugin Calls

```bash
# Focus on AppManager → LifecycleManager calls
python rdkappmanagers_time_analyzer.py wpeframework.log --plugin-a AppManager --plugin-b LifecycleManager

# See all calls involving RuntimeManager
python rdkappmanagers_time_analyzer.py wpeframework.log --plugin-a RuntimeManager
```

## Files Modified/Created

- `helpers/RDKAppManagersDebugTime.h` - Core timing infrastructure header
- `helpers/rdkappmanagers_time_analyzer.py` - Python analysis tool
- `helpers/sample_wpeframework.txt` - Sample log for testing
- `AppManager/AppManagerImplementation.cpp` - Instrumented
- `LifecycleManager/LifecycleManagerImplementation.cpp` - Instrumented
- `RuntimeManager/RuntimeManagerImplementation.cpp` - Instrumented
- `PackageManager/PackageManagerImplementation.cpp` - Instrumented
- `DownloadManager/DownloadManagerImplementation.cpp` - Instrumented
- `PreinstallManager/PreinstallManagerImplementation.cpp` - Instrumented
- `StorageManager/StorageManagerImplementation.cpp` - Instrumented
- `TelemetryMetrics/TelemetryMetricsImplementation.cpp` - Instrumented

## Troubleshooting

### No timing logs appearing
- Verify `ENABLE_RDKAPPMANAGERS_DEBUG` is defined during build
- Check log level includes INFO messages
- Ensure logging is not filtered by other configurations

### Analyzer reports "No events found"
- Verify log file contains `[RDKAPPMANAGERS_TIMING]` entries
- Check file path is correct
- Try with sample_wpeframework.txt first to verify tool works

### Incomplete cross-plugin analysis
- Ensure all relevant plugins are built with instrumentation enabled
- Some EXIT events may be missing if APIs return early or throw exceptions
- Use --verbose flag to see partial timing data

## Future Enhancements

Potential additions to this infrastructure:
- Real-time timing dashboard
- Automated performance regression detection
- Integration with profiling tools (perf, valgrind)
- JSON output format for automated analysis
- Histogram visualization of timing distributions
