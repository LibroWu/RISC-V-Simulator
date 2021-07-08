cmake_minimum_required(VERSION 3.16)
project(code)

set(CMAKE_CXX_STANDARD 14)

add_executable(code src/main.cpp src/binaryManager.cpp src/Operator.cpp src/riscVRunByOrder.cpp)