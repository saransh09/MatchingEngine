cmake_minimum_required(VERSION 3.20)

# -----------------------------------------------------------------------------
# Compiler Selection (must be before project())
# Use Homebrew LLVM on macOS if available
# -----------------------------------------------------------------------------
if(NOT DEFINED CMAKE_C_COMPILER AND NOT DEFINED CMAKE_CXX_COMPILER)
    if(EXISTS /opt/homebrew/opt/llvm/bin/clang)
        set(CMAKE_C_COMPILER /opt/homebrew/opt/llvm/bin/clang)
        set(CMAKE_CXX_COMPILER /opt/homebrew/opt/llvm/bin/clang++)
    elseif(EXISTS /usr/local/opt/llvm/bin/clang)
        # Intel Mac Homebrew location
        set(CMAKE_C_COMPILER /usr/local/opt/llvm/bin/clang)
        set(CMAKE_CXX_COMPILER /usr/local/opt/llvm/bin/clang++)
    endif()
endif()


project(MatchingEngine LANGUAGES CXX)

option(ENABLE_TESTING "Enable testing support" OFF)
option(ENABLE_BENCHMARKING "ENABLE BENCHMARKING" OFF)
option(ENABLE_LOGGING "Enable profiling instrumentation" ON)

message(STATUS "ENABLE_TESTING: ${ENABLE_TESTING}")
message(STATUS "ENABLE_BENCHMARKING ${ENABLE_BENCHMARKING}")
message(STATUS "ENABLE_PROFILING ${ENABLE_PROFILING}")


# find_package(TBB REQUIRED)

# -----------------------------------------------------------------------------
# C++ Standard
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------------------------------------------------------
# Build Settings
# -----------------------------------------------------------------------------
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# -----------------------------------------------------------------------------
# Compiler Warnings (important for concurrent code!)
# -----------------------------------------------------------------------------
add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE
    $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:
        -Wall -Wextra -Wpedantic -Werror
        -Wconversion -Wsign-conversion
        -Wnon-virtual-dtor -Wold-style-cast
        -Wcast-align -Wunused -Woverloaded-virtual
        -Wshadow -Wformat=2
    >
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4 /WX
    >
)

# -----------------------------------------------------------------------------
# Sanitizers (crucial for concurrent data structures)
# Usage: cmake -DSANITIZER=thread ..
# -----------------------------------------------------------------------------
set(SANITIZER "" CACHE STRING "Enable sanitizer (thread, address, undefined)")
if(SANITIZER)
    message(STATUS "Enabling ${SANITIZER} sanitizer")
    add_compile_options(-fsanitize=${SANITIZER} -fno-omit-frame-pointer)
    add_link_options(-fsanitize=${SANITIZER})
endif()

# -----------------------------------------------------------------------------
# Platform-specific linker flags (for Homebrew LLVM on macOS)
# -----------------------------------------------------------------------------
if(APPLE AND EXISTS /opt/homebrew/opt/llvm)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/opt/homebrew/opt/llvm/lib/c++ -Wl,-rpath,/opt/homebrew/opt/llvm/lib/c++")
endif()

# -----------------------------------------------------------------------------
# Options
# -----------------------------------------------------------------------------
option(BUILD_TESTS "Build unit tests" ON)


# -----------------------------------------------------------------------------
# Vendor Libraries
# -----------------------------------------------------------------------------
add_subdirectory(vendor)
