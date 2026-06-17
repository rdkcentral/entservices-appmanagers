# Fuzz Report - AppManager

Generated: 2026-06-14 09:21:52 UTC

## Summary
- total functions fuzzed: 639
- total seeds generated: 35231
- total executions: 1278000
- crashes found: 2
- ignored false positives: 0
- coverage summary: discovered targets=639, coverage gaps=199

## Applied Budgets
- RUNS: 2000
- MAX_TOTAL_TIME: 0
- PAIRWISE_BUDGET: 48
- STATE_SEQUENCE_SEEDS: 20
- MAX_JSON_SEEDS: 40

## Valid Crashes

### AppManagerImplementation::GetLoadedApps
- source file: AppManager/AppManagerImplementation.cpp
- line number: 1047
- crash type: sanitizer
- triggering input: fuzz/stats/crashes/AppManager/31337381b1bde14e_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
- sanitizer output:
```

    #8 0x000102443800 in fuzzer::Fuzzer::Loop(std::__1::vector<fuzzer::SizedFile, std::__1::allocator<fuzzer::SizedFile>>&) FuzzerLoop.cpp:872
    #9 0x0001024334bc in fuzzer::FuzzerDriver(int*, char***, int (*)(unsigned char const*, unsigned long)) FuzzerDriver.cpp:923
    #10 0x00010245deac in main FuzzerMain.cpp:20
    #11 0x00018d1abda0 in start+0x1b4c (dyld:arm64e+0x1fda0)

==61330==Register values:
 x[0] = 0x0000000000000096   x[1] = 0x0000000000000000   x[2] = 0x0000000000000000   x[3] = 0x000000016d9dd859  
 x[4] = 0x0000000000000000   x[5] = 0x000000000000001a   x[6] = 0x0a64657265746e45   x[7] = 0x0000000000000001  
 x[8] = 0x0000000000000000   x[9] = 0x000000016d9ddd48  x[10] = 0x0000000000000073  x[11] = 0x00000080204d923f  
x[12] = 0x0000000000000003  x[13] = 0x0000000000000005  x[14] = 0x05000000f9f90600  x[15] = 0x0000000000000000  
x[16] = 0x0000000102a83ed8  x[17] = 0x00000001fa9a9df0  x[18] = 0x0000000000000000  x[19] = 0x0000000102afe7e7  
x[20] = 0x00000001025c91fb  x[21] = 0x0000000102aff2d5  x[22] = 0xaaaaaaaaaaaaaaaa  x[23] = 0x0000000102aff1e9  
x[24] = 0x000000016d9dd490  x[25] = 0x00000001034fa170  x[26] = 0x0000000000000001  x[27] = 0x00000000ffffffff  
x[28] = 0x0000000000000096     fp = 0x000000016d9ddcf0     lr = 0x0000000102a82e2c     sp = 0x000000016d9dd430  
AddressSanitizer can not provide additional info.
SUMMARY: AddressSanitizer: SEGV AppManagerImplementation.cpp:1051 in WPEFramework::Plugin::AppManagerImplementation::GetLoadedApps(WPEFramework::RPC::IIteratorType<WPEFramework::Exchange::IAppManager::LoadedAppInfo, 66034u>*&)
==61330==ABORTING
MS: 0 ; base unit: 0000000000000000000000000000000000000000
0xa,
\012
artifact_prefix='/Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/AppManager/31337381b1bde14e_'; Test unit written to /Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/AppManager/31337381b1bde14e_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
Base64: Cg==
```
- root cause explanation: Memory safety fault reached in an in-scope production stack frame.
- why crash occurred: Mutated input exercised an unsafe code path.
- suggested fix: Add input validation and ownership-safe bounds checks around failing path.

### LifecycleInterfaceConnector::getLoadedApps
- source file: AppManager/LifecycleInterfaceConnector.cpp
- line number: 593
- crash type: sanitizer
- triggering input: fuzz/stats/crashes/AppManager/88215b7f5467d7bb_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
- sanitizer output:
```

    #8 0x000104543800 in fuzzer::Fuzzer::Loop(std::__1::vector<fuzzer::SizedFile, std::__1::allocator<fuzzer::SizedFile>>&) FuzzerLoop.cpp:872
    #9 0x0001045334bc in fuzzer::FuzzerDriver(int*, char***, int (*)(unsigned char const*, unsigned long)) FuzzerDriver.cpp:923
    #10 0x00010455deac in main FuzzerMain.cpp:20
    #11 0x00018d1abda0 in start+0x1b4c (dyld:arm64e+0x1fda0)

==61654==Register values:
 x[0] = 0x0000000000000096   x[1] = 0x0000000000000000   x[2] = 0x0000000000000000   x[3] = 0x000000016b8dd859  
 x[4] = 0x0000000000000000   x[5] = 0x000000000000001a   x[6] = 0x0a64657265746e45   x[7] = 0x0000000000000001  
 x[8] = 0x0000000000000000   x[9] = 0x000000016b8ddd48  x[10] = 0x0000000000000073  x[11] = 0x00000080208f923f  
x[12] = 0x0000000000000003  x[13] = 0x0000000000000005  x[14] = 0x05000000f9f90600  x[15] = 0x0000000000000000  
x[16] = 0x0000000104d7fed8  x[17] = 0x00000001fa9a9df0  x[18] = 0x0000000000000000  x[19] = 0x0000000104dfa7e7  
x[20] = 0x00000001046c91fb  x[21] = 0x0000000104dfb2d5  x[22] = 0xaaaaaaaaaaaaaaaa  x[23] = 0x0000000104dfb1e9  
x[24] = 0x000000016b8dd490  x[25] = 0x00000001057f6170  x[26] = 0x0000000000000001  x[27] = 0x00000000ffffffff  
x[28] = 0x0000000000000096     fp = 0x000000016b8ddcf0     lr = 0x0000000104d7ee2c     sp = 0x000000016b8dd430  
AddressSanitizer can not provide additional info.
SUMMARY: AddressSanitizer: SEGV AppManagerImplementation.cpp:1051 in WPEFramework::Plugin::AppManagerImplementation::GetLoadedApps(WPEFramework::RPC::IIteratorType<WPEFramework::Exchange::IAppManager::LoadedAppInfo, 66034u>*&)
==61654==ABORTING
MS: 0 ; base unit: 0000000000000000000000000000000000000000
0xa,
\012
artifact_prefix='/Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/AppManager/88215b7f5467d7bb_'; Test unit written to /Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/AppManager/88215b7f5467d7bb_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
Base64: Cg==
```
- root cause explanation: Memory safety fault reached in an in-scope production stack frame.
- why crash occurred: Mutated input exercised an unsafe code path.
- suggested fix: Add input validation and ownership-safe bounds checks around failing path.
