# Fuzz Report

Generated: 2026-06-14 09:21:52 UTC

## Summary
- total functions fuzzed: 639
- total seeds generated: 35231
- total executions: 1278000
- crashes found: 10
- ignored false positives: 0
- coverage summary: discovered targets=639, coverage gaps=199

## Applied Budgets
- RUNS: 2000
- MAX_TOTAL_TIME: 0
- PAIRWISE_BUDGET: 48
- STATE_SEQUENCE_SEEDS: 20
- MAX_JSON_SEEDS: 40

## Valid Crashes

### DownloadManagerImplementation::Deinitialize
- source file: DownloadManager/DownloadManagerImplementation.cpp
- line number: 148
- crash type: sanitizer
- triggering input: fuzz/stats/crashes/DownloadManager/296ff8eaa6700528_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
- sanitizer output:
```
ib:arm64+0x7b9f8)
    #8 0x00010376e780 in __ubsan_handle_type_mismatch_v1_abort+0x28 (libclang_rt.asan_osx_dynamic.dylib:arm64+0x92780)
    #9 0x000102ea292c in WPEFramework::Plugin::DownloadManagerImplementation::Deinitialize(WPEFramework::PluginHost::IShell*) DownloadManagerImplementation.cpp:178
    #10 0x000102daf340 in fuzzauto::RunTargetApi(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&, fuzzauto::InputReader&) fuzz_harness_common.h
    #11 0x000102dadcd0 in fuzzauto::FuzzHarnessRunForTarget(char const*, unsigned char const*, unsigned long) fuzz_harness_common.h:389
    #12 0x000102dcd528 in fuzzer::Fuzzer::ExecuteCallback(unsigned char const*, unsigned long) FuzzerLoop.cpp:619
    #13 0x000102dcccb8 in fuzzer::Fuzzer::RunOne(unsigned char const*, unsigned long, bool, fuzzer::InputInfo*, bool, bool*) FuzzerLoop.cpp:516
    #14 0x000102dcefac in fuzzer::Fuzzer::ReadAndExecuteSeedCorpora(std::__1::vector<fuzzer::SizedFile, std::__1::allocator<fuzzer::SizedFile>>&) FuzzerLoop.cpp:817
    #15 0x000102dcf4c8 in fuzzer::Fuzzer::Loop(std::__1::vector<fuzzer::SizedFile, std::__1::allocator<fuzzer::SizedFile>>&) FuzzerLoop.cpp:872
    #16 0x000102dbf184 in fuzzer::FuzzerDriver(int*, char***, int (*)(unsigned char const*, unsigned long)) FuzzerDriver.cpp:923
    #17 0x000102de9b74 in main FuzzerMain.cpp:20
    #18 0x00018d1abda0 in start+0x1b4c (dyld:arm64e+0x1fda0)

NOTE: libFuzzer has rudimentary signal handlers.
      Combine libFuzzer with AddressSanitizer or similar for better crash reports.
SUMMARY: libFuzzer: deadly signal
MS: 0 ; base unit: 0000000000000000000000000000000000000000
0xa,
\012
artifact_prefix='/Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/DownloadManager/296ff8eaa6700528_'; Test unit written to /Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/DownloadManager/296ff8eaa6700528_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
Base64: Cg==
```
- root cause explanation: Memory safety fault reached in an in-scope production stack frame.
- why crash occurred: Mutated input exercised an unsafe code path.
- suggested fix: Add input validation and ownership-safe bounds checks around failing path.

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

