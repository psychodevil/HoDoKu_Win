cmake_minimum_required(VERSION 3.20)
project(HoDoKu_Win
    VERSION 2.2.0
    DESCRIPTION "High-performance native C++20 Sudoku Studio replicating HoDoKu"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings and optimization with SIMD vectorization
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8 /O2 /arch:AVX2)
else()
    add_compile_options(-Wall -Wextra -O3 -mavx2 -mfma -mpopcnt)
endif()

include_directories(src)

# Core library (header-only algorithms)
add_library(hodoku_core INTERFACE)
target_include_directories(hodoku_core INTERFACE src)

# Test Suite Executables
add_executable(test_core tests/test_core.cpp)
target_link_libraries(test_core PRIVATE hodoku_core)

if(WIN32)
    add_executable(test_file_io tests/test_file_io.cpp)
    target_link_libraries(test_file_io PRIVATE hodoku_core gdiplus comctl32 gdi32 user32 comdlg32)

    add_executable(test_tutor tests/test_tutor.cpp)
    target_link_libraries(test_tutor PRIVATE hodoku_core gdiplus comctl32 gdi32 user32 comdlg32)
endif()

enable_testing()
add_test(NAME CoreEngineTests COMMAND test_core)
if(WIN32)
    add_test(NAME FileIoTests COMMAND test_file_io)
    add_test(NAME TutorTests COMMAND test_tutor)
endif()

# Native Windows GUI Executable
if(WIN32)
    add_executable(hodoku_native WIN32 src/app/main.cpp src/app/resources.rc)
    target_link_libraries(hodoku_native PRIVATE
        hodoku_core
        gdiplus
        comctl32
        gdi32
        user32
        comdlg32
    )
endif()

