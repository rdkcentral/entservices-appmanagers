# Fuzz Report - PreinstallManager

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
