file(GLOB LIBNEX_SOURCES CONFIGURE_DEPENDS
    src/*.c
)

add_library(nex STATIC
    ${LIBNEX_SOURCES}
)

target_include_directories(nex
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(nex
    PUBLIC
        6502
)

target_compile_features(nex PUBLIC c_std_11)