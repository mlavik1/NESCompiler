cmake_minimum_required(VERSION 3.3)
project(CNES)

# Gather c++ files
file(GLOB_RECURSE SRC_FILES 
    src/*.cpp
    src/*.h
)

add_executable(CNES ${SRC_FILES})
