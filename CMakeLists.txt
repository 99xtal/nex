cmake_minimum_required(VERSION 3.20)

project(nex
    VERSION 0.1.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(NEX_BUILD_CLI "Build the nex CLI frontend" ON)

add_subdirectory(external/lib6502)
add_subdirectory(lib)
add_subdirectory(cli)
