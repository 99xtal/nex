file(GLOB_RECURSE NEX_CLI_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
)

add_executable(nex-cli ${NEX_CLI_SOURCES})

target_link_libraries(nex-cli
    PRIVATE
        nex
)

target_compile_features(nex-cli
    PRIVATE
        c_std_11
)

target_compile_options(nex-cli
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

set_target_properties(nex-cli PROPERTIES
    OUTPUT_NAME nex
)