### StorageManagerImplementation::Configure
- source file: AppStorageManager/AppStorageManagerImplementation.cpp
- line number: 53
- crash type: sanitizer
- triggering input: fuzz/stats/crashes/AppStorageManager/3ac70fc1c804b80d_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
- sanitizer output:
```
b:arm64+0x7b9f8)
    #8 0x000101096780 in __ubsan_handle_type_mismatch_v1_abort+0x28 (libclang_rt.asan_osx_dynamic.dylib:arm64+0x92780)
    #9 0x000100b36e80 in WPEFramework::Plugin::StorageManagerImplementation::Configure(WPEFramework::PluginHost::IShell*) AppStorageManagerImplementation.cpp:63
    #10 0x000100a46714 in fuzzauto::RunTargetApi(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&, fuzzauto::InputReader&) fuzz_harness_common.h
    #11 0x000100a45cd0 in fuzzauto::FuzzHarnessRunForTarget(char const*, unsigned char const*, unsigned long) fuzz_harness_common.h:389
    #12 0x000100a65b14 in fuzzer::Fuzzer::ExecuteCallback(unsigned char const*, unsigned long) FuzzerLoop.cpp:619
    #13 0x000100a652a4 in fuzzer::Fuzzer::RunOne(unsigned char const*, unsigned long, bool, fuzzer::InputInfo*, bool, bool*) FuzzerLoop.cpp:516
    #14 0x000100a67598 in fuzzer::Fuzzer::ReadAndExecuteSeedCorpora(std::__1::vector<fuzzer::SizedFile, std::__1::allocator<fuzzer::SizedFile>>&) FuzzerLoop.cpp:817
    #15 0x000100a67ab4 in fuzzer::Fuzzer::Loop(std::__1::vector<fuzzer::SizedFile, std::__1::allocator<fuzzer::SizedFile>>&) FuzzerLoop.cpp:872
    #16 0x000100a57770 in fuzzer::FuzzerDriver(int*, char***, int (*)(unsigned char const*, unsigned long)) FuzzerDriver.cpp:923
    #17 0x000100a82160 in main FuzzerMain.cpp:20
    #18 0x00018d1abda0 in start+0x1b4c (dyld:arm64e+0x1fda0)

NOTE: libFuzzer has rudimentary signal handlers.
      Combine libFuzzer with AddressSanitizer or similar for better crash reports.
SUMMARY: libFuzzer: deadly signal
MS: 0 ; base unit: 0000000000000000000000000000000000000000
0xa,
\012
artifact_prefix='/Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/AppStorageManager/3ac70fc1c804b80d_'; Test unit written to /Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/AppStorageManager/3ac70fc1c804b80d_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
Base64: Cg==
```
- root cause explanation: Memory safety fault reached in an in-scope production stack frame.
- why crash occurred: Mutated input exercised an unsafe code path.
- suggested fix: Add input validation and ownership-safe bounds checks around failing path.

### PreinstallManagerImplementation::Configure
- source file: PreinstallManager/PreinstallManagerImplementation.cpp
- line number: 119
- crash type: sanitizer
- triggering input: fuzz/stats/crashes/PreinstallManager/50e023120c3ee087_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
- sanitizer output:
```
char_traits<char>, std::__1::allocator<char>>::__reset_internal_buffer[abi:nqe220107](std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>::__rep) fuzz_harness_common.h:389
    #12 0x000100455cd0 in std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>::~basic_string() string:1210
    #13 0x000100455cd0 in std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>::~basic_string() string:1210
    #14 0x000100455cd0 in fuzzauto::FuzzHarnessRunForTarget(char const*, unsigned char const*, unsigned long) fuzz_harness_common.h:389
    #15 0x000100474f84 in fuzzer::Fuzzer::ExecuteCallback(unsigned char const*, unsigned long) FuzzerLoop.cpp:619
    #16 0x000100474714 in fuzzer::Fuzzer::RunOne(unsigned char const*, unsigned long, bool, fuzzer::InputInfo*, bool, bool*) FuzzerLoop.cpp:516
    #17 0x000100476a08 in fuzzer::Fuzzer::ReadAndExecuteSeedCorpora(std::__1::vector<fuzzer::SizedFile, std::__1::allocator<fuzzer::SizedFile>>&) FuzzerLoop.cpp:817
    #18 0x000100476f24 in fuzzer::Fuzzer::Loop(std::__1::vector<fuzzer::SizedFile, std::__1::allocator<fuzzer::SizedFile>>&) FuzzerLoop.cpp:872
    #19 0x000100466be0 in fuzzer::FuzzerDriver(int*, char***, int (*)(unsigned char const*, unsigned long)) FuzzerDriver.cpp:923
    #20 0x0001004915d0 in main FuzzerMain.cpp:20
    #21 0x00018d1abda0 in start+0x1b4c (dyld:arm64e+0x1fda0)

NOTE: libFuzzer has rudimentary signal handlers.
      Combine libFuzzer with AddressSanitizer or similar for better crash reports.
SUMMARY: libFuzzer: deadly signal
MS: 0 ; base unit: 0000000000000000000000000000000000000000
0xa,
\012
artifact_prefix='/Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/PreinstallManager/50e023120c3ee087_'; Test unit written to /Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/PreinstallManager/50e023120c3ee087_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
Base64: Cg==
```
- root cause explanation: Memory safety fault reached in an in-scope production stack frame.
- why crash occurred: Mutated input exercised an unsafe code path.
- suggested fix: Add input validation and ownership-safe bounds checks around failing path.

