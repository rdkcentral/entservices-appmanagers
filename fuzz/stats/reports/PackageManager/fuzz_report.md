# Fuzz Report - PackageManager

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
