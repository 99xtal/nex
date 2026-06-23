cmake_minimum_required(VERSION 3.16)

project(nex C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_subdirectory(external/lib6502)
add_subdirectory(external/SDL)

file(GLOB NEX_SOURCES CONFIGURE_DEPENDS
    src/*.c
)

add_executable(nex
    ${NEX_SOURCES}
)

target_include_directories(nex PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(nex PRIVATE
    6502
    SDL3::SDL3
)

target_compile_options(nex PRIVATE
    -g
    -O0
    -Wall
    -Wextra
    -Wpedantic
)