### AppStorageManager::Initialize
- source file: AppStorageManager/AppStorageManager.cpp
- line number: 64
- crash type: sanitizer
- triggering input: fuzz/stats/crashes/AppStorageManager/5734e0c4dc438961_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
- sanitizer output:
```
b:arm64+0x7b9f8)
    #8 0x000102cf6780 in __ubsan_handle_type_mismatch_v1_abort+0x28 (libclang_rt.asan_osx_dynamic.dylib:arm64+0x92780)
    #9 0x000102652e80 in WPEFramework::Plugin::StorageManagerImplementation::Configure(WPEFramework::PluginHost::IShell*) AppStorageManagerImplementation.cpp:63
    #10 0x000102562714 in fuzzauto::RunTargetApi(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&, fuzzauto::InputReader&) fuzz_harness_common.h
    #11 0x000102561cd0 in fuzzauto::FuzzHarnessRunForTarget(char const*, unsigned char const*, unsigned long) fuzz_harness_common.h:389
    #12 0x000102581b14 in fuzzer::Fuzzer::ExecuteCallback(unsigned char const*, unsigned long) FuzzerLoop.cpp:619
    #13 0x0001025812a4 in fuzzer::Fuzzer::RunOne(unsigned char const*, unsigned long, bool, fuzzer::InputInfo*, bool, bool*) FuzzerLoop.cpp:516
    #14 0x000102583598 in fuzzer::Fuzzer::ReadAndExecuteSeedCorpora(std::__1::vector<fuzzer::SizedFile, std::__1::allocator<fuzzer::SizedFile>>&) FuzzerLoop.cpp:817
    #15 0x000102583ab4 in fuzzer::Fuzzer::Loop(std::__1::vector<fuzzer::SizedFile, std::__1::allocator<fuzzer::SizedFile>>&) FuzzerLoop.cpp:872
    #16 0x000102573770 in fuzzer::FuzzerDriver(int*, char***, int (*)(unsigned char const*, unsigned long)) FuzzerDriver.cpp:923
    #17 0x00010259e160 in main FuzzerMain.cpp:20
    #18 0x00018d1abda0 in start+0x1b4c (dyld:arm64e+0x1fda0)

NOTE: libFuzzer has rudimentary signal handlers.
      Combine libFuzzer with AddressSanitizer or similar for better crash reports.
SUMMARY: libFuzzer: deadly signal
MS: 0 ; base unit: 0000000000000000000000000000000000000000
0xa,
\012
artifact_prefix='/Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/AppStorageManager/5734e0c4dc438961_'; Test unit written to /Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/AppStorageManager/5734e0c4dc438961_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
Base64: Cg==
```
- root cause explanation: Memory safety fault reached in an in-scope production stack frame.
- why crash occurred: Mutated input exercised an unsafe code path.
- suggested fix: Add input validation and ownership-safe bounds checks around failing path.

