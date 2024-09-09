cmake_minimum_required(VERSION 3.10)

# Set the project name
project(msgpack-parse)

find_package(msgpack-cxx REQUIRED)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the executable
add_executable(msgpack-parse src/main.cpp)

target_link_libraries(msgpack-parse PUBLIC msgpack-cxx)
