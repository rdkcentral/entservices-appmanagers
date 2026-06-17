# Fuzz Report - RDKWindowManager

Generated: 2026-06-14 09:21:52 UTC

## Summary
- total functions fuzzed: 639
- total seeds generated: 35231
- total executions: 1278000
- crashes found: 1
- ignored false positives: 0
- coverage summary: discovered targets=639, coverage gaps=199

## Applied Budgets
- RUNS: 2000
- MAX_TOTAL_TIME: 0
- PAIRWISE_BUDGET: 48
- STATE_SEQUENCE_SEEDS: 20
- MAX_JSON_SEEDS: 40

## Valid Crashes

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
