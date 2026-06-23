add_compile_options("-fsanitize=address,fuzzer-no-link")
add_link_options("-fsanitize=address,fuzzer-no-link")


add_executable(archimedes_fuzz_tests
    kernel_fuzz.cpp
)

target_link_libraries(archimedes_fuzz_tests
    PRIVATE
    archimedes_core
    fuzztest::fuzztest            # The main logic
    fuzztest::fuzztest_gtest_main # Essential for GTest integration
    GTest::gtest                  # Standard GTest
)

target_compile_definitions(archimedes_fuzz_tests PRIVATE FUZZTEST_COMPILATION)
