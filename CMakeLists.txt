cmake_minimum_required(VERSION 3.31.6)
project(pack)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

pkg_check_modules(BLAKE3 REQUIRED libblake3)
pkg_check_modules(SODIUM REQUIRED libsodium)

include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

include_directories(${BLAKE3_INCLUDE_DIRS})
link_directories(${BLAKE3_LIBRARY_DIRS})

enable_testing()
add_library(key libs/include/KeyManager.hpp libs/src/KeyManager.cpp)
add_library(sync libs/include/Sync.hpp libs/src/Sync.cpp)
add_library(k libs/include/Pack.hpp libs/src/Pack.cpp)
target_link_libraries(k ${SODIUM_LIBRARIES})
add_executable(pack apps/pack/main.cpp)

target_link_libraries(pack k sync key ${BLAKE3_LIBRARIES} ${SODIUM_LIBRARIES})

install(TARGETS pack DESTINATION bin)
install(TARGETS k sync key DESTINATION libs)
install(FILES libs/include/Pack.hpp libs/include/KeyManager.hpp libs/include/Sync.hpp DESTINATION include)

add_executable(pack_test tests/main_test.cpp)
target_link_libraries(pack_test GTest::gtest GTest::gtest_main k sync key ${BLAKE3_LIBRARIES})

include(GoogleTest)
gtest_discover_tests(pack_test)
