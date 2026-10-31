cmake_minimum_required(VERSION 3.20)
project(riscv-disasm-trace
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Флаги предупреждений компилятора
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Основная библиотека эмулятора и декодера
add_library(riscv_core
    src/decoder.cpp
    src/disassembler.cpp
    src/emulator.cpp
)

target_include_directories(riscv_core PUBLIC include)

# Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

# Для корректной линковки на Windows с динамическим CRT
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_subdirectory(tests)
add_subdirectory(examples)
