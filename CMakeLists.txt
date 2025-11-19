cmake_minimum_required(VERSION 3.10)
project(smin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_executable(smin ./src/B11330203.cpp)

# add_executable(kisstodot ./src/kisstodot.cpp)