### PackageManagerImplementation::Initialize
- source file: PackageManager/PackageManagerImplementation.cpp
- line number: 115
- crash type: sanitizer
- triggering input: fuzz/stats/crashes/PackageManager/72bbf8e6829dc210_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
- sanitizer output:
```
ic.dylib:arm64+0x7b9f8)
    #8 0x000102e22780 in __ubsan_handle_type_mismatch_v1_abort+0x28 (libclang_rt.asan_osx_dynamic.dylib:arm64+0x92780)
    #9 0x0001028c60c8 in WPEFramework::Plugin::PackageManagerImplementation::Initialize(WPEFramework::PluginHost::IShell*) PackageManagerImplementation.cpp:122
    #10 0x000102797288 in fuzzauto::RunTargetApi(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&, fuzzauto::InputReader&) fuzz_harness_common.h
    #11 0x000102795cd0 in fuzzauto::FuzzHarnessRunForTarget(char const*, unsigned char const*, unsigned long) fuzz_harness_common.h:389
    #12 0x0001027b5418 in fuzzer::Fuzzer::ExecuteCallback(unsigned char const*, unsigned long) FuzzerLoop.cpp:619
    #13 0x0001027b4ba8 in fuzzer::Fuzzer::RunOne(unsigned char const*, unsigned long, bool, fuzzer::InputInfo*, bool, bool*) FuzzerLoop.cpp:516
    #14 0x0001027b6e9c in fuzzer::Fuzzer::ReadAndExecuteSeedCorpora(std::__1::vector<fuzzer::SizedFile, std::__1::allocator<fuzzer::SizedFile>>&) FuzzerLoop.cpp:817
    #15 0x0001027b73b8 in fuzzer::Fuzzer::Loop(std::__1::vector<fuzzer::SizedFile, std::__1::allocator<fuzzer::SizedFile>>&) FuzzerLoop.cpp:872
    #16 0x0001027a7074 in fuzzer::FuzzerDriver(int*, char***, int (*)(unsigned char const*, unsigned long)) FuzzerDriver.cpp:923
    #17 0x0001027d1a64 in main FuzzerMain.cpp:20
    #18 0x00018d1abda0 in start+0x1b4c (dyld:arm64e+0x1fda0)

NOTE: libFuzzer has rudimentary signal handlers.
      Combine libFuzzer with AddressSanitizer or similar for better crash reports.
SUMMARY: libFuzzer: deadly signal
MS: 0 ; base unit: 0000000000000000000000000000000000000000
0xa,
\012
artifact_prefix='/Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/PackageManager/72bbf8e6829dc210_'; Test unit written to /Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/PackageManager/72bbf8e6829dc210_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
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

### DownloadManagerImplementation::Initialize
- source file: DownloadManager/DownloadManagerImplementation.cpp
- line number: 100
- crash type: sanitizer
- triggering input: fuzz/stats/crashes/DownloadManager/ae44b26621ff2d4c_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
- sanitizer output:
```
ylib:arm64+0x7b9f8)
    #8 0x00010302e780 in __ubsan_handle_type_mismatch_v1_abort+0x28 (libclang_rt.asan_osx_dynamic.dylib:arm64+0x92780)
    #9 0x000102ac1854 in WPEFramework::Plugin::DownloadManagerImplementation::Initialize(WPEFramework::PluginHost::IShell*) DownloadManagerImplementation.cpp:108
    #10 0x0001029d3340 in fuzzauto::RunTargetApi(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&, fuzzauto::InputReader&) fuzz_harness_common.h
    #11 0x0001029d1cd0 in fuzzauto::FuzzHarnessRunForTarget(char const*, unsigned char const*, unsigned long) fuzz_harness_common.h:389
    #12 0x0001029f1528 in fuzzer::Fuzzer::ExecuteCallback(unsigned char const*, unsigned long) FuzzerLoop.cpp:619
    #13 0x0001029f0cb8 in fuzzer::Fuzzer::RunOne(unsigned char const*, unsigned long, bool, fuzzer::InputInfo*, bool, bool*) FuzzerLoop.cpp:516
    #14 0x0001029f2fac in fuzzer::Fuzzer::ReadAndExecuteSeedCorpora(std::__1::vector<fuzzer::SizedFile, std::__1::allocator<fuzzer::SizedFile>>&) FuzzerLoop.cpp:817
    #15 0x0001029f34c8 in fuzzer::Fuzzer::Loop(std::__1::vector<fuzzer::SizedFile, std::__1::allocator<fuzzer::SizedFile>>&) FuzzerLoop.cpp:872
    #16 0x0001029e3184 in fuzzer::FuzzerDriver(int*, char***, int (*)(unsigned char const*, unsigned long)) FuzzerDriver.cpp:923
    #17 0x000102a0db74 in main FuzzerMain.cpp:20
    #18 0x00018d1abda0 in start+0x1b4c (dyld:arm64e+0x1fda0)

