cmake_minimum_required(VERSION 3.6)
project(matan)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

set(SOURCE_FILES main.cpp)
add_executable(matan ${SOURCE_FILES})
