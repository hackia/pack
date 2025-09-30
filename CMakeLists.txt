cmake_minimum_required(VERSION 3.31.6)
project(pack)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(PkgConfig REQUIRED)
pkg_check_modules(BLAKE3 REQUIRED libblake3)

include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

include_directories(${BLAKE3_INCLUDE_DIRS})
link_directories(${BLAKE3_LIBRARY_DIRS})

enable_testing()
add_library(hash libs/include/Hash.hpp libs/src/Hash.cpp)
add_library(sync libs/include/Sync.hpp libs/src/Sync.cpp)
add_library(k libs/include/Pack.hpp libs/src/Pack.cpp)

add_executable(pack apps/pack/main.cpp)
target_link_libraries(pack k sync ${BLAKE3_LIBRARIES})

install(TARGETS pack DESTINATION bin)
install(TARGETS k DESTINATION libs)
install(FILES libs/include/Pack.hpp DESTINATION include)

add_executable(pack_test tests/main_test.cpp)
target_link_libraries(pack_test GTest::gtest GTest::gtest_main k ${BLAKE3_LIBRARIES})

include(GoogleTest)
gtest_discover_tests(pack_test)
