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

add_subdirectory(external/glfw)

add_library(glad
    external/glad/src/glad.c
)

target_include_directories(glad PUBLIC
    external/glad/include
)

add_library(imgui
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    external/imgui
    external/imgui/backends
)

target_link_libraries(imgui PUBLIC
    glfw
    glad
)

add_subdirectory(external/lib6502)
add_subdirectory(lib)

file(GLOB_RECURSE NEX_SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
)

add_executable(nex-app
    ${NEX_SOURCES}
)

target_include_directories(nex-app PRIVATE
    ${PROJECT_SOURCE_DIR}/external/imgui-filebrowser
)

target_link_libraries(nex-app
    PRIVATE
        nex
        glfw
        glad
        imgui
)

target_compile_features(nex-app
    PRIVATE
        cxx_std_20
)

target_compile_options(nex-app
    PRIVATE
        -g
        -O0
        -Wall
        -Wextra
        -Wpedantic
)

set_target_properties(nex-app PROPERTIES
    OUTPUT_NAME nex
)
