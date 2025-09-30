cmake_minimum_required(VERSION 4.0)
project(pack)

set(CMAKE_CXX_STANDARD 26)

add_executable(pack main.cpp)

install(TARGETS pack DESTINATION bin)
