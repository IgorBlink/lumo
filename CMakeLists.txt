cmake_minimum_required(VERSION 3.10)
project(LumoCompiler VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add include directory
include_directories(include)

# Gather source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Main executable
add_executable(lumo ${SOURCES})
