cmake_minimum_required(VERSION 3.10)
project(HFTAlgo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(inc)

file(GLOB SOURCES
    src/*.cpp
)

add_executable(hft_demo ${SOURCES})

