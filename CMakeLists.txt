cmake_minimum_required(VERSION 3.20)
project(GitClient CXX)

# --- Language settings ---
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)   # error out rather than silently downgrade
set(CMAKE_CXX_EXTENSIONS ON)         # -std=c++23, not -std=gnu++23

# --- Warnings, applied via an interface target we can attach to anything ---
add_library(warnings INTERFACE)
target_compile_options(warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion -Wshadow
)

# --- Sanitizers in Debug builds only ---
add_library(sanitizers INTERFACE)
target_compile_options(sanitizers INTERFACE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined -fno-omit-frame-pointer>
)
target_link_options(sanitizers INTERFACE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)

# --- The library: all real logic lives here ---
add_library(gitcore STATIC
    src/GitClient/repository.cpp
    src/GitClient/sha1.cpp
    src/GitClient/object_store.cpp
)
target_include_directories(gitcore PUBLIC src)
target_link_libraries(gitcore PUBLIC warnings sanitizers)

# --- The CLI executable: just argv parsing + dispatch ---
add_executable(GitClient src/main.cpp)
target_link_libraries(GitClient PRIVATE gitcore)

# --- Tests: doctest fetched automatically at configure time ---
include(FetchContent)
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
)
FetchContent_MakeAvailable(doctest)

add_executable(tests 
    tests/test_repository.cpp 
    tests/test_sha1.cpp
    tests/test_object_store.cpp
)
target_link_libraries(tests PRIVATE gitcore doctest::doctest)
target_link_libraries(tests PRIVATE gitcore doctest::doctest)

enable_testing()
add_test(NAME unit_tests COMMAND tests)