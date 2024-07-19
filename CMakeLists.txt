cmake_minimum_required(VERSION 3.21)

project(hdag VERSION 1 DESCRIPTION "Hash DAG database library and tools")
include(CTest)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

include_directories(include)

add_compile_options(
    -Wall -Wextra -Wpedantic
    -Werror
)

add_definitions(-D_GNU_SOURCE)

add_library(
    hdag
    lib/hdag/bundle.c
    lib/hdag/file.c
    lib/hdag/node_seq.c
    lib/hdag/nodes.c
    lib/hdag/node.c
)

add_executable(hdagt-file src/hdagt/hdagt-file.c)
target_link_libraries(hdagt-file hdag)
add_test(NAME file COMMAND hdagt-file)

set_target_properties(hdag PROPERTIES VERSION ${PROJECT_VERSION})

install(
    TARGETS hdag
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
