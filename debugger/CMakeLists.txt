file(GLOB_RECURSE NEX_DEBUGGER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(nex-debug
    ${NEX_DEBUGGER_SOURCES}
)

target_link_libraries(nex-debug
    PRIVATE
        nex
        glfw
        glad
        imgui
)

target_compile_features(nex-debug
    PRIVATE
        cxx_std_20
)

target_compile_options(nex-debug
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

set_target_properties(nex-debug PROPERTIES
    OUTPUT_NAME nex-debug
)