NOTE: libFuzzer has rudimentary signal handlers.
      Combine libFuzzer with AddressSanitizer or similar for better crash reports.
SUMMARY: libFuzzer: deadly signal
MS: 0 ; base unit: 0000000000000000000000000000000000000000
0xa,
\012
artifact_prefix='/Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/DownloadManager/ae44b26621ff2d4c_'; Test unit written to /Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/DownloadManager/ae44b26621ff2d4c_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
Base64: Cg==
```
- root cause explanation: Memory safety fault reached in an in-scope production stack frame.
- why crash occurred: Mutated input exercised an unsafe code path.
- suggested fix: Add input validation and ownership-safe bounds checks around failing path.

### RDKWindowManagerImplementation::Initialize
- source file: RDKWindowManager/RDKWindowManagerImplementation.cpp
- line number: 199
- crash type: sanitizer
- triggering input: fuzz/stats/crashes/RDKWindowManager/de2cb0f01c001cd1_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
- sanitizer output:
```
:arm64+0x7b9f8)
    #8 0x00010526a780 in __ubsan_handle_type_mismatch_v1_abort+0x28 (libclang_rt.asan_osx_dynamic.dylib:arm64+0x92780)
    #9 0x000104b681d0 in WPEFramework::Plugin::RDKWindowManagerImplementation::Initialize(WPEFramework::PluginHost::IShell*) RDKWindowManagerImplementation.cpp:209
    #10 0x000104a6e47c in fuzzauto::RunTargetApi(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&, fuzzauto::InputReader&) fuzz_harness_common.h
    #11 0x000104a6dcd0 in fuzzauto::FuzzHarnessRunForTarget(char const*, unsigned char const*, unsigned long) fuzz_harness_common.h:389
    #12 0x000104a8d7b4 in fuzzer::Fuzzer::ExecuteCallback(unsigned char const*, unsigned long) FuzzerLoop.cpp:619
    #13 0x000104a8cf44 in fuzzer::Fuzzer::RunOne(unsigned char const*, unsigned long, bool, fuzzer::InputInfo*, bool, bool*) FuzzerLoop.cpp:516
    #14 0x000104a8f238 in fuzzer::Fuzzer::ReadAndExecuteSeedCorpora(std::__1::vector<fuzzer::SizedFile, std::__1::allocator<fuzzer::SizedFile>>&) FuzzerLoop.cpp:817
    #15 0x000104a8f754 in fuzzer::Fuzzer::Loop(std::__1::vector<fuzzer::SizedFile, std::__1::allocator<fuzzer::SizedFile>>&) FuzzerLoop.cpp:872
    #16 0x000104a7f410 in fuzzer::FuzzerDriver(int*, char***, int (*)(unsigned char const*, unsigned long)) FuzzerDriver.cpp:923
    #17 0x000104aa9e00 in main FuzzerMain.cpp:20
    #18 0x00018d1abda0 in start+0x1b4c (dyld:arm64e+0x1fda0)

