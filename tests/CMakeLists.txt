add_executable(riscv_tests
    test_decoder.cpp
    test_emulator.cpp
)

target_link_libraries(riscv_tests
    PRIVATE
        riscv_core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(riscv_tests)
