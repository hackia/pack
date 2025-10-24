cmake_minimum_required(VERSION 3.31.6)
project(pack)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(GCC_WARNING_FLAGS
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wformat=2
        -Wconversion
        -Wsign-conversion
        -Werror
)
find_package(PkgConfig REQUIRED)
pkg_check_modules(BLAKE3 REQUIRED libblake3)
pkg_check_modules(ASIO REQUIRED asio)
pkg_check_modules(SODIUM REQUIRED libsodium)
include_directories(${BLAKE3_INCLUDE_DIRS})
link_directories(${BLAKE3_LIBRARY_DIRS})

add_library(kpack STATIC libs/src/Pack.cpp libs/include/Pack.hpp)
target_compile_options(kpack PRIVATE ${GCC_WARNING_FLAGS})

add_library(ksync STATIC libs/src/Sync.cpp libs/include/Sync.hpp)
target_compile_options(ksync PRIVATE ${GCC_WARNING_FLAGS})

add_library(sender STATIC libs/src/Sender.cpp libs/include/Sender.hpp)
target_compile_options(sender PRIVATE ${GCC_WARNING_FLAGS})

add_library(keys STATIC libs/src/KeyManager.cpp libs/include/KeyManager.hpp)
target_compile_options(keys PRIVATE ${GCC_WARNING_FLAGS})

add_executable(sync apps/sync/main.cpp)
target_compile_options(sync PRIVATE ${GCC_WARNINGS_FLAGS})

add_executable(k apps/k/main.cpp)
target_compile_options(k PRIVATE ${GCC_WARNINGS_FLAGS})

add_executable(pack apps/pack/main.cpp)
target_compile_options(pack PRIVATE ${GCC_WARNINGS_FLAGS})

install(TARGETS k pack sync DESTINATION bin)
install(FILES libs/include/KeyManager.hpp libs/include/Pack.hpp libs/include/Sync.hpp DESTINATION include)
install(TARGETS kpack keys ksync DESTINATION lib)
