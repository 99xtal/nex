cmake_minimum_required(VERSION 3.20)

project(nex
    VERSION 0.1.0
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(external/lib6502)
add_subdirectory(lib)
add_subdirectory(cli)
add_subdirectory(debugger)