NOTE: libFuzzer has rudimentary signal handlers.
      Combine libFuzzer with AddressSanitizer or similar for better crash reports.
SUMMARY: libFuzzer: deadly signal
MS: 0 ; base unit: 0000000000000000000000000000000000000000
0xa,
\012
artifact_prefix='/Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/RDKWindowManager/de2cb0f01c001cd1_'; Test unit written to /Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/RDKWindowManager/de2cb0f01c001cd1_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
Base64: Cg==
```
- root cause explanation: Memory safety fault reached in an in-scope production stack frame.
- why crash occurred: Mutated input exercised an unsafe code path.
- suggested fix: Add input validation and ownership-safe bounds checks around failing path.

### PackageManagerImplementation::Deinitialize
- source file: PackageManager/PackageManagerImplementation.cpp
- line number: 158
- crash type: sanitizer
- triggering input: fuzz/stats/crashes/PackageManager/eded3db3134dcf48_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
- sanitizer output:
```
.dylib:arm64+0x7b9f8)
    #8 0x000104e02780 in __ubsan_handle_type_mismatch_v1_abort+0x28 (libclang_rt.asan_osx_dynamic.dylib:arm64+0x92780)
    #9 0x000104620c18 in WPEFramework::Plugin::PackageManagerImplementation::Deinitialize(WPEFramework::PluginHost::IShell*) PackageManagerImplementation.cpp:165
    #10 0x0001044ef288 in fuzzauto::RunTargetApi(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&, fuzzauto::InputReader&) fuzz_harness_common.h
    #11 0x0001044edcd0 in fuzzauto::FuzzHarnessRunForTarget(char const*, unsigned char const*, unsigned long) fuzz_harness_common.h:389
    #12 0x00010450d418 in fuzzer::Fuzzer::ExecuteCallback(unsigned char const*, unsigned long) FuzzerLoop.cpp:619
    #13 0x00010450cba8 in fuzzer::Fuzzer::RunOne(unsigned char const*, unsigned long, bool, fuzzer::InputInfo*, bool, bool*) FuzzerLoop.cpp:516
    #14 0x00010450ee9c in fuzzer::Fuzzer::ReadAndExecuteSeedCorpora(std::__1::vector<fuzzer::SizedFile, std::__1::allocator<fuzzer::SizedFile>>&) FuzzerLoop.cpp:817
    #15 0x00010450f3b8 in fuzzer::Fuzzer::Loop(std::__1::vector<fuzzer::SizedFile, std::__1::allocator<fuzzer::SizedFile>>&) FuzzerLoop.cpp:872
    #16 0x0001044ff074 in fuzzer::FuzzerDriver(int*, char***, int (*)(unsigned char const*, unsigned long)) FuzzerDriver.cpp:923
    #17 0x000104529a64 in main FuzzerMain.cpp:20
    #18 0x00018d1abda0 in start+0x1b4c (dyld:arm64e+0x1fda0)

NOTE: libFuzzer has rudimentary signal handlers.
      Combine libFuzzer with AddressSanitizer or similar for better crash reports.
SUMMARY: libFuzzer: deadly signal
MS: 0 ; base unit: 0000000000000000000000000000000000000000
0xa,
\012
artifact_prefix='/Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/PackageManager/eded3db3134dcf48_'; Test unit written to /Users/mthiru270@apac.comcast.com/fuzz7/entservices-appmanagers/fuzz/stats/crashes/PackageManager/eded3db3134dcf48_crash-adc83b19e793491b1c6ea0fd8b46cd9f32e592fc
Base64: Cg==
```
- root cause explanation: Memory safety fault reached in an in-scope production stack frame.
- why crash occurred: Mutated input exercised an unsafe code path.
- suggested fix: Add input validation and ownership-safe bounds checks around failing path.
