cmake_minimum_required(VERSION 3.10)
project(smin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(smin ./src/main.cpp)

add_executable(kisstodot ./src/kisstodot.cpp)
