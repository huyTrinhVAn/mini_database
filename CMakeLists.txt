cmake_minimum_required(VERSION 3.16)
project(kvstore CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

if(MINGW)
    # MSYS2 ucrt64's dynamic libstdc++ link fails with ld error 116 in this
    # environment; static linking sidesteps it and yields a standalone exe.
    add_link_options(-static)
endif()

include_directories(src)

add_executable(kv_demo src/main.cpp)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(hash_table_test tests/hash_table_test.cpp)
target_link_libraries(hash_table_test GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(hash_table_test)
